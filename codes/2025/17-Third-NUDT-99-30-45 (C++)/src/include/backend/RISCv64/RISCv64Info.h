#ifndef RISCV64_INFO_H
#define RISCV64_INFO_H

#include "RISCv64LLIR.h"
#include <map>
#include <vector>

namespace sysy {

// 定义一个全局的、权威的指令信息表
// 它包含了指令的定义(def)和使用(use)操作数索引
// defs: {0} -> 第一个操作数是定义
// uses: {1, 2} -> 第二、三个操作数是使用
static const std::map<RVOpcodes, std::pair<std::vector<int>, std::vector<int>>> op_info = {
    // --- 整数计算 (R-Type) ---
    {RVOpcodes::ADD, {{0}, {1, 2}}}, 
    {RVOpcodes::SUB, {{0}, {1, 2}}}, 
    {RVOpcodes::MUL, {{0}, {1, 2}}},
    {RVOpcodes::MULH, {{0}, {1, 2}}},
    {RVOpcodes::DIV, {{0}, {1, 2}}}, 
    {RVOpcodes::DIVW, {{0}, {1, 2}}},
    {RVOpcodes::REM, {{0}, {1, 2}}}, 
    {RVOpcodes::REMW, {{0}, {1, 2}}}, 
    {RVOpcodes::ADDW, {{0}, {1, 2}}},
    {RVOpcodes::SUBW, {{0}, {1, 2}}}, 
    {RVOpcodes::MULW, {{0}, {1, 2}}}, 
    {RVOpcodes::SLT, {{0}, {1, 2}}}, 
    {RVOpcodes::SLTU, {{0}, {1, 2}}},
    {RVOpcodes::XOR, {{0}, {1, 2}}},
    {RVOpcodes::OR, {{0}, {1, 2}}},
    {RVOpcodes::AND, {{0}, {1, 2}}},
    {RVOpcodes::SLL, {{0}, {1, 2}}},
    {RVOpcodes::SRL, {{0}, {1, 2}}},
    {RVOpcodes::SRA, {{0}, {1, 2}}},
    {RVOpcodes::SLLW, {{0}, {1, 2}}},
    {RVOpcodes::SRLW, {{0}, {1, 2}}},
    {RVOpcodes::SRAW, {{0}, {1, 2}}},

    // --- 整数计算 (I-Type) ---
    {RVOpcodes::ADDI, {{0}, {1}}}, 
    {RVOpcodes::ADDIW, {{0}, {1}}}, 
    {RVOpcodes::XORI, {{0}, {1}}},
    {RVOpcodes::ORI, {{0}, {1}}},
    {RVOpcodes::ANDI, {{0}, {1}}},
    {RVOpcodes::SLTI, {{0}, {1}}}, 
    {RVOpcodes::SLTIU, {{0}, {1}}},
    {RVOpcodes::SLLI, {{0}, {1}}},
    {RVOpcodes::SLLIW, {{0}, {1}}},
    {RVOpcodes::SRLI, {{0}, {1}}},
    {RVOpcodes::SRLIW, {{0}, {1}}},
    {RVOpcodes::SRAI, {{0}, {1}}},
    {RVOpcodes::SRAIW, {{0}, {1}}},

    // --- 内存加载 ---
    {RVOpcodes::LW, {{0}, {}}}, {RVOpcodes::LH, {{0}, {}}}, {RVOpcodes::LB, {{0}, {}}},
    {RVOpcodes::LWU, {{0}, {}}}, {RVOpcodes::LHU, {{0}, {}}}, {RVOpcodes::LBU, {{0}, {}}},
    {RVOpcodes::LD, {{0}, {}}},
    {RVOpcodes::FLW, {{0}, {}}}, {RVOpcodes::FLD, {{0}, {}}},

    // --- 内存存储 ---
    {RVOpcodes::SW, {{}, {0, 1}}}, {RVOpcodes::SH, {{}, {0, 1}}}, {RVOpcodes::SB, {{}, {0, 1}}},
    {RVOpcodes::SD, {{}, {0, 1}}},
    {RVOpcodes::FSW, {{}, {0, 1}}}, {RVOpcodes::FSD, {{}, {0, 1}}},

    // --- 分支指令 ---
    {RVOpcodes::BEQ, {{}, {0, 1}}}, {RVOpcodes::BNE, {{}, {0, 1}}}, {RVOpcodes::BLT, {{}, {0, 1}}}, 
    {RVOpcodes::BGE, {{}, {0, 1}}}, {RVOpcodes::BLTU, {{}, {0, 1}}}, {RVOpcodes::BGEU, {{}, {0, 1}}},

    // --- 跳转 ---
    {RVOpcodes::JAL, {{0}, {}}}, // JAL的rd是def，但通常用x0表示不关心返回值，这里简化
    {RVOpcodes::JALR, {{0}, {1}}},
    {RVOpcodes::RET, {{}, {}}}, // RET是伪指令，通常展开为JALR

    // --- 伪指令 & 其他 ---
    {RVOpcodes::LI, {{0}, {}}}, {RVOpcodes::LA, {{0}, {}}},
    {RVOpcodes::MV, {{0}, {1}}}, 
    {RVOpcodes::NEG, {{0}, {1}}}, // sub rd, zero, rs1
    {RVOpcodes::NEGW, {{0}, {1}}}, // subw rd, zero, rs1
    {RVOpcodes::SEQZ, {{0}, {1}}}, 
    {RVOpcodes::SNEZ, {{0}, {1}}},
    
    // --- 函数调用 ---
    // CALL的use/def在getInstrUseDef中有特殊处理逻辑，这里可以不列出

    // --- 浮点指令 ---
    {RVOpcodes::FADD_S, {{0}, {1, 2}}}, {RVOpcodes::FSUB_S, {{0}, {1, 2}}},
    {RVOpcodes::FMUL_S, {{0}, {1, 2}}}, {RVOpcodes::FDIV_S, {{0}, {1, 2}}},
    {RVOpcodes::FMADD_S, {{0}, {1, 2, 3}}},
    {RVOpcodes::FEQ_S, {{0}, {1, 2}}}, {RVOpcodes::FLT_S, {{0}, {1, 2}}}, {RVOpcodes::FLE_S, {{0}, {1, 2}}},
    {RVOpcodes::FCVT_S_W, {{0}, {1}}}, {RVOpcodes::FCVT_W_S, {{0}, {1}}},
    {RVOpcodes::FCVT_W_S_RTZ, {{0}, {1}}},
    {RVOpcodes::FMV_S, {{0}, {1}}}, {RVOpcodes::FMV_W_X, {{0}, {1}}}, {RVOpcodes::FMV_X_W, {{0}, {1}}},
    {RVOpcodes::FNEG_S, {{0}, {1}}}
};

} // namespace sysy

#endif // RISCV64_INFO_H
