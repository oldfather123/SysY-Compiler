#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {

// static inline bool value_equal(std::shared_ptr<ir::Value> v1, std::shared_ptr<ir::Value> v2) {
//     if (v1 == v2) return true;
//     if (std::dynamic_pointer_cast<ir::ConstantInt>(v1) && std::dynamic_pointer_cast<ir::ConstantInt>(v2)) {
//         return std::dynamic_pointer_cast<ir::ConstantInt>(v1)->get_val() ==
//                std::dynamic_pointer_cast<ir::ConstantInt>(v2)->get_val();
//     }
//     return false;
// }

// static bool need_swap(std::shared_ptr<ir::Value> v1, std::shared_ptr<ir::Value> v2) {
//     auto const1 = std::dynamic_pointer_cast<ir::ConstantInt>(v1) != nullptr;
//     auto const2 = std::dynamic_pointer_cast<ir::ConstantInt>(v2) != nullptr;
//     if (const1 != const2) {
//         return const1;
//     }
//     if (const1) {
//         return std::dynamic_pointer_cast<ir::ConstantInt>(v1)->get_val() <
//                std::dynamic_pointer_cast<ir::ConstantInt>(v2)->get_val();
//     }
//     return v1.get() < v2.get();
// }

static inline std::shared_ptr<ir::ConstantInt> get_imm(int64_t v);

struct VarPair {
    std::shared_ptr<ir::Value> v1, v2;
    bool operator==(const VarPair &other) const { return v1 == other.v1 && v2 == other.v2; }
    static std::pair<VarPair, bool> create(std::shared_ptr<ir::Value> v1, std::shared_ptr<ir::Value> v2) {
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(v1);
            const_int && const_int->get_type() == ir::IntegerType::get(32))
            v1 = get_imm(const_int->get_val());
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(v2);
            const_int && const_int->get_type() == ir::IntegerType::get(32))
            v2 = get_imm(const_int->get_val());
        if (v1 > v2) {
            std::swap(v1, v2);
            return {VarPair{v1, v2}, true};
        }
        return {VarPair{v1, v2}, false};
    }

private:
    VarPair(std::shared_ptr<ir::Value> v1, std::shared_ptr<ir::Value> v2) : v1(v1), v2(v2) {}
};

struct VarPairHasher {
    size_t operator()(const VarPair &vp) const {
        return std::hash<std::shared_ptr<ir::Value>>{}(vp.v1) ^ std::hash<std::shared_ptr<ir::Value>>{}(vp.v2);
    }
};

enum class KnownRelation { UNKNOWN, TRUE, FALSE };
std::ostream &operator<<(std::ostream &os, KnownRelation rel) {
    switch (rel) {
        case KnownRelation::UNKNOWN:
            return os << "UNKNOWN";
        case KnownRelation::TRUE:
            return os << "TRUE";
        case KnownRelation::FALSE:
            return os << "FALSE";
    }
}

struct KnownRelations {
    KnownRelation equal = KnownRelation::UNKNOWN;
    KnownRelation less = KnownRelation::UNKNOWN;
    KnownRelation greater = KnownRelation::UNKNOWN;

    bool operator==(const KnownRelations &rhs) const {
        return equal == rhs.equal && less == rhs.less && greater == rhs.greater;
    }

    bool update(const KnownRelations &other) {
        auto old = *this;
        if (other.equal != KnownRelation::UNKNOWN && equal != other.equal) equal = other.equal;
        if (other.less != KnownRelation::UNKNOWN && less != other.less) less = other.less;
        if (other.greater != KnownRelation::UNKNOWN && greater != other.greater) greater = other.greater;
        return !(old == *this);
    }

    void merge(const KnownRelations &other) {
        if (equal != other.equal) equal = KnownRelation::UNKNOWN;
        if (less != other.less) less = KnownRelation::UNKNOWN;
        if (greater != other.greater) greater = KnownRelation::UNKNOWN;
    }

    bool infer() {
        auto old = *this;
        if (equal == KnownRelation::TRUE) {
            less = KnownRelation::FALSE;
            greater = KnownRelation::FALSE;
        }
        if (less == KnownRelation::TRUE) {
            equal = KnownRelation::FALSE;
            greater = KnownRelation::FALSE;
        }
        if (greater == KnownRelation::TRUE) {
            equal = KnownRelation::FALSE;
            less = KnownRelation::FALSE;
        }
        if (less == KnownRelation::FALSE && greater == KnownRelation::FALSE) equal = KnownRelation::TRUE;
        if (equal == KnownRelation::FALSE && less == KnownRelation::FALSE) greater = KnownRelation::TRUE;
        if (equal == KnownRelation::FALSE && greater == KnownRelation::FALSE) less = KnownRelation::TRUE;
        if (less == KnownRelation::FALSE && greater == KnownRelation::FALSE) equal = KnownRelation::TRUE;
        return !(old == *this);
    }
};

using CondSet = std::unordered_map<VarPair, KnownRelations, VarPairHasher>;

static bool modified = false;
static ir::opt_support::IntegerRangeInfo *int_range_info;
static std::unordered_map<
    std::shared_ptr<ir::BasicBlock>,
    std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::unordered_map<std::shared_ptr<ir::Value>, bool>>>
    edges;
static std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::unordered_map<std::shared_ptr<ir::BasicBlock>, CondSet>>
    edge_sets;
static std::unordered_map<std::shared_ptr<ir::BasicBlock>, CondSet> conditions;

void print_conditions() {
    for (auto &[block, set] : conditions) {
        std::cout << "\nBlock: " << block->get_name() << std::endl;
        for (auto &[pair, relations] : set) {
            std::cout << "  " << pair.v1->get_name() << " " << pair.v2->get_name() << " " << relations.equal << " "
                      << relations.less << " " << relations.greater << std::endl;
        }
    }
}

void print_edge_sets() {
    for (auto &[block1, set] : edge_sets) {
        for (auto &[block2, set2] : set) {
            std::cout << "  " << block1->get_name() << " " << block2->get_name() << " : " << std::endl;
            for (auto &[pair, relations] : set2) {
                std::cout << "    " << pair.v1->get_name() << " " << pair.v2->get_name() << " " << relations.equal
                          << " " << relations.less << " " << relations.greater << std::endl;
            }
        }
    }
}

void print_edges() {
    for (auto &[block1, set] : edges) {
        for (auto &[block2, val] : set) {
            for (auto &[cond, val] : val) {
                std::cout << "  " << block1->get_name() << " " << block2->get_name() << " " << cond->to_string() << " "
                          << val << std::endl;
            }
        }
    }
}

static std::shared_ptr<ir::ConstantInt> true_val = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), 1);
static std::shared_ptr<ir::ConstantInt> false_val = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), 0);

static void collect_facts(std::shared_ptr<ir::Function> func);
static void spread_facts(std::shared_ptr<ir::Function> func);
static void reduce(std::shared_ptr<ir::Function> func);
void replace_conditional_br(std::shared_ptr<ir::Function> func);

bool ConstraintReduction::run(ir::Module *module) {
    logger::INFO("Running ConstraintReduction...");
    modified = false;
    int_range_info = &module->opt_info.int_range_info;

    module->for_each_func([](auto &func) {
        // for (auto &[val, range] : int_range_info->ranges) {
        //     if (!range.is_any()) std::cout << val->to_string() << " : " << range << std::endl;
        // }
        // std::cout << "--------------------------------" << std::endl;
        collect_facts(func);
        // print_edges();
        // std::cout << "--------------------------------" << std::endl;
        // print_edge_sets();
        // std::cout << "--------------------------------" << std::endl;
        // print_conditions();

        spread_facts(func);
        // std::cout << "Spread finish" << std::endl;
        // std::cout << "--------------------------------" << std::endl;
        // print_edges();
        // std::cout << "--------------------------------" << std::endl;
        // print_edge_sets();
        // std::cout << "--------------------------------" << std::endl;
        // print_conditions();

        reduce(func);
        replace_conditional_br(func);
        edges.clear();
        edge_sets.clear();
        conditions.clear();
    });
    // assert(!modified);
    return modified;
}

static bool add_to_set(CondSet &set,
                       std::shared_ptr<ir::Value> lhs,
                       std::shared_ptr<ir::Value> rhs,
                       KnownRelations relations) {
    auto [pair, swapped] = VarPair::create(lhs, rhs);
    if (swapped) std::swap(relations.less, relations.greater);

    auto &ref = set[pair];
    bool m1 = ref.update(relations), m2 = ref.infer();
    return m1 | m2;
}

static std::shared_ptr<ir::ConstantInt> get_imm(int v) {
    static std::unordered_map<int, std::shared_ptr<ir::ConstantInt>> pool;
    if (auto it = pool.find(v); it != pool.end()) return it->second;
    return pool[v] = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), v);
}

static inline std::shared_ptr<ir::ConstantInt> get_imm(int64_t v) { return get_imm(static_cast<int>(v)); }

static void add_edge(std::unordered_map<std::shared_ptr<ir::Value>, bool> &edges,
                     std::shared_ptr<ir::Value> cond,
                     bool val) {
    if (!edges.emplace(cond, val).second) return;

    if (val) {
        auto and_inst = std::dynamic_pointer_cast<ir::And>(cond);
        if (!and_inst) return;
        auto v1 = and_inst->get_operands_ref()[0], v2 = and_inst->get_operands_ref()[1];
        add_edge(edges, v1, true);
        if (v1 != v2) add_edge(edges, v2, true);
    } else {
        auto or_inst = std::dynamic_pointer_cast<ir::Or>(cond);
        if (!or_inst) return;
        auto v1 = or_inst->get_operands_ref()[0], v2 = or_inst->get_operands_ref()[1];
        add_edge(edges, v1, false);
        add_edge(edges, v2, false);
    }
}

static bool calc_transitive_closure(CondSet &set) {
    static constexpr uint32_t max_system_size = 512;
    using Matrix = std::array<std::array<uint8_t, max_system_size>, max_system_size>;
    static constexpr uint8_t equal_lut[3][3] = {{0, 0, 0}, {0, 1, 2}, {0, 2, 0}};
    static constexpr uint8_t less_than_lut[3][3] = {{0, 0, 0}, {0, 1, 2}, {0, 2, 2}};

    std::vector<std::shared_ptr<ir::Value>> values;
    for (auto &[pair, relations] : set) {
        values.push_back(pair.v1);
        values.push_back(pair.v2);
    }
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    if (values.size() <= 2) return false;
    if (values.size() > max_system_size) return false;

    const auto get_value_idx = [&values](std::shared_ptr<ir::Value> v) -> uint32_t {
        return static_cast<uint32_t>(std::distance(values.begin(), std::lower_bound(values.begin(), values.end(), v)));
    };

    bool new_info = false;
    auto floyd = [&](Matrix &mat, auto transition_mat) {
        while (true) {
            bool changed = false;
            for (uint32_t k = 0; k < values.size(); ++k) {
                for (uint32_t i = 0; i < values.size(); ++i) {
                    if (!mat[i][k]) continue;
                    for (uint32_t j = 0; j < values.size(); ++j) {
                        const auto val = transition_mat(mat[i][k], mat[k][j]);
                        if (mat[i][j] < val) {
                            mat[i][j] = val;
                            changed = true;
                        }
                    }
                }
            }
            if (!changed) break;
            new_info = true;
        }
    };

    Matrix eq_mat{};
    for (uint32_t i = 0; i < values.size(); ++i) {
        eq_mat[i][i] = 1;
    }
    for (auto &[pair, relations] : set) {
        const auto i = get_value_idx(pair.v1), j = get_value_idx(pair.v2);
        if (relations.equal == KnownRelation::TRUE)
            eq_mat[i][j] = eq_mat[j][i] = 1;
        else if (relations.equal == KnownRelation::FALSE)
            eq_mat[i][j] = eq_mat[j][i] = 2;
    }
    for (uint32_t i = 0; i < values.size(); i++) {
        if (values[i]->get_type() == ir::IntegerType::get(1)) continue;
        if (auto lhs = std::dynamic_pointer_cast<ir::ConstantInt>(values[i])) {
            for (uint32_t j = i + 1; j < values.size(); j++) {
                if (values[j]->get_type() == ir::IntegerType::get(1)) continue;
                if (auto rhs = std::dynamic_pointer_cast<ir::ConstantInt>(values[j])) {
                    // FIXME(Xingkun): maybe need to compare signed extension val
                    if (*lhs == *rhs)
                        eq_mat[i][j] = eq_mat[j][i] = 1;
                    else
                        eq_mat[i][j] = eq_mat[j][i] = 2;
                }
            }
        }
    }
    floyd(eq_mat, [](uint8_t a, uint8_t b) { return equal_lut[a][b]; });

    Matrix lt_mat{};
    for (uint32_t i = 0; i < values.size(); ++i) {
        lt_mat[i][i] = 1;
    }
    for (auto &[pair, relations] : set) {
        const auto i = get_value_idx(pair.v1), j = get_value_idx(pair.v2);
        if (relations.less == KnownRelation::TRUE)
            lt_mat[i][j] = 2;
        else if (relations.greater == KnownRelation::FALSE)
            lt_mat[i][j] = 1;
        if (relations.greater == KnownRelation::TRUE)
            lt_mat[j][i] = 2;
        else if (relations.less == KnownRelation::FALSE)
            lt_mat[j][i] = 1;
    }
    for (uint32_t i = 0; i < values.size(); i++) {
        if (values[i]->get_type() == ir::IntegerType::get(1)) continue;
        if (auto lhs = std::dynamic_pointer_cast<ir::ConstantInt>(values[i])) {
            for (uint32_t j = i + 1; j < values.size(); ++j) {
                if (values[j]->get_type() == ir::IntegerType::get(1)) continue;
                if (auto rhs = std::dynamic_pointer_cast<ir::ConstantInt>(values[j])) {
                    if (*lhs <= *rhs) lt_mat[i][j] = 1;
                    if (*lhs >= *rhs) lt_mat[j][i] = 1;
                    if (*lhs < *rhs) lt_mat[i][j] = 2;
                    if (*lhs > *rhs) lt_mat[j][i] = 2;
                }
            }
        }
    }
    floyd(lt_mat, [](uint8_t a, uint8_t b) { return less_than_lut[a][b]; });
    if (!new_info) return false;

    bool changed = false;
    for (size_t i = 0; i < values.size(); i++) {
        for (size_t j = i + 1; j < values.size(); j++) {
            if (eq_mat[i][j] || lt_mat[i][j]) {
                if (std::dynamic_pointer_cast<ir::ConstantInt>(values[i]) &&
                    std::dynamic_pointer_cast<ir::ConstantInt>(values[j]))
                    continue;
                auto [pair, _] = VarPair::create(values[i], values[j]);
                // VarPair pair{values[i], values[j]};
                KnownRelations relations{};
                if (eq_mat[i][j] == 1 || eq_mat[j][i] == 1)
                    relations.equal = KnownRelation::TRUE;
                else if (eq_mat[i][j] == 2 || eq_mat[j][i] == 2)
                    relations.equal = KnownRelation::FALSE;
                if (lt_mat[i][j] == 2)
                    relations.less = KnownRelation::TRUE;
                else if (lt_mat[j][i] == 1)
                    relations.less = KnownRelation::FALSE;
                if (lt_mat[j][i] == 2)
                    relations.greater = KnownRelation::TRUE;
                else if (lt_mat[i][j] == 1)
                    relations.greater = KnownRelation::FALSE;

                changed |= set[pair].update(relations);
                changed |= set[pair].infer();
            }
        }
    }
    return changed;
}

static CondSet merge_sets(std::shared_ptr<ir::BasicBlock> cur_block,
                          const std::unordered_map<std::shared_ptr<ir::BasicBlock>, CondSet> &sets) {
    if (sets.empty()) return {};
    auto get_common_ancestor = [&]() -> std::shared_ptr<ir::BasicBlock> {
        std::shared_ptr<ir::BasicBlock> ancestor = nullptr;
        for (const auto &[block, set] : sets) {
            // if(!block->reachable)
            //     return nullptr;
            if (ancestor == nullptr) {
                ancestor = block;
            } else {
                ancestor = util::dom_lca(ancestor, block);
            }
        }
        if (cur_block != ancestor && ancestor->opt_info.dominates(cur_block)) {
            for (const auto &[block, set] : sets) {
                if (ancestor != block && !cur_block->opt_info.dominates(block)) return nullptr;
            }
            return ancestor;
        }
        return nullptr;
    };

    const auto common_ancestor = get_common_ancestor();
    const auto *const common_ancestor_set = sets.count(common_ancestor) ? &sets.at(common_ancestor) : nullptr;
    const CondSet *minimal = common_ancestor_set;
    if (!common_ancestor_set) {
        for (const auto &[block, set] : sets)
            if (!minimal || set.size() < minimal->size()) minimal = &set;
    }
    CondSet res = *minimal;

    std::vector<VarPair> to_remove;
    for (const auto &[block, set] : sets) {
        if (&set == minimal) continue;
        for (const auto &[pair, relations] : set) {
            if (res.count(pair)) res[pair].merge(relations);
        }
        if (!common_ancestor_set) {
            to_remove.clear();
            for (auto &[pair, relations] : res)
                if (!set.count(pair)) {
                    to_remove.push_back(pair);
                }
            for (auto &pair : to_remove) res.erase(pair);
        }
    }
    return res;
}

static void collect_facts(std::shared_ptr<ir::Function> func) {
    constexpr const uint32_t max_additional_facts = 32;
    uint32_t additional_fact = 0;
    for (const auto &block : func->get_basic_blocks_ref()) {
        auto &cond_set = conditions[block];
        for (const auto &inst : block->get_instructions_ref()) {
            if (inst->get_type() == ir::IntegerType::get(32) && additional_fact < max_additional_facts) {
                const auto r = int_range_info->query(inst, inst, 5);

                if (r.min() != std::numeric_limits<int32_t>::min()) {
                    if (++additional_fact <= max_additional_facts)
                        add_to_set(
                            cond_set,
                            inst,
                            get_imm(r.min()),
                            KnownRelations{KnownRelation::UNKNOWN, KnownRelation::FALSE, KnownRelation::UNKNOWN});
                }
                if (r.max() != std::numeric_limits<int32_t>::max()) {
                    if (++additional_fact <= max_additional_facts)
                        add_to_set(
                            cond_set,
                            inst,
                            get_imm(r.max()),
                            KnownRelations{KnownRelation::UNKNOWN, KnownRelation::UNKNOWN, KnownRelation::FALSE});
                }
            }

            if (auto br = std::dynamic_pointer_cast<ir::Br>(inst); br && br->is_cond_branch()) {
                const auto cond = br->get_operands_ref()[0];
                if (std::dynamic_pointer_cast<ir::ConstantInt>(cond)) continue;

                const auto true_target = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[1]);
                const auto false_target = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[2]);
                if (true_target == false_target) continue;

                add_edge(edges[true_target][block], cond, true);
                add_edge(edges[false_target][block], cond, false);
            }
        }
    }
}

static void spread_facts(std::shared_ptr<ir::Function> func) {
    std::queue<std::shared_ptr<ir::BasicBlock>> worklist;
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> in_queue;
    std::unordered_map<std::shared_ptr<ir::BasicBlock>, uint32_t> enqueue_cnt;
    constexpr const uint32_t max_enqueue_cnt = 60;

    auto update_edge_set = [&](const CondSet &src, CondSet &dst) {
        bool changed = false;
        for (const auto &[pair, rel] : src) {
            changed |= add_to_set(dst, pair.v1, pair.v2, rel);
        }
        return changed;
    };

    auto update_block = [&](std::shared_ptr<ir::BasicBlock> block) {
        auto &src = conditions[block];
        if (src.empty()) return;
        for (const auto &weak_succ : block->opt_info.successors) {
            auto succ = weak_succ.lock();
            if (update_edge_set(src, edge_sets[succ][block]) && in_queue.insert(succ).second &&
                ++enqueue_cnt[succ] < max_enqueue_cnt)
                worklist.push(succ);
        }
    };

    // 1. convert to edge_sets
    for (auto &block : func->get_basic_blocks_ref()) {
        auto &edge_set = edge_sets[block];
        for (auto &[src, edge] : edges[block]) {
            auto &set = edge_set[src];
            for (auto &[cond, val] : edge) {
                // std::cout << "src: " << src->get_name() << ", dst: " << block->get_name()
                //           << ", cond: " << cond->to_string() << ", val: " << val << std::endl;
                add_to_set(
                    set,
                    cond,
                    true_val,
                    {val ? KnownRelation::TRUE : KnownRelation::FALSE, KnownRelation::UNKNOWN, KnownRelation::UNKNOWN});
                auto icmp = std::dynamic_pointer_cast<ir::ICmp>(cond);
                if (!icmp) continue;
                auto v1 = icmp->get_operands_ref()[0], v2 = icmp->get_operands_ref()[1];
                auto op = val ? icmp->get_cmp_type() : ir::ICmp::get_inverted_op(icmp->get_cmp_type());
                switch (op) {
                    case ir::ICmp::ICmpType::EQ:
                        add_to_set(set, v1, v2, {KnownRelation::TRUE, KnownRelation::FALSE, KnownRelation::FALSE});
                        break;
                    case ir::ICmp::ICmpType::NE:
                        add_to_set(set, v1, v2, {KnownRelation::FALSE, KnownRelation::UNKNOWN, KnownRelation::UNKNOWN});
                        break;
                    case ir::ICmp::ICmpType::SLT:
                        add_to_set(set, v1, v2, {KnownRelation::FALSE, KnownRelation::TRUE, KnownRelation::FALSE});
                        break;
                    case ir::ICmp::ICmpType::SLE:
                        add_to_set(set, v1, v2, {KnownRelation::UNKNOWN, KnownRelation::UNKNOWN, KnownRelation::FALSE});
                        break;
                    case ir::ICmp::ICmpType::SGT:
                        add_to_set(set, v1, v2, {KnownRelation::FALSE, KnownRelation::FALSE, KnownRelation::TRUE});
                        break;
                    case ir::ICmp::ICmpType::SGE:
                        add_to_set(set, v1, v2, {KnownRelation::UNKNOWN, KnownRelation::FALSE, KnownRelation::UNKNOWN});
                        break;
                    default:
                        break;
                }
            }

            if (in_queue.insert(block).second) worklist.push(block);
            update_block(block);
        }
    }

    // std::cout << "--------------------------------" << std::endl;
    // print_edges();
    // std::cout << "--------------------------------" << std::endl;
    // print_edge_sets();
    // std::cout << "--------------------------------" << std::endl;
    // print_conditions();

    // 2. combine and infer to conditions
    while (!worklist.empty()) {
        const auto u = worklist.front();
        in_queue.erase(u);
        worklist.pop();

        auto preds = to_shared_vector(u->opt_info.predecessors);
        auto &dst = conditions[u];
        if (preds.size() == 1 && preds.front() != u) {
            bool changed = false;
            for (auto &[pair, rel] : edge_sets[u][preds.front()]) {
                changed |= add_to_set(dst, pair.v1, pair.v2, rel);
            }
            changed |= calc_transitive_closure(dst);
            if (changed) update_block(u);
        } else {
            auto &edge = edge_sets[u];
            for (auto pred : preds) edge[pred];
            auto new_dst = merge_sets(u, edge);
            for (auto &[pair, rel] : dst) add_to_set(new_dst, pair.v1, pair.v2, rel);
            for (auto &[pair, relations] : new_dst) relations.infer();
            calc_transitive_closure(new_dst);
            if (new_dst != dst) {
                dst.swap(new_dst);
                update_block(u);
            }
        }
    }
}

static void reduce(std::shared_ptr<ir::Function> func) {
    using ICmpType = ir::ICmp::ICmpType;
    auto solve_cmp = [](std::shared_ptr<ir::Value> v1,
                        std::shared_ptr<ir::Value> v2,
                        ICmpType op,
                        const CondSet &set) -> std::optional<bool> {
        if (v1 == v2) {
            switch (op) {
                case ICmpType::EQ:
                    return true;
                case ICmpType::NE:
                    return false;
                case ICmpType::SLT:
                    return false;
                case ICmpType::SLE:
                    return true;
                case ICmpType::SGT:
                    return false;
                case ICmpType::SGE:
                    return true;
                default:
                    return std::nullopt;
            }
        }

        auto [pair, swapped] = VarPair::create(v1, v2);
        if (swapped) op = ir::ICmp::get_reversed_op(op);
        const auto it = set.find(pair);
        // if (it == set.end()) {
        //     std::cout << "v1 id: " << v1->get_id() << " v2 id: " << v2->get_id() << std::endl;
        //     for (const auto &[pair, rel] : set) {
        //         if (pair.v1 == v1) {
        //             std::cout << "pair.v1.id: " << pair.v1->get_id() << " pair.v2.id: " << pair.v2->get_id()
        //                       << std::endl;
        //             std::cout << "v2 name: " << v2->get_name() << " pair.v2.name: " << pair.v2->get_name() <<
        //             std::endl; if (pair.v2 == v2) assert(false); auto c1 =
        //             std::dynamic_pointer_cast<ir::ConstantInt>(pair.v2); auto c2 =
        //             std::dynamic_pointer_cast<ir::ConstantInt>(v2); if (c1 && c2 && c1 == c2) assert(false); if (c1
        //             && c2 && *c1 == *c2) assert(false); if (c1 && c2 && c1->get_val() == c2->get_val())
        //             assert(false);
        //         }
        //     }
        // }
        if (it != set.end()) {
            const auto &rel = it->second;
            switch (op) {
                case ICmpType::EQ:
                    if (rel.equal != KnownRelation::UNKNOWN) return rel.equal == KnownRelation::TRUE;
                    break;
                case ICmpType::NE:
                    if (rel.equal != KnownRelation::UNKNOWN) return rel.equal == KnownRelation::FALSE;
                    break;
                case ICmpType::SLT:
                    if (rel.less != KnownRelation::UNKNOWN) return rel.less == KnownRelation::TRUE;
                    break;
                case ICmpType::SLE:
                    if (rel.greater != KnownRelation::UNKNOWN) return rel.greater == KnownRelation::FALSE;
                    break;
                case ICmpType::SGT:
                    if (rel.greater != KnownRelation::UNKNOWN) return rel.greater == KnownRelation::TRUE;
                    break;
                case ICmpType::SGE:
                    if (rel.less != KnownRelation::UNKNOWN) return rel.less == KnownRelation::FALSE;
                    break;
                default:
                    break;
            }
        }

        return std::nullopt;
    };

    auto solve_atomic = [&](std::shared_ptr<ir::Value> val, const CondSet &set) -> std::optional<bool> {
        if (auto res = solve_cmp(val, true_val, ICmpType::EQ, set); res.has_value()) return res;

        if (auto icmp = std::dynamic_pointer_cast<ir::ICmp>(val))
            return solve_cmp(icmp->get_operands_ref()[0], icmp->get_operands_ref()[1], icmp->get_cmp_type(), set);

        return std::nullopt;
    };

    using ValueMap = std::unordered_map<std::shared_ptr<ir::Value>, uint32_t>;

    auto infer = [&](std::shared_ptr<ir::Value> val, const CondSet &set) -> std::optional<bool> {
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(val)) return const_int->get_val() != 0;
        return solve_atomic(val, set);
    };

    auto explore =
        [&](std::shared_ptr<ir::Value> val, const CondSet &set, ValueMap &value_map, uint32_t depth, auto self) {
            if (value_map.count(val)) return;
            if (auto res = infer(val, set); res.has_value()) return;
            if (depth) {
                if (auto and_inst = std::dynamic_pointer_cast<ir::And>(val)) {
                    self(and_inst->get_operands_ref()[0], set, value_map, depth - 1, self);
                    self(and_inst->get_operands_ref()[1], set, value_map, depth - 1, self);
                    return;
                }
                if (auto or_inst = std::dynamic_pointer_cast<ir::Or>(val)) {
                    self(or_inst->get_operands_ref()[0], set, value_map, depth - 1, self);
                    self(or_inst->get_operands_ref()[1], set, value_map, depth - 1, self);
                    return;
                }
            }

            const auto id = static_cast<uint32_t>(value_map.size());
            value_map.emplace(val, id);
            return;
        };

    auto check_assign = [&](std::shared_ptr<ir::Value> val,
                            const ValueMap &map,
                            uint32_t assignment,
                            const CondSet &set,
                            auto self) -> bool {
        if (auto it = map.find(val); it != map.cend()) {
            return assignment & (1U << it->second);
        }
        if (auto res = infer(val, set); res.has_value()) return *res;

        if (auto and_inst = std::dynamic_pointer_cast<ir::And>(val)) {
            return self(and_inst->get_operands_ref()[0], map, assignment, set, self) &&
                   self(and_inst->get_operands_ref()[1], map, assignment, set, self);
        }
        if (auto or_inst = std::dynamic_pointer_cast<ir::Or>(val)) {
            return self(or_inst->get_operands_ref()[0], map, assignment, set, self) ||
                   self(or_inst->get_operands_ref()[1], map, assignment, set, self);
        }
        __builtin_unreachable();
    };

    auto is_valid_cmp = [](ICmpType op1, ICmpType op2) {
        switch (op1) {
            case ICmpType::EQ:
                return op2 != ICmpType::NE && op2 != ICmpType::SLT && op2 != ICmpType::SGT;
            case ICmpType::NE:
                return op2 != ICmpType::EQ;
            case ICmpType::SLT:
                return op2 != ICmpType::EQ && op2 != ICmpType::SGT && op2 != ICmpType::SGE;
            case ICmpType::SLE:
                return op2 != ICmpType::SGT;
            case ICmpType::SGT:
                return op2 != ICmpType::EQ && op2 != ICmpType::SLT && op2 != ICmpType::SLE;
            case ICmpType::SGE:
                return op2 != ICmpType::SLT;
            default:
                return true;
        }
    };

    auto is_valid_assignment = [&](uint32_t assignment, const ValueMap &map) -> bool {
        for (auto x = map.begin(); x != map.end(); ++x) {
            ICmpType cmp1, cmp2;
            if (auto icmp = std::dynamic_pointer_cast<ir::ICmp>(x->first)) {
                cmp1 = icmp->get_cmp_type();
                if (!(assignment & (1U << x->second))) cmp1 = ir::ICmp::get_inverted_op(cmp1);
                for (auto y = std::next(x); y != map.end(); ++y) {
                    if (auto icmp2 = std::dynamic_pointer_cast<ir::ICmp>(y->first)) {
                        cmp2 = icmp2->get_cmp_type();
                        if (!(assignment & (1U << y->second))) cmp2 = ir::ICmp::get_inverted_op(cmp2);
                        if (!is_valid_cmp(cmp1, cmp2)) return false;
                    }
                }
            }
        }
        return true;
    };

    auto solve_satisfy =
        [&](std::shared_ptr<ir::Value> val, const ValueMap &map, const CondSet &set) -> std::optional<bool> {
        bool has_false = false;
        bool has_true = false;

        const auto end = 1U << map.size();
        for (uint32_t assignment = 0; assignment < end; ++assignment) {
            if (!is_valid_assignment(assignment, map)) continue;
            if (check_assign(val, map, assignment, set, check_assign)) {
                has_true = true;
            } else {
                has_false = true;
            }
            if (has_true && has_false) return std::nullopt;
        }

        if (has_true && !has_false) return true;
        if (has_false && !has_true) return false;
        return std::nullopt;
    };

    auto solve = [&](std::shared_ptr<ir::Value> val, const CondSet &set) -> std::optional<bool> {
        if (auto res = solve_atomic(val, set); res.has_value()) return res;

        ValueMap val_map, last_map;
        for (uint32_t depth = 1; depth <= 5; ++depth) {
            val_map.clear();
            explore(val, set, val_map, depth, explore);
            if (val_map.size() > 8) break;
            if (depth != 1 && val_map == last_map) break;
            if (auto res = solve_satisfy(val, val_map, set); res.has_value()) return res;
            last_map = val_map;
        }
        return std::nullopt;
    };

    auto match_icmp_phi = [](std::shared_ptr<ir::Value> val)
        -> std::optional<std::tuple<std::shared_ptr<ir::Phi>, std::shared_ptr<ir::Value>, ICmpType>> {
        auto icmp = std::dynamic_pointer_cast<ir::ICmp>(val);
        if (!icmp) return std::nullopt;
        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(icmp->get_operands_ref()[0]))
            return std::make_tuple(phi, icmp->get_operands_ref()[1], icmp->get_cmp_type());
        else if (auto phi = std::dynamic_pointer_cast<ir::Phi>(icmp->get_operands_ref()[1]))
            return std::make_tuple(phi, icmp->get_operands_ref()[0], ir::ICmp::get_reversed_op(icmp->get_cmp_type()));
        return std::nullopt;
    };

    for (const auto &block : func->get_basic_blocks_ref()) {
        auto &ctx = conditions[block];
        for (auto &inst : block->get_instructions()) {
            if (inst->get_type() == ir::IntegerType::get(1)) {
                if (auto res = solve(inst, ctx); res.has_value()) {
                    modified = true;
                    util::substitute(inst, *res ? true_val : false_val);
                }
            }
            // std::cout << "cur inst: " << inst->to_string() << std::endl;

            for (const auto &op : inst->get_operands()) {
                if (op->get_type() != ir::IntegerType::get(1) || std::dynamic_pointer_cast<ir::Constant>(op)) continue;
                if (auto res = solve(op, ctx); res.has_value()) {
                    modified = true;
                    util::substitute_operand(inst, op, *res ? true_val : false_val);
                    continue;
                }

                auto match_res = match_icmp_phi(op);
                if (!match_res) continue;
                auto [phi, cmp_val, cmp_type] = *match_res;
                uint32_t res = 0x0;
                for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
                    auto val = phi->get_operands_ref()[i];
                    auto pred = std::dynamic_pointer_cast<ir::BasicBlock>(phi->get_operands_ref()[i + 1]);

                    auto &ctx_edge = edge_sets[phi->get_parent_block().lock()][pred];
                    auto res_edge = solve_cmp(val, cmp_val, cmp_type, ctx_edge);
                    if (!res_edge.has_value())
                        res = 0x11;
                    else
                        res |= (*res_edge ? 0x1 : 0x10);
                }
                if (res == 0x1) {
                    modified = true;
                    util::substitute_operand(inst, op, true_val);
                } else if (res == 0x10) {
                    modified = true;
                    util::substitute_operand(inst, op, false_val);
                }
            }
        }
    }
}

static void unconditional_redirect(std::shared_ptr<ir::Br> cond_br, std::shared_ptr<ir::BasicBlock> target) {
    auto block = cond_br->get_parent_block().lock();
    auto ori_block1 = std::dynamic_pointer_cast<ir::BasicBlock>(cond_br->get_operands_ref()[1]),
         ori_block2 = std::dynamic_pointer_cast<ir::BasicBlock>(cond_br->get_operands_ref()[2]);
    ori_block1->opt_info.remove_predecessor(block);
    ori_block2->opt_info.remove_predecessor(block);
    block->opt_info.remove_successor(ori_block1);
    block->opt_info.remove_successor(ori_block2);
    block->opt_info.successors.push_back(target);
    target->opt_info.predecessors.push_back(block);
    util::remove_all_operands(cond_br);
    cond_br->add_operand(target);
    target->add_user(cond_br);

    auto exclude_block = ori_block1 == target ? ori_block2 : ori_block1;
    for (auto &phi : util::get_phis(exclude_block)) {
        util::remove_phi_block(phi, block);
    }
}

void replace_conditional_br(std::shared_ptr<ir::Function> func) {
    for (const auto &block : func->get_basic_blocks_ref()) {
        for (const auto &inst : block->get_instructions_ref()) {
            auto br = std::dynamic_pointer_cast<ir::Br>(inst);
            if (!br || !br->is_cond_branch()) continue;
            auto cond_int = std::dynamic_pointer_cast<ir::ConstantInt>(br->get_operands_ref()[0]);
            if (!cond_int) continue;
            if (cond_int->get_val() == 1) {
                auto true_target = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[1]);
                unconditional_redirect(br, true_target);
            } else if (cond_int->get_val() == 0) {
                auto false_target = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[2]);
                unconditional_redirect(br, false_target);
            } else {
                __builtin_unreachable();
            }
        }
    }
}

}  // namespace opt::pass
