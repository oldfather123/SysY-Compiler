#include "../../include/riscv/riscv.h"

#include <cassert>

namespace riscv {

// std::string to_string(reg x) {
//     switch (x) {
//     // Integer registers
//     case x0: return "x0";
//     case x1: return "x1";
//     case x2: return "x2";
//     case x3: return "x3";
//     case x4: return "x4";
//     case x5: return "x5";
//     case x6: return "x6";
//     case x7: return "x7";
//     case x8: return "x8";
//     case x9: return "x9";
//     case x10: return "x10";
//     case x11: return "x11";
//     case x12: return "x12";
//     case x13: return "x13";
//     case x14: return "x14";
//     case x15: return "x15";
//     case x16: return "x16";
//     case x17: return "x17";
//     case x18: return "x18";
//     case x19: return "x19";
//     case x20: return "x20";
//     case x21: return "x21";
//     case x22: return "x22";
//     case x23: return "x23";
//     case x24: return "x24";
//     case x25: return "x25";
//     case x26: return "x26";
//     case x27: return "x27";
//     case x28: return "x28";
//     case x29: return "x29";
//     case x30: return "x30";
//     case x31: return "x31";

//     // Floating-point registers
//     case f0: return "f0";
//     case f1: return "f1";
//     case f2: return "f2";
//     case f3: return "f3";
//     case f4: return "f4";
//     case f5: return "f5";
//     case f6: return "f6";
//     case f7: return "f7";
//     case f8: return "f8";
//     case f9: return "f9";
//     case f10: return "f10";
//     case f11: return "f11";
//     case f12: return "f12";
//     case f13: return "f13";
//     case f14: return "f14";
//     case f15: return "f15";
//     case f16: return "f16";
//     case f17: return "f17";
//     case f18: return "f18";
//     case f19: return "f19";
//     case f20: return "f20";
//     case f21: return "f21";
//     case f22: return "f22";
//     case f23: return "f23";
//     case f24: return "f24";
//     case f25: return "f25";
//     case f26: return "f26";
//     case f27: return "f27";
//     case f28: return "f28";
//     case f29: return "f29";
//     case f30: return "f30";
//     case f31: return "f31";

//     default: assert(false && "unknown reg");
//     }
// }

std::string to_string(reg x) {
    switch (x) {
    // Integer registers
    case x0: return "zero";
    case x1: return "ra";    // return address
    case x2: return "sp";    // stack pointer
    case x3: return "gp";    // global pointer
    case x4: return "tp";    // thread pointer
    case x5: return "t0";    // temporary
    case x6: return "t1";    // temporary
    case x7: return "t2";    // temporary
    case x8: return "fp";    // saved register (frame pointer) that is "s0"
    case x9: return "s1";    // saved register
    case x10: return "a0";   // argument/return value
    case x11: return "a1";   // argument
    case x12: return "a2";   // argument
    case x13: return "a3";   // argument
    case x14: return "a4";   // argument
    case x15: return "a5";   // argument
    case x16: return "a6";   // argument
    case x17: return "a7";   // argument
    case x18: return "s2";   // saved register
    case x19: return "s3";   // saved register
    case x20: return "s4";   // saved register
    case x21: return "s5";   // saved register
    case x22: return "s6";   // saved register
    case x23: return "s7";   // saved register
    case x24: return "s8";   // saved register
    case x25: return "s9";   // saved register
    case x26: return "s10";  // saved register
    case x27: return "s11";  // saved register
    case x28: return "t3";   // temporary
    case x29: return "t4";   // temporary
    case x30: return "t5";   // temporary
    case x31: return "t6";  // temporary

    // Floating-point registers
    case f0: return "ft0";
    case f1: return "ft1";
    case f2: return "ft2";
    case f3: return "ft3";
    case f4: return "ft4";
    case f5: return "ft5";
    case f6: return "ft6";
    case f7: return "ft7";

    case f8: return "fs0";  // saved register (frame pointer) that is "fs0"
    case f9: return "fs1";  // saved register

    case f10: return "fa0";  // argument/return value
    case f11: return "fa1";  // argument
    case f12: return "fa2";  // argument
    case f13: return "fa3";  // argument
    case f14: return "fa4";  // argument
    case f15: return "fa5";  // argument
    case f16: return "fa6";  // argument
    case f17: return "fa7";  // argument

    case f18: return "fs2";  // saved register
    case f19: return "fs3";  // saved register
    case f20: return "fs4";  // saved register
    case f21: return "fs5";  // saved register
    case f22: return "fs6";  // saved register
    case f23: return "fs7";  // saved register
    case f24: return "fs8";  // saved register
    case f25: return "fs9";  // saved register
    case f26: return "fs10"; // saved register
    case f27: return "fs11"; // saved register
    case f28: return "ft8";  // temporary
    case f29: return "ft9";  // temporary
    case f30: return "ft10"; // temporary
    case f31: return "ft11"; // temporary
    // case NONE: return "NONE";  // Special value for unassigned registers
    default: assert(false && "unknown reg");
    }
}

std::string to_string(opcode op) {
    switch (op) {
    case opcode::lui: return "lui";
    case opcode::auipc: return "auipc";
    case opcode::jal: return "jal";
    case opcode::jalr: return "jalr";
    case opcode::beq: return "beq";
    case opcode::bne: return "bne";
    case opcode::blt: return "blt";
    case opcode::bge: return "bge";
    case opcode::bltu: return "bltu";
    case opcode::bgeu: return "bgeu";
    case opcode::lb: return "lb";
    case opcode::lh: return "lh";
    case opcode::lw: return "lw";
    case opcode::lbu: return "lbu";
    case opcode::lhu: return "lhu";
    case opcode::sb: return "sb";
    case opcode::sh: return "sh";
    case opcode::sw: return "sw";
    case opcode::addi: return "addi";
    case opcode::slti: return "slti";
    case opcode::sltiu: return "sltiu";
    case opcode::xori: return "xori";
    case opcode::ori: return "ori";
    case opcode::andi: return "andi";
    case opcode::slli: return "slli";
    case opcode::srli: return "srli";
    case opcode::srai: return "srai";
    case opcode::add: return "add";
    case opcode::sub: return "sub";
    case opcode::sll: return "sll";
    case opcode::slt: return "slt";
    case opcode::sltu: return "sltu";
    case opcode::xor_: return "xor";
    case opcode::srl: return "srl";
    case opcode::sra: return "sra";
    case opcode::or_: return "or";
    case opcode::and_: return "and";
    case opcode::sh1add: return "sh1add";
    case opcode::sh2add: return "sh2add";
    case opcode::sh3add: return "sh3add";
    case opcode::fence: return "fence";
    case opcode::fence_i: return "fence.i";
    case opcode::ecall: return "ecall";
    case opcode::ebreak: return "ebreak";
    case opcode::csrrw: return "csrrw";
    case opcode::csrrs: return "csrrs";
    case opcode::csrrc: return "csrrc";
    case opcode::csrrwi: return "csrrwi";
    case opcode::csrrsi: return "csrrsi";
    case opcode::csrrci: return "csrrci";
    case opcode::mul: return "mul";
    case opcode::mulh: return "mulh";
    case opcode::mulhsu: return "mulhsu";
    case opcode::mulhu: return "mulhu";
    case opcode::div: return "div";
    case opcode::divu: return "divu";
    case opcode::rem: return "rem";
    case opcode::remu: return "remu";
    case opcode::lwu: return "lwu";
    case opcode::ld: return "ld";
    case opcode::sd: return "sd";
    case opcode::addiw: return "addiw";
    case opcode::slliw: return "slliw";
    case opcode::srliw: return "srliw";
    case opcode::sraiw: return "sraiw";
    case opcode::addw: return "addw";
    case opcode::subw: return "subw";
    case opcode::sllw: return "sllw";
    case opcode::srlw: return "srlw";
    case opcode::sraw: return "sraw";
    case opcode::mulw: return "mulw";
    case opcode::divw: return "divw";
    case opcode::divuw: return "divuw";
    case opcode::remw: return "remw";
    case opcode::remuw: return "remuw";
    case opcode::flw: return "flw";
    case opcode::fsw: return "fsw";
    case opcode::fmadd_s: return "fmadd.s";
    case opcode::fmsub_s: return "fmsub.s";
    case opcode::fnmsub_s: return "fnmsub.s";
    case opcode::fnmadd_s: return "fnmadd.s";
    case opcode::fadd_s: return "fadd.s";
    case opcode::fsub_s: return "fsub.s";
    case opcode::fmul_s: return "fmul.s";
    case opcode::fdiv_s: return "fdiv.s";
    case opcode::fsqrt_s: return "fsqrt.s";
    case opcode::fsgnj_s: return "fsgnj.s";
    case opcode::fsgnjn_s: return "fsgnjn.s";
    case opcode::fsgnjx_s: return "fsgnjx.s";
    case opcode::fmin_s: return "fmin.s";
    case opcode::fmax_s: return "fmax.s";
    case opcode::fcvt_w_s: return "fcvt.w.s";
    case opcode::fcvt_wu_s: return "fcvt.wu.s";
    case opcode::fcvt_s_w: return "fcvt.s.w";
    case opcode::fcvt_s_wu: return "fcvt.s.wu";
    case opcode::fmv_x_s: return "fmv.x.s";
    case opcode::fmv_s_x: return "fmv.s.x";
    case opcode::feq_s: return "feq.s";
    case opcode::flt_s: return "flt.s";
    case opcode::fle_s: return "fle.s";
    case opcode::fclass_s: return "fclass.s";
    case opcode::fld: return "fld";
    case opcode::fsd: return "fsd";
    case opcode::fmadd_d: return "fmadd.d";
    case opcode::fmsub_d: return "fmsub.d";
    case opcode::fnmsub_d: return "fnmsub.d";
    case opcode::fnmadd_d: return "fnmadd.d";
    case opcode::fadd_d: return "fadd.d";
    case opcode::fsub_d: return "fsub.d";
    case opcode::fmul_d: return "fmul.d";
    case opcode::fdiv_d: return "fdiv.d";
    case opcode::fsqrt_d: return "fsqrt.d";
    case opcode::fsgnj_d: return "fsgnj.d";
    case opcode::fsgnjn_d: return "fsgnjn.d";
    case opcode::fsgnjx_d: return "fsgnjx.d";
    case opcode::fmin_d: return "fmin.d";
    case opcode::fmax_d: return "fmax.d";
    case opcode::fcvt_s_d: return "fcvt.s.d";
    case opcode::fcvt_d_s: return "fcvt.d.s";
    case opcode::fcvt_w_d: return "fcvt.w.d";
    case opcode::fcvt_wu_d: return "fcvt.wu.d";
    case opcode::fcvt_d_w: return "fcvt.d.w";
    case opcode::fcvt_d_wu: return "fcvt.d.wu";
    case opcode::fmv_x_d: return "fmv.x.d";
    case opcode::fmv_d_x: return "fmv.d.x";
    case opcode::feq_d: return "feq.d";
    case opcode::flt_d: return "flt.d";
    case opcode::fle_d: return "fle.d";
    case opcode::fclass_d: return "fclass.d";
    case opcode::nop: return "nop";
    case opcode::li: return "li";
    case opcode::la: return "la";
    case opcode::mv: return "mv";
    case opcode::not_: return "not";
    case opcode::neg: return "neg";
    case opcode::j: return "j";
    case opcode::ret: return "ret";
    case opcode::call: return "call";
    case opcode::tail: return "tail";
    case opcode::seqz: return "seqz";
    case opcode::snez: return "snez";
    case opcode::sltz: return "sltz";
    case opcode::sgtz: return "sgtz";
    case opcode::fmv_s: return "fmv.s";
    case opcode::fmv_d: return "fmv.d";
    case opcode::frcsr: return "frcsr";
    case opcode::fscsr: return "fscsr";
    case opcode::frrm: return "frrm";
    case opcode::fsrm: return "fsrm";
    case opcode::frflags: return "frflags";
    case opcode::fsflags: return "fsflags";
    case opcode::unknown: return "unknown";
    default: assert(false && "unknown opcode");
    }
}

}  // namespace riscv
