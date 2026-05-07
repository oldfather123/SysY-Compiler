#include <memory>
#include <optional>
#include <unordered_map>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool modified = false;
static ir::opt_support::IntegerRangeInfo *int_range_info;

static void fold_block(std::shared_ptr<ir::BasicBlock> block);
extern void replace_conditional_br(std::shared_ptr<ir::Function> func);

bool RangeFolding::run(ir::Module *module) {
    logger::INFO("Running RangeFolding...");
    modified = false;
    int_range_info = &module->opt_info.int_range_info;
    // for (auto &[val, range] : int_range_info->ranges) {
    //     if (!range.is_any()) std::cout << val->to_string() << " : " << range << std::endl;
    // }
    module->for_each_func([](auto &func) {
        func->for_each_block([](auto &block) { fold_block(block); });
        replace_conditional_br(func);
    });

    return modified;
}

static std::optional<std::shared_ptr<ir::Value>> fold_icmp(std::shared_ptr<ir::ICmp> icmp) {
    static std::shared_ptr<ir::ConstantInt> true_val = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), 1);
    static std::shared_ptr<ir::ConstantInt> false_val = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), 0);

    using ICmpType = ir::ICmp::ICmpType;
    auto lhs = int_range_info->query(icmp->get_operands_ref()[0], icmp, 8);
    auto rhs = int_range_info->query(icmp->get_operands_ref()[1], icmp, 8);

    switch (icmp->get_cmp_type()) {
        case ICmpType::EQ: {
            if (!lhs.intersect_with(rhs)) return false_val;
            break;
        }
        case ICmpType::NE: {
            if (!lhs.intersect_with(rhs)) return true_val;
            break;
        }
        case ICmpType::SLT: {
            if (lhs.max() < rhs.min()) return true_val;
            if (lhs.min() >= rhs.max()) return false_val;
            break;
        }
        case ICmpType::SLE: {
            if (lhs.max() <= rhs.min()) return true_val;
            if (lhs.min() > rhs.max()) return false_val;
            break;
        }
        case ICmpType::SGT: {
            if (lhs.min() > rhs.max()) return true_val;
            if (lhs.max() <= rhs.min()) return false_val;
            break;
        }
        case ICmpType::SGE: {
            if (lhs.min() >= rhs.max()) return true_val;
            if (lhs.max() < rhs.min()) return false_val;
            break;
        }
        default:
            __builtin_unreachable();
    }
    return std::nullopt;
}

static void fold_block(std::shared_ptr<ir::BasicBlock> block) {
    std::unordered_map<std::shared_ptr<ir::Instruction>, std::shared_ptr<ir::Value>> replacement_map;

    for (const auto &inst : block->get_instructions()) {
        if (!inst->get_type()->is_integer_ty()) continue;

        // 1. fold [a, a] -> a
        auto range = int_range_info->query(inst, inst, 8);
        if (range.min() == range.max()) {
            replacement_map[inst] = std::make_shared<ir::ConstantInt>(inst->get_type(), range.min());
            continue;
        }

        // 2. fold icmp
        if (auto icmp = std::dynamic_pointer_cast<ir::ICmp>(inst)) {
            if (auto res = fold_icmp(icmp); res.has_value()) replacement_map[inst] = *res;
        }
    }

    for (const auto &[inst, replacement] : replacement_map) {
        util::substitute(inst, replacement);
    }
}

}  // namespace opt::pass
