#include "rv_translator.hpp"

#include <cmath>
#include <cstdlib>
#include <map>
#include <queue>
#include <unordered_set>

#include "../../config.hpp"
#include "../../ir/instruction.hpp"
#include "../../ir/value.hpp"
#include "log.hpp"
#include "rv_instruction.hpp"
#include "rv_reg_info.hpp"

namespace backend::riscv {

RVModule *RVTranslator::translate(ir::Module *ir_module) {
    RVTranslator translator(ir_module);
    return translator.run();
}

RVModule *RVTranslator::run() {
    // python3 checker.py function -c ../cmake-build-debug/compiler
    rv_module = new RVModule();

    // 翻译所有全局变量
    for (const auto &ir_gv : ir_module->get_global_variables()) {
        translate_global_variable(ir_gv);
    }

    // 处理库函数
    // translate_lib_functions();

    // 翻译所有函数
    for (auto &ir_func : ir_module->get_functions()) {
        translate_function(ir_func);
    }

    return rv_module;
}

void RVTranslator::translate_global_variable(const std::shared_ptr<ir::GlobalVariable> &gv) {
    auto init = gv->get_init_value();
    if (std::dynamic_pointer_cast<ir::ConstantFloat>(init)) return;
    // 判断是否为零初始化
    bool is_zero = false;
    // 递归判断常量是否全为0
    std::function<bool(const std::shared_ptr<ir::Constant> &)> is_all_zero;
    is_all_zero = [&](const std::shared_ptr<ir::Constant> &c) -> bool {
        if (auto ci = std::dynamic_pointer_cast<ir::ConstantInt>(c)) {
            return ci->get_val() == 0;
        } else if (auto cf = std::dynamic_pointer_cast<ir::ConstantFloat>(c)) {
            return cf->get_val() == 0.0F;
        } else if (auto arr = std::dynamic_pointer_cast<ir::ConstantArray>(c)) {
            for (const auto &elem : arr->get_vals()) {
                if (!is_all_zero(elem)) return false;
            }
            return true;
        } else if (dynamic_cast<ir::ZeroInitializer *>(c.get())) {
            return true;
        }
        return false;
    };
    if (!init) {
        is_zero = true;
    } else {
        is_zero = is_all_zero(init);
    }
    // 插入到module
    if (is_zero) {
        rv_module->add_bss_variable(new RVGlobalVar(gv.get(), true));
    } else {
        rv_module->add_data_variable(new RVGlobalVar(gv.get(), false));
    }
}

void RVTranslator::translate_function(std::shared_ptr<ir::Function> ir_func) {
    // 创建RISC-V函数
    current_function = rv_module->new_function(ir_func->get_name());
    current_function->set_return_vir_reg(allocate_register());

    // 生成基本块标签映射
    int block_index = 1;
    std::string func_name = ir_func->get_name();
    if (!func_name.empty() && func_name[0] == '@') {
        func_name = func_name.substr(1);
    }
    for (auto &ir_bb : ir_func->get_basic_blocks()) {
        bb_to_label[ir_bb.get()] = func_name + "_block" + std::to_string(block_index++);
    }

    // 先创建所有 RVBasicBlock 并记录映射
    for (auto &ir_bb : ir_func->get_basic_blocks()) {
        auto label = get_label_name(ir_bb);
        auto *rv_bb = current_function->new_block(label);
        // 设置循环深度信息
        rv_bb->set_loop_depth(ir_bb->opt_info.get_loop_depth());
        bb_to_rvbb[ir_bb.get()] = rv_bb;
        // 建立 RVBasicBlock 到 ir::BasicBlock 的反向映射
        rvbb_to_irbb[rv_bb] = ir_bb.get();
    }

    // 同步前驱/后继信息
    for (auto &ir_bb : ir_func->get_basic_blocks()) {
        auto *rv_bb = bb_to_rvbb[ir_bb.get()];
        // 前驱
        for (auto &pred_weak : ir_bb->opt_info.predecessors) {
            if (auto pred = pred_weak.lock()) {
                auto it = bb_to_rvbb.find(pred.get());
                if (it != bb_to_rvbb.end()) {
                    rv_bb->add_pred(it->second);
                }
            }
        }
        // 后继
        for (auto &succ_weak : ir_bb->opt_info.successors) {
            if (auto succ = succ_weak.lock()) {
                auto it = bb_to_rvbb.find(succ.get());
                if (it != bb_to_rvbb.end()) {
                    rv_bb->add_succ(it->second);
                }
            }
        }
    }

    auto ir_entry_bb = ir_func->get_basic_blocks().front();
    auto rv_entry_bb = bb_to_rvbb[ir_entry_bb.get()];

    // assert(rv_entry_bb->get_loop_depth() == 0);

    // 处理函数参数
    // 为入口基本块创建一个临时的 value_to_operand 映射
    std::unordered_map<ir::Value *, RVOperand *> entry_value_to_operand;
    current_function->translate_args(ir_func->get_arguments_ref(), entry_value_to_operand, rv_entry_bb);

    // 将入口基本块的参数映射复制到新的按块分组的映射中
    for (const auto &[value, operand] : entry_value_to_operand) {
        set_operand_for_block(rv_entry_bb, value, operand);
    }

    bfs(ir_entry_bb);
    // dfs(ir_entry_bb, {});
}

void RVTranslator::dfs(std::shared_ptr<ir::BasicBlock> root,
                       std::unordered_set<std::shared_ptr<ir::BasicBlock>> visited) {
    if (visited.find(root) != visited.end()) {
        return;
    }
    visited.insert(root);
    auto *rv_bb = bb_to_rvbb[root.get()];
    translate_basic_block(root, rv_bb);
    for (auto &weak_immediate_dominated : root->opt_info.immediate_dominated) {
        auto immediate_dominated = weak_immediate_dominated.lock();
        dfs(immediate_dominated, visited);
    }
}

void RVTranslator::bfs(std::shared_ptr<ir::BasicBlock> root) {
    std::queue<std::shared_ptr<ir::BasicBlock>> queue;
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> visited;

    // 将根节点加入队列
    queue.push(root);
    visited.insert(root);

    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();

        // 翻译当前基本块
        auto *rv_bb = bb_to_rvbb[current.get()];
        translate_basic_block(current, rv_bb);

        // 将所有直接支配的基本块加入队列
        for (auto &weak_immediate_dominated : current->opt_info.immediate_dominated) {
            auto immediate_dominated = weak_immediate_dominated.lock();
            // logger::INFO(current->get_name(), " ",immediate_dominated->get_name());
            if (immediate_dominated && visited.find(immediate_dominated) == visited.end()) {
                queue.push(immediate_dominated);
                visited.insert(immediate_dominated);
            }
        }
    }
}

void RVTranslator::translate_basic_block(std::shared_ptr<ir::BasicBlock> ir_bb, RVBasicBlock::Ptr rv_bb) {
    // 直接在传入的 rv_bb 上翻译基本块中的所有指令
    for (auto &ir_inst : ir_bb->get_instructions()) {
        translate_instruction(ir_inst, rv_bb);
    }
}

void RVTranslator::translate_instruction(std::shared_ptr<ir::Instruction> ir_inst, RVBasicBlock::Ptr bb) {
    // logger::INFO(ir_inst->to_string());

    // 设置当前基本块，以便在递归调用时能够正确访问
    current_basic_block = bb;

    using InstrType = ir::Instruction::InstructionType;

    switch (ir_inst->get_ins_type()) {
        // 二元运算指令
        case InstrType::ADD:
        case InstrType::SUB:
        case InstrType::MUL:
        case InstrType::SDIV:
        case InstrType::SREM:
        case InstrType::AND:
        case InstrType::OR:
        case InstrType::XOR:
        case InstrType::SHL:
        case InstrType::LSHR:
        case InstrType::ASHR:
            translate_binary_op(ir_inst, bb);
            break;

        // 浮点运算指令
        case InstrType::FADD:
        case InstrType::FSUB:
        case InstrType::FMUL:
        case InstrType::FDIV:
            translate_binary_op(ir_inst, bb);
            break;
        case InstrType::FNEG:
            translate_unary_op(ir_inst, bb);
            break;
        case InstrType::PHI:
            translate_phi(std::dynamic_pointer_cast<ir::Phi>(ir_inst), bb);
            break;
        case InstrType::MOVE:
            translate_move(std::dynamic_pointer_cast<ir::Move>(ir_inst), bb);
            break;

        // 内存操作指令
        case InstrType::LOAD:
            translate_load(std::dynamic_pointer_cast<ir::Load>(ir_inst), bb);
            break;
        case InstrType::STORE:
            translate_store(std::dynamic_pointer_cast<ir::Store>(ir_inst), bb);
            break;
        case InstrType::ALLOCA:
            translate_alloca(std::dynamic_pointer_cast<ir::Alloca>(ir_inst), bb);
            break;
        case InstrType::MEMSET:
            translate_memset(std::dynamic_pointer_cast<ir::Memset>(ir_inst), bb);
            break;

        // 比较指令
        case InstrType::ICMP:
            translate_icmp(std::dynamic_pointer_cast<ir::ICmp>(ir_inst), bb);
            break;
        case InstrType::FCMP:
            translate_fcmp(std::dynamic_pointer_cast<ir::FCmp>(ir_inst), bb);
            break;

        // 控制流指令
        case InstrType::BR:
            translate_br(std::dynamic_pointer_cast<ir::Br>(ir_inst), bb);
            break;
        case InstrType::RET:
            translate_ret(std::dynamic_pointer_cast<ir::Ret>(ir_inst), bb);
            break;
        case InstrType::CALL:
            translate_call(std::dynamic_pointer_cast<ir::Call>(ir_inst), bb);
            break;
        case InstrType::GETELEMENTPTR:
            translate_gep(std::dynamic_pointer_cast<ir::Getelementptr>(ir_inst), bb);
            break;
        case InstrType::TRUNC:
        case InstrType::ZEXT:
        case InstrType::BITCAST:
        case InstrType::FPTOSI:
        case InstrType::SITOFP:
            translate_conversion(ir_inst, bb);
            break;

        default:
            assert(false && "[RVTranslator] 未实现的指令类型: %d\n");
            break;
    }
}

void RVTranslator::translate_binary_op(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    // 检查操作数数量
    auto &operands = inst->get_operands_ref();
    switch (inst->get_ins_type()) {
        case ir::Instruction::InstructionType::ADD:
            translate_add(inst, bb);
            break;
        case ir::Instruction::InstructionType::SUB:
            translate_sub(inst, bb);
            break;
        case ir::Instruction::InstructionType::MUL:
            translate_mul(inst, bb);
            break;
        case ir::Instruction::InstructionType::SDIV:
            translate_div(inst, bb);
            break;
        case ir::Instruction::InstructionType::SREM:
            translate_mod(inst, bb);
            break;
        case ir::Instruction::InstructionType::AND:
            translate_and(inst, bb);
            break;
        case ir::Instruction::InstructionType::OR:
            translate_or(inst, bb);
            break;
        case ir::Instruction::InstructionType::XOR:
            translate_xor(inst, bb);
            break;
        case ir::Instruction::InstructionType::SHL:
        case ir::Instruction::InstructionType::LSHR:
        case ir::Instruction::InstructionType::ASHR:
            // 移位操作保持原有实现
            {
                auto operand1 = get_operand(bb, operands[0]);
                auto operand2 = get_operand(bb, operands[1]);
                auto result = allocate_register();
                // 将结果存储到当前基本块中
                set_operand_for_block(bb, inst.get(), result);

                switch (inst->get_ins_type()) {
                    case ir::Instruction::InstructionType::SHL:
                        bb->add_inst(RVSllw::create(result, operand1, operand2));
                        break;
                    case ir::Instruction::InstructionType::LSHR:
                        bb->add_inst(RVSrl::create(result, operand1, operand2));
                        break;
                    case ir::Instruction::InstructionType::ASHR:
                        bb->add_inst(RVSra::create(result, operand1, operand2));
                        break;
                    default:
                        break;
                }
            }
            break;
        // 浮点运算指令
        case ir::Instruction::InstructionType::FADD:
        case ir::Instruction::InstructionType::FSUB:
        case ir::Instruction::InstructionType::FMUL:
        case ir::Instruction::InstructionType::FDIV:
            translate_fbin(inst, bb);
            break;
        default:
            assert(false && "[RVTranslator] 未实现的二元操作类型: %d\n");
            break;
    }
}

void RVTranslator::translate_add(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    auto &operands = inst->get_operands_ref();
    auto left_val = operands[0];
    auto right_val = operands[1];

    // 如果左操作数是常量，交换左右操作数
    if (std::dynamic_pointer_cast<ir::ConstantInt>(left_val)) {
        std::swap(left_val, right_val);
    }

    auto res_reg = get_res_reg(bb, inst, RVVirReg::RegType::INT_TYPE);
    auto *left = get_operand(bb, left_val);
    auto *right = get_operand_or_imm(bb, right_val, true);

    if (right->get_kind() == RVOperand::Kind::IMM) {
        auto *imm = dynamic_cast<RVImmediate *>(right);
        if (imm->value() == 0) {
            bb->add_inst(RVMv::create(res_reg, left));
        } else {
            bb->add_inst(RVAddiw::create(res_reg, left, right));
        }
    } else {
        bb->add_inst(RVAddw::create(res_reg, left, right));
    }

    set_operand_for_block(bb, inst.get(), res_reg);
}

void RVTranslator::translate_sub(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    auto &operands = inst->get_operands_ref();
    auto left_val = operands[0];
    auto right_val = operands[1];

    auto res_reg = get_res_reg(bb, inst, RVVirReg::RegType::INT_TYPE);
    auto left = get_operand(bb, left_val);

    // 如果右操作数是小常量，将其转换为负数
    if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(right_val)) {
        int val = const_int->get_val();
        if (val < 2048 && val >= -2048) {
            auto neg_val = std::make_shared<ir::ConstantInt>(const_int->get_type(), -val);
            right_val = neg_val;
        }
    }

    auto *right = get_operand_or_imm(bb, right_val, true);

    if (right->get_kind() == RVOperand::Kind::IMM) {
        auto *imm = dynamic_cast<RVImmediate *>(right);
        if (imm->value() == 0) {
            bb->add_inst(RVMv::create(res_reg, left));
        } else {
            bb->add_inst(RVAddiw::create(res_reg, left, right));
        }
    } else {
        bb->add_inst(RVSubw::create(res_reg, left, right));
    }

    set_operand_for_block(bb, inst.get(), res_reg);
}

void RVTranslator::translate_and(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    auto &operands = inst->get_operands_ref();
    auto left_val = operands[0];
    auto right_val = operands[1];

    // 如果左操作数是常量，交换左右操作数
    if (std::dynamic_pointer_cast<ir::ConstantInt>(left_val)) {
        std::swap(left_val, right_val);
    }

    auto *res_reg = get_res_reg(bb, inst, RVVirReg::RegType::INT_TYPE);
    auto *left = get_operand(bb, left_val);
    auto *right = get_operand_or_imm(bb, right_val, true);

    if (right->get_kind() == RVOperand::Kind::IMM) {
        auto *imm = dynamic_cast<RVImmediate *>(right);
        if (imm->value() == 0) {
            bb->add_inst(RVMv::create(res_reg, left));
        } else {
            bb->add_inst(RVAndi::create(res_reg, left, right));
        }
    } else {
        bb->add_inst(RVAnd::create(res_reg, left, right));
    }

    set_operand_for_block(bb, inst.get(), res_reg);
}

void RVTranslator::translate_or(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    auto &operands = inst->get_operands_ref();
    auto left_val = operands[0];
    auto right_val = operands[1];

    // 如果左操作数是常量，交换左右操作数
    if (std::dynamic_pointer_cast<ir::ConstantInt>(left_val)) {
        std::swap(left_val, right_val);
    }

    auto *res_reg = get_res_reg(bb, inst, RVVirReg::RegType::INT_TYPE);
    auto *left = get_operand(bb, left_val);
    auto *right = get_operand_or_imm(bb, right_val, true);

    if (right->get_kind() == RVOperand::Kind::IMM) {
        auto *imm = dynamic_cast<RVImmediate *>(right);
        if (imm->value() == 0) {
            bb->add_inst(RVMv::create(res_reg, left));
        } else {
            bb->add_inst(RVOri::create(res_reg, left, right));
        }
    } else {
        bb->add_inst(RVOr::create(res_reg, left, right));
    }

    set_operand_for_block(bb, inst.get(), res_reg);
}

void RVTranslator::translate_xor(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    auto &operands = inst->get_operands_ref();
    auto left_val = operands[0];
    auto right_val = operands[1];

    // 如果左操作数是常量，交换左右操作数
    if (std::dynamic_pointer_cast<ir::ConstantInt>(left_val)) {
        std::swap(left_val, right_val);
    }

    auto res_reg = get_res_reg(bb, inst, RVVirReg::RegType::INT_TYPE);
    auto left = get_operand(bb, left_val);
    auto right = get_operand_or_imm(bb, right_val, true);

    if (right->get_kind() == RVOperand::Kind::IMM) {
        auto imm = dynamic_cast<RVImmediate *>(right);
        if (imm->value() == 0) {
            bb->add_inst(RVMv::create(res_reg, left));
        } else {
            bb->add_inst(RVXori::create(res_reg, left, right));
        }
    } else {
        bb->add_inst(RVXor::create(res_reg, left, right));
    }

    set_operand_for_block(bb, inst.get(), res_reg);
}

// 辅助函数：检查数字是否可以优化为移位操作
std::vector<int> RVTranslator::can_optimize(long long num) {
    std::vector<int> result;
    long long i = 1;
    while (i < num) {
        i *= 2;
    }
    if (i == num) {
        result.push_back(static_cast<int>(i));
        return result;
    }

    // 检查是否为2的幂次方
    if (__builtin_popcountll(num) == 2) {
        for (long long j = 1; j < i; j *= 2) {
            if (((num - j) & (num - j - 1)) == 0) {
                result.push_back(static_cast<int>(j));
                result.push_back(static_cast<int>(num - j));
                break;
            }
        }
    } else if (__builtin_popcountll(i - num) == 1) {
        result.push_back(static_cast<int>(i));
        result.push_back(static_cast<int>(num - i));
    }
    return result;
}

// 辅助函数：计算移位位数
int RVTranslator::get_shift(int temp) {
    int shift = 0;
    while (temp >= 2) {
        shift++;
        temp /= 2;
    }
    return shift;
}

void RVTranslator::translate_mul(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    auto &operands = inst->get_operands_ref();
    auto left_val = operands[0];
    auto right_val = operands[1];

    // 如果左操作数是常量，交换左右操作数
    if (std::dynamic_pointer_cast<ir::ConstantInt>(left_val)) {
        std::swap(left_val, right_val);
    }

    auto res_reg = get_res_reg(bb, inst, RVVirReg::RegType::INT_TYPE);
    auto left = get_operand(bb, left_val);
    auto right = get_operand_or_imm(bb, right_val, true);

    // 检查是否为64位操作
    if (inst->get_type()->is_integer_ty() &&
        std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type())->bits_num() == 64) {
        assert(false);
        // 64位乘法，直接使用mul指令
        if (right->get_kind() == RVOperand::Kind::IMM) {
            auto imm = dynamic_cast<RVImmediate *>(right);
            auto right_reg = allocate_register();
            bb->add_inst(RVLi::create(right_reg, right));
            bb->add_inst(RVMul::create(res_reg, left, right_reg));
        } else {
            bb->add_inst(RVMul::create(res_reg, left, right));
        }
        set_operand_for_block(bb, inst.get(), res_reg);
        return;
    }

    // 32位乘法的优化
    if (right->get_kind() == RVOperand::Kind::IMM) {
        auto imm = dynamic_cast<RVImmediate *>(right);
        long long value = imm->value();

        if (value == 1) {
            // 乘以1，直接移动
            bb->add_inst(RVMv::create(res_reg, left));
        } else if (value == -1) {
            // 乘以-1，相当于0减去操作数
            bb->add_inst(RVSubw::create(res_reg, reg_info::get_zero(), left));
        } else if (value == 0) {
            // 乘以0，直接加载0
            bb->add_inst(RVLi::create(res_reg, RVImmediate::create(0)));
        } else {
            // 尝试优化为移位操作
            std::vector<int> opt_result = can_optimize(std::abs(value));
            if (!opt_result.empty()) {
                // 如果原数为负数，先取负
                if (value < 0) {
                    auto temp_reg = allocate_register();
                    bb->add_inst(RVSubw::create(temp_reg, reg_info::get_zero(), left));
                    left = temp_reg;
                }

                auto assist_reg = allocate_register();

                if (opt_result.size() == 1) {
                    // 单个移位操作
                    int shift = get_shift(std::abs(opt_result[0]));
                    bb->add_inst(RVSlliw::create(res_reg, left, RVImmediate::create(shift)));
                } else if (opt_result.size() == 2) {
                    // 两个移位操作的组合
                    // assert(opt_result[0] > 0);
                    int shift = get_shift(std::abs(opt_result[0]));

                    if (shift == 0) {
                        bb->add_inst(RVMv::create(res_reg, left));
                    } else {
                        bb->add_inst(RVSlliw::create(res_reg, left, RVImmediate::create(shift)));
                    }

                    bool flag = opt_result[1] > 0;
                    shift = get_shift(std::abs(opt_result[1]));

                    if (flag) {
                        // 加法
                        if (shift == 0) {
                            bb->add_inst(RVAddw::create(res_reg, left, res_reg));
                        } else {
                            bb->add_inst(RVSlliw::create(assist_reg, left, RVImmediate::create(shift)));
                            bb->add_inst(RVAddw::create(res_reg, assist_reg, res_reg));
                        }
                    } else {
                        // 减法
                        if (shift == 0) {
                            bb->add_inst(RVSubw::create(res_reg, res_reg, left));
                        } else {
                            bb->add_inst(RVSlliw::create(assist_reg, left, RVImmediate::create(shift)));
                            bb->add_inst(RVSubw::create(res_reg, res_reg, assist_reg));
                        }
                    }
                }
            } else {
                // 无法优化，需要将立即数加载到寄存器
                auto right_reg = allocate_register();
                bb->add_inst(RVLi::create(right_reg, right));
                bb->add_inst(RVMulw::create(res_reg, left, right_reg));
            }
        }
    } else {
        // 两个寄存器相乘
        bb->add_inst(RVMulw::create(res_reg, left, right));
    }

    set_operand_for_block(bb, inst.get(), res_reg);
}

void RVTranslator::translate_div(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    auto &operands = inst->get_operands_ref();
    auto left_val = operands[0];
    auto right_val = operands[1];

    auto res_reg = get_res_reg(bb, inst, RVVirReg::RegType::INT_TYPE);
    auto left = get_operand(bb, left_val);
    auto right = get_operand_or_imm(bb, right_val, true);

    // 检查右操作数是否为常量整数
    if (right->get_kind() == RVOperand::Kind::IMM) {
        auto imm = dynamic_cast<RVImmediate *>(right);
        int val = static_cast<int>(imm->value());

        if (val == 1) {
            // 除以1，直接移动
            bb->add_inst(RVMv::create(res_reg, left));
            set_operand_for_block(bb, inst.get(), res_reg);
            return;
        } else if (val == -1) {
            // 除以-1，相当于0减去操作数
            bb->add_inst(RVSubw::create(res_reg, reg_info::get_zero(), left));
            set_operand_for_block(bb, inst.get(), res_reg);
            return;
        } else {
            // 检查是否为2的幂次方
            int temp = std::abs(val);
            if ((temp & (temp - 1)) == 0) {
                // 是2的幂次方，使用移位优化
                bool flag = val < 0;
                int shift = 0;
                while (temp >= 2) {
                    shift++;
                    temp /= 2;
                }

                if (flag) {
                    // 如果除数是负数，先取负
                    auto reg1 = allocate_register();
                    bb->add_inst(RVSubw::create(reg1, reg_info::get_zero(), left));
                    left = reg1;
                }

                // 计算 (x + (x >> 31)) >> shift
                auto reg = allocate_register();
                bb->add_inst(RVSraiw::create(reg, left, RVImmediate::create(31)));
                bb->add_inst(RVSrliw::create(reg, reg, RVImmediate::create(32 - shift)));
                bb->add_inst(RVAddw::create(reg, left, reg));
                bb->add_inst(RVSraiw::create(res_reg, reg, RVImmediate::create(shift)));

                set_operand_for_block(bb, inst.get(), res_reg);
                return;
            } else {
                // 除法优化开关（默认启用）
                // 使用乘法逆元优化
                int divNum = val;
                int le = static_cast<int>(std::max(std::log(divNum) / std::log(2), 1.0));
                long long ini = (1LL + ((1LL << (31 + le)) / divNum)) - (1LL << 32);
                int shift = le - 1;

                bb->add_inst(RVLi::create(res_reg, RVImmediate::create(ini)));
                bb->add_inst(RVMul::create(res_reg, res_reg, left));
                bb->add_inst(RVSrli::create(res_reg, res_reg, RVImmediate::create(32)));
                bb->add_inst(RVAdd::create(res_reg, res_reg, left));
                bb->add_inst(RVSraiw::create(res_reg, res_reg, RVImmediate::create(shift)));

                // 处理符号
                auto assist_reg = allocate_register();
                bb->add_inst(RVSlt::create(assist_reg, left, reg_info::get_zero()));
                bb->add_inst(RVAdd::create(res_reg, res_reg, assist_reg));

                if (divNum < 0) {
                    bb->add_inst(RVSubw::create(res_reg, reg_info::get_zero(), res_reg));
                }

                set_operand_for_block(bb, inst.get(), res_reg);
                return;
            }
        }
    }

    // 一般情况：右操作数不是立即数
    bb->add_inst(RVDivw::create(res_reg, left, right));

    set_operand_for_block(bb, inst.get(), res_reg);
}

void RVTranslator::translate_mod(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    auto &operands = inst->get_operands_ref();
    auto left_val = operands[0];
    auto right_val = operands[1];

    auto res_reg = get_res_reg(bb, inst, RVVirReg::RegType::INT_TYPE);
    auto left = get_operand(bb, left_val);

    // 检查是否为64位操作
    if (std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type())->bits_num() == 64) {
        auto right = get_operand(bb, right_val);
        bb->add_inst(RVRem::create(res_reg, left, right));
        set_operand_for_block(bb, inst.get(), res_reg);
        return;
    }

    // 32位操作的处理
    auto right = get_operand_or_imm(bb, right_val, true);

    // 检查右操作数是否为常量整数
    if (right->get_kind() == RVOperand::Kind::IMM) {
        auto imm = dynamic_cast<RVImmediate *>(right);
        int val = imm->value();

        // 特殊值优化
        if (val == 1) {
            bb->add_inst(RVLi::create(res_reg, RVImmediate::create(0)));
            set_operand_for_block(bb, inst.get(), res_reg);
            return;
        }

        // 2的幂次方优化
        int temp = std::abs(val);
        if ((temp & (temp - 1)) == 0) {
            // 计算移位次数
            int shift = 0;
            while (temp >= 2) {
                shift++;
                temp /= 2;
            }

            // 使用位运算优化取模
            auto temp_reg = allocate_register();

            // sraiw temp_reg, left, 31  (算术右移31位，获取符号位)
            bb->add_inst(RVSraiw::create(temp_reg, left, RVImmediate::create(31)));

            // srliw temp_reg, temp_reg, 32-shift  (逻辑右移)
            bb->add_inst(RVSrliw::create(temp_reg, temp_reg, RVImmediate::create(32 - shift)));

            // addw temp_reg, left, temp_reg  (加上符号位)
            bb->add_inst(RVAddw::create(temp_reg, left, temp_reg));

            // srliw temp_reg, temp_reg, shift  (逻辑右移shift位)
            bb->add_inst(RVSrliw::create(temp_reg, temp_reg, RVImmediate::create(shift)));

            // slli temp_reg, temp_reg, shift  (逻辑左移shift位)
            bb->add_inst(RVSlli::create(temp_reg, temp_reg, RVImmediate::create(shift)));

            // subw res_reg, left, temp_reg  (减去结果)
            bb->add_inst(RVSubw::create(res_reg, left, temp_reg));

            set_operand_for_block(bb, inst.get(), res_reg);
            return;
        }

        // 常量但不是2的幂次方，需要加载到寄存器
        auto right_reg = allocate_register();
        bb->add_inst(RVLi::create(right_reg, right));
        bb->add_inst(RVRemw::create(res_reg, left, right_reg));
    } else {
        // 一般情况，使用remw指令
        bb->add_inst(RVRemw::create(res_reg, left, right));
    }

    set_operand_for_block(bb, inst.get(), res_reg);
}

void RVTranslator::translate_fbin(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    auto &operands = inst->get_operands_ref();
    auto left_val = operands[0];
    auto right_val = operands[1];

    auto res_reg = get_res_reg(bb, inst, RVVirReg::RegType::FLOAT_TYPE);
    auto left = get_operand(bb, left_val);
    auto right = get_operand(bb, right_val);

    switch (inst->get_ins_type()) {
        case ir::Instruction::InstructionType::FADD:
            bb->add_inst(RVFadd::create(res_reg, left, right));
            break;
        case ir::Instruction::InstructionType::FSUB:
            bb->add_inst(RVFsub::create(res_reg, left, right));
            break;
        case ir::Instruction::InstructionType::FMUL:
            bb->add_inst(RVFmul::create(res_reg, left, right));
            break;
        case ir::Instruction::InstructionType::FDIV:
            bb->add_inst(RVFdiv::create(res_reg, left, right));
            break;
        default:
            assert(false && "[RVTranslator] 未实现的浮点二元操作类型: %d\n");
            break;
    }

    set_operand_for_block(bb, inst.get(), res_reg);
}

void RVTranslator::translate_phi(std::shared_ptr<ir::Phi> phi, RVBasicBlock::Ptr bb) {
    // logger::INFO(phi->to_string());
    // PHI指令在SSA形式中用于合并来自不同基本块的值
    // 对于PHI指令，需要在全局范围内查找操作数
    if (find_operand_globally(phi.get()) == nullptr) {
        auto result = phi->get_type()->is_float_ty() ? allocate_fregister() : allocate_register();
        set_operand_for_block(bb, phi.get(), result);
    }
}

void RVTranslator::translate_move(std::shared_ptr<ir::Move> move, RVBasicBlock::Ptr bb) {
    auto &operands = move->get_operands_ref();
    auto src_val = operands[0];  // 源操作数
    auto dst_val = operands[1];  // 目标操作数

    // 如果目标操作数不是参数且还没有分配寄存器，则为其分配寄存器
    // 对于Move指令，需要在全局范围内查找操作数
    if (!std::dynamic_pointer_cast<ir::Argument>(dst_val) && find_operand_globally(dst_val.get()) == nullptr) {
        auto dst_reg = dst_val->get_type()->is_float_ty() ? allocate_fregister() : allocate_register();
        set_operand_for_block(bb, dst_val.get(), dst_reg);
    }

    // 获取源操作数和目标操作数的寄存器
    auto *src_reg = get_operand(bb, src_val);
    auto *dst_reg = get_operand(bb, dst_val);

    // 根据类型生成相应的移动指令
    if (dst_val->get_type()->is_float_ty()) {
        // 浮点类型使用RVFmv_s指令
        bb->add_inst(RVFmv_s::create(dst_reg, src_reg));
    } else {
        // 整数类型使用RVMv指令
        bb->add_inst(RVMv::create(dst_reg, src_reg));
    }
}

void RVTranslator::translate_unary_op(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    assert(false);
    // 检查操作数数量
    auto &operands = inst->get_operands_ref();
    auto operand = get_operand(bb, operands[0]);
    auto result = allocate_register();
    set_operand_for_block(bb, inst.get(), result);

    switch (inst->get_ins_type()) {
        case ir::Instruction::InstructionType::FNEG:
            // 浮点取负：使用异或操作
            bb->add_inst(RVXor::create(result, operand, RVImmediate::create(-1)));
            break;
        default:
            assert(false && "[RVTranslator] 未实现的一元操作类型: %d\n");
            break;
    }
}

void RVTranslator::translate_load(std::shared_ptr<ir::Load> load, RVBasicBlock::Ptr bb) {
    auto &operands = load->get_operands_ref();
    auto ptr = operands[0];
    auto content_ty = std::dynamic_pointer_cast<ir::PointerType>(ptr->get_type())->get_reference_type();
    bool is_8byte = content_ty->is_pointer_ty() || (content_ty->is_integer_ty() && content_ty->bits_num() == 64);
    RVVirReg *res_reg = nullptr;

    // 1. GEP
    if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(ptr)) {
        if (ptr2offset.find(ptr.get()) == ptr2offset.end() && find_operand_in_dominators(bb, ptr.get()) == nullptr) {
            // 消phi导致的指令提前
            translate_gep(gep, bb);
        }
        // translate_gep(gep, bb);
        if (ptr2offset.find(ptr.get()) != ptr2offset.end()) {
            assert(false);
            int offset = ptr2offset[ptr.get()];
            if (offset < 2048 && offset >= -2048) {
                if (content_ty->is_float_ty()) {
                    res_reg = get_res_reg(bb, load, RVVirReg::RegType::FLOAT_TYPE);
                    bb->add_inst(RVFlw::create(res_reg, reg_info::get_sp(), RVImmediate::create(offset)));
                } else {
                    res_reg = get_res_reg(bb, load, RVVirReg::RegType::INT_TYPE);
                    if (is_8byte) {
                        bb->add_inst(RVLd::create(res_reg, reg_info::get_sp(), RVImmediate::create(offset)));
                    } else {
                        auto *load_inst = RVLw::create(res_reg, reg_info::get_sp(), RVImmediate::create(offset));
                        // logger::INFO("case 1: " + load_inst->to_string());
                        bb->add_inst(load_inst);
                    }
                }
            } else {
                auto assist_reg = allocate_register();
                bb->add_inst(RVLi::create(assist_reg, RVImmediate::create(offset)));
                bb->add_inst(RVAdd::create(assist_reg, reg_info::get_sp(), assist_reg));
                if (content_ty->is_float_ty()) {
                    res_reg = get_res_reg(bb, load, RVVirReg::RegType::FLOAT_TYPE);
                    bb->add_inst(RVFlw::create(res_reg, assist_reg, RVImmediate::create(0)));
                } else {
                    // 整数 或 指针
                    res_reg = get_res_reg(bb, load, RVVirReg::RegType::INT_TYPE);
                    if (is_8byte) {
                        bb->add_inst(RVLd::create(res_reg, assist_reg, RVImmediate::create(0)));
                    } else {
                        auto *load_inst = RVLw::create(res_reg, assist_reg, RVImmediate::create(0));
                        // logger::INFO("case 2: " + load_inst->to_string());
                        bb->add_inst(load_inst);
                    }
                }
            }
        } else {
            auto base_reg = get_operand(bb, ptr);
            if (content_ty->is_float_ty()) {
                res_reg = get_res_reg(bb, load, RVVirReg::RegType::FLOAT_TYPE);
                bb->add_inst(RVFlw::create(res_reg, base_reg, RVImmediate::create(0)));
            } else {
                res_reg = get_res_reg(bb, load, RVVirReg::RegType::INT_TYPE);
                if (is_8byte) {
                    bb->add_inst(RVLd::create(res_reg, base_reg, RVImmediate::create(0)));
                } else {
                    auto *load_inst = RVLw::create(res_reg, base_reg, RVImmediate::create(0));
                    // logger::INFO("case 3: " + load_inst->to_string());
                    bb->add_inst(load_inst);
                }
            }
        }
    }
    // 2. GlobalVar
    else if (auto gv = std::dynamic_pointer_cast<ir::GlobalVariable>(ptr)) {
        RVVirReg *assist_reg = nullptr;
        // dangerous
        auto operand = find_operand_in_dominators(bb, ptr.get());
        if (auto *reg = dynamic_cast<RVVirReg *>(operand)) {
            assist_reg = reg;
        }

        if (!assist_reg) {
            auto label_name = gv->get_name();
            if (!label_name.empty() && label_name[0] == '@') {
                label_name = label_name.substr(1);
            }
            assist_reg = allocate_register();
            auto *label = RVLabel::create(label_name);
            bb->add_inst(RVLa::create(assist_reg, label));
        }

        if (content_ty->is_float_ty()) {
            res_reg = get_res_reg(bb, load, RVVirReg::RegType::FLOAT_TYPE);
            bb->add_inst(RVFlw::create(res_reg, assist_reg, RVImmediate::create(0)));
        } else {
            res_reg = get_res_reg(bb, load, RVVirReg::RegType::INT_TYPE);
            auto *load_inst = RVLw::create(res_reg, assist_reg, RVImmediate::create(0));
            // logger::INFO("case 4: " + load_inst->to_string());
            bb->add_inst(load_inst);
        }
    }
    // 3. Alloca
    else if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(ptr)) {
        translate_alloca(alloca, bb);
        int offset = -current_function->get_alloca_inst_offset(alloca.get());
        if (offset < 2048 && offset >= -2048) {
            if (content_ty->is_float_ty()) {
                res_reg = get_res_reg(bb, load, RVVirReg::RegType::FLOAT_TYPE);
                bb->add_inst(RVFlw::create(res_reg, reg_info::get_sp(), RVImmediate::create(offset)));
            } else {
                res_reg = get_res_reg(bb, load, RVVirReg::RegType::INT_TYPE);
                if (is_8byte) {
                    bb->add_inst(RVLd::create(res_reg, reg_info::get_sp(), RVImmediate::create(offset)));
                } else {
                    auto *load_inst = RVLw::create(res_reg, reg_info::get_sp(), RVImmediate::create(offset));
                    // logger::INFO("case 5: " + load_inst->to_string());
                    bb->add_inst(load_inst);
                }
            }
        } else {
            // auto assist_reg = allocate_register();
            // bb->add_inst(RVLi::create(assist_reg, RVImmediate::create(offset)));
            // auto assist_reg1 = allocate_register();
            // bb->add_inst(RVAdd::create(assist_reg1, reg_info::get_sp(), assist_reg));

            // dangerous
            auto assist_reg1 = get_operand(bb, ptr);

            if (content_ty->is_float_ty()) {
                res_reg = get_res_reg(bb, load, RVVirReg::RegType::FLOAT_TYPE);
                bb->add_inst(RVFlw::create(res_reg, assist_reg1, RVImmediate::create(0)));
            } else {
                res_reg = get_res_reg(bb, load, RVVirReg::RegType::INT_TYPE);
                if (is_8byte) {
                    bb->add_inst(RVLd::create(res_reg, assist_reg1, RVImmediate::create(0)));
                } else {
                    auto *load_inst = RVLw::create(res_reg, assist_reg1, RVImmediate::create(0));
                    // logger::INFO("case 6: " + load_inst->to_string());
                    bb->add_inst(load_inst);
                }
            }
        }
    }
    // 4. Argument/Phi
    else if (std::dynamic_pointer_cast<ir::Argument>(ptr) || std::dynamic_pointer_cast<ir::Phi>(ptr)) {
        auto assist_reg = get_operand(bb, ptr);
        if (content_ty->is_float_ty()) {
            res_reg = get_res_reg(bb, load, RVVirReg::RegType::FLOAT_TYPE);
            bb->add_inst(RVFlw::create(res_reg, assist_reg, RVImmediate::create(0)));
        } else {
            res_reg = get_res_reg(bb, load, RVVirReg::RegType::INT_TYPE);
            if (is_8byte) {
                bb->add_inst(RVLd::create(res_reg, assist_reg, RVImmediate::create(0)));
            } else {
                auto *load_inst = RVLw::create(res_reg, assist_reg, RVImmediate::create(0));
                // logger::INFO("case 7: " + load_inst->to_string());
                bb->add_inst(load_inst);
            }
        }
    }
    // 5. 其他
    else {
        // logger::INFO(ptr->to_string());
        assert(false && "[RVTranslator] 不支持的Load指针类型");
    }
    set_operand_for_block(bb, load.get(), res_reg);
}

void RVTranslator::translate_store(std::shared_ptr<ir::Store> store, RVBasicBlock::Ptr bb) {
    auto &operands = store->get_operands_ref();
    auto *value_op = get_operand(bb, operands[0]);
    auto ptr = operands[1];
    auto content_ty = std::dynamic_pointer_cast<ir::PointerType>(ptr->get_type())->get_reference_type();
    bool is_8byte = content_ty->is_pointer_ty() || (content_ty->is_integer_ty() && content_ty->bits_num() == 64);
    // 1. GEP
    if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(ptr)) {
        if (ptr2offset.find(ptr.get()) == ptr2offset.end() && find_operand_in_dominators(bb, ptr.get()) == nullptr) {
            // 消phi导致的指令提前
            translate_gep(gep, bb);
        }
        // translate_gep(gep, bb);
        if (ptr2offset.find(ptr.get()) != ptr2offset.end()) {
            assert(false);
            int offset = ptr2offset[ptr.get()];
            if (offset < 2048 && offset >= -2048) {
                if (content_ty->is_float_ty()) {
                    if (is_8byte) {
                        assert(false);
                        bb->add_inst(RVFsd::create(reg_info::get_sp(), value_op, RVImmediate::create(offset)));
                    } else {
                        bb->add_inst(RVFsw::create(reg_info::get_sp(), value_op, RVImmediate::create(offset)));
                    }
                } else {
                    if (is_8byte) {
                        bb->add_inst(RVSd::create(reg_info::get_sp(), value_op, RVImmediate::create(offset)));
                    } else {
                        bb->add_inst(RVSw::create(reg_info::get_sp(), value_op, RVImmediate::create(offset)));
                    }
                }
            } else {
                auto assist_reg = allocate_register();
                bb->add_inst(RVLi::create(assist_reg, RVImmediate::create(offset)));
                bb->add_inst(RVAdd::create(assist_reg, reg_info::get_sp(), assist_reg));
                if (content_ty->is_float_ty()) {
                    if (is_8byte) {
                        assert(false);
                        bb->add_inst(RVFsd::create(assist_reg, value_op, RVImmediate::create(0)));
                    } else {
                        bb->add_inst(RVFsw::create(assist_reg, value_op, RVImmediate::create(0)));
                    }
                } else {
                    if (is_8byte) {
                        bb->add_inst(RVSd::create(assist_reg, value_op, RVImmediate::create(0)));
                    } else {
                        bb->add_inst(RVSw::create(assist_reg, value_op, RVImmediate::create(0)));
                    }
                }
            }
        } else {
            auto base_reg = get_operand(bb, ptr);
            if (content_ty->is_float_ty()) {
                if (is_8byte) {
                    assert(false);
                    bb->add_inst(RVFsd::create(base_reg, value_op, RVImmediate::create(0)));
                } else {
                    bb->add_inst(RVFsw::create(base_reg, value_op, RVImmediate::create(0)));
                }
            } else {
                if (is_8byte) {
                    bb->add_inst(RVSd::create(base_reg, value_op, RVImmediate::create(0)));
                } else {
                    bb->add_inst(RVSw::create(base_reg, value_op, RVImmediate::create(0)));
                }
            }
        }
        return;
    }
    // 2. GlobalVar
    if (auto gv = std::dynamic_pointer_cast<ir::GlobalVariable>(ptr)) {
        RVVirReg *assist_reg = nullptr;
        // dangerous

        auto operand = find_operand_in_dominators(bb, ptr.get());
        if (auto *reg = dynamic_cast<RVVirReg *>(operand)) {
            assist_reg = reg;
        }

        if (!assist_reg) {
            auto label_name = gv->get_name();
            if (!label_name.empty() && label_name[0] == '@') {
                label_name = label_name.substr(1);
            }
            assist_reg = allocate_register();
            auto *label = RVLabel::create(label_name);
            bb->add_inst(RVLa::create(assist_reg, label));
        }

        if (content_ty->is_float_ty()) {
            if (is_8byte) {
                assert(false);
                bb->add_inst(RVFsd::create(assist_reg, value_op, RVImmediate::create(0)));
            } else {
                bb->add_inst(RVFsw::create(assist_reg, value_op, RVImmediate::create(0)));
            }
        } else {
            if (is_8byte) {
                bb->add_inst(RVSd::create(assist_reg, value_op, RVImmediate::create(0)));
            } else {
                bb->add_inst(RVSw::create(assist_reg, value_op, RVImmediate::create(0)));
            }
        }
        return;
    }
    // 3. Alloca
    if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(ptr)) {
        translate_alloca(alloca, bb);
        int offset = -current_function->get_alloca_inst_offset(alloca.get());
        if (offset < 2048 && offset >= -2048) {
            if (content_ty->is_float_ty()) {
                if (is_8byte) {
                    assert(false);
                    bb->add_inst(RVFsd::create(reg_info::get_sp(), value_op, RVImmediate::create(offset)));
                } else {
                    bb->add_inst(RVFsw::create(reg_info::get_sp(), value_op, RVImmediate::create(offset)));
                }
            } else {
                if (is_8byte) {
                    assert(offset % 8 == 0);
                    bb->add_inst(RVSd::create(reg_info::get_sp(), value_op, RVImmediate::create(offset)));
                } else {
                    bb->add_inst(RVSw::create(reg_info::get_sp(), value_op, RVImmediate::create(offset)));
                }
            }
        } else {
            // auto *assist_reg = allocate_register();
            // bb->add_inst(RVLi::create(assist_reg, RVImmediate::create(offset)));
            // auto *assist_reg1 = allocate_register();
            // bb->add_inst(RVAdd::create(assist_reg1, reg_info::get_sp(), assist_reg));

            // dangerous
            auto assist_reg1 = get_operand(bb, ptr);

            if (content_ty->is_float_ty()) {
                if (is_8byte) {
                    assert(false);
                    bb->add_inst(RVFsd::create(assist_reg1, value_op, RVImmediate::create(0)));
                } else {
                    bb->add_inst(RVFsw::create(assist_reg1, value_op, RVImmediate::create(0)));
                }
            } else {
                if (is_8byte) {
                    // assert(offset % 8 == 0);
                    bb->add_inst(RVSd::create(assist_reg1, value_op, RVImmediate::create(0)));
                } else {
                    bb->add_inst(RVSw::create(assist_reg1, value_op, RVImmediate::create(0)));
                }
            }
        }
        return;
    }
    // 4. Argument/Phi
    if (std::dynamic_pointer_cast<ir::Argument>(ptr) || std::dynamic_pointer_cast<ir::Phi>(ptr)) {
        auto *assist_reg = get_operand(bb, ptr);
        if (content_ty->is_float_ty()) {
            if (is_8byte) {
                assert(false);
                bb->add_inst(RVFsd::create(assist_reg, value_op, RVImmediate::create(0)));
            } else {
                bb->add_inst(RVFsw::create(assist_reg, value_op, RVImmediate::create(0)));
            }
        } else {
            if (is_8byte) {
                bb->add_inst(RVSd::create(assist_reg, value_op, RVImmediate::create(0)));
            } else {
                bb->add_inst(RVSw::create(assist_reg, value_op, RVImmediate::create(0)));
            }
        }
        return;
    }
    // logger::INFO("[RVTranslator] 未实现的Store指针类型");
}

void RVTranslator::translate_ret(const std::shared_ptr<ir::Ret> &ret, RVBasicBlock::Ptr bb) {
    if (!ret->get_operands_ref().empty()) {
        auto ret_val = ret->get_operands_ref().at(0);
        // 整数常量
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(ret_val)) {
            bb->add_inst(RVLi::create(reg_info::get_arg_reg(0), RVImmediate::create(const_int->get_val())));
        } else if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(ret_val)) {
            // 浮点常量
            auto *f_const = rv_module->get_or_create_float_const(const_float->get_val());
            auto label = RVLabel::create(f_const->getName());
            auto *assist_reg = allocate_register();
            bb->add_inst(RVLa::create(assist_reg, label));
            bb->add_inst(RVFlw::create(reg_info::get_float_arg_reg(0), assist_reg, RVImmediate::create(0)));
        } else {
            auto return_val = get_operand(bb, ret_val);
            if (ret_val->get_type()->is_integer_ty()) {
                bb->add_inst(RVMv::create(reg_info::get_arg_reg(0), return_val));
            } else if (ret_val->get_type()->is_float_ty()) {
                bb->add_inst(RVFmv_s::create(reg_info::get_float_arg_reg(0), return_val));
            } else {
                assert(false && "[RVTranslator] 未实现的返回类型\n");
            }
        }
    }
    bb->add_inst(RVMv::create(reg_info::get_ra(), current_function->get_return_vir_reg()));
    bb->add_inst(RVJr::create(reg_info::get_ra()));
}

void RVTranslator::translate_br(const std::shared_ptr<ir::Br> &br, const RVBasicBlock::Ptr &bb) {
    auto &operands = br->get_operands_ref();

    if (br->is_cond_branch()) {
        auto cond = get_operand(bb, operands[0]);
        auto true_block = std::dynamic_pointer_cast<ir::BasicBlock>(operands[1]);
        auto false_block = std::dynamic_pointer_cast<ir::BasicBlock>(operands[2]);

        auto true_label = RVLabel::create(get_label_name(true_block));
        auto false_label = RVLabel::create(get_label_name(false_block));

        auto zero = reg_info::get_zero();
        bb->add_inst(RVBne::create(cond, zero, true_label));
        bb->add_inst(RVJ::create(false_label));
    } else {
        // 无条件跳转
        auto target_block = std::dynamic_pointer_cast<ir::BasicBlock>(operands[0]);

        auto target_label = RVLabel::create(get_label_name(target_block));
        bb->add_inst(RVJ::create(target_label));
    }
}

// 采用延迟加载的方式，在需要的时候再分配寄存器
RVOperand *RVTranslator::get_operand(RVBasicBlock::Ptr bb, std::shared_ptr<ir::Value> ir_value) {
    // 检查是否是常量
    if (auto constant = std::dynamic_pointer_cast<ir::ConstantInt>(ir_value)) {
        // dangerous
        auto operand = find_operand_in_dominators(bb, ir_value.get());
        if (operand) {
            return operand;
        }
        auto *reg = allocate_register();
        auto *imm = RVImmediate::create(constant->get_val());
        // 生成 li 指令加载常量，注意这里不能来自零寄存器，即不能简化
        bb->add_inst(RVLi::create(reg, imm));
        // 将常量值存储到当前基本块中
        set_operand_for_block(bb, ir_value.get(), reg);
        return reg;
    }

    if (auto constant = std::dynamic_pointer_cast<ir::ConstantFloat>(ir_value)) {
        // dangerous
        auto operand = find_operand_in_dominators(bb, ir_value.get());
        if (operand) {
            return operand;
        }

        auto *f_const = rv_module->get_or_create_float_const(constant->get_val());
        auto label_name = f_const->getName();

        auto *addr_reg = allocate_register();
        auto *f_dest_reg = allocate_fregister();

        auto label = RVLabel::create(label_name);
        bb->add_inst(RVLa::create(addr_reg, label));
        bb->add_inst(RVFlw::create(f_dest_reg, addr_reg, RVImmediate::create(0)));

        // 将浮点常量存储到当前基本块中
        set_operand_for_block(bb, ir_value.get(), f_dest_reg);
        return f_dest_reg;
    }

    // 如果是全局变量
    if (auto gv = std::dynamic_pointer_cast<ir::GlobalVariable>(ir_value)) {
        // dangerous
        auto operand = find_operand_in_dominators(bb, ir_value.get());
        if (operand) {
            return operand;
        }
        auto addr_reg = allocate_register();
        auto label_name = gv->get_name();
        // 移除LLVM IR中的@符号，转换为有效的RISC-V符号名
        if (!label_name.empty() && label_name[0] == '@') {
            label_name = label_name.substr(1);
        }
        auto label = RVLabel::create(label_name);
        // 使用la指令加载全局变量地址
        bb->add_inst(RVLa::create(addr_reg, label));
        // 将全局变量地址存储到当前基本块中
        set_operand_for_block(bb, ir_value.get(), addr_reg);
        return addr_reg;
    }

    // 如果是参数
    if (auto arg = std::dynamic_pointer_cast<ir::Argument>(ir_value)) {
        // 参数已经在函数开始时处理，这里应该已经有映射
        // 首先尝试在当前基本块和支配块中查找
        // dangerous
        auto operand = find_operand_in_dominators(bb, ir_value.get());
        if (operand) {
            return operand;
        }
        // 获取参数在栈上的偏移
        int offset = 0;
        if (current_function) {
            offset = -current_function->get_arg_offset(arg.get());
        }
        // 判断偏移是否在[-2048, 2048)
        if (offset >= -2048 && offset < 2048) {
            if (arg->get_type()->is_pointer_ty()) {
                auto *reg = allocate_register();
                bb->add_inst(RVLd::create(reg, reg_info::get_sp(), RVImmediate::create(offset)));
                set_operand_for_block(bb, ir_value.get(), reg);
                return reg;
            }
            if (arg->get_type()->is_float_ty()) {
                auto *reg = allocate_fregister();
                bb->add_inst(RVFlw::create(reg, reg_info::get_sp(), RVImmediate::create(offset)));
                set_operand_for_block(bb, ir_value.get(), reg);
                return reg;
            }
            auto *reg = allocate_register();
            bb->add_inst(RVLw::create(reg, reg_info::get_sp(), RVImmediate::create(offset)));
            set_operand_for_block(bb, ir_value.get(), reg);
            return reg;
        }
        if (arg->get_type()->is_pointer_ty()) {
            auto *reg = allocate_register();
            bb->add_inst(RVLi::create(reg, RVImmediate::create(offset)));
            auto *reg1 = allocate_register();
            bb->add_inst(RVAdd::create(reg1, reg_info::get_sp(), reg));
            auto *reg2 = allocate_register();
            bb->add_inst(RVLd::create(reg2, reg1, RVImmediate::create(0)));
            set_operand_for_block(bb, ir_value.get(), reg2);
            return reg2;
        }
        if (arg->get_type()->is_float_ty()) {
            auto *vir_reg = allocate_register();
            bb->add_inst(RVLi::create(vir_reg, RVImmediate::create(offset)));
            auto *vir_reg1 = allocate_register();
            bb->add_inst(RVAdd::create(vir_reg1, reg_info::get_sp(), vir_reg));
            auto *reg = allocate_fregister();
            bb->add_inst(RVFlw::create(reg, vir_reg1, RVImmediate::create(0)));
            set_operand_for_block(bb, ir_value.get(), reg);
            return reg;
        }
        auto *reg = allocate_register();
        bb->add_inst(RVLi::create(reg, RVImmediate::create(offset)));
        auto *reg1 = allocate_register();
        bb->add_inst(RVAdd::create(reg1, reg_info::get_sp(), reg));
        auto *reg2 = allocate_register();
        bb->add_inst(RVLw::create(reg2, reg1, RVImmediate::create(0)));
        set_operand_for_block(bb, ir_value.get(), reg2);
        return reg2;
    }

    // alloca
    if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(ir_value)) {
        // dangerous
        auto operand = find_operand_in_dominators(bb, ir_value.get());
        if (operand) {
            return operand;
        }
        translate_alloca(alloca, bb);
        int offset = -current_function->get_alloca_inst_offset(alloca.get());
        RVVirReg *res_reg = allocate_register();
        if (offset >= -2048 && offset < 2048) {
            if (offset == 0) {
                bb->add_inst(RVMv::create(res_reg, reg_info::get_sp()));
            } else {
                bb->add_inst(RVAddi::create(res_reg, reg_info::get_sp(), RVImmediate::create(offset)));
            }
        } else {
            auto *tmp_reg = allocate_register();
            bb->add_inst(RVLi::create(tmp_reg, RVImmediate::create(offset)));
            bb->add_inst(RVAdd::create(res_reg, reg_info::get_sp(), tmp_reg));
        }
        set_operand_for_block(bb, ir_value.get(), res_reg);
        return res_reg;
    }

    // 如果是指令
    if (auto inst = std::dynamic_pointer_cast<ir::Instruction>(ir_value)) {
        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
            // 对于PHI指令，需要在全局范围内查找操作数
            auto phi_operand = find_operand_globally(phi.get());
            if (phi_operand) {
                return phi_operand;
            }
            auto *reg = phi->get_type()->is_float_ty() ? allocate_fregister() : allocate_register();
            set_operand_for_block(bb, ir_value.get(), reg);
            return reg;
        }

        auto *operand = find_operand_in_dominators(bb, ir_value.get());
        if (operand) {
            return operand;
        }
        translate_instruction(inst, bb);
        operand = find_operand_in_dominators(bb, ir_value.get());
        if (operand) {
            return operand;
        }
        // 兜底：报错
        // logger::ERROR(inst->to_string(), "SSA值未正确传递，指令未分配寄存器！");
        assert(false);
    }

    // logger::INFO("get_operand: " + ir_value->to_string());
    // 占位符
    if (auto *operand = find_operand_in_dominators(bb, ir_value.get())) {
        return operand;
    }

    auto *reg = ir_value->get_type()->is_float_ty() ? allocate_fregister() : allocate_register();
    set_operand_for_block(bb, ir_value.get(), reg);
    return reg;
}

RVOperand *RVTranslator::get_operand_or_imm(RVBasicBlock::Ptr bb,
                                            std::shared_ptr<ir::Value> ir_value,
                                            const bool _2048_flag) {
    // 检查是否是常量整数
    if (auto constant = std::dynamic_pointer_cast<ir::ConstantInt>(ir_value)) {
        int value = constant->get_val();
        if (_2048_flag && (value < -2048 || value >= 2048)) {
            // dangerous
            auto operand = find_operand_in_dominators(bb, ir_value.get());
            if (operand) {
                return operand;
            }

            // 超出立即数范围，需要加载到寄存器
            auto *reg = allocate_register();
            auto *imm = RVImmediate::create(value);
            bb->add_inst(RVLi::create(reg, imm));
            // 将常量值存储到当前基本块中
            set_operand_for_block(bb, ir_value.get(), reg);
            return reg;
        }
        // 在立即数范围内，直接返回立即数
        return RVImmediate::create(value);
    }

    // 其他情况都转换为寄存器
    return get_operand(bb, ir_value);
}

RVVirReg *RVTranslator::allocate_register() { return allocate_virtual_register(RVVirReg::RegType::INT_TYPE); }

RVVirReg *RVTranslator::allocate_fregister() { return allocate_virtual_register(RVVirReg::RegType::FLOAT_TYPE); }

RVVirReg *RVTranslator::allocate_virtual_register(RVVirReg::RegType reg_type) {
    return RVVirReg::create(current_function->get_next_vreg_index(), reg_type, current_function);
}

RVVirReg *RVTranslator::get_res_reg(RVBasicBlock::Ptr bb,
                                    std::shared_ptr<ir::Instruction> inst,
                                    RVVirReg::RegType reg_type) {
    // 在当前基本块和支配块中查找
    RVOperand *operand = nullptr;
    if (dynamic_cast<ir::Phi *>(inst.get()) || dynamic_cast<ir::Move *>(inst.get())) {
        operand = find_operand_globally(inst.get());
    } else {
        operand = find_operand_in_dominators(bb, inst.get());
    }
    if (operand) {
        auto *vir_reg = dynamic_cast<RVVirReg *>(operand);
        // assert(vir_reg && vir_reg->get_reg_type() == reg_type);
        return vir_reg;
    }
    return get_new_vir_reg(reg_type);
}

RVVirReg *RVTranslator::get_new_vir_reg(RVVirReg::RegType reg_type) { return allocate_virtual_register(reg_type); }

std::string RVTranslator::get_label_name(std::shared_ptr<ir::BasicBlock> bb) { return bb_to_label[bb.get()]; }

void RVTranslator::translate_icmp(std::shared_ptr<ir::ICmp> icmp, RVBasicBlock::Ptr bb) {
    auto &operands = icmp->get_operands_ref();
    auto lhs_val = operands[0];
    auto rhs_val = operands[1];

    // judgeCond: 只要有一个是Move就需要特殊处理
    bool has_move = false;
    if (std::dynamic_pointer_cast<ir::Move>(lhs_val) || std::dynamic_pointer_cast<ir::Move>(rhs_val)) {
        has_move = true;
    }

    auto *lhs = get_operand(bb, lhs_val);
    auto *rhs = get_operand(bb, rhs_val);
    auto *result = get_res_reg(bb, icmp, RVVirReg::RegType::INT_TYPE);
    set_operand_for_block(bb, icmp.get(), result);
    auto cmp_type = icmp->get_cmp_type();
    using CmpType = ir::ICmp::ICmpType;

    // 如果有Move，需要特殊处理
    if (has_move) {
        assert(false);
        bool need_result = false;

        // 遍历所有用户，检查是否需要寄存器结果
        for (auto &user : icmp->get_users_ref()) {
            if (auto br_inst = std::dynamic_pointer_cast<ir::Br>(user.lock())) {
                if (br_inst->is_cond_branch()) {
                    // 分支跳转优化：只支持有符号比较类型，无符号比较需要生成寄存器结果
                    if (cmp_type == CmpType::SLT || cmp_type == CmpType::SGE || cmp_type == CmpType::EQ ||
                        cmp_type == CmpType::NE || cmp_type == CmpType::SGT || cmp_type == CmpType::SLE ||
                        cmp_type == CmpType::UGE) {
                        // 获取当前基本块和下一个基本块
                        auto current_block = br_inst->get_parent_block().lock();
                        auto parent_func = current_block->get_parent_func().lock();

                        // 找到下一个基本块
                        std::shared_ptr<ir::BasicBlock> next_block = nullptr;
                        bool found_current = false;
                        for (auto &block : parent_func->get_basic_blocks()) {
                            if (found_current) {
                                next_block = block;
                                break;
                            }
                            if (block == current_block) {
                                found_current = true;
                            }
                        }

                        // 根据Br::create的实现，操作数顺序是：cond(0), true_target(1), false_target(2)
                        auto &operands = br_inst->get_operands_ref();
                        auto true_block = std::dynamic_pointer_cast<ir::BasicBlock>(operands[1]);
                        auto false_block = std::dynamic_pointer_cast<ir::BasicBlock>(operands[2]);

                        // 如果true和false块相同，跳过
                        if (true_block == false_block) {
                            continue;
                        }

                        // 检查是否有一个分支指向下一个基本块
                        bool reverse_flag = false;
                        std::string jump_label;

                        if (true_block == next_block) {
                            reverse_flag = false;
                            jump_label = get_label_name(false_block);
                        } else if (false_block == next_block) {
                            reverse_flag = true;
                            jump_label = get_label_name(true_block);
                        } else {
                            // 需要两个分支
                            need_result = true;
                            continue;
                        }

                        // 生成分支指令
                        auto *jump_target = RVLabel::create(jump_label);
                        RVInstruction *branch_inst = nullptr;

                        switch (cmp_type) {
                            case CmpType::SLT: {
                                if (reverse_flag) {
                                    // flag=true: blt -> bge
                                    branch_inst = RVBge::create(lhs, rhs, jump_target);
                                } else {
                                    // flag=false: blt -> blt
                                    branch_inst = RVBlt::create(lhs, rhs, jump_target);
                                }
                                break;
                            }
                            case CmpType::SGE: {
                                if (reverse_flag) {
                                    // flag=true: bge -> blt
                                    branch_inst = RVBlt::create(lhs, rhs, jump_target);
                                } else {
                                    // flag=false: bge -> bge
                                    branch_inst = RVBge::create(lhs, rhs, jump_target);
                                }
                                break;
                            }
                            case CmpType::EQ: {
                                if (reverse_flag) {
                                    // flag=true: beq -> bne
                                    branch_inst = RVBne::create(lhs, rhs, jump_target);
                                } else {
                                    // flag=false: beq -> beq
                                    branch_inst = RVBeq::create(lhs, rhs, jump_target);
                                }
                                break;
                            }
                            case CmpType::NE: {
                                if (reverse_flag) {
                                    // flag=true: bne -> beq
                                    branch_inst = RVBeq::create(lhs, rhs, jump_target);
                                } else {
                                    // flag=false: bne -> bne
                                    branch_inst = RVBne::create(lhs, rhs, jump_target);
                                }
                                break;
                            }
                            case CmpType::SGT: {
                                if (reverse_flag) {
                                    // flag=true: bgt -> ble (即 blt with swapped operands)
                                    branch_inst = RVBlt::create(rhs, lhs, jump_target);
                                } else {
                                    // flag=false: bgt -> bgt (即 blt with swapped operands)
                                    branch_inst = RVBlt::create(rhs, lhs, jump_target);
                                }
                                break;
                            }
                            case CmpType::SLE: {
                                if (reverse_flag) {
                                    // flag=true: ble -> bgt (即 bge with swapped operands)
                                    branch_inst = RVBge::create(rhs, lhs, jump_target);
                                } else {
                                    // flag=false: ble -> ble (即 bge with swapped operands)
                                    branch_inst = RVBge::create(rhs, lhs, jump_target);
                                }
                                break;
                            }
                            // case CmpType::UGE: {
                            //     if (reverse_flag) {
                            //         // flag=true: bgeu -> bltu
                            //         branch_inst = RVBltu::create(lhs, rhs, jump_target);
                            //     } else {
                            //         // flag=false: bgeu -> bgeu
                            //         branch_inst = RVBgeu::create(lhs, rhs, jump_target);
                            //     }
                            //     break;
                            // }
                            default:
                                need_result = true;
                                break;
                        }

                        if (branch_inst) {
                            bb->add_inst(branch_inst);
                            // 存储分支信息，供后续处理
                            br_to_branch[br_inst.get()] = {branch_inst};
                        }
                    } else {
                        need_result = true;
                    }
                } else {
                    need_result = true;
                }
            } else {
                need_result = true;
            }
        }

        // 如果需要寄存器结果，生成比较指令
        if (need_result) {
            switch (cmp_type) {
                case CmpType::EQ:
                    bb->add_inst(RVSubw::create(result, lhs, rhs));
                    bb->add_inst(RVSltiu::create(result, result, RVImmediate::create(1)));
                    break;
                case CmpType::NE:
                    bb->add_inst(RVSubw::create(result, rhs, lhs));
                    bb->add_inst(RVSltu::create(result, reg_info::get_zero(), result));
                    break;
                case CmpType::SLT:
                    bb->add_inst(RVSlt::create(result, lhs, rhs));
                    break;
                case CmpType::SLE:
                    bb->add_inst(RVSubw::create(result, lhs, rhs));
                    bb->add_inst(RVSlti::create(result, result, RVImmediate::create(1)));
                    break;
                case CmpType::SGT:
                    bb->add_inst(RVSlt::create(result, rhs, lhs));
                    break;
                case CmpType::SGE:
                    bb->add_inst(RVSubw::create(result, rhs, lhs));
                    bb->add_inst(RVSlti::create(result, result, RVImmediate::create(1)));
                    break;
                case CmpType::UGE:
                    bb->add_inst(RVSubw::create(result, rhs, lhs));
                    bb->add_inst(RVSltiu::create(result, result, RVImmediate::create(1)));
                    break;
                default:
                    assert(false && "[RVTranslator] 未实现的ICmp类型");
                    break;
            }
        }
        return;
    }

    // 没有Move，走原有分支优化（可根据需要补充分支跳转优化）
    switch (cmp_type) {
        case CmpType::EQ: {
            bb->add_inst(RVSubw::create(result, lhs, rhs));
            bb->add_inst(RVSltiu::create(result, result, RVImmediate::create(1)));
            break;
        }
        case CmpType::NE: {
            bb->add_inst(RVSubw::create(result, rhs, lhs));
            bb->add_inst(RVSltu::create(result, reg_info::get_zero(), result));
            break;
        }
        case CmpType::SLT:
            bb->add_inst(RVSlt::create(result, lhs, rhs));
            break;
        case CmpType::SLE: {
            bb->add_inst(RVSubw::create(result, lhs, rhs));
            bb->add_inst(RVSlti::create(result, result, RVImmediate::create(1)));
            break;
        }
        case CmpType::SGT:
            bb->add_inst(RVSlt::create(result, rhs, lhs));
            break;
        case CmpType::SGE: {
            bb->add_inst(RVSubw::create(result, rhs, lhs));
            bb->add_inst(RVSlti::create(result, result, RVImmediate::create(1)));
            break;
        }
        case CmpType::UGE: {
            bb->add_inst(RVSubw::create(result, rhs, lhs));
            bb->add_inst(RVSltiu::create(result, result, RVImmediate::create(1)));
            break;
        }
        default:
            assert(false && "[RVTranslator] 未实现的ICmp类型: %d\n");
            break;
    }
}

void RVTranslator::translate_fcmp(std::shared_ptr<ir::FCmp> fcmp, RVBasicBlock::Ptr bb) {
    auto &operands = fcmp->get_operands_ref();
    auto lhs_val = operands[0];
    auto rhs_val = operands[1];

    // judgeCond: 只要有一个是Move就需要特殊处理
    bool has_move = false;
    if (std::dynamic_pointer_cast<ir::Move>(lhs_val) || std::dynamic_pointer_cast<ir::Move>(rhs_val)) {
        has_move = true;
    }

    auto *lhs = get_operand(bb, lhs_val);
    auto *rhs = get_operand(bb, rhs_val);
    auto *result = get_res_reg(bb, fcmp, RVVirReg::RegType::INT_TYPE);
    set_operand_for_block(bb, fcmp.get(), result);
    auto cmp_type = fcmp->get_cmp_type();
    using FCmpType = ir::FCmp::FCmpType;

    // 如果有Move，需要特殊处理
    if (has_move) {
        assert(false);
        bool need_result = false;

        // 遍历所有用户，检查是否需要寄存器结果
        for (auto &user : fcmp->get_users_ref()) {
            if (auto br_inst = std::dynamic_pointer_cast<ir::Br>(user.lock())) {
                if (br_inst->is_cond_branch()) {
                    // 获取当前基本块和下一个基本块
                    auto current_block = br_inst->get_parent_block().lock();
                    auto parent_func = current_block->get_parent_func().lock();

                    // 找到下一个基本块
                    std::shared_ptr<ir::BasicBlock> next_block = nullptr;
                    bool found_current = false;
                    for (auto &block : parent_func->get_basic_blocks()) {
                        if (found_current) {
                            next_block = block;
                            break;
                        }
                        if (block == current_block) {
                            found_current = true;
                        }
                    }

                    // 根据Br::create的实现，操作数顺序是：cond(0), true_target(1), false_target(2)
                    auto &br_operands = br_inst->get_operands_ref();
                    auto true_block = std::dynamic_pointer_cast<ir::BasicBlock>(br_operands[1]);
                    auto false_block = std::dynamic_pointer_cast<ir::BasicBlock>(br_operands[2]);

                    // 如果true和false块相同，跳过
                    if (true_block == false_block) {
                        continue;
                    }

                    // 检查是否有一个分支指向下一个基本块
                    bool reverse_flag = false;
                    std::string jump_label;

                    if (true_block == next_block) {
                        reverse_flag = false;
                        jump_label = get_label_name(false_block);
                    } else if (false_block == next_block) {
                        reverse_flag = true;
                        jump_label = get_label_name(true_block);
                    } else {
                        // 需要两个分支
                        need_result = true;
                        continue;
                    }

                    // 先生成浮点比较指令
                    switch (cmp_type) {
                        case FCmpType::OEQ:
                            bb->add_inst(RVFeq::create(result, lhs, rhs));
                            break;
                        case FCmpType::ONE: {
                            auto tmp = allocate_register();
                            bb->add_inst(RVFeq::create(tmp, lhs, rhs));
                            bb->add_inst(RVXori::create(result, tmp, RVImmediate::create(1)));
                            break;
                        }
                        case FCmpType::OLT:
                            bb->add_inst(RVFlt::create(result, lhs, rhs));
                            break;
                        case FCmpType::OLE:
                            bb->add_inst(RVFle::create(result, lhs, rhs));
                            break;
                        case FCmpType::OGT:
                            bb->add_inst(RVFlt::create(result, rhs, lhs));
                            break;
                        case FCmpType::OGE:
                            bb->add_inst(RVFle::create(result, rhs, lhs));
                            break;
                        default:
                            need_result = true;
                            break;
                    }

                    // 生成分支指令
                    auto *jump_target = RVLabel::create(jump_label);
                    RVInstruction *branch_inst = nullptr;

                    if (!need_result) {
                        switch (cmp_type) {
                            case FCmpType::OEQ: {
                                if (reverse_flag) {
                                    // flag=true: beq -> bne
                                    branch_inst = RVBne::create(result, reg_info::get_zero(), jump_target);
                                } else {
                                    // flag=false: beq -> beq
                                    branch_inst = RVBeq::create(result, reg_info::get_zero(), jump_target);
                                }
                                break;
                            }
                            case FCmpType::ONE: {
                                if (reverse_flag) {
                                    // flag=true: bne -> beq
                                    branch_inst = RVBeq::create(result, reg_info::get_zero(), jump_target);
                                } else {
                                    // flag=false: bne -> bne
                                    branch_inst = RVBne::create(result, reg_info::get_zero(), jump_target);
                                }
                                break;
                            }
                            case FCmpType::OLT: {
                                if (reverse_flag) {
                                    // flag=true: blt -> bge
                                    branch_inst = RVBge::create(result, reg_info::get_zero(), jump_target);
                                } else {
                                    // flag=false: blt -> blt
                                    branch_inst = RVBlt::create(result, reg_info::get_zero(), jump_target);
                                }
                                break;
                            }
                            case FCmpType::OLE: {
                                if (reverse_flag) {
                                    // flag=true: ble -> bgt (即 blt with swapped operands)
                                    branch_inst = RVBlt::create(reg_info::get_zero(), result, jump_target);
                                } else {
                                    // flag=false: ble -> ble (即 bge with swapped operands)
                                    branch_inst = RVBge::create(reg_info::get_zero(), result, jump_target);
                                }
                                break;
                            }
                            case FCmpType::OGT: {
                                if (reverse_flag) {
                                    // flag=true: bgt -> ble (即 bge with swapped operands)
                                    branch_inst = RVBge::create(reg_info::get_zero(), result, jump_target);
                                } else {
                                    // flag=false: bgt -> bgt (即 blt with swapped operands)
                                    branch_inst = RVBlt::create(reg_info::get_zero(), result, jump_target);
                                }
                                break;
                            }
                            case FCmpType::OGE: {
                                if (reverse_flag) {
                                    // flag=true: bge -> blt
                                    branch_inst = RVBlt::create(result, reg_info::get_zero(), jump_target);
                                } else {
                                    // flag=false: bge -> bge
                                    branch_inst = RVBge::create(result, reg_info::get_zero(), jump_target);
                                }
                                break;
                            }
                            default:
                                need_result = true;
                                break;
                        }

                        if (branch_inst) {
                            bb->add_inst(branch_inst);
                            // 存储分支信息，供后续处理
                            br_to_branch[br_inst.get()] = {branch_inst};
                        }
                    }
                } else {
                    need_result = true;
                }
            } else {
                need_result = true;
            }
        }

        // 如果需要寄存器结果，生成比较指令
        if (need_result) {
            switch (cmp_type) {
                case FCmpType::OEQ:
                    bb->add_inst(RVFeq::create(result, lhs, rhs));
                    break;
                case FCmpType::ONE: {
                    auto tmp = allocate_register();
                    bb->add_inst(RVFeq::create(tmp, lhs, rhs));
                    bb->add_inst(RVXori::create(result, tmp, RVImmediate::create(1)));
                    break;
                }
                case FCmpType::OLT:
                    bb->add_inst(RVFlt::create(result, lhs, rhs));
                    break;
                case FCmpType::OLE:
                    bb->add_inst(RVFle::create(result, lhs, rhs));
                    break;
                case FCmpType::OGT:
                    bb->add_inst(RVFlt::create(result, rhs, lhs));
                    break;
                case FCmpType::OGE:
                    bb->add_inst(RVFle::create(result, rhs, lhs));
                    break;
                case FCmpType::UEQ:
                    // UEQ = !(UNE)
                    {
                        auto tmp = allocate_register();
                        bb->add_inst(RVFeq::create(tmp, lhs, rhs));
                        bb->add_inst(RVXori::create(result, tmp, RVImmediate::create(1)));
                    }
                    break;
                case FCmpType::UNE:
                    bb->add_inst(RVFeq::create(result, lhs, rhs));
                    break;
                case FCmpType::ULT:
                    bb->add_inst(RVFlt::create(result, lhs, rhs));
                    break;
                case FCmpType::ULE:
                    bb->add_inst(RVFle::create(result, lhs, rhs));
                    break;
                case FCmpType::UGT:
                    bb->add_inst(RVFlt::create(result, rhs, lhs));
                    break;
                case FCmpType::UGE:
                    bb->add_inst(RVFle::create(result, rhs, lhs));
                    break;
                case FCmpType::ORD:
                    // ORD = !(UNO)
                    {
                        auto tmp = allocate_register();
                        bb->add_inst(RVFeq::create(tmp, lhs, lhs));  // 检查lhs是否为NaN
                        auto tmp2 = allocate_register();
                        bb->add_inst(RVFeq::create(tmp2, rhs, rhs));  // 检查rhs是否为NaN
                        bb->add_inst(RVAnd::create(result, tmp, tmp2));
                    }
                    break;
                case FCmpType::UNO:
                    // UNO = isNaN(lhs) || isNaN(rhs)
                    {
                        auto tmp = allocate_register();
                        bb->add_inst(RVFeq::create(tmp, lhs, lhs));  // 检查lhs是否为NaN
                        auto tmp2 = allocate_register();
                        bb->add_inst(RVFeq::create(tmp2, rhs, rhs));  // 检查rhs是否为NaN
                        bb->add_inst(RVXori::create(tmp, tmp, RVImmediate::create(1)));
                        bb->add_inst(RVXori::create(tmp2, tmp2, RVImmediate::create(1)));
                        bb->add_inst(RVOr::create(result, tmp, tmp2));
                    }
                    break;
                default:
                    assert(false && "[RVTranslator] 未实现的FCmp类型");
                    break;
            }
        }
        return;
    }

    // 没有Move，走原有分支优化（可根据需要补充分支跳转优化）
    switch (cmp_type) {
        case FCmpType::OEQ:
            bb->add_inst(RVFeq::create(result, lhs, rhs));
            break;
        case FCmpType::ONE: {
            auto tmp = allocate_register();
            bb->add_inst(RVFeq::create(tmp, lhs, rhs));
            bb->add_inst(RVXori::create(result, tmp, RVImmediate::create(1)));
            break;
        }
        case FCmpType::OLT:
            bb->add_inst(RVFlt::create(result, lhs, rhs));
            break;
        case FCmpType::OLE:
            bb->add_inst(RVFle::create(result, lhs, rhs));
            break;
        case FCmpType::OGT:
            bb->add_inst(RVFlt::create(result, rhs, lhs));
            break;
        case FCmpType::OGE:
            bb->add_inst(RVFle::create(result, rhs, lhs));
            break;
        case FCmpType::UEQ:
            // UEQ = !(UNE)
            {
                auto tmp = allocate_register();
                bb->add_inst(RVFeq::create(tmp, lhs, rhs));
                bb->add_inst(RVXori::create(result, tmp, RVImmediate::create(1)));
            }
            break;
        case FCmpType::UNE:
            bb->add_inst(RVFeq::create(result, lhs, rhs));
            break;
        case FCmpType::ULT:
            bb->add_inst(RVFlt::create(result, lhs, rhs));
            break;
        case FCmpType::ULE:
            bb->add_inst(RVFle::create(result, lhs, rhs));
            break;
        case FCmpType::UGT:
            bb->add_inst(RVFlt::create(result, rhs, lhs));
            break;
        case FCmpType::UGE:
            bb->add_inst(RVFle::create(result, rhs, lhs));
            break;
        case FCmpType::ORD:
            // ORD = !(UNO)
            {
                auto tmp = allocate_register();
                bb->add_inst(RVFeq::create(tmp, lhs, lhs));  // 检查lhs是否为NaN
                auto tmp2 = allocate_register();
                bb->add_inst(RVFeq::create(tmp2, rhs, rhs));  // 检查rhs是否为NaN
                bb->add_inst(RVAnd::create(result, tmp, tmp2));
            }
            break;
        case FCmpType::UNO:
            // UNO = isNaN(lhs) || isNaN(rhs)
            {
                auto tmp = allocate_register();
                bb->add_inst(RVFeq::create(tmp, lhs, lhs));  // 检查lhs是否为NaN
                auto tmp2 = allocate_register();
                bb->add_inst(RVFeq::create(tmp2, rhs, rhs));  // 检查rhs是否为NaN
                bb->add_inst(RVXori::create(tmp, tmp, RVImmediate::create(1)));
                bb->add_inst(RVXori::create(tmp2, tmp2, RVImmediate::create(1)));
                bb->add_inst(RVOr::create(result, tmp, tmp2));
            }
            break;
        default:
            assert(false && "[RVTranslator] 未实现的FCmp类型: %d\n");
            break;
    }
}

void RVTranslator::translate_call(std::shared_ptr<ir::Call> call, RVBasicBlock::Ptr bb) {
    // logger::INFO(call->to_string());
    // 1. 获取被调用函数
    auto callee = std::dynamic_pointer_cast<ir::Function>(call->get_operands_ref().at(0));
    std::string func_name = callee->get_name();
    if (!func_name.empty() && func_name[0] == '@') {
        func_name = func_name.substr(1);
    }
    auto *call_inst = RVCall::create(reg_info::get_ra(), RVLabel::create(func_name));

    int stack_cur = 0, int_cur = 0, float_cur = 0;
    // 2. 参数传递
    for (size_t i = 1; i < call->get_operands_ref().size(); ++i) {
        auto &arg = call->get_operands_ref()[i];
        if (!arg) continue;
        // 浮点参数
        if (arg->get_type()->is_float_ty()) {
            if (float_cur < 8) {
                if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(arg)) {
                    // 浮点常量，加载到寄存器再传递
                    auto *f_const = rv_module->get_or_create_float_const(const_float->get_val());
                    auto label = RVLabel::create(f_const->getName());
                    auto *reg1 = allocate_register();
                    bb->add_inst(RVLa::create(reg1, label));
                    auto *tmp_reg = allocate_fregister();
                    bb->add_inst(RVFlw::create(tmp_reg, reg1, RVImmediate::create(0)));
                    auto *arg_reg = reg_info::get_float_arg_reg(float_cur);
                    bb->add_inst(RVFmv_s::create(arg_reg, tmp_reg));
                    call_inst->add_call_used_reg(arg_reg);
                } else if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(arg)) {
                    assert(false && "不支持将int常量作为float参数");
                } else {
                    auto *arg_reg = reg_info::get_float_arg_reg(float_cur);
                    auto *reg = get_operand(bb, arg);
                    bb->add_inst(RVFmv_s::create(arg_reg, reg));
                    call_inst->add_call_used_reg(arg_reg);
                }
            } else {
                stack_cur++;
                int offset = stack_cur * 8;
                if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(arg)) {
                    auto *f_const = rv_module->get_or_create_float_const(const_float->get_val());
                    auto label = RVLabel::create(f_const->getName());
                    auto *reg1 = allocate_register();
                    bb->add_inst(RVLa::create(reg1, label));
                    auto *reg2 = allocate_fregister();
                    bb->add_inst(RVFlw::create(reg2, reg1, RVImmediate::create(0)));
                    auto *reg = allocate_register();
                    bb->add_inst(RVLi::create(reg, RVStackFixer::create(current_function, offset)));
                    bb->add_inst(RVSub::create(reg, reg_info::get_sp(), reg));
                    bb->add_inst(RVFsd::create(reg, reg2, RVImmediate::create(0)));
                    // logger::INFO("fsd case 1: ", call->to_string());
                } else if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(arg)) {
                    assert(false && "不支持将int常量作为float参数");
                } else {
                    auto *reg = get_operand(bb, arg);
                    auto *off_reg = allocate_register();
                    bb->add_inst(RVLi::create(off_reg, RVStackFixer::create(current_function, offset)));
                    bb->add_inst(RVSub::create(off_reg, reg_info::get_sp(), off_reg));
                    bb->add_inst(RVFsd::create(off_reg, reg, RVImmediate::create(0)));
                    // logger::INFO("fsd case 2: ", call->to_string());
                }
            }
            float_cur++;
        } else {  // 整数/指针参数
            if (int_cur < 8) {
                auto *tmp_reg = allocate_register();
                auto *reg = reg_info::get_arg_reg(int_cur);
                call_inst->add_call_used_reg(reg);
                if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(arg)) {
                    bb->add_inst(RVLi::create(tmp_reg, RVImmediate::create(const_int->get_val())));
                } else if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(arg)) {
                    assert(false && "不支持将float常量作为int参数");
                } else if (auto gv = std::dynamic_pointer_cast<ir::GlobalVariable>(arg)) {
                    std::string label_name = gv->get_name();
                    if (!label_name.empty() && label_name[0] == '@') label_name = label_name.substr(1);
                    bb->add_inst(RVLa::create(tmp_reg, RVLabel::create(label_name)));
                } else {
                    auto *vir_reg = get_operand(bb, arg);
                    bb->add_inst(RVMv::create(tmp_reg, vir_reg));
                }
                bb->add_inst(RVMv::create(reg, tmp_reg));
            } else {
                stack_cur++;
                int offset = stack_cur * 8;
                auto *vir_reg = get_operand(bb, arg);
                auto *assist_reg = allocate_register();
                bb->add_inst(RVLi::create(assist_reg, RVStackFixer::create(current_function, offset)));
                bb->add_inst(RVSub::create(assist_reg, reg_info::get_sp(), assist_reg));
                bb->add_inst(RVSd::create(assist_reg, vir_reg, RVImmediate::create(0)));
            }
            int_cur++;
        }
    }

    // 3. 调用前栈指针调整，注意库函数是需要隔离的
    // if (is_no_stack_frame_lib_function(func_name)) {
    //     bb->add_inst(call_inst);
    // } else {
    auto *reg_up = reg_info::get_cpu_reg(9);  // x9
    bb->add_inst(RVLi::create(reg_up, RVStackFixer::create(current_function, 0)));
    bb->add_inst(RVSub::create(reg_info::get_sp(), reg_info::get_sp(), reg_up));
    // 4. 插入CALL
    bb->add_inst(call_inst);
    // 5. 恢复栈指针
    bb->add_inst(RVAdd::create(reg_info::get_sp(), reg_info::get_sp(), reg_up));
    // }

    // 6. 处理返回值
    if (callee->get_return_type()->is_float_ty()) {
        // logger::INFO(call->to_string() + "55555");
        auto *res_reg = get_res_reg(bb, call, RVVirReg::RegType::FLOAT_TYPE);
        bb->add_inst(RVFmv_s::create(res_reg, reg_info::get_float_arg_reg(0)));
        set_operand_for_block(bb, call.get(), res_reg);
    } else if (callee->get_return_type()->is_integer_ty()) {
        // logger::INFO(call->to_string() + "6666");
        auto *res_reg = get_res_reg(bb, call, RVVirReg::RegType::INT_TYPE);
        bb->add_inst(RVMv::create(res_reg, reg_info::get_arg_reg(0)));
        set_operand_for_block(bb, call.get(), res_reg);
    }
}

void RVTranslator::translate_alloca(std::shared_ptr<ir::Alloca> alloca, RVBasicBlock::Ptr bb) {
    // 不预先存在寄存器里, 先存个信息, 用到再加载出来
    // 注意alloca不能存进value2operand里！！
    // predefines?
    current_function->alloc_stack(alloca.get());
}

// GEP指令翻译：支持指针+多级偏移寻址
void RVTranslator::translate_gep(std::shared_ptr<ir::Getelementptr> gep, RVBasicBlock::Ptr bb) {
    auto indexes = gep->get_indexes();
    auto base = get_operand(bb, gep->base_ptr());
    auto result = allocate_register();
    set_operand_for_block(bb, gep.get(), result);

    // 获取数组类型信息
    auto type = gep->base_ptr()->get_type();
    if (type->is_pointer_ty()) {
        type = std::dynamic_pointer_cast<ir::PointerType>(type)->get_reference_type();
    }
    // 收集所有维度的信息（从外到内）
    std::vector<int> dim_sizes;
    auto current_type = type;
    while (current_type && current_type->is_array_ty()) {
        auto arr_ty = std::dynamic_pointer_cast<ir::ArrayType>(current_type);
        dim_sizes.push_back(arr_ty->get_size());
        current_type = arr_ty->get_element_type();
    }
    // 元素类型的字节数
    int elem_bytes = current_type->bits_num() / 8;
    // 计算每一维的跨度（以元素为单位）
    std::vector<int> strides(dim_sizes.size() + 1, 1);
    for (int i = dim_sizes.size() - 1; i >= 0; --i) {
        strides[i] = strides[i + 1] * dim_sizes[i];
    }

    // 检查是否所有indexes都是常数
    bool all_constants = true;
    std::vector<int> const_indexes;
    for (int i = 0; i < indexes.size(); ++i) {
        auto idx_const = std::dynamic_pointer_cast<ir::ConstantInt>(indexes[i]);
        if (idx_const) {
            const_indexes.push_back(idx_const->get_val());
        } else {
            all_constants = false;
            break;
        }
    }

    // 如果所有indexes都是常数，直接计算偏移
    if (all_constants) {
        int total_offset = 0;
        for (int i = 0; i < const_indexes.size(); ++i) {
            if (const_indexes[i] != 0) {  // 跳过为0的索引
                total_offset += const_indexes[i] * strides[i];
            }
        }
        total_offset *= elem_bytes;  // 转换为字节偏移

        if (total_offset == 0) {
            // 偏移为0，直接复制基址
            bb->add_inst(RVMv::create(result, base));
        } else if (total_offset >= -2048 && total_offset <= 2047) {
            // 偏移在addi范围内，使用立即数加法
            bb->add_inst(RVAddi::create(result, base, RVImmediate::create(total_offset)));
        } else {
            // 偏移超出addi范围，使用add指令
            auto offset_reg = allocate_register();
            bb->add_inst(RVLi::create(offset_reg, RVImmediate::create(total_offset)));
            bb->add_inst(RVAdd::create(result, base, offset_reg));
        }
        return;
    }

    // 原有的动态计算逻辑
    // 计算偏移
    RVVirReg *offset = nullptr;
    for (int i = 0; i < indexes.size(); ++i) {
        // 如果index为常数0，跳过
        auto idx_const = std::dynamic_pointer_cast<ir::ConstantInt>(indexes[i]);
        if (idx_const && idx_const->get_val() == 0) continue;
        auto idx = get_operand(bb, indexes[i]);
        int stride_val = strides[i];
        auto tmp = allocate_register();
        if (stride_val != 1) {
            // 如果偏移是2的幂，使用左移指令
            if (stride_val > 0 && (stride_val & (stride_val - 1)) == 0) {
                int shift = 0;
                int temp = stride_val;
                while (temp > 1) {
                    temp >>= 1;
                    shift++;
                }
                bb->add_inst(RVSlliw::create(tmp, idx, RVImmediate::create(shift)));
            } else {
                auto stride_reg = allocate_register();
                bb->add_inst(RVLi::create(stride_reg, RVImmediate::create(stride_val)));
                bb->add_inst(RVMulw::create(tmp, idx, stride_reg));
            }
        } else {
            bb->add_inst(RVMv::create(tmp, idx));
        }
        if (offset) {
            auto new_offset = allocate_register();
            bb->add_inst(RVAddw::create(new_offset, offset, tmp));
            offset = new_offset;
        } else {
            offset = tmp;
        }
    }
    if (offset && elem_bytes != 1) {
        int shift = 0;
        int t = elem_bytes;
        while ((t & 1) == 0) {
            ++shift;
            t >>= 1;
        }
        if ((1 << shift) == elem_bytes) {
            auto new_offset = allocate_register();
            bb->add_inst(RVSlliw::create(new_offset, offset, RVImmediate::create(shift)));
            offset = new_offset;
        } else {
            auto elem_reg = allocate_register();
            auto new_offset = allocate_register();
            bb->add_inst(RVLi::create(elem_reg, RVImmediate::create(elem_bytes)));
            bb->add_inst(RVMulw::create(new_offset, offset, elem_reg));
            offset = new_offset;
        }
    }
    if (offset) {
        bb->add_inst(RVAdd::create(result, base, offset));
    } else {
        bb->add_inst(RVMv::create(result, base));
    }
}

void RVTranslator::translate_conversion(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb) {
    // 检查操作数数量
    auto &operands = inst->get_operands_ref();
    using InstrType = ir::Instruction::InstructionType;
    auto src = get_operand(bb, operands[0]);
    // 只分配一次虚拟寄存器并记录
    RVOperand *result;
    switch (inst->get_ins_type()) {
        case InstrType::TRUNC:
        case InstrType::ZEXT:
        case InstrType::BITCAST:
            result = allocate_register();
            set_operand_for_block(bb, inst.get(), result);
            bb->add_inst(RVMv::create(result, src));
            break;
        case InstrType::FPTOSI:
            result = allocate_register();
            set_operand_for_block(bb, inst.get(), result);
            bb->add_inst(RVFcvt_w_s::create(result, src));
            break;
        case InstrType::SITOFP:
            result = allocate_fregister();
            set_operand_for_block(bb, inst.get(), result);
            bb->add_inst(RVFcvt_s_w::create(result, src));
            break;
        default:
            assert(false && "[RVTranslator] 未实现的类型转换指令: %d\n");
            break;
    }
}

void RVTranslator::translate_memset(std::shared_ptr<ir::Memset> memset, RVBasicBlock::Ptr bb) {
    // 获取参数
    auto &operands = memset->get_operands_ref();
    // base地址
    auto base = get_operand(bb, operands[0]);
    // val和size直接从memset对象获取
    int val = memset->get_val();
    int size = memset->get_size();

    // a0: 起始地址
    bb->add_inst(RVMv::create(reg_info::get_arg_reg(0), base));
    // a1: 值
    // auto *tmp_reg1 = allocate_register();
    // if (val == 0) {
    //     bb->add_inst(RVMv::create(tmp_reg1, reg_info::get_zero()));
    // } else {
    //     bb->add_inst(RVLi::create(tmp_reg1, RVImmediate::create(val)));
    //     bb->add_inst(RVMv::create(reg_info::get_arg_reg(1), tmp_reg1));
    // }
    if (val == 0) {
        bb->add_inst(RVMv::create(reg_info::get_arg_reg(1), reg_info::get_zero()));
    } else {
        bb->add_inst(RVLi::create(reg_info::get_arg_reg(1), RVImmediate::create(val)));
    }

    // a2: 长度
    // auto *tmp_reg2 = allocate_register();
    // bb->add_inst(RVLi::create(tmp_reg2, RVImmediate::create(size)));
    // bb->add_inst(RVMv::create(reg_info::get_arg_reg(2), tmp_reg2));
    bb->add_inst(RVLi::create(reg_info::get_arg_reg(2), RVImmediate::create(size)));

    // 使用RVStackFixer动态适配当前函数的栈帧
    auto *reg_up = reg_info::get_cpu_reg(9);  // x9
    bb->add_inst(RVLi::create(reg_up, RVStackFixer::create(current_function, 0)));
    bb->add_inst(RVSub::create(reg_info::get_sp(), reg_info::get_sp(), reg_up));
    // 调用memset
    auto *call_inst = RVCall::create(reg_info::get_ra(), RVLabel::create("memset"));
    bb->add_inst(call_inst);
    // 恢复sp
    bb->add_inst(RVAdd::create(reg_info::get_sp(), reg_info::get_sp(), reg_up));
}

void RVTranslator::translate_lib_functions() {
    using ir::Function;
    const auto &lib_funcs = Function::get_lib_funcs();
    for (const auto &func : lib_funcs) {
        if (!func) continue;
        // logger::INFO(func->get_name());
        current_function = rv_module->new_function(func->get_name());
        current_function->set_return_vir_reg(allocate_register());
        // 库函数没有基本块，使用空的映射
        std::unordered_map<ir::Value *, RVOperand *> empty_map;
        current_function->translate_args(func->get_arguments_ref(), empty_map, nullptr);
    }
}

bool RVTranslator::is_no_stack_frame_lib_function(const std::string &func_name) {
    // 定义无需分配栈帧的库函数列表
    static const std::unordered_set<std::string> no_stack_frame_lib_functions = {"getint",
                                                                                 "getch",
                                                                                 "getfloat",
                                                                                 "getarray",
                                                                                 "getfarray",
                                                                                 "putint",
                                                                                 "putch",
                                                                                 "putfloat",
                                                                                 "putarray",
                                                                                 "putfarray",
                                                                                 "starttime",
                                                                                 "stoptime"};

    // 检查函数名是否在无需栈帧的库函数列表中
    return no_stack_frame_lib_functions.find(func_name) != no_stack_frame_lib_functions.end();
}

// 新增：按支配关系查找操作数的方法
RVOperand *RVTranslator::find_operand_in_dominators(RVBasicBlock *current_block, ir::Value *ir_value) {
    // 倒序遍历，最后一个支配块是自己
    auto ir_block_it = rvbb_to_irbb.find(current_block);
    if (ir_block_it != rvbb_to_irbb.end()) {
        ir::BasicBlock *ir_block = ir_block_it->second;
        // logger::INFO("node: ", ir_block->get_name());

        // 在支配当前块的所有基本块中查找，倒序遍历支配块
        const auto &dominators = ir_block->opt_info.dominator;
        for (auto it = dominators.rbegin(); it != dominators.rend(); ++it) {
            if (auto dominator = it->lock()) {
                // logger::INFO("dominator: ", dominator->get_name());
                // 找到支配块对应的后端基本块
                auto dominator_rv_it = bb_to_rvbb.find(dominator.get());
                if (dominator_rv_it != bb_to_rvbb.end()) {
                    auto *operand = get_operand_for_block(dominator_rv_it->second, ir_value);
                    if (operand) {
                        return operand;
                    }
                }
            }
        }
    }

    return nullptr;
}

// 新增：全局查找操作数的方法，用于PHI和Move指令
RVOperand *RVTranslator::find_operand_globally(ir::Value *ir_value) {
    // 遍历所有后端基本块，查找操作数
    for (const auto &[rv_block, operand_map] : value_to_operand_by_block) {
        auto operand_it = operand_map.find(ir_value);
        if (operand_it != operand_map.end()) {
            return operand_it->second;
        }
    }

    return nullptr;
}

void RVTranslator::set_operand_for_block(RVBasicBlock *block, ir::Value *ir_value, RVOperand *operand) {
    value_to_operand_by_block[block][ir_value] = operand;
}

RVOperand *RVTranslator::get_operand_for_block(RVBasicBlock *block, ir::Value *ir_value) {
    auto it = value_to_operand_by_block.find(block);
    if (it != value_to_operand_by_block.end()) {
        auto operand_it = it->second.find(ir_value);
        if (operand_it != it->second.end()) {
            return operand_it->second;
        }
    }
    return nullptr;
}

}  // namespace backend::riscv
