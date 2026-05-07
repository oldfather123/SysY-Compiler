// This file contains the support structures for the optimization passes.
#ifndef GEECEECEE_IR_SUPPORT_HPP
#define GEECEECEE_IR_SUPPORT_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace std {
template <typename T>
struct hash<weak_ptr<T>> {
    size_t operator()(const weak_ptr<T> &wp) const {
        auto sp = wp.lock();
        return sp ? hash<shared_ptr<T>>{}(sp) : 0;
    }
};

template <typename T>
struct less<weak_ptr<T>> {
    bool operator()(const weak_ptr<T> &lhs, const weak_ptr<T> &rhs) const {
        auto lhs_sp = lhs.lock();
        auto rhs_sp = rhs.lock();
        if (!lhs_sp && !rhs_sp) return false;
        if (!lhs_sp) return true;
        if (!rhs_sp) return false;
        return lhs_sp < rhs_sp;
    }
};
}  // namespace std

namespace ir {
class Value;
class Function;
class BasicBlock;
class Instruction;
class ConstantInt;
}  // namespace ir

namespace ir::opt_support {
class IntegerRange {
    int64_t max_val, min_val;
    uint32_t zeros, ones;

public:
    explicit IntegerRange();
    explicit IntegerRange(int val);
    explicit IntegerRange(std::shared_ptr<ir::ConstantInt> cint);
    [[nodiscard]] static IntegerRange get_non_negative();

    void set_range(int64_t min_val, int64_t max_val);
    void set_known_bits(uint32_t zeros, uint32_t ones);
    [[nodiscard]] uint32_t known_zeros() const { return zeros; }
    [[nodiscard]] uint32_t known_ones() const { return ones; }
    [[nodiscard]] int64_t max() const { return max_val; }
    [[nodiscard]] int64_t min() const { return min_val; }
    [[nodiscard]] bool is_any() const { return *this == IntegerRange(); }
    [[nodiscard]] bool intersect_with(const IntegerRange &other) const;

    void sync();
    [[nodiscard]] IntegerRange intersect_set(const IntegerRange &rhs) const;
    [[nodiscard]] IntegerRange union_set(const IntegerRange &rhs) const;
    [[nodiscard]] std::pair<uint32_t, uint32_t> infer_suffix() const;
    [[nodiscard]] bool is_non_negative() const;

    [[nodiscard]] bool operator==(const IntegerRange &rhs) const;
    [[nodiscard]] bool operator!=(const IntegerRange &rhs) const;
    [[nodiscard]] IntegerRange operator+(const IntegerRange &rhs) const;
    [[nodiscard]] IntegerRange operator-(const IntegerRange &rhs) const;
    [[nodiscard]] IntegerRange operator*(const IntegerRange &rhs) const;
    [[nodiscard]] IntegerRange operator/(const IntegerRange &rhs) const;
    [[nodiscard]] IntegerRange operator%(const IntegerRange &rhs) const;

    friend std::ostream &operator<<(std::ostream &os, const IntegerRange &range);
};

struct IntegerRangeInfo {
    std::unordered_map<std::shared_ptr<ir::Value>, IntegerRange> ranges;
    std::unordered_map<std::shared_ptr<ir::Value>, std::unordered_map<std::shared_ptr<ir::BasicBlock>, IntegerRange>>
        block_ctx_ranges;
    std::unordered_map<std::shared_ptr<ir::Value>, std::unordered_map<std::shared_ptr<ir::Instruction>, IntegerRange>>
        inst_ctx_ranges;

    IntegerRange query(std::shared_ptr<ir::Value> val, std::shared_ptr<ir::Instruction> ctx, uint32_t depth) const;

    void print_block_ctx_range(std::shared_ptr<ir::Value> val) const;
    void print_range(std::shared_ptr<ir::Value> val) const;
    void print_all_ranges() const;
};
}  // namespace ir::opt_support

namespace ir::opt_support {
class AliasInfo {
    std::unordered_set<uint64_t> distinct_pairs;
    std::vector<std::unordered_set<uint32_t>> distinct_groups;
    std::unordered_map<std::shared_ptr<ir::Value>, std::vector<uint32_t>> pointer_attrs;
    std::vector<uint32_t> empty;

public:
    void add_pair(uint32_t attr1, uint32_t attr2);
    void add_distinct_group(std::unordered_set<uint32_t> group);
    void add_value(std::shared_ptr<ir::Value> p, std::vector<uint32_t> attrs);
    bool is_distinct(std::shared_ptr<ir::Value> p1, std::shared_ptr<ir::Value> p2) const;
    const std::vector<uint32_t> &inherit_from(std::shared_ptr<ir::Value> ptr) const;
    bool append_attr(std::shared_ptr<ir::Value> p, const std::vector<uint32_t> &new_attrs);
    bool append_attr(std::shared_ptr<ir::Value> p, uint32_t new_attr);
    void clear();
    const std::unordered_map<std::shared_ptr<ir::Value>, std::vector<uint32_t>> &get_pointer_attrs() const;
};
}  // namespace ir::opt_support

namespace ir::opt_support {
struct IndVarInfo {
    std::shared_ptr<ir::Value> ind_var;  // header's arg
    std::shared_ptr<ir::Value> init;
    std::shared_ptr<ir::Value> bound;
    std::shared_ptr<ir::Value> alu;
    enum CondType { LE, LT, GE, GT, EQ, NE } cond_type;
    std::shared_ptr<ir::Value> step;

    friend std::ostream &operator<<(std::ostream &os, const IndVarInfo &info);
};

struct LoopInfo {
    // The unique Basic Block that dominates all Basic Blocks in the Loop. This Basic Block is called Header
    std::shared_ptr<ir::BasicBlock> header;
    // Basic Blocks that have Header as their successor, i.e., blocks that jump to the loop header
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> latches;
    // The first Basic Block reached after exiting the loop
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> exits;
    // Blocks that are about to exit the loop, i.e., Exiting blocks can jump to Exit blocks
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> exitings;
    // All Basic Blocks contained in the loop
    std::vector<std::shared_ptr<ir::BasicBlock>> blocks;
    // extry blocks that can jump to the loop header
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> enterings;
    // if the loop has only one entring, it's the pre-header
    std::shared_ptr<ir::BasicBlock> pre_header;
    // Sub-loops
    std::vector<std::shared_ptr<LoopInfo>> children;
    // Parent loop
    std::weak_ptr<LoopInfo> parent;
    // Depth of the loop
    int depth;
    // induction var info
    std::vector<IndVarInfo> ind_vars;
    // times the loop will be executed
    std::optional<int> trip_count;

    // recursive judge if given block is in the loop
    [[nodiscard]] bool contains(std::shared_ptr<ir::BasicBlock> block) const;
    // recusive search all blocks in the loop
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> all_blocks() const;
    // determine if the loop defines a value
    [[nodiscard]] bool defined(std::shared_ptr<ir::Value> value) const;
    // get the first latch block, assume the loop has been simplified
    [[nodiscard]] std::shared_ptr<ir::BasicBlock> latch() const { return *latches.begin(); }
    // trick count * instructions count
    [[nodiscard]] std::optional<int> size() const;

    friend std::ostream &operator<<(std::ostream &os, const LoopInfo &loop);
};
class LoopForest {
    std::vector<std::shared_ptr<LoopInfo>> top_loops;

public:
    void clear() { top_loops.clear(); }
    void add_top_loop(std::shared_ptr<LoopInfo> loop) { top_loops.push_back(loop); }

    class LoopIterator {
    private:
        std::vector<std::shared_ptr<LoopInfo>> stack;
        std::unordered_set<std::shared_ptr<LoopInfo>> visited;

    public:
        LoopIterator() = default;

        explicit LoopIterator(const std::vector<std::shared_ptr<LoopInfo>> &top_loops) {
            for (auto it = top_loops.rbegin(); it != top_loops.rend(); ++it) {
                stack.push_back(*it);
            }
        }
        std::shared_ptr<LoopInfo> operator*() const {
            if (stack.empty()) {
                return nullptr;
            }
            return stack.back();
        }
        std::shared_ptr<LoopInfo> operator->() const { return operator*(); }
        LoopIterator &operator++() {
            if (stack.empty()) return *this;
            auto current = stack.back();
            stack.pop_back();
            if (visited.find(current) == visited.end()) {
                visited.insert(current);
                for (auto it = current->children.rbegin(); it != current->children.rend(); ++it) {
                    stack.push_back(*it);
                }
            }
            return *this;
        }
        LoopIterator operator++(int) {
            LoopIterator temp = *this;
            ++(*this);
            return temp;
        }
        bool operator==(const LoopIterator &other) const { return stack.empty() && other.stack.empty(); }
        bool operator!=(const LoopIterator &other) const { return !(*this == other); }
    };
    LoopIterator begin() const { return LoopIterator(top_loops); }
    LoopIterator end() const { return LoopIterator(); }
    size_t size() const { return top_loops.size(); }
    bool empty() const { return top_loops.empty(); }
};
}  // namespace ir::opt_support

namespace ir::opt_support {
struct SCEVExpr {
    enum SCEVType { CONSTANT, ADD_REC } type;
    std::variant<int, std::vector<std::shared_ptr<SCEVExpr>>> value;
    std::weak_ptr<LoopInfo> loop;

    // helper function attributes, decode variant
    std::optional<int> constant() const;
    std::optional<std::vector<std::shared_ptr<SCEVExpr>>> operands() const;
    std::optional<std::reference_wrapper<std::vector<std::shared_ptr<SCEVExpr>>>> operands_ref();

    std::shared_ptr<SCEVExpr> clone() const;

    SCEVExpr(SCEVType type,
             std::variant<int, std::vector<std::shared_ptr<SCEVExpr>>> value,
             std::shared_ptr<ir::opt_support::LoopInfo> loop = nullptr)
        : type(type), value(value), loop(loop) {}
    friend std::ostream &operator<<(std::ostream &os, const SCEVExpr &expr);
};

class SCEVInfo {
    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<SCEVExpr>> expr_map;

public:
    std::optional<std::shared_ptr<SCEVExpr>> query(std::shared_ptr<ir::Value> value);
    void add_expr(std::shared_ptr<ir::Value> value, std::shared_ptr<SCEVExpr> expr) { expr_map[value] = expr; }
    bool contains(std::shared_ptr<ir::Value> value) { return expr_map.find(value) != expr_map.end(); }
    friend std::ostream &operator<<(std::ostream &os, const SCEVInfo &info);
};
}  // namespace ir::opt_support

namespace ir::opt_support {
struct PointerBaseInfo {
    void put(std::shared_ptr<ir::Value> ptr1, std::shared_ptr<ir::Value> ptr2) { base_map[ptr1] = ptr2; }
    [[nodiscard]] bool contains(std::shared_ptr<ir::Value> val) { return base_map.count(val); }
    [[nodiscard]] std::optional<std::shared_ptr<ir::Value>> query(std::shared_ptr<ir::Value> val) {
        if (!contains(val)) return std::nullopt;
        return base_map.at(val);
    }
    void clear() { base_map.clear(); }
    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> map() { return base_map; }

private:
    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> base_map;
};
}  // namespace ir::opt_support

namespace ir::opt_support {
struct FunctionOptInfo {
    ir::Function *parent_func;

    // marked by FunctionAnalysis
    bool has_inout = false;
    bool has_global_memory_read = false;
    bool has_global_memory_write = false;
    bool has_local_memory_write = false;
    // marked by PureFunctionAnalysis
    bool is_pure = false;
    // marked by PureFunctionAnalysis, the global vars that this function affects
    std::unordered_set<std::shared_ptr<ir::Value>> effected_global_vars{};
    // marked by SideEffectAnalyzation
    bool has_side_effect = true;
    // the functions that this function calls
    std::set<std::weak_ptr<ir::Function>> callees{};
    // the functions that call this function
    std::set<std::weak_ptr<ir::Function>> callers{};
    // marked by CallGraphAnalysis
    bool is_recursive = false;
    // loop forest contains all top-level loops
    LoopForest loop_forest{};
};

struct BasicBlockOptInfo {
    ir::BasicBlock *parent_block;

    // control flow graph
    std::vector<std::weak_ptr<BasicBlock>> successors{};
    std::vector<std::weak_ptr<BasicBlock>> predecessors{};

    // dominance
    std::vector<std::weak_ptr<BasicBlock>> dominated{};
    std::vector<std::weak_ptr<BasicBlock>> dominator{};
    std::weak_ptr<BasicBlock> immediate_dominator{};
    std::vector<std::weak_ptr<BasicBlock>> immediate_dominated{};
    std::optional<int> dominance_depth{};
    std::vector<std::weak_ptr<BasicBlock>> dominance_frontier{};

    // loop
    std::weak_ptr<LoopInfo> loop{};
    bool is_loop_header = false;

    void remove_successor(const std::shared_ptr<ir::BasicBlock> &successor);
    void remove_predecessor(const std::shared_ptr<ir::BasicBlock> &predecessor);
    int get_loop_depth() const {
        if (loop.expired()) return 0;
        return loop.lock()->depth;
    }

    bool dominates(std::shared_ptr<BasicBlock> block);
};

struct ModuleOptInfo {
    AliasInfo alias_info;
    SCEVInfo scev_info;
    PointerBaseInfo pointer_base_info;
    IntegerRangeInfo int_range_info;
};

}  // namespace ir::opt_support

#endif  // GEECEECEE_IR_SUPPORT_HPP
