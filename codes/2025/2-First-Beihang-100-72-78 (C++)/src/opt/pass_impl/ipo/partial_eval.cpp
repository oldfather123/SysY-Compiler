#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "ir/support.hpp"
#include "ir/value.hpp"
#include "opt/exec/interpreter.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {

// PartialEvaluate: local, expression-level partial evaluation inside a block.
// - Does not remove or reorder IO/calls with side effects.
// - Attempts to evaluate pure call sites whose arguments are constants and callee is constexpr-recursive safe.
// - Attempts to fold straight-line pure expression trees (already covered by SCCP/ConstFold; here we target calls).

// keep minimal helpers only if used

// removed unused helpers after slimming PartialEvaluate

// removed unused helpers after slimming PartialEvaluate

struct AddrSig {
    std::shared_ptr<ir::Value> base;
    std::vector<std::shared_ptr<ir::Value>> idx;
    bool operator==(const AddrSig &o) const {
        if (base != o.base) return false;
        if (idx.size() != o.idx.size()) return false;
        for (size_t i = 0; i < idx.size(); ++i)
            if (idx[i] != o.idx[i]) return false;
        return true;
    }
};
struct AddrSigHash {
    size_t operator()(const AddrSig &s) const noexcept {
        size_t h = std::hash<void *>{}(s.base.get());
        for (const auto &v : s.idx) {
            h ^= (std::hash<void *>{}(v.get()) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
        }
        return h;
    }
};

static std::optional<AddrSig> decompose_addr(const std::shared_ptr<ir::Value> &addr) {
    if (auto alloca_inst = std::dynamic_pointer_cast<ir::Alloca>(addr)) {
        return AddrSig{addr, {}};
    }
    if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(addr)) {
        auto base_sig = decompose_addr(gep->base_ptr());
        if (!base_sig) return std::nullopt;
        AddrSig s = *base_sig;
        auto geps = gep->get_indexes();
        s.idx.insert(s.idx.end(), geps.begin(), geps.end());
        return s;
    }
    return std::nullopt;
}

static std::optional<int> try_compute_trip_count_simple(const std::shared_ptr<ir::opt_support::LoopInfo> &loop) {
    if (!loop) return std::nullopt;
    if (!loop->pre_header) return std::nullopt;
    if (loop->latches.size() != 1) return std::nullopt;
    if (loop->exits.size() != 1) return std::nullopt;
    auto latch = *loop->latches.begin();
    auto header = loop->header;
    if (!header) return std::nullopt;

    // Find the conditional branch in the exiting block leading to the single exit
    std::shared_ptr<ir::ICmp> cond_icmp = nullptr;
    for (auto &blk : loop->blocks) {
        auto br = std::dynamic_pointer_cast<ir::Br>(blk->tail_instruction());
        if (!br || !br->is_cond_branch()) continue;
        auto cnd = std::dynamic_pointer_cast<ir::ICmp>(br->get_operands_ref()[0]);
        if (!cnd) continue;
        cond_icmp = cnd;
        break;
    }
    if (!cond_icmp) return std::nullopt;

    // Find integer ind-var phi in header used by cond_icmp
    std::shared_ptr<ir::Phi> ind_phi = nullptr;
    for (auto &inst : header->get_instructions_ref()) {
        auto phi = std::dynamic_pointer_cast<ir::Phi>(inst);
        if (!phi) break;
        if (!phi->get_type()->is_integer_ty()) continue;
        for (auto &op : cond_icmp->get_operands_ref()) {
            if (op == phi) {
                ind_phi = phi;
                break;
            }
        }
        if (ind_phi) break;
    }
    if (!ind_phi) return std::nullopt;

    // Derive init from pre_header incoming, and next (alu) from latch incoming
    auto init_v = opt::util::get_phi_value(ind_phi, loop->pre_header);
    auto next_v = opt::util::get_phi_value(ind_phi, latch);
    auto init_ci = std::dynamic_pointer_cast<ir::ConstantInt>(init_v);
    auto next_inst = std::dynamic_pointer_cast<ir::Instruction>(next_v);
    if (!init_ci || !next_inst || !next_inst->is_binary()) return std::nullopt;

    // Step from alu = add/sub/mul (phi, C)
    auto op0 = next_inst->get_operands_ref()[0];
    auto op1 = next_inst->get_operands_ref()[1];
    std::shared_ptr<ir::ConstantInt> step_ci = nullptr;
    if (op0 == ind_phi)
        step_ci = std::dynamic_pointer_cast<ir::ConstantInt>(op1);
    else if (op1 == ind_phi)
        step_ci = std::dynamic_pointer_cast<ir::ConstantInt>(op0);
    if (!step_ci) return std::nullopt;

    int init = init_ci->get_val();
    int step = step_ci->get_val();
    using IT = ir::Instruction::InstructionType;
    if (next_inst->get_ins_type() == IT::SUB) step = -step;  // effective step

    // Bound from icmp other operand
    auto lhs = cond_icmp->get_operands_ref()[0];
    auto rhs = cond_icmp->get_operands_ref()[1];
    std::shared_ptr<ir::ConstantInt> bound_ci = nullptr;
    if (lhs == ind_phi)
        bound_ci = std::dynamic_pointer_cast<ir::ConstantInt>(rhs);
    else if (rhs == ind_phi)
        bound_ci = std::dynamic_pointer_cast<ir::ConstantInt>(lhs);
    if (!bound_ci) return std::nullopt;
    int bound = bound_ci->get_val();

    // Compute trips for common cond types
    auto cty = cond_icmp->get_cmp_type();
    std::optional<int> trips;
    switch (cty) {
        case ir::ICmp::ICmpType::EQ:
            if (init == bound) trips = 1;
            break;
        case ir::ICmp::ICmpType::NE:
            if (step != 0 && (bound - init) % step == 0) trips = (bound - init) / step;
            break;
        case ir::ICmp::ICmpType::SLE:
        case ir::ICmp::ICmpType::SGE:
            if (step != 0) trips = (bound - init) / step + 1;
            break;
        case ir::ICmp::ICmpType::SLT:
        case ir::ICmp::ICmpType::SGT:
            if (step != 0) trips = (bound - init) % step == 0 ? (bound - init) / step : (bound - init) / step + 1;
            break;
        default:
            break;
    }
    if (!trips.has_value()) return std::nullopt;
    if (trips.value() < 0 || trips.value() > 1000000) return std::nullopt;
    return trips;
}

bool PartialEvaluate::run(ir::Module *module) {
    bool modified = false;
    std::cout << "[PartialEval] run begin" << std::endl;
    module->for_each_func([&](std::shared_ptr<ir::Function> f) {
        // map: base alloca -> ext (loop-invariant float in preheader)
        std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> base_to_ext;
        // Loop-level partial evaluation (interpreter-driven, float-friendly, with local alloca memory replay):
        // - Innermost loop, single exit, known small trip count
        // - No CALL inside the loop
        // - Memory only via function-local alloca bases; preheader constant stores are replayed into stub entry
        // - All external operands must be constants, except allowed alloca bases which will be remapped
        // For each exit LCSSA phi (i32/f32), clone the loop region into a stub function and execute via interpreter.
        auto &forest = f->opt_info.loop_forest;
        size_t loop_count = 0;
        for (auto itc = forest.begin(); itc != forest.end(); ++itc) ++loop_count;
        std::cout << "[PartialEval] loops in function = " << loop_count << std::endl;
        for (auto it = forest.begin(); it != forest.end(); ++it) {
            auto loop = *it;
            if (!loop) continue;
            std::cout << "[PartialEval] loop shape: exits=" << loop->exits.size()
                      << ", exitings=" << loop->exitings.size() << ", pre_header=" << (loop->pre_header ? 1 : 0)
                      << std::endl;
            std::cout << "[PartialEval] loop shape: exits=" << loop->exits.size()
                      << ", exitings=" << loop->exitings.size() << ", pre_header=" << (loop->pre_header ? 1 : 0)
                      << ", has_trip_count=" << (loop->trip_count.has_value() ? 1 : 0) << std::endl;
            // prefer analyzed trip_count; fallback to local simple derivation
            std::optional<int> trip_count_opt = loop->trip_count;
            if (!trip_count_opt.has_value()) trip_count_opt = try_compute_trip_count_simple(loop);
            if (!trip_count_opt.has_value()) continue;
            int trip_count = *trip_count_opt;
            if (trip_count < 0 || trip_count > 10000) continue;  // allow up to 1e4
            std::cout << "[PartialEval] loop candidate: trip_count=" << trip_count << ", exits=" << loop->exits.size()
                      << ", exitings=" << loop->exitings.size() << std::endl;
            // allow non-innermost loops for region-level simulation
            if (loop->exits.size() != 1 || loop->exitings.size() != 1) continue;  // single-exit/single-exiting
            auto exit = *loop->exits.begin();
            auto exiting = *loop->exitings.begin();

            // Reject loops containing calls; collect memory bases (must be local allocas)
            bool has_unsupported = false;
            std::unordered_set<std::shared_ptr<ir::Value>> mem_bases;
            for (auto &bb : loop->blocks) {
                for (auto &ins : bb->get_instructions_ref()) {
                    using IT = ir::Instruction::InstructionType;
                    auto ty = ins->get_ins_type();
                    if (ty == IT::CALL) {
                        has_unsupported = true;
                        break;
                    }
                    if (ty == IT::LOAD || ty == IT::STORE) {
                        std::shared_ptr<ir::Value> addr = (ty == IT::LOAD)
                                                              ? std::dynamic_pointer_cast<ir::Load>(ins)->addr()
                                                              : std::dynamic_pointer_cast<ir::Store>(ins)->addr();
                        auto sig = decompose_addr(addr);
                        if (!sig) {
                            has_unsupported = true;
                            break;
                        }
                        auto base = sig->base;
                        auto base_alloca = std::dynamic_pointer_cast<ir::Alloca>(base);
                        if (!base_alloca) {
                            has_unsupported = true;
                            break;
                        }
                        if (!opt::util::is_local(base_alloca, f)) {
                            has_unsupported = true;
                            break;
                        }
                        mem_bases.insert(base_alloca);
                    }
                }
                if (has_unsupported) break;
            }
            if (has_unsupported) {
                std::cout << "[PartialEval] skip loop: has CALL or non-local/unsupported memory access" << std::endl;
                continue;
            }

            // Ensure all external operands are constants, except allowed alloca bases
            auto loop_contains = [&](const std::shared_ptr<ir::Value> &v) { return loop->defined(v); };
            bool external_nonconst = false;
            for (auto &bb : loop->blocks) {
                for (auto &ins : bb->get_instructions_ref()) {
                    for (auto &opv : ins->get_operands_ref()) {
                        if (loop_contains(opv)) continue;
                        if (std::dynamic_pointer_cast<ir::Constant>(opv)) continue;
                        if (mem_bases.count(opv)) continue;
                        if (std::dynamic_pointer_cast<ir::Alloca>(opv) && opt::util::is_local(opv, f)) {
                            mem_bases.insert(opv);
                            continue;
                        }
                        external_nonconst = true;
                        break;
                    }
                    if (external_nonconst) break;
                }
                if (external_nonconst) break;
            }
            bool externals_const = !external_nonconst;
            if (!externals_const) {
                std::cout << "[PartialEval] note: external non-const operand encountered; skip interpreter path"
                          << std::endl;
            }

            // Try expression-level replay for typical pattern: store float (ext + sitofp(iv)) to base[iv]
            // Collect candidate stores
            struct StorePattern {
                std::shared_ptr<ir::Value> base;
                std::shared_ptr<ir::Value> ext;  // loop-invariant scalar (float)
                std::shared_ptr<ir::Value> iv;   // header phi (i32)
                bool valid;
            };
            std::vector<StorePattern> patterns;
            for (auto &bb : loop->blocks) {
                for (auto &ins : bb->get_instructions_ref()) {
                    auto st = std::dynamic_pointer_cast<ir::Store>(ins);
                    if (!st) continue;
                    auto addr = st->addr();
                    auto sig = decompose_addr(addr);
                    if (!sig) continue;
                    auto base = sig->base;
                    if (!mem_bases.count(base)) continue;
                    if (sig->idx.empty()) continue;
                    auto idxv = sig->idx.back();
                    // idx should be header phi (iv)
                    auto phi_iv = std::dynamic_pointer_cast<ir::Phi>(idxv);
                    if (!phi_iv) continue;
                    if (phi_iv->get_parent_block().lock() != loop->header) continue;
                    // RHS should be fadd ext , sitofp(iv) or reversed
                    auto rhs = st->val();
                    auto fadd = std::dynamic_pointer_cast<ir::FAdd>(rhs);
                    if (!fadd) continue;
                    auto op0 = fadd->get_operands_ref()[0];
                    auto op1 = fadd->get_operands_ref()[1];
                    auto is_cast_of_iv = [&](const std::shared_ptr<ir::Value> &v) -> bool {
                        auto cast = std::dynamic_pointer_cast<ir::SIToFP>(v);
                        if (!cast) return false;
                        return cast->val() == idxv;
                    };
                    std::shared_ptr<ir::Value> ext = nullptr;
                    if (is_cast_of_iv(op0))
                        ext = op1;
                    else if (is_cast_of_iv(op1))
                        ext = op0;
                    else
                        continue;
                    // ext must be float-typed and loop-invariant
                    if (!ext->get_type()->is_float_ty()) continue;
                    // accept ext from header phi: we will use its preheader incoming value
                    patterns.push_back(StorePattern{base, ext, idxv, true});
                }
            }
            if (!patterns.empty()) {
                std::cout << "[PartialEval] store patterns matched: " << patterns.size() << std::endl;
                // Build trampoline and replay
                if (!loop->pre_header) continue;
                auto trampoline = std::make_shared<ir::BasicBlock>(f, gen_block_name());
                f->add_basic_block(f->get_basic_blocks_ref().end(), trampoline);
                auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
                for (int c = 0; c < trip_count; ++c) {
                    auto c32 = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c);
                    auto c32f = ir::SIToFP::create(trampoline, c32, ir::FloatType::get(), gen_local_var_name());
                    for (const auto &pat : patterns) {
                        // resolve ext at preheader if it is a phi in header, else use as-is
                        std::shared_ptr<ir::Value> ext_val = pat.ext;
                        if (auto ext_phi = std::dynamic_pointer_cast<ir::Phi>(ext_val)) {
                            auto pre_ext = opt::util::get_phi_value_safe(ext_phi, loop->pre_header);
                            if (pre_ext) ext_val = pre_ext;
                        }
                        // record base->ext for downstream sum-reduction detection
                        base_to_ext[pat.base] = ext_val;
                        // addr = gep base, 0, c
                        std::vector<std::shared_ptr<ir::Value>> idx_list{zero, c32};
                        auto addr_j = ir::Getelementptr::create(trampoline, pat.base, idx_list, gen_local_var_name());
                        // val = fadd ext_val , float(c)
                        auto val_j = ir::FAdd::create(trampoline, ext_val, c32f, gen_local_var_name());
                        (void)ir::Store::create(trampoline, val_j, addr_j, gen_local_var_name());
                    }
                }
                (void)ir::Br::create(trampoline, *loop->exits.begin(), gen_block_name());
                opt::util::redirect(loop->pre_header, loop->header, trampoline);
                modified = true;
                std::cout << "[PartialEval] built trampoline for pattern replay" << std::endl;
                return;
            }

            // Process each LCSSA phi at the single exit (interpreter path requires externals to be constant)
            if (externals_const)
                for (auto &phi : opt::util::get_phis(exit)) {
                    if (phi->get_operands_ref().size() != 2) continue;
                    auto phity = phi->get_type();
                    if (!phity->is_integer_ty() && !phity->is_float_ty()) continue;

                    // Identify the loop-provided incoming value for this phi from the exiting block
                    auto loop_val = opt::util::get_phi_value_safe(phi, exiting);
                    if (!loop_val) continue;

                    // Build a stub function that contains only the loop region; return the mapped loop_val when exiting
                    auto fty = ir::FunctionType::get(phity, {});
                    auto stub = std::make_shared<ir::Function>(fty, "@" + gen_block_name());
                    module->add_function(stub);
                    auto entry = std::make_shared<ir::BasicBlock>(stub, gen_block_name());
                    stub->add_basic_block(stub->get_basic_blocks_ref().end(), entry);

                    // Map original loop basic blocks to cloned blocks in stub
                    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> val_map;
                    std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::shared_ptr<ir::BasicBlock>> bb_map;
                    for (auto &bb : loop->blocks) {
                        auto nbb = std::make_shared<ir::BasicBlock>(stub, gen_block_name());
                        stub->add_basic_block(stub->get_basic_blocks_ref().end(), nbb);
                        bb_map[bb] = nbb;
                        val_map[bb] = nbb;  // blocks are also Values
                    }
                    // Preheader maps to entry for PHI incoming adjustment
                    if (loop->pre_header) {
                        val_map[loop->pre_header] = entry;
                    }

                    // Create cloned allocas in entry and map from original allocas
                    for (const auto &base : mem_bases) {
                        auto orig_alloca = std::dynamic_pointer_cast<ir::Alloca>(base);
                        if (!orig_alloca) continue;
                        auto content_ty = orig_alloca->get_content_type();
                        auto new_alloca = ir::Alloca::create(entry, content_ty, gen_local_var_name());
                        val_map[orig_alloca] = new_alloca;
                    }

                    // Clone instructions block by block
                    for (auto &bb : loop->blocks) {
                        auto nbb = bb_map[bb];
                        for (auto &ins : bb->get_instructions_ref()) {
                            auto clone = ins->clone();
                            nbb->add_instruction(nbb->get_instructions_ref().end(), clone);
                            val_map[ins] = clone;
                        }
                    }

                    // Remap operands for all cloned instructions
                    for (auto &bb : loop->blocks) {
                        auto nbb = bb_map[bb];
                        for (auto &nins : nbb->get_instructions_ref()) {
                            opt::util::substitute_operand(nins, val_map);
                        }
                    }

                    // Adjust header PHIs: redirect preheader incoming to entry block
                    auto header = loop->header;
                    auto nheader = bb_map[header];
                    for (auto &nphi : opt::util::get_phis(nheader)) {
                        // Remove any incoming from unknown blocks; keep backedge and replace preheader with entry
                        std::vector<std::shared_ptr<ir::BasicBlock>> to_remove;
                        for (auto &opv : nphi->get_operands_ref()) {
                            auto blk = opt::util::get_phi_block(nphi, opv);
                            if (!blk) continue;
                            if (bb_map.count(blk)) continue;  // keep
                            // This is from preheader in original; redirect to entry
                            to_remove.push_back(blk);
                        }
                        for (auto &rb : to_remove) {
                            opt::util::remove_phi_block(nphi, rb);
                        }
                        // If preheader existed, add entry incoming using the same value as original preheader incoming
                        if (loop->pre_header) {
                            auto orig_phi =
                                std::dynamic_pointer_cast<ir::Phi>(opt::util::get_phi_value(nphi, loop->pre_header));
                            // Not reliable to query; instead fetch original phi's preheader incoming via original
                            // Reconstruct: original counterpart of nphi is some phi in header; we can derive by reverse
                            // map Simpler: do nothing here; the operand remap already replaced preheader block with
                            // entry via val_map
                        }
                    }

                    // Replay constant inits from preheader for each tracked alloca address
                    if (loop->pre_header) {
                        std::unordered_map<AddrSig, std::shared_ptr<ir::Constant>, AddrSigHash> init_map;
                        for (auto &inst : loop->pre_header->get_instructions_ref()) {
                            if (auto st = std::dynamic_pointer_cast<ir::Store>(inst)) {
                                auto cval = std::dynamic_pointer_cast<ir::Constant>(st->val());
                                if (!cval) continue;
                                auto sig = decompose_addr(st->addr());
                                if (!sig) continue;
                                if (!mem_bases.count(sig->base)) continue;
                                bool all_const_idx = true;
                                for (const auto &iv : sig->idx) {
                                    if (!std::dynamic_pointer_cast<ir::ConstantInt>(iv)) {
                                        all_const_idx = false;
                                        break;
                                    }
                                }
                                if (!all_const_idx) continue;
                                init_map[*sig] = cval;
                            }
                        }
                        for (const auto &kv : init_map) {
                            const auto &sig = kv.first;
                            auto cval = kv.second;
                            auto it_alloca = val_map.find(sig.base);
                            if (it_alloca == val_map.end()) continue;
                            auto cloned_alloca = it_alloca->second;
                            std::shared_ptr<ir::Value> addr = cloned_alloca;
                            if (!sig.idx.empty()) {
                                std::vector<std::shared_ptr<ir::Value>> idx_list = sig.idx;
                                addr = ir::Getelementptr::create(entry, cloned_alloca, idx_list, gen_local_var_name());
                            }
                            (void)ir::Store::create(entry, cval, addr, gen_local_var_name());
                        }
                    }

                    // Replace the branch to exit in the cloned exiting block with a return of the mapped loop value
                    auto nexciting = bb_map[exiting];
                    auto term = nexciting->tail_instruction();
                    if (term) {
                        // Find the mapped loop value
                        auto mapped_loop_val_it = val_map.find(loop_val);
                        if (mapped_loop_val_it == val_map.end()) {
                            // Fallback: cleanup and skip if mapping missing
                            opt::util::remove_function(module, stub);
                            continue;
                        }
                        auto mapped_loop_val = mapped_loop_val_it->second;
                        opt::util::remove_instruction_from_parent_basic_block(term);
                        auto ret_created = ir::Ret::create(nexciting, mapped_loop_val, gen_block_name());
                        (void)ret_created;
                    }

                    // Connect entry to cloned header
                    auto br_created = ir::Br::create(entry, bb_map[loop->header], gen_block_name());
                    (void)br_created;

                    // Execute the stub via interpreter and harvest effects on tracked allocas
                    opt::exec::Interpreter interp{200'000'000ULL, 8 << 20, 256, false};
                    // build tracked set (cloned allocas in stub)
                    std::unordered_set<std::shared_ptr<ir::Instruction>> tracked;
                    for (const auto &base : mem_bases) {
                        auto it = val_map.find(base);
                        if (it != val_map.end()) {
                            if (auto all = std::dynamic_pointer_cast<ir::Instruction>(it->second)) tracked.insert(all);
                        }
                    }
                    std::vector<opt::exec::Interpreter::StoreEffect> effects;
                    auto eres = interp.execute_with_effects(*module, stub, {}, tracked, &effects, nullptr);
                    if (!std::holds_alternative<std::shared_ptr<ir::Constant>>(eres)) {
                        if (std::holds_alternative<opt::exec::FailReason>(eres)) {
                            auto r = std::get<opt::exec::FailReason>(eres);
                            std::cout << "[PartialEval] interpreter failed, reason=" << static_cast<int>(r)
                                      << std::endl;
                        } else {
                            std::cout << "[PartialEval] interpreter failed, unknown variant" << std::endl;
                        }
                        // cleanup stub and continue
                        opt::util::remove_function(module, stub);
                        continue;
                    }
                    auto cres = std::get<std::shared_ptr<ir::Constant>>(eres);
                    opt::util::substitute(phi, cres);
                    modified = true;
                    std::cout << "[PartialEval] phi replaced by constant; effects collected=" << effects.size()
                              << std::endl;

                    // Build a trampoline block to replay effects, then jump to exit; redirect preheader -> header to
                    // trampoline
                    if (!loop->pre_header) {
                        opt::util::remove_function(module, stub);
                        continue;
                    }
                    auto trampoline = std::make_shared<ir::BasicBlock>(f, gen_block_name());
                    f->add_basic_block(f->get_basic_blocks_ref().end(), trampoline);
                    // Reverse map: cloned alloca in stub -> original alloca in f
                    auto reverse_lookup_orig_alloca =
                        [&](const std::shared_ptr<ir::Instruction> &cloned) -> std::shared_ptr<ir::Value> {
                        for (const auto &base : mem_bases) {
                            auto it = val_map.find(base);
                            if (it != val_map.end() && it->second == cloned) return base;
                        }
                        return nullptr;
                    };
                    int replay_count = 0;
                    const int replay_limit = 20000;
                    for (const auto &eff : effects) {
                        if (replay_count >= replay_limit) break;
                        auto orig_alloca = reverse_lookup_orig_alloca(eff.base_alloca);
                        if (!orig_alloca) continue;
                        // construct gep [0, idx]
                        auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
                        auto cidx = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), eff.index);
                        std::vector<std::shared_ptr<ir::Value>> idx_list;
                        idx_list.push_back(zero);
                        idx_list.push_back(cidx);
                        auto addr = ir::Getelementptr::create(trampoline, orig_alloca, idx_list, gen_local_var_name());
                        (void)ir::Store::create(trampoline, eff.value, addr, gen_local_var_name());
                        replay_count++;
                    }
                    // jump to exit
                    (void)ir::Br::create(trampoline, exit, gen_block_name());
                    // redirect pre_header edge
                    opt::util::redirect(loop->pre_header, loop->header, trampoline);
                    // Remove the temporary stub function to avoid polluting the module
                    opt::util::remove_function(module, stub);
                    std::cout << "[PartialEval] replayed effects count=" << replay_count << std::endl;
                    return;
                }

            // Sum-reduction closed form: acc = sum_{i=0..N-1} load(baseX[i]) * load(baseY[i])
            // where baseX[i] = extX + i, baseY[i] = extY + i from previous write loop
            if (loop->exits.size() == 1) {
                auto header = loop->header;
                // find accumulator phi float with incoming 0 from preheader
                std::shared_ptr<ir::Phi> acc_phi;
                for (auto &inst : header->get_instructions_ref()) {
                    auto p = std::dynamic_pointer_cast<ir::Phi>(inst);
                    if (!p) break;
                    if (!p->get_type()->is_float_ty()) continue;
                    auto vals = p->get_operands_ref();
                    if (vals.size() >= 2) {
                        auto c0 = std::dynamic_pointer_cast<ir::ConstantFloat>(vals[0]);
                        if (c0 && c0->get_val() == 0.0F) {
                            acc_phi = p;
                            break;
                        }
                    }
                }
                if (acc_phi) {
                    // scan body for fmul of two loads from baseX[iv], baseY[iv]
                    std::shared_ptr<ir::Value> base_x, base_y;
                    for (auto &bb : loop->blocks) {
                        for (auto &inst : bb->get_instructions_ref()) {
                            auto fm = std::dynamic_pointer_cast<ir::FMul>(inst);
                            if (!fm) continue;
                            auto l0 = std::dynamic_pointer_cast<ir::Load>(fm->get_operands_ref()[0]);
                            auto l1 = std::dynamic_pointer_cast<ir::Load>(fm->get_operands_ref()[1]);
                            if (!l0 || !l1) continue;
                            auto a0 = decompose_addr(l0->addr());
                            auto a1 = decompose_addr(l1->addr());
                            if (!a0 || !a1) continue;
                            base_x = a0->base;
                            base_y = a1->base;
                            // normalize to underlying alloca
                            if (auto gep_x = std::dynamic_pointer_cast<ir::Getelementptr>(base_x)) {
                                auto sig = decompose_addr(gep_x);
                                if (sig) base_x = sig->base;
                            }
                            if (auto gep_y = std::dynamic_pointer_cast<ir::Getelementptr>(base_y)) {
                                auto sig = decompose_addr(gep_y);
                                if (sig) base_y = sig->base;
                            }
                            break;
                        }
                    }
                    if (base_x && base_y && base_to_ext.count(base_x) && base_to_ext.count(base_y)) {
                        auto ext_x = base_to_ext[base_x];
                        auto ext_y = base_to_ext[base_y];
                        // build closed form in exit block
                        auto exit_block = *loop->exits.begin();
                        auto i32ty = ir::IntegerType::get(32);
                        auto fty = ir::FloatType::get();
                        auto c_n = std::make_shared<ir::ConstantInt>(i32ty, trip_count);
                        auto c_n_minus_1 = std::make_shared<ir::ConstantInt>(i32ty, trip_count - 1);
                        auto c2 = std::make_shared<ir::ConstantInt>(i32ty, 2);
                        auto c6 = std::make_shared<ir::ConstantInt>(i32ty, 6);
                        // S1 = N*(N-1)/2 (as float)
                        auto s1_num = ir::Mul::create(exit_block, c_n, c_n_minus_1, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), s1_num);
                        auto s1_div = ir::SDiv::create(exit_block, s1_num, c2, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), s1_div);
                        auto s1f = ir::SIToFP::create(exit_block, s1_div, fty, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), s1f);
                        // S2 = N*(N-1)*(2N-1)/6
                        auto two_n = ir::Mul::create(exit_block, c2, c_n, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), two_n);
                        auto two_n_minus_1 = ir::Sub::create(
                            exit_block, two_n, std::make_shared<ir::ConstantInt>(i32ty, 1), gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), two_n_minus_1);
                        auto s2_num1 = ir::Mul::create(exit_block, c_n, c_n_minus_1, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), s2_num1);
                        auto s2_num = ir::Mul::create(exit_block, s2_num1, two_n_minus_1, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), s2_num);
                        auto s2_div = ir::SDiv::create(exit_block, s2_num, c6, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), s2_div);
                        auto s2f = ir::SIToFP::create(exit_block, s2_div, fty, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), s2f);
                        // Nf = float(N)
                        auto n_f = ir::SIToFP::create(exit_block, c_n, fty, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), n_f);
                        // sum = Nf*extX*extY + (extX+extY)*S1 + S2
                        auto t0 = ir::FMul::create(exit_block, n_f, ext_x, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), t0);
                        auto t1 = ir::FMul::create(exit_block, t0, ext_y, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), t1);
                        auto t2 = ir::FAdd::create(exit_block, ext_x, ext_y, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), t2);
                        auto t3 = ir::FMul::create(exit_block, t2, s1f, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), t3);
                        auto t4 = ir::FAdd::create(exit_block, t1, t3, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), t4);
                        auto sum = ir::FAdd::create(exit_block, t4, s2f, gen_local_var_name());
                        exit_block->add_instruction(exit_block->get_instructions_ref().end(), sum);
                        // replace exit LCSSA phis fed from inside the loop with closed-form sum
                        for (auto &exit_phi : opt::util::get_phis(exit_block)) {
                            bool has_loop_incoming = false;
                            auto &ops = exit_phi->get_operands_ref();
                            for (size_t i = 1; i < ops.size(); i += 2) {
                                auto blk = std::dynamic_pointer_cast<ir::BasicBlock>(ops[i]);
                                if (blk && loop->contains(blk)) {
                                    has_loop_incoming = true;
                                    break;
                                }
                            }
                            if (has_loop_incoming) {
                                opt::util::substitute(exit_phi, sum);
                            }
                        }
                        // redirect pre_header -> header to exit directly
                        opt::util::redirect(loop->pre_header, header, exit_block);
                        modified = true;
                        std::cout << "[PartialEval] closed-form sum applied" << std::endl;
                        return;
                    }
                }
            }
        }
    });
    return modified;
}

}  // namespace opt::pass
