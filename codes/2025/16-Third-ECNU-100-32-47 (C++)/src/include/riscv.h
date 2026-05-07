/**
 * @file riscv.h
 * 该头文件定义了 RISCV 相关的体系架构信息
 */

#ifndef AAAC_RISCV_H
#define AAAC_RISCV_H

#include <algorithm>
#include <set>
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace aaac {
namespace backend {

/**
 * @brief RISC-V 32位整数寄存器枚举
 *
 */
enum class RISCV_Reg {
  __zero, // x0
  __ra,   // x1 (return address)
  __sp,   // x2 (stack pointer)
  __gp,   // x3 (global pointer)
  __tp,   // x4 (thread pointer)
  __t0,   // x5 (temporary 0)
  __t1,   // x6 (temporary 1)
  __t2,   // x7 (temporary 2)
  __s0,   // x8/fp (saved 0/frame pointer)
  __s1,   // x9 (saved 1)
  __a0,   // x10 (argument/return 0)
  __a1,   // x11 (argument/return 1)
  __a2,   // x12 (argument 2)
  __a3,   // x13 (argument 3)
  __a4,   // x14 (argument 4)
  __a5,   // x15 (argument 5)
  __a6,   // x16 (argument 6)
  __a7,   // x17 (argument 7)
  __s2,   // x18 (saved 2)
  __s3,   // x19 (saved 3)
  __s4,   // x20 (saved 4)
  __s5,   // x21 (saved 5)
  __s6,   // x22 (saved 6)
  __s7,   // x23 (saved 7)
  __s8,   // x24 (saved 8)
  __s9,   // x25 (saved 9)
  __s10,  // x26 (saved 10)
  __s11,  // x27 (saved 11)
  __t3,   // x28 (temporary 3)
  __t4,   // x29 (temporary 4)
  __t5,   // x30 (temporary 5)
  __t6,   // x31 (temporary 6)
  // Floating-point registers
  __ft0,  // f0  (temporary)
  __ft1,  // f1  (temporary)
  __ft2,  // f2  (temporary)
  __ft3,  // f3  (temporary)
  __ft4,  // f4  (temporary)
  __ft5,  // f5  (temporary)
  __ft6,  // f6  (temporary)
  __ft7,  // f7  (temporary)
  __fs0,  // f8  (saved)
  __fs1,  // f9  (saved)
  __fa0,  // f10 (argument 0)
  __fa1,  // f11 (argument 1)
  __fa2,  // f12 (argument 2)
  __fa3,  // f13 (argument 3)
  __fa4,  // f14 (argument 4)
  __fa5,  // f15 (argument 5)
  __fa6,  // f16 (argument 6)
  __fa7,  // f17 (argument 7)
  __fs2,  // f18 (saved)
  __fs3,  // f19 (saved)
  __fs4,  // f20 (saved)
  __fs5,  // f21 (saved)
  __fs6,  // f22 (saved)
  __fs7,  // f23 (saved)
  __fs8,  // f24 (saved)
  __fs9,  // f25 (saved)
  __fs10, // f26 (saved)
  __fs11, // f27 (saved)
  __ft8,  // f28 (temporary)
  __ft9,  // f29 (temporary)
  __ft10, // f30 (temporary)
  __ft11, // f31 (temporary)

  INVALID
};

constexpr size_t REG_COUNT = static_cast<size_t>(RISCV_Reg::INVALID);

static std::set<RISCV_Reg> caller_save = {
  RISCV_Reg::__t0,RISCV_Reg::__t1,RISCV_Reg::__t2,RISCV_Reg::__t3,RISCV_Reg::__t4,RISCV_Reg::__t5,RISCV_Reg::__t6,
  RISCV_Reg::__ft0,RISCV_Reg::__ft1,RISCV_Reg::__ft2,RISCV_Reg::__ft3,RISCV_Reg::__ft4,RISCV_Reg::__ft5,RISCV_Reg::__ft6,
  RISCV_Reg::__ft8,RISCV_Reg::__ft9,RISCV_Reg::__ft10,RISCV_Reg::__ft11,
  RISCV_Reg::__a0,RISCV_Reg::__a1,RISCV_Reg::__a2,RISCV_Reg::__a3,RISCV_Reg::__a4,RISCV_Reg::__a5,RISCV_Reg::__a6,RISCV_Reg::__a7,
  RISCV_Reg::__fa0,RISCV_Reg::__fa1,RISCV_Reg::__fa2,RISCV_Reg::__fa3,RISCV_Reg::__fa4,RISCV_Reg::__fa5,RISCV_Reg::__fa6,RISCV_Reg::__fa7,  
};

static std::set<RISCV_Reg> callee_save = {
  RISCV_Reg::__s0,RISCV_Reg::__s1,RISCV_Reg::__s2,RISCV_Reg::__s3,RISCV_Reg::__s4,RISCV_Reg::__s5,RISCV_Reg::__s6,RISCV_Reg::__s7,RISCV_Reg::__s8,RISCV_Reg::__s9,RISCV_Reg::__s10,RISCV_Reg::__s11,
  RISCV_Reg::__fs0,RISCV_Reg::__fs1,RISCV_Reg::__fs2,RISCV_Reg::__fs3,RISCV_Reg::__fs4,RISCV_Reg::__fs5,RISCV_Reg::__fs6,RISCV_Reg::__fs7,RISCV_Reg::__fs8,RISCV_Reg::__fs9,RISCV_Reg::__fs10,RISCV_Reg::__fs11,
};

constexpr std::array<RISCV_Reg, 11> CALLEE_SAVED_GPRS{
    RISCV_Reg::__s1, RISCV_Reg::__s2, RISCV_Reg::__s3, RISCV_Reg::__s4,
    RISCV_Reg::__s5, RISCV_Reg::__s6, RISCV_Reg::__s7, RISCV_Reg::__s8,
    RISCV_Reg::__s9, RISCV_Reg::__s10, RISCV_Reg::__s11};

constexpr std::array<RISCV_Reg, 12> CALLEE_SAVED_FPRS{
    RISCV_Reg::__fs0, RISCV_Reg::__fs1, RISCV_Reg::__fs2, RISCV_Reg::__fs3,
    RISCV_Reg::__fs4, RISCV_Reg::__fs5, RISCV_Reg::__fs6, RISCV_Reg::__fs7,
    RISCV_Reg::__fs8, RISCV_Reg::__fs9, RISCV_Reg::__fs10, RISCV_Reg::__fs11};

constexpr std::array<const char *, 65> REG_NAMES = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
    "ft0",  "ft1",  "ft2",  "ft3",  "ft4",  "ft5",  "ft6",  "ft7",
    "fs0",  "fs1", "fa0", "fa1", "fa2", "fa3", "fa4",
    "fa5", "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6",
    "fs7", "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11",
    "INVALID"};

inline const char *reg_to_string(RISCV_Reg reg) {
  const auto idx = static_cast<size_t>(reg);
  return (idx < REG_NAMES.size()) ? REG_NAMES[idx] : "unknown";
}

inline constexpr int reg_to_index(RISCV_Reg reg) {
  return static_cast<int>(reg);
}

/**
 * @brief RV64GC 合法指令枚举类
 *
 */
enum class RV64GC_ISA {
  // RV64I 基础指令
  ADD,
  ADDI,
  SUB,
  AND,
  ANDI,
  OR,
  ORI,
  XOR,
  XORI,
  SLL,
  SLLI,
  SRL,
  SRLI,
  SRA,
  SRAI,
  SLT,
  SLTI,
  SLTU,
  SLTIU,
  LUI,
  AUIPC,
  ADDIW,
  ADDW,
  SUBW,
  SLLW,
  SLLIW,
  SRLW,
  SRLIW,
  SRAW,
  SRAIW,
  JAL,
  JALR,
  BEQ,
  BNE,
  BLT,
  BGE,
  BLTU,
  BGEU,
  LB,
  LBU,
  LH,
  LHU,
  LW,
  LWU,
  LD,
  SB,
  SH,
  SW,
  SD,
  FENCE,
  ECALL,
  EBREAK,

  // RV64M 乘除指令
  MUL,
  MULH,
  MULHU,
  MULHSU,
  DIV,
  DIVU,
  REM,
  REMU,
  MULW,
  DIVW,
  DIVUW,
  REMW,
  REMUW,

  // RV64A 原子指令
  LR_W,
  LR_D,
  SC_W,
  SC_D,
  AMOSWAP_W,
  AMOSWAP_D,
  AMOADD_W,
  AMOADD_D,
  AMOAND_W,
  AMOAND_D,
  AMOOR_W,
  AMOOR_D,
  AMOXOR_W,
  AMOXOR_D,
  AMOMIN_W,
  AMOMIN_D,
  AMOMAX_W,
  AMOMAX_D,
  AMOMINU_W,
  AMOMINU_D,
  AMOMAXU_W,
  AMOMAXU_D,

  // RV64F 单精度浮点
  FADD_S,
  FSUB_S,
  FMUL_S,
  FDIV_S,
  FSQRT_S,
  FMIN_S,
  FMAX_S,
  FMADD_S,
  FMSUB_S,
  FNMADD_S,
  FNMSUB_S,
  FEQ_S,
  FLT_S,
  FLE_S,
  FCVT_W_S,
  FCVT_WU_S,
  FCVT_S_W,
  FCVT_S_WU,
  FCVT_L_S,
  FCVT_LU_S,
  FCVT_S_L,
  FCVT_S_LU,
  FMV_X_W,
  FMV_W_X,
  FCLASS_S,

  // RV64D 双精度浮点
  FADD_D,
  FSUB_D,
  FMUL_D,
  FDIV_D,
  FSQRT_D,
  FMIN_D,
  FMAX_D,
  FMADD_D,
  FMSUB_D,
  FNMADD_D,
  FNMSUB_D,
  FEQ_D,
  FLT_D,
  FLE_D,
  FCVT_W_D,
  FCVT_WU_D,
  FCVT_D_W,
  FCVT_D_WU,
  FCVT_L_D,
  FCVT_LU_D,
  FCVT_D_L,
  FCVT_D_LU,
  FCVT_S_D,
  FCVT_D_S,
  FMV_X_D,
  FMV_D_X,
  FCLASS_D,

  // RV64C 压缩指令
  C_ADDI4SPN,
  C_FLD,
  C_LQ,
  C_LW,
  C_FLW,
  C_LD,
  C_FSD,
  C_SQ,
  C_SW,
  C_FSW,
  C_SD,
  C_NOP,
  C_ADDI,
  C_JAL,
  C_ADDIW,
  C_LI,
  C_ADDI16SP,
  C_LUI,
  C_SRLI,
  C_SRAI,
  C_ANDI,
  C_SUB,
  C_XOR,
  C_OR,
  C_AND,
  C_SUBW,
  C_ADDW,
  C_J,
  C_BEQZ,
  C_BNEZ,
  C_SLLI,
  C_FLDSP,
  C_LQSP,
  C_LWSP,
  C_FLWSP,
  C_LDSP,
  C_JR,
  C_MV,
  C_EBREAK,
  C_JALR,
  C_ADD,
  C_FSDSP,
  C_SQSP,
  C_SWSP,
  C_FSWSP,
  C_SDSP
};

/**
 * @brief RV64GC 合法指令映射表
 *
 * 用于生成汇编语句
 */
static const std::unordered_map<RV64GC_ISA, std::string> ISA_STRING_MAP = {
    // RV64I 基础指令
    {RV64GC_ISA::ADD, "add"},
    {RV64GC_ISA::ADDI, "addi"},
    {RV64GC_ISA::SUB, "sub"},
    {RV64GC_ISA::AND, "and"},
    {RV64GC_ISA::ANDI, "andi"},
    {RV64GC_ISA::OR, "or"},
    {RV64GC_ISA::ORI, "ori"},
    {RV64GC_ISA::XOR, "xor"},
    {RV64GC_ISA::XORI, "xori"},
    {RV64GC_ISA::SLL, "sll"},
    {RV64GC_ISA::SLLI, "slli"},
    {RV64GC_ISA::SRL, "srl"},
    {RV64GC_ISA::SRLI, "srli"},
    {RV64GC_ISA::SRA, "sra"},
    {RV64GC_ISA::SRAI, "srai"},
    {RV64GC_ISA::SLT, "slt"},
    {RV64GC_ISA::SLTI, "slti"},
    {RV64GC_ISA::SLTU, "sltu"},
    {RV64GC_ISA::SLTIU, "sltiu"},
    {RV64GC_ISA::LUI, "lui"},
    {RV64GC_ISA::AUIPC, "auipc"},
    {RV64GC_ISA::ADDIW, "addiw"},
    {RV64GC_ISA::ADDW, "addw"},
    {RV64GC_ISA::SUBW, "subw"},
    {RV64GC_ISA::SLLW, "sllw"},
    {RV64GC_ISA::SLLIW, "slliw"},
    {RV64GC_ISA::SRLW, "srlw"},
    {RV64GC_ISA::SRLIW, "srliw"},
    {RV64GC_ISA::SRAW, "sraw"},
    {RV64GC_ISA::SRAIW, "sraiw"},
    {RV64GC_ISA::JAL, "jal"},
    {RV64GC_ISA::JALR, "jalr"},
    {RV64GC_ISA::BEQ, "beq"},
    {RV64GC_ISA::BNE, "bne"},
    {RV64GC_ISA::BLT, "blt"},
    {RV64GC_ISA::BGE, "bge"},
    {RV64GC_ISA::BLTU, "bltu"},
    {RV64GC_ISA::BGEU, "bgeu"},
    {RV64GC_ISA::LB, "lb"},
    {RV64GC_ISA::LBU, "lbu"},
    {RV64GC_ISA::LH, "lh"},
    {RV64GC_ISA::LHU, "lhu"},
    {RV64GC_ISA::LW, "lw"},
    {RV64GC_ISA::LWU, "lwu"},
    {RV64GC_ISA::LD, "ld"},
    {RV64GC_ISA::SB, "sb"},
    {RV64GC_ISA::SH, "sh"},
    {RV64GC_ISA::SW, "sw"},
    {RV64GC_ISA::SD, "sd"},
    {RV64GC_ISA::FENCE, "fence"},
    {RV64GC_ISA::ECALL, "ecall"},
    {RV64GC_ISA::EBREAK, "ebreak"},

    // RV64M 乘除指令
    {RV64GC_ISA::MUL, "mul"},
    {RV64GC_ISA::MULH, "mulh"},
    {RV64GC_ISA::MULHU, "mulhu"},
    {RV64GC_ISA::MULHSU, "mulhsu"},
    {RV64GC_ISA::DIV, "div"},
    {RV64GC_ISA::DIVU, "divu"},
    {RV64GC_ISA::REM, "rem"},
    {RV64GC_ISA::REMU, "remu"},
    {RV64GC_ISA::MULW, "mulw"},
    {RV64GC_ISA::DIVW, "divw"},
    {RV64GC_ISA::DIVUW, "divuw"},
    {RV64GC_ISA::REMW, "remw"},
    {RV64GC_ISA::REMUW, "remuw"},

    // RV64A 原子指令
    {RV64GC_ISA::LR_W, "lr.w"},
    {RV64GC_ISA::LR_D, "lr.d"},
    {RV64GC_ISA::SC_W, "sc.w"},
    {RV64GC_ISA::SC_D, "sc.d"},
    {RV64GC_ISA::AMOSWAP_W, "amoswap.w"},
    {RV64GC_ISA::AMOSWAP_D, "amoswap.d"},
    {RV64GC_ISA::AMOADD_W, "amoadd.w"},
    {RV64GC_ISA::AMOADD_D, "amoadd.d"},
    {RV64GC_ISA::AMOAND_W, "amoand.w"},
    {RV64GC_ISA::AMOAND_D, "amoand.d"},
    {RV64GC_ISA::AMOOR_W, "amoor.w"},
    {RV64GC_ISA::AMOOR_D, "amoor.d"},
    {RV64GC_ISA::AMOXOR_W, "amoxor.w"},
    {RV64GC_ISA::AMOXOR_D, "amoxor.d"},
    {RV64GC_ISA::AMOMIN_W, "amomin.w"},
    {RV64GC_ISA::AMOMIN_D, "amomin.d"},
    {RV64GC_ISA::AMOMAX_W, "amomax.w"},
    {RV64GC_ISA::AMOMAX_D, "amomax.d"},
    {RV64GC_ISA::AMOMINU_W, "amominu.w"},
    {RV64GC_ISA::AMOMINU_D, "amominu.d"},
    {RV64GC_ISA::AMOMAXU_W, "amomaxu.w"},
    {RV64GC_ISA::AMOMAXU_D, "amomaxu.d"},

    // RV64F 单精度浮点指令
    {RV64GC_ISA::FADD_S, "fadd.s"},
    {RV64GC_ISA::FSUB_S, "fsub.s"},
    {RV64GC_ISA::FMUL_S, "fmul.s"},
    {RV64GC_ISA::FDIV_S, "fdiv.s"},
    {RV64GC_ISA::FSQRT_S, "fsqrt.s"},
    {RV64GC_ISA::FMIN_S, "fmin.s"},
    {RV64GC_ISA::FMAX_S, "fmax.s"},
    {RV64GC_ISA::FMADD_S, "fmadd.s"},
    {RV64GC_ISA::FMSUB_S, "fmsub.s"},
    {RV64GC_ISA::FNMADD_S, "fnmadd.s"},
    {RV64GC_ISA::FNMSUB_S, "fnmsub.s"},
    {RV64GC_ISA::FEQ_S, "feq.s"},
    {RV64GC_ISA::FLT_S, "flt.s"},
    {RV64GC_ISA::FLE_S, "fle.s"},
    {RV64GC_ISA::FCVT_W_S, "fcvt.w.s"},
    {RV64GC_ISA::FCVT_WU_S, "fcvt.wu.s"},
    {RV64GC_ISA::FCVT_S_W, "fcvt.s.w"},
    {RV64GC_ISA::FCVT_S_WU, "fcvt.s.wu"},
    {RV64GC_ISA::FCVT_L_S, "fcvt.l.s"},
    {RV64GC_ISA::FCVT_LU_S, "fcvt.lu.s"},
    {RV64GC_ISA::FCVT_S_L, "fcvt.s.l"},
    {RV64GC_ISA::FCVT_S_LU, "fcvt.s.lu"},
    {RV64GC_ISA::FMV_X_W, "fmv.x.w"},
    {RV64GC_ISA::FMV_W_X, "fmv.w.x"},
    {RV64GC_ISA::FCLASS_S, "fclass.s"},

    // RV64D 双精度浮点指令
    {RV64GC_ISA::FADD_D, "fadd.d"},
    {RV64GC_ISA::FSUB_D, "fsub.d"},
    {RV64GC_ISA::FMUL_D, "fmul.d"},
    {RV64GC_ISA::FDIV_D, "fdiv.d"},
    {RV64GC_ISA::FSQRT_D, "fsqrt.d"},
    {RV64GC_ISA::FMIN_D, "fmin.d"},
    {RV64GC_ISA::FMAX_D, "fmax.d"},
    {RV64GC_ISA::FMADD_D, "fmadd.d"},
    {RV64GC_ISA::FMSUB_D, "fmsub.d"},
    {RV64GC_ISA::FNMADD_D, "fnmadd.d"},
    {RV64GC_ISA::FNMSUB_D, "fnmsub.d"},
    {RV64GC_ISA::FEQ_D, "feq.d"},
    {RV64GC_ISA::FLT_D, "flt.d"},
    {RV64GC_ISA::FLE_D, "fle.d"},
    {RV64GC_ISA::FCVT_W_D, "fcvt.w.d"},
    {RV64GC_ISA::FCVT_WU_D, "fcvt.wu.d"},
    {RV64GC_ISA::FCVT_D_W, "fcvt.d.w"},
    {RV64GC_ISA::FCVT_D_WU, "fcvt.d.wu"},
    {RV64GC_ISA::FCVT_L_D, "fcvt.l.d"},
    {RV64GC_ISA::FCVT_LU_D, "fcvt.lu.d"},
    {RV64GC_ISA::FCVT_D_L, "fcvt.d.l"},
    {RV64GC_ISA::FCVT_D_LU, "fcvt.d.lu"},
    {RV64GC_ISA::FCVT_S_D, "fcvt.s.d"},
    {RV64GC_ISA::FCVT_D_S, "fcvt.d.s"},
    {RV64GC_ISA::FMV_X_D, "fmv.x.d"},
    {RV64GC_ISA::FMV_D_X, "fmv.d.x"},
    {RV64GC_ISA::FCLASS_D, "fclass.d"},

    // RV64C 压缩指令
    {RV64GC_ISA::C_ADDI4SPN, "c.addi4spn"},
    {RV64GC_ISA::C_FLD, "c.fld"},
    {RV64GC_ISA::C_LQ, "c.lq"},
    {RV64GC_ISA::C_LW, "c.lw"},
    {RV64GC_ISA::C_FLW, "c.flw"},
    {RV64GC_ISA::C_LD, "c.ld"},
    {RV64GC_ISA::C_FSD, "c.fsd"},
    {RV64GC_ISA::C_SQ, "c.sq"},
    {RV64GC_ISA::C_SW, "c.sw"},
    {RV64GC_ISA::C_FSW, "c.fsw"},
    {RV64GC_ISA::C_SD, "c.sd"},
    {RV64GC_ISA::C_NOP, "c.nop"},
    {RV64GC_ISA::C_ADDI, "c.addi"},
    {RV64GC_ISA::C_JAL, "c.jal"},
    {RV64GC_ISA::C_ADDIW, "c.addiw"},
    {RV64GC_ISA::C_LI, "c.li"},
    {RV64GC_ISA::C_ADDI16SP, "c.addi16sp"},
    {RV64GC_ISA::C_LUI, "c.lui"},
    {RV64GC_ISA::C_SRLI, "c.srli"},
    {RV64GC_ISA::C_SRAI, "c.srai"},
    {RV64GC_ISA::C_ANDI, "c.andi"},
    {RV64GC_ISA::C_SUB, "c.sub"},
    {RV64GC_ISA::C_XOR, "c.xor"},
    {RV64GC_ISA::C_OR, "c.or"},
    {RV64GC_ISA::C_AND, "c.and"},
    {RV64GC_ISA::C_SUBW, "c.subw"},
    {RV64GC_ISA::C_ADDW, "c.addw"},
    {RV64GC_ISA::C_J, "c.j"},
    {RV64GC_ISA::C_BEQZ, "c.beqz"},
    {RV64GC_ISA::C_BNEZ, "c.bnez"},
    {RV64GC_ISA::C_SLLI, "c.slli"},
    {RV64GC_ISA::C_FLDSP, "c.fldsp"},
    {RV64GC_ISA::C_LQSP, "c.lqsp"},
    {RV64GC_ISA::C_LWSP, "c.lwsp"},
    {RV64GC_ISA::C_FLWSP, "c.flwsp"},
    {RV64GC_ISA::C_LDSP, "c.ldsp"},
    {RV64GC_ISA::C_JR, "c.jr"},
    {RV64GC_ISA::C_MV, "c.mv"},
    {RV64GC_ISA::C_EBREAK, "c.ebreak"},
    {RV64GC_ISA::C_JALR, "c.jalr"},
    {RV64GC_ISA::C_ADD, "c.add"},
    {RV64GC_ISA::C_FSDSP, "c.fsdsp"},
    {RV64GC_ISA::C_SQSP, "c.sqsp"},
    {RV64GC_ISA::C_SWSP, "c.swsp"},
    {RV64GC_ISA::C_FSWSP, "c.fswsp"},
    {RV64GC_ISA::C_SDSP, "c.sdsp"}};

inline RISCV_Reg index_to_reg(int idx) {
    if (idx < 0 || idx >= reg_to_index(RISCV_Reg::INVALID))
        return RISCV_Reg::INVALID;
    return static_cast<RISCV_Reg>(idx);
}


inline RISCV_Reg string_to_reg(std::string reg) {
  for (int i = 0; i < REG_NAMES.size(); i++) {
    if (REG_NAMES[i] == reg) {
      return index_to_reg(i);
    }
  }  
  return RISCV_Reg::INVALID;
}

inline std::string isa_to_string(RV64GC_ISA isa) {
  auto it = ISA_STRING_MAP.find(isa);
  return (it != ISA_STRING_MAP.end()) ? it->second : "unknown";
}

static inline bool isFloatReg(RISCV_Reg r) {
    return r >= RISCV_Reg::__ft0 && r <= RISCV_Reg::__ft11;
}

inline bool is_callersave(RISCV_Reg r) {
  return caller_save.count(r);
}

inline bool is_calleesave(RISCV_Reg r) {
  return callee_save.count(r);
}

} // namespace backend
} // namespace aaac
#endif // AAAC_RISCV_H
