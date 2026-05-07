#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "../include/bitset.h"
#include "../include/function.h"
#include "../include/ir.h"
#include "../include/lexer.h"
#include "../include/pass.h"
#include "../include/symbol.h"
#include "../include/sysy.h"
#include "../include/util.h"
#include "../include/value.h"
#include "../include/vector.h"
Symbol *getSymbol(Function *fnc, int32_t ind);
bool    ValueIsFloat(const Value *v) {
    if ((v->ty & TY_FLOAT && !(v->ty & IS_ARRAY)) && !(v->ty & IS_POINTER)) {
        return true;
    }
    return false;
}
bool ValueIsArray(const Value *v) {
    if (v->ty & IS_ARRAY) {
        return true;
    }
    return false;
}
bool ValueIsInt(const Value *v) {
    if (v->ty & TY_INT && !(v->ty & IS_ARRAY) && !(v->ty & IS_POINTER)) {
        return true;
    }
    return false;
}
bool ValueIsImm(const Value *v) {
    // if (v->ty & IS_IMM) {
    //     return true;
    // }
    // return false;
    if ((~BTYPE_MASK & v->ty) == IS_IMM) {
        return true;
    }
    return false;
}

int   getImmIntValue(const Value *v) { return v->imm.i; }
float getImmFloatValue(const Value *v) { return v->imm.f; }

Value symbolToValue(Symbol *s) {
    Value v = {.symbol = s->ind, .ty = (s->type) | IS_VAR};
    if (!s->isTemp) {
        v.ty |= s->scope == 0 ? IS_GLOBAL : 0;
    }
    if (s->arrayTy) {
        v.ty |= IS_ARRAY | IS_POINTER;
    }
    return v;
}

Value createTempVariable(Function *fnc, Type ty) {
    Symbol *sym = sy_malloc(sizeof(Symbol));
    sym->isTemp = 1;
    sym->type = ty;
    sym->tmpInd = fnc->tmpVariableNum++;
    sym->ind = vector_len(&fnc->symPool);
    vector_push_back(&fnc->symPool, sym);
    // TODO bug,看看llvm的实现
    return symbolToValue(sym);
}

// BUG,此符号没有SYMBOL
Value createSSATempVariable(Function *fnc, Type ty) {
    uint32_t   ind;
    ssaSymbol *sym = sy_malloc(sizeof(ssaSymbol));
    sym->ssaind = vector_len(&fnc->SSAValuePool);
    sym->valueind = fnc->tmpVariableNum++;
    sym->ty = ty;
    vector_push_back(&fnc->SSAValuePool, sym);
    return (Value){.ssaind = sym->ssaind, .ty = ty | IS_VAR | IS_SSA};
}

Value createAddF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    inst->ret = createTempVariable(fnc, arg1->ty);
    inst->op = IR_OP_ADDF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createAdd(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    inst->ret = createTempVariable(fnc, arg1->ty);
    inst->op = IR_OP_ADD;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createSubF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    inst->ret = createTempVariable(fnc, arg1->ty);
    inst->op = IR_OP_SUBF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createSub(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    inst->ret = createTempVariable(fnc, arg1->ty);
    inst->op = IR_OP_SUB;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createMulF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    inst->ret = createTempVariable(fnc, arg1->ty);
    inst->op = IR_OP_MULF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createMul(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    inst->ret = createTempVariable(fnc, arg1->ty);
    inst->op = IR_OP_MUL;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createDivF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    inst->ret = createTempVariable(fnc, arg1->ty);
    inst->op = IR_OP_DIVF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createDiv(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    inst->ret = createTempVariable(fnc, arg1->ty);
    inst->op = IR_OP_DIV;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createMod(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    inst->ret = createTempVariable(fnc, arg1->ty);
    inst->op = IR_OP_MOD;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}

// 1 dest and 1 src
Value createF2I(Function *fnc, Value *src) {
    assert(ValueIsFloat(src));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *src;
    inst->ret = createTempVariable(fnc, TY_INT);
    inst->op = IR_OP_F2I;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createI2F(Function *fnc, Value *src) {
    assert(ValueIsInt(src));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *src;
    inst->ret = createTempVariable(fnc, TY_FLOAT);
    inst->op = IR_OP_I2F;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createNeg(Function *fnc, Value *src) {
    assert(ValueIsInt(src));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *src;
    inst->ret = createTempVariable(fnc, TY_INT);
    inst->op = IR_OP_NEG;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createNegF(Function *fnc, Value *src) {
    assert(ValueIsFloat(src));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *src;
    inst->ret = createTempVariable(fnc, TY_FLOAT);
    inst->op = IR_OP_NEGF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createNot(Function *fnc, Value *src) {
    assert(ValueIsInt(src));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *src;
    inst->ret = createTempVariable(fnc, TY_INT);
    inst->op = IR_OP_NOT;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createNotF(Function *fnc, Value *src) {
    assert(ValueIsFloat(src));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *src;
    inst->ret = createTempVariable(fnc, TY_FLOAT);
    inst->op = IR_OP_NOTF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createPos(Function *fnc, Value *src) {
    assert(ValueIsInt(src));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *src;
    inst->ret = createTempVariable(fnc, TY_INT);
    inst->op = IR_OP_POS;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createPosF(Function *fnc, Value *src) {
    assert(ValueIsFloat(src));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *src;
    inst->ret = createTempVariable(fnc, TY_FLOAT);
    inst->op = IR_OP_POSF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
// TODO 没有做判断类型和类型转换
void createStore(Function *fnc, Value *dest, Value *src) {
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *src;
    inst->ret = *dest;
    inst->op = IR_OP_SOTRE;
    // TODO
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
}
Value createLoad(Function *fnc, Value *src) {
    instruction *inst = sy_malloc(sizeof(instruction));
    if (!(src->ty & IS_POINTER)) {
        assert(0);
    }
    inst->args[0] = *src;
    inst->ret = createTempVariable(fnc, src->ty & BTYPE_MASK);
    Symbol *s = getSymbol(fnc, inst->ret.symbol);
    s->def = inst;
    inst->ret.ty |= IS_VAR;
    inst->op = IR_OP_LOAD;
    // TODO
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
instruction *createParam(Function *fnc, Value *v) {
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->ret = *v;
    inst->op = IR_OP_PARAM;
    return inst;
}
void createArgument(Function *fnc, Value *v) {
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->ret = *v;
    inst->op = IR_OP_ARGUMENT;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
}
Value createCall(Function *fnc, Value *dest) {
    instruction *inst = sy_malloc(sizeof(instruction));
    Symbol      *s = getGlobalSymbol(dest->symbol);
    assert(s);
    inst->ret = createTempVariable(fnc, s->fnc->fncTy.ret);
    inst->args[0] = *dest;
    inst->op = IR_OP_CALL;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}

BasicBlock *createBasicBlock(Function *fnc) {
    BasicBlock *bb = sy_malloc(sizeof(BasicBlock));
    bb->bbName = vector_len(&fnc->BBs);

    vector_push_back(&fnc->BBs, bb);
    return bb;
}

/**
 * @brief 设置IR插入的基本块
 * @param[in] *fnc
 * @param[in] *bb
 */
void setCurrentBB(Function *fnc, BasicBlock *bb) {
    assert(bb);
    fnc->currentBB = bb;
}
void setCurrentBBNext(Function *fnc, BasicBlock *bb) {
    assert(bb);
    fnc->currentBB->nextblock = bb;
}
void setCurrentBBBranch(Function *fnc, BasicBlock *bb) {
    assert(bb);
    fnc->currentBB->branchblock = bb;
}

Value createCmpGt(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_GT;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createCmpGe(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_GE;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createCmpLe(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_LE;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createCmpLt(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_LT;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createCmpEq(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_EQ;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createCmpNe(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_NE;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}

Value createCmpGtF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_GTF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createCmpGeF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_GEF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createCmpLeF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_LEF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}

Value createCmpLtF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_LTF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createCmpEqF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_EQF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Value createCmpNeF(Function *fnc, Value *arg1, Value *arg2) {
    assert(ValueIsFloat(arg1) && ValueIsFloat(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    Type t = TY_INT;
    inst->ret = createTempVariable(fnc, t);
    inst->op = IR_OP_NEF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}
Symbol *getSymbol(Function *fnc, int32_t ind) {
    return vector_get(&fnc->symPool, ind);
}

Symbol *getArrayNextVal(Function *fnc, Value *arg1) {}
Value   createGetPtr(Function *fnc, Value *arg1, Value *arg2) {
    Symbol *s, *s1;
    assert(ValueIsArray(arg1));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;
    if (arg1->ty & IS_GLOBAL) {
        s = getGlobalSymbol(arg1->symbol);
    } else {
        s = getSymbol(fnc, arg1->symbol);
    }
    Type t = s->type & (~(IS_ARRAY | IS_POINTER | IS_GLOBAL));
    if (s->arrayTy) {
        t |= IS_POINTER;
    }
    inst->ret = createTempVariable(fnc, t);
    s1 = getSymbol(fnc, inst->ret.symbol);
    s1->def = inst;
    if (s->arrayTy && s->arrayTy->next) {
        s1->arrayTy = s->arrayTy->next;
    }
    inst->op = IR_OP_GETPTR;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst->ret;
}

Function *createFunction(SYContext *context, Symbol *sym) {
    assert(sym);
    Function *fnc = sy_malloc(sizeof(Function));
    fnc->name = getToken(sym->tok)->str;
    fnc->symbol = sym;
    sym->fnc = fnc;
    return fnc;
}

/**
 * @brief 接收一个Bool值,但在sy中定义为int
 * @param[in] *fnc
 * @param[in] *arg
 */
void createIF(Function *fnc, Value *arg) {
    assert(ValueIsInt(arg));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->ret = *arg;
    inst->op = IR_OP_IF;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
}

/**
 * @brief 没有值的return
 * @param[in] *fnc
 */
void createRet(Function *fnc) {
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->op = IR_OP_RET;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
}
void createRetI(Function *fnc, Value *arg) {
    assert(ValueIsInt(arg));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->op = IR_OP_RETI;
    inst->ret = *arg;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
}
void createRetF(Function *fnc, Value *arg) {
    assert(ValueIsFloat(arg));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->op = IR_OP_RETF;
    inst->ret = *arg;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
}

instruction *createMOV(Function *fnc, Value *dest, Value *src) {
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->op = IR_OP_MOV;
    inst->ret = *dest;
    inst->args[0] = *src;
    if (!fnc->insert) {
        vector_push_back(&fnc->currentBB->inst, inst);
    } else {
        DL_APPEND(fnc->currentBB->instlist, inst);
    }
    return inst;
}
/**
 * @brief 注意phi语句一定要在块首
 * @param[in] *fnc
 */
instruction *createPHI(Function *fnc) {
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->op = IR_OP_PHI;
    DL_PREPEND(fnc->currentBB->instlist, inst);
    // if (vector_len(&fnc->currentBB->inst) == 0) {
    //     if (!fnc->insert) {
    //         vector_push_back(&fnc->currentBB->inst, inst);
    //     } else {
    //         DL_APPEND(fnc->currentBB->instlist, inst);
    //     }
    // } else {
    //     vector_insert(&fnc->currentBB->inst, 0, inst);
    // }
    inst->args[0].phi = sy_malloc(sizeof(phiFunction));
    return inst;
}
void addPHIParam(instruction *inst, Value *arg) {
    assert(inst->op == IR_OP_PHI);
    // vector_push_back(&inst->args[0].phi->param, *arg);
}
/**
 * @brief 数组寻址,比较高级的IR,ret = args0[args1...]
 * @param[in] *fnc
 */
instruction *createGetitemptr(Function *fnc, Value *array) {
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->op = IR_OP_GETITEMPTR;
    inst->args[0] = *array;
    inst->args[1].arrayAddr = sy_malloc(sizeof(arrayAddr));
    Type t = array->ty & BTYPE_MASK;
    assert(t != 0);
    inst->ret = createTempVariable(fnc, t | IS_POINTER);
    return inst;
}
void addArrayIndex(instruction *inst, Value *arg) {
    assert(inst->op == IR_OP_GETITEMPTR);
    Value v = *arg;
    vector_push_back(&inst->args[1].arrayAddr->index, v);
}

void printSSAValue(FILE *fp, Function *fnc, ssaSymbol *sym) {
    if (!sym->sym) {
        fprintf(fp, "_%d", sym->valueind);
    } else if (sym->sym->isTemp) {
        fprintf(fp, "_%d", sym->sym->tmpInd);
    } else {
        fprintf(fp, "%s_%d", getToken(sym->sym->tok)->str, sym->valueind);
    }
}
// Value createVariableValue(Function *fnc, Symbol *s) {
//     int32_t ind = vector_len(&fnc->symPool);

// }
/**
 * @brief 打印值
 * @param[in] *fp
 * @param[in] *fnc
 * @param[in] *v
 */
void printValue(FILE *fp, Function *fnc, Value *v) {
    if (ValueIsImm(v)) {
        if (v->ty & TY_FLOAT)
            fprintf(fp, "%g", getImmFloatValue(v));
        else if (v->ty & TY_INT)
            fprintf(fp, "%I32d", getImmIntValue(v));
        else
            assert(0);
    } else if (v->ty & IS_VAR || v->ty & IS_POINTER) {
        if (v->ty & IS_SSA) {
            ssaSymbol *sym = getSSAlVariable(fnc, v->ssaind);
            if (!sym->sym) {
                fprintf(fp, "_%d", sym->valueind);
            } else if (sym->sym->isTemp) {
                fprintf(fp, "_%d", sym->sym->tmpInd);
            } else {
                fprintf(fp, "%s_%d", getToken(sym->sym->tok)->str,
                        sym->valueind);
            }
        } else {
            NULL;
            Symbol *sym = NULL;
            if (v->ty & IS_GLOBAL) {
                sym = getGlobalSymbol(v->symbol);
            } else {
                sym = getSymbol(fnc, v->symbol);
            }
            if (sym->isTemp) {
                fprintf(fp, "_%d", sym->tmpInd);
            } else {
                fprintf(fp, "%s", getToken(sym->tok)->str);
            }
        }
    } else if (v->ty & TY_VOID && v->ty & IS_POINTER) {
        fprintf(fp, "\"%s\"", v->imm.str);
    } else if (v->ty & IS_GLOBAL) {
        Symbol *sym = getGlobalSymbol(v->symbol);
        fprintf(fp, "%s", getToken(sym->tok)->str);
    } else if (v->ty & TY_VOID) {
        fprintf(fp, "TY_VOID");
    } else
        assert(0);
}
void printIns(FILE *fp, Function *fnc, instruction *inst) {
    if (fnc->instNumbering) {
        fprintf(fp, "%-5d%-15s", inst->n, op_3AC_name[inst->op]);
    } else
        fprintf(fp, "%-20s", op_3AC_name[inst->op]);
    if (inst->op == IR_OP_PHI) {
        phiFunction *phi = inst->args[0].phi;
        phiParam     p;
        size_t       i;
        printValue(fp, fnc, &inst->ret);
        fputs("=\t", fp);
        vector_each(&phi->param, i, p) {
            printValue(fp, fnc, &p.v);
            fputs("\t", fp);
        }
        fputs("\n", fp);
        return;
    } else if (inst->op == IR_OP_GETPTR) {
        printValue(fp, fnc, &inst->ret);
        fputs("\t", fp);
        printValue(fp, fnc, &inst->args[0]);
        fputs("\t", fp);
        printValue(fp, fnc, &inst->args[1]);
        fputs("\t", fp);
        fputs("\n", fp);
        return;
    } else if (inst->op == IR_OP_ADDPTR) {
        printValue(fp, fnc, &inst->ret);
        fputs("\t", fp);
        printValue(fp, fnc, &inst->args[0]);
        fputs("\t", fp);
        printValue(fp, fnc, &inst->args[1]);
        fputs("\t", fp);
        fputs("\n", fp);
        return;
    } else if (inst->op == IR_OP_GETITEMPTR) {
        printValue(fp, fnc, &inst->ret);
        fputs(" = ", fp);

        arrayAddr *ptr = inst->args[1].arrayAddr;
        Value     *index;
        size_t     i;
        printValue(fp, fnc, &inst->args[0]);
        vector_each_address(&ptr->index, i, index) {
            fputs("[", fp);
            printValue(fp, fnc, index);
            fputs("]", fp);
        }
        fputs("\n", fp);
        return;
    }

    CASEINST(
        {
            printValue(fp, fnc, &inst->args[0]);
            fputs("\t", fp);
        },
        {
            printValue(fp, fnc, &inst->args[1]);
            fputs("\t", fp);
        },
        {
            printValue(fp, fnc, &inst->ret);
            fputs("\t", fp);
        });
}
void printBasicBlock(FILE *fp, Function *fnc, BasicBlock *bb) {}
void printBasicAllBlock(FILE *fp, Function *fnc) {
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        fprintf(fp, "<bb %d>\n", bb->bbName);
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (fnc->instNumbering) {
                fprintf(fp, "%-5d%-15s", inst->n, op_3AC_name[inst->op]);
            } else
                fprintf(fp, "%-20s", op_3AC_name[inst->op]);
            if (inst->op == IR_OP_PHI) {
                phiFunction *phi = inst->args[0].phi;
                phiParam     p;
                size_t       i;
                printValue(fp, fnc, &inst->ret);
                fputs("=\t", fp);
                vector_each(&phi->param, i, p) {
                    printValue(fp, fnc, &p.v);
                    fputs("\t", fp);
                }
                fputs("\n", fp);
                continue;
            } else if (inst->op == IR_OP_GETPTR) {
                printValue(fp, fnc, &inst->ret);
                fputs("\t", fp);
                printValue(fp, fnc, &inst->args[0]);
                fputs("\t", fp);
                printValue(fp, fnc, &inst->args[1]);
                fputs("\t", fp);
                fputs("\n", fp);
                continue;
            } else if (inst->op == IR_OP_ADDPTR) {
                printValue(fp, fnc, &inst->ret);
                fputs("\t", fp);
                printValue(fp, fnc, &inst->args[0]);
                fputs("\t", fp);
                printValue(fp, fnc, &inst->args[1]);
                fputs("\t", fp);
                fputs("\n", fp);
                continue;
            } else if (inst->op == IR_OP_GETITEMPTR) {
                printValue(fp, fnc, &inst->ret);
                fputs(" = ", fp);

                arrayAddr *ptr = inst->args[1].arrayAddr;
                Value     *index;
                size_t     i;
                printValue(fp, fnc, &inst->args[0]);
                vector_each_address(&ptr->index, i, index) {
                    fputs("[", fp);
                    printValue(fp, fnc, index);
                    fputs("]", fp);
                }
                fputs("\n", fp);
                continue;
            }

            CASEINST(
                {
                    printValue(fp, fnc, &inst->args[0]);
                    fputs("\t", fp);
                },
                {
                    printValue(fp, fnc, &inst->args[1]);
                    fputs("\t", fp);
                },
                {
                    printValue(fp, fnc, &inst->ret);
                    fputs("\t", fp);
                });
            // printValue(fp, fnc, );
            fputs("\n", fp);
        }
        if (bb->nextblock) fprintf(fp, "next:<bb %d>\t", bb->nextblock->bbName);
        if (bb->branchblock)
            fprintf(fp, "branchbb:<bb %d>\t", bb->branchblock->bbName);
        fprintf(fp, "\n");
        fprintf(fp, "\n");
    }
}

/**
 * @brief 打印出此函数的Markdown格式的控制流图
 * @param[in] *fnc1
 */
void printCFG(Function *fnc) {
    FILE *fp = fopen("cfg.md", "w");
    if (!fp) sy_error("failed to create cfg.md");
    fprintf(fp, "```mermaid\n");
    fprintf(fp, "graph TD\n");
    char *ch;

    LOOPALLBB({
        // 这里加上一个空格,[]不能为空
        fprintf(fp, "bb%d[\"bb_%d<br> ", bb->bbName, bb->bbName);

        LOOPBBALLINST({
            if (fnc->instNumbering) fprintf(fp, "%d:", inst->n);
            switch (inst->op) {
                case IR_OP_PARAM: {
                    size_t _i = i;
                    while (vector_get(&bb->inst, i)->op != IR_OP_CALL) {
                        i++;
                    };
                    printValue(fp, fnc, &vector_get(&bb->inst, i)->ret);
                    fputs("= ", fp);
                    printValue(fp, fnc, &vector_get(&bb->inst, i)->args[0]);
                    fputs("(", fp);
                    for (; _i < i; _i++) {
                        printValue(fp, fnc, &vector_get(&bb->inst, _i)->ret);
                        fputs(",", fp);
                    }
                    fputs(")", fp);
                    break;
                }
                case IR_OP_CALL:
                    printValue(fp, fnc, &vector_get(&bb->inst, i)->ret);
                    fputs("= ", fp);
                    printValue(fp, fnc, &vector_get(&bb->inst, i)->args[0]);
                    break;
                case IR_OP_GETPTR: {
                    printValue(fp, fnc, &inst->args[0]);
                    while (vector_get(&bb->inst, i)->op == IR_OP_GETPTR) {
                        fputs("[", fp);
                        printValue(fp, fnc, &vector_get(&bb->inst, i)->args[1]);
                        fputs("]", fp);
                        i++;
                    }
                    if (vector_get(&bb->inst, i)->op == IR_OP_SOTRE) {
                        fputs("= ", fp);
                        printValue(fp, fnc, &vector_get(&bb->inst, i)->args[0]);
                        i++;
                    }
                    i--;
                    break;
                }
                case IR_OP_PHI: {
                    phiFunction *phi = inst->args[0].phi;
                    phiParam     p;
                    size_t       i;
                    printValue(fp, fnc, &inst->ret);
                    fputs("= ", fp);
                    fputs("phi ", fp);
                    vector_each(&phi->param, i, p) {
                        printValue(fp, fnc, &p.v);
                        fputs(", ", fp);
                    }
                    break;
                }
                case IR_OP_SOTRE:
                    printValue(fp, fnc, &inst->ret);
                    fprintf(fp, "= ");
                    printValue(fp, fnc, &inst->args[0]);
                    break;
                case IR_OP_MOV:
                case IR_OP_LOAD:
                    printValue(fp, fnc, &inst->ret);
                    fprintf(fp, "= ");
                    printValue(fp, fnc, &inst->args[0]);
                    break;
                case IR_OP_IF:
                    fprintf(fp, "if ");
                    printValue(fp, fnc, &inst->ret);
                    break;
                case IR_OP_ADD:
                case IR_OP_ADDF:
                    ch = "+";
                    goto _end;
                case IR_OP_SUB:
                case IR_OP_SUBF:
                    ch = "-";
                    goto _end;
                case IR_OP_MUL:
                case IR_OP_MULF:
                    ch = "*";
                    goto _end;
                case IR_OP_DIV:
                case IR_OP_DIVF:
                    ch = "/";
                    goto _end;
                case IR_OP_GE:
                case IR_OP_GEF:
                    ch = ">=";
                    goto _end;
                case IR_OP_GT:
                case IR_OP_GTF:
                    ch = ">";
                    goto _end;
                case IR_OP_LE:
                case IR_OP_LEF:
                    ch = "<=";
                    goto _end;
                case IR_OP_LT:
                case IR_OP_LTF:
                    ch = "<";
                    goto _end;
                case IR_OP_NE:
                case IR_OP_NEF:
                    ch = "!=";
                    goto _end;
                case IR_OP_EQ:
                case IR_OP_EQF:
                    ch = "==";
                _end:
                    printValue(fp, fnc, &inst->ret);
                    fprintf(fp, "= ");
                    printValue(fp, fnc, &inst->args[0]);
                    fprintf(fp, "%s ", ch);
                    printValue(fp, fnc, &inst->args[1]);
                    break;

                case IR_OP_RETI:
                case IR_OP_RETF:
                    fprintf(fp, "return ");
                    printValue(fp, fnc, &inst->ret);
                    break;
                case IR_OP_NOT:
                case IR_OP_NOTF:
                    ch = "!";
                    goto skip2;
                case IR_OP_NEG:
                case IR_OP_NEGF:
                    ch = "-";
                skip2:
                    printValue(fp, fnc, &inst->ret);
                    fprintf(fp, "= %s", ch);
                    printValue(fp, fnc, &inst->args[0]);
                    break;
                case IR_OP_RET:
                    fprintf(fp, "return ");
                    break;
                default:
                    assert(0);
            }
            fprintf(fp, "<br>");
        })

        fprintf(fp, "\"]\n");
    })

    LOOPALLBB({
        LOOPALLBBNEXT(
            { fprintf(fp, "\nbb%d --> bb%d\n", parent->bbName, bb->bbName); });
    })
    fprintf(fp, "\n```");
    fclose(fp);
}