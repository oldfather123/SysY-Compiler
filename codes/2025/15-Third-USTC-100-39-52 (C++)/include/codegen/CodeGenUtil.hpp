#pragma once

#include <stdexcept>

/* 关于位宽 */
#define IMM_12_MAX 0x7FF
#define IMM_12_MIN -0x800

#define LOW_12_MASK 0x00000FFF
#define LOW_20_MASK 0x000FFFFF
#define LOW_32_MASK 0xFFFFFFFF

inline unsigned ALIGN(unsigned x, unsigned alignment) {
    return ((x + (alignment - 1)) & ~(alignment - 1));
}

inline bool IS_IMM_12(int x) { return x <= IMM_12_MAX and x >= IMM_12_MIN; }

/* 栈帧相关 */
#define PROLOGUE_OFFSET_BASE 16 // $ra $fp
#define PROLOGUE_ALIGN 16

// /* 龙芯指令 */
// // Arithmetic
// #define ADD "add"
// #define SUB "sub"
// #define MUL "mul"
// #define DIV "div"

// #define ADDI "addi"

// #define FADD "fadd"
// #define FSUB "fsub"
// #define FMUL "fmul"
// #define FDIV "fdiv"

// #define ORI "ori"

// #define LU12I_W "lu12i.w"
// #define LU32I_D "lu32i.d"
// #define LU52I_D "lu52i.d"

// // Data transfer (greg <-> freg)
// #define GR2FR "movgr2fr"
// #define FR2GR "movfr2gr"

// // Memory access
// #define LOAD "ld"
// #define STORE "st"
// #define FLOAD "fld"
// #define FSTORE "fst"

// #define BYTE ".b"
// #define HALF_WORD ".h"
// #define WORD ".w"
// #define DOUBLE ".d"

// #define SINGLE ".s" // float
// #define LONG ".l"

// // ASM syntax sugar
// #define LOAD_ADDR "la.local"
/* RISC-V指令 */

// ==== Arithmetic (Integer) ====
#define ADD "add"  //整数加法：rd = rs1 + rs2
#define SUB "sub"  //整数减法：rd = rs1 - rs2
#define MUL "mul"  //整数乘法（低位）：rd = rs1 * rs2
#define DIV "div"  //整数除法（有符号）：rd = rs1 / rs2
#define REM "rem"  //整数取余（有符号）：rd = rs1 % rs2
#define SLL "sll"  //逻辑左移：rd = rs1 << (rs2 & 0x3F)
#define SRL "srl"  //逻辑右移：rd = rs1 >> (rs2 & 0x3F)（零扩展）
#define SRA "sra"  //算术右移：保留符号位
#define SLT "slt"  //比较小于（有符号）：rd = (rs1 < rs2)
#define SLTU "sltu"  //比较小于（无符号）：rd = (rs1 < rs2)
#define XOR "xor"  //位异或
#define OR "or"  //位或
#define AND "and"  //位与

// ==== Immediate Arithmetic ====
#define ADDI "addi"  //加立即数：rd = rs1 + imm
#define SLTI "slti"  //小于立即数（有符号）
#define SLTIU "sltiu"  //小于立即数（无符号）
#define XORI "xori"  //异或立即数
#define ORI "ori"  //或立即数
#define ANDI "andi"  //与立即数
#define SLLI "slli"  //左移立即数
#define SRLI "srli"  //逻辑右移立即数
#define SRAI "srai"  //算术右移立即数

// ==== Control Flow ====
#define JAL "jal"  //跳转并链接：rd = PC+4; PC += imm
#define JALR "jalr"  //跳转寄存器：rd = PC+4; PC = rs1 + imm
#define BEQ "beq"  //相等跳转：若 rs1 == rs2 则跳转
#define BNE "bne"  //不等跳转
#define BLT "blt"  //小于跳转（有符号）
#define BGE "bge"  //大于等于跳转（有符号）
#define BLTU "bltu"  //	小于跳转（无符号）
#define BGEU "bgeu"  //大于等于跳转（无符号）

// ==== Load Upper ====
#define LUI "lui"  //加载高20位：rd = imm << 12
#define AUIPC "auipc"  //当前PC加上imm高20位：rd = PC + (imm << 12)
#define LU12I_W "lui"  // 同 LUI，部分实现中使用此别名表示“加载12位常数”same as LUI

// ==== RV64 Word Arithmetic ====
#define ADDIW "addiw"  //32位加法：结果符号扩展到64位
#define SLLIW "slliw"  //32位左移
#define SRLIW "srliw"  //32位逻辑右移
#define SRAIW "sraiw"  //32位算术右移
#define ADDW "addw"  //32位寄存器加法
#define SUBW "subw"  //32位寄存器减法
#define SLLW "sllw"  //32位左移（寄存器）
#define SRLW "srlw"  //	32位右移（寄存器）
#define SRAW "sraw"  //	32位算术右移（寄存器）

// ==== RV64M (Multiply/Divide/Mod) ====
#define MULH "mulh"  //高位乘积（有符号）
#define MULHU "mulhu"  //高位乘积（无符号）
#define MULHSU "mulhsu"  //高位乘积（rs1 有符号，rs2 无符号）
#define DIVU "divu"  //无符号除法
#define REMU "remu"  //无符号取余
#define MULW "mulw"  //32位乘法，结果符号扩展
#define DIVW "divw"  //	32位除法（有符号）
#define DIVUW "divuw"  //32位除法（无符号）
#define REMW "remw"  //32位取余（有符号）

// ==== Floating Point Arithmetic ====
#define FADD "fadd.s"  //浮点加法：f[rd] = f[rs1] + f[rs2]
#define FSUB "fsub.s"  //浮点减法
#define FMUL "fmul.s"  //浮点乘法
#define FDIV "fdiv.s"  //	浮点除法
#define FSQRT "fsqrt.s"  //	浮点开方
#define FMIN "fmin.s"  //取较小浮点值
#define FMAX "fmax.s"  //取较大浮点值

// ==== Floating Point Compare & Convert ====
#define FEQ "feq.s"  //浮点相等比较
#define FLT "flt.s"  //小于比较
#define FLE "fle.s"  //小于等于比较
#define FCVT_W_S "fcvt.w.s"  //	浮点转整数（float → int）
#define FCVT_S_W "fcvt.s.w"  //	整数转浮点（int → float）
#define FMV_X_W "fmv.x.w"  //浮点寄存器内容原样移入整数寄存器
#define FMV_W_X "fmv.w.x"  //整数寄存器内容原样移入浮点寄存器
#define FMV_S "fmv.s"  // 浮点寄存器之间移动

// ==== Data Transfer (greg <-> freg) ====
#define GR2FR "fcvt.s.w"  // int -> float整数转单精度浮点
#define FR2GR "fcvt.w.s"  // float -> int单精度浮点转整数

// ==== Memory Access (Integer) ====
#define LOAD_BYTE "lb"  //从内存加载字节（符号扩展）
#define LOAD_HALF "lh"  //加载半字（16位）
#define LOAD_WORD "lw"  //加载32位整数
#define LOAD_UWORD "lwu"  //加载32位整数（零扩展）
#define LOAD_DOUBLE "ld"  //加载64位整数
#define STORE_BYTE "sb"  //存储8位
#define STORE_HALF "sh"  //存储16位
#define STORE_WORD "sw"  //存储32位
#define STORE_DOUBLE "sd"  //存储64位

// ==== Memory Access (Float) ====
#define FLOAD_SINGLE "flw"  //加载32位 float 到 FPR
#define FSTORE_SINGLE "fsw"  //存储32位 float
#define FLOAD_DOUBLE "fld"  //加载64位 double 到 FPR
#define FSTORE_DOUBLE "fsd"  //存储64位 double

// ==== ASM Syntax Sugar / Pseudo-instructions ====
#define LI "li"  //加载立即数（伪指令，转为 lui+addi）
#define LA "la"  //加载地址（伪指令，转为 auipc + addi）
#define LLA "lla"  //加载本地地址（Local LA）
#define MV "mv"  //寄存器拷贝：rd = rs（转为 addi rd, rs, 0）
#define NEG "neg"  //取负值：rd = -rs（转为 sub rd, x0, rs）
#define NOT "not"  //位取反：rd = ~rs（转为 xori rd, rs, -1）
#define RET "ret"  //返回：jalr x0, ra, 0
#define CALL "call"  //调用函数（伪指令）
#define TAIL "tail"  //尾调用
#define NOP "nop"  //空指令：addi x0, x0, 0
#define SEQZ "seqz"  //rd = (rs == 0)（转为 sltiu rd, rs, 1）
#define SNEZ "snez"  //rd = (rs != 0)
#define BEQZ "beqz"  //beq rs, x0, label
#define BNEZ "bnez"  //bne rs, x0, label
#define BLEZ "blez"  //bge x0, rs, label
#define BGEZ "bgez"  //bge rs, x0, label
#define BLTZ "bltz"  //blt rs, x0, label
#define BGTZ "bgtz"  //blt x0, rs, label

// ==== Placeholders for Size Tokens ====
#define BYTE "b"  //字节（8位）
#define HALF_WORD "h"  //半字（16位）
#define WORD "w"  //字（32位）
#define DOUBLE "d"  //双字（64位）

  // ASM syntax sugar
#define LOAD_ADDR "la"  //la，加载地址的伪指令


// errors
class not_implemented_error : public std::logic_error {
  public:
    explicit not_implemented_error(std::string &&err_msg = "")
        : std::logic_error(err_msg){};
};

class unreachable_error : public std::logic_error {
  public:
    explicit unreachable_error(std::string &&err_msg = "")
        : std::logic_error(err_msg){};
};
