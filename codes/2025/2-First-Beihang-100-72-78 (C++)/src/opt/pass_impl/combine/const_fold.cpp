#include <cassert>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/module.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
static bool modified = false;
void fold_cal_inst(ir::Module *module);
void optimize_power_of_2_mod(ir::Module *module);
void optimize_arithmetic_distribution(ir::Module *module);
void optimize_strength_reduction(ir::Module *module);
void optimize_constant_sub_add_patterns(ir::Module *module);
void optimize_mul_div_patterns(ir::Module *module);
void optimize_constant_chain_folding(ir::Module *module);
void optimize_arithmetic_identity_patterns(ir::Module *module);
void optimize_sub_add_patterns(ir::Module *module);
void optimize_mul_plus_var_patterns(ir::Module *module);
void optimize_consecutive_negation(ir::Module *module);
void optimize_mul_rem_patterns(ir::Module *module);
void optimize_byte_packing_patterns(ir::Module *module);
void optimize_power_of_two_mul_rem_identity(ir::Module *module);
void optimize_enhanced_consecutive_negation(ir::Module *module);
void optimize_negated_addition_patterns(ir::Module *module);
void optimize_rotl_simulation_patterns(ir::Module *module);
void optimize_arithmetic_constant_reassociation(ir::Module *module);
void optimize_subtract_same_patterns(ir::Module *module);
void optimize_addition_chain_folding(ir::Module *module);
std::shared_ptr<ir::Value> simplify_inst(std::shared_ptr<ir::Instruction> inst);

bool ConstFold::run(ir::Module *module) {
    logger::INFO("Running ConstFold...");
    bool any_modified = false;
    do {
        modified = false;
        fold_cal_inst(module);
        optimize_power_of_2_mod(module);
        optimize_arithmetic_distribution(module);
        optimize_constant_sub_add_patterns(module);
        optimize_mul_div_patterns(module);
        optimize_constant_chain_folding(module);
        optimize_sub_add_patterns(module);
        optimize_arithmetic_identity_patterns(module);
        optimize_mul_plus_var_patterns(module);
        optimize_consecutive_negation(module);
        optimize_mul_rem_patterns(module);
        optimize_byte_packing_patterns(module);
        // optimize_power_of_two_mul_rem_identity(module);
        optimize_enhanced_consecutive_negation(module);
        optimize_negated_addition_patterns(module);
        optimize_rotl_simulation_patterns(module);
        optimize_arithmetic_constant_reassociation(module);
        optimize_subtract_same_patterns(module);
        optimize_addition_chain_folding(module);
        //  optimize_strength_reduction(module);  // 后端已有更好的优化
        any_modified |= modified;
    } while (modified);
    // util::static_check(module);
    return any_modified;
}

void fold_cal_inst(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            for (auto it = instructions.begin(); it != instructions.end();) {
                auto inst = *it;
                auto new_val = simplify_inst(inst);
                if (new_val != inst) {
                    modified = true;
                    it = util::substitute(inst, new_val);
                } else {
                    ++it;
                }
            }
        });
    });
}

// helper function to make binary instruction constant fold(two operands are both constant)
std::optional<std::shared_ptr<ir::Constant>> try_fold_binary_constant(std::shared_ptr<ir::Instruction> inst,
                                                                      std::shared_ptr<ir::Value> left,
                                                                      std::shared_ptr<ir::Value> right) {
    // TODO(Xingkun): Is it necessary to fold AND, XOR, etc.?
    auto left_const = std::dynamic_pointer_cast<ir::Constant>(left);
    auto right_const = std::dynamic_pointer_cast<ir::Constant>(right);
    if (!left_const || !right_const) {
        return std::nullopt;
    }

    // clang-format off
    switch (inst->get_ins_type()) {
        case ir::Instruction::InstructionType::ADD:
        case ir::Instruction::InstructionType::FADD: return *left_const + *right_const;
        case ir::Instruction::InstructionType::SUB:
        case ir::Instruction::InstructionType::FSUB: return *left_const - *right_const;
        case ir::Instruction::InstructionType::MUL:
        case ir::Instruction::InstructionType::FMUL: return *left_const * *right_const;
        case ir::Instruction::InstructionType::SDIV:
        case ir::Instruction::InstructionType::FDIV: return *left_const / *right_const;
        case ir::Instruction::InstructionType::SREM:
        case ir::Instruction::InstructionType::FREM: return *left_const % *right_const;
        case ir::Instruction::InstructionType::AND: {
            auto left_int = std::dynamic_pointer_cast<ir::ConstantInt>(left_const);
            auto right_int = std::dynamic_pointer_cast<ir::ConstantInt>(right_const);
            if (left_int && right_int) {
                return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), left_int->get_val() & right_int->get_val());
            }
            return std::nullopt;
        }
        case ir::Instruction::InstructionType::OR: {
            auto left_int = std::dynamic_pointer_cast<ir::ConstantInt>(left_const);
            auto right_int = std::dynamic_pointer_cast<ir::ConstantInt>(right_const);
            if (left_int && right_int) {
                return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), left_int->get_val() | right_int->get_val());
            }
            return std::nullopt;
        }
        case ir::Instruction::InstructionType::XOR: {
            auto left_int = std::dynamic_pointer_cast<ir::ConstantInt>(left_const);
            auto right_int = std::dynamic_pointer_cast<ir::ConstantInt>(right_const);
            if (left_int && right_int) {
                return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), left_int->get_val() ^ right_int->get_val());
            }
            return std::nullopt;
        }
        case ir::Instruction::InstructionType::ICMP: {
            auto left_int = std::dynamic_pointer_cast<ir::ConstantInt>(left_const);
            auto right_int = std::dynamic_pointer_cast<ir::ConstantInt>(right_const);
            auto icmp_inst = std::dynamic_pointer_cast<ir::ICmp>(inst);
            switch (icmp_inst->get_cmp_type()) {
                case ir::ICmp::ICmpType::EQ: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_int == *right_int);
                case ir::ICmp::ICmpType::NE: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_int != *right_int);
                case ir::ICmp::ICmpType::SGT:
                case ir::ICmp::ICmpType::UGT: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_int > *right_int);
                case ir::ICmp::ICmpType::SGE:
                case ir::ICmp::ICmpType::UGE: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_int >= *right_int);
                case ir::ICmp::ICmpType::SLT:
                case ir::ICmp::ICmpType::ULT: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_int < *right_int);
                case ir::ICmp::ICmpType::SLE:
                case ir::ICmp::ICmpType::ULE: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_int <= *right_int);
                default: __builtin_unreachable();
            }
        }
        case ir::Instruction::InstructionType::FCMP: {
            auto left_float = std::dynamic_pointer_cast<ir::ConstantFloat>(left_const);
            auto right_float = std::dynamic_pointer_cast<ir::ConstantFloat>(right_const);
            auto fcmp_inst = std::dynamic_pointer_cast<ir::FCmp>(inst);
            switch (fcmp_inst->get_cmp_type()) {
                case ir::FCmp::FCmpType::OEQ:
                case ir::FCmp::FCmpType::UEQ: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_float == *right_float);
                case ir::FCmp::FCmpType::OGT:
                case ir::FCmp::FCmpType::UGT: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_float > *right_float);
                case ir::FCmp::FCmpType::OGE:
                case ir::FCmp::FCmpType::UGE: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_float >= *right_float);
                case ir::FCmp::FCmpType::OLT:
                case ir::FCmp::FCmpType::ULT: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_float < *right_float);
                case ir::FCmp::FCmpType::OLE:
                case ir::FCmp::FCmpType::ULE: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_float <= *right_float);
                case ir::FCmp::FCmpType::ONE:
                case ir::FCmp::FCmpType::UNE: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_float != *right_float);
                case ir::FCmp::FCmpType::ORD:
                case ir::FCmp::FCmpType::UNO: return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), *left_float != *right_float);
                default: __builtin_unreachable();
            }
        }

        default: return std::nullopt;
    }
    // clang-format on
}

// clang-format off
static const std::unordered_map<ir::Instruction::InstructionType, std::vector<std::function<std::optional<std::shared_ptr<ir::Value>>(std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>)>>> BINARY_INST_SIMPLIFY_PRINCIPLES = {
    // Add
    {ir::Instruction::InstructionType::ADD, {
        // a + 0 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(left); const_int && const_int->get_val() == 0) return right;
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == 0) return left;
            return std::nullopt;
        },
        // a + (const - a) = const
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto sub_inst = std::dynamic_pointer_cast<ir::Sub>(right)) {
                auto right_lhs = sub_inst->get_operands_ref()[0];
                auto right_rhs = sub_inst->get_operands_ref()[1];
                if (std::dynamic_pointer_cast<ir::Constant>(right_lhs) && right_lhs == left) return right_rhs;
            }
            if (auto sub_inst = std::dynamic_pointer_cast<ir::Sub>(left)) {
                auto left_lhs = sub_inst->get_operands_ref()[0];
                auto left_rhs = sub_inst->get_operands_ref()[1];
                if (std::dynamic_pointer_cast<ir::Constant>(left_lhs) && left_lhs == right) return left_rhs;
            }
            return std::nullopt;
        },
        // (x - C) + C = x   C + (x - C) = x
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            auto sub = std::dynamic_pointer_cast<ir::Sub>(left);
            auto c   = std::dynamic_pointer_cast<ir::ConstantInt>(right);
            if (sub && c) {
                auto sub_left = sub->get_operands_ref()[0];
                auto sub_right = sub->get_operands_ref()[1];
                if (auto k = std::dynamic_pointer_cast<ir::ConstantInt>(sub_right);
                    k && k->get_val() == c->get_val())
                    return sub_left;
            }
            sub = std::dynamic_pointer_cast<ir::Sub>(right);
            c   = std::dynamic_pointer_cast<ir::ConstantInt>(left);
            if (sub && c) {
                auto sub_left = sub->get_operands_ref()[0];
                auto sub_right = sub->get_operands_ref()[1];
                if (auto k = std::dynamic_pointer_cast<ir::ConstantInt>(sub_right);
                    k && k->get_val() == c->get_val())
                    return sub_left;
            }
            return std::nullopt;
        },
        // C + (0 - C) = 0 (symmetric case)
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            auto const_left = std::dynamic_pointer_cast<ir::ConstantInt>(left);
            auto sub_right = std::dynamic_pointer_cast<ir::Sub>(right);
            if (const_left && sub_right) {
                auto sub_ops = sub_right->get_operands_ref();
                if (auto zero = std::dynamic_pointer_cast<ir::ConstantInt>(sub_ops[0]);
                    zero && zero->get_val() == 0) {
                    if (auto const_right = std::dynamic_pointer_cast<ir::ConstantInt>(sub_ops[1]);
                        const_right && const_right->get_val() == const_left->get_val()) {
                        return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
                    }
                }
            }
            return std::nullopt;
        },

    }},
    // FAdd
    {ir::Instruction::InstructionType::FADD, {
        // a + 0.0 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(left); const_float && const_float->get_val() == 0.0) return right;
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(right); const_float && const_float->get_val() == 0.0) return left;
            return std::nullopt;
        },
        // a + (const - a) = const
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto sub_inst = std::dynamic_pointer_cast<ir::FSub>(right)) {
                auto right_lhs = sub_inst->get_operands_ref()[0];
                auto right_rhs = sub_inst->get_operands_ref()[1];
                if (std::dynamic_pointer_cast<ir::ConstantFloat>(right_lhs) && right_lhs == left) return right_rhs;
            }
            if (auto sub_inst = std::dynamic_pointer_cast<ir::FSub>(left)) {
                auto left_lhs = sub_inst->get_operands_ref()[0];
                auto left_rhs = sub_inst->get_operands_ref()[1];
                if (std::dynamic_pointer_cast<ir::ConstantFloat>(left_lhs) && left_lhs == right) return left_rhs;
            }
            return std::nullopt;
        }
    }},
    // Sub
    {ir::Instruction::InstructionType::SUB, {
        // a - 0 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == 0) return left;
            return std::nullopt;
        },
        // a - a = 0
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (left == right) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            return std::nullopt;
        },
        // (x + C) - C = x   (C + x) - C = x
    [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            auto add = std::dynamic_pointer_cast<ir::Add>(left);
            auto c   = std::dynamic_pointer_cast<ir::ConstantInt>(right);
            if (add && c) {
                auto add_left = add->get_operands_ref()[0];
                auto add_right = add->get_operands_ref()[1];
                if (auto k = std::dynamic_pointer_cast<ir::ConstantInt>(add_right);
                    k && k->get_val() == c->get_val())
                    return add_left;
                if (auto k = std::dynamic_pointer_cast<ir::ConstantInt>(add_left);
                    k && k->get_val() == c->get_val())
                    return add_right;
            }
            return std::nullopt;
        },

    }},
    // FSub
    {ir::Instruction::InstructionType::FSUB, {
        // a - 0.0 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(right); const_float && const_float->get_val() == 0.0) return left;
            return std::nullopt;
        },
        // a - a = 0
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (left == right) return std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0F);
            return std::nullopt;
        }
    }},
    // Mul
    {ir::Instruction::InstructionType::MUL, {
        // a * 0 = 0
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(left); const_int && const_int->get_val() == 0) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == 0) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            return std::nullopt;
        },
        // a * 1 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(left); const_int && const_int->get_val() == 1) return right;
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == 1) return left;
            return std::nullopt;
        },
        // a * 2^k = a << k (for power of 2 constants)
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            std::shared_ptr<ir::Value> var = nullptr;
            std::shared_ptr<ir::ConstantInt> const_val = nullptr;

            if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(left)) {
                const_val = c;
                var = right;
            } else if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(right)) {
                const_val = c;
                var = left;
            }

            if (const_val && var) {
                int val = const_val->get_val();
                // Check if val is power of 2 (positive and has only one bit set)
                if (val > 1 && (val & (val - 1)) == 0) {
                    // This is a power of 2 multiplication
                    // Note: In current IR, we don't have shift instructions defined in simplify principles
                    // This optimization should be done in strength reduction or backend
                    return std::nullopt;
                }
            }
            return std::nullopt;
        }
    }},
    // FMul
    {ir::Instruction::InstructionType::FMUL, {
        // a * 0.0 = 0.0
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(left); const_float && const_float->get_val() == 0.0) return std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0F);
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(right); const_float && const_float->get_val() == 0.0) return std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0F);
            return std::nullopt;
        },
        // a * 1.0 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(left); const_float && const_float->get_val() == 1.0) return right;
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(right); const_float && const_float->get_val() == 1.0) return left;
            return std::nullopt;
        }
    }},
    // SDiv
    {ir::Instruction::InstructionType::SDIV, {
        // a / 1 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == 1) return left;
            return std::nullopt;
        },
        // 0 / a = 0
        [](auto left, auto /*right*/) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(left); const_int && const_int->get_val() == 0) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            return std::nullopt;
        },
        // (x * c1) / c2 = x * (c1/c2) when c1 % c2 == 0 (handled in simplify rules for immediate patterns)
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            auto mul = std::dynamic_pointer_cast<ir::Mul>(left);
            auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>(right);
            if (mul && c2 && c2->get_val() != 0) {
                auto mul_ops = mul->get_operands_ref();
                if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[1])) {
                    if (c1->get_val() % c2->get_val() == 0) {
                        int result = c1->get_val() / c2->get_val();
                        if (result == 1) return mul_ops[0]; // x * c / c = x
                        // For result != 1, let optimize_mul_div_patterns handle it
                    }
                } else if (auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[0])) {
                    if (c1->get_val() % c2->get_val() == 0) {
                        int result = c1->get_val() / c2->get_val();
                        if (result == 1) return mul_ops[1]; // c * x / c = x
                        // For result != 1, let optimize_mul_div_patterns handle it
                    }
                }
            }
            return std::nullopt;
        }
    }},
    // FDiv
    {ir::Instruction::InstructionType::FDIV, {
        // a / 1.0 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(right); const_float && const_float->get_val() == 1.0) return left;
            return std::nullopt;
        },
        // 0.0 / a = 0.0
        [](auto left, auto /*right*/) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(left); const_float && const_float->get_val() == 0.0) return std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0F);
            return std::nullopt;
        }
    }},
    // SRem
    {ir::Instruction::InstructionType::SREM, {
        // a % a = 0
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (left == right) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            return std::nullopt;
        },
        // 0 % a = 0
        [](auto left, auto /*right*/) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(left); const_int && const_int->get_val() == 0) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            return std::nullopt;
        },
        // a % 1 = 0
        [](auto /*left*/, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == 1) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            return std::nullopt;
        }
    }},
    // And
    {ir::Instruction::InstructionType::AND, {
        // a & a = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (left == right) return left;
            return std::nullopt;
        },
        // a & 0 = 0
        [](auto /*left*/, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == 0) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            return std::nullopt;
        },
        // a & -1 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == -1) return left;
            return std::nullopt;
        }
    }},
    // Or
    {ir::Instruction::InstructionType::OR, {
        // a | a = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (left == right) return left;
            return std::nullopt;
        },
        // a | 0 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == 0) return left;
            return std::nullopt;
        },
        // a | -1 = -1
        [](auto /*left*/, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == -1) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), -1);
            return std::nullopt;
        }
    }},
    // Xor
    {ir::Instruction::InstructionType::XOR, {
        // a ^ a = 0
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (left == right) return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
            return std::nullopt;
        },
        // a ^ 0 = a
        [](auto left, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right); const_int && const_int->get_val() == 0) return left;
            return std::nullopt;
        }
    }},
    // FRem
    {ir::Instruction::InstructionType::FREM, {
        // a % 1.0 = 0.0
        [](auto /*left*/, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(right); const_float && const_float->get_val() == 1.0) return std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0F);
            return std::nullopt;
        },
        // 0.0 % a = 0.0
        [](auto left, auto /*right*/) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(left); const_float && const_float->get_val() == 0.0) return std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0F);
            return std::nullopt;
        },
        // a % 1.0 = 0.0
        [](auto /*left*/, auto right) -> std::optional<std::shared_ptr<ir::Value>> {
            if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(right); const_float && const_float->get_val() == 1.0) return std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), 0.0F);
            return std::nullopt;
        }
    }}

};
// clang-format on

template <ir::Instruction::InstructionType ty>
std::shared_ptr<ir::Value> simplify_binary_inst(std::shared_ptr<ir::Instruction> inst) {
    static_assert(ty >= ir::Instruction::InstructionType::ADD && ty <= ir::Instruction::InstructionType::XOR);
    auto left = inst->get_operands_ref()[0];
    auto right = inst->get_operands_ref()[1];

    // try fold constant
    if (auto const_val = try_fold_binary_constant(inst, left, right)) {
        return const_val.value();
    }

    // principles
    for (const auto &principle : BINARY_INST_SIMPLIFY_PRINCIPLES.at(ty)) {
        auto res = principle(left, right);
        if (res.has_value()) {
            return res.value();
        }
    }
    return inst;
}

template <ir::Instruction::InstructionType ty>
std::shared_ptr<ir::Value> simplify_cmp_inst(std::shared_ptr<ir::Instruction> inst) {
    static_assert(ty == ir::Instruction::InstructionType::ICMP || ty == ir::Instruction::InstructionType::FCMP);
    auto left = inst->get_operands_ref()[0];
    auto right = inst->get_operands_ref()[1];

    // try fold constant
    if (auto const_val = try_fold_binary_constant(inst, left, right)) {
        return const_val.value();
    }

    // Enhanced comparison optimizations for a cmp a
    if (left == right) {
        if (ty == ir::Instruction::InstructionType::ICMP) {
            auto icmp_type = std::dynamic_pointer_cast<ir::ICmp>(inst)->get_cmp_type();
            switch (icmp_type) {
                case ir::ICmp::ICmpType::EQ:
                case ir::ICmp::ICmpType::SGE:
                case ir::ICmp::ICmpType::SLE:
                case ir::ICmp::ICmpType::UGE:
                case ir::ICmp::ICmpType::ULE:
                    return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), 1);
                case ir::ICmp::ICmpType::NE:
                case ir::ICmp::ICmpType::SGT:
                case ir::ICmp::ICmpType::SLT:
                case ir::ICmp::ICmpType::UGT:
                case ir::ICmp::ICmpType::ULT:
                    return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), 0);
                default:
                    break;
            }
        } else if (ty == ir::Instruction::InstructionType::FCMP) {
            auto fcmp_type = std::dynamic_pointer_cast<ir::FCmp>(inst)->get_cmp_type();
            switch (fcmp_type) {
                case ir::FCmp::FCmpType::OEQ:
                case ir::FCmp::FCmpType::UEQ:
                case ir::FCmp::FCmpType::OGE:
                case ir::FCmp::FCmpType::UGE:
                case ir::FCmp::FCmpType::OLE:
                case ir::FCmp::FCmpType::ULE:
                case ir::FCmp::FCmpType::ORD:
                    return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), 1);
                case ir::FCmp::FCmpType::ONE:
                case ir::FCmp::FCmpType::UNE:
                case ir::FCmp::FCmpType::OGT:
                case ir::FCmp::FCmpType::UGT:
                case ir::FCmp::FCmpType::OLT:
                case ir::FCmp::FCmpType::ULT:
                case ir::FCmp::FCmpType::UNO:
                    return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(1), 0);
                default:
                    break;
            }
        }
    }

    return inst;
}

std::shared_ptr<ir::Value> simplify_conversion_inst(std::shared_ptr<ir::Instruction> inst) {
    if (inst->get_ins_type() == ir::Instruction::InstructionType::ZEXT) {
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(inst->get_operands_ref()[0])) {
            return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), const_int->get_val());
        }
        return inst;
    }
    if (inst->get_ins_type() == ir::Instruction::InstructionType::FPTOSI) {
        if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(inst->get_operands_ref()[0])) {
            return std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32),
                                                     static_cast<int>(const_float->get_val()));
        }
    }
    if (inst->get_ins_type() == ir::Instruction::InstructionType::SITOFP) {
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(inst->get_operands_ref()[0])) {
            return std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), static_cast<float>(const_int->get_val()));
        }
    }
    return inst;
}

std::shared_ptr<ir::Value> simplify_inst(std::shared_ptr<ir::Instruction> inst) {
    // clang-format off
    switch (inst->get_ins_type()) {
        case ir::Instruction::InstructionType::ADD: return simplify_binary_inst<ir::Instruction::InstructionType::ADD>(inst);
        case ir::Instruction::InstructionType::SUB: return simplify_binary_inst<ir::Instruction::InstructionType::SUB>(inst);
        case ir::Instruction::InstructionType::MUL: return simplify_binary_inst<ir::Instruction::InstructionType::MUL>(inst);
        case ir::Instruction::InstructionType::SDIV: return simplify_binary_inst<ir::Instruction::InstructionType::SDIV>(inst);
        case ir::Instruction::InstructionType::SREM: return simplify_binary_inst<ir::Instruction::InstructionType::SREM>(inst);
        case ir::Instruction::InstructionType::AND: return simplify_binary_inst<ir::Instruction::InstructionType::AND>(inst);
        case ir::Instruction::InstructionType::OR: return simplify_binary_inst<ir::Instruction::InstructionType::OR>(inst);
        case ir::Instruction::InstructionType::XOR: return simplify_binary_inst<ir::Instruction::InstructionType::XOR>(inst);
        case ir::Instruction::InstructionType::FADD: return simplify_binary_inst<ir::Instruction::InstructionType::FADD>(inst);
        case ir::Instruction::InstructionType::FSUB: return simplify_binary_inst<ir::Instruction::InstructionType::FSUB>(inst);
        case ir::Instruction::InstructionType::FMUL: return simplify_binary_inst<ir::Instruction::InstructionType::FMUL>(inst);
        case ir::Instruction::InstructionType::FDIV: return simplify_binary_inst<ir::Instruction::InstructionType::FDIV>(inst);
        case ir::Instruction::InstructionType::FREM: return simplify_binary_inst<ir::Instruction::InstructionType::FREM>(inst);
        case ir::Instruction::InstructionType::ICMP: return simplify_cmp_inst<ir::Instruction::InstructionType::ICMP>(inst);
        case ir::Instruction::InstructionType::FCMP: return simplify_cmp_inst<ir::Instruction::InstructionType::FCMP>(inst);
        case ir::Instruction::InstructionType::ZEXT:
        case ir::Instruction::InstructionType::FPTOSI:
        case ir::Instruction::InstructionType::SITOFP: return simplify_conversion_inst(inst);
        default: return inst;
    }
    // clang-format on
}

// Optimize power-of-2 modulo to bitwise AND: x % 2^k => x & (2^k - 1)
void optimize_power_of_2_mod(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto srem_inst = std::dynamic_pointer_cast<ir::SRem>(*it);
                if (!srem_inst) continue;

                auto left = srem_inst->get_operands_ref()[0];
                auto right = srem_inst->get_operands_ref()[1];

                auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right);
                if (!const_int) continue;

                int val = const_int->get_val();
                // Check if val is power of 2 (positive and has only one bit set)
                if (val > 0 && (val & (val - 1)) == 0) {
                    // Check if srem result is only used for comparison with 0
                    bool only_used_for_zero_compare = true;
                    for (auto weak_user : srem_inst->get_users_ref()) {
                        auto user = weak_user.lock();
                        if (!user) continue;

                        auto icmp_inst = std::dynamic_pointer_cast<ir::ICmp>(user);
                        if (!icmp_inst) {
                            only_used_for_zero_compare = false;
                            break;
                        }

                        // Check if comparing with 0
                        auto operands = icmp_inst->get_operands_ref();
                        bool comparing_with_zero = false;
                        for (auto op : operands) {
                            auto const_zero = std::dynamic_pointer_cast<ir::ConstantInt>(op);
                            if (const_zero && const_zero->get_val() == 0) {
                                comparing_with_zero = true;
                                break;
                            }
                        }

                        if (!comparing_with_zero || (icmp_inst->get_cmp_type() != ir::ICmp::ICmpType::EQ &&
                                                     icmp_inst->get_cmp_type() != ir::ICmp::ICmpType::NE)) {
                            only_used_for_zero_compare = false;
                            break;
                        }
                    }

                    // Only optimize if used only for == 0 or != 0 comparisons
                    if (only_used_for_zero_compare) {
                        // Create mask: 2^k - 1
                        auto mask = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), val - 1);
                        // Create AND instruction: left & mask
                        auto and_inst = ir::And::create(basic_block, left, mask, gen_local_var_name());

                        // Insert before the srem instruction
                        basic_block->add_instruction(srem_inst->node, and_inst);

                        // Replace all uses of srem with and
                        util::replace_all_uses_with(srem_inst, and_inst);

                        // Mark for removal
                        to_remove.push_back(srem_inst);
                        modified = true;
                    }
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize arithmetic distribution: b * a + c * a -> (b + c) * a
void optimize_arithmetic_distribution(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto add_inst = std::dynamic_pointer_cast<ir::Add>(*it);
                auto sub_inst = std::dynamic_pointer_cast<ir::Sub>(*it);

                if (!add_inst && !sub_inst) continue;

                auto left = (*it)->get_operands_ref()[0];
                auto right = (*it)->get_operands_ref()[1];

                // Check for pattern: mul + mul or mul - mul
                auto left_mul = std::dynamic_pointer_cast<ir::Mul>(left);
                auto right_mul = std::dynamic_pointer_cast<ir::Mul>(right);

                if (!left_mul || !right_mul) continue;

                // Get operands of both multiplications
                auto left_mul_ops = left_mul->get_operands_ref();
                auto right_mul_ops = right_mul->get_operands_ref();

                // Find common factor
                std::shared_ptr<ir::Value> common_factor = nullptr;
                std::shared_ptr<ir::Value> left_coeff = nullptr;
                std::shared_ptr<ir::Value> right_coeff = nullptr;

                // Check all combinations for common factor
                if (left_mul_ops[0] == right_mul_ops[0]) {
                    common_factor = left_mul_ops[0];
                    left_coeff = left_mul_ops[1];
                    right_coeff = right_mul_ops[1];
                } else if (left_mul_ops[0] == right_mul_ops[1]) {
                    common_factor = left_mul_ops[0];
                    left_coeff = left_mul_ops[1];
                    right_coeff = right_mul_ops[0];
                } else if (left_mul_ops[1] == right_mul_ops[0]) {
                    common_factor = left_mul_ops[1];
                    left_coeff = left_mul_ops[0];
                    right_coeff = right_mul_ops[1];
                } else if (left_mul_ops[1] == right_mul_ops[1]) {
                    common_factor = left_mul_ops[1];
                    left_coeff = left_mul_ops[0];
                    right_coeff = right_mul_ops[0];
                }

                if (!common_factor) continue;

                // Check if we can only use this if the mul instructions have single use
                if (left_mul->get_users_ref().size() > 1 || right_mul->get_users_ref().size() > 1) {
                    continue;
                }

                // Create the new coefficient operation
                std::shared_ptr<ir::Instruction> coeff_op;
                if (add_inst) {
                    // b * a + c * a -> (b + c) * a
                    coeff_op = ir::Add::create(basic_block, left_coeff, right_coeff, gen_local_var_name());
                } else {
                    // b * a - c * a -> (b - c) * a
                    coeff_op = ir::Sub::create(basic_block, left_coeff, right_coeff, gen_local_var_name());
                }

                // Create the new multiplication
                auto new_mul = ir::Mul::create(basic_block, coeff_op, common_factor, gen_local_var_name());

                // Insert the new instructions before the current instruction
                basic_block->add_instruction((*it)->node, coeff_op);
                basic_block->add_instruction((*it)->node, new_mul);

                // Replace all uses
                util::replace_all_uses_with(*it, new_mul);

                // Mark instructions for removal
                to_remove.push_back(*it);
                to_remove.push_back(left_mul);
                to_remove.push_back(right_mul);

                modified = true;
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize strength reduction: a * 2 -> a + a, etc.
void optimize_strength_reduction(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto mul_inst = std::dynamic_pointer_cast<ir::Mul>(*it);
                if (!mul_inst) continue;

                auto left = mul_inst->get_operands_ref()[0];
                auto right = mul_inst->get_operands_ref()[1];

                // Check for a * 2 -> a + a
                auto const_right = std::dynamic_pointer_cast<ir::ConstantInt>(right);
                auto const_left = std::dynamic_pointer_cast<ir::ConstantInt>(left);

                std::shared_ptr<ir::Value> var = nullptr;
                int constant_val = 0;

                if (const_right && const_right->get_val() == 2) {
                    var = left;
                    constant_val = 2;
                } else if (const_left && const_left->get_val() == 2) {
                    var = right;
                    constant_val = 2;
                }

                if (var && constant_val == 2) {
                    // Create a + a instead of a * 2
                    auto add_inst = ir::Add::create(basic_block, var, var, gen_local_var_name());

                    // Insert before the mul instruction
                    basic_block->add_instruction(mul_inst->node, add_inst);

                    // Replace all uses
                    util::replace_all_uses_with(mul_inst, add_inst);

                    // Mark for removal
                    to_remove.push_back(mul_inst);
                    modified = true;
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize (C - x) + C = 2*C - x patterns
void optimize_constant_sub_add_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto add_inst = std::dynamic_pointer_cast<ir::Add>(*it);
                if (!add_inst) continue;

                auto left = add_inst->get_operands_ref()[0];
                auto right = add_inst->get_operands_ref()[1];

                // Pattern 1: (C - x) + C = 2*C - x
                auto sub_l = std::dynamic_pointer_cast<ir::Sub>(left);
                auto c_r = std::dynamic_pointer_cast<ir::ConstantInt>(right);
                if (sub_l && c_r) {
                    auto sub_operands = sub_l->get_operands_ref();
                    auto c_l = std::dynamic_pointer_cast<ir::ConstantInt>(sub_operands[0]);
                    if (c_l && c_l->get_val() == c_r->get_val()) {
                        // Create 2*C - x
                        auto new_c = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c_r->get_val() * 2);
                        auto new_sub = ir::Sub::create(basic_block, new_c, sub_operands[1], gen_local_var_name());

                        // Insert before the add instruction
                        basic_block->add_instruction(add_inst->node, new_sub);

                        // Replace all uses
                        util::replace_all_uses_with(add_inst, new_sub);

                        // Mark for removal
                        to_remove.push_back(add_inst);
                        if (sub_l->get_users_ref().size() == 1) {
                            to_remove.push_back(sub_l);
                        }
                        modified = true;
                        continue;
                    }
                }

                // Pattern 2: C + (C - x) = 2*C - x (symmetric case)
                auto sub_r = std::dynamic_pointer_cast<ir::Sub>(right);
                auto c_l = std::dynamic_pointer_cast<ir::ConstantInt>(left);
                if (sub_r && c_l) {
                    auto sub_operands = sub_r->get_operands_ref();
                    auto c_r_in_sub = std::dynamic_pointer_cast<ir::ConstantInt>(sub_operands[0]);
                    if (c_r_in_sub && c_r_in_sub->get_val() == c_l->get_val()) {
                        // Create 2*C - x
                        auto new_c = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), c_l->get_val() * 2);
                        auto new_sub = ir::Sub::create(basic_block, new_c, sub_operands[1], gen_local_var_name());

                        // Insert before the add instruction
                        basic_block->add_instruction(add_inst->node, new_sub);

                        // Replace all uses
                        util::replace_all_uses_with(add_inst, new_sub);

                        // Mark for removal
                        to_remove.push_back(add_inst);
                        if (sub_r->get_users_ref().size() == 1) {
                            to_remove.push_back(sub_r);
                        }
                        modified = true;
                    }
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize multiplication followed by division: (x * c1) / c2 → x * (c1/c2) when c1 % c2 == 0
void optimize_mul_div_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto div_inst = std::dynamic_pointer_cast<ir::SDiv>(*it);
                if (!div_inst) continue;

                auto left = div_inst->get_operands_ref()[0];
                auto right = div_inst->get_operands_ref()[1];

                // Check pattern: sdiv (mul x, c1), c2
                auto mul_inst = std::dynamic_pointer_cast<ir::Mul>(left);
                auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>(right);

                if (!mul_inst || !c2) continue;
                if (c2->get_val() == 0) continue;  // Avoid division by zero

                auto mul_operands = mul_inst->get_operands_ref();
                std::shared_ptr<ir::Value> x = nullptr;
                std::shared_ptr<ir::ConstantInt> c1 = nullptr;

                // Find the constant in multiplication
                if (auto const_left = std::dynamic_pointer_cast<ir::ConstantInt>(mul_operands[0])) {
                    c1 = const_left;
                    x = mul_operands[1];
                } else if (auto const_right = std::dynamic_pointer_cast<ir::ConstantInt>(mul_operands[1])) {
                    c1 = const_right;
                    x = mul_operands[0];
                }

                if (!c1 || !x) continue;

                // Check if c1 is divisible by c2
                if (c1->get_val() % c2->get_val() == 0) {
                    int new_const = c1->get_val() / c2->get_val();

                    if (new_const == 1) {
                        // (x * c1) / c1 = x
                        util::replace_all_uses_with(div_inst, x);
                        to_remove.push_back(div_inst);
                        if (mul_inst->get_users_ref().size() == 1) {
                            to_remove.push_back(mul_inst);
                        }
                    } else {
                        // (x * c1) / c2 = x * (c1/c2)
                        auto new_c = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), new_const);
                        auto new_mul = ir::Mul::create(basic_block, x, new_c, gen_local_var_name());

                        // Insert before the div instruction
                        basic_block->add_instruction(div_inst->node, new_mul);

                        // Replace all uses
                        util::replace_all_uses_with(div_inst, new_mul);

                        // Mark for removal
                        to_remove.push_back(div_inst);
                        if (mul_inst->get_users_ref().size() == 1) {
                            to_remove.push_back(mul_inst);
                        }
                    }
                    modified = true;
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize constant arithmetic chains by detecting and simplifying small constant expressions
void optimize_constant_chain_folding(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                // Pattern: detect (x * small_const) / large_const where result is likely 0
                // Like: (x * 3) / 1000, when x < 333, result is always 0
                auto div_inst = std::dynamic_pointer_cast<ir::SDiv>(*it);
                if (!div_inst) continue;

                auto left = div_inst->get_operands_ref()[0];
                auto divisor = div_inst->get_operands_ref()[1];

                auto mul_inst = std::dynamic_pointer_cast<ir::Mul>(left);
                auto divisor_const = std::dynamic_pointer_cast<ir::ConstantInt>(divisor);

                if (!mul_inst || !divisor_const) continue;

                auto mul_ops = mul_inst->get_operands_ref();
                std::shared_ptr<ir::ConstantInt> multiplier = nullptr;
                std::shared_ptr<ir::Value> variable = nullptr;

                if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[0])) {
                    multiplier = c;
                    variable = mul_ops[1];
                } else if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[1])) {
                    multiplier = c;
                    variable = mul_ops[0];
                }

                if (!multiplier || !variable) continue;

                // Check for patterns like (x * 3) / 1000
                // If multiplier < divisor, this could often be 0 for reasonable range of x
                int mult_val = multiplier->get_val();
                int div_val = divisor_const->get_val();

                // Special case: if we can prove the division result is likely very small
                // For now, let's focus on more direct optimizations

                // Look ahead to see if there's a following multiplication
                auto next_it = std::next(it);
                if (next_it != instructions.end()) {
                    auto next_mul = std::dynamic_pointer_cast<ir::Mul>(*next_it);
                    if (next_mul) {
                        auto next_ops = next_mul->get_operands_ref();
                        if (next_ops[0] == div_inst) {
                            // Found pattern: div_result * constant
                            if (auto next_const = std::dynamic_pointer_cast<ir::ConstantInt>(next_ops[1])) {
                                // Optimize: (x * c1) / c2 * c3 → x * (c1 * c3) / c2
                                int combined_mult = mult_val * next_const->get_val();

                                // Check for exact division or near-identity patterns
                                if (combined_mult % div_val == 0) {
                                    // Perfect division: can simplify to x * (combined/divisor)
                                    int final_mult = combined_mult / div_val;
                                    if (final_mult == 0) {
                                        // Result is 0
                                        auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
                                        util::replace_all_uses_with(next_mul, zero);
                                        to_remove.push_back(next_mul);
                                        to_remove.push_back(div_inst);
                                        if (mul_inst->get_users_ref().size() == 1) {
                                            to_remove.push_back(mul_inst);
                                        }
                                        modified = true;
                                        ++it;  // Skip the next instruction since we processed it
                                        continue;
                                    } else if (final_mult == 1) {
                                        // Result is just the variable
                                        util::replace_all_uses_with(next_mul, variable);
                                        to_remove.push_back(next_mul);
                                        to_remove.push_back(div_inst);
                                        if (mul_inst->get_users_ref().size() == 1) {
                                            to_remove.push_back(mul_inst);
                                        }
                                        modified = true;
                                        ++it;  // Skip the next instruction
                                        continue;
                                    } else {
                                        // Result is x * final_mult
                                        auto new_const =
                                            std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), final_mult);
                                        auto new_mul =
                                            ir::Mul::create(basic_block, variable, new_const, gen_local_var_name());

                                        basic_block->add_instruction((*it)->node, new_mul);
                                        util::replace_all_uses_with(next_mul, new_mul);

                                        to_remove.push_back(next_mul);
                                        to_remove.push_back(div_inst);
                                        if (mul_inst->get_users_ref().size() == 1) {
                                            to_remove.push_back(mul_inst);
                                        }
                                        modified = true;
                                        ++it;  // Skip the next instruction
                                        continue;
                                    }
                                }
                            }
                        } else if (next_ops[1] == div_inst) {
                            // Found pattern: constant * div_result
                            if (auto next_const = std::dynamic_pointer_cast<ir::ConstantInt>(next_ops[0])) {
                                int combined_mult = mult_val * next_const->get_val();

                                if (combined_mult % div_val == 0) {
                                    int final_mult = combined_mult / div_val;
                                    if (final_mult == 0) {
                                        auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
                                        util::replace_all_uses_with(next_mul, zero);
                                        to_remove.push_back(next_mul);
                                        to_remove.push_back(div_inst);
                                        if (mul_inst->get_users_ref().size() == 1) {
                                            to_remove.push_back(mul_inst);
                                        }
                                        modified = true;
                                        ++it;
                                        continue;
                                    } else if (final_mult == 1) {
                                        util::replace_all_uses_with(next_mul, variable);
                                        to_remove.push_back(next_mul);
                                        to_remove.push_back(div_inst);
                                        if (mul_inst->get_users_ref().size() == 1) {
                                            to_remove.push_back(mul_inst);
                                        }
                                        modified = true;
                                        ++it;
                                        continue;
                                    } else {
                                        auto new_const =
                                            std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), final_mult);
                                        auto new_mul =
                                            ir::Mul::create(basic_block, variable, new_const, gen_local_var_name());

                                        basic_block->add_instruction((*it)->node, new_mul);
                                        util::replace_all_uses_with(next_mul, new_mul);

                                        to_remove.push_back(next_mul);
                                        to_remove.push_back(div_inst);
                                        if (mul_inst->get_users_ref().size() == 1) {
                                            to_remove.push_back(mul_inst);
                                        }
                                        modified = true;
                                        ++it;
                                        continue;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize a - (a + b) = -b patterns
void optimize_sub_add_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto sub_inst = std::dynamic_pointer_cast<ir::Sub>(*it);
                if (!sub_inst) continue;

                auto left = sub_inst->get_operands_ref()[0];
                auto right = sub_inst->get_operands_ref()[1];

                // Check if right is (left + something)
                auto add_inst = std::dynamic_pointer_cast<ir::Add>(right);
                if (!add_inst) continue;

                auto add_ops = add_inst->get_operands_ref();
                std::shared_ptr<ir::Value> other = nullptr;

                if (add_ops[0] == left) {
                    other = add_ops[1];
                } else if (add_ops[1] == left) {
                    other = add_ops[0];
                } else {
                    continue;
                }

                // Pattern: left - (left + other) = -other
                auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
                auto neg = ir::Sub::create(basic_block, zero, other, gen_local_var_name());

                basic_block->add_instruction(sub_inst->node, neg);
                util::replace_all_uses_with(sub_inst, neg);
                to_remove.push_back(sub_inst);

                if (add_inst->get_users_ref().size() == 1) {
                    to_remove.push_back(add_inst);
                }
                modified = true;
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize -x + x = 0 and x + (-x) = 0 patterns
void optimize_arithmetic_identity_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto add_inst = std::dynamic_pointer_cast<ir::Add>(*it);
                if (!add_inst) continue;

                auto left = add_inst->get_operands_ref()[0];
                auto right = add_inst->get_operands_ref()[1];

                // Check both orders: (0 - x) + x and x + (0 - x)
                for (int order = 0; order < 2; ++order) {
                    auto sub_val = (order == 0) ? left : right;
                    auto other_val = (order == 0) ? right : left;

                    auto sub_inst = std::dynamic_pointer_cast<ir::Sub>(sub_val);
                    if (!sub_inst) continue;

                    auto sub_ops = sub_inst->get_operands_ref();
                    auto zero_const = std::dynamic_pointer_cast<ir::ConstantInt>(sub_ops[0]);

                    if (zero_const && zero_const->get_val() == 0 && sub_ops[1] == other_val) {
                        // Pattern: (0 - x) + x = 0 or x + (0 - x) = 0
                        auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
                        util::replace_all_uses_with(add_inst, zero);
                        to_remove.push_back(add_inst);

                        if (sub_inst->get_users_ref().size() == 1) {
                            to_remove.push_back(sub_inst);
                        }
                        modified = true;
                        break;
                    }
                }
            }

            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize x * C + x = x * (C+1) patterns, like x * 8192 + x = x * 8193
void optimize_mul_plus_var_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto add_inst = std::dynamic_pointer_cast<ir::Add>(*it);
                if (!add_inst) continue;

                auto left = add_inst->get_operands_ref()[0];
                auto right = add_inst->get_operands_ref()[1];

                // Try both orders: mul + var and var + mul
                for (int order = 0; order < 2; ++order) {
                    auto mul_val = (order == 0) ? left : right;
                    auto var_val = (order == 0) ? right : left;

                    auto mul_inst = std::dynamic_pointer_cast<ir::Mul>(mul_val);
                    if (!mul_inst) continue;

                    auto mul_ops = mul_inst->get_operands_ref();
                    std::shared_ptr<ir::Value> var_in_mul = nullptr;
                    std::shared_ptr<ir::ConstantInt> const_val = nullptr;

                    // Check if one operand in mul equals var_val
                    if (mul_ops[0] == var_val) {
                        var_in_mul = mul_ops[0];
                        const_val = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[1]);
                    } else if (mul_ops[1] == var_val) {
                        var_in_mul = mul_ops[1];
                        const_val = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[0]);
                    }

                    if (var_in_mul && const_val) {
                        // Pattern: x * C + x = x * (C+1)
                        int new_const = const_val->get_val() + 1;
                        auto new_c = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), new_const);
                        auto new_mul = ir::Mul::create(basic_block, var_val, new_c, gen_local_var_name());

                        // Insert before the add instruction
                        basic_block->add_instruction(add_inst->node, new_mul);

                        // Replace all uses
                        util::replace_all_uses_with(add_inst, new_mul);

                        // Mark for removal
                        to_remove.push_back(add_inst);
                        if (mul_inst->get_users_ref().size() == 1) {
                            to_remove.push_back(mul_inst);
                        }
                        modified = true;
                        break;
                    }
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize consecutive negation: 0 - (0 - x) = x
void optimize_consecutive_negation(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto outer_sub = std::dynamic_pointer_cast<ir::Sub>(*it);
                if (!outer_sub) continue;

                auto left = outer_sub->get_operands_ref()[0];
                auto right = outer_sub->get_operands_ref()[1];

                // Check for pattern: 0 - (0 - x)
                auto zero_left = std::dynamic_pointer_cast<ir::ConstantInt>(left);
                auto inner_sub = std::dynamic_pointer_cast<ir::Sub>(right);

                if (zero_left && zero_left->get_val() == 0 && inner_sub) {
                    auto inner_ops = inner_sub->get_operands_ref();
                    auto zero_inner = std::dynamic_pointer_cast<ir::ConstantInt>(inner_ops[0]);

                    if (zero_inner && zero_inner->get_val() == 0) {
                        // Pattern: 0 - (0 - x) = x
                        auto x = inner_ops[1];
                        util::replace_all_uses_with(outer_sub, x);
                        to_remove.push_back(outer_sub);

                        if (inner_sub->get_users_ref().size() == 1) {
                            to_remove.push_back(inner_sub);
                        }
                        modified = true;
                    }
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize (x * C) % C + (x % C) = x % C patterns
void optimize_mul_rem_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto add_inst = std::dynamic_pointer_cast<ir::Add>(*it);
                if (!add_inst) continue;

                auto left = add_inst->get_operands_ref()[0];
                auto right = add_inst->get_operands_ref()[1];

                // Try both orders: (x*C)%C + x%C and x%C + (x*C)%C
                for (int order = 0; order < 2; ++order) {
                    auto rem1_val = (order == 0) ? left : right;
                    auto rem2_val = (order == 0) ? right : left;

                    auto rem1_inst = std::dynamic_pointer_cast<ir::SRem>(rem1_val);
                    auto rem2_inst = std::dynamic_pointer_cast<ir::SRem>(rem2_val);

                    if (!rem1_inst || !rem2_inst) continue;

                    auto rem1_ops = rem1_inst->get_operands_ref();
                    auto rem2_ops = rem2_inst->get_operands_ref();

                    // Check if both have the same divisor
                    auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(rem1_ops[1]);
                    auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>(rem2_ops[1]);

                    if (!c1 || !c2 || c1->get_val() != c2->get_val()) continue;

                    // Check if first operand is (x * C) and second is x
                    auto mul_inst = std::dynamic_pointer_cast<ir::Mul>(rem1_ops[0]);
                    if (!mul_inst) continue;

                    auto mul_ops = mul_inst->get_operands_ref();
                    std::shared_ptr<ir::Value> x = nullptr;
                    std::shared_ptr<ir::ConstantInt> mul_const = nullptr;

                    if (mul_ops[0] == rem2_ops[0]) {
                        x = mul_ops[0];
                        mul_const = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[1]);
                    } else if (mul_ops[1] == rem2_ops[0]) {
                        x = mul_ops[1];
                        mul_const = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[0]);
                    }

                    if (x && mul_const && mul_const->get_val() == c1->get_val()) {
                        // Pattern: (x * C) % C + x % C = x % C
                        util::replace_all_uses_with(add_inst, rem2_inst);
                        to_remove.push_back(add_inst);

                        if (rem1_inst->get_users_ref().size() == 1) {
                            to_remove.push_back(rem1_inst);
                            if (mul_inst->get_users_ref().size() == 1) {
                                to_remove.push_back(mul_inst);
                            }
                        }
                        modified = true;
                        break;
                    }
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

void optimize_byte_packing_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto final_add = std::dynamic_pointer_cast<ir::Add>(*it);
                if (!final_add) continue;

                // Look for patterns like: ((a*16777216 + b*65536) + c*256) + d
                // This is a common byte packing pattern
                auto left = final_add->get_operands_ref()[0];
                auto right = final_add->get_operands_ref()[1];

                // Check if this looks like the final step of byte packing
                auto prev_add = std::dynamic_pointer_cast<ir::Add>(left);
                if (!prev_add) continue;

                auto prev_left = prev_add->get_operands_ref()[0];
                auto prev_right = prev_add->get_operands_ref()[1];

                // Check if prev_right is c*256
                auto mul_256 = std::dynamic_pointer_cast<ir::Mul>(prev_right);
                if (!mul_256) continue;

                auto mul_256_ops = mul_256->get_operands_ref();
                bool is_256_mul = false;
                for (auto op : mul_256_ops) {
                    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(op)) {
                        if (c->get_val() == 256) {
                            is_256_mul = true;
                            break;
                        }
                    }
                }

                if (!is_256_mul) continue;

                // Check the next level: prev_left should be (a*16777216 + b*65536)
                auto prev_prev_add = std::dynamic_pointer_cast<ir::Add>(prev_left);
                if (!prev_prev_add) continue;

                auto pp_left = prev_prev_add->get_operands_ref()[0];
                auto pp_right = prev_prev_add->get_operands_ref()[1];

                // Check for 16777216 and 65536 multipliers
                auto mul_16777216 = std::dynamic_pointer_cast<ir::Mul>(pp_left);
                auto mul_65536 = std::dynamic_pointer_cast<ir::Mul>(pp_right);

                if (!mul_16777216 || !mul_65536) continue;

                // Verify the constants
                bool has_16777216 = false, has_65536 = false;

                for (auto op : mul_16777216->get_operands_ref()) {
                    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(op)) {
                        if (c->get_val() == 16777216) {
                            has_16777216 = true;
                            break;
                        }
                    }
                }

                for (auto op : mul_65536->get_operands_ref()) {
                    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(op)) {
                        if (c->get_val() == 65536) {
                            has_65536 = true;
                            break;
                        }
                    }
                }

                if (has_16777216 && has_65536) {
                    // This is definitely a 32-bit big-endian byte packing pattern
                    // We could optimize this by recognizing it as a byte-swap operation
                    // For now, we'll let the existing arithmetic optimizations handle it
                    // But we could add a comment or specific handling here in the future

                    // Skip optimization for now, but this pattern is recognized
                    continue;
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

void optimize_power_of_two_mul_rem_identity(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto add_inst = std::dynamic_pointer_cast<ir::Add>(*it);
                if (!add_inst) continue;

                auto left = add_inst->get_operands_ref()[0];
                auto right = add_inst->get_operands_ref()[1];

                // Try both orders: mul + rem and rem + mul
                for (int order = 0; order < 2; ++order) {
                    auto mul_val = (order == 0) ? left : right;
                    auto rem_val = (order == 0) ? right : left;

                    auto mul_inst = std::dynamic_pointer_cast<ir::Mul>(mul_val);
                    auto rem_inst = std::dynamic_pointer_cast<ir::SRem>(rem_val);

                    if (!mul_inst || !rem_inst) continue;

                    auto mul_ops = mul_inst->get_operands_ref();
                    auto rem_ops = rem_inst->get_operands_ref();

                    // Check if both operate on the same variable
                    std::shared_ptr<ir::Value> variable = nullptr;
                    std::shared_ptr<ir::ConstantInt> mul_const = nullptr;
                    std::shared_ptr<ir::ConstantInt> rem_const = nullptr;

                    // Check mul operands
                    if (mul_ops[0] == rem_ops[0]) {
                        variable = mul_ops[0];
                        mul_const = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[1]);
                        rem_const = std::dynamic_pointer_cast<ir::ConstantInt>(rem_ops[1]);
                    } else if (mul_ops[1] == rem_ops[0]) {
                        variable = mul_ops[1];
                        mul_const = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[0]);
                        rem_const = std::dynamic_pointer_cast<ir::ConstantInt>(rem_ops[1]);
                    }

                    if (variable && mul_const && rem_const && mul_const->get_val() == rem_const->get_val()) {
                        int constant = mul_const->get_val();

                        // Check if constant is power of 2
                        if (constant > 0 && (constant & (constant - 1)) == 0) {
                            // Pattern: (x * 2^k) + (x % 2^k) = x
                            // This is mathematically correct for non-negative integers
                            util::replace_all_uses_with(add_inst, variable);
                            to_remove.push_back(add_inst);

                            if (mul_inst->get_users_ref().size() == 1) {
                                to_remove.push_back(mul_inst);
                            }
                            if (rem_inst->get_users_ref().size() == 1) {
                                to_remove.push_back(rem_inst);
                            }
                            modified = true;
                            break;
                        }
                    }
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Enhanced consecutive negation optimizer that handles more complex patterns
void optimize_enhanced_consecutive_negation(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto outer_sub = std::dynamic_pointer_cast<ir::Sub>(*it);
                if (!outer_sub) continue;

                auto left = outer_sub->get_operands_ref()[0];
                auto right = outer_sub->get_operands_ref()[1];

                // Pattern 1: 0 - (0 - x) = x (already handled in basic version)
                auto zero_left = std::dynamic_pointer_cast<ir::ConstantInt>(left);
                auto inner_sub = std::dynamic_pointer_cast<ir::Sub>(right);

                if (zero_left && zero_left->get_val() == 0 && inner_sub) {
                    auto inner_ops = inner_sub->get_operands_ref();
                    auto zero_inner = std::dynamic_pointer_cast<ir::ConstantInt>(inner_ops[0]);

                    if (zero_inner && zero_inner->get_val() == 0) {
                        // Pattern: 0 - (0 - x) = x
                        auto x = inner_ops[1];
                        util::replace_all_uses_with(outer_sub, x);
                        to_remove.push_back(outer_sub);

                        if (inner_sub->get_users_ref().size() == 1) {
                            to_remove.push_back(inner_sub);
                        }
                        modified = true;
                        continue;
                    }
                }

                // Pattern 2: 0 - (expr + constant) where expr came from 0 - something
                // This handles chains like: 0 - ((0 - x) + y) which should become x - y
                if (zero_left && zero_left->get_val() == 0) {
                    auto add_inst = std::dynamic_pointer_cast<ir::Add>(right);
                    if (add_inst) {
                        auto add_ops = add_inst->get_operands_ref();

                        // Check if one operand is (0 - something)
                        for (int i = 0; i < 2; ++i) {
                            auto neg_sub = std::dynamic_pointer_cast<ir::Sub>(add_ops[i]);
                            if (neg_sub) {
                                auto neg_ops = neg_sub->get_operands_ref();
                                auto neg_zero = std::dynamic_pointer_cast<ir::ConstantInt>(neg_ops[0]);

                                if (neg_zero && neg_zero->get_val() == 0) {
                                    // Pattern: 0 - ((0 - x) + y) = x - y
                                    auto x = neg_ops[1];
                                    auto y = add_ops[1 - i];

                                    auto new_sub = ir::Sub::create(basic_block, x, y, gen_local_var_name());
                                    basic_block->add_instruction(outer_sub->node, new_sub);
                                    util::replace_all_uses_with(outer_sub, new_sub);

                                    to_remove.push_back(outer_sub);
                                    if (add_inst->get_users_ref().size() == 1) {
                                        to_remove.push_back(add_inst);
                                        if (neg_sub->get_users_ref().size() == 1) {
                                            to_remove.push_back(neg_sub);
                                        }
                                    }
                                    modified = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize addition chains like: ((a + b) + c) + d + e
// This can help reduce instruction count in long arithmetic sequences
void optimize_addition_chain_folding(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto final_add = std::dynamic_pointer_cast<ir::Add>(*it);
                if (!final_add) continue;

                // Look for chains of additions: ((x + a) + b) + c
                // Where a, b, c are constants, we can combine them into x + (a+b+c)
                std::vector<std::shared_ptr<ir::ConstantInt>> constants;
                std::vector<std::shared_ptr<ir::Value>> variables;
                std::vector<std::shared_ptr<ir::Add>> add_chain;

                // Traverse the addition chain
                auto current_add = final_add;
                bool is_valid_chain = true;

                while (current_add && is_valid_chain) {
                    add_chain.push_back(current_add);
                    auto ops = current_add->get_operands_ref();

                    // Check if one operand is constant and other is either variable or another add
                    std::shared_ptr<ir::ConstantInt> const_op = nullptr;
                    std::shared_ptr<ir::Value> other_op = nullptr;

                    if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(ops[0])) {
                        const_op = c;
                        other_op = ops[1];
                    } else if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(ops[1])) {
                        const_op = c;
                        other_op = ops[0];
                    }

                    if (const_op) {
                        constants.push_back(const_op);

                        // Check if other operand is another add
                        auto next_add = std::dynamic_pointer_cast<ir::Add>(other_op);
                        if (next_add) {
                            current_add = next_add;
                        } else {
                            // This is the base variable
                            variables.push_back(other_op);
                            current_add = nullptr;
                        }
                    } else {
                        // No constant found, not a valid chain
                        is_valid_chain = false;
                    }
                }

                // If we found a chain with at least 2 constants and 1 variable, optimize it
                if (is_valid_chain && constants.size() >= 2 && variables.size() == 1) {
                    // Calculate sum of all constants
                    int total_constant = 0;
                    for (const auto &const_val : constants) {
                        total_constant += const_val->get_val();
                    }

                    // Create optimized instruction: variable + total_constant
                    if (total_constant == 0) {
                        // Sum is 0, just use the variable
                        util::replace_all_uses_with(final_add, variables[0]);
                        to_remove.push_back(final_add);
                    } else {
                        auto new_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), total_constant);
                        auto new_add = ir::Add::create(basic_block, variables[0], new_const, gen_local_var_name());

                        basic_block->add_instruction(final_add->node, new_add);
                        util::replace_all_uses_with(final_add, new_add);
                        to_remove.push_back(final_add);
                    }

                    // Mark intermediate add instructions for removal if they have single use
                    for (int i = 1; i < add_chain.size(); ++i) {
                        if (add_chain[i]->get_users_ref().size() == 1) {
                            to_remove.push_back(add_chain[i]);
                        }
                    }

                    modified = true;
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize patterns like: 0 - (a + b) -> (-a) + (-b)
// This can help identify more optimization opportunities
void optimize_negated_addition_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto sub_inst = std::dynamic_pointer_cast<ir::Sub>(*it);
                if (!sub_inst) continue;

                auto operands = sub_inst->get_operands_ref();
                auto left = operands[0];
                auto right = operands[1];

                // Look for pattern: 0 - (a + b)
                auto zero_const = std::dynamic_pointer_cast<ir::ConstantInt>(left);
                if (!zero_const || zero_const->get_val() != 0) continue;

                auto add_inst = std::dynamic_pointer_cast<ir::Add>(right);
                if (!add_inst) continue;

                auto add_operands = add_inst->get_operands_ref();
                auto a = add_operands[0];
                auto b = add_operands[1];

                // For -(a + b), this pattern appears frequently but doesn't necessarily
                // need to be transformed to (-a) + (-b) as it may not reduce instructions.
                // Skip this optimization for now as it may increase instruction count.
            }

            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

void optimize_rotl_simulation_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto add_inst = std::dynamic_pointer_cast<ir::Add>(*it);
                if (!add_inst) continue;

                auto left = add_inst->get_operands_ref()[0];
                auto right = add_inst->get_operands_ref()[1];

                // Try both orders: mul + rem and rem + mul
                for (int order = 0; order < 2; ++order) {
                    auto mul_val = (order == 0) ? left : right;
                    auto rem_val = (order == 0) ? right : left;

                    auto mul_inst = std::dynamic_pointer_cast<ir::Mul>(mul_val);
                    auto rem_inst = std::dynamic_pointer_cast<ir::SRem>(rem_val);

                    if (!mul_inst || !rem_inst) continue;

                    auto mul_ops = mul_inst->get_operands_ref();
                    auto rem_ops = rem_inst->get_operands_ref();

                    // Check if they operate on the same variable
                    std::shared_ptr<ir::Value> var = nullptr;
                    std::shared_ptr<ir::ConstantInt> const_val = nullptr;

                    if (mul_ops[0] == rem_ops[0]) {
                        var = mul_ops[0];
                        const_val = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[1]);
                    } else if (mul_ops[1] == rem_ops[0]) {
                        var = mul_ops[1];
                        const_val = std::dynamic_pointer_cast<ir::ConstantInt>(mul_ops[0]);
                    }

                    auto rem_const = std::dynamic_pointer_cast<ir::ConstantInt>(rem_ops[1]);

                    if (var && const_val && rem_const && const_val->get_val() == rem_const->get_val() &&
                        const_val->get_val() > 0 && (const_val->get_val() & (const_val->get_val() - 1)) == 0) {
                        // This pattern represents a rotation simulation
                        // For certain cases like x*2 + x%2, this might be optimizable
                        // But we need to be careful about the mathematical meaning

                        // For now, just add a comment for future optimization
                        // This could be optimized to actual rotation instructions in the backend

                        // Don't optimize this pattern for now as it's complex
                        continue;
                    }
                }
            }
        });
    });
}

// Optimize patterns like: a - a = 0, (a + b) - a = b, (a + b) - b = a
void optimize_subtract_same_patterns(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto sub_inst = std::dynamic_pointer_cast<ir::Sub>(*it);
                if (!sub_inst) continue;

                auto operands = sub_inst->get_operands_ref();
                auto left = operands[0];
                auto right = operands[1];

                // Pattern 1: a - a = 0
                if (left == right) {
                    auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
                    util::replace_all_uses_with(sub_inst, zero);
                    to_remove.push_back(sub_inst);
                    modified = true;
                    continue;
                }

                // Pattern 2: (a + b) - a = b, (a + b) - b = a
                auto add_inst = std::dynamic_pointer_cast<ir::Add>(left);
                if (add_inst) {
                    auto add_operands = add_inst->get_operands_ref();
                    auto add_left = add_operands[0];
                    auto add_right = add_operands[1];

                    if (add_left == right) {
                        // (a + b) - a = b
                        util::replace_all_uses_with(sub_inst, add_right);
                        to_remove.push_back(sub_inst);
                        if (add_inst->get_users_ref().size() == 1) {
                            to_remove.push_back(add_inst);
                        }
                        modified = true;
                        continue;
                    } else if (add_right == right) {
                        // (a + b) - b = a
                        util::replace_all_uses_with(sub_inst, add_left);
                        to_remove.push_back(sub_inst);
                        if (add_inst->get_users_ref().size() == 1) {
                            to_remove.push_back(add_inst);
                        }
                        modified = true;
                        continue;
                    }
                }
            }

            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

// Optimize arithmetic constant reassociation patterns from cmmc ArithmeticReduce.cpp
// This implements patterns like:
// add (add x c1) c2 -> add x (c1+c2)
// add (sub x c1) c2 -> add x (c2-c1)
// sub (sub x c1) c2 -> add x -(c1+c2)
// sub (add x c1) c2 -> add x (c1-c2)
void optimize_arithmetic_constant_reassociation(ir::Module *module) {
    module->for_each_func([](auto &&function) {
        function->for_each_block([](auto &&basic_block) {
            auto &instructions = basic_block->get_instructions_ref();
            std::vector<std::shared_ptr<ir::Instruction>> to_remove;

            for (auto it = instructions.begin(); it != instructions.end(); ++it) {
                auto inst = *it;

                // Pattern 1: add (add x c1) c2 -> add x (c1+c2)
                if (auto add_outer = std::dynamic_pointer_cast<ir::Add>(inst)) {
                    auto left = add_outer->get_operands_ref()[0];
                    auto right = add_outer->get_operands_ref()[1];

                    // Try both operand orders: (add x c1) + c2 and c2 + (add x c1)
                    for (int order = 0; order < 2; ++order) {
                        auto add_inner = std::dynamic_pointer_cast<ir::Add>((order == 0) ? left : right);
                        auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>((order == 0) ? right : left);

                        if (add_inner && c2) {
                            auto inner_ops = add_inner->get_operands_ref();

                            // Find the constant in inner add: x + c1 or c1 + x
                            std::shared_ptr<ir::Value> x = nullptr;
                            std::shared_ptr<ir::ConstantInt> c1 = nullptr;

                            if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(inner_ops[0])) {
                                c1 = c;
                                x = inner_ops[1];
                            } else if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(inner_ops[1])) {
                                c1 = c;
                                x = inner_ops[0];
                            }

                            if (x && c1) {
                                // Create: add x (c1+c2)
                                int combined = c1->get_val() + c2->get_val();
                                if (combined == 0) {
                                    // Result is just x
                                    util::replace_all_uses_with(add_outer, x);
                                } else {
                                    auto new_const =
                                        std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), combined);
                                    auto new_add = ir::Add::create(basic_block, x, new_const, gen_local_var_name());
                                    basic_block->add_instruction(add_outer->node, new_add);
                                    util::replace_all_uses_with(add_outer, new_add);
                                }

                                to_remove.push_back(add_outer);
                                if (add_inner->get_users_ref().size() == 1) {
                                    to_remove.push_back(add_inner);
                                }
                                modified = true;
                                break;
                            }
                        }
                    }
                }

                // Pattern 2: add (sub x c1) c2 -> add x (c2-c1)
                if (auto add_outer = std::dynamic_pointer_cast<ir::Add>(inst)) {
                    auto left = add_outer->get_operands_ref()[0];
                    auto right = add_outer->get_operands_ref()[1];

                    for (int order = 0; order < 2; ++order) {
                        auto sub_inner = std::dynamic_pointer_cast<ir::Sub>((order == 0) ? left : right);
                        auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>((order == 0) ? right : left);

                        if (sub_inner && c2) {
                            auto inner_ops = sub_inner->get_operands_ref();
                            auto x = inner_ops[0];
                            auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(inner_ops[1]);

                            if (c1) {
                                // Create: add x (c2-c1)
                                int combined = c2->get_val() - c1->get_val();
                                if (combined == 0) {
                                    util::replace_all_uses_with(add_outer, x);
                                } else {
                                    auto new_const =
                                        std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), combined);
                                    auto new_add = ir::Add::create(basic_block, x, new_const, gen_local_var_name());
                                    basic_block->add_instruction(add_outer->node, new_add);
                                    util::replace_all_uses_with(add_outer, new_add);
                                }

                                to_remove.push_back(add_outer);
                                if (sub_inner->get_users_ref().size() == 1) {
                                    to_remove.push_back(sub_inner);
                                }
                                modified = true;
                                break;
                            }
                        }
                    }
                }

                // Pattern 3: sub (sub x c1) c2 -> add x -(c1+c2)
                if (auto sub_outer = std::dynamic_pointer_cast<ir::Sub>(inst)) {
                    auto left = sub_outer->get_operands_ref()[0];
                    auto right = sub_outer->get_operands_ref()[1];

                    auto sub_inner = std::dynamic_pointer_cast<ir::Sub>(left);
                    auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>(right);

                    if (sub_inner && c2) {
                        auto inner_ops = sub_inner->get_operands_ref();
                        auto x = inner_ops[0];
                        auto c1 = std::dynamic_pointer_cast<ir::ConstantInt>(inner_ops[1]);

                        if (c1) {
                            // Create: add x -(c1+c2)
                            int combined = -(c1->get_val() + c2->get_val());
                            if (combined == 0) {
                                util::replace_all_uses_with(sub_outer, x);
                            } else {
                                auto new_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), combined);
                                auto new_add = ir::Add::create(basic_block, x, new_const, gen_local_var_name());
                                basic_block->add_instruction(sub_outer->node, new_add);
                                util::replace_all_uses_with(sub_outer, new_add);
                            }

                            to_remove.push_back(sub_outer);
                            if (sub_inner->get_users_ref().size() == 1) {
                                to_remove.push_back(sub_inner);
                            }
                            modified = true;
                        }
                    }
                }

                // Pattern 4: sub (add x c1) c2 -> add x (c1-c2)
                if (auto sub_outer = std::dynamic_pointer_cast<ir::Sub>(inst)) {
                    auto left = sub_outer->get_operands_ref()[0];
                    auto right = sub_outer->get_operands_ref()[1];

                    auto add_inner = std::dynamic_pointer_cast<ir::Add>(left);
                    auto c2 = std::dynamic_pointer_cast<ir::ConstantInt>(right);

                    if (add_inner && c2) {
                        auto inner_ops = add_inner->get_operands_ref();

                        // Find the constant in inner add: x + c1 or c1 + x
                        std::shared_ptr<ir::Value> x = nullptr;
                        std::shared_ptr<ir::ConstantInt> c1 = nullptr;

                        if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(inner_ops[0])) {
                            c1 = c;
                            x = inner_ops[1];
                        } else if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(inner_ops[1])) {
                            c1 = c;
                            x = inner_ops[0];
                        }

                        if (x && c1) {
                            // Create: add x (c1-c2)
                            int combined = c1->get_val() - c2->get_val();
                            if (combined == 0) {
                                util::replace_all_uses_with(sub_outer, x);
                            } else {
                                auto new_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), combined);
                                auto new_add = ir::Add::create(basic_block, x, new_const, gen_local_var_name());
                                basic_block->add_instruction(sub_outer->node, new_add);
                                util::replace_all_uses_with(sub_outer, new_add);
                            }

                            to_remove.push_back(sub_outer);
                            if (add_inner->get_users_ref().size() == 1) {
                                to_remove.push_back(add_inner);
                            }
                            modified = true;
                        }
                    }
                }
            }

            // Remove dead instructions
            for (const auto &inst : to_remove) {
                util::remove_instruction(inst);
            }
        });
    });
}

}  // namespace opt::pass
