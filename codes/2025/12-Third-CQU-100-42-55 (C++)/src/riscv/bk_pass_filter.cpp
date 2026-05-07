#include "../../include/riscv/bk_pass_filter.h"
#include <iostream>
#include <algorithm>

namespace riscv {

void BackendFilterPass::apply_filter_pass(riscvModule* module) {
    if (!module) return;
    
    // 对模块中的每个函数应用过滤优化
    for (auto& func : module->func_list) {
        apply_filter_pass(func.get());
    }
}

void BackendFilterPass::apply_filter_pass(riscvFunction* function) {
    if (!function) return;
    
    // 获取所有基本块
    std::vector<riscvBlock*> all_blocks = get_all_blocks(function);
    
    // 对每个基本块应用过滤优化
    for (size_t i = 0; i < all_blocks.size(); ++i) {
        std::string next_block_name = "";
        
        // 如果有下一个基本块，获取其名称
        if (i + 1 < all_blocks.size()) {
            next_block_name = all_blocks[i + 1]->name;
        }
        
        apply_filter_pass(all_blocks[i], next_block_name);
    }
}

void BackendFilterPass::apply_filter_pass(riscvBlock* block, const std::string& next_block_name) {
    if (!block) return;
    
    std::vector<std::unique_ptr<riscvInstr>> filtered_instructions;
    
    for (auto& instr : block->inst_list) {
        bool should_remove = false;
        
        // 首先尝试li到addi的优化（在最后一步进行）
        optimize_li_to_addi(instr.get());
        
        // 检查各种冗余操作
        if (is_redundant_move(instr.get())) {
            should_remove = true;
        } else if (is_redundant_add_zero(instr.get())) {
            should_remove = true;
        } else if (is_redundant_jump(instr.get(), next_block_name)) {
            should_remove = true;
        } else if (is_other_redundant_operation(instr.get())) {
            should_remove = true;
        }
        
        // 如果不是冗余操作，保留该指令
        if (!should_remove) {
            filtered_instructions.push_back(std::move(instr));
        }
    }
    
    // 更新基本块的指令列表
    block->inst_list = std::move(filtered_instructions);
}

bool BackendFilterPass::is_redundant_move(const riscvInstr* instr) {
    if (!instr) return false;
    
    // 检查mv指令：mv rd, rs1
    if (instr->op == opcode::mv) {
        return instr->rd == instr->rs1;
    }
    
    // 检查浮点move指令：fmv.s rd, rs1 和 fmv.d rd, rs1
    if (instr->op == opcode::fmv_s || instr->op == opcode::fmv_d) {
        return instr->rd == instr->rs1;
    }
    
    return false;
}

bool BackendFilterPass::is_redundant_add_zero(const riscvInstr* instr) {
    if (!instr) return false;
    
    // 检查立即数加法指令
    if (instr->op == opcode::addi || instr->op == opcode::addiw) {
        // addi rd, rs1, 0 且 rd == rs1
        return instr->imm == 0 && instr->rd == instr->rs1;
    }
    
    // 检查浮点加法指令（加0.0）
    if (instr->op == opcode::fadd_s || instr->op == opcode::fadd_d) {
        // 这种情况需要检查rs2是否为浮点零寄存器
        // 由于浮点零不是直接的寄存器，这里暂时不处理
        // 可以扩展为检查rs2是否为存储0.0的寄存器
    }
    
    return false;
}

bool BackendFilterPass::is_redundant_jump(const riscvInstr* instr, const std::string& next_block_name) {
    if (!instr || next_block_name.empty()) return false;
    
    // 检查无条件跳转：jal zero, label
    if (instr->op == opcode::jal && instr->rd == zero) {
        // 如果跳转的目标就是下一个基本块，则该跳转是冗余的
        return instr->label == next_block_name;
    }
    
    // 检查伪指令jump：j label
    if (instr->op == opcode::j) {
        return instr->label == next_block_name;
    }
    
    return false;
}

bool BackendFilterPass::is_other_redundant_operation(const riscvInstr* instr) {
    if (!instr) return false;
    
    // 检查其他冗余操作
    
    // 1. 逻辑运算的恒等操作
    if (instr->op == opcode::or_ || instr->op == opcode::xor_) {
        // or rd, rs1, zero 或 xor rd, rs1, zero 等价于 mv rd, rs1
        if (instr->rs2 == zero && instr->rd == instr->rs1) {
            return true;
        }
    }
    
    // 2. 位运算的恒等操作
    if (instr->op == opcode::ori || instr->op == opcode::xori) {
        // ori rd, rs1, 0 或 xori rd, rs1, 0 等价于 mv rd, rs1
        if (instr->imm == 0 && instr->rd == instr->rs1) {
            return true;
        }
    }
    
    // 3. 左移/右移0位
    if (instr->op == opcode::slli || instr->op == opcode::srli || instr->op == opcode::srai ||
        instr->op == opcode::slliw || instr->op == opcode::srliw || instr->op == opcode::sraiw) {
        if (instr->imm == 0 && instr->rd == instr->rs1) {
            return true;
        }
    }
    
    // 4. 乘以1的操作（需要检查立即数或寄存器值）
    // 这里只能检查明显的立即数情况
    
    // 5. 与自身的AND操作
    if (instr->op == opcode::and_) {
        if (instr->rs1 == instr->rs2 && instr->rd == instr->rs1) {
            return true;
        }
    }
    
    // 6. 与全1的AND操作或与0的OR操作（立即数版本）
    if (instr->op == opcode::andi) {
        // andi rd, rs1, -1 (全1) 等价于 mv rd, rs1
        if (instr->imm == -1 && instr->rd == instr->rs1) {
            return true;
        }
    }
    
    return false;
}

std::vector<riscvBlock*> BackendFilterPass::get_all_blocks(riscvFunction* function) {
    std::vector<riscvBlock*> blocks;
    
    if (!function) return blocks;
    
    // 添加entry block
    if (function->entry_block) {
        blocks.push_back(function->entry_block.get());
    }
    
    // 添加其他基本块
    for (auto& block : function->basic_blocks) {
        blocks.push_back(block.get());
    }
    
    return blocks;
}

bool BackendFilterPass::optimize_li_to_addi(riscvInstr* instr) {
    if (!instr) return false;
    
    // 检查是否为li指令
    if (instr->op != opcode::li) {
        return false;
    }
    
    // 检查立即数是否在12位有符号数范围内
    if (!is_12bit_signed_imm(instr->imm)) {
        return false;
    }
    
    // 将li指令转换为addi指令
    // li rd, imm -> addi rd, zero, imm
    instr->op = opcode::addi;
    instr->rs1 = zero;  // rs1设置为zero寄存器
    // rd和imm保持不变
    
    return true;
}

bool BackendFilterPass::is_12bit_signed_imm(int imm) {
    // 12位有符号数的范围是 -2048 到 2047
    return (imm >= -2048) && (imm <= 2047);
}

} // namespace riscv
