#ifndef ARM_H
#define ARM_H

#include <string.h>

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h> // vprintf printf
#include <stdlib.h> // exit
#include <assert.h>
#include <utility> // std::pair
#include <map>
#include <set>
#include <queue>
#include<iostream>

#include <cstring> // strcpy, strlen

#include "array.h"
#include "general.h"
#include "Module.h"

using namespace std;



enum Int_Reg : uint8 {
    r0 = 0,
    r1 = 1,
    r2 = 2,
    r3 = 3,
    r4 = 4,
    r5 = 5,
    r6 = 6,
    r7 = 7,
    r8 = 8,
    r9 = 9,
    r10 = 10,
    r11 = 11,

    r12 = 12,
    r13 = 13,
    r14 = 14,
    r15 = 15,
    REG_COUNT = 16,

    // alias
    fp = r11,
    ip = r12,
    sp = r13,
    lr = r14,
    pc = r15,

};
// 浮点数寄存器
enum float_reg : uint8{
    s0 = 0,
    s1 = 1,
    s2 = 2,
    s3 = 3,
    s4 = 4,
    s5 = 5,
    s6 = 6,
    s7 = 7,
    s8 = 8,
    s9 = 9,
    s10 = 10,
    s11 = 11,
    s12 = 12,
    s13 = 13,
    s14 = 14,
    s15 = 15,
    SREG_COUNT = 16
};

union float2int
{
    int bin_value;
    float f_value;
};

enum Binary_Op_Type {
	BINARY_ADD,
	BINARY_SUBTRACT,
	BINARY_MULTIPLY,
	BINARY_SMMUL,
	BINARY_DIVIDE,
	BINARY_MOD,
    F_NEG,

	/* ASM BINARY INSTRUCTIONS */
	BINARY_LSL,
	BINARY_LSR,
	BINARY_ASL,
	BINARY_ASR,
	BINARY_RSB, // reverse sub for machine instruction
	BINARY_BITWISE_AND, // bitwise or 
	BINARY_BITWISE_OR, // bitwise or 
	BINARY_BIC,

    BINARY_LOGICAL_AND,
    BINARY_LOGICAL_OR,
    BINARY_NOT_EQUAL_TO,
    BINARY_EQUAL_TO,
    BINARY_LESS_THAN_OR_EQUAL_TO,
    BINARY_GREATER_THAN_OR_EQUAL_TO,
    BINARY_LESS_THAN,
    BINARY_GREATER_THAN,
    UNARY_XOR,
    FLOAT_GE
};


//
enum Operand_Tag {
	//UNDEFINED,
	ERRORTYPE,
	REG, // physical register. allocated or preallocated
	VREG, // virtual register, waiting to be allocated
    SREG, // physical float register
    VSREG, // virtual float register
	IMM,
    SIMM,
	ADR_GLOBAL
};


enum Shift_Tag {
	Nothing,
	LSL,
	LSR,
	ASR,
	ASL
};

enum Branch_Condition {
    NO_CONDITION          = 0,

    LESS_THAN             = 1,
    GREATER_THAN          = 2,
    NOT_EQUAL             = 3,

    GREATER_THAN_OR_EQUAL = 4,
    LESS_THAN_OR_EQUAL    = 5,
    EQUAL                 = 6,
    float_ge              = 7,
    float_lt              = 8,
    float_ne              = 9
};

struct MOperand {
    Operand_Tag tag = ERRORTYPE;

    /* @note
     * for tag == STACK, value is the imm offset to sp register
     * e.g. value=4 means [sp+4]
     */
    int32 value; 
	Shift_Tag s_tag = Nothing; // @TODO This doesn't belongs to an operand?
	uint8 s_value = 0;
	const char* adr = NULL;
    MOperand() {}
	MOperand(Operand_Tag tag, const char* adr) : tag(tag), adr(adr) {};
    MOperand(Operand_Tag tag, int32 value) : tag(tag), value(value) {};
	MOperand(Operand_Tag tag, int32 value, Shift_Tag s_tag, uint8 s_value) : tag(tag), value(value), s_tag(s_tag), s_value(s_value) {};
    bool operator<(const MOperand &b) const {
        if (tag != b.tag) return tag < b.tag;

        if (tag == REG || tag == VREG || tag == IMM || tag == SREG ||tag == VSREG || tag == SIMM) {
            return value < b.value;
        }

        if (tag == ADR_GLOBAL) {
            return adr < b.adr;
        }

        assert(false);
        return false;
    }

    bool operator==(const MOperand &b) const {
        if (tag != b.tag) return false;
        if (tag == REG || tag == VREG || tag == IMM || tag == SREG ||tag == VSREG || tag == SIMM) return value == b.value;
        if (tag == ADR_GLOBAL) return adr == b.adr;
        assert(false);
        return false;
    }
};

MOperand make_imm(int32 constant);
MOperand make_simm(float constant);
MOperand make_reg(uint8 reg);
MOperand make_vreg(int32 vreg_index);
MOperand make_sreg(uint8 sreg);
MOperand make_vsreg(int32 vsreg_index);


enum MI_Tag {
    MI_MOVE,
    MI_BINARY,
    MI_CLZ,
    MI_RETURN,
    MI_BRANCH,
    MI_COMPARE,
    MI_FUNC_CALL, // bl func
    MI_PUSH,
    MI_POP,
    MI_LOAD, // ldr
    MI_STORE, // str
    // next is float use
    MI_VMOVE, //
    MI_VBINARY,
    MI_VCOMPARE,
    MI_VPUSH,
    MI_VPOP,
    MI_VLOAD, // vldr
    MI_VSTORE,  // vstr
    MI_VCVT   // convert
};


struct Machine_Block;
/* Machine Instruction */
/* Instructions are stored in machine block as a linked list
 * to simplify inserting */
struct MI {
    MI_Tag tag;
    MI *prev = NULL, *next = NULL;
    Machine_Block *mb = NULL;
    Branch_Condition cond = NO_CONDITION;
    bool32 marked = false;
    bool32 update_flags = false;

    int32 n = -1; // for linear scan

    MI(MI_Tag tag) : tag(tag) {}
    MI(MI_Tag tag, Branch_Condition cond) : tag(tag), cond(cond) {}
    MI *erase_from_parent();
    void mark() { marked = true; }
};

struct MI_Move : MI {
    MOperand dst;
    MOperand src;
    bool neg = false; // mvn

    MI_Move(MOperand dst, MOperand src) : MI(MI_MOVE), dst(dst), src(src) {};
};

struct MI_Clz : MI {
    MOperand dst;
    MOperand operand;

    MI_Clz() : MI(MI_CLZ) {};
};


struct MI_Move_Pointer_Cmp {
    bool operator()(const MI_Move *lhs, const MI_Move *rhs) const {
        if (!(lhs->dst == rhs->dst)) return (lhs->dst) < (rhs->dst);
        if (!(lhs->neg == rhs->neg)) return (lhs->neg) < (rhs->neg);
        return (lhs->src) < (rhs->src);
    }
};

struct MI_VMove_Pointer_Cmp
{
    bool operator()(const MI_Move *lhs, const MI_Move *rhs) const {
        if (!(lhs->dst == rhs->dst)) return (lhs->dst) < (rhs->dst);
        if (!(lhs->neg == rhs->neg)) return (lhs->neg) < (rhs->neg);
        return (lhs->src) < (rhs->src);
    }
};


struct MI_Binary : MI {
    Binary_Op_Type op;
    MOperand dst; 
    MOperand lhs, rhs;

	MI_Binary() : MI(MI_BINARY) {}
    MI_Binary(Binary_Op_Type op, MOperand dst, MOperand lhs, MOperand rhs) : 
        MI(MI_BINARY), op(op), dst(dst), lhs(lhs), rhs(rhs) {};
};

struct MI_Compare : MI {
    MOperand lhs, rhs;
    bool neg = false;

    MI_Compare(MOperand lhs, MOperand rhs) : 
        MI(MI_COMPARE), lhs(lhs), rhs(rhs) {};
};

struct MI_Func_Call : MI {
    const char *func_name;
    int arg_count = 0;

    MI_Func_Call(const char *func_name) : MI(MI_FUNC_CALL), func_name(func_name) {};
};

struct MI_Branch : MI {
    Machine_Block *true_target;
    Machine_Block *false_target;

    MI_Branch() : MI(MI_BRANCH) {};
    MI_Branch(Branch_Condition cond, Machine_Block *true_target, Machine_Block *false_target=NULL) : 
        MI(MI_BRANCH, cond), true_target(true_target), false_target(false_target) {};
};

struct MI_Return : MI {
    MI_Return() : MI(MI_RETURN) {};
};

// @note: the struct layout of MI_Push and MI_Pop
// must be the same.
struct MI_Push : MI {
    Array<MOperand> operands;
    MI_Push() : MI(MI_PUSH) {};
};

struct MI_Pop : MI {
    Array<MOperand> operands;
    MI_Pop() : MI(MI_POP) {};
};

struct MI_VPush : MI {
    Array<MOperand> operands;
    MI_VPush() : MI(MI_VPUSH) {};
};

struct MI_VPop : MI {
    Array<MOperand> operands;
    MI_VPop() : MI(MI_VPOP) {};
};

enum Mem_Tag {
    MEM_UNDEFINED,
    MEM_ARRAY,
    MEM_LOAD_ARG,
    MEM_LOAD_GLOBAL_REF,
    MEM_LOAD_FROM_LITERAL_POOL,
    MEM_LOAD_SPILL,
    MEM_SAVE_SPILL,
    MEM_PREP_ARG
};

// @note: the struct layout of MI_Load and MI_Store
// must be the same.
struct MI_Load : MI {
    Mem_Tag mem_tag = MEM_UNDEFINED;
    MOperand reg;
    MOperand base;
	MOperand offset;
    MI_Load() : MI(MI_LOAD) {};
};

// str
struct MI_Store : MI {
    Mem_Tag mem_tag = MEM_UNDEFINED;
    MOperand reg;
    MOperand base;
	MOperand offset;
    MI_Store() : MI(MI_STORE) {};
};

// -------------------------float
struct MI_VMove : MI {
    MOperand dst;
    MOperand src;
    bool both_float = false; // vmov.f32

    MI_VMove() : MI(MI_VMOVE){};
    MI_VMove(MOperand dst, MOperand src, bool both) : MI(MI_VMOVE), dst(dst), src(src) ,both_float(both){};

};

struct MI_VBinary : MI {
    Binary_Op_Type op;
    MOperand dst;
    MOperand lhs, rhs;
    int fneg=0;

    MI_VBinary() : MI(MI_VBINARY) {}
    MI_VBinary(Binary_Op_Type op, MOperand dst, MOperand lhs, MOperand rhs) : MI(MI_VBINARY), op(op), dst(dst), lhs(lhs), rhs(rhs) {};
};

struct MI_VCompare : MI {
    MOperand lhs, rhs;
    bool neg = false;

    MI_VCompare(MOperand lhs, MOperand rhs) : MI(MI_VCOMPARE), lhs(lhs), rhs(rhs) {}
};



struct MI_VLoad : MI {
    Mem_Tag mem_tag = MEM_UNDEFINED;
    MOperand reg;
    MOperand base;
    MOperand offset;
    MI_VLoad() : MI(MI_VLOAD) {};
};

struct MI_VStore : MI {
    Mem_Tag mem_tag = MEM_UNDEFINED;
    MOperand reg;
    MOperand base;
	MOperand offset;
    MI_VStore() : MI(MI_VSTORE) {};
};

enum Vcvt_Type {
    U32 = 0, // 无符号32位整数
    S32 = 1, // 有符号32位整数
    F32 = 2  // 浮点数
};
struct MI_VCvt : MI {
    Vcvt_Type from_type;
    Vcvt_Type to_type;
    MOperand dst;
    MOperand src;
    MI_VCvt() : MI(MI_VCVT) {};
    MI_VCvt(Vcvt_Type from, Vcvt_Type to) : MI(MI_VCVT), from_type(from), to_type(to) {};
};

struct Machine_Block {
    int32 i = -1;
    MI *inst = NULL;
    MI *last_inst = NULL;
    MI *control_transfer_inst = NULL;
    Array<Machine_Block *> preds;
    Array<Machine_Block *> succs; // at most two successors, can be optimized for @Performance

    // for bb placement pass
    bool visited = false;
    bool condified = false;

    LabelPtr label;

    uint32 loop_depth = -1;
    int32 belongs_to_loop = -1;

    void erase_marked_values();
};

struct Func_Asm {
    int vreg_count = 0;
    int vsreg_count = 0;
	int index;
	int stack_size = 0;
    bool has_return_value = true;
    bool too_many_globals = false; // max_flow WA

    string name;
    Array<Machine_Block *> mbs;
	Array<const char *> global_value;

    Array<MI *> local_array_bases; // need to fixup sp of these
};

struct Program_Asm {
    Array<Func_Asm *> functions;
};

void insert(MI *mi, MI *before);
void push(MI *mi, Machine_Block *mb);
bool can_be_imm_ror(int32 x);
bool can_be_imm12(int32 x);
MI_Load *emit_load_of_constant(MOperand vreg, int32 constant);

Func_Asm *emit_function_asm(Module func_IR, int i, vector<VariablePtr> &globalValues);
Program_Asm *emit_asm(Module program_IR);
bool is_callee_save(uint8 reg);
bool is_caller_save(uint8 reg);
bool s_is_callee_save(uint8 reg);
bool s_is_caller_save(uint8 reg);
void build_cvt_type(String_Builder* s, Vcvt_Type t);

void build_operand(String_Builder *s, MOperand op);
void build_function_asm(String_Builder *s, Func_Asm *func);
void build_program_asm(String_Builder *s, Program_Asm *pro, vector<VariablePtr> & globalVariables);

void print_operand(MOperand op);
void print_function_asm(Func_Asm *func);
void print_program_asm(Program_Asm *pro, vector<VariablePtr> & globalVariables);
Branch_Condition invert_branch_cond(Branch_Condition cond);
#endif
