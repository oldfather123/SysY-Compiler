#pragma once
#include <iostream>

namespace riscv {

enum reg {
    x0,
    x1,
    x2,
    x3,
    x4,
    x5,
    x6,
    x7,
    x8,
    x9,
    x10,
    x11,
    x12,
    x13,
    x14,
    x15,
    x16,
    x17,
    x18,
    x19,
    x20,
    x21,
    x22,
    x23,
    x24,
    x25,
    x26,
    x27,
    x28,
    x29,
    x30,
    x31,

    f0,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
    f13,
    f14,
    f15,
    f16,
    f17,
    f18,
    f19,
    f20,
    f21,
    f22,
    f23,
    f24,
    f25,
    f26,
    f27,
    f28,
    f29,
    f30,
    f31,
};

enum class opcode {
    // RV32I Base Instruction Set
    lui,      // Load Upper Immediate
    auipc,    // Add Upper Immediate to PC
    jal,      // Jump and Link
    jalr,     // Jump and Link Register
    beq,      // Branch Equal
    bne,      // Branch Not Equal
    blt,      // Branch Less Than
    bge,      // Branch Greater or Equal
    bltu,     // Branch Less Than Unsigned
    bgeu,     // Branch Greater or Equal Unsigned
    lb,       // Load Byte
    lh,       // Load Halfword
    lw,       // Load Word
    lbu,      // Load Byte Unsigned
    lhu,      // Load Halfword Unsigned
    sb,       // Store Byte
    sh,       // Store Halfword
    sw,       // Store Word
    addi,     // Add Immediate
    slti,     // Set Less Than Immediate
    sltiu,    // Set Less Than Immediate Unsigned
    xori,     // XOR Immediate
    ori,      // OR Immediate
    andi,     // AND Immediate
    slli,     // Shift Left Logical Immediate
    srli,     // Shift Right Logical Immediate
    srai,     // Shift Right Arithmetic Immediate
    add,      // Add
    sub,      // Subtract
    sll,      // Shift Left Logical
    slt,      // Set Less Than
    sltu,     // Set Less Than Unsigned
    xor_,     // XOR
    srl,      // Shift Right Logical
    sra,      // Shift Right Arithmetic
    or_,      // OR
    and_,     // AND
    fence,    // Fence
    fence_i,  // Fence Instruction
    ecall,    // Environment Call
    ebreak,   // Environment Break
    csrrw,    // CSR Read/Write
    csrrs,    // CSR Read and Set
    csrrc,    // CSR Read and Clear
    csrrwi,   // CSR Read/Write Immediate
    csrrsi,   // CSR Read and Set Immediate
    csrrci,   // CSR Read and Clear Immediate

    // RV32M Standard Extension for Integer Multiply/Divide
    mul,     // Multiply
    mulh,    // Multiply High Signed
    mulhsu,  // Multiply High Signed-Unsigned
    mulhu,   // Multiply High Unsigned
    div,     // Divide Signed
    divu,    // Divide Unsigned
    rem,     // Remainder Signed
    remu,    // Remainder Unsigned

    // RV64I Base Instruction Set Extensions
    lwu,    // Load Word Unsigned (64-bit)
    ld,     // Load Doubleword
    sd,     // Store Doubleword
    addiw,  // Add Immediate Word
    slliw,  // Shift Left Logical Immediate Word
    srliw,  // Shift Right Logical Immediate Word
    sraiw,  // Shift Right Arithmetic Immediate Word
    addw,   // Add Word
    subw,   // Subtract Word
    sllw,   // Shift Left Logical Word
    srlw,   // Shift Right Logical Word
    sraw,   // Shift Right Arithmetic Word

    // RV64M Standard Extension
    mulw,   // Multiply Word
    divw,   // Divide Signed Word
    divuw,  // Divide Unsigned Word
    remw,   // Remainder Signed Word
    remuw,  // Remainder Unsigned Word

    // Bitmanip Extension (Zba)
    sh1add, // Shift Left by 1 and Add (rs2 + (rs1 << 1))
    sh2add, // Shift Left by 2 and Add (rs2 + (rs1 << 2))
    sh3add, // Shift Left by 3 and Add (rs2 + (rs1 << 3))

    // RV32F Standard Extension for Single-Precision Floating-Point
    flw,        // Floating-point Load Word
    fsw,        // Floating-point Store Word
    fmadd_s,    // Fused Multiply-Add Single
    fmsub_s,    // Fused Multiply-Subtract Single
    fnmsub_s,   // Fused Negative Multiply-Subtract Single
    fnmadd_s,   // Fused Negative Multiply-Add Single
    fadd_s,     // Floating-point Add Single
    fsub_s,     // Floating-point Subtract Single
    fmul_s,     // Floating-point Multiply Single
    fdiv_s,     // Floating-point Divide Single
    fsqrt_s,    // Floating-point Square Root Single
    fsgnj_s,    // Floating-point Sign Injection Single
    fsgnjn_s,   // Floating-point Sign Injection Negative Single
    fsgnjx_s,   // Floating-point Sign Injection XOR Single
    fmin_s,     // Floating-point Minimum Single
    fmax_s,     // Floating-point Maximum Single
    fcvt_w_s,   // Convert Float to Word Single
    fcvt_wu_s,  // Convert Float to Unsigned Word Single
    fcvt_s_w,   // Convert Word to Float Single
    fcvt_s_wu,  // Convert Unsigned Word to Float Single
    fmv_x_s,    // Move Float to Integer Single
    fmv_s_x,    // Move Integer to Float Single
    feq_s,      // Floating-point Equal Single
    flt_s,      // Floating-point Less Than Single
    fle_s,      // Floating-point Less or Equal Single
    fclass_s,   // Floating-point Classify Single

    // RV32D Standard Extension for Double-Precision Floating-Point
    fld,        // Floating-point Load Double
    fsd,        // Floating-point Store Double
    fmadd_d,    // Fused Multiply-Add Double
    fmsub_d,    // Fused Multiply-Subtract Double
    fnmsub_d,   // Fused Negative Multiply-Subtract Double
    fnmadd_d,   // Fused Negative Multiply-Add Double
    fadd_d,     // Floating-point Add Double
    fsub_d,     // Floating-point Subtract Double
    fmul_d,     // Floating-point Multiply Double
    fdiv_d,     // Floating-point Divide Double
    fsqrt_d,    // Floating-point Square Root Double
    fsgnj_d,    // Floating-point Sign Injection Double
    fsgnjn_d,   // Floating-point Sign Injection Negative Double
    fsgnjx_d,   // Floating-point Sign Injection XOR Double
    fmin_d,     // Floating-point Minimum Double
    fmax_d,     // Floating-point Maximum Double
    fcvt_s_d,   // Convert Single to Double
    fcvt_d_s,   // Convert Double to Single
    fcvt_w_d,   // Convert Double to Word
    fcvt_wu_d,  // Convert Double to Unsigned Word
    fcvt_d_w,   // Convert Word to Double
    fcvt_d_wu,  // Convert Unsigned Word to Double
    fmv_x_d,    // Move Double to Integer
    fmv_d_x,    // Move Integer to Double
    feq_d,      // Floating-point Equal Double
    flt_d,      // Floating-point Less Than Double
    fle_d,      // Floating-point Less or Equal Double
    fclass_d,   // Floating-point Classify Double

    // Pseudo-instructions
    nop,      // No Operation (pseudo)
    li,       // Load Immediate (pseudo)
    la,       // Load Address (pseudo)
    mv,       // Move (pseudo)
    not_,     // Bitwise NOT (pseudo)
    neg,      // Negate (pseudo)
    j,        // Jump (pseudo)
    ret,      // Return (pseudo)
    call,     // Call (pseudo)
    tail,     // Tail call (pseudo)
    seqz,     // Set if Equal to Zero (pseudo)
    snez,     // Set if Not Equal to Zero (pseudo)
    sltz,     // Set if Less Than Zero (pseudo)
    sgtz,     // Set if Greater Than Zero (pseudo)
    fmv_s,    // Floating-point Move Single (pseudo)
    fmv_d,    // Floating-point Move Double (pseudo)
    frcsr,    // Read FP CSR (pseudo)
    fscsr,    // Write FP CSR (pseudo)
    frrm,     // Read FP Rounding Mode (pseudo)
    fsrm,     // Write FP Rounding Mode (pseudo)
    frflags,  // Read FP Flags (pseudo)
    fsflags,  // Write FP Flags (pseudo)

    // Special value
    unknown,  // Unknown or invalid opcode
};

// ABI
const reg zero = x0;
const reg ra = x1;
const reg sp = x2;
const reg gp = x3;
const reg tp = x4;
const reg fp = x8;
const reg t[] = {x5, x6, x7, x28, x29, x30, x31};
const reg a[] = {x10, x11, x12, x13, x14, x15, x16, x17};
const reg s[] = {x8, x9, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27};

const reg ft[] = {f0, f1, f2, f3, f4, f5, f6, f7, f28, f29, f30, f31};

// Special register value for invalid/unassigned registers
const reg NONE = static_cast<reg>(-1);
const reg fa[] = {f10, f11, f12, f13, f14, f15, f16, f17};
const reg fs[] = {f8, f9, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27};

std::string to_string(reg r);
std::string to_string(opcode op);
}  // namespace riscv
