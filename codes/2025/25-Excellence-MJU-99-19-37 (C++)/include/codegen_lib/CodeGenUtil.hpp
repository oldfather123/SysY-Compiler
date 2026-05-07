#pragma once

#include <stdexcept>

/* === Immediate Ranges === */
#define IMM_12_MAX 0x7FF
#define IMM_12_MIN -0x800

#define LOW_12_MASK 0x00000FFF
#define LOW_20_MASK 0x000FFFFF
#define LOW_32_MASK 0xFFFFFFFF // 补充32位掩码

inline bool IS_IMM_12(int64_t x) { return x <= IMM_12_MAX && x >= IMM_12_MIN; }

/* === Stack Frame Layout === */
#define PROLOGUE_OFFSET_BASE 16 // ra + fp (各8字节)
#define PROLOGUE_ALIGN 16      // 16字节对齐

/* === RISC-V RV32I Instruction Set === */

// Integer Register-Register Arithmetic
#define ADD "add"
#define SUB "sub"
#define SLL "sll"
#define SLT "slt"
#define SLTU "sltu"
#define XOR "xor"
#define SRL "srl"
#define SRA "sra"
#define OR "or"
#define AND "and"

// Integer Immediate Arithmetic
#define ADDI "addi"
#define SLTI "slti"
#define SLTIU "sltiu"
#define XORI "xori"
#define ORI "ori"
#define ANDI "andi"
#define SLLI "slli"
#define SRLI "srli"
#define SRAI "srai"

// Load Instructions (RV32I)
#define LB "lb"
#define LH "lh"
#define LW "lw"
#define LBU "lbu"
#define LHU "lhu"

// Store Instructions (RV32I)
#define SB "sb"
#define SH "sh"
#define SW "sw"

// Branch Instructions
#define BEQ "beq"
#define BNE "bne"
#define BLT "blt"
#define BGE "bge"
#define BLTU "bltu"
#define BGEU "bgeu"

// Jump Instructions
#define JAL "jal"
#define JALR "jalr"

// Fence/System
#define FENCE "fence"
#define FENCE_I "fence.i"
#define ECALL "ecall"
#define EBREAK "ebreak"

/* === RV32F (Float - Single Precision) === */

// Floating-point Arithmetic
#define FADD "fadd.s"
#define FSUB "fsub.s"
#define FMUL "fmul.s"
#define FDIV "fdiv.s"

// Floating-point Compare
#define FEQ "feq.s"
#define FLT "flt.s"
#define FLE "fle.s"

// Floating-point Conversion
#define FCVT_W_S "fcvt.w.s"
#define FCVT_S_W "fcvt.s.w"
#define FMV_W_X "fmv.w.x"
#define FMV_X_W "fmv.x.w"

// Floating-point Load/Store
#define FLOAD "flw"
#define FSTORE "fsw"

/* === Pseudoinstructions / Syntax Sugar === */
#define LOAD_ADDR "la"
#define NOP "nop"       // addi x0, x0, 0
#define MV "mv"         // addi rd, rs, 0
#define NOT "not"       // xori rd, rs, -1
#define NEG "neg"       // sub rd, x0, rs
#define RET "ret"       // jalr x0, ra, 0

/* === Assembly Data Directives === */
#define BYTE ".byte"
#define HALF_WORD ".half"
#define WORD ".word"
#define DOUBLE ".double"   // usually .d in float64 context
#define SINGLE ".float"    // better than ".single" — standard

// 64位加载指令
#define LD "ld"
#define SD "sd"

// 64位整数指令
#define ADDW "addw"
#define SUBW "subw"
#define MULW "mulw"
#define DIVW "divw"

// 地址加载伪指令
#define AUIPC "auipc"

/* === Error Classes === */
class not_implemented_error : public std::logic_error {
  public:
    explicit not_implemented_error(std::string &&err_msg = "")
        : std::logic_error(err_msg) {}
};

class unreachable_error : public std::logic_error {
  public:
    explicit unreachable_error(std::string &&err_msg = "")
        : std::logic_error(err_msg) {}
};
