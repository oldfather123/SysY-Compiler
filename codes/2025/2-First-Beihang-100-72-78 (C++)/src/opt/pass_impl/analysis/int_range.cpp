#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
using namespace ir::opt_support;

static IntegerRangeInfo *int_range_info;
static SCEVInfo *scev_info;
static PointerBaseInfo *pointer_base_info;

const int MAX_ITER_TIME = 32;
const int MAX_STEP = 65536;
std::unordered_map<std::shared_ptr<ir::Value>, int> enqueue_cnt;
std::unordered_set<std::shared_ptr<ir::Instruction>> in_queue;
std::queue<std::shared_ptr<ir::Instruction>> worklist;

static void analyze_func(std::shared_ptr<ir::Function> func);
static IntegerRange analyze_arg(std::shared_ptr<ir::Argument> arg);
static void iter_by_scev(std::shared_ptr<ir::Instruction> inst);
static void iter_by_inst(std::shared_ptr<ir::Instruction> inst);

bool IntegerRangeAnalyzation::run(ir::Module *module) {
    logger::INFO("Running IntegerRangeAnalyzation...");
    int_range_info = &module->opt_info.int_range_info;
    scev_info = &module->opt_info.scev_info;
    pointer_base_info = &module->opt_info.pointer_base_info;
    int_range_info->ranges.clear();
    int_range_info->block_ctx_ranges.clear();
    int_range_info->inst_ctx_ranges.clear();
    module->for_each_func([](auto &func) { analyze_func(func); });

    return false;
}

static void analyze_func(std::shared_ptr<ir::Function> func) {
    for (auto &arg : func->get_arguments_ref()) {
        if (arg->get_type() == ir::IntegerType::get(32)) {
            int_range_info->ranges[arg] = analyze_arg(arg);
        }
    }

    for (const auto &block : func->get_basic_blocks_ref()) {
        for (const auto &inst : block->get_instructions_ref()) {
            if (inst->get_type() == ir::IntegerType::get(32)) {
                int_range_info->ranges[inst] = IntegerRange{};
            }
            worklist.push(inst);
            in_queue.insert(inst);
        }
    }

    while (!worklist.empty()) {
        auto inst = worklist.front();
        worklist.pop();
        // std::cout << "caling inst: " << inst->to_string() << std::endl;
        in_queue.erase(inst);
        iter_by_scev(inst);
        iter_by_inst(inst);
        // std::cout << "------------------------ after update ------------------------" << std::endl;
        // std::cout << "******************** range: ";
        // int_range_info->print_range(inst);
        // std::cout << "\n&&&&&&&&&&&&&&&&&&&& block ctx range: ";
        // int_range_info->print_block_ctx_range(inst);
        // std::cout << "\n@@@@@@@@@@@@@@@@@@@@ all ranges: ";
        // int_range_info->print_all_ranges();
        // std::cout << "\n\n\n";
    }

    enqueue_cnt.clear();
    in_queue.clear();

    // for (auto &[val, range] : int_range_info->ranges) {
    //     if (!range.is_any()) std::cout << val->to_string() << " : " << range << std::endl;
    // }
}

static void update(std::shared_ptr<ir::Instruction> inst, const IntegerRange &new_range) {
    if (inst->get_type() != ir::IntegerType::get(32)) return;

    auto &range = int_range_info->ranges[inst];
    const auto intersection = range.intersect_set(new_range);
    if (range != intersection) {
        range = intersection;
        for (auto &weak_user : inst->get_users_ref()) {
            auto user = std::dynamic_pointer_cast<ir::Instruction>(weak_user.lock());
            if (!in_queue.count(user) && ++enqueue_cnt[user] < MAX_ITER_TIME) {
                worklist.push(user);
                in_queue.insert(user);
            }
        }
    }
}

static void update_block_ctx(std::shared_ptr<ir::Value> value,
                             std::shared_ptr<ir::BasicBlock> block,
                             const IntegerRange &new_range) {
    if (value->get_type() != ir::IntegerType::get(32)) return;
    if (new_range.is_any()) return;
    if (std::dynamic_pointer_cast<ir::Constant>(value) || std::dynamic_pointer_cast<ir::Argument>(value))
        return;  // not trackable
    auto inst = std::dynamic_pointer_cast<ir::Instruction>(value);
    if (inst && inst->get_parent_block().lock() == block) {
        update(inst, new_range);
        return;
    }

    auto &range = int_range_info->block_ctx_ranges[value][block];
    const auto intersection = range.intersect_set(new_range);
    if (range != intersection) {
        range = intersection;
        for (auto &weak_user : inst->get_users_ref()) {
            auto user = std::dynamic_pointer_cast<ir::Instruction>(weak_user.lock());
            if (block->opt_info.dominates(user->get_parent_block().lock()) && !in_queue.count(user) &&
                ++enqueue_cnt[user] < MAX_ITER_TIME) {
                worklist.push(user);
                in_queue.insert(user);
            }
        }
    }
}

static IntegerRange analyze_arg(std::shared_ptr<ir::Argument> arg) {
    // TODO
    return IntegerRange{};
}

static void iter_by_scev(std::shared_ptr<ir::Instruction> inst) {
    using SCEVType = SCEVExpr::SCEVType;
    auto s = scev_info->query(inst);
    if (!s.has_value()) return;
    auto scev = s.value();
    switch (scev->type) {
        case SCEVType::CONSTANT: {
            update(inst, IntegerRange{scev->constant().value()});
            break;
        }
        case SCEVType::ADD_REC: {
            auto operands = scev->operands().value();
            if (operands.size() == 2) {
                const auto initial = operands[0];
                const auto step = operands[1];
                bool cond1 = initial->type == SCEVType::CONSTANT && step->type == SCEVType::CONSTANT;
                if (!cond1) break;
                bool cond2 = std::abs(step->constant().value()) < MAX_STEP;
                bool cond3 = std::numeric_limits<int32_t>::min() <= initial->constant().value() &&
                             initial->constant().value() <= std::numeric_limits<int32_t>::max();
                if (cond2 && cond3) {
                    IntegerRange add_rec_range;
                    if (step->constant().value() > 0)
                        add_rec_range.set_range(initial->constant().value(), std::numeric_limits<int32_t>::max());
                    else
                        add_rec_range.set_range(std::numeric_limits<int32_t>::min(), initial->constant().value());
                    add_rec_range.sync();
                    update(inst, add_rec_range);
                }
            }
            break;
        }
    }
}

static void iter_by_inst(std::shared_ptr<ir::Instruction> inst) {
    using InstructionType = ir::Instruction::InstructionType;

    constexpr uint32_t max_depth = 8;
    auto get_range = [](std::shared_ptr<ir::Value> val, std::shared_ptr<ir::Instruction> ctx) {
        return int_range_info->query(val, ctx, max_depth);
    };
    auto get_operand_range = [&](uint32_t idx) { return get_range(inst->get_operands_ref()[idx], inst); };

    switch (inst->get_ins_type()) {
        case InstructionType::ADD: {
            update(inst, get_operand_range(0) + get_operand_range(1));
            break;
        }
        case InstructionType::SUB: {
            update(inst, get_operand_range(0) - get_operand_range(1));
            break;
        }
        case InstructionType::MUL: {
            update(inst, get_operand_range(0) * get_operand_range(1));
            break;
        }
        case InstructionType::SDIV: {
            update(inst, get_operand_range(0) / get_operand_range(1));
            break;
        }
        case InstructionType::SREM: {
            update(inst, get_operand_range(0) % get_operand_range(1));
            break;
        }
        case InstructionType::PHI: {
            auto phi = std::dynamic_pointer_cast<ir::Phi>(inst);
            std::optional<IntegerRange> range;
            for (const auto &operand : util::get_phi_values(phi)) {
                const auto sub_range = get_range(operand, inst);
                if (range)
                    *range = range->union_set(sub_range);
                else
                    range = sub_range;
            }
            if (range.has_value()) update(inst, *range);

            range.reset();
            std::vector<std::shared_ptr<ir::Instruction>> users;
            for (const auto &operand : util::get_phi_values(phi)) {
                bool is_user = false;
                for (auto &weak_user : inst->get_users_ref()) {
                    if (weak_user.lock() == operand) {
                        is_user = true;
                        break;
                    }
                }
                if (is_user)
                    users.push_back(std::dynamic_pointer_cast<ir::Instruction>(operand));
                else {
                    const auto sub_range = get_range(operand, inst);
                    if (range)
                        *range = range->union_set(sub_range);
                    else
                        range = sub_range;
                }
            }

            if (range.has_value() && users.size() == 1) {
                auto add = std::dynamic_pointer_cast<ir::Add>(users.front());
                if (!add) goto end;
                auto match_res = util::match_binary_operands<ir::Phi, ir::ConstantInt>(add);
                if (!match_res.has_value()) goto end;
                auto [phi_operand, const_int] = match_res.value();
                if (phi_operand != inst) goto end;
                const auto inc = const_int->get_val();
                const auto mask = (inc & -inc) - 1;
                const auto bask_mask = ((~range->known_zeros()) & -(~range->known_zeros())) - 1;
                const auto common_mask = mask & bask_mask;
                if (common_mask) {
                    IntegerRange r;
                    r.set_known_bits(static_cast<uint32_t>(common_mask), 0);
                    r.sync();
                    update(inst, r);
                }
            }
        end:
            break;
        }
        case InstructionType::ZEXT: {
            // FIXME: maybe wrong
            const auto width = inst->get_operands_ref()[0]->get_type()->bits_num();
            const auto max_v = (1LL << width) - 1;
            IntegerRange range;
            range.set_range(0, max_v);
            range.sync();

            update(inst, range);
            break;
        }
        case InstructionType::GETELEMENTPTR: {
            auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst);
            auto type = gep->base_ptr()->get_type();
            for (auto index : gep->get_indexes()) {
                if (index->get_type()->is_integer_ty() && type->is_array_ty() &&
                    !std::dynamic_pointer_cast<ir::ConstantInt>(index)) {
                    IntegerRange range;
                    const auto arr_type = std::dynamic_pointer_cast<ir::ArrayType>(type);
                    const auto size = arr_type->get_size();
                    range.set_range(static_cast<int32_t>(0), static_cast<int32_t>(size - 1));
                    range.sync();

                    for (auto &weak_user : inst->get_users_ref()) {
                        auto user = std::dynamic_pointer_cast<ir::Instruction>(weak_user.lock());
                        if (user->get_ins_type() == InstructionType::LOAD ||
                            user->get_ins_type() == InstructionType::STORE)
                            update_block_ctx(index, user->get_parent_block().lock(), range);
                    }

                    if (type->is_pointer_ty())
                        type = std::dynamic_pointer_cast<ir::PointerType>(type)->get_reference_type();
                    else if (type->is_array_ty())
                        type = std::dynamic_pointer_cast<ir::ArrayType>(type)->get_element_type();
                    else {
                        logger::ERROR("Unsupported GEP type: ", type->to_string());
                        __builtin_unreachable();
                    }
                }
            }
            break;
        }
        case InstructionType::CALL: {
            auto call = std::dynamic_pointer_cast<ir::Call>(inst);

            auto get_ptr_bound = [&](std::shared_ptr<ir::Value> ptr) {
                const auto base = pointer_base_info->query(ptr);
                if (!base) return IntegerRange::get_non_negative();
                const auto &base_val = base.value();
                if (!std::dynamic_pointer_cast<ir::Alloca>(base_val) &&
                    !std::dynamic_pointer_cast<ir::GlobalVariable>(base_val))
                    return IntegerRange::get_non_negative();
                auto array_size =
                    std::dynamic_pointer_cast<ir::PointerType>(base_val->get_type())->get_reference_type()->bits_num() /
                    8;
                IntegerRange range;
                range.set_range(1, static_cast<intmax_t>(array_size) / static_cast<intmax_t>(sizeof(int32_t)));
                range.sync();
                return range;
            };

            if (call->get_type() == ir::IntegerType::get(32)) {
                auto func = call->get_function();
                if (func == ir::Function::getch) {
                    IntegerRange range;
                    range.set_range(0, 127);
                    range.sync();
                    update(inst, range);
                } else if (func == ir::Function::getarray || func == ir::Function::getfarray) {
                    update(inst, get_ptr_bound(inst->get_operands_ref()[1]));
                } else if (func == ir::Function::putarray || func == ir::Function::putfarray) {
                    update_block_ctx(inst->get_operands_ref()[1],
                                     inst->get_parent_block().lock(),
                                     get_ptr_bound(inst->get_operands_ref()[2]));
                }
            }
            break;
        }
        case InstructionType::ICMP: {
            const auto set_cmp_ctx = [&](std::shared_ptr<ir::Value> lhs,
                                         std::shared_ptr<ir::Value> rhs,
                                         ir::ICmp::ICmpType cmp_type,
                                         auto update_func) {
                using ICmpType = ir::ICmp::ICmpType;
                const auto lhs_range = get_range(lhs, inst);
                const auto rhs_range = get_range(rhs, inst);

                switch (cmp_type) {
                    case ICmpType::EQ: {
                        update_func(lhs, rhs_range);
                        update_func(rhs, lhs_range);
                        break;
                    }
                    case ICmpType::NE: {
                        break;
                    }
                    case ICmpType::SLT: {
                        IntegerRange lhs_new_range;
                        lhs_new_range.set_range(std::numeric_limits<int32_t>::min(), rhs_range.max() - 1);
                        lhs_new_range.sync();
                        update_func(lhs, lhs_new_range);

                        IntegerRange rhs_new_range;
                        rhs_new_range.set_range(lhs_range.min() + 1, std::numeric_limits<int32_t>::max());
                        rhs_new_range.sync();
                        update_func(rhs, rhs_new_range);
                        break;
                    }
                    case ICmpType::SLE: {
                        IntegerRange lhs_new_range;
                        lhs_new_range.set_range(std::numeric_limits<int32_t>::min(), rhs_range.max());
                        lhs_new_range.sync();
                        update_func(lhs, lhs_new_range);

                        IntegerRange rhs_new_range;
                        rhs_new_range.set_range(lhs_range.min(), std::numeric_limits<int32_t>::max());
                        rhs_new_range.sync();
                        update_func(rhs, rhs_new_range);
                        break;
                    }
                    case ICmpType::SGT: {
                        IntegerRange lhs_new_range;
                        lhs_new_range.set_range(rhs_range.min() + 1, std::numeric_limits<int32_t>::max());
                        lhs_new_range.sync();
                        update_func(lhs, lhs_new_range);

                        IntegerRange rhs_new_range;
                        rhs_new_range.set_range(std::numeric_limits<int32_t>::min(), lhs_range.max() - 1);
                        rhs_new_range.sync();
                        update_func(rhs, rhs_new_range);
                        break;
                    }
                    case ICmpType::SGE: {
                        IntegerRange lhs_new_range;
                        lhs_new_range.set_range(rhs_range.min(), std::numeric_limits<int32_t>::max());
                        lhs_new_range.sync();
                        update_func(lhs, lhs_new_range);

                        IntegerRange rhs_new_range;
                        rhs_new_range.set_range(std::numeric_limits<int32_t>::min(), lhs_range.max());
                        rhs_new_range.sync();
                        update_func(rhs, rhs_new_range);
                        break;
                    }
                    default: {
                        logger::ERROR("[IntegerRangeAnalyzation::iter_by_inst] Unsupported ICmp type: ", cmp_type);
                        __builtin_unreachable();
                    }
                }
            };
            const auto icmp = std::dynamic_pointer_cast<ir::ICmp>(inst);
            const auto op = icmp->get_cmp_type();
            for (const auto &weak_user : icmp->get_users_ref()) {
                const auto user = std::dynamic_pointer_cast<ir::Instruction>(weak_user.lock());
                if (auto br = std::dynamic_pointer_cast<ir::Br>(user)) {
                    const auto true_target = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[1]);
                    const auto false_target = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[2]);
                    if (true_target == false_target) continue;

                    const auto block = br->get_parent_block().lock();
                    if (block != true_target && block->opt_info.dominates(true_target) &&
                        true_target->opt_info.predecessors.size() == 1) {
                        set_cmp_ctx(icmp->get_operands_ref()[0],
                                    icmp->get_operands_ref()[1],
                                    op,
                                    [&](std::shared_ptr<ir::Value> val, const IntegerRange &new_range) {
                                        update_block_ctx(val, true_target, new_range);
                                    });
                    }
                    if (block != false_target && block->opt_info.dominates(false_target) &&
                        false_target->opt_info.predecessors.size() == 1) {
                        set_cmp_ctx(icmp->get_operands_ref()[0],
                                    icmp->get_operands_ref()[1],
                                    ir::ICmp::get_inverted_op(op),
                                    [&](std::shared_ptr<ir::Value> val, const IntegerRange &new_range) {
                                        update_block_ctx(val, false_target, new_range);
                                    });
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

}  // namespace opt::pass
