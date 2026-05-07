#ifndef SYSY_ASM_H
#define SYSY_ASM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../../include/utlist.h"

#define _SY_ASM \
    _X(fcvtds)  \
    _X(fmv)     \
    _X(fmvs)    \
    _X(fcvtws)  \
    _X(fcvtsw)  \
    _X(ret)     \
    _X(label)   \
    _X(call)    \
    _X(J)       \
    _X(li)      \
    _X(la)      \
    _X(seqz)    \
    _X(negw)    \
    _X(fneg)    \
    _X(mv)      \
    _X(sextw)   \
    _X(add)     \
    _X(addi)    \
    _X(sub)     \
    _X(subi)    \
    _X(mul)     \
    _X(muli)    \
    _X(div)     \
    _X(divi)    \
    _X(rem)     \
    _X(bge)     \
    _X(bgt)     \
    _X(blt)     \
    _X(ble)     \
    _X(beq)     \
    _X(bne)     \
    _X(fsw)     \
    _X(flw)     \
    _X(sw)      \
    _X(lw)      \
    _X(sd)      \
    _X(ld)      \
    _X(fge)     \
    _X(fgt)     \
    _X(flt)     \
    _X(fle)     \
    _X(feq)     \
    _X(fne)     \
    _X(fadd)    \
    _X(fsub)    \
    _X(fmul)    \
    _X(fdiv)    \
    _X(debug)

#define _X(op) ASM_##op,

typedef enum riscvAsm { _SY_ASM } riscvAsm;
extern char *_asmstr[];
typedef struct asminstruction {
    riscvAsm op;
    int8_t   dest;
    struct {
        struct {
            int8_t reg;
            int    index;
        };
        int   imm;
        char *symbol;
    } src[2];
    void                  *i;
    struct asminstruction *prev, *next;
    uint8_t                isPO : 1;
} asminstruction;

typedef enum AsmState {
    ASM_1,   // op symbol:j symbol,call symbol
    ASM_2,   // op reg,reg,reg
    ASM_3,   // op reg,reg,imm
    ASM_4,   // op reg,imm
    ASM_5,   // op reg,reg
    ASM_6,   // op reg,reg,symbol :bge ble
    ASM_7,   // op reg,imm(reg) :ld lw sw sd
    ASM_8,   // op reg,symbol :la symbol
    ASM_9,   // label
    ASM_10,  // op   : ret
    ASM_11,  // fcvt.w.s
    ASM_12,  // feq
    ASM_13,  // debug
} AsmState;
asminstruction *createAsm12(riscvAsm asmop, int r0, int r1, int r2);
asminstruction *createAsm13(void *i);
asminstruction *createAsm11(riscvAsm asmop, int r0, int r1);
asminstruction *createAsm10(riscvAsm asmop);
asminstruction *createAsm9(riscvAsm asmop, char *symbol);
asminstruction *createAsm8(riscvAsm asmop, int r0, char *symbol);
asminstruction *createAsm7(riscvAsm asmop, int r0, int r1, int index);
asminstruction *createAsm6(riscvAsm asmop, int r0, int r1, char *symbol);
asminstruction *createAsm5(riscvAsm asmop, int r0, int r1);
asminstruction *createAsm4(riscvAsm asmop, int r0, int imm);
asminstruction *createAsm2(riscvAsm asmop, int r0, int r1, int r2);
asminstruction *createAsm3(riscvAsm asmop, int r0, int r1, int imm);
asminstruction *outputAsm2(FILE *fp, asminstruction *ins);
asminstruction *createAsm1(riscvAsm asmop, char *symbol);
asminstruction *outputAsm1(FILE *fp, asminstruction *ins);

#endif