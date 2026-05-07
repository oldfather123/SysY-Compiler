#ifndef _TAC_H
#define _TAC_H

#include <stdbool.h>
#include <stdlib.h>

#include "function.h"
#include "utlist.h"
#include "value.h"

/* 2 source and 1 dest */
#define OP3_3AC_OPER \
    X(IR_OP_ADD)     \
    X(IR_OP_ADDF)    \
    X(IR_OP_SUB)     \
    X(IR_OP_SUBF)    \
    X(IR_OP_MUL)     \
    X(IR_OP_MULF)    \
    X(IR_OP_DIV) X(IR_OP_DIVF) X(IR_OP_MOD) X(IR_OP_LSHIFT) X(IR_OP_RSHIFT)
/* 2 source and 1 dest */
#define OP3_3AC_COND \
    X(IR_OP_GE)      \
    X(IR_OP_GEF)     \
    X(IR_OP_GT)      \
    X(IR_OP_GTF)     \
    X(IR_OP_EQ)      \
    X(IR_OP_EQF)     \
    X(IR_OP_NE) X(IR_OP_NEF) X(IR_OP_LT) X(IR_OP_LTF) X(IR_OP_LE) X(IR_OP_LEF)

// ret = args0
/* 1 source and 1 dest */
#define OP2_3AC    \
    X(IR_OP_SOTRE) \
    X(IR_OP_LOAD)  \
    X(IR_OP_I2F)   \
    X(IR_OP_F2I)   \
    X(IR_OP_ADDR)  \
    X(IR_OP_CALL)  \
    X(IR_OP_MOV)   \
    X(IR_OP_POS)   \
    X(IR_OP_POSF) X(IR_OP_NEG) X(IR_OP_NEGF) X(IR_OP_NOT) X(IR_OP_NOTF)

/* 1 source */
#define OP1_3AC \
    X(IR_OP_RETI) X(IR_OP_RETF) X(IR_OP_IF) X(IR_OP_PARAM) X(IR_OP_ARGUMENT)

/* 0 source */
#define OP0_3AC  \
    X(IR_OP_RET) \
    X(IR_OP_NOP) \
    X(IR_OP_PHI) \
    X(IR_OP_JMP) X(IR_OP_GETPTR) X(IR_OP_ADDPTR) X(IR_OP_GETITEMPTR)

// IR_OP_ADDPTR指令专为强度削减而诞生
#define X(op) op,
typedef enum TAC { OP3_3AC_OPER OP3_3AC_COND OP2_3AC OP1_3AC OP0_3AC } TAC;
#undef X
#define X(op) #op,
__attribute__((weak))
const char *op_3AC_name[] = {OP3_3AC_OPER OP3_3AC_COND OP2_3AC OP1_3AC OP0_3AC};

#undef X
#define X(op) case op:

/**
 * @brief 处理大部分指令,其余的指令需要提前处理
 */
#define CASEINST(args1body, args2body, retbody) \
    TAC opcode;                                 \
    switch (inst->op) {                         \
        OP3_3AC_OPER {                          \
            retbody;                            \
            args1body;                          \
            args2body;                          \
        }                                       \
        break;                                  \
        OP3_3AC_COND {                          \
            retbody;                            \
            args1body;                          \
            args2body;                          \
        }                                       \
        break;                                  \
        OP2_3AC {                               \
            retbody;                            \
            args1body;                          \
        }                                       \
        break;                                  \
        OP1_3AC { retbody; }                    \
        break;                                  \
        case IR_OP_RET:                         \
        case IR_OP_NOP:                         \
        case IR_OP_JMP:                         \
            break;                              \
        default:                                \
            assert(0);                          \
    }

/**
 * @brief 遍历基本块的指令
 */
#define LOOPBBALLINST(body)                      \
    {                                            \
        size_t              i;                   \
        struct instruction *inst = NULL;         \
        vector_each(&bb->inst, i, inst) { body } \
    }
/**
 * @brief 遍历函数的所有基本块
 */
#define LOOPALLBB(body)                        \
    {                                          \
        size_t      i;                         \
        BasicBlock *bb = NULL;                 \
        vector_each(&fnc->BBs, i, bb) { body } \
    }

/**
 * @brief 遍历函数的所有基本块的每个后继节点
 */
#define LOOPALLBBNEXT(body)                       \
    {                                             \
        BasicBlock *parent = bb;                  \
        if (parent->nextblock) {                  \
            BasicBlock *bb = parent->nextblock;   \
            body                                  \
        }                                         \
        if (parent->branchblock) {                \
            BasicBlock *bb = parent->branchblock; \
            body                                  \
        }                                         \
    }
/**
 * @brief 遍历函数的所有基本块的每个前继节点
 */
#define LOOPALLBBPREV(body)                                       \
    {                                                             \
        BasicBlock *_bb = bb;                                     \
        LOOPALLBB({                                               \
            if (bb->nextblock == _bb || bb->branchblock == _bb) { \
                body                                              \
            }                                                     \
        })                                                        \
        bb = _bb;                                                 \
    }
#define LOOPBBALLPHI(body)                \
    {                                     \
        size_t       i;                   \
        instruction *inst = NULL;         \
        vector_each(&bb->inst, i, inst) { \
            if (inst->op == IR_OP_PHI) {  \
                body                      \
            }                             \
        }                                 \
    }

typedef struct instruction {
    struct {
        union {
            struct instruction *join;
            struct {
                bool isSpill0;
                bool isSpill1;
                bool isSpill2;
                bool isMem0;
                bool isMem1;
                bool isMem2;
            };
            bool marked;
            bool redundance;
        };
        size_t n;
    };
    TAC                 op;
    Value               args[2];
    Value               ret;
    struct instruction *prev, *next;
} instruction;

Value        createAddF(Function *fnc, Value *arg1, Value *arg2);
Value        createAdd(Function *fnc, Value *arg1, Value *arg2);
Value        createSubF(Function *fnc, Value *arg1, Value *arg2);
Value        createSub(Function *fnc, Value *arg1, Value *arg2);
Value        createMulF(Function *fnc, Value *arg1, Value *arg2);
Value        createMul(Function *fnc, Value *arg1, Value *arg2);
Value        createDivF(Function *fnc, Value *arg1, Value *arg2);
Value        createDiv(Function *fnc, Value *arg1, Value *arg2);
Value        createMod(Function *fnc, Value *arg1, Value *arg2);
Value        createF2I(Function *fnc, Value *src);
void         createStore(Function *fnc, Value *dest, Value *src);
Value        createLoad(Function *fnc, Value *src);
instruction *createParam(Function *fnc, Value *v);
void         createArgument(Function *fnc, Value *v);
Value        createCall(Function *fnc, Value *dest);
Value        createI2F(Function *fnc, Value *src);
Value        createNeg(Function *fnc, Value *src);
Value        createNegF(Function *fnc, Value *src);
Value        createNot(Function *fnc, Value *src);
Value        createNotF(Function *fnc, Value *src);
Value        createPos(Function *fnc, Value *src);

Value        createPosF(Function *fnc, Value *src);

void         setCurrentBB(Function *fnc, BasicBlock *);
void         setCurrentBBNext(Function *fnc, BasicBlock *bb);
void         setCurrentBBBranch(Function *fnc, BasicBlock *bb);
BasicBlock  *createBasicBlock(Function *fnc);

Value        createCmpGt(Function *fnc, Value *arg1, Value *arg2);
Value        createCmpGe(Function *fnc, Value *arg1, Value *arg2);
Value        createCmpLe(Function *fnc, Value *arg1, Value *arg2);
Value        createCmpLt(Function *fnc, Value *arg1, Value *arg2);
Value        createCmpEq(Function *fnc, Value *arg1, Value *arg2);
Value        createCmpNe(Function *fnc, Value *arg1, Value *arg2);

Value        createCmpGtF(Function *fnc, Value *arg1, Value *arg2);
Value        createCmpGeF(Function *fnc, Value *arg1, Value *arg2);
Value        createCmpLeF(Function *fnc, Value *arg1, Value *arg2);
Value        createCmpLtF(Function *fnc, Value *arg1, Value *arg2);
Value        createCmpEqF(Function *fnc, Value *arg1, Value *arg2);

Value        createCmpNeF(Function *fnc, Value *arg1, Value *arg2);
Value        createGetPtr(Function *fnc, Value *arg1, Value *arg2);

void         createIF(Function *fnc, Value *arg);
void         createRet(Function *fnc);
void         createRetI(Function *fnc, Value *arg);
void         createRetF(Function *fnc, Value *arg);
instruction *createMOV(Function *fnc, Value *src, Value *dest);
instruction *createPHI(Function *fnc);
void         addPHIParam(instruction *inst, Value *arg);
instruction *createGetitemptr(Function *fnc, Value *array);
void         addArrayIndex(instruction *inst, Value *arg);
Value        symbolToValue(Symbol *s);
Value        createSSATempVariable(Function *fnc, Type ty);
void         setVariableDef(Function *fnc, uint32_t ind, instruction *inst);
/**
 * @brief bb1是否支配bb2
 * @param[in] *fnc
 * @param[in] *bb1
 * @param[in] *bb2
 */
bool         isDominetor(Function *fnc, BasicBlock *bb1, BasicBlock *bb2);
bool         foldValue(TAC op, Value *l, Value *r, Value *result);
#endif