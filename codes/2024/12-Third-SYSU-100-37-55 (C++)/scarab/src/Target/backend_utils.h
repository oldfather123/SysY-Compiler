#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <utility>
#include <map>
#include <set>
#include <queue>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include "Module.h"
#include <filesystem>

#include <vector>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

template <typename A, typename B>
using Pair = std::pair<A, B>;

template <typename A, typename B>
using Map = std::map<A, B>;

template <typename A, typename Compare = std::less<A>>
using Set = std::set<A, Compare>;

template <typename A>
using Queue = std::queue<A>;

struct Asm_Buffer {
    std::vector<char> buffer;

    size_t size() { return buffer.size(); }
    char *c_str() { return buffer.data(); }
    void add_terminator() { buffer.push_back('\0'); }
    void print(const char *s, ...);
    void read_from_file(const char* filename);
};


void report_error(const char *format, ...);

const int REG_COUNT = 32;
const int FREG_COUNT = 32;

enum Int_Reg : uint8 {
    x1 = 1,
    x2 = 2,
    x3 = 3,
    x4 = 4,
    x5 = 5,
    x6 = 6,
    x7 = 7,
    x8 = 8,
    x9 = 9,
    x10 = 10,
    x11 = 11,
    x12 = 12,
    x13 = 13,
    x14 = 14,
    x15 = 15,
    x16 = 16,
    x17 = 17,
    x18 = 18,
    x19 = 19,
    x20 = 20,
    x21 = 21,
    x22 = 22,
    x23 = 23,
    x24 = 24,
    x25 = 25,
    x26 = 26,
    x27 = 27,
    x28 = 28,
    x29 = 29,
    x30 = 30,
    x31 = 31,
    pc = 33,

    zero = 0,
    ra = x1,
    sp = x2,
    gp = x3,
    tp = x4,
    t0 = x5,
    t1 = x6,
    t2 = x7,
    fp = x8,
    s0 = x8,
    s1 = x9,
    a0 = x10,
    a1 = x11,
    a2 = x12,
    a3 = x13,
    a4 = x14,
    a5 = x15, 
    a6 = x16,
    a7 = x17,
    s2 = x18,
    s3 = x19,
    s4 = x20,
    s5 = x21,
    s6 = x22,
    s7 = x23,
    s8 = x24,
    s9 = x25,
    s10 = x26,
    s11 = x27,
    t3 = x28,
    t4 = x29,
    t5 = x30,
    t6 = x31
};
// 浮点数寄存器
enum float_reg : uint8{
    f0 = 0,
    f1 = 1,
    f2 = 2,
    f3 = 3,
    f4 = 4,
    f5 = 5,
    f6 = 6,
    f7 = 7,
    f8 = 8,
    f9 = 9,
    f10 = 10,
    f11 = 11,
    f12 = 12,
    f13 = 13,
    f14 = 14,
    f15 = 15,
    f16 = 16,
    f17 = 17,
    f18 = 18,
    f19 = 19,
    f20 = 20,
    f21 = 21,
    f22 = 22,
    f23 = 23,
    f24 = 24,
    f25 = 25,
    f26 = 26,
    f27 = 27,
    f28 = 28,
    f29 = 29,
    f30 = 30,
    f31 = 31,

    
    fa0 = f10,
    fa1 = f11,
    fa2 = f12,
    fa3 = f13,
    fa4 = f14,
    fa5 = f15,
    fa6 = f16, 
    fa7 = f17,
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
	BINARY_DIVIDE,
	BINARY_MOD,
    BINARY_SH2ADD,
    F_NEG,

	BINARY_LSL,
	BINARY_LSR,
	BINARY_ASL,
	BINARY_ASR,
	BINARY_BITWISE_AND,
	BINARY_BITWISE_OR,
    BINARY_XOR,
    UNARY_XOR,
    ATOMIC_ADD

};


enum Operand_Tag {
	ERRORTYPE,
	REG,
	VREG,
    FREG,
    VFREG,
	IMM,
	ADR_GLOBAL
};

enum Branch_Condition {
    NO_CONDITION          = 0,

    LESS_THAN             = 1,
    GREATER_THAN          = 2,
    NOT_EQUAL             = 3,

    GREATER_THAN_OR_EQUAL = 4,
    LESS_THAN_OR_EQUAL    = 5,
    EQUAL                 = 6
};

enum VI_Tag {
    VI_MOVE=0,
    VI_BINARY,
    VI_CLZ,
    VI_RETURN,
    VI_BRANCH,
    VI_COMPARE,
    VI_SLT,
    VI_SEQZ,
    VI_SNEZ,
    VI_FUNC_CALL,
    VI_PUSH,
    VI_POP,
    VI_LOAD,
    VI_STORE,


    VI_FMOVE,
    VI_VBINARY,
    VI_FCMP_SET,
    VI_VPUSH,
    VI_VPOP,
    VI_VLOAD,
    VI_VSTORE,
    VI_VCVT,
    VI_FCAST
};

enum SLT_Tag {
    SLT,
    SLT_I,
    SLT_U,
    SLT_IU
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

enum Vcvt_Type {
    U32 = 0,
    S32 = 1,
    F32 = 2
};

struct VI;
struct VI_Use;
struct VOper;


struct MachineBlock {
    int32 index = -1;
    VI *inst = NULL;
    VI *last_inst = NULL;
    VI *control_transfer_inst = NULL;
    vector<MachineBlock *> preds;
    vector<MachineBlock *> succs;
    
    bool visited = false;
    bool condified = false;

    LabelPtr label;

    uint32 loop_depth = -1;

    map<pair<VOper, VOper>, int32> vopr_offset_map;
    void erase_marked_values();
};

struct Func_Asm {
    int vreg_num = 0;
    int vfreg_num = 0;
	int index;
	int stack_size = 0;
    int reg_needs_save = 0;
    int sreg_needs_save = 0;
    int reg_spill_size = 0;
    int sreg_spill_size = 0;

    string name;
    vector<MachineBlock *> mbs;
	vector<const char *> global_value;

    map<VOper, VI_Use *> use_head_map;
    map<VOper, VI_Use *> s_use_head_map;
    map<VOper, VI *> def_I_map;

    // yyy: 这个函数在alloc中会把对应的指令添加进来
    vector<VI *> local_array_bases; 
    // yyy: int_val_map 把vreg映射到constant
    map<VOper, int> int_val_map;

    void clear_use_head_map();
    void clear_def_I_map();
    int call_function_num=0;
};

struct Program_Asm {
    vector<Func_Asm *> functions;
};

struct VOper {
    Operand_Tag tag = ERRORTYPE;

    int32 value; 
	const char* adr = nullptr;
    VOper() {}
	VOper(Operand_Tag tag, const char* adr) : tag(tag), adr(adr){};
    VOper(Operand_Tag tag, int32 value) : tag(tag), value(value){};
    bool operator<(const VOper &b) const {
        if (tag != b.tag) return tag < b.tag;

        if (tag == REG || tag == VREG || tag == IMM || tag == FREG ||tag == VFREG) {
            return value < b.value;
        }

        if (tag == ADR_GLOBAL) {
            return adr < b.adr;
        }

        fprintf(stderr, "Operand_Tag: %d, value: %d, adr: %s\n", tag, value, adr);
        assert(false);
        return false;
    }

    bool operator==(const VOper &b) const {
        if (tag != b.tag) return false;
        if (tag == REG || tag == FREG || tag == VREG || tag == VFREG || tag == IMM) return value == b.value;
        if (tag == ADR_GLOBAL) return adr == b.adr;
        assert(false);
        return false;
    }

    VOper& operator=(const VOper &b) {
        if (this == &b) {
            return *this;
        }

        this->tag = b.tag;
        this->value = b.value;
        this->adr = b.adr;

        return *this;
    }

    void set_use_head(VI_Use *use, Func_Asm *func);
    void s_set_use_head(VI_Use *use, Func_Asm *func);
    VI_Use *get_use_head(Func_Asm *func);
    VI_Use *f_get_use_head(Func_Asm *func);
    void add_use(VI_Use *u, Func_Asm *func);
    void s_add_use(VI_Use *u, Func_Asm *func);
    VI *get_def_I(Func_Asm *func);
    void set_def_I(VI *I, Func_Asm *func);
};

struct VI {
    VI_Tag tag;
    VI *prev = NULL, *next = NULL;
    MachineBlock *mb = NULL;
    bool marked = false;
    bool is_dw = false; // yyy: is_double_word

    VI(){};
    VI(VI_Tag tag) : tag(tag) {}
    void erase_from_parent();
    void mark() { marked = true; }
};

// yyy: 在VI这条语句中,val被user用到了
struct VI_Use {
    VI_Use *prev = nullptr, *next = nullptr;
    VOper *val = nullptr;
    VI *I = nullptr;
    VOper *user = nullptr;
    VI_Use() {}
    VI_Use(VOper *val, VOper *user, VI *I): val(val), user(user), I(I) {}
    void rm_use(Func_Asm *func);
};

bool is_callee_save(uint8 reg);
bool is_caller_save(uint8 reg);
bool s_is_callee_save(uint8 r);
bool s_is_caller_save(uint8 sreg);
bool can_be_imm12(int32 x);
bool is_fcmp_cond(Branch_Condition cond);
bool is_float_moperand(VOper opr);
bool is_float_reg(VOper opr);
#endif
