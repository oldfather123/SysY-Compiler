#include <unordered_set>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {

// Global state for current block processing
static std::vector<std::shared_ptr<ir::Instruction>> reduced_list;
static std::unordered_set<std::shared_ptr<ir::Instruction>> del_list;
static std::vector<std::shared_ptr<ir::Instruction>> snap;
static size_t idx;

// Check if instruction type is associative
static bool is_associative(const std::shared_ptr<ir::Instruction> &inst) {
    using IT = ir::Instruction::InstructionType;
    const auto ty = inst->get_ins_type();
    return ty == IT::ADD || ty == IT::MUL || ty == IT::FADD || ty == IT::FMUL || ty == IT::AND || ty == IT::OR ||
           ty == IT::XOR;
}

// Check if instruction is binary operation
static bool is_binary_operation(const std::shared_ptr<ir::Instruction> &inst) { return inst->is_binary(); }

// Swap operands for associative instructions
static void swap_operands(const std::shared_ptr<ir::Instruction> &inst) {
    auto &operands = inst->get_operands_ref();
    if (operands.size() >= 2) {
        std::swap(operands[0], operands[1]);
    }
}

static void run_on_binary_inst(const std::shared_ptr<ir::Instruction> &inst);
static void reduce_add(const std::shared_ptr<ir::Add> &inst);
static void reduce_sub(const std::shared_ptr<ir::Sub> &inst);
static void reduce_mul(const std::shared_ptr<ir::Mul> &inst);
static void reduce_div(const std::shared_ptr<ir::SDiv> &inst);
static void reduce_rem(const std::shared_ptr<ir::SRem> &inst);
static void reduce_fadd(const std::shared_ptr<ir::FAdd> &inst);
static void reduce_fsub(const std::shared_ptr<ir::FSub> &inst);
static void reduce_fmul(const std::shared_ptr<ir::FMul> &inst);
static void reduce_fdiv(const std::shared_ptr<ir::FDiv> &inst);

static void run_on_binary_inst(const std::shared_ptr<ir::Instruction> &inst) {
    // Default: assume instructions have participated in constant folding,
    // binary operands can only have one constant
    // Put constant in operand 1
    if (is_associative(inst)) {
        auto &operands = inst->get_operands_ref();
        if (operands.size() >= 2 && std::dynamic_pointer_cast<ir::ConstantInt>(operands[1])) {
            swap_operands(inst);
        }
    }

    using IT = ir::Instruction::InstructionType;
    switch (inst->get_ins_type()) {
        case IT::ADD:
            reduce_add(std::dynamic_pointer_cast<ir::Add>(inst));
            break;
        case IT::SUB:
            reduce_sub(std::dynamic_pointer_cast<ir::Sub>(inst));
            break;
        case IT::MUL:
            reduce_mul(std::dynamic_pointer_cast<ir::Mul>(inst));
            break;
        case IT::SDIV:
            reduce_div(std::dynamic_pointer_cast<ir::SDiv>(inst));
            break;
        case IT::SREM:
            reduce_rem(std::dynamic_pointer_cast<ir::SRem>(inst));
            break;
        case IT::FADD:
            reduce_fadd(std::dynamic_pointer_cast<ir::FAdd>(inst));
            break;
        case IT::FSUB:
            reduce_fsub(std::dynamic_pointer_cast<ir::FSub>(inst));
            break;
        case IT::FMUL:
            reduce_fmul(std::dynamic_pointer_cast<ir::FMul>(inst));
            break;
        case IT::FDIV:
            reduce_fdiv(std::dynamic_pointer_cast<ir::FDiv>(inst));
            break;
        default:
            reduced_list.push_back(inst);
            break;
    }
}

bool ArithReduce::run(ir::Module *module) {
    bool modified = false;

    module->for_each_func([&](const auto &func) {
        func->for_each_block([&](const auto &block) {
            // Process block
            reduced_list.clear();
            del_list.clear();

            // Create snapshot of instructions
            snap = std::vector<std::shared_ptr<ir::Instruction>>(block->get_instructions_ref().begin(),
                                                                 block->get_instructions_ref().end());
            idx = 0;

            while (idx < snap.size()) {
                auto instruction = snap[idx];
                if (instruction && is_binary_operation(instruction)) {
                    run_on_binary_inst(instruction);
                } else if (instruction) {
                    reduced_list.push_back(instruction);
                }
                idx++;
            }

            // Clear the block and rebuild with reduced instructions
            size_t old_size = block->get_instructions_ref().size();
            block->get_instructions_ref().clear();
            for (const auto &inst : reduced_list) {
                if (del_list.find(inst) == del_list.end()) {
                    block->add_instruction(block->get_instructions_ref().end(), inst);
                }
            }

            if (block->get_instructions_ref().size() != old_size) {
                modified = true;
            }
        });
    });

    return modified;
}

static void reduce_add(const std::shared_ptr<ir::Add> &inst) {
    auto operand_1 = inst->get_operands_ref()[0];
    auto operand_2 = inst->get_operands_ref()[1];
    auto block = inst->get_parent_block().lock();

    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(operand_1)) {
        // 0 + v -> v
        if (c->get_val() == 0) {
            util::substitute(inst, operand_2);
            del_list.insert(inst);
            return;
        }

        // c2 + (c1 + x) -> (c1 + c2) + x
        if (auto add = std::dynamic_pointer_cast<ir::Add>(operand_2)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(add->get_operands_ref()[0])) {
                auto sum_const =
                    std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c->get_val() + c1->get_val());
                auto new_add = ir::Add::create(block, sum_const, add->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_add);
                del_list.insert(inst);
                snap.insert(snap.begin() + idx + 1, new_add);
                return;
            }
        }

        // c2 + (c1 - x) -> (c2 + c1) - x
        if (auto sub = std::dynamic_pointer_cast<ir::Sub>(operand_2)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[0])) {
                auto sum_const =
                    std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c->get_val() + c1->get_val());
                auto new_sub = ir::Sub::create(block, sum_const, sub->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_sub);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_sub);
                return;
            }

            // c2 + (x - c1) -> (c2 - c1) + x
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[1])) {
                auto diff_const =
                    std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c->get_val() - c1->get_val());
                auto new_add = ir::Add::create(block, diff_const, sub->get_operands_ref()[0], gen_local_var_name());
                util::substitute(inst, new_add);
                del_list.insert(inst);
                snap.insert(snap.begin() + idx + 1, new_add);
                return;
            }
        }
    }

    // a + (0 - b) -> a - b
    if (auto sub = std::dynamic_pointer_cast<ir::Sub>(operand_2)) {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[0]); c && c->get_val() == 0) {
            auto new_sub = ir::Sub::create(block, operand_1, sub->get_operands_ref()[1], gen_local_var_name());
            util::substitute(inst, new_sub);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_sub);
            return;
        }
    }

    // a * b + a -> (1 + b) * a
    // b * a + a -> (1 + b) * a
    if (auto mul1 = std::dynamic_pointer_cast<ir::Mul>(operand_1)) {
        if (mul1->get_users_ref().size() == 1) {
            if (mul1->get_operands_ref()[0] == operand_2) {
                auto one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 1);
                auto add = ir::Add::create(block, one, mul1->get_operands_ref()[1], gen_local_var_name());
                auto new_mul = ir::Mul::create(block, add, operand_2, gen_local_var_name());
                util::substitute(inst, new_mul);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), add);
                snap.insert(snap.begin() + static_cast<long>(idx + 2), new_mul);
                return;
            } else if (mul1->get_operands_ref()[1] == operand_2) {
                auto one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 1);
                auto add = ir::Add::create(block, one, mul1->get_operands_ref()[0], gen_local_var_name());
                auto new_mul = ir::Mul::create(block, add, operand_2, gen_local_var_name());
                util::substitute(inst, new_mul);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), add);
                snap.insert(snap.begin() + static_cast<long>(idx + 2), new_mul);
                return;
            }
        }
    }

    // a + a * b -> (1 + b) * a
    // a + b * a -> (1 + b) * a
    if (auto mul2 = std::dynamic_pointer_cast<ir::Mul>(operand_2)) {
        if (mul2->get_users_ref().size() == 1) {
            if (mul2->get_operands_ref()[0] == operand_1) {
                auto one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 1);
                auto add = ir::Add::create(block, one, mul2->get_operands_ref()[1], gen_local_var_name());
                auto new_mul = ir::Mul::create(block, add, operand_1, gen_local_var_name());
                util::substitute(inst, new_mul);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), add);
                snap.insert(snap.begin() + static_cast<long>(idx + 2), new_mul);
                return;
            } else if (mul2->get_operands_ref()[1] == operand_1) {
                auto one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 1);
                auto add = ir::Add::create(block, one, mul2->get_operands_ref()[0], gen_local_var_name());
                auto new_mul = ir::Mul::create(block, add, operand_1, gen_local_var_name());
                util::substitute(inst, new_mul);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), add);
                snap.insert(snap.begin() + static_cast<long>(idx + 2), new_mul);
                return;
            }
        }
    }

    // b * a + c * a -> (b + c) * a
    // a * b + c * a -> (b + c) * a
    // a * b + a * c -> (b + c) * a
    // b * a + a * c -> (b + c) * a
    if (auto mul1 = std::dynamic_pointer_cast<ir::Mul>(operand_1)) {
        if (auto mul2 = std::dynamic_pointer_cast<ir::Mul>(operand_2)) {
            if (mul1->get_users_ref().size() == 1 || mul2->get_users_ref().size() == 1) {
                std::shared_ptr<ir::Value> a = nullptr, b = nullptr, c = nullptr;

                if (mul1->get_operands_ref()[0] == mul2->get_operands_ref()[0]) {
                    a = mul1->get_operands_ref()[0];
                    b = mul1->get_operands_ref()[1];
                    c = mul2->get_operands_ref()[1];
                } else if (mul1->get_operands_ref()[0] == mul2->get_operands_ref()[1]) {
                    a = mul1->get_operands_ref()[0];
                    b = mul1->get_operands_ref()[1];
                    c = mul2->get_operands_ref()[0];
                } else if (mul1->get_operands_ref()[1] == mul2->get_operands_ref()[0]) {
                    a = mul1->get_operands_ref()[1];
                    b = mul1->get_operands_ref()[0];
                    c = mul2->get_operands_ref()[1];
                } else if (mul1->get_operands_ref()[1] == mul2->get_operands_ref()[1]) {
                    a = mul1->get_operands_ref()[1];
                    b = mul1->get_operands_ref()[0];
                    c = mul2->get_operands_ref()[0];
                }

                if (a != nullptr) {
                    auto add = ir::Add::create(block, b, c, gen_local_var_name());
                    auto new_mul = ir::Mul::create(block, a, add, gen_local_var_name());
                    util::substitute(inst, new_mul);
                    del_list.insert(inst);
                    snap.insert(snap.begin() + static_cast<long>(idx + 1), add);
                    snap.insert(snap.begin() + static_cast<long>(idx + 2), new_mul);
                    return;
                }
            }
        }
    }

    reduced_list.push_back(inst);
}

static void reduce_sub(const std::shared_ptr<ir::Sub> &inst) {
    auto operand_1 = inst->get_operands_ref()[0];
    auto operand_2 = inst->get_operands_ref()[1];
    auto block = inst->get_parent_block().lock();

    // a - 0 -> a
    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(operand_2); c && c->get_val() == 0) {
        util::substitute(inst, operand_1);
        del_list.insert(inst);
        return;
    }

    // a - a -> 0
    if (operand_1 == operand_2) {
        auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
        util::substitute(inst, zero);
        del_list.insert(inst);
        return;
    }

    // 0 - (0 - a) -> a
    if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(operand_1)) {
        if (c1->get_val() == 0) {
            if (auto sub = std::dynamic_pointer_cast<ir::Sub>(operand_2)) {
                // 0 - (0 - a) -> a
                if (auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[0]);
                    c2 && c2->get_val() == 0) {
                    util::substitute(inst, sub->get_operands_ref()[1]);
                    del_list.insert(inst);
                    return;
                }

                // 0 - (a - b) -> (b - a)
                if (sub->get_users_ref().size() == 1) {
                    auto new_sub = ir::Sub::create(
                        block, sub->get_operands_ref()[1], sub->get_operands_ref()[0], gen_local_var_name());
                    util::substitute(inst, new_sub);
                    del_list.insert(inst);
                    snap.insert(snap.begin() + static_cast<long>(idx + 1), new_sub);
                    return;
                }
            }
        }

        // c1 - (c2 + x) -> (c1 - c2) - x
        if (auto add = std::dynamic_pointer_cast<ir::Add>(operand_2)) {
            if (auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>(add->get_operands_ref()[0])) {
                auto diff_const =
                    std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c1->get_val() - c2->get_val());
                auto new_sub = ir::Sub::create(block, diff_const, add->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_sub);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_sub);
                return;
            }
        }
    }

    if (auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>(operand_2)) {
        if (auto sub = std::dynamic_pointer_cast<ir::Sub>(operand_1)) {
            // (x - c1) - c2 -> x + -(c1 + c2)
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[1])) {
                auto neg_sum =
                    std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), -c1->get_val() - c2->get_val());
                auto new_add = ir::Add::create(block, sub->get_operands_ref()[0], neg_sum, gen_local_var_name());
                util::substitute(inst, new_add);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_add);
                return;
            }

            // (c1 - x) - c2 -> (c1 - c2) - x
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[0])) {
                auto diff_const =
                    std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c1->get_val() - c2->get_val());
                auto new_sub = ir::Sub::create(block, diff_const, sub->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_sub);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_sub);
                return;
            }
        }

        // (c1 + x) - c2 -> (c1 - c2) + x
        if (auto add = std::dynamic_pointer_cast<ir::Add>(operand_1)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(add->get_operands_ref()[0])) {
                auto diff_const =
                    std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c1->get_val() - c2->get_val());
                auto new_add = ir::Add::create(block, diff_const, add->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_add);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_add);
                return;
            }
        }

        // a - c -> (-c) + a
        auto neg_c = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), -c2->get_val());
        auto new_add = ir::Add::create(block, neg_c, operand_1, gen_local_var_name());
        util::substitute(inst, new_add);
        del_list.insert(inst);
        snap.insert(snap.begin() + static_cast<long>(idx + 1), new_add);
        return;
    }

    // a - (0 - b) -> a + b
    if (auto sub = std::dynamic_pointer_cast<ir::Sub>(operand_2)) {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[0]); c && c->get_val() == 0) {
            auto new_add = ir::Add::create(block, operand_1, sub->get_operands_ref()[1], gen_local_var_name());
            util::substitute(inst, new_add);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_add);
            return;
        }
    }

    // a - (a + b) -> 0 - b
    // a - (b + a) -> 0 - b
    if (auto add = std::dynamic_pointer_cast<ir::Add>(operand_2)) {
        if (add->get_operands_ref()[0] == operand_1) {
            auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            auto new_sub = ir::Sub::create(block, zero, add->get_operands_ref()[1], gen_local_var_name());
            util::substitute(inst, new_sub);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_sub);
            return;
        }
        if (add->get_operands_ref()[1] == operand_1) {
            auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            auto new_sub = ir::Sub::create(block, zero, add->get_operands_ref()[0], gen_local_var_name());
            util::substitute(inst, new_sub);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_sub);
            return;
        }
    }

    // Similar to add: factoring for mul
    if (auto mul1 = std::dynamic_pointer_cast<ir::Mul>(operand_1)) {
        if (auto mul2 = std::dynamic_pointer_cast<ir::Mul>(operand_2)) {
            if (mul1->get_users_ref().size() == 1 || mul2->get_users_ref().size() == 1) {
                std::shared_ptr<ir::Value> a = nullptr, b = nullptr, c = nullptr;

                if (mul1->get_operands_ref()[0] == mul2->get_operands_ref()[0]) {
                    a = mul1->get_operands_ref()[0];
                    b = mul1->get_operands_ref()[1];
                    c = mul2->get_operands_ref()[1];
                } else if (mul1->get_operands_ref()[0] == mul2->get_operands_ref()[1]) {
                    a = mul1->get_operands_ref()[0];
                    b = mul1->get_operands_ref()[1];
                    c = mul2->get_operands_ref()[0];
                } else if (mul1->get_operands_ref()[1] == mul2->get_operands_ref()[0]) {
                    a = mul1->get_operands_ref()[1];
                    b = mul1->get_operands_ref()[0];
                    c = mul2->get_operands_ref()[1];
                } else if (mul1->get_operands_ref()[1] == mul2->get_operands_ref()[1]) {
                    a = mul1->get_operands_ref()[1];
                    b = mul1->get_operands_ref()[0];
                    c = mul2->get_operands_ref()[0];
                }

                if (a != nullptr) {
                    auto sub = ir::Sub::create(block, b, c, gen_local_var_name());
                    auto new_mul = ir::Mul::create(block, a, sub, gen_local_var_name());
                    util::substitute(inst, new_mul);
                    del_list.insert(inst);
                    snap.insert(snap.begin() + static_cast<long>(idx + 1), sub);
                    snap.insert(snap.begin() + static_cast<long>(idx + 2), new_mul);
                    return;
                }
            }
        }
    }

    reduced_list.push_back(inst);
}

static void reduce_mul(const std::shared_ptr<ir::Mul> &inst) {
    auto operand_1 = inst->get_operands_ref()[0];
    auto operand_2 = inst->get_operands_ref()[1];
    auto block = inst->get_parent_block().lock();

    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(operand_1)) {
        // 0 * v -> 0
        if (c->get_val() == 0) {
            auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            util::substitute(inst, zero);
            del_list.insert(inst);
            return;
        }

        // 1 * v -> v
        if (c->get_val() == 1) {
            util::substitute(inst, operand_2);
            del_list.insert(inst);
            return;
        }

        // -1 * v -> 0 - v
        if (c->get_val() == -1) {
            auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            auto new_sub = ir::Sub::create(block, zero, operand_2, gen_local_var_name());
            util::substitute(inst, new_sub);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_sub);
            return;
        }

        // c * (0 - x) -> -c * x
        if (auto sub = std::dynamic_pointer_cast<ir::Sub>(operand_2)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[0]);
                c1 && c1->get_val() == 0) {
                auto neg_c = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), -c->get_val());
                auto new_mul = ir::Mul::create(block, neg_c, sub->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_mul);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_mul);
                return;
            }
        }
    }

    reduced_list.push_back(inst);
}

static void reduce_div(const std::shared_ptr<ir::SDiv> &inst) {
    auto operand_1 = inst->get_operands_ref()[0];
    auto operand_2 = inst->get_operands_ref()[1];
    auto block = inst->get_parent_block().lock();

    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(operand_1)) {
        // 0 / v -> 0
        if (c->get_val() == 0) {
            auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            util::substitute(inst, zero);
            del_list.insert(inst);
            return;
        }
    }

    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(operand_2)) {
        // v / 1 -> v
        if (c->get_val() == 1) {
            util::substitute(inst, operand_1);
            del_list.insert(inst);
            return;
        }

        // v / -1 -> 0 - v
        if (c->get_val() == -1) {
            auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            auto new_sub = ir::Sub::create(block, zero, operand_1, gen_local_var_name());
            util::substitute(inst, new_sub);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_sub);
            return;
        }

        // (c1 * x) / c2 -> (c1 / c2) * x if c1 % c2 == 0, 确保 c1 * x 只有一个作用点
        if (auto mul = std::dynamic_pointer_cast<ir::Mul>(operand_1)) {
            if (mul->get_users_ref().size() == 1) {
                if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(mul->get_operands_ref()[0])) {
                    if (c1->get_val() % c->get_val() == 0) {
                        auto new_const =
                            std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c1->get_val() / c->get_val());
                        auto new_mul =
                            ir::Mul::create(block, new_const, mul->get_operands_ref()[1], gen_local_var_name());
                        util::substitute(inst, new_mul);
                        del_list.insert(inst);
                        snap.insert(snap.begin() + static_cast<long>(idx + 1), new_mul);
                        return;
                    }
                }
            }
        }

        // (0 - x) / c -> x / -c
        if (auto sub = std::dynamic_pointer_cast<ir::Sub>(operand_1)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[0]);
                c1 && c1->get_val() == 0) {
                auto neg_c = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), -c->get_val());
                auto new_div = ir::SDiv::create(block, sub->get_operands_ref()[1], neg_c, gen_local_var_name());
                util::substitute(inst, new_div);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_div);
                return;
            }
        }
    }

    // v / v -> 1
    if (operand_1 == operand_2) {
        auto one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 1);
        util::substitute(inst, one);
        del_list.insert(inst);
        return;
    }

    // v / (0 - v) -> -1
    if (auto sub = std::dynamic_pointer_cast<ir::Sub>(operand_2)) {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[0]);
            c && c->get_val() == 0 && sub->get_operands_ref()[1] == operand_1) {
            auto neg_one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), -1);
            util::substitute(inst, neg_one);
            del_list.insert(inst);
            return;
        }
    }

    if (auto sub = std::dynamic_pointer_cast<ir::Sub>(operand_1)) {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(sub->get_operands_ref()[0]); c && c->get_val() == 0) {
            // (0 - v) / v -> -1
            if (sub->get_operands_ref()[1] == operand_2) {
                auto neg_one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), -1);
                util::substitute(inst, neg_one);
                del_list.insert(inst);
                return;
            }

            // (0 - v) / c -> v / -c
            if (auto constant = std::dynamic_pointer_cast<ir::ConstantInt>(operand_2)) {
                auto neg_c = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), -constant->get_val());
                auto new_div = ir::SDiv::create(block, sub->get_operands_ref()[1], neg_c, gen_local_var_name());
                util::substitute(inst, new_div);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_div);
                return;
            }
        }
    }

    // v / (v * a) or (a * v) -> 1 / a 确保 v * a 只有一个作用点
    if (auto mul = std::dynamic_pointer_cast<ir::Mul>(operand_2)) {
        if (mul->get_users_ref().size() == 1) {
            if (mul->get_operands_ref()[0] == operand_1) {
                auto one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 1);
                auto new_div = ir::SDiv::create(block, one, mul->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_div);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_div);
                return;
            } else if (mul->get_operands_ref()[1] == operand_1) {
                auto one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 1);
                auto new_div = ir::SDiv::create(block, one, mul->get_operands_ref()[0], gen_local_var_name());
                util::substitute(inst, new_div);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_div);
                return;
            }
        }
    }

    // v / a / b -> v / (a * b) 确保 a / b 只有一个作用点
    if (auto div = std::dynamic_pointer_cast<ir::SDiv>(operand_1)) {
        if (div->get_users_ref().size() == 1) {
            auto mul = ir::Mul::create(block, div->get_operands_ref()[1], operand_2, gen_local_var_name());
            auto new_div = ir::SDiv::create(block, div->get_operands_ref()[0], mul, gen_local_var_name());
            util::substitute(inst, new_div);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), mul);
            snap.insert(snap.begin() + static_cast<long>(idx + 2), new_div);
            return;
        }
    }

    reduced_list.push_back(inst);
}

static void reduce_rem(const std::shared_ptr<ir::SRem> &inst) {
    auto operand_1 = inst->get_operands_ref()[0];
    auto operand_2 = inst->get_operands_ref()[1];

    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(operand_1)) {
        // 0 % v -> 0
        if (c->get_val() == 0) {
            auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            util::substitute(inst, zero);
            del_list.insert(inst);
            return;
        }
    }

    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(operand_2)) {
        // v % 1 -> 0
        if (c->get_val() == 1) {
            auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            util::substitute(inst, zero);
            del_list.insert(inst);
            return;
        }

        // v % -1 -> 0
        if (c->get_val() == -1) {
            auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            util::substitute(inst, zero);
            del_list.insert(inst);
            return;
        }
    }

    // v % v -> 0
    if (operand_1 == operand_2) {
        auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
        util::substitute(inst, zero);
        del_list.insert(inst);
        return;
    }

    // (c1 * x) % c2 -> 0 if c1 % c2 == 0
    if (auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>(operand_2)) {
        if (auto mul = std::dynamic_pointer_cast<ir::Mul>(operand_1)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(mul->get_operands_ref()[0])) {
                if (c1->get_val() % c2->get_val() == 0) {
                    auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
                    util::substitute(inst, zero);
                    del_list.insert(inst);
                    return;
                }
            }
        }
    }

    reduced_list.push_back(inst);
}

// Float operations - more comprehensive but still conservative
static void reduce_fadd(const std::shared_ptr<ir::FAdd> &inst) {
    auto operand_1 = inst->get_operands_ref()[0];
    auto operand_2 = inst->get_operands_ref()[1];
    auto block = inst->get_parent_block().lock();

    if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(operand_1)) {
        // 0.0 + v -> v
        if (c->get_val() == 0.0f) {
            util::substitute(inst, operand_2);
            del_list.insert(inst);
            return;
        }

        // c2 + (c1 + x) -> (c1 + c2) + x
        if (auto fadd = std::dynamic_pointer_cast<ir::FAdd>(operand_2)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantFloat>(fadd->get_operands_ref()[0])) {
                auto sum_const =
                    std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), c->get_val() + c1->get_val());
                auto new_fadd = ir::FAdd::create(block, sum_const, fadd->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_fadd);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fadd);
                return;
            }
        }

        // c2 + (c1 - x) -> (c2 + c1) - x
        if (auto fsub = std::dynamic_pointer_cast<ir::FSub>(operand_2)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[0])) {
                auto sum_const =
                    std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), c->get_val() + c1->get_val());
                auto new_fsub = ir::FSub::create(block, sum_const, fsub->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_fsub);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fsub);
                return;
            }

            // c2 + (x - c1) -> (c2 - c1) + x
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[1])) {
                auto diff_const =
                    std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), c->get_val() - c1->get_val());
                auto new_fadd = ir::FAdd::create(block, diff_const, fsub->get_operands_ref()[0], gen_local_var_name());
                util::substitute(inst, new_fadd);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fadd);
                return;
            }
        }
    }

    // a + (0.0 - b) -> a - b
    if (auto fsub = std::dynamic_pointer_cast<ir::FSub>(operand_2)) {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[0]);
            c && c->get_val() == 0.0f) {
            auto new_fsub = ir::FSub::create(block, operand_1, fsub->get_operands_ref()[1], gen_local_var_name());
            util::substitute(inst, new_fsub);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fsub);
            return;
        }
    }

    reduced_list.push_back(inst);
}

static void reduce_fsub(const std::shared_ptr<ir::FSub> &inst) {
    auto operand_1 = inst->get_operands_ref()[0];
    auto operand_2 = inst->get_operands_ref()[1];
    auto block = inst->get_parent_block().lock();

    if (auto c1 = std::dynamic_pointer_cast<ir::ConstantFloat>(operand_1)) {
        // 0.0 - (0.0 - a) -> a
        if (c1->get_val() == 0.0f) {
            if (auto fsub = std::dynamic_pointer_cast<ir::FSub>(operand_2)) {
                if (auto c2 = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[0]);
                    c2 && c2->get_val() == 0.0f) {
                    util::substitute(inst, fsub->get_operands_ref()[1]);
                    del_list.insert(inst);
                    return;
                }
            }
        }

        // c1 - (c2 + x) -> (c1 - c2) - x
        if (auto fadd = std::dynamic_pointer_cast<ir::FAdd>(operand_2)) {
            if (auto c2 = std::dynamic_pointer_cast<ir::ConstantFloat>(fadd->get_operands_ref()[0])) {
                auto diff_const =
                    std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), c1->get_val() - c2->get_val());
                auto new_fsub = ir::FSub::create(block, diff_const, fadd->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_fsub);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fsub);
                return;
            }
        }
    }

    if (auto c2 = std::dynamic_pointer_cast<ir::ConstantFloat>(operand_2)) {
        // a - 0.0 -> a
        if (c2->get_val() == 0.0f) {
            util::substitute(inst, operand_1);
            del_list.insert(inst);
            return;
        }

        if (auto fsub = std::dynamic_pointer_cast<ir::FSub>(operand_1)) {
            // (x - c1) - c2 -> x + -(c1 + c2)
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[1])) {
                auto neg_sum =
                    std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), -c1->get_val() - c2->get_val());
                auto new_fadd = ir::FAdd::create(block, fsub->get_operands_ref()[0], neg_sum, gen_local_var_name());
                util::substitute(inst, new_fadd);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fadd);
                return;
            }

            // (c1 - x) - c2 -> (c1 - c2) - x
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[0])) {
                auto diff_const =
                    std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), c1->get_val() - c2->get_val());
                auto new_fsub = ir::FSub::create(block, diff_const, fsub->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_fsub);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fsub);
                return;
            }
        }

        // (c1 + x) - c2 -> (c1 - c2) + x
        if (auto fadd = std::dynamic_pointer_cast<ir::FAdd>(operand_1)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantFloat>(fadd->get_operands_ref()[0])) {
                auto diff_const =
                    std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), c1->get_val() - c2->get_val());
                auto new_fadd = ir::FAdd::create(block, diff_const, fadd->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_fadd);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fadd);
                return;
            }
        }

        // a - c -> (-c) + a
        auto neg_c = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), -c2->get_val());
        auto new_fadd = ir::FAdd::create(block, neg_c, operand_1, gen_local_var_name());
        util::substitute(inst, new_fadd);
        del_list.insert(inst);
        snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fadd);
        return;
    }

    // a - a -> 0.0
    if (operand_1 == operand_2) {
        auto zero = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0f);
        util::substitute(inst, zero);
        del_list.insert(inst);
        return;
    }

    // a - (0.0 - b) -> a + b
    if (auto fsub = std::dynamic_pointer_cast<ir::FSub>(operand_2)) {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[0]);
            c && c->get_val() == 0.0f) {
            auto new_fadd = ir::FAdd::create(block, operand_1, fsub->get_operands_ref()[1], gen_local_var_name());
            util::substitute(inst, new_fadd);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fadd);
            return;
        }
    }

    reduced_list.push_back(inst);
}

static void reduce_fmul(const std::shared_ptr<ir::FMul> &inst) {
    auto operand_1 = inst->get_operands_ref()[0];
    auto operand_2 = inst->get_operands_ref()[1];
    auto block = inst->get_parent_block().lock();

    if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(operand_1)) {
        // 0.0 * v -> 0.0
        if (c->get_val() == 0.0f) {
            auto zero = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0f);
            util::substitute(inst, zero);
            del_list.insert(inst);
            return;
        }

        // 1.0 * v -> v
        if (c->get_val() == 1.0f) {
            util::substitute(inst, operand_2);
            del_list.insert(inst);
            return;
        }

        // -1.0 * v -> 0.0 - v
        if (c->get_val() == -1.0f) {
            auto zero = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0f);
            auto new_fsub = ir::FSub::create(block, zero, operand_2, gen_local_var_name());
            util::substitute(inst, new_fsub);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fsub);
            return;
        }

        // c * (0.0 - x) -> -c * x
        if (auto fsub = std::dynamic_pointer_cast<ir::FSub>(operand_2)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[0]);
                c1 && c1->get_val() == 0.0f) {
                auto neg_c = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), -c->get_val());
                auto new_fmul = ir::FMul::create(block, neg_c, fsub->get_operands_ref()[1], gen_local_var_name());
                util::substitute(inst, new_fmul);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fmul);
                return;
            }
        }
    }

    reduced_list.push_back(inst);
}

static void reduce_fdiv(const std::shared_ptr<ir::FDiv> &inst) {
    auto operand_1 = inst->get_operands_ref()[0];
    auto operand_2 = inst->get_operands_ref()[1];
    auto block = inst->get_parent_block().lock();

    if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(operand_1)) {
        // 0.0 / v -> 0.0
        if (c->get_val() == 0.0f) {
            auto zero = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0f);
            util::substitute(inst, zero);
            del_list.insert(inst);
            return;
        }
    }

    if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(operand_2)) {
        // v / 1.0 -> v
        if (c->get_val() == 1.0f) {
            util::substitute(inst, operand_1);
            del_list.insert(inst);
            return;
        }

        // v / -1.0 -> 0.0 - v
        if (c->get_val() == -1.0f) {
            auto zero = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0f);
            auto new_fsub = ir::FSub::create(block, zero, operand_1, gen_local_var_name());
            util::substitute(inst, new_fsub);
            del_list.insert(inst);
            snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fsub);
            return;
        }

        // (0.0 - x) / c -> x / -c
        if (auto fsub = std::dynamic_pointer_cast<ir::FSub>(operand_1)) {
            if (auto c1 = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[0]);
                c1 && c1->get_val() == 0.0f) {
                auto neg_c = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), -c->get_val());
                auto new_fdiv = ir::FDiv::create(block, fsub->get_operands_ref()[1], neg_c, gen_local_var_name());
                util::substitute(inst, new_fdiv);
                del_list.insert(inst);
                snap.insert(snap.begin() + static_cast<long>(idx + 1), new_fdiv);
                return;
            }
        }
    }

    // v / v -> 1.0
    if (operand_1 == operand_2) {
        auto one = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 1.0f);
        util::substitute(inst, one);
        del_list.insert(inst);
        return;
    }

    // v / (0.0 - v) -> -1.0
    if (auto fsub = std::dynamic_pointer_cast<ir::FSub>(operand_2)) {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[0]);
            c && c->get_val() == 0.0f && fsub->get_operands_ref()[1] == operand_1) {
            auto neg_one = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), -1.0f);
            util::substitute(inst, neg_one);
            del_list.insert(inst);
            return;
        }
    }

    // (0.0 - v) / v -> -1.0
    if (auto fsub = std::dynamic_pointer_cast<ir::FSub>(operand_1)) {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(fsub->get_operands_ref()[0]);
            c && c->get_val() == 0.0f && fsub->get_operands_ref()[1] == operand_2) {
            auto neg_one = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), -1.0f);
            util::substitute(inst, neg_one);
            del_list.insert(inst);
            return;
        }
    }

    reduced_list.push_back(inst);
}

}  // namespace opt::pass
