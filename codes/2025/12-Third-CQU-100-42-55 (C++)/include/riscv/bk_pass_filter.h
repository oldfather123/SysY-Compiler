#pragma once

#include "code_gen.h"

namespace riscv {

/**
 * @brief RISC-V 冗余指令过滤优化pass
 * 
 * 该pass用于过滤和移除RISC-V汇编代码中的冗余操作，包括：
 * 1. 自身移动指令 (如 mv s1, s1)
 * 2. 自身加0操作 (如 addi s1, s1, 0)
 * 3. 冗余跳转指令 (如 jal zero, label 当下一个块就是该label时)
 * 4. 其他等价的冗余操作
 */
class BackendFilterPass {
public:
    /**
     * @brief 对RISC-V模块应用过滤优化
     * @param module 待优化的RISC-V模块
     */
    static void apply_filter_pass(riscvModule* module);

    /**
     * @brief 对RISC-V函数应用过滤优化
     * @param function 待优化的RISC-V函数
     */
    static void apply_filter_pass(riscvFunction* function);

    /**
     * @brief 对RISC-V基本块应用过滤优化
     * @param block 待优化的RISC-V基本块
     * @param next_block_name 下一个基本块的名称（用于检查冗余跳转）
     */
    static void apply_filter_pass(riscvBlock* block, const std::string& next_block_name = "");

private:
    /**
     * @brief 检查指令是否为冗余的自身移动
     * @param instr 待检查的指令
     * @return 如果是冗余指令返回true
     */
    static bool is_redundant_move(const riscvInstr* instr);

    /**
     * @brief 检查指令是否为冗余的加0操作
     * @param instr 待检查的指令
     * @return 如果是冗余指令返回true
     */
    static bool is_redundant_add_zero(const riscvInstr* instr);

    /**
     * @brief 检查指令是否为冗余的跳转
     * @param instr 待检查的指令
     * @param next_block_name 下一个基本块的名称
     * @return 如果是冗余指令返回true
     */
    static bool is_redundant_jump(const riscvInstr* instr, const std::string& next_block_name);

    /**
     * @brief 检查指令是否为其他类型的冗余操作
     * @param instr 待检查的指令
     * @return 如果是冗余指令返回true
     */
    static bool is_other_redundant_operation(const riscvInstr* instr);

    /**
     * @brief 将合适的li指令转换为addi指令
     * @param instr 待转换的指令
     * @return 如果进行了转换返回true
     */
    static bool optimize_li_to_addi(riscvInstr* instr);

    /**
     * @brief 检查立即数是否在12位有符号数范围内
     * @param imm 立即数
     * @return 如果在范围内返回true
     */
    static bool is_12bit_signed_imm(int imm);

    /**
     * @brief 获取基本块列表（包含entry block）
     * @param function RISC-V函数
     * @return 基本块指针列表
     */
    static std::vector<riscvBlock*> get_all_blocks(riscvFunction* function);
};

} // namespace riscv
