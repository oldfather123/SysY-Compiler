#ifndef SYSY_RISCV_H
#define SYSY_RISCV_H

#include <stdbool.h>
#include <stdio.h>

#include "../../include/function.h"
#include "../../include/ir.h"
#include "../../include/vector.h"

#define bytealigned(src, aligned) ((src + aligned - 1) & ~(aligned - 1))
#define __R \
    _X(a0)  \
    _X(a1)  \
    _X(a2)  \
    _X(a3)  \
    _X(a4)  \
    _X(a5)  \
    _X(a6)  \
    _X(a7)  \
    _X(s0)  \
    _X(s1)  \
    _X(s2)  \
    _X(s3)  \
    _X(s4) _X(s5) _X(s6) _X(s7) _X(s8) _X(s9) _X(s10) _X(s11)
#define __FR \
    _X(fa0)  \
    _X(fa1)  \
    _X(fa2)  \
    _X(fa3)  \
    _X(fa4)  \
    _X(fa5)  \
    _X(fa6)  \
    _X(fa7)  \
    _X(fs0)  \
    _X(fs1)  \
    _X(fs2)  \
    _X(fs3) _X(fs4) _X(fs5) _X(fs6) _X(fs7) _X(fs8) _X(fs9) _X(fs10) _X(fs11)

#define _X(op) REG_##op,

typedef enum Reg {
    __R REG_t0,
    REG_t1,
    REG_t2,
    REG_t3,
    __FR REG_sp,
    REG_ra,
    REG_zero,
    REG_fpt8,
    REG_fpt9,
    REG_fpt10,
    REG_fpt11,
} Reg;
#undef _X

extern const char* regName[];

#define REG_RET      REG_a0
#define FPREG_RET    REG_fa0

#define REG_CON      REG_t0
#define REG_SPILL    REG_t1
#define REG_SPILL2   REG_t2

#define FPREG_SPILL  REG_fpt8
#define FPREG_SPILL2 REG_fpt9

typedef struct liveRange {
    size_t begin;
    size_t end;
} liveRange;

typedef struct liveIntervals {
    bool   isFP;
    bool   isLoop;  // 循环内定义的变量
    int    loc;     // 溢出的局部地址
    bool   iscall;
    size_t callNum;  // 调用个数
    size_t callind;  // 函数调用的指令号
    vector_template(size_t, callinds);
    int8_t reg;
    vector_template(liveRange*, LRs);
    vector_template(uint32_t, ssaind);
    bool                  use;
    uint32_t              v;
    size_t                from, to;
    struct liveIntervals* join;
} liveIntervals;

typedef struct Register {
    const int8_t   reg;
    bool           used;
    bool           isused;
    int32_t        w;
    int            loc;  // 用于calee寄存器
    liveIntervals* ll;
    size_t         llind;
    int*           addr;
} Register;

// BUG 不要忘记了清除0
typedef struct functionCallInfo {
    size_t num;
    bool   isret;
    vector_template(int, reg);
    vector_template(instruction*, params);
    vector_template(instruction*, insts1);
    vector_template(instruction*, insts2);
} functionCallInfo;

void loadImm(Function* fnc, FILE* fp, Reg r, Value* v);
void loadFPImm(Function* fnc, FILE* fp, Reg r, Value* v);
void loadReg(Function* fnc, FILE* fp, Reg r, bool isMem, Value* v);

#endif