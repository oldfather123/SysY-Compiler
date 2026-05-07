#pragma once
#ifndef RV_Instruction_H
#define RV_Instruction_H

#include <iostream>
#include <string>


//RV32I指令集合
enum INSTRUCTIONS {
	//RV32I
	//RV32I-R
	ADD, SUB, SLL, SLT, SGT, SLTU, SGTU, XOR, SRL, SRA, OR, AND,
	//RV64-32位
	SLLW, SRLW, SRAW, ADDW, SUBW,
	//RV32I-I
	JALR, LB, LH, LW, LBU, LHU, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
	//RV64-32位
	SLLIW, SRLIW, SRAIW, ADDIW, LWU, LD,
	//RV32I-S
	SB, SH, SW,
	//RV64
	SD,
	//RV32I-B
	BEQ, BNE, BLT, BGE, BLTU, BGEU,
	//RV32I-U
	LUI, AUIPC,
	//RV32I-J
	JAL,

	//RV32M
	//乘法指令
	MUL, MULH, MULHU, MULHSU, MULW,
	//除法指令
	DIV, DIVU, DIVW, DIVUW,
	//取余指令
	REM, REMU, REMW, REMUW,

	//RV32F
	//加载存储指令
	FLW, FSW, FLD, FSD,
	//算数指令,单精度
	FADDS, FSUBS, FMULS, FDIVS, FSQRTS,
	//算数指令,双精度
	FADDD, FSUBD, FMULD, FDIVD, FSQRTD,
	//多算数操作指令,单精度
	FMADDS, FMSUBS, FNMADDS, FNMSUBS,
	//多算数操作指令,双精度
	FMADDD, FMSUBD, FNMADDD, FNMSUBD,
	//最大最小值指令
	FMINS, FMAXS, FMIND, FMAXD,
	//比较指令，单精度
	FEQS, FLTS, FLES, FGTS, FGES,
	//比较指令，双精度
	FEQD, FLTD, FLED, FGTD, FGED,
	//分类指令
	FCLASSS, FCLASSD,
	//浮点注入指令
	FSGNJS, FSGNJNS, FSGNJXS,
	FSGNJD, FSGNJND, FSGNJXD,
	//浮点转换指令
	FCVTSW, FCVTDW, FCVTSWU, FCVTDWU,
	FCVTWS, FCVTWD, FCVTWUS, FCVTWUD,
	FCVTSD, FCVTDS,
	//浮点搬运指令
	FMVXW, FMVWX,

	//伪指令
	LA, LLA, P_LB, P_LH, P_LW, P_LD,
	P_SB, P_SH, P_SW, P_SD, P_FLW, P_FLD, P_FSW, P_FSD,
	LI,
	NOP,
	MV, NOT, NEG, NEGW, SEXTW, SEQZ, SNEZ, SLTZ, SGTZ,
	FMVS, FABSS, FNEGS, FMVD, FABSD, FNEGD,
	BEQZ, BNEZ, BLEZ, BGEZ, BLTZ, BGTZ,
	BGT, BLE, BGTU, BLEU,
	J, P_JAL, JR, P_JALR, RET, CALL, TAIL,

	null,
	Inst_num
};

//RV32I指令名称
const std::string instructions[Inst_num-1] = {
	//RV32I
	"add", "sub", "sll", "slt", "sgt", "sltu", "sgtu", "xor", "srl", "sra", "or", "and",
	"sllw", "srlw", "sraw", "addw", "subw",
	"jalr", "lb", "lh", "lw", "lbu", "lhu", "addi", "slti", "sltiu", "xori", "ori", "andi", "slli", "srli", "srai",
	"slliw", "srliw", "sraiw", "addiw", "lwu", "ld",
	"sb", "sh", "sw",
	"sd",
	"beq", "bne", "blt", "bge", "bltu", "bgeu",
	"lui", "auipc",
	"jal",

	//RV32M
	"mul", "mulh", "mulhu", "mulhsu", "mulw",
	"div", "divu", "divw", "divuw",
	"rem", "remu", "remw", "ewmuw",

	//RV32F
	"flw", "fsw", "fld", "fsd",
	"fadd.s", "fsub.s", "fmul.s", "fdiv.s", "fsqrt.s",
	"fadd.d", "fsub.d", "fmul.d", "fdiv.d", "fsqrt.d",
	"fmadd.s", "fmsub.s", "fnmadd.s", "fnmsub.s",
	"fmadd.d", "fmsub.d", "fnmadd.d", "fnmsub.d",
	"fmin.s", "fmax.s", "fmin.d", "fmax.d",
	"feq.s", "flt.s", "fle.s", "fgt.s", "fge.s",
	"feq.d", "flt.d", "fle.d", "fgt.d", "fge.z",
	"fclass.s", "fclass.d",
	"fsgnj.s", "fsgnjn.s", "fsgnjx.s",
	"fsgnj.d", "fsgnjn.d", "fsgnjx.d",
	"fcvt.s.w", "fcvt.d.w", "fcvt.s.wu", "fcvt.d.wu",
	"fcvt.w.s", "fcvt.w.d", "fcvt.wu.s", "fcvt.wu.d",
	"fcvt.s.d", "fcvt.d.s",
	"fmv.x.w", "fmv.w.x",

	//伪指令
	"la", "lla", "lb", "lh", "lw", "ld",
	"sb", "sh", "sw", "sd", "flw", "fld", "fsw", "fsd",
	"li", "nop",
	"mv", "not", "neg", "negw", "sext.w", "seqz", "snez", "sltz", "sgtz",
	"fmv.s", "fabs.s", "fneg.s", "fmv.d", "fabs.d", "fneg.d",
	"beqz", "bnez", "blez", "bgez", "bltz", "bgtz",
	"bgt", "ble", "bgtu", "bleu",
	"j", "jal", "jr", "jalr", "ret", "call", "tail"
};

enum InstrunicsTypes {
	IType,
	RType,
	JType,
	UType,
	SType,
	BType
};

//判断立即数是否符合要求
bool isLegalImm(InstrunicsTypes type, int iimm);

int f32Toi32(float f);


#endif
