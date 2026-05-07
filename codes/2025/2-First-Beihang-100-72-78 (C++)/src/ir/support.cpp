#include "ir/support.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "ir/type.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "util.hpp"

namespace ir::opt_support {
constexpr bool is_overflow(int64_t x) {
    return x < std::numeric_limits<int32_t>::min() || x > std::numeric_limits<int32_t>::max();
}

void IntegerRange::set_range(int64_t min_val, int64_t max_val) {
    this->min_val = min_val;
    this->max_val = max_val;
}

void IntegerRange::set_known_bits(uint32_t zeros, uint32_t ones) {
    this->zeros = zeros;
    this->ones = ones;
}

IntegerRange IntegerRange::get_non_negative() {
    IntegerRange res;
    res.set_range(0, std::numeric_limits<int32_t>::max());
    res.sync();
    return res;
}

void IntegerRange::sync() {
    // 1. signed -> bit
    const auto common = static_cast<uint32_t>(min_val ^ max_val);
    if (common == 0) {
        ones |= static_cast<uint32_t>(min_val);
        zeros |= ~static_cast<uint32_t>(min_val);
    } else {
        const auto lcp_len = static_cast<uint32_t>(__builtin_clz(common));
        const auto mask = ~((0xffffffff << lcp_len) >> lcp_len);
        ones |= static_cast<uint32_t>(min_val) & mask;
        zeros |= (~static_cast<uint32_t>(max_val)) & mask;
    }

    // 2. bit -> signed
    // TODO(Xingkun)

    if (max_val < min_val) {
        logger::WARN("[IntegerRange::sync] max_val < min_val");
        *this = IntegerRange();
    }
}

bool IntegerRange::intersect_with(const IntegerRange &other) const {
    return std::max(min_val, other.min_val) <= std::min(max_val, other.max_val) &&
           (((ones | other.zeros) & (ones | other.ones)) == 0);
}

IntegerRange IntegerRange::intersect_set(const IntegerRange &rhs) const {
    IntegerRange res;
    res.set_range(std::max(min_val, rhs.min_val), std::min(max_val, rhs.max_val));
    res.set_known_bits(zeros | rhs.zeros, ones | rhs.ones);
    res.sync();
    return res;
}

IntegerRange IntegerRange::union_set(const IntegerRange &rhs) const {
    IntegerRange res;
    res.set_range(std::min(min_val, rhs.min_val), std::max(max_val, rhs.max_val));
    res.set_known_bits(zeros & rhs.zeros, ones & rhs.ones);
    res.sync();
    return res;
}

std::pair<uint32_t, uint32_t> IntegerRange::infer_suffix() const {
    const auto determined = ones | zeros;
    if (determined == 0xffffffff) return {ones, 32U};
    const auto len = __builtin_clz(~determined);
    return {ones & ((1U << len) - 1), len};
}

void IntegerRangeInfo::print_block_ctx_range(std::shared_ptr<ir::Value> val) const {
    std::cout << "val: " << val->to_string() << "  block ctx info:" << std::endl;
    auto it = block_ctx_ranges.find(val);
    if (it == block_ctx_ranges.end()) {
        std::cout << "none" << std::endl;
        return;
    }
    for (const auto &[block, range] : it->second) {
        std::cout << "block: " << block->get_name() << " range: " << range << std::endl << std::endl;
    }
}

void IntegerRangeInfo::print_range(std::shared_ptr<ir::Value> val) const {
    if (ranges.find(val) == ranges.end()) {
        std::cout << "val: " << val->to_string() << " range: none" << std::endl;
        return;
    }
    std::cout << "val: " << val->to_string() << " range: " << ranges.at(val) << std::endl;
}

void IntegerRangeInfo::print_all_ranges() const {
    for (const auto &[val, range] : ranges) {
        std::cout << "val: " << val->to_string() << " range: " << range << std::endl;
    }
}

IntegerRange IntegerRangeInfo::query(std::shared_ptr<ir::Value> val,
                                     std::shared_ptr<ir::Instruction> ctx,
                                     uint32_t depth) const {
    if (auto cint = std::dynamic_pointer_cast<ir::ConstantInt>(val)) {
        if (cint->get_type() == ir::IntegerType::get(32)) return IntegerRange(cint);
    }
    IntegerRange range;
    if (auto iter = ranges.find(val); iter != ranges.cend()) {
        range = iter->second;
    }
    if (ctx) {
        if (auto iter = inst_ctx_ranges.find(val); iter != inst_ctx_ranges.cend()) {
            const auto &ctx_ranges = iter->second;
            if (auto it = ctx_ranges.find(ctx); it != ctx_ranges.cend()) {
                range = range.intersect_set(it->second);
            }
        }
    }
    auto val_inst = std::dynamic_pointer_cast<ir::Instruction>(val);
    if (ctx && val_inst && ctx->get_parent_block().lock() != val_inst->get_parent_block().lock()) {
        if (auto iter = block_ctx_ranges.find(val); iter != block_ctx_ranges.cend()) {
            const auto &ctx_ranges = iter->second;
            if (ctx_ranges.size() <= depth) {
                for (const auto &[block, r] : ctx_ranges) {
                    if (block->opt_info.dominates(ctx->get_parent_block().lock())) {
                        range = range.intersect_set(r);
                    }
                }
            } else {
                auto block = ctx->get_parent_block().lock();
                for (uint32_t k = 0; k < depth; k++) {
                    if (auto it = ranges.find(block); it != ranges.end()) range = range.intersect_set(it->second);
                    block = block->opt_info.immediate_dominator.lock();
                    if (!block) break;
                }
            }
        }
    }
    return range;
}

bool IntegerRange::operator==(const IntegerRange &rhs) const {
    return min_val == rhs.min_val && max_val == rhs.max_val && zeros == rhs.zeros && ones == rhs.ones;
}

bool IntegerRange::operator!=(const IntegerRange &rhs) const { return !(*this == rhs); }

IntegerRange::IntegerRange()
    : max_val(std::numeric_limits<int>::max()), min_val(std::numeric_limits<int>::min()), zeros(0), ones(0) {}

IntegerRange::IntegerRange(int val) : max_val(val), min_val(val) {
    const auto zval = static_cast<uint32_t>(val);
    set_known_bits(~zval, zval);
}

bool IntegerRange::is_non_negative() const { return min_val >= 0; }

IntegerRange IntegerRange::operator+(const IntegerRange &rhs) const {
    if (zeros == 0xffffffff) return rhs;
    if (rhs.zeros == 0xffffffff) return *this;

    IntegerRange res;
    const auto low = min_val + rhs.min_val, high = max_val + rhs.max_val;
    if (!is_overflow(low) && !is_overflow(high)) res.set_range(low, high);
    res.set_known_bits((1U << std::min(__builtin_ctz(~zeros), __builtin_ctz(~rhs.zeros))) - 1U, 0);
    res.sync();
    return res;
}

IntegerRange IntegerRange::operator-(const IntegerRange &rhs) const {
    IntegerRange res;
    const auto low = min_val - rhs.max_val, high = max_val - rhs.min_val;
    if (!is_overflow(low) && !is_overflow(high)) res.set_range(low, high);
    res.sync();
    return res;
}

IntegerRange IntegerRange::operator*(const IntegerRange &rhs) const {
    IntegerRange res;
    const auto s1 = min_val * rhs.min_val, s2 = min_val * rhs.max_val, s3 = max_val * rhs.min_val,
               s4 = max_val * rhs.max_val;
    if (!is_overflow(s1) && !is_overflow(s2) && !is_overflow(s3) && !is_overflow(s4))
        res.set_range(std::min(std::min(s1, s2), std::min(s3, s4)), std::max(std::max(s1, s2), std::max(s3, s4)));

    // auto [lhs_suffix, lhs_suffix_len] = infer_suffix();
    // auto [rhs_suffix, rhs_suffix_len] = rhs.infer_suffix();

    // if (lhs_suffix_len && rhs_suffix_len) {
    //     const auto len = std::min(lhs_suffix_len, rhs_suffix_len);
    //     const auto mask = (len >= 32U ? 0xffffffff : ((1U << len) - 1));
    //     const auto val = lhs_suffix * rhs_suffix;
    //     res.set_known_bits((~val) & mask, val & mask);
    // } else if (lhs_suffix_len) {
    //     const auto len = lhs_suffix ? static_cast<uint32_t>(__builtin_clz(lhs_suffix)) : lhs_suffix_len;
    //     const auto mask = (len >= 32U ? 0xffffffff : ((1U << len) - 1));
    //     res.set_known_bits(mask, 0);
    // } else if (rhs_suffix_len) {
    //     const auto length = rhs_suffix ? static_cast<uint32_t>(__builtin_ctz(rhs_suffix)) : rhs_suffix_len;
    //     const auto mask = (length >= 32U ? 0xffffffff : ((1U << length) - 1));
    //     res.set_known_bits(mask, 0);
    // }
    // res.sync();
    return res;
}

IntegerRange IntegerRange::operator/(const IntegerRange &rhs) const {
    IntegerRange res;
    if (rhs.min_val == 0 && rhs.max_val == 0) return res;

    const auto wrap = [](int64_t &x) {
        if (x == -static_cast<int64_t>(std::numeric_limits<int32_t>::min())) x = std::numeric_limits<int32_t>::min();
    };
    if (rhs.min_val >= 0) {
        const auto s1 = min_val / std::max(static_cast<int64_t>(1), rhs.min_val),
                   s2 = min_val / std::max(static_cast<int64_t>(1), rhs.max_val),
                   s3 = max_val / std::max(static_cast<int64_t>(1), rhs.min_val),
                   s4 = max_val / std::max(static_cast<int64_t>(1), rhs.max_val);
        res.set_range(std::min(std::min(s1, s2), std::min(s3, s4)), std::max(std::max(s1, s2), std::max(s3, s4)));
    } else if (rhs.max_val <= 0) {
        auto s1 = min_val / std::min(static_cast<int64_t>(-1), rhs.min_val),
             s2 = min_val / std::min(static_cast<int64_t>(-1), rhs.max_val),
             s3 = max_val / std::min(static_cast<int64_t>(-1), rhs.min_val),
             s4 = max_val / std::min(static_cast<int64_t>(-1), rhs.max_val);
        wrap(s1);
        wrap(s2);
        wrap(s3);
        wrap(s4);
        res.set_range(std::min(std::min(s1, s2), std::min(s3, s4)), std::max(std::max(s1, s2), std::max(s3, s4)));
    } else {
        std::array<int64_t, 2> lhs = {min_val, max_val};
        std::array<int64_t, 4> rhs_list{rhs.min_val, -1, 1, rhs.max_val};
        int64_t min_v = res.max_val, max_v = res.min_val;
        for (auto lhs_val : lhs)
            for (auto rhs_val : rhs_list) {
                if (!rhs_val) continue;
                auto val = lhs_val / rhs_val;
                wrap(val);
                min_v = std::min(min_v, val);
                max_v = std::max(max_v, val);
            }
        res.set_range(min_v, max_v);
    }

    res.sync();
    return res;
}

IntegerRange IntegerRange::operator%(const IntegerRange &rhs) const {
    IntegerRange ret;

    if (rhs.min_val == 0 && rhs.max_val == 0) return ret;

    if (rhs.min_val >= 0) {
        if (is_non_negative())
            ret.set_range(0, rhs.max_val - 1);
        else
            ret.set_range(-rhs.max_val + 1, rhs.max_val - 1);
        if (min_val != std::numeric_limits<int32_t>::min()) {
            const auto max_range = std::max(std::abs(max_val), std::abs(min_val));
            if (max_range < rhs.max_val) {
                ret.min_val = std::max(ret.min_val, -max_range);
                ret.max_val = std::min(ret.max_val, max_range);
            }
        }
    } else if (rhs.max_val <= 0) {
        ret.set_range(rhs.min_val + 1, -rhs.min_val - 1);
    } else {
        const auto val = std::max(-rhs.min_val, rhs.max_val);
        ret.set_range(-val + 1, val - 1);
    }

    ret.sync();
    return ret;
}

IntegerRange::IntegerRange(std::shared_ptr<ir::ConstantInt> cint) : IntegerRange(cint->get_val()) {}

std::ostream &operator<<(std::ostream &os, const IntegerRange &range) {
    os << "[" << range.min_val << ", " << range.max_val << "]";
    return os;
}

}  // namespace ir::opt_support

namespace ir::opt_support {
static uint64_t encode(uint32_t v1, uint32_t v2) {
    assert(v1 != v2);
    if (v1 < v2) std::swap(v1, v2);
    return (static_cast<uint64_t>(v1) << 32) | v2;
}

void AliasInfo::add_pair(uint32_t attr1, uint32_t attr2) { distinct_pairs.insert(encode(attr1, attr2)); }

void AliasInfo::add_distinct_group(std::unordered_set<uint32_t> group) {
    if (!group.empty()) distinct_groups.push_back(std::move(group));
}

void AliasInfo::add_value(std::shared_ptr<ir::Value> p, std::vector<uint32_t> attrs) {
    pointer_attrs.emplace(p, std::move(attrs));
}

bool AliasInfo::is_distinct(std::shared_ptr<ir::Value> p1, std::shared_ptr<ir::Value> p2) const {
    if (!pointer_attrs.count(p1) || !pointer_attrs.count(p2)) return false;
    if (p1 == p2) return false;
    const auto &attr1 = pointer_attrs.at(p1);
    const auto &attr2 = pointer_attrs.at(p2);
    for (auto attr_x : attr1) {
        for (auto attr_y : attr2) {
            if (attr_x == attr_y) continue;
            if (distinct_pairs.count(encode(attr_x, attr_y))) return true;
            for (const auto &group : distinct_groups) {
                if (group.count(attr_x) && group.count(attr_y)) return true;
            }
        }
    }
    return false;
}

const std::vector<uint32_t> &AliasInfo::inherit_from(std::shared_ptr<ir::Value> ptr) const {
    if (!pointer_attrs.count(ptr)) return empty;
    return pointer_attrs.at(ptr);
}

bool AliasInfo::append_attr(std::shared_ptr<ir::Value> p, const std::vector<uint32_t> &new_attrs) {
    if (new_attrs.empty()) return false;
    assert(pointer_attrs.count(p));
    auto &attrs = pointer_attrs.at(p);
    const auto old_size = attrs.size();
    attrs.insert(attrs.cend(), new_attrs.cbegin(), new_attrs.cend());
    std::sort(attrs.begin(), attrs.end());
    attrs.erase(std::unique(attrs.begin(), attrs.end()), attrs.end());
    return attrs.size() != old_size;
}

bool AliasInfo::append_attr(std::shared_ptr<ir::Value> p, uint32_t new_attr) {
    assert(pointer_attrs.count(p));
    auto &attrs = pointer_attrs.at(p);
    if (std::find(attrs.cbegin(), attrs.cend(), new_attr) == attrs.end()) {
        attrs.push_back(new_attr);
        return true;
    }
    return false;
}

void AliasInfo::clear() {
    distinct_pairs.clear();
    distinct_groups.clear();
    pointer_attrs.clear();
}

const std::unordered_map<std::shared_ptr<ir::Value>, std::vector<uint32_t>> &AliasInfo::get_pointer_attrs() const {
    return pointer_attrs;
}

}  // namespace ir::opt_support

namespace ir::opt_support {
std::optional<int> SCEVExpr::constant() const {
    if (std::holds_alternative<int>(value)) {
        return std::get<int>(value);
    }
    return std::nullopt;
}

std::optional<std::vector<std::shared_ptr<SCEVExpr>>> SCEVExpr::operands() const {
    if (std::holds_alternative<std::vector<std::shared_ptr<SCEVExpr>>>(value)) {
        return std::get<std::vector<std::shared_ptr<SCEVExpr>>>(value);
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<std::vector<std::shared_ptr<SCEVExpr>>>> SCEVExpr::operands_ref() {
    if (std::holds_alternative<std::vector<std::shared_ptr<SCEVExpr>>>(value)) {
        return std::ref(std::get<std::vector<std::shared_ptr<SCEVExpr>>>(value));
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<SCEVExpr>> SCEVInfo::query(std::shared_ptr<ir::Value> value) {
    if (!expr_map.count(value)) {
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(value)) {
            expr_map[value] = std::make_shared<SCEVExpr>(SCEVExpr::SCEVType::CONSTANT, const_int->get_val());
        } else {
            return std::nullopt;
        }
    }
    return expr_map.at(value);
}

std::shared_ptr<SCEVExpr> SCEVExpr::clone() const {
    std::variant<int, std::vector<std::shared_ptr<SCEVExpr>>> cloned_value;
    if (std::holds_alternative<int>(value)) {
        cloned_value = constant().value();
    } else {
        auto ori_operands = operands().value();
        std::vector<std::shared_ptr<SCEVExpr>> cloned_operands;
        cloned_operands.reserve(ori_operands.size());
        for (const auto &operand : ori_operands) {
            cloned_operands.push_back(operand->clone());
        }
        cloned_value = std::move(cloned_operands);
    }
    return std::make_shared<SCEVExpr>(type, std::move(cloned_value), loop.lock());
}

std::ostream &operator<<(std::ostream &os, const SCEVExpr &expr) {
    if (auto constant = expr.constant()) {
        os << "constant " << constant.value();
    } else {
        os << "AddRec{";
        auto operands = expr.operands().value();
        for (size_t i = 0; i < operands.size(); ++i) {
            os << *operands[i];
            if (i < operands.size() - 1) {
                os << " + ";
            }
        }
        os << "}";
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const SCEVInfo &info) {
    for (const auto &[value, expr] : info.expr_map) {
        os << value->get_name() << " -> " << *expr << std::endl;
    }
    return os;
}

}  // namespace ir::opt_support

namespace ir::opt_support {
bool LoopInfo::contains(std::shared_ptr<ir::BasicBlock> block) const {
    if (std::find(blocks.begin(), blocks.end(), block) != blocks.end()) return true;
    for (const auto &child : children) {
        if (child->contains(block)) return true;
    }
    return false;
}

std::unordered_set<std::shared_ptr<ir::BasicBlock>> LoopInfo::all_blocks() const {
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> blocks;
    blocks.insert(this->blocks.begin(), this->blocks.end());
    for (const auto &child : children) {
        auto child_blocks = child->all_blocks();
        // FIXME: remove this assertion
        for (const auto &child_block : child_blocks) {
            assert(blocks.find(child_block) == blocks.end() && "block in child loop should not be in parent loop");
        }
        blocks.insert(child_blocks.begin(), child_blocks.end());
    }
    return blocks;
}

std::optional<int> LoopInfo::size() const {
    if (!trip_count.has_value()) return std::nullopt;
    int size = 0;
    for (const auto &block : all_blocks()) {
        size += static_cast<int>(block->get_instructions_ref().size());
    }
    return trip_count.value() * size;
}

bool LoopInfo::defined(std::shared_ptr<ir::Value> value) const {
    if (std::dynamic_pointer_cast<ir::Constant>(value)) return false;
    if (std::dynamic_pointer_cast<ir::Argument>(value)) return false;
    assert(std::dynamic_pointer_cast<ir::Instruction>(value));
    return this->contains((std::dynamic_pointer_cast<ir::Instruction>(value))->get_parent_block().lock());
}

std::ostream &operator<<(std::ostream &os, const LoopInfo &loop) {
    os << "LoopInfo: " << std::endl;
    os << "preheader: " << loop.pre_header->get_name() << std::endl;
    os << "header: " << loop.header->get_name() << std::endl;
    os << "latches: ";
    for (const auto &latch : loop.latches) {
        os << latch->get_name() << " ";
    }
    os << std::endl;
    os << "exits: ";
    for (const auto &exit : loop.exits) {
        os << exit->get_name() << " ";
    }
    os << std::endl;
    os << "blocks: ";
    for (const auto &block : loop.blocks) {
        os << block->get_name() << " ";
    }
    os << std::endl;
    return os;
}

std::ostream &operator<<(std::ostream &os, const IndVarInfo &info) {
    os << "ind_var: " << info.ind_var->get_name() << std::endl;
    os << "init: " << info.init->get_name() << std::endl;
    os << "bound: " << info.bound->get_name() << std::endl;
    os << "alu: " << info.alu->get_name() << std::endl;
    os << "cond_type: " << info.cond_type << std::endl;
    os << "step: " << info.step->get_name() << std::endl;
    return os;
}

void BasicBlockOptInfo::remove_successor(const std::shared_ptr<ir::BasicBlock> &successor) {
    successors.erase(std::remove_if(successors.begin(),
                                    successors.end(),
                                    [&](const std::weak_ptr<BasicBlock> &weak_successor) {
                                        return weak_successor.expired() || weak_successor.lock() == successor;
                                    }),
                     successors.end());
}

void BasicBlockOptInfo::remove_predecessor(const std::shared_ptr<ir::BasicBlock> &predecessor) {
    predecessors.erase(std::remove_if(predecessors.begin(),
                                      predecessors.end(),
                                      [&](const std::weak_ptr<BasicBlock> &weak_predecessor) {
                                          return weak_predecessor.expired() || weak_predecessor.lock() == predecessor;
                                      }),
                       predecessors.end());
}
bool BasicBlockOptInfo::dominates(std::shared_ptr<BasicBlock> block) {
    auto shared_dominated = to_shared_vector(dominated);
    return std::find(shared_dominated.begin(), shared_dominated.end(), block) != shared_dominated.end();
}
}  // namespace ir::opt_support
