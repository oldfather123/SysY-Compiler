#include "../../include/riscv/code_gen.h"
#include "../../debug.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <tuple>
#include <unordered_map>
#include <algorithm>

#include "../../include/riscv/function_reg_alloc.h"
#include "../../include/riscv/linear_scan_alloc.h"
#include <assert.h>

namespace riscv {
// 全局变量用于指令生成过程中的函数上下文
static riscvFunction* current_riscv_function = nullptr;
// 全局变量用于存储LLVM模块，以便查找函数声明
static llvm_ir::Module* current_llvm_module = nullptr;

// 根据函数名查找函数声明
llvm_ir::Function* find_function_declaration(const std::string& function_name) {
    if (!current_llvm_module) {
        return nullptr;
    }
    
    for (const auto& func : current_llvm_module->functions) {
        if (func->name == function_name) {
            return func.get();
        }
    }
    return nullptr;
}

// 将浮点数转换为IEEE754格式的整数表示
inline int32_t float_to_ieee754_int(float value) {
    return *(reinterpret_cast<const int32_t*>(&value));
}

// 将双精度浮点数转换为IEEE754格式的64位整数表示
inline int64_t double_to_ieee754_long(double value) {
    return *(reinterpret_cast<const int64_t*>(&value));
}

// 判断寄存器字符串是否是浮点寄存器
bool is_float_reg_string(const std::string& reg_str) {
    return reg_str.size() > 0 && (reg_str[0] == 'f' &&  reg_str!="fp");
}

// 生成处理大立即数的指令序列
std::string generate_large_immediate_addi(const std::string& rd_str, const std::string& rs_str, int imm) {
    std::string result;

    // RISC-V立即数范围：-2048 到 +2047
    if (imm >= -2048 && imm <= 2047) {
        // 在范围内，直接使用addi
        result += "\taddi\t" + rd_str + ", " + rs_str + ", " + std::to_string(imm) + "\n";
    } else {
        // 超出范围，需要拆分
        // 使用li指令来处理大立即数

        // 使用t0 (x5) 作为临时寄存器
        result += "\tli\tx5, " + std::to_string(imm) + "\n";
        result += "\tadd\t" + rd_str + ", " + rs_str + ", x5\n";
    }

    return result;
}

// 生成处理大偏移的存储指令
std::string generate_large_offset_store(const std::string& src_reg, const std::string& base_reg, int offset) {
    std::string result;

    // RISC-V存储指令偏移范围：-2048 到 +2047
    if (offset >= -2048 && offset <= 2047) {
        // 在范围内，直接使用sd
        result += "\tsd\t" + src_reg + ", " + std::to_string(offset) + "(" + base_reg + ")\n";
    } else {
        // 超出范围，使用临时寄存器计算地址
        // 使用t1 (x6) 作为临时寄存器
        result += "\tli\tx6, " + std::to_string(offset) + "\n";
        result += "\tadd\tx6, " + base_reg + ", x6\n";
        result += "\tsd\t" + src_reg + ", 0(x6)\n";
    }

    return result;
}

// 生成处理大偏移的浮点存储指令
std::string generate_large_offset_fstore(const std::string& src_reg, const std::string& base_reg, int offset) {
    std::string result;

    // RISC-V存储指令偏移范围：-2048 到 +2047
    if (offset >= -2048 && offset <= 2047) {
        // 在范围内，直接使用fsd
        result += "\tfsd\t" + src_reg + ", " + std::to_string(offset) + "(" + base_reg + ")\n";
    } else {
        // 超出范围，使用临时寄存器计算地址
        // 使用t1 (x6) 作为临时寄存器
        result += "\tli\tx6, " + std::to_string(offset) + "\n";
        result += "\tadd\tx6, " + base_reg + ", x6\n";
        result += "\tfsd\t" + src_reg + ", 0(x6)\n";
    }

    return result;
}

// 生成处理大偏移的加载指令
std::string generate_large_offset_load(const std::string& dst_reg, const std::string& base_reg, int offset) {
    std::string result;

    // RISC-V加载指令偏移范围：-2048 到 +2047
    if (offset >= -2048 && offset <= 2047) {
        // 在范围内，直接使用ld
        result += "\tld\t" + dst_reg + ", " + std::to_string(offset) + "(" + base_reg + ")\n";
    } else {
        // 超出范围，使用临时寄存器计算地址
        // 使用t1 (x6) 作为临时寄存器
        result += "\tli\tx6, " + std::to_string(offset) + "\n";
        result += "\tadd\tx6, " + base_reg + ", x6\n";
        result += "\tld\t" + dst_reg + ", 0(x6)\n";
    }

    return result;
}

// 生成处理大偏移的浮点加载指令
std::string generate_large_offset_fload(const std::string& dst_reg, const std::string& base_reg, int offset) {
    std::string result;

    // RISC-V加载指令偏移范围：-2048 到 +2047
    if (offset >= -2048 && offset <= 2047) {
        // 在范围内，直接使用fld
        result += "\tfld\t" + dst_reg + ", " + std::to_string(offset) + "(" + base_reg + ")\n";
    } else {
        // 超出范围，使用临时寄存器计算地址
        // 使用t1 (x6) 作为临时寄存器
        result += "\tli\tx6, " + std::to_string(offset) + "\n";
        result += "\tadd\tx6, " + base_reg + ", x6\n";
        result += "\tfld\t" + dst_reg + ", 0(x6)\n";
    }

    return result;
}

std::string riscvInstr::toString() {
    std::string result = "\t";

    switch (op) {
    // 无操作数指令
    case opcode::nop:
    case opcode::ret:
    case opcode::ecall:
    case opcode::ebreak:
    case opcode::fence:
    case opcode::fence_i: result += to_string(op); break;

    // 立即数指令 (rd, imm) 或 (rd, label)
    case opcode::li:
    case opcode::lui:
    case opcode::auipc:
        if (!label.empty()) {
            result += to_string(op) + "\t" + to_string(rd) + ", " + label;
        } else {
            result += to_string(op) + "\t" + to_string(rd) + ", " + std::to_string(imm);
        }
        break;

    // 跳转指令 (label)
    case opcode::j:
    case opcode::call:
    case opcode::tail: result += to_string(op) + "\t" + label; break;

    // 地址加载指令 (rd, label)
    case opcode::la: result += to_string(op) + "\t" + to_string(rd) + ", " + label; break;

    // 寄存器到寄存器指令 (rd, rs1)
    case opcode::mv:
    case opcode::not_:
    case opcode::neg:
    case opcode::seqz:
    case opcode::snez:
    case opcode::sltz:
    case opcode::sgtz:
    case opcode::fmv_s:
    case opcode::fmv_d:
    case opcode::fsqrt_s:
    case opcode::fsqrt_d:
    case opcode::fclass_s:
    case opcode::fclass_d:
    case opcode::fmv_x_s:
    case opcode::fmv_s_x:
    case opcode::fmv_x_d:
    case opcode::fmv_d_x: result += to_string(op) + "\t" + to_string(rd) + ", " + to_string(rs1); break;

    // 立即数算术指令 (rd, rs1, imm) 或 (rd, rs1, label)
    case opcode::addi:
    case opcode::slti:
    case opcode::sltiu:
    case opcode::xori:
    case opcode::ori:
    case opcode::andi:
    case opcode::slli:
    case opcode::srli:
    case opcode::srai:
    case opcode::addiw:
    case opcode::slliw:
    case opcode::srliw:
    case opcode::sraiw:
        if (!label.empty() && op == opcode::addi) {
            result += to_string(op) + "\t" + to_string(rd) + ", " + to_string(rs1) + ", " + label;
        } else {
            result += to_string(op) + "\t" + to_string(rd) + ", " + to_string(rs1) + ", " + std::to_string(imm);
        }
        break;

    // 加载指令 (rd, imm(rs1))
    case opcode::lb:
    case opcode::lh:
    case opcode::lw:
    case opcode::lbu:
    case opcode::lhu:
    case opcode::lwu:
    case opcode::ld:
    case opcode::flw:
    case opcode::fld: result += to_string(op) + "\t" + to_string(rd) + ", " + std::to_string(imm) + "(" + to_string(rs1) + ")"; break;

    // 存储指令 (rs2, imm(rs1))
    case opcode::sb:
    case opcode::sh:
    case opcode::sw:
    case opcode::sd:
    case opcode::fsw:
    case opcode::fsd: result += to_string(op) + "\t" + to_string(rs2) + ", " + std::to_string(imm) + "(" + to_string(rs1) + ")"; break;

    // 分支指令 (rs1, rs2, label)
    case opcode::beq:
    case opcode::bne:
    case opcode::blt:
    case opcode::bge:
    case opcode::bltu:
    case opcode::bgeu: result += to_string(op) + "\t" + to_string(rs1) + ", " + to_string(rs2) + ", " + label; break;

    // 跳转链接指令 (rd, label)
    case opcode::jal: 
    // result += to_string(op) + "\t" + to_string(rd) + ", " + label; break;
    result += "j\t" + label; break;

    // 跳转链接寄存器指令 (rd, rs1, imm)
    case opcode::jalr: result += to_string(op) + "\t" + to_string(rd) + ", " + to_string(rs1) + ", " + std::to_string(imm); break;

    // 三寄存器算术指令 (rd, rs1, rs2)
    case opcode::add:
    case opcode::sub:
    case opcode::sll:
    case opcode::slt:
    case opcode::sltu:
    case opcode::xor_:
    case opcode::srl:
    case opcode::sra:
    case opcode::or_:
    case opcode::and_:
    case opcode::mul:
    case opcode::mulh:
    case opcode::mulhsu:
    case opcode::mulhu:
    case opcode::div:
    case opcode::divu:
    case opcode::rem:
    case opcode::remu:
    case opcode::addw:
    case opcode::subw:
    case opcode::sllw:
    case opcode::srlw:
    case opcode::sraw:
    case opcode::mulw:
    case opcode::divw:
    case opcode::divuw:
    case opcode::remw:
    case opcode::remuw:
    case opcode::fadd_s:
    case opcode::fsub_s:
    case opcode::fmul_s:
    case opcode::fdiv_s:
    case opcode::fsgnj_s:
    case opcode::fsgnjn_s:
    case opcode::fsgnjx_s:
    case opcode::fmin_s:
    case opcode::fmax_s:
    case opcode::fadd_d:
    case opcode::fsub_d:
    case opcode::fmul_d:
    case opcode::fdiv_d:
    case opcode::fsgnj_d:
    case opcode::fsgnjn_d:
    case opcode::fsgnjx_d:
    case opcode::fmin_d:
    case opcode::fmax_d:
    case opcode::feq_s:
    case opcode::flt_s:
    case opcode::fle_s:
    case opcode::feq_d:
    case opcode::flt_d:
    case opcode::fle_d: result += to_string(op) + "\t" + to_string(rd) + ", " + to_string(rs1) + ", " + to_string(rs2); break;

    // 类型转换指令 (rd, rs1, rm) - 需要处理舍入模式
    case opcode::fcvt_w_s:
    case opcode::fcvt_wu_s:
    case opcode::fcvt_w_d:
    case opcode::fcvt_wu_d:
        // 浮点到整数转换，需要舍入模式
        if (imm == 1) {
            result += to_string(op) + "\t" + to_string(rd) + ", " + to_string(rs1) + ", rtz";  // 向零舍入
        } else {
            result += to_string(op) + "\t" + to_string(rd) + ", " + to_string(rs1);  // 默认舍入模式
        }
        break;
    
    // 其他类型转换指令 (rd, rs1)
    case opcode::fcvt_s_w:
    case opcode::fcvt_s_wu:
    case opcode::fcvt_d_w:
    case opcode::fcvt_d_wu:
    case opcode::fcvt_s_d:
    case opcode::fcvt_d_s: result += to_string(op) + "\t" + to_string(rd) + ", " + to_string(rs1); break;

    // CSR指令 (rd, csr, rs1)
    case opcode::csrrw:
    case opcode::csrrs:
    case opcode::csrrc: result += to_string(op) + "\t" + to_string(rd) + ", " + std::to_string(imm) + ", " + to_string(rs1); break;

    // CSR立即数指令 (rd, csr, imm)
    case opcode::csrrwi:
    case opcode::csrrsi:
    case opcode::csrrci: result += to_string(op) + "\t" + to_string(rd) + ", " + std::to_string(imm) + ", " + std::to_string(imm); break;

    default: result += to_string(op) + "\t# Unknown instruction format"; break;
    }

    result += "\n";
    return result;
}

std::string riscvBlock::toString() {
    std::string result;

    // 如果块名不是函数入口块，输出标签
    if (!name.empty() && name != "entry") {
        result += name + ":\n";
    }

    // 输出所有指令
    for (const auto& inst : inst_list) {
        result += inst->toString();
    }

    // 如果块没有终止指令（ret, br等），添加fallthrough注释
    if (!inst_list.empty()) {
        const auto& last_inst = inst_list.back();
        if (last_inst->op != opcode::ret && last_inst->op != opcode::j && last_inst->op != opcode::jal && last_inst->op != opcode::jalr && last_inst->op != opcode::beq && last_inst->op != opcode::bne && last_inst->op != opcode::blt && last_inst->op != opcode::bge && last_inst->op != opcode::bltu && last_inst->op != opcode::bgeu) {
            result += "\t# fallthrough to next block\n";
        }
    }

    return result;
}

std::string riscvFunction::toString() {
    std::string result;

    // 函数声明
    result += "\t.globl\t" + name + "\n";
    result += "\t.align\t" + std::to_string(func_align) + "\n";
    result += "\t.type\t" + name + ", @function\n";

    // 函数标签
    result += name + ":\n";

    // 使用预先计算好的栈大小
    int stack_size = this->total_stack_size;
    
    // 重新分析函数需要保存的寄存器（用于生成prologue/epilogue）
    std::set<reg> used_callee_saved_regs;
    bool needs_ra_save = false;

    // 1. 检查是否需要保存ra（有函数调用）
    // 收集所有基本块进行检查
    std::vector<riscvBlock*> all_blocks;
    
    // 添加entry block
    if (entry_block) {
        all_blocks.push_back(entry_block.get());
    }
    
    // 添加所有basic blocks
    for (const auto& block : basic_blocks) {
        all_blocks.push_back(block.get());
    }
    
    // 检查所有基本块中是否有函数调用
    for (const auto& block : all_blocks) {
        for (const auto& inst : block->inst_list) {
            if (inst->op == opcode::call || inst->op == opcode::jal) {
                needs_ra_save = true;
                break;
            }
        }
        
        if (needs_ra_save) {
            break;
        }
    }

    // 2. 检查使用了哪些callee-saved寄存器（s0-s11）
    // 通过遍历寄存器分配结果来获取
    if (this->name != "main") {
        for (const auto& allocation : reg_alloc_result.reg_alloc_map) {
            const llvm_ir::RegAllocResult& alloc_result = allocation.second;
            if (alloc_result.is_reg) {
                reg physical_reg = alloc_result.regid;
                // 检查是否是callee-saved寄存器（s0-s11: x8-x9, x18-x27）
                if ((physical_reg >= x8 && physical_reg <= x9) || (physical_reg >= x18 && physical_reg <= x27)) {
                    used_callee_saved_regs.insert(physical_reg);
                }
                // 检查是否是浮点callee-saved寄存器（fs0-fs11: f8-f9, f18-f27）
                if ((physical_reg >= f8 && physical_reg <= f9) || (physical_reg >= f18 && physical_reg <= f27)) {
                    used_callee_saved_regs.insert(physical_reg);
                }
            }
        }
    }

    // 5. 生成prologue
    if (stack_size > 0) {
        result += "\t# Function prologue\n";

        int offset = stack_size;
        result += generate_large_immediate_addi("sp", "sp", -stack_size);
        
        offset -= this->Caller_size;

        // 保存ra
        if (needs_ra_save) {
            offset -= 8;
            result += generate_large_offset_store("ra", "sp", offset);
        }
        
        // 保存callee-saved寄存器
        for (reg saved_reg : used_callee_saved_regs) {
            offset -= 8;
            std::string reg_str = to_string(saved_reg);
            if (is_float_reg_string(reg_str)) {
                result += generate_large_offset_fstore(reg_str, "sp", offset);
            } else {
                result += generate_large_offset_store(reg_str, "sp", offset);
            }
        }


        
    }

    // 6. 函数体 - 输出所有基本块，但需要修改ret指令
    std::string function_body;
    if (entry_block) {
        function_body += entry_block->toString();
    }
    for (const auto& block : basic_blocks) {
        if (block) {
            function_body += block->toString();
        }
    }

    // 7. 处理ret指令，在ret前插入epilogue
    if (stack_size > 0) {
        // 在每个ret指令前插入epilogue代码
        size_t pos = 0;
        while ((pos = function_body.find("\tret\n", pos)) != std::string::npos) {
            std::string epilogue = "\t# Function epilogue\n";

            int offset = stack_size;
            offset -= this->Caller_size;

            // 恢复ra
            if (needs_ra_save) {
                offset -= 8;
                epilogue += generate_large_offset_load("ra", "sp", offset);
            }

            // 恢复callee-saved寄存器
            for (reg saved_reg : used_callee_saved_regs) {
                offset -= 8;
                std::string reg_str = to_string(saved_reg);
                if (is_float_reg_string(reg_str)) {
                    epilogue += generate_large_offset_fload(reg_str, "sp", offset);
                } else {
                    epilogue += generate_large_offset_load(reg_str, "sp", offset);
                }
            }

            // 恢复栈指针
            epilogue += generate_large_immediate_addi("sp", "sp", stack_size);

            function_body.insert(pos, epilogue);
            pos += epilogue.length() + 5;  // 跳过插入的epilogue和ret
        }
    }

    result += function_body;

    // 函数大小信息
    result += ".size\t" + name + ", .-" + name + "\n";

    return result;
}

std::string riscvModule::toString() {
    std::string result;

    // 汇编文件头部信息
    result += "\t.file\t\"generated.s\"\n";
    result += "\t.option nopic\n";
    result += "\t.attribute arch, \"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0\"\n";
    result += "\t.attribute unaligned_access, 0\n";
    result += "\t.attribute stack_align, 16\n";
    result += "\n";

    // 1. 输出 .data 段和全局变量
    if (!global_vars.empty()) {
        result += "\t.data\n";

        for (const auto& gvar : global_vars) {
            // 生成全局变量定义
            result += "\t.globl\t" + gvar->name + "\n";
            result += "\t.type\t" + gvar->name + ", @object\n";
            result += "\t.size\t" + gvar->name + ", " + gvar->size + "\n";
            result += gvar->name + ":\n";

            // 生成初始化数据
            if (gvar->initializers.empty()) {
                // 没有初始化数据，全部用零填充
                result += "\t.zero\t" + gvar->size + "\n";
            } else {
                // 根据initializers生成数据
                for (const auto& [type, value] : gvar->initializers) {
                    if (type == 1) {
                        // .word value
                        result += "\t.word\t" + value + "\n";
                    } else if (type == 0) {
                        // .zero N
                        result += "\t.zero\t" + value + "\n";
                    }
                }
            }
            result += "\n";
        }
    }

    result += "\t.align\t4\n";  // .data 16字节对齐

    // 2. 输出 .text 段和所有函数
    result += "\t.section\t.text\n";
    result += "\n";

    for (const auto& func : func_list) {
        result += func->toString();
        result += "\n";
    }

    // 文件尾部标识
    result += "\t.ident\t\"RISC-V Compiler\"\n";
    result += "\t.section\t.note.GNU-stack,\"\",@progbits\n";

    return result;
}

riscvModule* gen_module(llvm_ir::Module* llvm_program) {
    auto riscv_module = new riscvModule();
    
    // 设置全局模块引用，以便在代码生成时查找函数声明
    current_llvm_module = llvm_program;

    // 创建全局变量名到riscvGlobalVariable的映射
    std::unordered_map<std::string, riscvGlobalVariable*> global_var_map;

    // 维护数组初始化状态：变量名 -> {当前位置, 数组大小, 元素大小(字节)}
    std::unordered_map<std::string, std::tuple<int, int, int>> array_init_state;

    // 先创建所有全局变量的基本结构
    for (const auto& llvm_gvar : llvm_program->globalVariables) {
        auto riscv_gvar = std::make_unique<riscvGlobalVariable>();
        riscv_gvar->name = llvm_gvar->name;

        // 根据类型设置size和初始化状态
        if (llvm_gvar->type == llvm_ir::Type::Array) {
            // 数组类型
            int element_size = 4;  // 默认4字节
            switch (llvm_gvar->elementType) {
            case llvm_ir::Type::I32:
            case llvm_ir::Type::Float: element_size = 4; break;
            case llvm_ir::Type::I64: element_size = 8; break;
            case llvm_ir::Type::I8: element_size = 1; break;
            default: element_size = 4; break;
            }

            int total_size = llvm_gvar->arraySize * element_size;
            riscv_gvar->size = std::to_string(total_size);

            // 初始化数组状态：{当前位置, 数组大小, 元素大小}
            array_init_state[llvm_gvar->name] = std::make_tuple(0, llvm_gvar->arraySize, element_size);
        } else {
            // 标量类型
            switch (llvm_gvar->type) {
            case llvm_ir::Type::I32:
            case llvm_ir::Type::Float: riscv_gvar->size = "4"; break;
            case llvm_ir::Type::I64: riscv_gvar->size = "8"; break;
            case llvm_ir::Type::I8: riscv_gvar->size = "1"; break;
            default: riscv_gvar->size = "4"; break;
            }
        }

        global_var_map[llvm_gvar->name] = riscv_gvar.get();
        riscv_module->global_vars.push_back(std::move(riscv_gvar));
    }

    // 处理global函数中的初始化指令
    if (!llvm_program->functions.empty() && llvm_program->functions.back()->name == "global") {
        // const auto& init_instrs = llvm_program->functions.back()->getEntryBlock()->successors[0]->instructions;
        const auto& init_instrs = llvm_program->functions.back()->getEntryBlock()->instructions;
        std::vector<llvm_ir::Instruction*> init_instrs_vec;
        for (const auto& inst : init_instrs) {
            init_instrs_vec.push_back(inst.get());
        }

        for (int i = 0; i < init_instrs_vec.size(); ++i) {
            if (auto* gep_inst = dynamic_cast<llvm_ir::GetElementPtrInst*>(init_instrs_vec[i])) {
                // 数组初始化：getelementptr inbounds i32, ptr @s0_a, i32 0
                auto ptr = gep_inst->operands[0];
                if (ptr->name[0] == '@') {
                    std::string var_name = ptr->name.substr(1);  // 去掉@

                    // 获取索引
                    auto index_operand = gep_inst->operands[1];
                    int index = 0;
                    if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(index_operand)) {
                        index = static_cast<int>(const_int->value);
                    }

                    // 确保下一条指令是store指令
                    if (i + 1 < init_instrs_vec.size()) {
                        if (auto* store_inst = dynamic_cast<llvm_ir::StoreInst*>(init_instrs_vec[i + 1])) {
                            auto value_operand = store_inst->getValueOperand();
                            std::string value_str;

                            // 获取存储的值
                            if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(value_operand)) {
                                value_str = std::to_string(const_int->value);
                            } else if (auto* const_float = dynamic_cast<llvm_ir::ConstantFloat*>(value_operand)) {
                                // 将浮点数转换为IEEE754格式的整数表示
                                if (const_float->type == llvm_ir::Type::Float) {
                                    int32_t ieee754_int = float_to_ieee754_int(const_float->value);
                                    value_str = std::to_string(ieee754_int);
                                } else if (const_float->type == llvm_ir::Type::Double) {
                                    int64_t ieee754_long = double_to_ieee754_long(const_float->value);
                                    value_str = std::to_string(ieee754_long);
                                }
                            } else {
                                value_str = "0";  // 默认值
                            }

                            // 处理数组初始化
                            if (global_var_map.find(var_name) != global_var_map.end() && array_init_state.find(var_name) != array_init_state.end()) {
                                auto& [current_pos, array_size, element_size] = array_init_state[var_name];
                                auto* riscv_gvar = global_var_map[var_name];

                                // 如果当前索引大于当前位置，需要用.zero填充中间的gap
                                if (index > current_pos) {
                                    int gap = index - current_pos;
                                    riscv_gvar->initializers.push_back({0, std::to_string(gap * element_size)});
                                }

                                // 添加当前值
                                riscv_gvar->initializers.push_back({1, value_str});

                                // 更新当前位置
                                current_pos = index + 1;
                            }

                            // 跳过下一条store指令
                            ++i;
                        }
                    }
                }
            } else if (auto* store_inst = dynamic_cast<llvm_ir::StoreInst*>(init_instrs_vec[i])) {
                // 常量初始化：store i32 10, ptr @s0_a
                auto ptr = store_inst->getPointerOperand();
                if (ptr->name[0] == '@') {
                    std::string var_name = ptr->name.substr(1);  // 去掉@
                    auto value_operand = store_inst->getValueOperand();
                    std::string value_str;

                    // 获取存储的值
                    if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(value_operand)) {
                        value_str = std::to_string(const_int->value);
                    } else if (auto* const_float = dynamic_cast<llvm_ir::ConstantFloat*>(value_operand)) {
                        // 将浮点数转换为IEEE754格式的整数表示
                        if (const_float->type == llvm_ir::Type::Float) {
                            int32_t ieee754_int = float_to_ieee754_int(const_float->value);
                            value_str = std::to_string(ieee754_int);
                        } else if (const_float->type == llvm_ir::Type::Double) {
                            int64_t ieee754_long = double_to_ieee754_long(const_float->value);
                            value_str = std::to_string(ieee754_long);
                        }
                    } else {
                        value_str = "0";  // 默认值
                    }

                    // 添加到对应的全局变量（标量变量）
                    if (global_var_map.find(var_name) != global_var_map.end() && array_init_state.find(var_name) == array_init_state.end()) {  // 不是数组
                        global_var_map[var_name]->initializers.push_back({1, value_str});
                    }
                }
            }
        }

        // 完成数组初始化，填充剩余部分
        for (auto& [var_name, state] : array_init_state) {
            auto& [current_pos, array_size, element_size] = state;
            if (current_pos < array_size) {
                int remaining = array_size - current_pos;
                auto* riscv_gvar = global_var_map[var_name];
                riscv_gvar->initializers.push_back({0, std::to_string(remaining * element_size)});
            }
        }
    }

    // 生成riscvFunction列表
    for (const auto& llvm_func : llvm_program->functions) {
        if (!llvm_func->isDeclaration()) {  // 只处理有实现的函数
            // 跳过辅助函数（如global全局变量初始化函数）
            if (llvm_func->name == "global") {
                continue;
            }

            auto riscv_func = gen_function(llvm_func.get());
            if (riscv_func) {
                riscv_module->func_list.push_back(std::unique_ptr<riscvFunction>(riscv_func));
            }
        }
    }

    return riscv_module;
}



riscvFunction* gen_function(llvm_ir::Function* llvm_func) {
    auto riscv_func = new riscvFunction();
    riscv_func->name = llvm_func->name;
    
    // 保存函数参数信息
    riscv_func->function_parameters = llvm_func->parameters;

    // 执行函数级别的寄存器分配和栈管理
    std::cout << "[CodeGen] 为函数 " << llvm_func->name << " 执行寄存器分配和栈管理..." << std::endl;

    // 使用函数级寄存器分配器（内部会调用线性扫描分配器），传入溢出参数
    llvm_ir::FunctionRegAllocator reg_allocator(llvm_func);
    auto reg_alloc_result = reg_allocator.allocate();

    riscv_func->setRegAllocResult(reg_alloc_result);

    // 预扫描计算所有alloca指令的总大小
    int total_alloca_size = 0;
    
    // 扫描所有basic blocks中的alloca指令
    for (const auto& llvm_block : llvm_func->basicBlocks) {
        if (llvm_block) {
            for (const auto& inst : llvm_block->instructions) {
                if (auto* alloca_inst = dynamic_cast<llvm_ir::AllocaInst*>(inst.get())) {
                    int element_count = alloca_inst->arraySize;
                    if (element_count == 0) element_count = 1;  // 单个元素
                    
                    int element_size = 0;
                    switch (alloca_inst->allocatedType) {
                    case llvm_ir::Type::I32: element_size = 4; break;
                    case llvm_ir::Type::I64: element_size = 8; break;
                    case llvm_ir::Type::Float: element_size = 4; break;
                    case llvm_ir::Type::Double: element_size = 8; break;
                    case llvm_ir::Type::Ptr: element_size = 8; break;
                    default: 
                        std::cout << "[CodeGen] WARNING: 未知的alloca类型: " << (int)alloca_inst->allocatedType << std::endl;
                        element_size = 4; // 默认4字节
                        break;
                    }
                    
                    int total_size = element_count * element_size;
                    total_alloca_size += total_size;
                    
                }
            }
        }
    }
    
    // 设置total alloca size
    riscv_func->total_alloca_size= total_alloca_size;
    std::cout << "[CodeGen] 总alloca大小: " << total_alloca_size << "B" << std::endl;
    
    // 分析函数需要保存的寄存器
    std::set<reg> used_callee_saved_regs;  // s0-s11寄存器
    bool needs_ra_save = false;

    // 1. 检查是否需要保存ra（暂时设为需要，因为还没生成指令）
    // 在实际应用中，可以通过分析函数调用来确定
    needs_ra_save = true;  // 保守估计

    // 2. 分析当前函数使用的寄存器类型
    // 2.1 现在所有参数都有内存
    
    // 2.2 通过遍历寄存器分配结果来获取callee-saved寄存器
    if (llvm_func->name != "main") {
        for (const auto& allocation : reg_alloc_result.reg_alloc_map) {
            const llvm_ir::RegAllocResult& alloc_result = allocation.second;
            if (alloc_result.is_reg) {
                reg physical_reg = alloc_result.regid;
                
                // 检查是否是callee-saved寄存器（s0-s11: x8-x9, x18-x27）
                if ((physical_reg >= x8 && physical_reg <= x9) || (physical_reg >= x18 && physical_reg <= x27)) {
                    used_callee_saved_regs.insert(physical_reg);
                }
                // 检查是否是浮点callee-saved寄存器（fs0-fs11: f8-f9, f18-f27）
                if ((physical_reg >= f8 && physical_reg <= f9) || (physical_reg >= f18 && physical_reg <= f27)) {
                    used_callee_saved_regs.insert(physical_reg);
                }
            }
        }
    }
    
    // 3. 计算各部分栈大小
    if (needs_ra_save) riscv_func->Rasize = 8; // ra寄存器
    
    // Callee_size: 当前函数使用的callee-saved寄存器保存空间
    riscv_func->Callee_size = used_callee_saved_regs.size() * 8;
    
    // Caller_size: 为所有函数参数和caller-save临时寄存器分配内存空间
    // 新约定：所有参数都在栈上，caller负责将参数放到callee栈顶
    // 同时为分配到caller-save寄存器的变量预分配保存空间
    int param_space = riscv_func->function_parameters.size() * 8;  // 参数空间
    int caller_save_space = reg_alloc_result.used_caller_save_regs.size() * 8;  // caller-save物理寄存器空间
    riscv_func->Caller_size = param_space + caller_save_space;
    
    std::cout << "[CodeGen] Caller_size breakdown: parameters=" << param_space 
              << " + caller-save registers=" << caller_save_space 
              << " = " << riscv_func->Caller_size << std::endl;
    
    // 总的寄存器保存空间
    riscv_func->Regs2Save_size = riscv_func->Caller_size + riscv_func->Callee_size;

    // 4. 计算总栈大小
    int tot = 0;
    tot += riscv_func->Rasize;
    tot += riscv_func->Regs2Save_size;  // callee-saved + caller-saved 寄存器

    // 使用函数级分配结果中的栈大小
    riscv_func->SpilledVRegs_size = reg_alloc_result.total_stack_size;
    tot += riscv_func->SpilledVRegs_size; // spill寄存器

    tot += riscv_func->total_alloca_size;  // 添加alloca的空间
   
    // 5. 确保16字节对齐
    if (tot % 16 != 0) {
        tot = ((tot + 15) / 16) * 16;
    }

    riscv_func->total_stack_size = tot;

    reg_alloc_result = reg_allocator.post_allocate(riscv_func->total_stack_size - riscv_func->Rasize - riscv_func->Callee_size - riscv_func->Caller_size);

    // 分配参数寄存器的栈偏移（在栈的最顶部）
    int sz = riscv_func->function_parameters.size();
    int int_sz = 0;
    int float_sz = 0;
    for(int p = 0; p < sz; p++){
        int param_offset = riscv_func->total_stack_size - 8*(p+1);  // 参数在栈顶
        llvm_ir::Value* param = riscv_func->function_parameters[p];
        if (param->type == llvm_ir::Type::Float || param->type == llvm_ir::Type::Double) {
            float_sz++;
            if(float_sz < 8){
                reg_alloc_result.reg_alloc_map[param] = {true, riscv::fa[float_sz-1], param_offset};
            }
            else{
                reg_alloc_result.reg_alloc_map[param] = {false, riscv::NONE, param_offset};
            }
        } 
        else{
            int_sz++;
            if(int_sz < 8){
                reg_alloc_result.reg_alloc_map[param] = {true, riscv::a[int_sz-1], param_offset};
            }
            else{
                reg_alloc_result.reg_alloc_map[param] = {false, riscv::NONE, param_offset};
            }
        }
    }
    
    // 为caller-save物理寄存器分配栈偏移（在参数下面）
    std::unordered_map<riscv::reg, int> caller_save_reg_offsets;
    int caller_save_idx = 0;
    for (riscv::reg phys_reg : reg_alloc_result.used_caller_save_regs) {
        int caller_save_offset = riscv_func->total_stack_size - param_space - 8*(caller_save_idx+1);
        caller_save_reg_offsets[phys_reg] = caller_save_offset;
        
        std::cout << "[CodeGen] Caller-save physical register " << (int)phys_reg
                  << " assigned stack offset " << caller_save_offset << std::endl;
        caller_save_idx++;
    }
    
    // 为需要caller-save的虚拟寄存器建立到内存偏移的映射
    // 通过虚拟寄存器→物理寄存器→内存偏移的路径
    for (llvm_ir::Value* vreg : reg_alloc_result.caller_save_vregs) {
        auto it = reg_alloc_result.reg_alloc_map.find(vreg);
        if (it != reg_alloc_result.reg_alloc_map.end() && it->second.is_reg) {
            riscv::reg phys_reg = it->second.regid;
            auto offset_it = caller_save_reg_offsets.find(phys_reg);
            if (offset_it != caller_save_reg_offsets.end()) {
                it->second.stack_offset = offset_it->second;
                std::cout << "[CodeGen] Caller-save variable " << vreg->name 
                          << " (physical reg " << (int)phys_reg << ")"
                          << " assigned stack offset " << offset_it->second << std::endl;
            }
        }
    }
    
    // 设置寄存器分配结果
    riscv_func->setRegAllocResult(reg_alloc_result);
    

    std::cout << "[CodeGen] 计算结果: " << std::endl;
    std::cout << "[CodeGen] Rasize " << riscv_func->Rasize
              << " Caller_size " << riscv_func->Caller_size 
              << " Callee_size " << riscv_func->Callee_size
              << " Regs2Save_size " << riscv_func->Regs2Save_size
              << " SpilledVRegs_size " << riscv_func->SpilledVRegs_size
              << " total_alloca_size " << riscv_func->total_alloca_size
              << " sp缩减总大小 " << riscv_func->total_stack_size << std::endl;

    // 为所有参数建立映射关系
    // 新约定：所有参数都在栈上，caller负责将参数放到callee栈顶
    int assigned_int_params = 0;   // 已分配的整数参数计数
    int assigned_float_params = 0; // 已分配的浮点参数计数
    

    // 输出寄存器分配结果
    for (const auto& [vreg, alloc] : reg_alloc_result.reg_alloc_map) {
        if (alloc.is_reg) {
            std::cout << "[CodeGen] 寄存器分配结果: " << vreg->name << " -> " 
                      << riscv::to_string(alloc.regid) << " (寄存器)" << std::endl;
        } else {
            std::cout << "[CodeGen] 寄存器分配结果: " << vreg->name << " -> " 
                      << "栈偏移 " << alloc.stack_offset << " (溢出)" << std::endl;
        }
    }
    // 设置当前函数上下文
    current_riscv_function = riscv_func;
    // 初始化指令计数器
    riscv_func->current_instruction_index = 1;

    for (const auto& llvm_block : llvm_func->basicBlocks) {
        if (llvm_block) {
            auto riscv_block = gen_block(llvm_block.get());
            if (riscv_block) {
                if (llvm_block.get() == llvm_func->getEntryBlock()) {
                    riscv_func->entry_block = std::unique_ptr<riscvBlock>(riscv_block);
                } else {
                    riscv_func->basic_blocks.push_back(std::unique_ptr<riscvBlock>(riscv_block));
                }
            }
        }
    }
    return riscv_func;
}

riscvBlock* gen_block(llvm_ir::BasicBlock* llvm_block) {
    auto riscv_block = new riscvBlock();
    riscv_block->name = llvm_block->label;

    // 为每个LLVM指令生成对应的RISC-V指令
    for (const auto& llvm_inst : llvm_block->instructions) {
        if (current_riscv_function) {
            current_riscv_function->init_temp_reg_pool();
        }
        auto generated_insts = gen_instruction(llvm_inst.get());
        for (auto& inst : generated_insts) {
            riscv_block->inst_list.push_back(std::move(inst));
        }
        // 更新指令计数器
        if (current_riscv_function) {
            current_riscv_function->current_instruction_index++;
        }
    }

    return riscv_block;
}

// 指令分派主函数
std::vector<std::unique_ptr<riscvInstr>> gen_instruction(llvm_ir::Instruction* llvm_inst) {
    std::vector<std::unique_ptr<riscvInstr>> result;
    using llvm_ir::Opcode;
    switch (llvm_inst->opcode) {
    case Opcode::Shl:
    case Opcode::LShr:
    case Opcode::AShr:
    case Opcode::And:
    case Opcode::Or:
    case Opcode::Xor:
    case Opcode::Add:
    case Opcode::Sub:
    case Opcode::Mul:
    case Opcode::SDiv:
    case Opcode::FAdd:
    case Opcode::FSub:
    case Opcode::FMul:
    case Opcode::FDiv:
    case Opcode::FRem:
    case Opcode::URem:
    case Opcode::SRem:
        if (auto* bin_op = dynamic_cast<llvm_ir::BinaryOperator*>(llvm_inst)) {
            auto insts = gen_binary_op(bin_op);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::Load:
        if (auto* load_inst = dynamic_cast<llvm_ir::LoadInst*>(llvm_inst)) {
            auto insts = gen_load(load_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::Store:
        if (auto* store_inst = dynamic_cast<llvm_ir::StoreInst*>(llvm_inst)) {
            auto insts = gen_store(store_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::Ret: result = gen_ret(llvm_inst); break;
    case Opcode::Br: result = gen_br(llvm_inst); break;
    case Opcode::Alloca:
        if (auto* alloca_inst = dynamic_cast<llvm_ir::AllocaInst*>(llvm_inst)) {
            auto insts = gen_alloca(alloca_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::ICmp:
        if (auto* icmp_inst = dynamic_cast<llvm_ir::ICmpInst*>(llvm_inst)) {
            auto insts = gen_icmp(icmp_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::FCmp:
        if (auto* fcmp_inst = dynamic_cast<llvm_ir::FCmpInst*>(llvm_inst)) {
            auto insts = gen_fcmp(fcmp_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::Call: result = gen_call(llvm_inst); break;
    case Opcode::GetElementPtr:
        if (auto* gep_inst = dynamic_cast<llvm_ir::GetElementPtrInst*>(llvm_inst)) {
            auto insts = gen_getelementptr(gep_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::SIToFP:
        if (auto* cast_inst = dynamic_cast<llvm_ir::SIToFPInst*>(llvm_inst)) {
            auto insts = gen_sitofp(cast_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::FPToSI:
        if (auto* cast_inst = dynamic_cast<llvm_ir::FPToSIInst*>(llvm_inst)) {
            auto insts = gen_fptosi(cast_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::ZExt:
        if (auto* zext_inst = dynamic_cast<llvm_ir::ZExtInst*>(llvm_inst)) {
            auto insts = gen_zext(zext_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;
    case Opcode::SExt:
        if (auto* sext_inst = dynamic_cast<llvm_ir::SExtInst*>(llvm_inst)) {
            auto insts = gen_sext(sext_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
    case Opcode::Move:
        if (auto* move_inst = dynamic_cast<llvm_ir::MoveInst*>(llvm_inst)) {
            auto insts = gen_move(move_inst);
            result.insert(result.end(), std::make_move_iterator(insts.begin()), std::make_move_iterator(insts.end()));
        }
        break;

    default:
        // 未知或暂未支持的指令，生成nop
        std::cout << "[CodeGen] WARNING: 未支持的指令类型，opcode=" << static_cast<int>(llvm_inst->opcode) << std::endl;
        assert(false && "Unsupported instruction type in RISC-V code generation");
        auto riscv_inst = std::make_unique<riscvInstr>();
        riscv_inst->op = opcode::nop;
        riscv_inst->rd = reg::x0;
        riscv_inst->rs1 = reg::x0;
        riscv_inst->rs2 = reg::x0;
        riscv_inst->imm = 0;
        result.push_back(std::move(riscv_inst));
        break;
    }
    return result;
}

bool riscvFunction::is_spilled(llvm_ir::Value* vreg) {
    auto it = reg_alloc_result.reg_alloc_map.find(vreg);
    if (it != reg_alloc_result.reg_alloc_map.end()) {
        return !it->second.is_reg;
    }
    return false;
}

int riscvFunction::get_spill_offset(llvm_ir::Value* vreg) {
    auto it = reg_alloc_result.reg_alloc_map.find(vreg);
    if (it != reg_alloc_result.reg_alloc_map.end()) {
        const auto& alloc = it->second;
        return alloc.stack_offset;
    }
    return -1;  // 未找到或未溢出
}

const llvm_ir::RegAllocResult* riscvFunction::get_allocation_info(llvm_ir::Value* vreg) {
    auto it = reg_alloc_result.reg_alloc_map.find(vreg);
    if (it != reg_alloc_result.reg_alloc_map.end()) {
        return &it->second;
    }
    return nullptr;
}

// 检查是否为alloca产生的静态指针
bool riscvFunction::is_alloca_pointer(llvm_ir::Value* vreg) {
    return alloca_ptr_to_offset.find(vreg) != alloca_ptr_to_offset.end();
}

// 获取alloca指针对应的栈偏移
int riscvFunction::get_alloca_offset(llvm_ir::Value* vreg) {
    auto it = alloca_ptr_to_offset.find(vreg);
    if (it != alloca_ptr_to_offset.end()) {
        return it->second;
    }
    return 0;  // 未找到，返回0
}

// 记录alloca指针到栈偏移的映射
void riscvFunction::record_alloca_pointer(llvm_ir::Value* vreg, int stack_offset) {
    alloca_ptr_to_offset[vreg] = stack_offset;
}

// 检查立即数是否在合法范围内的辅助函数
static bool is_valid_immediate(int value) { return value >= -2048 && value <= 2047; }

// 优化的立即数加载：使用li指令
static void generate_optimized_immediate_load(std::vector<std::unique_ptr<riscvInstr>>& insts, reg rd, int imm_value) {
    // 直接使用li指令，无论立即数大小
    auto li_inst = std::make_unique<riscvInstr>();
    li_inst->op = opcode::li;
    li_inst->rd = rd;
    li_inst->rs1 = reg::x0;
    li_inst->rs2 = reg::x0;
    li_inst->imm = imm_value;
    insts.push_back(std::move(li_inst));
}

// 生成安全的立即数指令：如果立即数超出范围则使用li+reg指令
// 生成安全的32位addi指令：专门用于二元运算
static void generate_safe_addiw(std::vector<std::unique_ptr<riscvInstr>>& insts, reg rd, reg rs1, int imm_value, riscvFunction* func) {
    if (is_valid_immediate(imm_value)) {
        // 立即数在范围内，直接使用addiw
        auto addiw = std::make_unique<riscvInstr>();
        addiw->op = opcode::addiw;
        addiw->rd = rd;
        addiw->rs1 = rs1;
        addiw->rs2 = reg::x0;
        addiw->imm = imm_value;
        insts.push_back(std::move(addiw));
    } else {
        // 立即数超出范围，先用优化的立即数加载到临时寄存器，再用addw
        reg temp_reg = func->get_t_reg();  // 使用临时寄存器管理

        generate_optimized_immediate_load(insts, temp_reg, imm_value);
        // insts.push_back(std::move(li));

        auto addw = std::make_unique<riscvInstr>();
        addw->op = opcode::addw;
        addw->rd = rd;
        addw->rs1 = rs1;
        addw->rs2 = temp_reg;
        addw->imm = 0;
        insts.push_back(std::move(addw));
    }
}

static void generate_safe_addi(std::vector<std::unique_ptr<riscvInstr>>& insts, reg rd, reg rs1, int imm_value, riscvFunction* func) {
    if (is_valid_immediate(imm_value)) {
        // 立即数在范围内，直接使用addi
        auto addi = std::make_unique<riscvInstr>();
        addi->op = opcode::addi;
        addi->rd = rd;
        addi->rs1 = rs1;
        addi->rs2 = reg::x0;
        addi->imm = imm_value;
        insts.push_back(std::move(addi));
    } else {
        // 立即数超出范围，先用优化的立即数加载到临时寄存器，再用add
        reg temp_reg = func->get_t_reg();  // 使用临时寄存器管理

        generate_optimized_immediate_load(insts, temp_reg, imm_value);

        auto add = std::make_unique<riscvInstr>();
        add->op = opcode::add;
        add->rd = rd;
        add->rs1 = rs1;
        add->rs2 = temp_reg;
        add->imm = 0;
        insts.push_back(std::move(add));
    }
}

// 生成安全的li指令：使用优化的立即数加载
static void generate_safe_li(std::vector<std::unique_ptr<riscvInstr>>& insts, reg rd, int imm_value, riscvFunction* func) {
    generate_optimized_immediate_load(insts, rd, imm_value);
}

// 生成安全的load指令：如果偏移超出范围则使用临时寄存器
static void generate_safe_load(std::vector<std::unique_ptr<riscvInstr>>& insts, opcode load_op, reg rd, reg base, int offset, riscvFunction* func) {
    if (is_valid_immediate(offset)) {
        // 偏移在范围内，直接使用load指令
        auto load_inst = std::make_unique<riscvInstr>();
        load_inst->op = load_op;
        load_inst->rd = rd;
        load_inst->rs1 = base;
        load_inst->rs2 = reg::x0;
        load_inst->imm = offset;
        insts.push_back(std::move(load_inst));
    } else {
        // 偏移超出范围，先计算地址到临时寄存器，再从[temp+0]加载
        reg temp_reg = func->get_t_reg();
        
        // 先用优化的立即数加载偏移到临时寄存器
        generate_optimized_immediate_load(insts, temp_reg, offset);
        
        // 计算地址：temp = base + offset
        auto add = std::make_unique<riscvInstr>();
        add->op = opcode::add;
        add->rd = temp_reg;
        add->rs1 = base;
        add->rs2 = temp_reg;
        add->imm = 0;
        insts.push_back(std::move(add));
        
        // 从[temp+0]加载
        auto load_inst = std::make_unique<riscvInstr>();
        load_inst->op = load_op;
        load_inst->rd = rd;
        load_inst->rs1 = temp_reg;
        load_inst->rs2 = reg::x0;
        load_inst->imm = 0;
        insts.push_back(std::move(load_inst));
    }
}

// 生成安全的store指令：如果偏移超出范围则使用临时寄存器
static void generate_safe_store(std::vector<std::unique_ptr<riscvInstr>>& insts, opcode store_op, reg src, reg base, int offset, riscvFunction* func) {
    if (is_valid_immediate(offset)) {
        // 偏移在范围内，直接使用store指令
        auto store_inst = std::make_unique<riscvInstr>();
        store_inst->op = store_op;
        store_inst->rd = reg::x0;
        store_inst->rs1 = base;
        store_inst->rs2 = src;
        store_inst->imm = offset;
        insts.push_back(std::move(store_inst));
    } else {
        // 偏移超出范围，先计算地址到临时寄存器，再存储到[temp+0]
        reg temp_reg = func->get_t_reg();
        
        // 先用li加载偏移到临时寄存器
        auto li = std::make_unique<riscvInstr>();
        li->op = opcode::li;
        li->rd = temp_reg;
        li->rs1 = reg::x0;
        li->rs2 = reg::x0;
        li->imm = offset;
        insts.push_back(std::move(li));
        
        // 计算地址：temp = base + offset
        auto add = std::make_unique<riscvInstr>();
        add->op = opcode::add;
        add->rd = temp_reg;
        add->rs1 = base;
        add->rs2 = temp_reg;
        add->imm = 0;
        insts.push_back(std::move(add));
        
        // 存储到[temp+0]
        auto store_inst = std::make_unique<riscvInstr>();
        store_inst->op = store_op;
        store_inst->rd = reg::x0;
        store_inst->rs1 = temp_reg;
        store_inst->rs2 = src;
        store_inst->imm = 0;
        insts.push_back(std::move(store_inst));
    }
}

// 辅助函数：检查寄存器是否为保留的临时寄存器（只包含前5个）
static bool is_temp_reg(riscv::reg r) {
    // 只有前5个临时寄存器被保留给代码生成器使用
    // 整数临时寄存器: t0-t4 (x5, x6, x7, x28, x29)
    if (r == riscv::x5 || r == riscv::x6 || r == riscv::x7 || r == riscv::x28 || r == riscv::x29) {
        return true;
    }
    // 浮点临时寄存器: ft0-ft4 (f0, f1, f2, f3, f4)
    if (r == riscv::f0 || r == riscv::f1 || r == riscv::f2 || r == riscv::f3 || r == riscv::f4) {
        return true;
    }
    return false;
}

// 辅助函数：检查寄存器是否为保存寄存器
static bool is_saved_reg(riscv::reg r) {
    // 整数保存寄存器: s0-s11 (x8, x9, x18-x27)
    if (r == riscv::x8 || r == riscv::x9 || (r >= riscv::x18 && r <= riscv::x27)) {
        return true;
    }
    // 浮点保存寄存器: fs0-fs11 (f8, f9, f18-f27)
    if (r == riscv::f8 || r == riscv::f9 || (r >= riscv::f18 && r <= riscv::f27)) {
        return true;
    }
    return false;
}

// 初始化临时寄存器池
void riscvFunction::init_temp_reg_pool() {
    // 清空现有池
    available_temp_regs.clear();

    // 只添加前5个临时寄存器到可用池（其他的已经加入寄存器分配池）
    // 整数临时寄存器 (只保留t0-t4)
    available_temp_regs.insert(riscv::x5);   // t0
    available_temp_regs.insert(riscv::x6);   // t1
    available_temp_regs.insert(riscv::x7);   // t2
    available_temp_regs.insert(riscv::x28);  // t3
    available_temp_regs.insert(riscv::x29);  // t4
    // t5-t6 (x30, x31) 已经加入寄存器分配池

    // 浮点临时寄存器 (只保留ft0-ft4)
    available_temp_regs.insert(riscv::f0);   // ft0
    available_temp_regs.insert(riscv::f1);   // ft1
    available_temp_regs.insert(riscv::f2);   // ft2
    available_temp_regs.insert(riscv::f3);   // ft3
    available_temp_regs.insert(riscv::f4);   // ft4
    // ft5-ft11 (f5, f6, f7, f28, f29, f30, f31) 已经加入寄存器分配池

    std::cout << "[TempReg] 初始化临时寄存器池（仅保留t0-t4, ft0-ft4用于临时计算）" << std::endl;
}

// 获取临时寄存器（仅用于临时计算，不用于def操作）
// 简化的获取临时寄存器（仅用于use）- 基于线性扫描分配结果
reg riscvFunction::get_temp_reg_for_use(llvm_ir::Value* vreg, std::vector<std::unique_ptr<riscvInstr>>& post_insts) {
    // 处理alloca静态指针：直接生成地址计算指令
    if (is_alloca_pointer(vreg)) {
        std::cout << "[RegAlloc] 检测到alloca静态指针: " << vreg->name << std::endl;
        
        reg temp_reg = get_t_reg(false);  // 地址始终用整数寄存器
        if (temp_reg == riscv::NONE) {
            std::cout << "[RegAlloc] ERROR: 无法为alloca指针 " << vreg->name << " 分配寄存器" << std::endl;
            abort();
        }
        
        int stack_offset = get_alloca_offset(vreg);
        
        // 生成地址计算指令：temp_reg = sp + offset
        generate_safe_addi(post_insts, temp_reg, reg::x2, stack_offset, this);
        
        std::cout << "[RegAlloc] 为alloca指针 " << vreg->name << " 生成地址计算: " 
                  << riscv::to_string(temp_reg) << " = sp + " << stack_offset << std::endl;
        
        return temp_reg;
    }
    
    // 处理全局变量：检查name的第一个字符是否为'@'
    if (!vreg->name.empty() && vreg->name[0] == '@') {
        std::cout << "[RegAlloc] 检测到全局变量: " << vreg->name << std::endl;
        
        // 为全局变量分配临时寄存器来存储其地址
        bool is_float = (vreg->type == llvm_ir::Type::Float || vreg->type == llvm_ir::Type::Double);
        reg temp_addr_reg = get_t_reg(false);  // 地址始终用整数寄存器
        
        if (temp_addr_reg == riscv::NONE) {
            std::cout << "[RegAlloc] ERROR: 无法为全局变量 " << vreg->name << " 分配地址寄存器" << std::endl;
            abort();
        }
        
        // 生成la指令加载全局变量地址
        auto la_inst = std::make_unique<riscvInstr>();
        la_inst->op = opcode::la;
        la_inst->rd = temp_addr_reg;
        la_inst->rs1 = reg::x0;
        la_inst->rs2 = reg::x0;
        la_inst->imm = 0;
        la_inst->label = vreg->name.substr(1);  // 去掉'@'前缀
        post_insts.push_back(std::move(la_inst));
        
        // 生成load指令从全局变量地址加载值到临时寄存器
        reg temp_value_reg = get_t_reg(is_float);
        if (temp_value_reg == riscv::NONE) {
            std::cout << "[RegAlloc] ERROR: 无法为全局变量 " << vreg->name << " 分配值寄存器" << std::endl;
            abort();
        }
        
        auto load_inst = std::make_unique<riscvInstr>();
        // 根据类型选择合适的load指令
        if (vreg->type == llvm_ir::Type::Float) {
            load_inst->op = opcode::flw;
        } else if (vreg->type == llvm_ir::Type::Double) {
            load_inst->op = opcode::fld;
        } else if (vreg->type == llvm_ir::Type::I32 || vreg->type == llvm_ir::Type::I1) {
            load_inst->op = opcode::lw;
        } else if (vreg->type == llvm_ir::Type::I64 || vreg->type == llvm_ir::Type::Ptr) {
            load_inst->op = opcode::ld;
        } else {
            load_inst->op = opcode::lw;  // 默认32位加载
        }
        
        load_inst->rd = temp_value_reg;
        load_inst->rs1 = temp_addr_reg;
        load_inst->rs2 = reg::x0;
        load_inst->imm = 0;  // 偏移为0
        post_insts.push_back(std::move(load_inst));
        
        std::cout << "[RegAlloc] 为全局变量 " << vreg->name << " 生成la+load指令，值寄存器: " 
                  << riscv::to_string(temp_value_reg) << std::endl;
        
        return temp_value_reg;
    }
    
    // 处理常数值
    if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(vreg)) {
        reg temp_reg = get_t_reg(false);  // 获取整数临时寄存器
        if (temp_reg != riscv::NONE) {
            // 使用安全的立即数加载
            std::vector<std::unique_ptr<riscvInstr>> load_insts;
            generate_safe_li(load_insts, temp_reg, const_int->value, this);
            
            // 将生成的指令添加到post_insts
            for (auto& inst : load_insts) {
                post_insts.push_back(std::move(inst));
            }
            
            std::cout << "[RegAlloc] 为常数 " << const_int->value << " 分配临时寄存器 " 
                      << riscv::to_string(temp_reg) << " (USE)" << std::endl;
        }
        return temp_reg;
    }
    
    if (auto* const_float = dynamic_cast<llvm_ir::ConstantFloat*>(vreg)) {
        reg temp_reg = get_t_reg(true);  // 获取浮点临时寄存器
        if (temp_reg != riscv::NONE) {
            if (const_float->type == llvm_ir::Type::Float) {
                // 单精度浮点常量处理
                reg int_temp = get_t_reg(false);
                
                // 使用安全的立即数加载
                std::vector<std::unique_ptr<riscvInstr>> load_insts;
                int32_t ieee754_int = float_to_ieee754_int(const_float->value);
                generate_safe_li(load_insts, int_temp, ieee754_int, this);
                
                // 将生成的指令添加到post_insts
                for (auto& inst : load_insts) {
                    post_insts.push_back(std::move(inst));
                }
                
                auto fmv_inst = std::make_unique<riscvInstr>();
                fmv_inst->op = opcode::fmv_s_x;
                fmv_inst->rd = temp_reg;
                fmv_inst->rs1 = int_temp;
                fmv_inst->rs2 = reg::x0;
                fmv_inst->imm = 0;
                post_insts.push_back(std::move(fmv_inst));
                
                std::cout << "[RegAlloc] 为单精度浮点常数 " << const_float->value << " 分配临时寄存器 " 
                          << riscv::to_string(temp_reg) << " (USE)" << std::endl;
            } else if (const_float->type == llvm_ir::Type::Double) {
                // 双精度浮点常量处理
                reg int_temp = get_t_reg(false);
                
                // 使用安全的立即数加载（64位值需要特殊处理）
                std::vector<std::unique_ptr<riscvInstr>> load_insts;
                int64_t ieee754_long = double_to_ieee754_long(const_float->value);
                
                // 对于64位立即数，需要分两部分加载
                int32_t high_bits = (ieee754_long >> 32) & 0xFFFFFFFF;
                int32_t low_bits = ieee754_long & 0xFFFFFFFF;
                
                // 加载高32位
                generate_safe_li(load_insts, int_temp, high_bits, this);
                
                // 左移32位
                auto slli_inst = std::make_unique<riscvInstr>();
                slli_inst->op = opcode::slli;
                slli_inst->rd = int_temp;
                slli_inst->rs1 = int_temp;
                slli_inst->rs2 = reg::x0;
                slli_inst->imm = 32;
                load_insts.push_back(std::move(slli_inst));
                
                // 如果低32位非零，需要加上低32位
                if (low_bits != 0) {
                    reg low_temp = get_t_reg(false);
                    generate_safe_li(load_insts, low_temp, low_bits, this);
                    
                    auto add_inst = std::make_unique<riscvInstr>();
                    add_inst->op = opcode::add;
                    add_inst->rd = int_temp;
                    add_inst->rs1 = int_temp;
                    add_inst->rs2 = low_temp;
                    add_inst->imm = 0;
                    load_insts.push_back(std::move(add_inst));
                }
                
                // 将生成的指令添加到post_insts
                for (auto& inst : load_insts) {
                    post_insts.push_back(std::move(inst));
                }
                
                auto fmv_inst = std::make_unique<riscvInstr>();
                fmv_inst->op = opcode::fmv_d_x;
                fmv_inst->rd = temp_reg;
                fmv_inst->rs1 = int_temp;
                fmv_inst->rs2 = reg::x0;
                fmv_inst->imm = 0;
                post_insts.push_back(std::move(fmv_inst));
                
                std::cout << "[RegAlloc] 为双精度浮点常数 " << const_float->value << " 分配临时寄存器 " 
                          << riscv::to_string(temp_reg) << " (USE)" << std::endl;
            }
        }
        return temp_reg;
    }



    // 查找虚拟寄存器的分配结果
    auto it = reg_alloc_result.reg_alloc_map.find(vreg);
    if (it != reg_alloc_result.reg_alloc_map.end()) {
        const auto& alloc = it->second;
        if (alloc.is_reg) {
            // 变量分配在物理寄存器中，直接使用
            std::cout << "[RegAlloc] " << vreg->name << " 使用物理寄存器 " 
                      << riscv::to_string(alloc.regid) << " (USE)" << std::endl;
            return alloc.regid;
        } else {
            // 变量溢出到栈中，需要加载到临时寄存器
            std::cout << "[RegAlloc] " << vreg->name << " 溢出到栈，加载到临时寄存器 (USE)" << std::endl;
            
            bool is_float = (vreg->type == llvm_ir::Type::Float || vreg->type == llvm_ir::Type::Double);
            reg temp_reg = get_t_reg(is_float);
            
            if (temp_reg == riscv::NONE) {
                std::cout << "[RegAlloc] ERROR: 没有可用的临时寄存器给 " << vreg->name << " (USE)" << std::endl;
                abort();
            }
            
            // 生成从栈加载到临时寄存器的指令
            auto load_insts = gen_load_from_stack(vreg, temp_reg);
            for (auto& load_inst : load_insts) {
                post_insts.push_back(std::move(load_inst));
            }
            std::cout << "[RegAlloc] 生成load指令：从栈加载 " << vreg->name 
                      << " 到临时寄存器 " << riscv::to_string(temp_reg) << std::endl;
            
            return temp_reg;
        }
    } else {
        std::cout << "[RegAlloc] ERROR: " << vreg->name << " 没有分配信息 (USE)" << std::endl;
        abort();
    }
}

// 获取寄存器用于def（不能使用临时寄存器）


// 简化的获取寄存器用于def - 基于线性扫描分配结果
reg riscvFunction::get_reg_for_def(llvm_ir::Value* vreg, std::vector<std::unique_ptr<riscvInstr>>& post_insts) {
    // 处理alloca静态指针：这种情况不应该发生，因为alloca指针不应该被重新定义
    if (is_alloca_pointer(vreg)) {
        std::cout << "[RegAlloc] ERROR: 试图重新定义alloca静态指针: " << vreg->name << std::endl;
        abort();  // alloca指针不应该被重新定义
    }
    
    auto it = reg_alloc_result.reg_alloc_map.find(vreg);
    std::cout << "[RegAlloc] 获取寄存器用于def: " << vreg->name << std::endl;
    
    if (it != reg_alloc_result.reg_alloc_map.end()) {
        const auto& alloc = it->second;
        if (alloc.is_reg) {
            // 变量分配在物理寄存器中，直接使用
            std::cout << "[RegAlloc] " << vreg->name << " -> " << riscv::to_string(alloc.regid) << " (DEF)" << std::endl;
            return alloc.regid;
        } else {
            // 变量溢出到栈中，分配临时寄存器进行计算
            // 在计算完成后，调用方需要生成store指令将结果存储到栈
            std::cout << "[RegAlloc] " << vreg->name << " -> SPILLED (DEF)，使用临时寄存器计算" << std::endl;
            
            bool is_float = (vreg->type == llvm_ir::Type::Float || vreg->type == llvm_ir::Type::Double);
            reg temp_reg = get_t_reg(is_float);
            
            if (temp_reg == riscv::NONE) {
                std::cout << "[RegAlloc] ERROR: 没有可用的临时寄存器给 " << vreg->name << " (DEF)" << std::endl;
                abort();
            }
            
            std::cout << "[RegAlloc] 为 " << vreg->name << " 分配临时寄存器 " 
                      << riscv::to_string(temp_reg) << " (DEF，后续需要store)" << std::endl;

            return temp_reg;
        }
    } else {
        std::cout << "[RegAlloc] Warning No used Def: " << vreg->name << " 没有分配信息 (DEF)" << std::endl;
        bool is_float = (vreg->type == llvm_ir::Type::Float || vreg->type == llvm_ir::Type::Double);
        return get_t_reg(is_float); // 获取一个临时寄存器 这个情况是def但是没有活跃区间 这就意味着这个def没有use 只需要临时寄存器暂存即可
        //abort();
    }
}

// 辅助函数：获取类型大小
int riscvFunction::get_type_size(llvm_ir::Type type) {
    switch (type) {
    case llvm_ir::Type::I1: return 4;
    case llvm_ir::Type::I8: return 4;
    case llvm_ir::Type::I32: return 4;
    case llvm_ir::Type::I64: return 8;
    case llvm_ir::Type::Float: return 4;
    case llvm_ir::Type::Double: return 8;
    case llvm_ir::Type::Ptr: return 8;
    default: return 4;
    }
}

// 辅助函数：获取类型对齐
int riscvFunction::get_type_alignment(llvm_ir::Type type) {
    switch (type) {
    case llvm_ir::Type::I1: return 4;
    case llvm_ir::Type::I8: return 4;
    case llvm_ir::Type::I32: return 4;
    case llvm_ir::Type::I64: return 8;
    case llvm_ir::Type::Float: return 4;
    case llvm_ir::Type::Double: return 8;
    case llvm_ir::Type::Ptr: return 8;
    default: return 4;
    }
}


void riscvFunction::save_caller_saved_regs(std::vector<std::unique_ptr<riscvInstr>>& insts) {
    // 新的caller-save逻辑：
    // 只保存那些满足以下条件的寄存器：
    // 1. 虚拟寄存器被分配到caller-save寄存器
    // 2. 虚拟寄存器的活跃区间起点在函数调用前
    // 3. 虚拟寄存器的活跃区间终点在函数调用后
    // 4. 函数调用中会使用这个物理寄存器（参数传递等）
    
    int current_call_position = current_riscv_function->current_instruction_index;
    std::cout << "[CallerSave] Function call at position " << current_call_position << std::endl;
    
    // 1. 处理参数寄存器 - 这些在函数调用时总是会被使用
    int sz = current_riscv_function->function_parameters.size();
    int float_sz = 0;
    int int_sz = 0;
    for(int i = 0; i < sz; i++){
        llvm_ir::Value* param = current_riscv_function->function_parameters[i];

        if (param->type == llvm_ir::Type::Float || param->type == llvm_ir::Type::Double) {
            float_sz++;
            if(float_sz > 8) continue;
            
            // 参数寄存器在函数调用时会被使用，需要检查是否跨越调用
            auto param_alloc_it = current_riscv_function->reg_alloc_result.reg_alloc_map.find(param);
            if (param_alloc_it != current_riscv_function->reg_alloc_result.reg_alloc_map.end() && 
                param_alloc_it->second.is_reg) {
                
                // 检查参数的活跃区间是否跨越函数调用
                auto interval_it = current_riscv_function->reg_alloc_result.intervals.find(param);
                if (interval_it != current_riscv_function->reg_alloc_result.intervals.end()) {
                    const auto& interval = interval_it->second;
                    if (!interval.ranges.empty()) {
                        const auto& range = interval.ranges[0];
                        // 活跃区间跨越函数调用点才需要保存
                        if (range.start < current_call_position && range.end > current_call_position) {
                            int offset = param_alloc_it->second.stack_offset;
                            generate_safe_store(insts, opcode::fsd, riscv::fa[float_sz - 1], sp, offset, this);
                            std::cout << "[CallerSave] Saving parameter fa" << (float_sz - 1) << " (" << param->name 
                                      << ") spans call [" << range.start << "," << range.end << ") at " << current_call_position << std::endl;
                        }
                    }
                }
            }
        } else {
            int_sz++;
            if(int_sz > 8) continue;

            // 检查参数寄存器
            auto param_alloc_it = current_riscv_function->reg_alloc_result.reg_alloc_map.find(param);
            if (param_alloc_it != current_riscv_function->reg_alloc_result.reg_alloc_map.end() && 
                param_alloc_it->second.is_reg) {
                
                // 检查参数的活跃区间是否跨越函数调用
                auto interval_it = current_riscv_function->reg_alloc_result.intervals.find(param);
                if (interval_it != current_riscv_function->reg_alloc_result.intervals.end()) {
                    const auto& interval = interval_it->second;
                    if (!interval.ranges.empty()) {
                        const auto& range = interval.ranges[0];
                        // 活跃区间跨越函数调用点才需要保存
                        if (range.start < current_call_position && range.end > current_call_position) {
                            int offset = param_alloc_it->second.stack_offset;
                            generate_safe_store(insts, opcode::sd, riscv::a[int_sz - 1], sp, offset, this);
                            std::cout << "[CallerSave] Saving parameter a" << (int_sz - 1) << " (" << param->name 
                                      << ") spans call [" << range.start << "," << range.end << ") at " << current_call_position << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    // 2. 处理分配给变量的caller-save寄存器
    // 只保存那些活跃区间跨越函数调用点的变量
    for (llvm_ir::Value* vreg : current_riscv_function->reg_alloc_result.caller_save_vregs) {
        current_riscv_function->init_temp_reg_pool();
        auto alloc_it = current_riscv_function->reg_alloc_result.reg_alloc_map.find(vreg);
        if (alloc_it != current_riscv_function->reg_alloc_result.reg_alloc_map.end() && alloc_it->second.is_reg) {
            
            // 检查活跃区间是否跨越函数调用点
            auto interval_it = current_riscv_function->reg_alloc_result.intervals.find(vreg);
            if (interval_it != current_riscv_function->reg_alloc_result.intervals.end()) {
                const auto& interval = interval_it->second;
                if (!interval.ranges.empty()) {
                    const auto& range = interval.ranges[0];
                    
                    // 只有当活跃区间跨越函数调用点时才需要保存
                    if (range.start < current_call_position && range.end > current_call_position) {
                        riscv::reg physical_reg = alloc_it->second.regid;
                        int offset = alloc_it->second.stack_offset;
                        
                        // 根据寄存器类型选择存储指令
                        if (vreg->type == llvm_ir::Type::Float || vreg->type == llvm_ir::Type::Double) {
                            generate_safe_store(insts, opcode::fsd, physical_reg, sp, offset, this);
                        } else {
                            generate_safe_store(insts, opcode::sd, physical_reg, sp, offset, this);
                        }
                        
                        std::cout << "[CallerSave] Saving caller-save variable " << vreg->name 
                                  << " from " << riscv::to_string(physical_reg) << " spans call [" << range.start 
                                  << "," << range.end << ") at " << current_call_position << std::endl;
                    } else {
                        std::cout << "[CallerSave] Skipping " << vreg->name 
                                  << " - interval [" << range.start << "," << range.end 
                                  << ") does not span call at " << current_call_position << std::endl;
                    }
                }
            }
        }
    }
}

void riscvFunction::restore_caller_saved_regs(std::vector<std::unique_ptr<riscvInstr>>& insts) {
    // 新的caller-restore逻辑：与save_caller_saved_regs对应
    // 只恢复那些活跃区间跨越函数调用的寄存器
    
    int current_call_position = current_riscv_function->current_instruction_index;
    std::cout << "[CallerRestore] Function call at position " << current_call_position << std::endl;
    
    // 1. 处理参数寄存器恢复
    int sz = current_riscv_function->function_parameters.size();
    int float_sz = 0;
    int int_sz = 0;
    for(int i = 0; i < sz; i++){
        llvm_ir::Value* param = current_riscv_function->function_parameters[i];

        if (param->type == llvm_ir::Type::Float || param->type == llvm_ir::Type::Double) {
            float_sz++;
            if(float_sz > 8) continue;

            // 检查参数是否需要恢复 - 必须与save的条件一致
            auto param_alloc_it = current_riscv_function->reg_alloc_result.reg_alloc_map.find(param);
            if (param_alloc_it != current_riscv_function->reg_alloc_result.reg_alloc_map.end() && 
                param_alloc_it->second.is_reg) {
                
                // 检查参数的活跃区间是否跨越函数调用
                auto interval_it = current_riscv_function->reg_alloc_result.intervals.find(param);
                if (interval_it != current_riscv_function->reg_alloc_result.intervals.end()) {
                    const auto& interval = interval_it->second;
                    if (!interval.ranges.empty()) {
                        const auto& range = interval.ranges[0];
                        // 活跃区间跨越函数调用点才需要恢复
                        if (range.start < current_call_position && range.end > current_call_position) {
                            int offset = param_alloc_it->second.stack_offset;
                            generate_safe_load(insts, opcode::fld, riscv::fa[float_sz - 1], sp, offset, this);
                            std::cout << "[CallerRestore] Restoring parameter fa" << (float_sz - 1) << " (" << param->name 
                                      << ") spans call [" << range.start << "," << range.end << ") at " << current_call_position << std::endl;
                        }
                    }
                }
            }
        } else {
            int_sz++;
            if(int_sz > 8) continue;

            // 检查参数是否需要恢复
            auto param_alloc_it = current_riscv_function->reg_alloc_result.reg_alloc_map.find(param);
            if (param_alloc_it != current_riscv_function->reg_alloc_result.reg_alloc_map.end() && 
                param_alloc_it->second.is_reg) {
                
                // 检查参数的活跃区间是否跨越函数调用
                auto interval_it = current_riscv_function->reg_alloc_result.intervals.find(param);
                if (interval_it != current_riscv_function->reg_alloc_result.intervals.end()) {
                    const auto& interval = interval_it->second;
                    if (!interval.ranges.empty()) {
                        const auto& range = interval.ranges[0];
                        // 活跃区间跨越函数调用点才需要恢复
                        if (range.start < current_call_position && range.end > current_call_position) {
                            int offset = param_alloc_it->second.stack_offset;
                            generate_safe_load(insts, opcode::ld, riscv::a[int_sz - 1], sp, offset, this);
                            std::cout << "[CallerRestore] Restoring parameter a" << (int_sz - 1) << " (" << param->name 
                                      << ") spans call [" << range.start << "," << range.end << ") at " << current_call_position << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    // 2. 处理分配给变量的caller-save寄存器恢复
    for (llvm_ir::Value* vreg : current_riscv_function->reg_alloc_result.caller_save_vregs) {
        current_riscv_function->init_temp_reg_pool();
        auto alloc_it = current_riscv_function->reg_alloc_result.reg_alloc_map.find(vreg);
        if (alloc_it != current_riscv_function->reg_alloc_result.reg_alloc_map.end() && alloc_it->second.is_reg) {
            
            // 检查活跃区间是否跨越函数调用点 - 必须与save的条件一致
            auto interval_it = current_riscv_function->reg_alloc_result.intervals.find(vreg);
            if (interval_it != current_riscv_function->reg_alloc_result.intervals.end()) {
                const auto& interval = interval_it->second;
                if (!interval.ranges.empty()) {
                    const auto& range = interval.ranges[0];
                    
                    // 只有当活跃区间跨越函数调用点时才需要恢复
                    if (range.start < current_call_position && range.end > current_call_position) {
                        riscv::reg physical_reg = alloc_it->second.regid;
                        int offset = alloc_it->second.stack_offset;
                        
                        // 根据寄存器类型选择加载指令
                        if (vreg->type == llvm_ir::Type::Float || vreg->type == llvm_ir::Type::Double) {
                            generate_safe_load(insts, opcode::fld, physical_reg, sp, offset, this);
                        } else {
                            generate_safe_load(insts, opcode::ld, physical_reg, sp, offset, this);
                        }
                        
                        std::cout << "[CallerRestore] Restoring caller-save variable " << vreg->name 
                                  << " from stack offset " << offset << " to " << riscv::to_string(physical_reg) 
                                  << " spans call [" << range.start << "," << range.end << ") at " << current_call_position << std::endl;
                    } else {
                        std::cout << "[CallerRestore] Skipping " << vreg->name 
                                  << " - interval [" << range.start << "," << range.end 
                                  << ") does not span call at " << current_call_position << std::endl;
                    }
                }
            }
        }
    }
}


// 获取一个可用的临时寄存器
reg riscvFunction::get_t_reg(bool is_float) {
    // 如果没有可用的临时寄存器，断言失败
    if (available_temp_regs.empty()) {
        assert(false && "临时寄存器不足！！！");
    }

    // 从可用寄存器中找到合适类型的寄存器
    for (auto it = available_temp_regs.begin(); it != available_temp_regs.end(); ++it) {
        reg candidate = *it;
        bool reg_is_float = (candidate >= riscv::f0 && candidate <= riscv::f31);
        
        if (is_float == reg_is_float) {
            // 找到合适的寄存器
            available_temp_regs.erase(it);
            std::cout << "[TempReg] 分配临时寄存器 " << riscv::to_string(candidate) << " (类型: " << (is_float ? "float" : "int") << ")" << std::endl;
            return candidate;
        }
    }
    
    // 如果没有找到合适类型的寄存器，断言失败
    assert(false && "临时寄存器不足！！！");
}

std::vector<std::unique_ptr<riscvInstr>> riscvFunction::gen_load_from_stack(llvm_ir::Value* vreg, reg dest_reg) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    int raw_offset = get_spill_offset(vreg);
    if (raw_offset == -1) {
        std::cout << "[Load] 错误：无法获取 " << vreg->name << " 的栈偏移" << std::endl;
        return insts;
    }

    // 修复：post_allocate已经将原始offset转换为相对于sp的正确偏移，直接使用即可
    int corrected_offset = raw_offset;

    // 根据类型选择加载指令操作码
    opcode load_op;
    if (vreg->type == llvm_ir::Type::Float) {
        load_op = opcode::flw;
    } else if (vreg->type == llvm_ir::Type::Double) {
        load_op = opcode::fld;
    } else if (vreg->type == llvm_ir::Type::I64) {
        load_op = opcode::ld;
    } else if (vreg->type == llvm_ir::Type::Ptr){
        load_op = opcode::ld;
    } else {
        load_op = opcode::lw;
    }

    // 使用安全的load指令生成
    generate_safe_load(insts, load_op, dest_reg, riscv::sp, corrected_offset, this);

    std::cout << "[Load] 生成LOAD指令: " << vreg->name << " -> " << riscv::to_string(dest_reg) 
              << " (stack_offset: " << corrected_offset << ")" << std::endl;

    return insts;
}

std::vector<std::unique_ptr<riscvInstr>> riscvFunction::gen_store_to_stack(llvm_ir::Value* vreg, reg src_reg) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    int raw_offset = get_spill_offset(vreg);
    if (raw_offset == -1) {
        std::cout << "[Store] 错误：无法获取 " << vreg->name << " 的栈偏移" << std::endl;
        return insts;
    }

    // 修复：post_allocate已经将原始offset转换为相对于sp的正确偏移，直接使用即可
    int corrected_offset = raw_offset;

    // 根据类型选择存储指令操作码
    opcode store_op;
    if (vreg->type == llvm_ir::Type::Float) {
        store_op = opcode::fsw;
    } else if (vreg->type == llvm_ir::Type::Double) {
        store_op = opcode::fsd;
    } else if (vreg->type == llvm_ir::Type::Ptr) {
        store_op = opcode::sd;
    } else {
        store_op = opcode::sw;
    }

    // 使用安全的store指令生成
    generate_safe_store(insts, store_op, src_reg, riscv::sp, corrected_offset, this);
    
    std::cout << "[Store] 生成STORE指令: " << riscv::to_string(src_reg) << " -> " << vreg->name 
              << " (stack_offset: " << corrected_offset << ")" << std::endl;

    return insts;
}

// 辅助函数：为溢出变量生成store指令
void add_spilled_store_if_needed(std::vector<std::unique_ptr<riscvInstr>>& insts, 
                                 llvm_ir::Value* vreg, reg src_reg, riscvFunction* func) {
    if (func->is_spilled(vreg)) {
        auto store_insts = func->gen_store_to_stack(vreg, src_reg);
        for (auto& store_inst : store_insts) {
            insts.push_back(std::move(store_inst));
        }
        std::cout << "[RegAlloc] 生成store指令：将 " << vreg->name 
                  << " 从寄存器 " << riscv::to_string(src_reg) << " 存储到栈" << std::endl;
        // 如果src_reg是临时寄存器，也需要释放
    }
}

// 算术/ALU指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_binary_op(llvm_ir::BinaryOperator* bin_op) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    if (!bin_op) {
        std::cout << "[CodeGen] WARNING: null binary operator" << std::endl;
        return insts;
    }

    riscvFunction* func = current_riscv_function;
    
    // 获取目标寄存器，处理可能的寄存器冲突
    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    reg rd = func->get_reg_for_def(bin_op, post_insts);
    
    // 将后续指令加入指令序列
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }

    // 处理源操作数
    std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
    reg rs1 = func->get_temp_reg_for_use(bin_op->operands[0], rs1_load_insts);
    
    // 添加rs1的load指令
    for (auto&& inst : rs1_load_insts) {
        insts.push_back(std::move(inst));
    }
    
    // 处理第二个操作数，检查是否可以使用立即数
    reg rs2 = riscv::NONE;
    bool use_immediate = false;
    int imm_value = 0;
    
    // 如果第二个操作数是常量，尝试使用立即数指令
    if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(bin_op->operands[1])) {
        imm_value = const_int->value;
        bool is_float = (bin_op->type == llvm_ir::Type::Float || bin_op->type == llvm_ir::Type::Double);
        
        if (!is_float) {
            switch (bin_op->opcode) {
            case llvm_ir::Opcode::Add:
                if (is_valid_immediate(imm_value)) use_immediate = true;
                break;
            case llvm_ir::Opcode::Sub:  // 减法可以转换为addi负数
                if (is_valid_immediate(-imm_value)) use_immediate = true;
                break;
            case llvm_ir::Opcode::Shl:  // 左移
                if (imm_value >= 0 && imm_value < 32) use_immediate = true;
                break;
            case llvm_ir::Opcode::LShr: // 逻辑右移
                if (imm_value >= 0 && imm_value < 32) use_immediate = true;
                break;
            case llvm_ir::Opcode::AShr: // 算术右移
                if (imm_value >= 0 && imm_value < 32) use_immediate = true;
                break;
            default: break;
            }
        }
    }
    
    if (!use_immediate) {
        std::vector<std::unique_ptr<riscvInstr>> rs2_load_insts;
        rs2 = func->get_temp_reg_for_use(bin_op->operands[1], rs2_load_insts);
        
        // 添加rs2的load指令
        for (auto&& inst : rs2_load_insts) {
            insts.push_back(std::move(inst));
        }
    }
    
    // 检查是否需要类型转换
    bool is_float_op = (bin_op->type == llvm_ir::Type::Float || bin_op->type == llvm_ir::Type::Double);
    
    if (is_float_op) {
        // 浮点运算，检查操作数是否需要转换
        
        // 检查第一个操作数
        if (bin_op->operands[0]->type == llvm_ir::Type::I32) {
            std::cout << "[CodeGen] Converting first operand from int to float" << std::endl;
            reg temp_float_reg = func->get_t_reg(true);  // 获取浮点寄存器
            
            auto fcvt_inst = std::make_unique<riscvInstr>();
            fcvt_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fcvt_s_w : opcode::fcvt_d_w;
            fcvt_inst->rd = temp_float_reg;
            fcvt_inst->rs1 = rs1;
            fcvt_inst->rs2 = reg::x0;
            fcvt_inst->imm = 0;
            insts.push_back(std::move(fcvt_inst));
            
            rs1 = temp_float_reg;  // 更新rs1为转换后的浮点寄存器
        }
        
        // 检查第二个操作数（如果不是立即数）
        if (!use_immediate && bin_op->operands[1]->type == llvm_ir::Type::I32) {
            std::cout << "[CodeGen] Converting second operand from int to float" << std::endl;
            reg temp_float_reg = func->get_t_reg(true);  // 获取浮点寄存器
            
            auto fcvt_inst = std::make_unique<riscvInstr>();
            fcvt_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fcvt_s_w : opcode::fcvt_d_w;
            fcvt_inst->rd = temp_float_reg;
            fcvt_inst->rs1 = rs2;
            fcvt_inst->rs2 = reg::x0;
            fcvt_inst->imm = 0;
            insts.push_back(std::move(fcvt_inst));
            
            rs2 = temp_float_reg;  // 更新rs2为转换后的浮点寄存器
        }
        
        // 如果第二个操作数是整数常量，需要转换为浮点常量
        if (use_immediate && bin_op->operands[1]->type == llvm_ir::Type::I32) {
            std::cout << "[CodeGen] Converting immediate value from int to float" << std::endl;
            
            // 先将整数立即数加载到整数寄存器
            reg temp_int_reg = func->get_t_reg(false);
            auto li_inst = std::make_unique<riscvInstr>();
            li_inst->op = opcode::li;
            li_inst->rd = temp_int_reg;
            li_inst->rs1 = reg::x0;
            li_inst->rs2 = reg::x0;
            li_inst->imm = imm_value;
            insts.push_back(std::move(li_inst));
            
            // 然后转换为浮点数
            reg temp_float_reg = func->get_t_reg(true);
            auto fcvt_inst = std::make_unique<riscvInstr>();
            fcvt_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fcvt_s_w : opcode::fcvt_d_w;
            fcvt_inst->rd = temp_float_reg;
            fcvt_inst->rs1 = temp_int_reg;
            fcvt_inst->rs2 = reg::x0;
            fcvt_inst->imm = 0;
            insts.push_back(std::move(fcvt_inst));
            
            rs2 = temp_float_reg;
            use_immediate = false;  // 不再使用立即数
        }
    }
    
    // 如果使用立即数指令
    if (use_immediate) {
        auto alu_inst = std::make_unique<riscvInstr>();
        // 处理立即数运算
        switch (bin_op->opcode) {
        case llvm_ir::Opcode::Add:
            generate_safe_addiw(insts, rd, rs1, imm_value, func);
            break;
        case llvm_ir::Opcode::Sub:
            generate_safe_addiw(insts, rd, rs1, -imm_value, func);  // 减法转换为加负数
            break;
        case llvm_ir::Opcode::Shl:
            if (is_valid_immediate(imm_value)) {
                alu_inst->op = opcode::slliw;  // 32位左移立即数
                alu_inst->rd = rd;
                alu_inst->rs1 = rs1;
                alu_inst->imm = imm_value;
                insts.push_back(std::move(alu_inst));
            } else {
                // 左移值超出范围，需要用寄存器版本
                reg temp_reg = func->get_t_reg();
                auto li = std::make_unique<riscvInstr>();
                li->op = opcode::li;
                li->rd = temp_reg;
                li->imm = imm_value;
                insts.push_back(std::move(li));
                
                alu_inst->op = opcode::sllw;  // 32位左移
                alu_inst->rd = rd;
                alu_inst->rs1 = rs1;
                alu_inst->rs2 = temp_reg;
                alu_inst->imm = 0;
                insts.push_back(std::move(alu_inst));
            }
            break;
        case llvm_ir::Opcode::LShr:
            if (is_valid_immediate(imm_value)) {
                alu_inst->op = opcode::srliw;  // 32位逻辑右移立即数
                alu_inst->rd = rd;
                alu_inst->rs1 = rs1;
                alu_inst->imm = imm_value;
                insts.push_back(std::move(alu_inst));
            } else {
                // 右移值超出范围，需要用寄存器版本
                reg temp_reg = func->get_t_reg();
                generate_optimized_immediate_load(insts, temp_reg, imm_value);
                
                alu_inst->op = opcode::srlw;  // 32位逻辑右移
                alu_inst->rd = rd;
                alu_inst->rs1 = rs1;
                alu_inst->rs2 = temp_reg;
                alu_inst->imm = 0;
                insts.push_back(std::move(alu_inst));
            }
            break;
        case llvm_ir::Opcode::AShr:
            if (is_valid_immediate(imm_value)) {
                alu_inst->op = opcode::sraiw;  // 32位算术右移立即数
                alu_inst->rd = rd;
                alu_inst->rs1 = rs1;
                alu_inst->imm = imm_value;
                insts.push_back(std::move(alu_inst));
            } else {
                // 右移值超出范围，需要用寄存器版本
                reg temp_reg = func->get_t_reg();
                generate_optimized_immediate_load(insts, temp_reg, imm_value);
                
                alu_inst->op = opcode::sraw;  // 32位算术右移
                alu_inst->rd = rd;
                alu_inst->rs1 = rs1;
                alu_inst->rs2 = temp_reg;
                alu_inst->imm = 0;
                insts.push_back(std::move(alu_inst));
            }
            break;
        default: assert(false && "Unsupported immediate operation"); break;
        }
        
        // 释放临时寄存器
        
        // 检查结果是否需要store到栈（溢出变量）
        add_spilled_store_if_needed(insts, bin_op, rd, func);
        
        return insts;  // 立即数情况提前返回
    }

    // 检查是否为位运算，直接进行位操作
    if (bin_op->opcode == llvm_ir::Opcode::And || 
        bin_op->opcode == llvm_ir::Opcode::Or || 
        bin_op->opcode == llvm_ir::Opcode::Xor) {
        
        // 位运算的实现：直接对操作数进行按位运算，不需要转换为布尔值
        
        // 生成位运算指令
        auto bitwise_inst = std::make_unique<riscvInstr>();
        switch (bin_op->opcode) {
        case llvm_ir::Opcode::And:
            bitwise_inst->op = opcode::and_;  // 按位与
            break;
        case llvm_ir::Opcode::Or:
            bitwise_inst->op = opcode::or_;   // 按位或
            break;
        case llvm_ir::Opcode::Xor:
            bitwise_inst->op = opcode::xor_;  // 按位异或
            break;
        default:
            bitwise_inst->op = opcode::and_;
            break;
        }
        bitwise_inst->rd = rd;
        bitwise_inst->rs1 = rs1;
        bitwise_inst->rs2 = rs2;
        bitwise_inst->imm = 0;
        insts.push_back(std::move(bitwise_inst));
        
        // 检查结果是否需要store到栈（溢出变量）
        add_spilled_store_if_needed(insts, bin_op, rd, func);
        
        return insts;
    }

    // 特殊处理浮点余数运算（FRem）
    if (bin_op->opcode == llvm_ir::Opcode::FRem) {
        // 浮点余数运算：a % b = a - trunc(a/b) * b
        // 需要使用多个指令来实现
        
        // 1. 计算 a / b
        reg temp_div = func->get_t_reg(true);  // 浮点临时寄存器
        auto fdiv_inst = std::make_unique<riscvInstr>();
        fdiv_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fdiv_s : opcode::fdiv_d;
        fdiv_inst->rd = temp_div;
        fdiv_inst->rs1 = rs1;
        fdiv_inst->rs2 = rs2;
        fdiv_inst->imm = 0;
        insts.push_back(std::move(fdiv_inst));
        
        // 2. 截断除法结果到整数（向零舍入）
        reg temp_int = func->get_t_reg(false);  // 整数临时寄存器
        auto fcvt_inst = std::make_unique<riscvInstr>();
        fcvt_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fcvt_w_s : opcode::fcvt_w_d;
        fcvt_inst->rd = temp_int;
        fcvt_inst->rs1 = temp_div;
        fcvt_inst->rs2 = reg::x0;
        fcvt_inst->imm = 1;  // RTZ (Round Toward Zero) 舍入模式
        insts.push_back(std::move(fcvt_inst));
        
        // 3. 将截断结果转换回浮点数
        reg temp_trunc = func->get_t_reg(true);  // 浮点临时寄存器
        auto fcvt_back_inst = std::make_unique<riscvInstr>();
        fcvt_back_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fcvt_s_w : opcode::fcvt_d_w;
        fcvt_back_inst->rd = temp_trunc;
        fcvt_back_inst->rs1 = temp_int;
        fcvt_back_inst->rs2 = reg::x0;
        fcvt_back_inst->imm = 0;
        insts.push_back(std::move(fcvt_back_inst));
        
        // 4. 计算 trunc(a/b) * b
        reg temp_mul = func->get_t_reg(true);  // 浮点临时寄存器
        auto fmul_inst = std::make_unique<riscvInstr>();
        fmul_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fmul_s : opcode::fmul_d;
        fmul_inst->rd = temp_mul;
        fmul_inst->rs1 = temp_trunc;
        fmul_inst->rs2 = rs2;
        fmul_inst->imm = 0;
        insts.push_back(std::move(fmul_inst));
        
        // 5. 计算 a - (trunc(a/b) * b)
        auto fsub_inst = std::make_unique<riscvInstr>();
        fsub_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fsub_s : opcode::fsub_d;
        fsub_inst->rd = rd;
        fsub_inst->rs1 = rs1;
        fsub_inst->rs2 = temp_mul;
        fsub_inst->imm = 0;
        insts.push_back(std::move(fsub_inst));
        
        // 释放临时寄存器
        
        // 检查结果是否需要store到栈（溢出变量）
        add_spilled_store_if_needed(insts, bin_op, rd, func);
        
        return insts;
    }

    // 生成ALU指令（寄存器运算）
    auto alu_inst = std::make_unique<riscvInstr>();
    bool is_float = (bin_op->type == llvm_ir::Type::Float || bin_op->type == llvm_ir::Type::Double);

    switch (bin_op->opcode) {
    case llvm_ir::Opcode::Add: 
        if (is_float) {
            alu_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fadd_s : opcode::fadd_d;
        } else {
            alu_inst->op = opcode::addw;
        }
        break;
    case llvm_ir::Opcode::Sub: 
        if (is_float) {
            alu_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fsub_s : opcode::fsub_d;
        } else {
            alu_inst->op = opcode::subw;
        }
        break;
    case llvm_ir::Opcode::Mul: 
        if (is_float) {
            alu_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fmul_s : opcode::fmul_d;
        } else {
            alu_inst->op = opcode::mulw;
        }
        break;
    case llvm_ir::Opcode::SDiv: 
        if (is_float) {
            alu_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fdiv_s : opcode::fdiv_d;
        } else {
            alu_inst->op = opcode::divw;
        }
        break;
    case llvm_ir::Opcode::URem: alu_inst->op = opcode::remuw; break;  // 无符号余数
    case llvm_ir::Opcode::SRem: alu_inst->op = opcode::remw; break;   // 有符号余数
    case llvm_ir::Opcode::Shl: alu_inst->op = opcode::sllw; break;  // 32位左移
    case llvm_ir::Opcode::LShr: alu_inst->op = opcode::srlw; break; // 32位逻辑右移
    case llvm_ir::Opcode::AShr: alu_inst->op = opcode::sraw; break; // 32位算术右移
    case llvm_ir::Opcode::FAdd: alu_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fadd_s : opcode::fadd_d; break;
    case llvm_ir::Opcode::FSub: alu_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fsub_s : opcode::fsub_d; break;
    case llvm_ir::Opcode::FMul: alu_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fmul_s : opcode::fmul_d; break;
    case llvm_ir::Opcode::FDiv: alu_inst->op = (bin_op->type == llvm_ir::Type::Float) ? opcode::fdiv_s : opcode::fdiv_d; break;
    default: alu_inst->op = opcode::addw; break;
    }
    alu_inst->rd = rd;
    alu_inst->rs1 = rs1;
    alu_inst->rs2 = rs2;
    alu_inst->imm = 0;
    insts.push_back(std::move(alu_inst));

    // 释放临时寄存器

    // 检查结果是否需要store到栈（溢出变量）
    add_spilled_store_if_needed(insts, bin_op, rd, func);

    return insts;
}

reg riscvFunction::get_param_reg_value(llvm_ir::Value* param, std::vector<std::unique_ptr<riscvInstr>>& load_insts){
    // 这个函数用于从caller-saved的内存位置加载参数寄存器的值到临时寄存器
    // 当参数被分配到a寄存器或fa寄存器中，但在函数调用前已被保存到栈中时使用
    
    if (!param) {
        std::cout << "[ParamReg] ERROR: null parameter" << std::endl;
        abort();
        return riscv::NONE;
    }
    
    // 查找参数的分配信息
    auto it = reg_alloc_result.reg_alloc_map.find(param);
    if (it == reg_alloc_result.reg_alloc_map.end()) {
        std::cout << "[ParamReg] ERROR: 参数 " << param->name << " 没有分配信息" << std::endl;
        abort();
        return riscv::NONE;
    }
    
    const auto& alloc = it->second;
    
    // 确定参数类型，分配合适的临时寄存器
    bool is_float = (param->type == llvm_ir::Type::Float || param->type == llvm_ir::Type::Double);
    reg temp_reg = get_t_reg(is_float);
    
    if (temp_reg == riscv::NONE) {
        std::cout << "[ParamReg] ERROR: 无法获取临时寄存器给参数 " << param->name << std::endl;
        abort();
        return riscv::NONE;
    }
    
    // 从caller-saved的栈位置加载到临时寄存器
    int stack_offset = alloc.stack_offset;
    
    // 根据参数类型选择合适的加载指令
    opcode load_op;
    if (param->type == llvm_ir::Type::Float) {
        load_op = opcode::flw;  // 单精度浮点加载
    } else if (param->type == llvm_ir::Type::Double) {
        load_op = opcode::fld;  // 双精度浮点加载
    } else if (param->type == llvm_ir::Type::I32 || param->type == llvm_ir::Type::I1 || param->type == llvm_ir::Type::I8) {
        load_op = opcode::lw;   // 32位整数加载
    } else if (param->type == llvm_ir::Type::Ptr) {
        load_op = opcode::ld;   // 64位整数/指针加载
    } else {
        load_op = opcode::lw;   // 默认32位加载
    }
    
    // 使用安全的load指令生成，处理可能的偏移超出范围问题
    generate_safe_load(load_insts, load_op, temp_reg, riscv::sp, stack_offset, this);
    
    std::cout << "[ParamReg] 从caller-saved内存位置加载参数 " << param->name 
              << " (stack_offset: " << stack_offset << ") 到临时寄存器 " 
              << riscv::to_string(temp_reg) << std::endl;
    
    return temp_reg;
}

// Call指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_call(llvm_ir::Instruction* inst) {
    llvm_ir::CallInst* call_inst = dynamic_cast<llvm_ir::CallInst*>(inst);
    std::vector<std::unique_ptr<riscvInstr>> insts;

    std::cout << "[CodeGen] Processing call instruction: " << call_inst->functionName << " with " << call_inst->operands.size() << " operands" << std::endl;

    // 在调用前保存caller-saved寄存器
    current_riscv_function->save_caller_saved_regs(insts);
    current_riscv_function->init_temp_reg_pool();
    

    // 准备参数 - operands包含所有参数，functionName包含函数名
    int param_count = call_inst->operands.size();
    std::cout << "[CodeGen] Function has " << param_count << " parameters" << std::endl;

    // 查找函数声明以获取参数类型信息
    llvm_ir::Function* func_decl = find_function_declaration(call_inst->functionName);
    std::cout << "[CodeGen] Function declaration found: " << (func_decl ? "yes" : "no") << std::endl;

    // === 第一步：先将所有参数存储到callee的栈顶位置（保存原始值）===
    // 计算callee的栈帧大小，以确定参数在栈中的偏移位置
    
    for (int param_idx = 0; param_idx < param_count; param_idx++) {
        current_riscv_function->init_temp_reg_pool();

        auto* arg_value = call_inst->operands[param_idx];
        if (!arg_value) {
            std::cout << "[CodeGen] WARNING: null parameter at index " << param_idx << std::endl;
            continue;
        }

        std::cout << "[CodeGen] Processing parameter " << param_idx << ": arg_type=" << static_cast<int>(arg_value->type) << std::endl;

        // === 计算参数在callee栈顶的偏移位置 ===
        // 参数从栈顶向下存放，第0个参数在sp-8，第1个参数在sp-16，以此类推
        int callee_stack_offset = -(param_idx + 1) * 8;
        bool if_valid_stack_offset = is_valid_immediate(callee_stack_offset);
        std::cout << "[CodeGen] Parameter " << param_idx << " -> callee stack offset " << callee_stack_offset << std::endl;

        // === 处理实参值 ===
        reg source_reg = riscv::NONE;
        
        // 处理常数 - 所有参数都存储到callee栈顶
        if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(arg_value)) {
            // 整数常量：先加载到临时寄存器，再存储到callee栈顶
            source_reg = current_riscv_function->get_t_reg(false);
            generate_optimized_immediate_load(insts, source_reg, const_int->value);
            
            // 存储到callee栈顶位置 - 使用安全存储函数
            opcode store_op;
            if (arg_value->type == llvm_ir::Type::Float || arg_value->type == llvm_ir::Type::Double) {
                store_op = opcode::fsd;  // 浮点数统一使用双精度存储(8字节)
            } else {
                store_op = opcode::sd;   // 整数/指针统一使用64位存储(8字节)
            }
            
            generate_safe_store(insts, store_op, source_reg, riscv::sp, callee_stack_offset, current_riscv_function);
            
        } else if (auto* const_float = dynamic_cast<llvm_ir::ConstantFloat*>(arg_value)) {
            // 浮点常量：先转换并加载到临时寄存器，再存储到callee栈顶
            reg temp_int_reg = current_riscv_function->get_t_reg(false);
            reg final_float_reg = current_riscv_function->get_t_reg(true);
            
            if (const_float->type == llvm_ir::Type::Float) {
                // 单精度浮点常量
                int32_t ieee754_int = float_to_ieee754_int(const_float->value);
                
                if (is_valid_immediate(ieee754_int)) {
                    auto li = std::make_unique<riscvInstr>();
                    li->op = opcode::addi;
                    li->rd = temp_int_reg;
                    li->rs1 = reg::x0;
                    li->rs2 = reg::x0;
                    li->imm = ieee754_int;
                    insts.push_back(std::move(li));
                } else {
                    auto li = std::make_unique<riscvInstr>();
                    li->op = opcode::li;
                    li->rd = temp_int_reg;
                    li->rs1 = reg::x0;
                    li->rs2 = reg::x0;
                    li->imm = ieee754_int;
                    insts.push_back(std::move(li));
                }
                
                auto fmv = std::make_unique<riscvInstr>();
                fmv->op = opcode::fmv_s_x;
                fmv->rd = final_float_reg;
                fmv->rs1 = temp_int_reg;
                fmv->rs2 = reg::x0;
                fmv->imm = 0;
                insts.push_back(std::move(fmv));
                
                // 存储到callee栈顶位置 - 使用安全存储函数
                opcode store_op;
                if (arg_value->type == llvm_ir::Type::Float || arg_value->type == llvm_ir::Type::Double) {
                    store_op = opcode::fsd;  // 浮点数统一使用双精度存储(8字节)
                } else {
                    store_op = opcode::sd;   // 整数/指针统一使用64位存储(8字节)
                }
                
                generate_safe_store(insts, store_op, final_float_reg, riscv::sp, callee_stack_offset, current_riscv_function);
                
            } else if (const_float->type == llvm_ir::Type::Double) {
                // 双精度浮点常量
                int64_t ieee754_long = double_to_ieee754_long(const_float->value);
                int32_t high_bits = (ieee754_long >> 32) & 0xFFFFFFFF;
                int32_t low_bits = ieee754_long & 0xFFFFFFFF;
                
                // 加载高32位
                if (is_valid_immediate(high_bits)) {
                    auto li_high = std::make_unique<riscvInstr>();
                    li_high->op = opcode::addi;
                    li_high->rd = temp_int_reg;
                    li_high->rs1 = reg::x0;
                    li_high->rs2 = reg::x0;
                    li_high->imm = high_bits;
                    insts.push_back(std::move(li_high));
                } else {
                    auto li_high = std::make_unique<riscvInstr>();
                    li_high->op = opcode::li;
                    li_high->rd = temp_int_reg;
                    li_high->rs1 = reg::x0;
                    li_high->rs2 = reg::x0;
                    li_high->imm = high_bits;
                    insts.push_back(std::move(li_high));
                }
                
                // 左移32位
                auto slli_inst = std::make_unique<riscvInstr>();
                slli_inst->op = opcode::slli;
                slli_inst->rd = temp_int_reg;
                slli_inst->rs1 = temp_int_reg;
                slli_inst->rs2 = reg::x0;
                slli_inst->imm = 32;
                insts.push_back(std::move(slli_inst));
                
                // 处理低32位
                if (low_bits != 0) {
                    reg low_temp = current_riscv_function->get_t_reg(false);
                    if (is_valid_immediate(low_bits)) {
                        auto li_low = std::make_unique<riscvInstr>();
                        li_low->op = opcode::addi;
                        li_low->rd = low_temp;
                        li_low->rs1 = reg::x0;
                        li_low->rs2 = reg::x0;
                        li_low->imm = low_bits;
                        insts.push_back(std::move(li_low));
                    } else {
                        auto li_low = std::make_unique<riscvInstr>();
                        li_low->op = opcode::li;
                        li_low->rd = low_temp;
                        li_low->rs1 = reg::x0;
                        li_low->rs2 = reg::x0;
                        li_low->imm = low_bits;
                        insts.push_back(std::move(li_low));
                    }
                    
                    auto add_inst = std::make_unique<riscvInstr>();
                    add_inst->op = opcode::add;
                    add_inst->rd = temp_int_reg;
                    add_inst->rs1 = temp_int_reg;
                    add_inst->rs2 = low_temp;
                    add_inst->imm = 0;
                    insts.push_back(std::move(add_inst));
                }
                
                auto fmv = std::make_unique<riscvInstr>();
                fmv->op = opcode::fmv_d_x;
                fmv->rd = final_float_reg;
                fmv->rs1 = temp_int_reg;
                fmv->rs2 = reg::x0;
                fmv->imm = 0;
                insts.push_back(std::move(fmv));
                
                // 存储到callee栈顶位置 - 使用安全存储函数
                opcode store_op;
                if (arg_value->type == llvm_ir::Type::Float || arg_value->type == llvm_ir::Type::Double) {
                    store_op = opcode::fsd;  // 浮点数统一使用双精度存储(8字节)
                } else {
                    store_op = opcode::sd;   // 整数/指针统一使用64位存储(8字节)
                }
                
                generate_safe_store(insts, store_op, final_float_reg, riscv::sp, callee_stack_offset, current_riscv_function);
            }        } else if (arg_value->name[0] == '@') {
            // 全局变量
            reg temp_addr_reg = current_riscv_function->get_t_reg(false);
            
            // 加载全局变量地址
            auto la_inst = std::make_unique<riscvInstr>();
            la_inst->op = opcode::la;
            la_inst->rd = temp_addr_reg;
            la_inst->rs1 = reg::x0;
            la_inst->rs2 = reg::x0;
            la_inst->imm = 0;
            la_inst->label = arg_value->name.substr(1);  // 去掉 '@' 前缀
            insts.push_back(std::move(la_inst));
            
            reg final_reg = current_riscv_function->get_t_reg(arg_value->type == llvm_ir::Type::Float || arg_value->type == llvm_ir::Type::Double);
            
            // 检查是否需要从地址加载值还是直接使用地址
            if (arg_value->type == llvm_ir::Type::Ptr || arg_value->name.find("_array") != std::string::npos) {
                // 指针或数组：直接使用地址
                auto mv = std::make_unique<riscvInstr>();
                mv->op = opcode::add;
                mv->rd = final_reg;
                mv->rs1 = temp_addr_reg;
                mv->rs2 = reg::x0;
                mv->imm = 0;
                insts.push_back(std::move(mv));
            } else {
                // 标量：从地址加载值
                auto load_inst = std::make_unique<riscvInstr>();
                if (arg_value->type == llvm_ir::Type::Float) {
                    load_inst->op = opcode::flw;
                } else if (arg_value->type == llvm_ir::Type::Double) {
                    load_inst->op = opcode::fld;
                } else if (arg_value->type == llvm_ir::Type::I64) {
                    load_inst->op = opcode::ld;
                } else if (arg_value->type == llvm_ir::Type::Ptr) {
                    load_inst->op = opcode::ld;
                } else {
                    load_inst->op = opcode::lw;
                }
                load_inst->rd = final_reg;
                load_inst->rs1 = temp_addr_reg;
                load_inst->rs2 = reg::x0;
                load_inst->imm = 0;
                insts.push_back(std::move(load_inst));
            }
            
            // 存储到callee栈顶位置 - 使用安全存储函数
            opcode store_op;
            if (arg_value->type == llvm_ir::Type::Float || arg_value->type == llvm_ir::Type::Double) {
                store_op = opcode::fsd;  // 浮点数统一使用双精度存储(8字节)
            } else {
                store_op = opcode::sd;   // 整数/指针统一使用64位存储(8字节)
            }
            
            generate_safe_store(insts, store_op, final_reg, riscv::sp, callee_stack_offset, current_riscv_function);
            
        } else {
            // 变量：从寄存器分配获取值
            std::vector<std::unique_ptr<riscvInstr>> src_load_insts;
            source_reg = current_riscv_function->get_temp_reg_for_use(arg_value, src_load_insts);
            
            // 添加获取实参值的指令
            for (auto&& inst : src_load_insts) {
                insts.push_back(std::move(inst));
            }

            if (source_reg == NONE) {
                std::cout << "[CodeGen] Error: Cannot get source register for argument" << std::endl;
                continue;
            }

            // === 特殊处理：如果实参在a寄存器中，需要从caller保存的内存位置获取值 ===
            auto arg_alloc_it = current_riscv_function->reg_alloc_result.reg_alloc_map.find(arg_value);
            if (arg_alloc_it != current_riscv_function->reg_alloc_result.reg_alloc_map.end()) {
                const auto& arg_alloc = arg_alloc_it->second;
                
                if (arg_alloc.is_reg) {
                    reg allocated_reg = arg_alloc.regid;
                    
                    // 检查实参是否在a寄存器或fa寄存器中（参数寄存器）
                    bool is_in_a_reg = false;
                    for (int i = 0; i < 8; i++) {
                        if (allocated_reg == riscv::a[i] || allocated_reg == riscv::fa[i]) {
                            is_in_a_reg = true;
                            break;
                        }
                    }
                    
                    if (is_in_a_reg) {
                        // 实参在a寄存器中，需要从caller保存的内存位置获取值
                        std::cout << "[CodeGen] Argument in parameter register " << riscv::to_string(allocated_reg) 
                                  << ", loading from caller-saved memory location" << std::endl;
                        
                        // 使用get_param_reg_value从caller-saved内存位置加载值
                        std::vector<std::unique_ptr<riscvInstr>> param_load_insts;
                        reg actual_source_reg = current_riscv_function->get_param_reg_value(arg_value, param_load_insts);
                        
                        // 添加从内存加载参数寄存器值的指令
                        for (auto&& inst : param_load_insts) {
                            insts.push_back(std::move(inst));
                        }
                        
                        // 更新source_reg为从内存加载的实际值
                        source_reg = actual_source_reg;
                        
                        std::cout << "[CodeGen] Updated source register to " << riscv::to_string(source_reg) 
                                  << " (loaded from caller-saved memory)" << std::endl;
                    }
                }
            }

            // 存储到callee栈顶位置
            if(if_valid_stack_offset){   
                auto store_inst = std::make_unique<riscvInstr>();  
                // 根据实参类型选择存储指令 
                if (arg_value->type == llvm_ir::Type::Float) {
                       store_inst->op = opcode::fsw;  
                } 
                else if(arg_value->type == llvm_ir::Type::Double){
                    store_inst->op = opcode::fsd;  // 双精度浮点数使用fsd
                }
                else if(arg_value->type == llvm_ir::Type::I32 || arg_value->type == llvm_ir::Type::I1 || arg_value->type == llvm_ir::Type::I8){
                    store_inst->op = opcode::sw;   
                }
                else{
                    store_inst->op = opcode::sd;   // 整数/指针统一使用64位存储(8字节)
                }
                
                store_inst->rs1 = riscv::sp;
                store_inst->rs2 = source_reg;
                store_inst->imm = callee_stack_offset;
                insts.push_back(std::move(store_inst));
            }
            else{
                auto tmp1 = current_riscv_function->get_t_reg(false);
                auto li = std::make_unique<riscvInstr>();
                li->op = opcode::li;
                li->rd = tmp1;
                li->rs1 = reg::x0;
                li->rs2 = reg::x0;
                li->imm = callee_stack_offset;
                insts.push_back(std::move(li));

                auto tmp2 = current_riscv_function->get_t_reg(false);    
                auto add = std::make_unique<riscvInstr>();
                add->op = opcode::add;
                add->rd = tmp2;
                add->rs1 = riscv::sp;
                add->rs2 = tmp1;
                add->imm = 0;
                insts.push_back(std::move(add));

                auto store_inst = std::make_unique<riscvInstr>();
                // 根据实参类型选择存储指令
                if (arg_value->type == llvm_ir::Type::Float) {
                    store_inst->op = opcode::fsw;  
                } 
                else if(arg_value->type == llvm_ir::Type::Double){
                    store_inst->op = opcode::fsd;  // 双精度浮点数使用fsd
                }
                else if(arg_value->type == llvm_ir::Type::I32 || arg_value->type == llvm_ir::Type::I1 || arg_value->type == llvm_ir::Type::I8){
                    store_inst->op = opcode::sw;   
                }
                else{
                    store_inst->op = opcode::sd;   // 整数/指针统一使用64位存储(8字节)
                }
                
                store_inst->rs1 = tmp2;
                store_inst->rs2 = source_reg;
                store_inst->imm = 0;
                insts.push_back(std::move(store_inst));
            }
        }
    }

    // === 第二步：为callee加载参数到a和fa寄存器 ===
    int int_param_count = 0;
    int float_param_count = 0;
    
    for (int param_idx = 0; param_idx < param_count; param_idx++) {
        current_riscv_function->init_temp_reg_pool();
        auto* arg_value = call_inst->operands[param_idx];
        if (!arg_value) continue;
        
        reg target_reg = riscv::NONE;
        bool is_float_param = (arg_value->type == llvm_ir::Type::Float || arg_value->type == llvm_ir::Type::Double);
        
        if (is_float_param) {
            if (float_param_count < 8) {
                target_reg = riscv::fa[float_param_count];
                float_param_count++;
            }
        } else {
            if (int_param_count < 8) {
                target_reg = riscv::a[int_param_count];
                int_param_count++;
            }
        }
        
        if (target_reg == riscv::NONE) {
            continue; // 超过8个参数，跳过（将在栈上传递）
        }
        
        std::cout << "[CodeGen] Loading parameter " << param_idx << " into " << riscv::to_string(target_reg) << std::endl;
        
        // 获取参数值并加载到目标寄存器
        if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(arg_value)) {
            // 整数常量直接加载
            generate_safe_li(insts, target_reg, const_int->value, current_riscv_function);
        } else if (auto* const_float = dynamic_cast<llvm_ir::ConstantFloat*>(arg_value)) {
            // 浮点常量需要先转换为IEEE754格式
            reg temp_int_reg = current_riscv_function->get_t_reg(false);
            
            if (const_float->type == llvm_ir::Type::Float) {
                int32_t ieee754_int = float_to_ieee754_int(const_float->value);
                generate_safe_li(insts, temp_int_reg, ieee754_int, current_riscv_function);
                
                auto fmv_inst = std::make_unique<riscvInstr>();
                fmv_inst->op = opcode::fmv_s_x;
                fmv_inst->rd = target_reg;
                fmv_inst->rs1 = temp_int_reg;
                fmv_inst->rs2 = reg::x0;
                fmv_inst->imm = 0;
                insts.push_back(std::move(fmv_inst));
            } else if (const_float->type == llvm_ir::Type::Double) {
                int64_t ieee754_long = double_to_ieee754_long(const_float->value);
                // 对于64位立即数，需要分两部分加载
                int32_t high_bits = (ieee754_long >> 32) & 0xFFFFFFFF;
                int32_t low_bits = ieee754_long & 0xFFFFFFFF;
                
                generate_safe_li(insts, temp_int_reg, high_bits, current_riscv_function);
                
                auto slli_inst = std::make_unique<riscvInstr>();
                slli_inst->op = opcode::slli;
                slli_inst->rd = temp_int_reg;
                slli_inst->rs1 = temp_int_reg;
                slli_inst->rs2 = reg::x0;
                slli_inst->imm = 32;
                insts.push_back(std::move(slli_inst));
                
                if (low_bits != 0) {
                    reg low_temp = current_riscv_function->get_t_reg(false);
                    generate_safe_li(insts, low_temp, low_bits, current_riscv_function);
                    
                    auto add_inst = std::make_unique<riscvInstr>();
                    add_inst->op = opcode::add;
                    add_inst->rd = temp_int_reg;
                    add_inst->rs1 = temp_int_reg;
                    add_inst->rs2 = low_temp;
                    add_inst->imm = 0;
                    insts.push_back(std::move(add_inst));
                }
                
                auto fmv_inst = std::make_unique<riscvInstr>();
                fmv_inst->op = opcode::fmv_d_x;
                fmv_inst->rd = target_reg;
                fmv_inst->rs1 = temp_int_reg;
                fmv_inst->rs2 = reg::x0;
                fmv_inst->imm = 0;
                insts.push_back(std::move(fmv_inst));
            }
        } else if (arg_value->name[0] == '@') {
            // 全局变量
            reg temp_addr_reg = current_riscv_function->get_t_reg(false);
            
            auto la_inst = std::make_unique<riscvInstr>();
            la_inst->op = opcode::la;
            la_inst->rd = temp_addr_reg;
            la_inst->rs1 = reg::x0;
            la_inst->rs2 = reg::x0;
            la_inst->imm = 0;
            la_inst->label = arg_value->name.substr(1);  // 去掉 '@' 前缀
            insts.push_back(std::move(la_inst));

            std::cout<<"@全局变量test"<<std::endl;
            
            // 检查是否需要从地址加载值还是直接使用地址
            if (arg_value->type == llvm_ir::Type::Ptr || arg_value->name.find("_array") != std::string::npos) {
                // 指针或数组：直接使用地址
                auto mv_inst = std::make_unique<riscvInstr>();
                mv_inst->op = opcode::add;
                mv_inst->rd = target_reg;
                mv_inst->rs1 = temp_addr_reg;
                mv_inst->rs2 = reg::x0;
                mv_inst->imm = 0;
                insts.push_back(std::move(mv_inst));
            } else {
                // 标量：从地址加载值
                auto load_inst = std::make_unique<riscvInstr>();
                if (arg_value->type == llvm_ir::Type::Float) {
                    load_inst->op = opcode::flw;
                } else if (arg_value->type == llvm_ir::Type::Double) {
                    load_inst->op = opcode::fld;
                } else if (arg_value->type == llvm_ir::Type::I64) {
                    load_inst->op = opcode::ld;
                } else if (arg_value->type == llvm_ir::Type::Ptr) {
                    load_inst->op = opcode::ld;
                } else {
                    load_inst->op = opcode::lw;
                }
                load_inst->rd = target_reg;
                load_inst->rs1 = temp_addr_reg;
                load_inst->rs2 = reg::x0;
                load_inst->imm = 0;
                insts.push_back(std::move(load_inst));
            }
        } else {
            // 变量：从寄存器分配获取值
            std::vector<std::unique_ptr<riscvInstr>> src_load_insts;
            reg source_reg = current_riscv_function->get_temp_reg_for_use(arg_value, src_load_insts);
            
            // 添加获取实参值的指令
            for (auto&& inst : src_load_insts) {
                insts.push_back(std::move(inst));
            }
            
            if (source_reg == riscv::NONE) {
                std::cout << "[CodeGen] Error: Cannot get source register for argument" << std::endl;
                continue;
            }
            
            // === 特殊处理：如果实参在a寄存器中，需要从caller保存的内存位置获取值 ===
            auto arg_alloc_it = current_riscv_function->reg_alloc_result.reg_alloc_map.find(arg_value);
            if (arg_alloc_it != current_riscv_function->reg_alloc_result.reg_alloc_map.end()) {
                const auto& arg_alloc = arg_alloc_it->second;
                
                if (arg_alloc.is_reg) {
                    reg allocated_reg = arg_alloc.regid;
                    
                    // 检查实参是否在a寄存器或fa寄存器中（参数寄存器）
                    bool is_in_a_reg = false;
                    for (int i = 0; i < 8; i++) {
                        if (allocated_reg == riscv::a[i] || allocated_reg == riscv::fa[i]) {
                            is_in_a_reg = true;
                            break;
                        }
                    }
                    
                    if (is_in_a_reg) {
                        // 实参在a寄存器中，需要从caller保存的内存位置获取值
                        std::cout << "[CodeGen] Step2: Argument in parameter register " << riscv::to_string(allocated_reg) 
                                  << ", loading from caller-saved memory location" << std::endl;
                        
                        // 使用get_param_reg_value从caller-saved内存位置加载值
                        std::vector<std::unique_ptr<riscvInstr>> param_load_insts;
                        reg actual_source_reg = current_riscv_function->get_param_reg_value(arg_value, param_load_insts);
                        
                        // 添加从内存加载参数寄存器值的指令
                        for (auto&& inst : param_load_insts) {
                            insts.push_back(std::move(inst));
                        }
                        
                        // 更新source_reg为从内存加载的实际值
                        source_reg = actual_source_reg;
                        
                        std::cout << "[CodeGen] Step2: Updated source register to " << riscv::to_string(source_reg) 
                                  << " (loaded from caller-saved memory)" << std::endl;
                    }
                }
            }
            
            // 直接移动到目标寄存器，不进行类型转换
            auto mv_inst = std::make_unique<riscvInstr>();
            if (is_float_param) {
                mv_inst->op = (arg_value->type == llvm_ir::Type::Float) ? opcode::fmv_s : opcode::fmv_d;
            } else {
                mv_inst->op = opcode::add;
            }
            mv_inst->rd = target_reg;
            mv_inst->rs1 = source_reg;
            mv_inst->rs2 = reg::x0;
            mv_inst->imm = 0;
            insts.push_back(std::move(mv_inst));
        }
    }

    

    // 生成call指令
    auto call = std::make_unique<riscvInstr>();
    call->op = opcode::call;
    call->rd = reg::x0;
    call->rs1 = reg::x0;
    call->rs2 = reg::x0;
    call->imm = 0;

    // 设置函数名标签
    if (!call_inst->functionName.empty()) {
        // 跳过对辅助函数的调用（如global全局变量初始化函数）
        if (call_inst->functionName == "global") {
            std::cout << "[CodeGen] 跳过对辅助函数的调用: " << call_inst->functionName << std::endl;
            return insts;  // 不生成call指令，全局变量初始化将在.data段中直接处理
        }
        call->label = call_inst->functionName;
    } else if (call_inst->operands.size() > 0) {
        // 备用：从操作数中获取函数名
        auto* called_func = call_inst->operands.back();
        if (called_func && !called_func->name.empty()) {
            // 跳过对辅助函数的调用
            if (called_func->name == "global") {
                std::cout << "[CodeGen] 跳过对辅助函数的调用: " << called_func->name << std::endl;
                return insts;  // 不生成call指令
            }
            call->label = called_func->name;
        } else {
            call->label = "unknown_func";
        }
    } else {
        call->label = "unknown_func";
    }

    std::cout << "[CodeGen] Generated call to: " << call->label << std::endl;
    insts.push_back(std::move(call));

    // 处理返回值
    if (call_inst->type != llvm_ir::Type::Void) {
        std::vector<std::unique_ptr<riscvInstr>> post_insts;
        reg ret_reg = current_riscv_function->get_reg_for_def(call_inst, post_insts);  // 返回值是写入操作
        if (ret_reg == NONE) {
            // 如果没有分配寄存器，说明这个返回值不会被使用
            return insts;
        }

        // 添加后续指令（如果有溢出处理）
        for (auto&& inst : post_insts) {
            insts.push_back(std::move(inst));
        }

        auto mv_ret = std::make_unique<riscvInstr>();

        // 根据返回类型选择源寄存器
        if (call_inst->type == llvm_ir::Type::Float || call_inst->type == llvm_ir::Type::Double) {
            // 浮点返回值在fa0，直接使用浮点移动指令
            if (call_inst->type == llvm_ir::Type::Float) {
                mv_ret->op = opcode::fmv_s;  // 单精度浮点移动
            } else {
                mv_ret->op = opcode::fmv_d;  // 双精度浮点移动
            }
            mv_ret->rd = ret_reg;
            mv_ret->rs1 = riscv::fa[0];  // fa0 = f10
            mv_ret->rs2 = reg::x0;  // 对于fmv指令，rs2通常不使用
        } else {
            // 整数返回值在a0 (x10)
            mv_ret->op = opcode::add;
            mv_ret->rd = ret_reg;
            mv_ret->rs1 = riscv::a[0];  // 返回值在a0
            mv_ret->rs2 = reg::x0;
        }
        mv_ret->imm = 0;
        insts.push_back(std::move(mv_ret));
        
        // 检查并生成store指令（如果变量被溢出）
        add_spilled_store_if_needed(insts, call_inst, ret_reg, current_riscv_function);
        
        // 释放返回值寄存器（如果是临时寄存器）
    }

    // 恢复caller-saved寄存器（直接使用类成员变量）
    current_riscv_function->init_temp_reg_pool();
    current_riscv_function->restore_caller_saved_regs(insts);
    

    return insts;
}

// Return指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_ret(llvm_ir::Instruction* inst) {
    llvm_ir::ReturnInst* ret_inst = dynamic_cast<llvm_ir::ReturnInst*>(inst);
    std::vector<std::unique_ptr<riscvInstr>> insts;

    if (!ret_inst) {
        std::cout << "[CodeGen] WARNING: null return instruction" << std::endl;
        return insts;
    }

    // 如果有返回值，移动到正确的返回寄存器
    if (ret_inst->operands.size() > 0 && ret_inst->operands[0]) {
        auto* return_value = ret_inst->operands[0];

        // 检查是否是常数
        if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(return_value)) {
            // 直接加载常数到a0
            auto li = std::make_unique<riscvInstr>();
            li->op = opcode::li;
            li->rd = riscv::a[0];
            li->rs1 = reg::x0;
            li->rs2 = reg::x0;
            li->imm = const_int->value;
            std::cout << "[CodeGen] Loading constant " << const_int->value << " to a0" << std::endl;
            insts.push_back(std::move(li));
        } else if (auto* const_float = dynamic_cast<llvm_ir::ConstantFloat*>(return_value)) {
            // 浮点常数需要加载到fa0
            if (const_float->type == llvm_ir::Type::Float) {
                // 单精度浮点常量
                int32_t ieee754_int = float_to_ieee754_int(const_float->value);
                
                // 先加载到整数寄存器然后移动到浮点寄存器
                reg temp_reg = current_riscv_function->get_t_reg(false);  // 用整数寄存器存位模式
                
                if (is_valid_immediate(ieee754_int)) {
                    auto li = std::make_unique<riscvInstr>();
                    li->op = opcode::addi;
                    li->rd = temp_reg;
                    li->rs1 = reg::x0;
                    li->rs2 = reg::x0;
                    li->imm = ieee754_int;
                    insts.push_back(std::move(li));
                } else {
                    auto li = std::make_unique<riscvInstr>();
                    li->op = opcode::li;
                    li->rd = temp_reg;
                    li->rs1 = reg::x0;
                    li->rs2 = reg::x0;
                    li->imm = ieee754_int;
                    insts.push_back(std::move(li));
                }

                auto fmv = std::make_unique<riscvInstr>();
                fmv->op = opcode::fmv_s_x;  // 整数到浮点寄存器的位模式移动
                fmv->rd = riscv::fa[0];     // fa0
                fmv->rs1 = temp_reg;
                fmv->rs2 = reg::x0;
                fmv->imm = 0;
                insts.push_back(std::move(fmv));
                
            } else if (const_float->type == llvm_ir::Type::Double) {
                // 双精度浮点常量
                int64_t ieee754_long = double_to_ieee754_long(const_float->value);
                
                // 先加载到整数寄存器然后移动到浮点寄存器
                reg temp_reg = current_riscv_function->get_t_reg(false);  // 用整数寄存器存位模式
                
                // 对于64位立即数，需要分两部分加载
                int32_t high_bits = (ieee754_long >> 32) & 0xFFFFFFFF;
                int32_t low_bits = ieee754_long & 0xFFFFFFFF;
                
                // 加载高32位
                if (is_valid_immediate(high_bits)) {
                    auto li_high = std::make_unique<riscvInstr>();
                    li_high->op = opcode::addi;
                    li_high->rd = temp_reg;
                    li_high->rs1 = reg::x0;
                    li_high->rs2 = reg::x0;
                    li_high->imm = high_bits;
                    insts.push_back(std::move(li_high));
                } else {
                    auto li_high = std::make_unique<riscvInstr>();
                    li_high->op = opcode::li;
                    li_high->rd = temp_reg;
                    li_high->rs1 = reg::x0;
                    li_high->rs2 = reg::x0;
                    li_high->imm = high_bits;
                    insts.push_back(std::move(li_high));
                }
                
                // 左移32位
                auto slli_inst = std::make_unique<riscvInstr>();
                slli_inst->op = opcode::slli;
                slli_inst->rd = temp_reg;
                slli_inst->rs1 = temp_reg;
                slli_inst->rs2 = reg::x0;
                slli_inst->imm = 32;
                insts.push_back(std::move(slli_inst));
                
                // 如果低32位非零，需要加上低32位
                if (low_bits != 0) {
                    reg low_temp = current_riscv_function->get_t_reg(false);
                    if (is_valid_immediate(low_bits)) {
                        auto li_low = std::make_unique<riscvInstr>();
                        li_low->op = opcode::addi;
                        li_low->rd = low_temp;
                        li_low->rs1 = reg::x0;
                        li_low->rs2 = reg::x0;
                        li_low->imm = low_bits;
                        insts.push_back(std::move(li_low));
                    } else {
                        auto li_low = std::make_unique<riscvInstr>();
                        li_low->op = opcode::li;
                        li_low->rd = low_temp;
                        li_low->rs1 = reg::x0;
                        li_low->rs2 = reg::x0;
                        li_low->imm = low_bits;
                        insts.push_back(std::move(li_low));
                    }
                    
                    auto add_inst = std::make_unique<riscvInstr>();
                    add_inst->op = opcode::add;
                    add_inst->rd = temp_reg;
                    add_inst->rs1 = temp_reg;
                    add_inst->rs2 = low_temp;
                    add_inst->imm = 0;
                    insts.push_back(std::move(add_inst));
                }

                auto fmv = std::make_unique<riscvInstr>();
                fmv->op = opcode::fmv_d_x;  // 整数到双精度浮点寄存器的位模式移动
                fmv->rd = riscv::fa[0];     // fa0
                fmv->rs1 = temp_reg;
                fmv->rs2 = reg::x0;
                fmv->imm = 0;
                insts.push_back(std::move(fmv));
            }
            
            // 释放临时寄存器
        } else {
            // 从寄存器移动值
            std::vector<std::unique_ptr<riscvInstr>> src_load_insts;
            reg src_reg = current_riscv_function->get_temp_reg_for_use(return_value, src_load_insts);
            
            // 添加src的load指令
            for (auto&& inst : src_load_insts) {
                insts.push_back(std::move(inst));
            }

            if (src_reg == NONE) {
                // 如果是常量，直接处理（这种情况应该在前面被捕获，但作为备用）
                if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(return_value)) {
                    auto li = std::make_unique<riscvInstr>();
                    li->op = opcode::li;
                    li->rd = riscv::a[0];
                    li->rs1 = reg::x0;
                    li->rs2 = reg::x0;
                    li->imm = const_int->value;
                    insts.push_back(std::move(li));
                }
            } else {
                auto mv = std::make_unique<riscvInstr>();

                // 判断返回值类型
                if (return_value->type == llvm_ir::Type::Float) {
                    // 浮点返回值移动到fa0
                    mv->op = opcode::fmv_s;   // 使用浮点移动指令
                    mv->rd = riscv::fa[0];    // fa0
                    mv->rs1 = src_reg;
                    mv->rs2 = reg::x0;
                } else if (return_value->type == llvm_ir::Type::Double) {
                    // 双精度浮点返回值移动到fa0
                    mv->op = opcode::fmv_d;   // 使用双精度浮点移动指令
                    mv->rd = riscv::fa[0];    // fa0
                    mv->rs1 = src_reg;
                    mv->rs2 = reg::x0;
                } else {
                    // 整数返回值移动到a0
                    mv->op = opcode::add;  // mv用add实现
                    mv->rd = riscv::a[0];
                    mv->rs1 = src_reg;
                    mv->rs2 = reg::x0;
                }
                mv->imm = 0;
                std::cout << "[CodeGen] Moving register " << riscv::to_string(src_reg) << " to a0" << std::endl;
                insts.push_back(std::move(mv));
                
                // 释放可能使用的临时寄存器
            }
        }
    }

    // 返回指令
    auto ret_instr = std::make_unique<riscvInstr>();
    ret_instr->op = opcode::ret;
    ret_instr->rd = reg::x0;
    ret_instr->rs1 = riscv::ra;
    ret_instr->rs2 = reg::x0;
    ret_instr->imm = 0;
    insts.push_back(std::move(ret_instr));

    return insts;
}

// Branch指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_br(llvm_ir::Instruction* inst) {
    llvm_ir::BranchInst* br_inst = dynamic_cast<llvm_ir::BranchInst*>(inst);
    std::vector<std::unique_ptr<riscvInstr>> insts;

    if (br_inst->isConditional()) {
        std::cout << "[1  CodeGen] Processing conditional branch instruction" << br_inst->toString() << std::endl;
        // 条件分支
        std::vector<std::unique_ptr<riscvInstr>> cond_load_insts;
        reg cond_reg = current_riscv_function->get_temp_reg_for_use(br_inst->operands[0], cond_load_insts);
        
        // 添加条件寄存器的load指令
        for (auto&& inst : cond_load_insts) {
            insts.push_back(std::move(inst));
        }
        // std::cout << 
        if (cond_reg == riscv::NONE) {
            assert(false && "fuck your mother");
        }

        // 条件分支 - 如果条件为真则跳转
        auto beq = std::make_unique<riscvInstr>();
        beq->op = opcode::bne;  // 如果不为零则跳转到true_label
        beq->rd = reg::x0;
        beq->rs1 = cond_reg;
        beq->rs2 = riscv::zero;
        beq->imm = 0;                     // 偏移量由链接时确定
        beq->label = br_inst->trueLabel;  // 跳转到true_label
        insts.push_back(std::move(beq));

        // 无条件跳转到false_label
        auto j = std::make_unique<riscvInstr>();
        j->op = opcode::jal;
        j->rd = reg::x0;
        j->rs1 = reg::x0;
        j->rs2 = reg::x0;
        j->imm = 0;                      // 偏移量由链接时确定
        j->label = br_inst->falseLabel;  // 跳转到false_label
        insts.push_back(std::move(j));
        
        // 释放可能使用的临时寄存器
        if (current_riscv_function) {
        }
    } else {
        // 无条件分支
        std::cout << "[2  CodeGen] Processing unconditional branch instruction: " << br_inst->toString() << std::endl;
        auto j = std::make_unique<riscvInstr>();
        j->op = opcode::jal;
        j->rd = reg::x0;
        j->rs1 = reg::x0;
        j->rs2 = reg::x0;
        j->imm = 0;                     // 偏移量由链接时确定
        j->label = br_inst->trueLabel;  // 跳转到true_label
        std::cout << "[CodeGen] Generated unconditional jump to label: " << j->toString() << std::endl;
        insts.push_back(std::move(j));
    }

    return insts;
}

// Store指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_store(llvm_ir::StoreInst* store_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;
    riscvFunction* func = current_riscv_function;

    reg rs2;  // value register
    reg rs1;  // addr register

    // 处理要存储的值（第一个操作数）
    if (auto* const_int = dynamic_cast<llvm_ir::ConstantInt*>(store_inst->operands[0])) {
        // 整数常量值：需要先加载到寄存器
        bool is_float = (store_inst->operands[0]->type == llvm_ir::Type::Float || store_inst->operands[0]->type == llvm_ir::Type::Double);
        rs2 = func->get_t_reg(is_float);

        // 生成常量加载指令
        if (is_valid_immediate(const_int->value)) {
            // 立即数在范围内，使用addi
            auto li = std::make_unique<riscvInstr>();
            li->op = opcode::addi;
            li->rd = rs2;
            li->rs1 = reg::x0;
            li->rs2 = reg::x0;
            li->imm = const_int->value;
            insts.push_back(std::move(li));
        } else {
            // 立即数超出范围，使用li
            auto li = std::make_unique<riscvInstr>();
            li->op = opcode::li;
            li->rd = rs2;
            li->rs1 = reg::x0;
            li->rs2 = reg::x0;
            li->imm = const_int->value;
            insts.push_back(std::move(li));
        }
    } else if (auto* const_float = dynamic_cast<llvm_ir::ConstantFloat*>(store_inst->operands[0])) {
        // 浮点常量值：需要先加载到浮点寄存器
        rs2 = func->get_t_reg(true);  // 获取浮点寄存器

        if (const_float->type == llvm_ir::Type::Float) {
            // 单精度浮点常量
            int32_t ieee754_int = float_to_ieee754_int(const_float->value);
            
            // 先将IEEE754整数加载到整数寄存器
            reg temp_int_reg = func->get_t_reg(false);
            if (is_valid_immediate(ieee754_int)) {
                auto li = std::make_unique<riscvInstr>();
                li->op = opcode::addi;
                li->rd = temp_int_reg;
                li->rs1 = reg::x0;
                li->rs2 = reg::x0;
                li->imm = ieee754_int;
                insts.push_back(std::move(li));
            } else {
                auto li = std::make_unique<riscvInstr>();
                li->op = opcode::li;
                li->rd = temp_int_reg;
                li->rs1 = reg::x0;
                li->rs2 = reg::x0;
                li->imm = ieee754_int;
                insts.push_back(std::move(li));
            }
            
            // 然后使用fmv.s.x将整数寄存器的值移动到浮点寄存器
            auto fmv = std::make_unique<riscvInstr>();
            fmv->op = opcode::fmv_s_x;
            fmv->rd = rs2;
            fmv->rs1 = temp_int_reg;
            fmv->rs2 = reg::x0;
            fmv->imm = 0;
            insts.push_back(std::move(fmv));
            
        } else if (const_float->type == llvm_ir::Type::Double) {
            // 双精度浮点常量
            int64_t ieee754_long = double_to_ieee754_long(const_float->value);
            
            // 先将IEEE754长整数加载到整数寄存器
            reg temp_int_reg = func->get_t_reg(false);
            
            // 对于64位立即数，需要分两部分加载
            int32_t high_bits = (ieee754_long >> 32) & 0xFFFFFFFF;
            int32_t low_bits = ieee754_long & 0xFFFFFFFF;
            
            // 加载高32位
            if (is_valid_immediate(high_bits)) {
                auto li_high = std::make_unique<riscvInstr>();
                li_high->op = opcode::addi;
                li_high->rd = temp_int_reg;
                li_high->rs1 = reg::x0;
                li_high->rs2 = reg::x0;
                li_high->imm = high_bits;
                insts.push_back(std::move(li_high));
            } else {
                auto li_high = std::make_unique<riscvInstr>();
                li_high->op = opcode::li;
                li_high->rd = temp_int_reg;
                li_high->rs1 = reg::x0;
                li_high->rs2 = reg::x0;
                li_high->imm = high_bits;
                insts.push_back(std::move(li_high));
            }
            
            // 左移32位
            auto slli_inst = std::make_unique<riscvInstr>();
            slli_inst->op = opcode::slli;
            slli_inst->rd = temp_int_reg;
            slli_inst->rs1 = temp_int_reg;
            slli_inst->rs2 = reg::x0;
            slli_inst->imm = 32;
            insts.push_back(std::move(slli_inst));
            
            // 如果低32位非零，需要加上低32位
            if (low_bits != 0) {
                reg low_temp = func->get_t_reg(false);
                if (is_valid_immediate(low_bits)) {
                    auto li_low = std::make_unique<riscvInstr>();
                    li_low->op = opcode::addi;
                    li_low->rd = low_temp;
                    li_low->rs1 = reg::x0;
                    li_low->rs2 = reg::x0;
                    li_low->imm = low_bits;
                    insts.push_back(std::move(li_low));
                } else {
                    auto li_low = std::make_unique<riscvInstr>();
                    li_low->op = opcode::li;
                    li_low->rd = low_temp;
                    li_low->rs1 = reg::x0;
                    li_low->rs2 = reg::x0;
                    li_low->imm = low_bits;
                    insts.push_back(std::move(li_low));
                }
                
                auto add_inst = std::make_unique<riscvInstr>();
                add_inst->op = opcode::add;
                add_inst->rd = temp_int_reg;
                add_inst->rs1 = temp_int_reg;
                add_inst->rs2 = low_temp;
                add_inst->imm = 0;
                insts.push_back(std::move(add_inst));
            }
            
            // 然后使用fmv.d.x将整数寄存器的值移动到浮点寄存器
            auto fmv = std::make_unique<riscvInstr>();
            fmv->op = opcode::fmv_d_x;
            fmv->rd = rs2;
            fmv->rs1 = temp_int_reg;
            fmv->rs2 = reg::x0;
            fmv->imm = 0;
            insts.push_back(std::move(fmv));
        }
    } else {
        // 非常量：从寄存器分配器获取
        std::vector<std::unique_ptr<riscvInstr>> rs2_load_insts;
        rs2 = func->get_temp_reg_for_use(store_inst->operands[0], rs2_load_insts);
        
        // 添加rs2的load指令
        for (auto&& inst : rs2_load_insts) {
            insts.push_back(std::move(inst));
        }
        
        // 如果值操作数溢出到栈，需要先从栈加载
        if (rs2 == NONE && func->is_spilled(store_inst->operands[0])) {
            bool is_float = (store_inst->operands[0]->type == llvm_ir::Type::Float || store_inst->operands[0]->type == llvm_ir::Type::Double);
            rs2 = func->get_t_reg(is_float);
            auto load_insts = func->gen_load_from_stack(store_inst->operands[0], rs2);
            for (auto& load_inst : load_insts) {
                insts.push_back(std::move(load_inst));
            }
            std::cout << "[Store] 从栈加载值操作数到 " << riscv::to_string(rs2) << std::endl;
        } else if (rs2 == NONE) {
            rs2 = reg::x12;  // 默认寄存器
        }
    }

    // 处理目标地址（第二个操作数）
    if (store_inst->operands[1]->name[0] == '@') {
        // 全局变量：首先加载地址到临时寄存器
        rs1 = func->get_t_reg(false);  // 使用临时寄存器作为地址寄存器
        auto la_inst = std::make_unique<riscvInstr>();
        la_inst->op = opcode::la;
        la_inst->rd = rs1;
        la_inst->label = store_inst->operands[1]->name.substr(1);  // 移除@前缀
        insts.push_back(std::move(la_inst));
    } else {
        // 局部变量或其他情况：从寄存器分配器获取地址
        std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
        rs1 = func->get_temp_reg_for_use(store_inst->operands[1], rs1_load_insts);
        
        // 添加rs1的load指令
        for (auto&& inst : rs1_load_insts) {
            insts.push_back(std::move(inst));
        }
        
        // 如果地址操作数溢出到栈，需要先从栈加载地址
        if (rs1 == NONE && func->is_spilled(store_inst->operands[1])) {
            rs1 = func->get_t_reg(false);  // 地址总是整数类型
            auto addr_load_insts = func->gen_load_from_stack(store_inst->operands[1], rs1);
            for (auto& addr_load : addr_load_insts) {
                insts.push_back(std::move(addr_load));
            }
            std::cout << "[Store] 从栈加载地址操作数到 " << riscv::to_string(rs1) << std::endl;
        } else if (rs1 == NONE) {
            rs1 = reg::x11;  // 默认寄存器
        }
    }

    auto store = std::make_unique<riscvInstr>();
    // 类型判断决定sw/sd/fsw等
    llvm_ir::Type valType = store_inst->operands[0]->type;
    if (valType == llvm_ir::Type::Float)
        store->op = opcode::fsw;
    else if (valType == llvm_ir::Type::Double)
        store->op = opcode::fsd;
    else if (valType == llvm_ir::Type::I64)
        store->op = opcode::sd ;
    else if (valType == llvm_ir::Type::Ptr ){
        store->op = opcode::sd ;
    } else
        store->op = opcode::sw;
    store->rs1 = rs1;
    store->rs2 = rs2;
    store->imm = 0;
    insts.push_back(std::move(store));
    
    // 释放临时寄存器
    
    return insts;
}

// Load指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_load(llvm_ir::LoadInst* load_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;
    riscvFunction* func = current_riscv_function;
    
    // 获取目标寄存器
    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    reg rd = func->get_reg_for_def(load_inst, post_insts);  // load的目标寄存器
    
    // 将后续指令加入指令序列
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }

    // 检查是否是全局变量访问
    if (load_inst->operands[0]->name[0] == '@') {
        // 全局变量：首先加载地址，然后从地址读取
        auto la_inst = std::make_unique<riscvInstr>();
        la_inst->op = opcode::la;
        la_inst->rd = func->get_t_reg(false);
        la_inst->label = load_inst->operands[0]->name.substr(1);  // 移除@前缀

        reg tmp = la_inst->rd;
        insts.push_back(std::move(la_inst));

        // 然后从该地址读取值
        auto load = std::make_unique<riscvInstr>();
        llvm_ir::Type valType = load_inst->type;
        if (valType == llvm_ir::Type::Float)
            load->op = opcode::flw;
        else if (valType == llvm_ir::Type::Double)
            load->op = opcode::fld;
        else if (valType == llvm_ir::Type::I64)
            load->op = opcode::ld;
        else if (valType == llvm_ir::Type::Ptr)
            load->op = opcode::ld;  // 假设指针类型也是lw
        else
            load->op = opcode::lw;
        load->rd = rd;
        load->rs1 = tmp;  // 从刚才加载的地址读取
        load->imm = 0;
        insts.push_back(std::move(load));
        
    } else {
        // 局部变量或其他情况
        std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
        reg rs1 = func->get_temp_reg_for_use(load_inst->operands[0], rs1_load_insts);
        
        // 添加rs1的load指令
        for (auto&& inst : rs1_load_insts) {
            insts.push_back(std::move(inst));
        }
        
        // 如果地址操作数溢出到栈，需要先加载地址
        if (rs1 == NONE && func->is_spilled(load_inst->operands[0])) {
            // 地址操作数溢出，从栈加载到临时寄存器
            rs1 = func->get_t_reg(false);  // 地址总是整数类型
            auto addr_load_insts = func->gen_load_from_stack(load_inst->operands[0], rs1);
            for (auto& addr_load : addr_load_insts) {
                insts.push_back(std::move(addr_load));
            }
            std::cout << "[Load] 从栈加载地址操作数到 " << riscv::to_string(rs1) << std::endl;
        } else if (rs1 == NONE) {
            rs1 = reg::x11;  // 默认寄存器
        }
        
        auto load = std::make_unique<riscvInstr>();
        llvm_ir::Type valType = load_inst->type;
        if (valType == llvm_ir::Type::Float)
            load->op = opcode::flw;
        else if (valType == llvm_ir::Type::Double)
            load->op = opcode::fld;
        else if (valType == llvm_ir::Type::I64)
            load->op = opcode::ld;
        else if (valType == llvm_ir::Type::Ptr)
            load->op = opcode::ld;
        else
            load->op = opcode::lw;
        load->rd = rd;
        load->rs1 = rs1;
        load->imm = 0;
        insts.push_back(std::move(load));
        
        // 释放地址操作数的临时寄存器
    }
    
    // 检查结果是否需要store到栈（溢出变量）  
    add_spilled_store_if_needed(insts, load_inst, rd, func);
    
    return insts;
}

// ICmp指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_icmp(llvm_ir::ICmpInst* icmp_inst) {
    std::cout << "[ICMP] Processing ICmp instruction: " << icmp_inst->toString() << std::endl;
    std::vector<std::unique_ptr<riscvInstr>> insts;
    riscvFunction* func = current_riscv_function;

    // 获取目标寄存器
    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    reg rd = func->get_reg_for_def(icmp_inst, post_insts);  // rd是目标寄存器
    std::cout << "[ICMP]   Target register for result: " << riscv::to_string(rd) << std::endl;
    
    // 将后续指令加入指令序列
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }

    std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
    reg rs1 = func->get_temp_reg_for_use(icmp_inst->operands[0], rs1_load_insts);  // rs1是源操作数
    std::cout << "[ICMP]   Source register rs1: " << riscv::to_string(rs1) << std::endl;
    
    // 添加rs1的load指令
    for (auto&& inst : rs1_load_insts) {
        insts.push_back(std::move(inst));
    }
    
    std::vector<std::unique_ptr<riscvInstr>> rs2_load_insts;
    reg rs2 = func->get_temp_reg_for_use(icmp_inst->operands[1], rs2_load_insts);  // rs2是源操作数
    std::cout << "[ICMP]   Source register rs2: " << riscv::to_string(rs2) << std::endl;
    
    // 添加rs2的load指令
    for (auto&& inst : rs2_load_insts) {
        insts.push_back(std::move(inst));
    }

    auto cmp_inst = std::make_unique<riscvInstr>();

    // 根据比较类型选择指令
    switch (icmp_inst->condition) {
    case llvm_ir::ICmpCond::EQ: {
        // 相等：xor然后seqz
        cmp_inst->op = opcode::xor_;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));

        // seqz设置结果
        auto seqz = std::make_unique<riscvInstr>();
        seqz->op = opcode::seqz;
        seqz->rd = rd;
        seqz->rs1 = rd;
        seqz->rs2 = reg::x0;
        seqz->imm = 0;
        insts.push_back(std::move(seqz));
        break;
    }

    case llvm_ir::ICmpCond::NE: {
        // 不等：xor然后snez
        cmp_inst->op = opcode::xor_;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));

        auto snez = std::make_unique<riscvInstr>();
        snez->op = opcode::snez;
        snez->rd = rd;
        snez->rs1 = rd;
        snez->rs2 = reg::x0;
        snez->imm = 0;
        insts.push_back(std::move(snez));
        break;
    }

    case llvm_ir::ICmpCond::SLT: {
        cmp_inst->op = opcode::slt;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::ICmpCond::SLE: {
        // rs1 <= rs2 等价于 rs2 >= rs1
        cmp_inst->op = opcode::slt;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs2;  // 交换操作数
        cmp_inst->rs2 = rs1;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));

        // 取反
        auto xori = std::make_unique<riscvInstr>();
        xori->op = opcode::xori;
        xori->rd = rd;
        xori->rs1 = rd;
        xori->rs2 = reg::x0;
        xori->imm = 1;
        insts.push_back(std::move(xori));
        break;
    }

    case llvm_ir::ICmpCond::SGT: {
        // rs1 > rs2 等价于 rs2 < rs1
        cmp_inst->op = opcode::slt;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs2;  // 交换操作数
        cmp_inst->rs2 = rs1;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::ICmpCond::SGE: {
        // rs1 >= rs2 等价于 !(rs1 < rs2)
        cmp_inst->op = opcode::slt;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));

        // 取反
        auto xori = std::make_unique<riscvInstr>();
        xori->op = opcode::xori;
        xori->rd = rd;
        xori->rs1 = rd;
        xori->rs2 = reg::x0;
        xori->imm = 1;
        insts.push_back(std::move(xori));
        break;
    }

    case llvm_ir::ICmpCond::ULT: {
        // 无符号小于
        cmp_inst->op = opcode::sltu;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::ICmpCond::ULE: {
        // rs1 <= rs2 (无符号) 等价于 rs2 >= rs1 (无符号)
        cmp_inst->op = opcode::sltu;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs2;  // 交换操作数
        cmp_inst->rs2 = rs1;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));

        // 取反
        auto xori = std::make_unique<riscvInstr>();
        xori->op = opcode::xori;
        xori->rd = rd;
        xori->rs1 = rd;
        xori->rs2 = reg::x0;
        xori->imm = 1;
        insts.push_back(std::move(xori));
        break;
    }

    case llvm_ir::ICmpCond::UGT: {
        // rs1 > rs2 (无符号) 等价于 rs2 < rs1 (无符号)
        cmp_inst->op = opcode::sltu;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs2;  // 交换操作数
        cmp_inst->rs2 = rs1;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::ICmpCond::UGE: {
        // rs1 >= rs2 (无符号) 等价于 !(rs1 < rs2) (无符号)
        cmp_inst->op = opcode::sltu;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));

        // 取反
        auto xori = std::make_unique<riscvInstr>();
        xori->op = opcode::xori;
        xori->rd = rd;
        xori->rs1 = rd;
        xori->rs2 = reg::x0;
        xori->imm = 1;
        insts.push_back(std::move(xori));
        break;
    }

    default: {
        assert(false && "cmp type not support");
        break;
    }
    }

    // 释放临时寄存器

    // 检查结果是否需要store到栈（溢出变量）
    add_spilled_store_if_needed(insts, icmp_inst, rd, func);

    return insts;
}

// FCmp指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_fcmp(llvm_ir::FCmpInst* fcmp_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    riscvFunction* func = current_riscv_function;
    
    // 获取目标寄存器
    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    reg rd = func->get_reg_for_def(fcmp_inst, post_insts);  // 使用get_reg_for_def
    
    // 将后续指令加入指令序列
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }
    
    // 确保结果寄存器是整数寄存器，因为浮点比较返回0/1整数值
    if (rd >= reg::f0 && rd <= reg::f31) {
        // 如果分配的是浮点寄存器，使用对应的整数寄存器
        rd = static_cast<reg>(static_cast<int>(rd) - static_cast<int>(reg::f0) + static_cast<int>(reg::x10));
    }

    std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
    reg rs1 = current_riscv_function ? current_riscv_function->get_temp_reg_for_use(fcmp_inst->operands[0], rs1_load_insts) : reg::f11;  // 源操作数
    
    // 添加rs1的load指令
    for (auto&& inst : rs1_load_insts) {
        insts.push_back(std::move(inst));
    }
    
    std::vector<std::unique_ptr<riscvInstr>> rs2_load_insts;
    reg rs2 = current_riscv_function ? current_riscv_function->get_temp_reg_for_use(fcmp_inst->operands[1], rs2_load_insts) : reg::f12;  // 源操作数
    
    // 添加rs2的load指令
    for (auto&& inst : rs2_load_insts) {
        insts.push_back(std::move(inst));
    }

    auto cmp_inst = std::make_unique<riscvInstr>();

    // 根据比较类型选择浮点比较指令
    switch (fcmp_inst->condition) {
    case llvm_ir::FCmpCond::OEQ: {
        // 有序相等
        cmp_inst->op = opcode::feq_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::FCmpCond::ONE: {
        // 有序不等：先比较相等，然后取反
        cmp_inst->op = opcode::feq_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));

        // 取反
        auto xori = std::make_unique<riscvInstr>();
        xori->op = opcode::xori;
        xori->rd = rd;
        xori->rs1 = rd;
        xori->rs2 = reg::x0;
        xori->imm = 1;
        insts.push_back(std::move(xori));
        break;
    }

    case llvm_ir::FCmpCond::OLT: {
        // 有序小于
        cmp_inst->op = opcode::flt_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::FCmpCond::OLE: {
        // 有序小于等于
        cmp_inst->op = opcode::fle_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::FCmpCond::OGT: {
        // 有序大于：交换操作数使用flt
        cmp_inst->op = opcode::flt_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs2;  // 交换操作数
        cmp_inst->rs2 = rs1;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::FCmpCond::OGE: {
        // 有序大于等于：交换操作数使用fle
        cmp_inst->op = opcode::fle_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs2;  // 交换操作数
        cmp_inst->rs2 = rs1;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::FCmpCond::UEQ: {
        // 无序相等（包括NaN情况）
        // 这需要更复杂的逻辑，简化处理：使用feq_s
        cmp_inst->op = opcode::feq_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::FCmpCond::UNE: {
        // 无序不等（包括NaN情况）
        // 简化处理：先比较相等，然后取反
        cmp_inst->op = opcode::feq_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));

        // 取反
        auto xori = std::make_unique<riscvInstr>();
        xori->op = opcode::xori;
        xori->rd = rd;
        xori->rs1 = rd;
        xori->rs2 = reg::x0;
        xori->imm = 1;
        insts.push_back(std::move(xori));
        break;
    }

    case llvm_ir::FCmpCond::ULT: {
        // 无序小于，简化处理
        cmp_inst->op = opcode::flt_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::FCmpCond::ULE: {
        // 无序小于等于，简化处理
        cmp_inst->op = opcode::fle_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs1;
        cmp_inst->rs2 = rs2;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::FCmpCond::UGT: {
        // 无序大于，简化处理
        cmp_inst->op = opcode::flt_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs2;  // 交换操作数
        cmp_inst->rs2 = rs1;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    case llvm_ir::FCmpCond::UGE: {
        // 无序大于等于，简化处理
        cmp_inst->op = opcode::fle_s;
        cmp_inst->rd = rd;
        cmp_inst->rs1 = rs2;  // 交换操作数
        cmp_inst->rs2 = rs1;
        cmp_inst->imm = 0;
        insts.push_back(std::move(cmp_inst));
        break;
    }

    default: {
        assert(false && "cmp type not support");
        break;
    }
    }

    // 释放可能使用的临时寄存器

    // 检查结果是否需要store到栈（溢出变量）
    add_spilled_store_if_needed(insts, fcmp_inst, rd, func);

    return insts;
}

std::vector<std::unique_ptr<riscvInstr>> gen_alloca(llvm_ir::AllocaInst* alloca_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    int element_count = alloca_inst->arraySize;
    if (element_count == 0) {
        element_count = 1;  // 单个元素
    }

    int element_size;
    switch (alloca_inst->allocatedType) {
    case llvm_ir::Type::I32:
        element_size = 4;  // i32类型每个元素4字节
        std::cout<< "i32\n";
        break;
    case llvm_ir::Type::I64:
        element_size = 8;  // i64类型每个元素8字节
        std::cout<< "i64\n";
        break;
    case llvm_ir::Type::Float:
        element_size = 4;  // float类型每个元素4字节
        std::cout<< "Float\n";
        break;
    case llvm_ir::Type::Double:
        element_size = 8;  // double类型每个元素8字节
        std::cout<< "Double\n";
        break;
    case llvm_ir::Type::Ptr:
        element_size = 8;  // 指针类型每个元素8字节
        std::cout<< "Ptr\n";
        break;
    default: 
        std::cout << "[CodeGen] WARNING: 不支持的alloca类型: " << (int)alloca_inst->allocatedType << std::endl; 
        element_size = 4; // 默认4字节
        std::cout<< "Default\n";
        break;
    }
    
    int total_size = element_count * element_size;

    // 新逻辑：不为alloca指针分配物理寄存器，而是记录到映射表
    riscvFunction* func = current_riscv_function;
    if (!func) return insts;
    
    // 使用成员变量跟踪当前alloca的累积偏移
    func->current_alloca_offset += total_size;

    // 计算栈偏移：正确的地址应该是 sp + (total_stack_size - current_alloca_offset - pre)
    int total_stack_size = func->total_stack_size;
    int stack_offset = total_stack_size - func->current_alloca_offset - (func->Rasize + func->Regs2Save_size + func->SpilledVRegs_size);
    
    // 记录alloca指针到栈偏移的映射，而不是分配物理寄存器
    func->record_alloca_pointer(alloca_inst, stack_offset);

    std::cout << "[CodeGen] " 
              << alloca_inst->name << " -> 静态指针映射到栈偏移 " 
              << stack_offset << " (总大小: " << total_size << "B, 累积偏移: " << func->current_alloca_offset 
              << ", 总栈大小: " << total_stack_size << ")" << std::endl;

    // 返回空指令列表，因为alloca指针不需要生成任何指令
    return insts;
}

// GetElementPtr指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_getelementptr(llvm_ir::GetElementPtrInst* gep_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    if (!gep_inst || gep_inst->operands.size() < 2) {
        std::cout << "[CodeGen] WARNING: GetElementPtr指令操作数不足" << std::endl;
        return insts;
    }

    riscvFunction* func = current_riscv_function;
    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    reg rd = func->get_reg_for_def(gep_inst, post_insts);
    
    // 添加后续指令
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }

    // 获取基地址和索引
    auto* base_ptr = gep_inst->operands[0];
    auto* index = gep_inst->operands[1];

    // 处理不同情况的索引
    if (gep_inst->operands.size() == 3) {
        // 二维情况：getelementptr [8 x i32], ptr %array, i32 0, i32 index
        // 第一个索引通常是0（数组基地址），第二个是实际索引
        index = gep_inst->operands[2];
    }

    // 检查优化条件：base_ptr是alloca的静态指针 或 index是常量
    bool is_base_alloca_ptr = func->is_alloca_pointer(base_ptr);
    bool is_index_constant = dynamic_cast<llvm_ir::ConstantInt*>(index) != nullptr;
    
    if (is_base_alloca_ptr && is_index_constant) {
        // case 1: base_ptr是alloca静态指针 且 index是常量
        // rd = (sp + base_offset) + (index * element_size)
        auto* const_idx = dynamic_cast<llvm_ir::ConstantInt*>(index);
        int base_offset = func->get_alloca_offset(base_ptr);
        int index_offset = const_idx->value * 4;  // 假设i32类型，每个元素4字节
        int total_offset = base_offset + index_offset;
            
        // 直接计算 sp + total_offset
        generate_safe_addi(insts, rd, reg::x2, total_offset, func);
        //return insts;
    }
        
    else{
        reg base_reg;
        // 检查是否为全局数组
        if (base_ptr->name[0] == '@') {
            // 全局变量，需要先加载地址
            std::string global_name = base_ptr->name.substr(1);  // 去掉@符号

            // 使用临时寄存器来存储全局变量地址
            base_reg = func->get_t_reg(false);  // 使用临时寄存器基地址寄存器

            // 生成加载全局变量地址的指令
            // 使用la指令直接加载全局变量地址
            auto la_inst = std::make_unique<riscvInstr>();
            la_inst->op = opcode::la;
            la_inst->rd = base_reg;
            la_inst->label = global_name;
            insts.push_back(std::move(la_inst));

            std::cout << "[CodeGen] GEP全局数组: " << global_name << " 地址加载到 " << to_string(base_reg) << std::endl;
        } else {
            // 局部变量或其他寄存器值
            std::vector<std::unique_ptr<riscvInstr>> base_load_insts;
            base_reg = func->get_temp_reg_for_use(base_ptr, base_load_insts);
            
            // 添加base的load指令
            for (auto&& inst : base_load_insts) {
                insts.push_back(std::move(inst));
            }
        }

        // 检查索引是否为常量
        if (auto* const_idx = dynamic_cast<llvm_ir::ConstantInt*>(index)) {
            // 常量索引：base + offset
            int offset = const_idx->value * 4;  // 假设i32类型，每个元素4字节
            if (offset == 0) {
                // 零偏移，直接复制基地址（如果base_reg != rd）
                if (base_reg != rd) {
                    auto mv_inst = std::make_unique<riscvInstr>();
                    mv_inst->op = opcode::mv;
                    mv_inst->rd = rd;
                    mv_inst->rs1 = base_reg;
                    insts.push_back(std::move(mv_inst));
                }
            } else {
                // 非零偏移，检查立即数范围
                if (offset >= -2048 && offset <= 2047) {
                    // 在范围内，使用addi
                    auto addi_inst = std::make_unique<riscvInstr>();
                    addi_inst->op = opcode::addi;
                    addi_inst->rd = rd;
                    addi_inst->rs1 = base_reg;
                    addi_inst->imm = offset;
                    insts.push_back(std::move(addi_inst));
                } else {
                    // 超出范围，使用li指令加载偏移量
                    riscv::reg tmp = func->get_t_reg();  // 使用临时寄存器

                    // li 加载偏移量
                    auto li_inst = std::make_unique<riscvInstr>();
                    li_inst->op = opcode::li;
                    li_inst->rd = tmp; 
                    li_inst->imm = offset;
                    insts.push_back(std::move(li_inst));

                    // add 基地址 + 偏移
                    auto add_inst = std::make_unique<riscvInstr>();
                    add_inst->op = opcode::add;
                    add_inst->rd = rd;
                    add_inst->rs1 = base_reg;
                    add_inst->rs2 = tmp;
                    insts.push_back(std::move(add_inst));
                }
            }
            std::cout << "[CodeGen] GEP常量索引: base=" << to_string(base_reg) << ", offset=" << offset << " -> " << to_string(rd) << std::endl;
        } else {
            // 变量索引：需要计算 base + index * sizeof(element)
            std::vector<std::unique_ptr<riscvInstr>> idx_load_insts;
            reg idx_reg = func->get_temp_reg_for_use(index, idx_load_insts);
            
            // 添加index的load指令
            for (auto&& inst : idx_load_insts) {
                insts.push_back(std::move(inst));
            }

            // 索引乘以元素大小（4字节）
    #if USE_SH2ADD
            // 使用sh2add指令：rd = rs1 + (rs2 << 2)，一条指令完成索引*4+基地址
            auto sh2add_inst = std::make_unique<riscvInstr>();
            sh2add_inst->op = opcode::sh2add;
            sh2add_inst->rd = rd;
            sh2add_inst->rs1 = base_reg;  // 基地址
            sh2add_inst->rs2 = idx_reg;   // 索引
            insts.push_back(std::move(sh2add_inst));

            std::cout << "[CodeGen] GEP变量索引(sh2add): base=" << to_string(base_reg) << ", index=" << to_string(idx_reg) << " -> " << to_string(rd) << std::endl;
    #else
            // 原始实现：使用slli + add
            auto slli_inst = std::make_unique<riscvInstr>();
            slli_inst->op = opcode::slli;
            slli_inst->rd = func->get_t_reg();  // 使用临时寄存器
            slli_inst->rs1 = idx_reg;
            slli_inst->imm = 2;  // 左移2位 = 乘以4
            reg scaled_idx_reg = slli_inst->rd;  // 确保slli_inst->rd是一个临时寄存器
            insts.push_back(std::move(slli_inst));

            // 基地址加上偏移
            auto add_inst = std::make_unique<riscvInstr>();
            add_inst->op = opcode::add;
            add_inst->rd = rd;
            add_inst->rs1 = base_reg;
            add_inst->rs2 = scaled_idx_reg;
            insts.push_back(std::move(add_inst));

            std::cout << "[CodeGen] GEP变量索引(slli+add): base=" << to_string(base_reg) << ", index=" << to_string(idx_reg) << " -> " << to_string(rd) << std::endl;
    #endif
        }



    }
    // 检查并生成store指令（如果变量被溢出）
    add_spilled_store_if_needed(insts, gep_inst, rd, func);

    return insts;
}

// SIToFP指令生成（有符号整数到浮点数）
std::vector<std::unique_ptr<riscvInstr>> gen_sitofp(llvm_ir::SIToFPInst* cast_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    reg rd = current_riscv_function ? current_riscv_function->get_reg_for_def(cast_inst, post_insts) : reg::f10;
    
    std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
    reg rs1 = current_riscv_function ? current_riscv_function->get_temp_reg_for_use(cast_inst->operands[0], rs1_load_insts) : reg::x10;  // 源操作数
    
    // 添加后续指令
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }
    
    // 添加rs1的load指令
    for (auto&& inst : rs1_load_insts) {
        insts.push_back(std::move(inst));
    }

    auto conv_inst = std::make_unique<riscvInstr>();
    // 根据目标类型选择转换指令
    if (cast_inst->type == llvm_ir::Type::Float) {
        conv_inst->op = opcode::fcvt_s_w;
    } else if (cast_inst->type == llvm_ir::Type::Double) {
        conv_inst->op = opcode::fcvt_d_w;
    } else {
        // 默认单精度浮点
        conv_inst->op = opcode::fcvt_s_w;
    }

    conv_inst->rd = rd;
    conv_inst->rs1 = rs1;
    conv_inst->rs2 = reg::x0;  // 未使用
    conv_inst->imm = 0;
    insts.push_back(std::move(conv_inst));

    // 检查并生成store指令（如果变量被溢出）
    if (current_riscv_function) {
        add_spilled_store_if_needed(insts, cast_inst, rd, current_riscv_function);
    }


    return insts;
}

// FPToSI指令生成（浮点数到有符号整数）
std::vector<std::unique_ptr<riscvInstr>> gen_fptosi(llvm_ir::FPToSIInst* cast_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    reg rd = current_riscv_function ? current_riscv_function->get_reg_for_def(cast_inst, post_insts) : reg::x10;
    
    std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
    reg rs1 = current_riscv_function ? current_riscv_function->get_temp_reg_for_use(cast_inst->operands[0], rs1_load_insts) : reg::f10;  // 源操作数
    
    // 添加后续指令
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }
    
    // 添加rs1的load指令
    for (auto&& inst : rs1_load_insts) {
        insts.push_back(std::move(inst));
    }

    auto conv_inst = std::make_unique<riscvInstr>();
    // 根据源类型选择转换指令
    if (cast_inst->operands[0]->type == llvm_ir::Type::Float) {
        conv_inst->op = opcode::fcvt_w_s;
    } else if (cast_inst->operands[0]->type == llvm_ir::Type::Double) {
        conv_inst->op = opcode::fcvt_w_d;
    } else {
        // 默认从单精度浮点转换
        conv_inst->op = opcode::fcvt_w_s;
    }

    conv_inst->rd = rd;
    conv_inst->rs1 = rs1;
    conv_inst->rs2 = reg::x0;  // 未使用
    conv_inst->imm = 1;  // RTZ (Round Toward Zero) 舍入模式 - 向零舍入，符合C语言标准
    insts.push_back(std::move(conv_inst));

    // 检查并生成store指令（如果变量被溢出）
    if (current_riscv_function) {
        add_spilled_store_if_needed(insts, cast_inst, rd, current_riscv_function);
    }

    return insts;
}

// ZExt指令生成（零扩展）
std::vector<std::unique_ptr<riscvInstr>> gen_zext(llvm_ir::ZExtInst* zext_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    reg rd = current_riscv_function ? current_riscv_function->get_reg_for_def(zext_inst, post_insts) : reg::x10;
    
    std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
    reg rs1 = current_riscv_function ? current_riscv_function->get_temp_reg_for_use(zext_inst->operands[0], rs1_load_insts) : reg::x10;
    
    // 添加后续指令
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }
    
    // 添加rs1的load指令
    for (auto&& inst : rs1_load_insts) {
        insts.push_back(std::move(inst));
    }

    // RISC-V中的零扩展实现
    // 对于i1到i32的零扩展，使用andi指令清除高位
    // 对于其他情况，可能需要组合指令
    
    if (zext_inst->operands[0]->type == llvm_ir::Type::I1 && zext_inst->type == llvm_ir::Type::I32) {
        // i1到i32的零扩展：只保留最低位
        auto zext_inst_riscv = std::make_unique<riscvInstr>();
        zext_inst_riscv->op = opcode::andi;
        zext_inst_riscv->rd = rd;
        zext_inst_riscv->rs1 = rs1;
        zext_inst_riscv->rs2 = reg::x0;  // 未使用
        zext_inst_riscv->imm = 1;        // 掩码：只保留最低位
        insts.push_back(std::move(zext_inst_riscv));
    } else {
        // 其他情况的零扩展，直接移动（RISC-V 32位寄存器天然零扩展）
        auto mv_inst = std::make_unique<riscvInstr>();
        mv_inst->op = opcode::add;
        mv_inst->rd = rd;
        mv_inst->rs1 = rs1;
        mv_inst->rs2 = reg::x0;  // rs1 + 0 = rs1
        mv_inst->imm = 0;
        insts.push_back(std::move(mv_inst));
    }

    // 检查并生成store指令（如果变量被溢出）
    if (current_riscv_function) {
        add_spilled_store_if_needed(insts, zext_inst, rd, current_riscv_function);
    }


    return insts;
}

// SExt指令生成（符号扩展）
std::vector<std::unique_ptr<riscvInstr>> gen_sext(llvm_ir::SExtInst* sext_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;

    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    reg rd = current_riscv_function ? current_riscv_function->get_reg_for_def(sext_inst, post_insts) : reg::x10;
    
    std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
    reg rs1 = current_riscv_function ? current_riscv_function->get_temp_reg_for_use(sext_inst->operands[0], rs1_load_insts) : reg::x10;
    
    // 添加后续指令
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }
    
    // 添加rs1的load指令
    for (auto&& inst : rs1_load_insts) {
        insts.push_back(std::move(inst));
    }

    // RISC-V中的符号扩展实现
    if (sext_inst->operands[0]->type == llvm_ir::Type::I1 && sext_inst->type == llvm_ir::Type::I32) {
        // i1到i32的符号扩展：0 -> 0, 1 -> -1 (0xFFFFFFFF)
        // 使用 sub x0, rs1 指令：0 - rs1，当rs1=0时结果0，当rs1=1时结果-1
        auto sext_inst_riscv = std::make_unique<riscvInstr>();
        sext_inst_riscv->op = opcode::sub;
        sext_inst_riscv->rd = rd;
        sext_inst_riscv->rs1 = reg::x0;  // 0 - rs1
        sext_inst_riscv->rs2 = rs1;
        sext_inst_riscv->imm = 0;
        insts.push_back(std::move(sext_inst_riscv));
    } else if (sext_inst->operands[0]->type == llvm_ir::Type::I8 && sext_inst->type == llvm_ir::Type::I32) {
        // i8到i32的符号扩展：需要先将byte符号扩展到32位
        // 使用左移24位再算术右移24位的方法
        reg temp_reg = current_riscv_function ? current_riscv_function->get_t_reg(false) : reg::x5;
        
        // 左移24位
        auto slli_inst = std::make_unique<riscvInstr>();
        slli_inst->op = opcode::slli;
        slli_inst->rd = temp_reg;
        slli_inst->rs1 = rs1;
        slli_inst->rs2 = reg::x0;
        slli_inst->imm = 24;
        insts.push_back(std::move(slli_inst));
        
        // 算术右移24位
        auto srai_inst = std::make_unique<riscvInstr>();
        srai_inst->op = opcode::srai;
        srai_inst->rd = rd;
        srai_inst->rs1 = temp_reg;
        srai_inst->rs2 = reg::x0;
        srai_inst->imm = 24;
        insts.push_back(std::move(srai_inst));
        
        if (current_riscv_function) {
        }
    } else {
        // 其他情况，直接移动（假设已经是正确的表示）
        auto mv_inst = std::make_unique<riscvInstr>();
        mv_inst->op = opcode::add;
        mv_inst->rd = rd;
        mv_inst->rs1 = rs1;
        mv_inst->rs2 = reg::x0;  // rs1 + 0 = rs1
        mv_inst->imm = 0;
        insts.push_back(std::move(mv_inst));
    }

    // 检查并生成store指令（如果变量被溢出）
    if (current_riscv_function) {
        add_spilled_store_if_needed(insts, sext_inst, rd, current_riscv_function);
    }

    return insts;
}

// Move指令生成
std::vector<std::unique_ptr<riscvInstr>> gen_move(llvm_ir::MoveInst* move_inst) {
    std::vector<std::unique_ptr<riscvInstr>> insts;
    std::vector<std::unique_ptr<riscvInstr>> post_insts;
    riscvFunction* func = current_riscv_function;

    // 在获取寄存器时，我们使用的是temp_verg_value，而不是move_inst的地址
    // 除非temp_verg_value为nullpter（这意味着move_inst是对应的原始phi指令）
    llvm_ir::Value* temp_verg_value = move_inst->get_value_ptr();
    
    // 获取目标寄存器，处理可能的寄存器冲突
    reg rd = func->get_reg_for_def(temp_verg_value, post_insts);
    
    // 将后续指令加入指令序列
    for (auto&& inst : post_insts) {
        insts.push_back(std::move(inst));
    }
    
    // 处理源操作数
    std::vector<std::unique_ptr<riscvInstr>> rs1_load_insts;
    reg rs1 = func->get_temp_reg_for_use(move_inst->operands[0], rs1_load_insts);
    
    // 添加rs1的load指令
    for (auto&& inst : rs1_load_insts) {
        insts.push_back(std::move(inst));
    }

    // 如果源操作数溢出到栈，需要先加载
    if (rs1 == NONE && func->is_spilled(move_inst->operands[0])) {
        // 源操作数溢出，从栈加载到临时寄存器
        rs1 = func->get_t_reg(move_inst->operands[0]->type == llvm_ir::Type::Float || 
                              move_inst->operands[0]->type == llvm_ir::Type::Double);
        auto src_load_insts = func->gen_load_from_stack(move_inst->operands[0], rs1);
        for (auto& src_load : src_load_insts) {
            insts.push_back(std::move(src_load));
        }
    } else if (rs1 == NONE) {
        // 如果仍然没有寄存器，使用默认寄存器
        rs1 = (move_inst->operands[0]->type == llvm_ir::Type::Float || 
               move_inst->operands[0]->type == llvm_ir::Type::Double) ? reg::f0 : reg::x11;
    }

    // 生成move指令
    auto mv_inst = std::make_unique<riscvInstr>();
    
    // 检查是否为浮点寄存器移动
    bool is_float_move = (move_inst->operands[0]->type == llvm_ir::Type::Float || 
                          move_inst->operands[0]->type == llvm_ir::Type::Double);
    
    if (is_float_move) {
        // 浮点寄存器移动使用fmv指令
        mv_inst->op = (move_inst->operands[0]->type == llvm_ir::Type::Float) ? opcode::fmv_s : opcode::fmv_d;
    } else {
        // 整数寄存器移动使用mv指令
        mv_inst->op = opcode::mv;
    }
    
    mv_inst->rd = rd;
    mv_inst->rs1 = rs1;

    insts.push_back(std::move(mv_inst));
    
    // 检查结果是否需要store到栈（溢出变量）
    add_spilled_store_if_needed(insts, temp_verg_value, rd, func);
    
    return insts;
}

}  // namespace riscv
