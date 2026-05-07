// load elimination with intra-block reuse and inter-block phi-based reuse
#include <cassert>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {

// state
static bool g_modified = false;
static ir::opt_support::PointerBaseInfo *g_ptr_base_info = nullptr;
static ir::opt_support::AliasInfo *g_alias_info = nullptr;

// helpers
struct CanonPtr {
    std::shared_ptr<ir::Value> base;
    std::vector<int> indices;
    bool valid{false};
};

static std::optional<CanonPtr> canonicalize_pointer(std::shared_ptr<ir::Value> ptr) {
    CanonPtr res{};
    res.valid = true;
    // unwrap bitcasts
    while (auto bc = std::dynamic_pointer_cast<ir::Bitcast>(ptr)) {
        ptr = bc->val();
    }
    // collect constant indices across nested GEPs
    std::vector<int> reversed;
    auto cur = ptr;
    while (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(cur)) {
        auto idxs = gep->get_indexes();
        for (int i = static_cast<int>(idxs.size()) - 1; i >= 0; --i) {
            if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(idxs[static_cast<size_t>(i)])) {
                reversed.push_back(c->get_val());
            } else {
                return std::nullopt;
            }
        }
        cur = gep->base_ptr();
        while (auto bc2 = std::dynamic_pointer_cast<ir::Bitcast>(cur)) cur = bc2->val();
    }
    res.base = cur;
    res.indices.assign(reversed.rbegin(), reversed.rend());
    return res;
}

static bool canon_equal_ptr(std::shared_ptr<ir::Value> a, std::shared_ptr<ir::Value> b) {
    if (a == b) return true;
    auto ca = canonicalize_pointer(a);
    auto cb = canonicalize_pointer(b);
    if (!ca.has_value() || !cb.has_value()) return false;
    return ca->base == cb->base && ca->indices == cb->indices;
}

static std::optional<std::shared_ptr<ir::Value>> map_lookup_equiv(
    const std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> &m,
    std::shared_ptr<ir::Value> key) {
    if (auto it = m.find(key); it != m.end()) return it->second;
    for (const auto &kv : m) {
        if (canon_equal_ptr(kv.first, key)) return kv.second;
    }
    return std::nullopt;
}

static void map_erase_equiv(std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> &m,
                            std::shared_ptr<ir::Value> key) {
    for (auto it = m.begin(); it != m.end();) {
        if (it->first == key || canon_equal_ptr(it->first, key))
            it = m.erase(it);
        else
            ++it;
    }
}
static inline bool call_may_write_memory(const std::shared_ptr<ir::Call> &call) {
    if (!call) return true;
    auto callee = call->get_function();
    if (!callee) return true;
    const auto &info = callee->opt_info;
    // only memory writes are barriers; pure or side-effect-only calls without writes are not
    if (info.is_pure) return false;
    return info.has_global_memory_write || info.has_local_memory_write;
}
static inline bool is_terminator(const std::shared_ptr<ir::Instruction> &inst) {
    using IT = ir::Instruction::InstructionType;
    const auto ty = inst->get_ins_type();
    return ty == IT::RET || ty == IT::BR;
}

static bool defined_earlier_in_block(const std::shared_ptr<ir::Instruction> &candidate,
                                     const std::shared_ptr<ir::Instruction> &use_site,
                                     const std::shared_ptr<ir::BasicBlock> &block) {
    if (!candidate || !use_site) return false;
    if (candidate->get_parent_block().lock() != block) return false;
    bool seen_candidate = false;
    for (const auto &ins : block->get_instructions_ref()) {
        if (ins == candidate) {
            seen_candidate = true;
        }
        if (ins == use_site) {
            return seen_candidate;
        }
    }
    return false;
}

// Intra-block: replace loads with the last known value for the same pointer (exact address)
static bool run_intra_block(std::shared_ptr<ir::BasicBlock> block) {
    bool modified = false;
    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> last_value_of_ptr;

    // iterate over a copy to avoid iterator invalidation when substituting/removing instructions
    auto instructions_copy = block->get_instructions_ref();
    for (const auto &inst : instructions_copy) {
        if (auto load = std::dynamic_pointer_cast<ir::Load>(inst)) {
            auto ptr = load->addr();
            if (auto rep = map_lookup_equiv(last_value_of_ptr, ptr)) {
                auto replacement = rep.value();
                // safety: only substitute if replacement dominates this load within the same block,
                // or replacement is not an instruction (e.g., constant or argument)
                auto repl_inst = std::dynamic_pointer_cast<ir::Instruction>(replacement);
                if (!repl_inst || defined_earlier_in_block(repl_inst, load, block)) {
                    modified = true;
                    util::substitute(load, replacement);
                } else {
                    // do not substitute across blocks or with later instructions
                    last_value_of_ptr[ptr] = load;
                }
            } else {
                last_value_of_ptr[ptr] = load;
            }
        } else if (auto store = std::dynamic_pointer_cast<ir::Store>(inst)) {
            auto addr = store->addr();
            // if store address has non-constant index (canonicalization fails), clear whole mapping conservatively
            if (!canonicalize_pointer(addr)) {
                last_value_of_ptr.clear();
                continue;
            }
            auto val = store->val();
            auto val_inst = std::dynamic_pointer_cast<ir::Instruction>(val);
            // Only record load instructions, not constants or other values
            // This prevents aggressive constant propagation which should be handled by dedicated passes
            if (val_inst && std::dynamic_pointer_cast<ir::Load>(val_inst) &&
                defined_earlier_in_block(val_inst, std::dynamic_pointer_cast<ir::Instruction>(inst), block)) {
                last_value_of_ptr[addr] = val;
            } else {
                // Clear mapping for this address
                map_erase_equiv(last_value_of_ptr, addr);
            }
        } else if (auto ms = std::dynamic_pointer_cast<ir::Memset>(inst)) {
            // treat memset as write to its pointer: clear mapping conservatively
            // alternatively, remove only that base if desired
            (void)ms;  // suppress unused warning if needed
            last_value_of_ptr.clear();
        } else if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
            if (call_may_write_memory(call)) {
                last_value_of_ptr.clear();
            }
        }
    }

    return modified;
}

// compute last map for a block (for reuse in inter-block)
static std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> compute_block_last_map(
    const std::shared_ptr<ir::BasicBlock> &block) {
    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> m;
    for (const auto &inst : block->get_instructions_ref()) {
        if (auto load = std::dynamic_pointer_cast<ir::Load>(inst)) {
            m[load->addr()] = load;
        } else if (auto store = std::dynamic_pointer_cast<ir::Store>(inst)) {
            auto val = store->val();
            auto val_inst = std::dynamic_pointer_cast<ir::Instruction>(val);
            // For inter-block, record all values that dominate (including constants)
            // This is needed for PHI creation
            if (!val_inst ||
                defined_earlier_in_block(val_inst, std::dynamic_pointer_cast<ir::Instruction>(inst), block)) {
                m[store->addr()] = val;
            } else {
                // value not dominating here, do not record
                map_erase_equiv(m, store->addr());
            }
        } else if (std::dynamic_pointer_cast<ir::Call>(inst)) {
            m.clear();
        }
    }
    return m;
}

// Inter-block: at merge blocks, reuse predecessor last values via phi; create per-pred indirect blocks for misses
static bool run_inter_block(std::shared_ptr<ir::BasicBlock> block) {
    // collect candidate loads in this block until a barrier
    std::vector<std::shared_ptr<ir::Load>> candidate_loads;
    std::vector<std::shared_ptr<ir::Value>> seen_store_ptrs;

    for (const auto &inst : block->get_instructions_ref()) {
        if (auto load = std::dynamic_pointer_cast<ir::Load>(inst)) {
            auto load_ptr = load->addr();
            // allow constant-index GEP produced locally if safe: def dominates use and no writes to same base
            // in-between
            if (auto ptr_inst = std::dynamic_pointer_cast<ir::Instruction>(load_ptr)) {
                auto def_block = ptr_inst->get_parent_block().lock();
                auto can = canonicalize_pointer(load_ptr);
                bool allow_local_gep = false;
                if (def_block == block) {
                    if (can && defined_earlier_in_block(ptr_inst, load, block)) {
                        // ensure no writes to same base between def and load in this block
                        bool seen_def = false;
                        bool wrote_base = false;
                        for (const auto &ins2 : block->get_instructions_ref()) {
                            if (ins2 == ptr_inst) {
                                seen_def = true;
                                continue;
                            }
                            if (!seen_def) continue;
                            if (ins2 == load) break;
                            if (auto st = std::dynamic_pointer_cast<ir::Store>(ins2)) {
                                auto sc = canonicalize_pointer(st->addr());
                                if (sc && sc->base == can->base) {
                                    wrote_base = true;
                                    break;
                                }
                            } else if (auto ms = std::dynamic_pointer_cast<ir::Memset>(ins2)) {
                                auto mc = canonicalize_pointer(ms->get_operands()[0]);
                                if (mc && mc->base == can->base) {
                                    wrote_base = true;
                                    break;
                                }
                            }
                        }
                        allow_local_gep = !wrote_base;
                    }
                } else {
                    // pointer defined in other block: require constant-index and def block dominates current block
                    if (can && def_block && def_block->opt_info.dominates(block)) {
                        allow_local_gep = true;
                    }
                }
                if (!allow_local_gep) continue;
            }

            // skip loads that feed branch conditions within this block
            bool feeds_branch = false;
            for (const auto &wu : load->get_users_ref()) {
                auto user = wu.lock();
                auto user_inst = std::dynamic_pointer_cast<ir::Instruction>(user);
                if (!user_inst) continue;
                if (user_inst->get_parent_block().lock() != block) continue;
                auto ty = user_inst->get_ins_type();
                if (ty == ir::Instruction::InstructionType::ICMP || ty == ir::Instruction::InstructionType::FCMP) {
                    for (const auto &wuu : user_inst->get_users_ref()) {
                        auto u2 = wuu.lock();
                        auto u2_inst = std::dynamic_pointer_cast<ir::Instruction>(u2);
                        if (!u2_inst) continue;
                        if (u2_inst->get_parent_block().lock() != block) continue;
                        if (u2_inst->get_ins_type() == ir::Instruction::InstructionType::BR) {
                            feeds_branch = true;
                            break;
                        }
                    }
                    if (feeds_branch) break;
                }
            }
            if (feeds_branch) continue;

            bool may_be_overridden = false;
            for (const auto &store_ptr : seen_store_ptrs) {
                if (!g_alias_info->is_distinct(store_ptr, load_ptr)) {
                    may_be_overridden = true;
                    break;
                }
            }
            if (!may_be_overridden) candidate_loads.push_back(load);
        } else if (auto store = std::dynamic_pointer_cast<ir::Store>(inst)) {
            seen_store_ptrs.push_back(store->addr());
        } else if (auto ms = std::dynamic_pointer_cast<ir::Memset>(inst)) {
            // treat memset as a write to its pointer
            auto ptr = ms->get_operands()[0];
            seen_store_ptrs.push_back(ptr);
        } else if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
            if (call_may_write_memory(call)) break;  // barrier
        }
    }

    if (candidate_loads.empty()) return false;

    // gather predecessors
    std::vector<std::shared_ptr<ir::BasicBlock>> predecessors;
    for (const auto &weak_pred : block->opt_info.predecessors) {
        if (auto pred = weak_pred.lock()) predecessors.push_back(pred);
    }
    if (predecessors.empty()) return false;
    // conservative: only handle true merge blocks with multiple predecessors
    if (predecessors.size() < 2) return false;

    // precompute last maps for predecessors to avoid repeated scans
    std::unordered_map<std::shared_ptr<ir::BasicBlock>,
                       std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>>>
        pred_last_map;
    for (const auto &pred : predecessors) pred_last_map[pred] = compute_block_last_map(pred);

    // for each candidate load, find reusable values from predecessors
    std::unordered_map<std::shared_ptr<ir::Load>,
                       std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::shared_ptr<ir::Value>>>
        reuse_values;  // load -> (pred -> value)

    for (const auto &pred : predecessors) {
        for (const auto &load : candidate_loads) {
            auto ptr = load->addr();
            // if this block later stores to the same pointer, skip to avoid interfering with swap-like patterns
            bool will_store_same_ptr = false;
            bool after_load = false;
            for (const auto &ins : block->get_instructions_ref()) {
                if (ins == load) {
                    after_load = true;
                    continue;
                }
                if (!after_load) continue;
                if (auto st = std::dynamic_pointer_cast<ir::Store>(ins)) {
                    if (st->addr() == ptr) {
                        will_store_same_ptr = true;
                        break;
                    }
                }
                if (auto ms = std::dynamic_pointer_cast<ir::Memset>(ins)) {
                    if (ms->get_operands()[0] == ptr) {
                        will_store_same_ptr = true;
                        break;
                    }
                }
            }
            if (will_store_same_ptr) continue;
            auto last = map_lookup_equiv(pred_last_map[pred], ptr);
            if (!last.has_value()) continue;

            // defer reuse if last value is a load unused within pred block (prefer deferred load)
            if (auto last_load = std::dynamic_pointer_cast<ir::Load>(last.value())) {
                bool used_in_block = false;
                for (const auto &weak_user : last_load->get_users_ref()) {
                    if (auto user = weak_user.lock()) {
                        auto user_inst = std::dynamic_pointer_cast<ir::Instruction>(user);
                        if (!user_inst) continue;
                        if (user_inst->get_parent_block().lock() != pred) continue;
                        if (is_terminator(user_inst)) continue;
                        used_in_block = true;
                        break;
                    }
                }
                if (!used_in_block) continue;
            }

            reuse_values[load].emplace(pred, last.value());
        }
    }

    // nothing reusable
    bool any_reuse = false;
    for (const auto &kv : reuse_values) {
        if (!kv.second.empty()) {
            any_reuse = true;
            break;
        }
    }
    if (!any_reuse) return false;

    // create trampolines for predecessors that miss some loads
    std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::shared_ptr<ir::BasicBlock>> pred_to_trampoline;
    for (const auto &pred : predecessors) {
        bool should_create = false;
        for (const auto &load : candidate_loads) {
            if (!reuse_values[load].count(pred)) {
                should_create = true;
                break;
            }
        }
        if (should_create) {
            std::unordered_set<std::shared_ptr<ir::BasicBlock>> one_pred{pred};
            auto trampoline = util::split_block_predecessors(block, one_pred);
            pred_to_trampoline[pred] = trampoline;
        }
    }

    // helper: if all predecessor values are identical (same constant or same SSA), substitute directly
    auto values_identical =
        [](const std::vector<std::shared_ptr<ir::Value>> &vals) -> std::optional<std::shared_ptr<ir::Value>> {
        if (vals.empty()) return std::nullopt;
        auto first = vals.front();
        for (size_t i = 1; i < vals.size(); ++i) {
            if (vals[i] != first) return std::nullopt;
        }
        return first;
    };

    // build phi per candidate load and substitute
    for (const auto &load : candidate_loads) {
        bool all_have = true;
        std::vector<std::shared_ptr<ir::Value>> vals;
        vals.reserve(predecessors.size());
        for (const auto &pred : predecessors) {
            if (!reuse_values[load].count(pred)) {
                all_have = false;
                break;
            }
            vals.push_back(reuse_values[load][pred]);
        }
        if (all_have) {
            if (auto same = values_identical(vals)) {
                util::substitute(load, same.value());
                continue;
            }
        }
        auto phi = ir::Phi::create(block,
                                   std::vector<std::shared_ptr<ir::Value>>{},
                                   std::vector<std::shared_ptr<ir::BasicBlock>>{},
                                   load->get_type(),
                                   gen_local_var_name());
        // insert phi before the first non-phi instruction (at the phi cluster)
        auto &instrs = block->get_instructions_ref();
        auto insert_it = instrs.begin();
        while (insert_it != instrs.end() && std::dynamic_pointer_cast<ir::Phi>(*insert_it)) ++insert_it;
        block->add_instruction(insert_it, phi);

        for (const auto &pred : predecessors) {
            if (reuse_values[load].count(pred)) {
                // If this pred has a trampoline, use the trampoline as the source
                if (pred_to_trampoline.count(pred)) {
                    auto trampoline = pred_to_trampoline.at(pred);
                    util::add_incoming(phi, trampoline, reuse_values[load].at(pred));
                } else {
                    util::add_incoming(phi, pred, reuse_values[load].at(pred));
                }
            } else {
                auto trampoline = pred_to_trampoline.at(pred);
                auto load_on_edge = ir::Load::create(trampoline, load->addr(), gen_local_var_name());
                // insert after existing PHIs in trampoline
                auto &tramp_instrs = trampoline->get_instructions_ref();
                auto it = tramp_instrs.begin();
                while (it != tramp_instrs.end() && std::dynamic_pointer_cast<ir::Phi>(*it)) ++it;
                trampoline->add_instruction(it, load_on_edge);
                util::add_incoming(phi, trampoline, load_on_edge);
            }
        }

        util::substitute(load, phi);
    }

    return true;
}

bool LoadElimination::run(ir::Module *module) {
    g_modified = false;
    g_ptr_base_info = &module->opt_info.pointer_base_info;
    g_alias_info = &module->opt_info.alias_info;
    module->for_each_func([&](auto &func) {
        // intra-block across all blocks first
        func->for_each_block([&](auto &block) { g_modified |= run_intra_block(block); });

        // inter-block: process one transformation at a time (let pipeline iterate)
        for (auto &block : func->get_basic_blocks_ref()) {
            if (run_inter_block(block)) {
                g_modified = true;
                // assert(false);
                return;  // early exit after one round like cmmc
            }
        }
    });

    return g_modified;
}

}  // namespace opt::pass
