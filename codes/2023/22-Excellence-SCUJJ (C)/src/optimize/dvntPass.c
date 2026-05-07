// /*
// 该文件实现了基于支配者的值编号,shay
//  */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/bitset.h"
#include "../include/function.h"
#include "../include/ir.h"
#include "../include/lexer.h"
#include "../include/pass.h"
#include "../include/symbol.h"
#include "../include/sysy.h"
#include "../include/uthash.h"
#include "../include/util.h"
#include "../include/value.h"
#include "../include/vector.h"

typedef enum ValueNumberType {
    VN_INT = 0x1,
    VN_FLOAT,
    VN_VAR,
} ValueNumberType;

typedef struct valueNumber {
    ValueNumberType ty;
    union {
        int      i;
        float    f;
        uint32_t var;
    };
} valueNumber;

typedef struct valueNumberTable {
    valueNumber    key;
    size_t         ind;
    UT_hash_handle hh;
} valueNumberTable;

typedef struct valueNumberExpr {
    TAC      op;
    uint32_t VNarg1, VNarg2;
} valueNumberExpr;

typedef struct valueNumberExprTable {
    valueNumberExpr expr;
    uint32_t        ind;
    UT_hash_handle  hh;
} valueNumberExprTable;

// struct iHashTable {
//     int            i;
//     valueNumber   *vn;
//     UT_hash_handle hh;
// };
// struct fHashTable {
//     float          f;
//     valueNumber   *vn;
//     UT_hash_handle hh;
// };

// struct vHashTable {
//     uint32_t       v;
//     valueNumber   *vn;
//     UT_hash_handle hh;
// };

// typedef struct VNHashTable {
//     valueNumber        v;
//     struct iHashTable *ih;
//     struct fHashTable *fh;
//     struct vHashTable *vh;
// } VNHashTable;

typedef struct hashTable {
    valueNumberTable     *hashTable;
    valueNumberExprTable *exprTable;
    BasicBlock           *bb;
} hashTable;

typedef struct valueNumberState {
    hashTable *curHH;
    vector_template(hashTable *, hhlist);
    vector_template(valueNumberTable *, VNPools);
} valueNumberState;

// typedef struct exprNumber {
//     /* data */
//     UT_hash_handle hh;
// } exprNumber;

// valueNumber *createValueNumber(valueNumberState *state) {
//     valueNumber *ptr = sy_malloc(sizeof(valueNumber));
//     ptr->ind = vector_len(&state->arrVN);
//     return ptr;
// }

static void createNewScopeHH(valueNumberState *state, BasicBlock *bb) {
    hashTable *ptr = (hashTable *)sy_malloc(sizeof(hashTable));
    ptr->bb = bb;
    state->curHH = ptr;
    vector_push_back(&state->hhlist, ptr);
}
static void deleteScopeHH(valueNumberState *state, BasicBlock *bb) {
    hashTable *ptr;
    vector_pop_back(&state->hhlist, ptr);
    assert(ptr->bb == bb);

    valueNumberTable     *item1, *tmp1;
    valueNumberExprTable *item2, *tmp2;

    HASH_ITER(hh, ptr->hashTable, item1, tmp1) {
        HASH_DEL(ptr->hashTable, item1);
    }
    HASH_ITER(hh, ptr->exprTable, item2, tmp2) {
        HASH_DEL(ptr->exprTable, item2);
        free(item2);
    }
    free(ptr);
}
/**
 * @brief 为常量或者ssa值寻找或者分配值编号
 * @param[in] *state
 * @param[in] *v
 */
uint32_t createValueNumber(valueNumberState *state, Value *v) {
    assert(v);
    valueNumberTable  vn;
    valueNumberTable *ptr = NULL;
    if (ValueIsImm(v)) {
        if (ValueIsFloat(v)) {
            vn.key.ty = VN_FLOAT;
            vn.key.f = getImmFloatValue(v);
        } else if (ValueIsInt(v)) {
            vn.key.ty = VN_INT;
            vn.key.i = getImmIntValue(v);
        } else {
            assert(0);
        }
    } else {
        if (v->ty & IS_SSA) {
            vn.key.ty = VN_VAR;
            vn.key.var = v->ssaind;
        } else {
            assert(0);
        }
    }
    size_t     i;
    hashTable *tmpHH;
    vector_each_reverse(&state->hhlist, i, tmpHH) {
        HASH_FIND(hh, tmpHH->hashTable, &vn.key, sizeof(valueNumber), ptr);
        if (ptr) {
            return ptr->ind;
        }
    }

    ptr = sy_malloc(sizeof(valueNumberTable));
    ptr->key = vn.key;
    ptr->ind = vector_len(&state->VNPools);
    HASH_ADD(hh, state->curHH->hashTable, key, sizeof(valueNumber), ptr);
    vector_push_back(&state->VNPools, ptr);
    return ptr->ind;
}

/**
 * @brief 设置值编号
 * @param[in] *fnc
 * @param[in] *state
 * @param[in] *v
 */
uint32_t setValueNumber(Function *fnc, valueNumberState *state, Value *v,
                        uint32_t result) {
    uint32_t ind = createValueNumber(state, v);
    vector_get(&state->VNPools, ind)->ind = result;
    return ind;
}

/**
 * @brief 设置值编号
 * @param[in] *fnc
 * @param[in] *state
 * @param[in] *v
 */
bool findValueNumber(Function *fnc, valueNumberState *state, Value *v,
                     uint32_t *result) {
    assert(v);
    valueNumberTable  vn;
    valueNumberTable *ptr = NULL;
    if (ValueIsImm(v)) {
        if (ValueIsFloat(v)) {
            vn.key.ty = VN_FLOAT;
            vn.key.f = getImmFloatValue(v);
        } else if (ValueIsInt(v)) {
            vn.key.ty = VN_INT;
            vn.key.i = getImmIntValue(v);
        } else {
            assert(0);
        }
    } else {
        if (v->ty & IS_SSA) {
            vn.key.ty = VN_VAR;
            vn.key.var = v->ssaind;
        } else {
            assert(0);
        }
    }
    size_t     i;
    hashTable *tmpHH;
    vector_each_reverse(&state->hhlist, i, tmpHH) {
        HASH_FIND(hh, tmpHH->hashTable, &vn.key, sizeof(valueNumber), ptr);
        if (ptr) {
            *result = ptr->ind;
            return true;
        }
        return false;
    }
}

void        insertExpr(Function *fnc, valueNumberState *state) {}

static void valueNumberToValue(Function *fnc, valueNumberState *state,
                               uint32_t vn, Type ty, Value *v) {
    valueNumber l = vector_get(&state->VNPools, vn)->key;

    if (l.ty == VN_FLOAT) {
        *v = SET_IMM_FLOAT(l.f);
    } else if (l.ty == VN_INT) {
        *v = SET_IMM_INT(l.i);
    } else if (l.ty == VN_VAR) {
        // 这里可能有BUG,IR中也许有额外
        v->ty = ty;
        v->ssaind = l.var;
    }
}

/**
 * @brief   1,fold value,saved in expr->VNarg1;
 *          2,simplified expr;
 * @param[in] *fnc
 * @param[in] *state
 * @param[in] *expr
 * @param[in] ty1
 * @param[in] ty2
 */
static int simplifiedExpr(Function *fnc, valueNumberState *state,
                          valueNumberExpr *expr, Type ty1, Type ty2) {
    Value var1, var2, result;
    valueNumberToValue(fnc, state, expr->VNarg1, ty1, &var1);
    valueNumberToValue(fnc, state, expr->VNarg2, ty2, &var2);
    if (foldValue(expr->op, &var1, &var2, &result)) {
        expr->VNarg1 = createValueNumber(state, &result);
        return 1;
    }
    // TODO simplified expr
    if ((var1.ty & IS_IMM) &&
        ((var2.ty & IS_SSA) && (!(var2.ty & IS_POINTER))) &&
        expr->op == IR_OP_ADD) {
        ssaSymbol *ssa = getSSAlVariable(fnc, var2.ssaind);
        Value      pvar1, pvar2, presult;
        if (ssa->def->op == IR_OP_ADD) {
            valueNumberToValue(fnc, state, ssa->def->args[0].VNind,
                               ssa->def->args[0].ty, &pvar1);
            valueNumberToValue(fnc, state, ssa->def->args[1].VNind,
                               ssa->def->args[1].ty, &pvar2);
            if (pvar1.ty & IS_IMM) {
                if (foldValue(expr->op, &var1, &pvar1, &presult)) {
                    expr->VNarg1 = createValueNumber(state, &presult);
                    expr->VNarg2 = ssa->def->args[1].VNind;
                    return 2;
                }
            } else if (pvar2.ty & IS_IMM) {
                if (foldValue(expr->op, &var1, &pvar2, &presult)) {
                    expr->VNarg1 = createValueNumber(state, &presult);
                    expr->VNarg2 = ssa->def->args[0].VNind;
                    return 2;
                }
            }
        }

    } else if ((var2.ty & IS_IMM) &&
               ((var1.ty & IS_SSA) && (!(var1.ty & IS_POINTER))) &&
               expr->op == IR_OP_ADD) {
        ssaSymbol *ssa = getSSAlVariable(fnc, var1.ssaind);
        Value      pvar1, pvar2, presult;
        if (ssa->def->op == IR_OP_ADD) {
            valueNumberToValue(fnc, state, ssa->def->args[0].VNind,
                               ssa->def->args[0].ty, &pvar1);
            valueNumberToValue(fnc, state, ssa->def->args[1].VNind,
                               ssa->def->args[1].ty, &pvar2);
            if (pvar1.ty & IS_IMM) {
                if (foldValue(expr->op, &var2, &pvar1, &presult)) {
                    expr->VNarg2 = createValueNumber(state, &presult);
                    expr->VNarg1 = ssa->def->args[1].VNind;
                    return 2;
                }
            } else if (pvar2.ty & IS_IMM) {
                if (foldValue(expr->op, &var2, &pvar2, &presult)) {
                    expr->VNarg2 = createValueNumber(state, &presult);
                    expr->VNarg1 = ssa->def->args[0].VNind;
                    return 2;
                }
            }
        }
    }

    return 3;
}

static valueNumberExprTable *FindExpr(Function *fnc, valueNumberState *state,
                                      valueNumberExpr *expr) {
    valueNumberExprTable temp, *ptr;
    size_t               i;
    hashTable           *tmpHH;

    temp.expr = *expr;
    vector_each_reverse(&state->hhlist, i, tmpHH) {
        HASH_FIND(hh, tmpHH->exprTable, &temp.expr, sizeof(valueNumberExpr),
                  ptr);
        if (ptr) {
            return ptr;
        }
    }
    return NULL;
}

static valueNumberExprTable *createExpr(Function *fnc, valueNumberState *state,
                                        valueNumberExpr *expr,
                                        uint32_t         result) {
    valueNumberExprTable *ptr;
    ptr = sy_malloc(sizeof(valueNumberExprTable));
    ptr->expr = *expr;
    ptr->ind = result;
    HASH_ADD(hh, state->curHH->exprTable, expr, sizeof(valueNumberExpr), ptr);
    return ptr;
}

bool isSwap(TAC op) {
    switch (op) {
        case IR_OP_ADD:
        case IR_OP_MUL:
        case IR_OP_ADDF:
        case IR_OP_MULF:
        case IR_OP_EQ:
        case IR_OP_EQF:
        case IR_OP_NE:
        case IR_OP_NEF:
            return true;
    }
    return false;
}
/**
 * @brief phi函数是否是冗余的
 * @param[in] *fnc
 * @param[in] *inst
 */
static bool phiIsRedundance(Function *fnc, instruction *inst) {
    phiFunction *phi = inst->args[0].phi;
    phiParam    *p;
    size_t       i;
    bool         y = true, first = true;
    uint32_t     cmp;
    vector_each_address(&phi->param, i, p) {
        if (p->isVN) {
            if (first) {
                first = false;
                cmp = p->v.VNind;
            } else {
                if (cmp != p->v.VNind) {
                    return false;
                }
            }
        } else {
            return false;
        }
    }
    return true;
}

bool isPHIparam(Function *fnc, Value *v) {
    assert(v->ty & IS_SSA);
    LOOPALLBB({LOOPBBALLINST({
        if (inst->op == IR_OP_PHI) {
            phiFunction *phi = inst->args[0].phi;
            phiParam    *p;
            size_t       i;
            vector_each_address(&phi->param, i, p) {
                if (p->v.ssaind == v->ssaind) return true;
            }
        }
    })})
    return false;
}

bool isPHIparam1(Function *fnc, Value *v, Value *r) {
    assert(v->ty & IS_SSA);
    LOOPALLBB({LOOPBBALLINST({
        if (inst->op == IR_OP_PHI) {
            phiFunction *phi = inst->args[0].phi;
            phiParam    *p;
            size_t       i;
            vector_each_address(&phi->param, i, p) {
                if (p->v.ssaind == v->ssaind) {
                    *r = inst->ret;
                    return true;
                }
            }
        }
    })})
    return false;
}

valueNumberState *state;
vector_template(Value, constantParams);

/**
 * @brief 基于支配者的值编号,参考自eac
 * @param[in] *fnc
 * @param[in] *bb
 */
void DVNT(Function *fnc, BasicBlock *bb) {
    if (bb->visited) return;
    bb->visited = 1;
    createNewScopeHH(state, bb);
    if (fnc->isCopy && fnc->firstBB == bb && bb->idom == bb) {
        size_t               i;
        valueNumberExprTable temp, *ptr;
        struct instruction  *inst = NULL;
        // 第一个基本块的IR全是IR_OP_ARGUMENT
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_ARGUMENT) {
                Value    con = vector_get(&constantParams, i);
                uint32_t ind = createValueNumber(state, &con);
                setValueNumber(fnc, state, &inst->ret, ind);
                inst->redundance = true;
            }
        }
    }
    struct instruction *inst = NULL;

    vector_each3(&bb->inst, inst) {
        if (inst->op == IR_OP_PHI) {
            phiFunction *phi = inst->args[0].phi;
            phiParam    *p;
            size_t       i;

            if (phiIsRedundance(fnc, inst)) {
                inst->redundance = true;

                inst->ret.VNind = setValueNumber(
                    fnc, state, &inst->ret,
                    vector_get(&inst->args[0].phi->param, 0).v.VNind);
            } else {
                // 为什么不处理phi参数?如果是常量,我没有实现,如果是循环还没有实现
                //  inst->ret.VNind = createValueNumber(state, &inst->ret);
                //  vector_each_address(&phi->param, i, p) {
                //      if (!p->isVN) {
                //          p->v.VNind = createValueNumber(state, &p->v);
                //          p->isVN = true;
                //      }
                //  }
            }
        }
    }

    valueNumberExpr      expr;

    // 先试试其正确性
    size_t               i;
    valueNumberExprTable temp, *ptr;
    vector_each(&bb->inst, i, inst) {
        switch (inst->op) {
            // 不可以消除判断,和后端实现有关
            OP3_3AC_COND
            inst->args[1].VNind = createValueNumber(state, &inst->args[1]);
            inst->args[0].VNind = createValueNumber(state, &inst->args[0]);
            break;
            // 如果表达式找到了值,那就代表表达式是冗余,标记删除即可,不要重写,不可以被定义第二次
            /*  x = y op z
            y op z,found v :  vn[x] = found v
            y op z,not found: vn[x] = x
             */
            OP3_3AC_OPER
            inst->args[1].VNind = createValueNumber(state, &inst->args[1]);
            inst->args[0].VNind = createValueNumber(state, &inst->args[0]);
            expr.op = inst->op;
            expr.VNarg1 = inst->args[0].VNind;
            expr.VNarg2 = inst->args[1].VNind;
            // TODO 简化表达式
            int t = simplifiedExpr(fnc, state, &expr, inst->args[0].ty,
                                   inst->args[1].ty);
            if (t == 1) {
                setValueNumber(fnc, state, &inst->ret, expr.VNarg1);
                // 标志冗余
                inst->redundance = true;
                // if(isPHIparam(&inst->ret)){
                //     inst->args[]
                // }
            }
            if (t == 2) {
                inst->args[0].VNind = expr.VNarg1;
                inst->args[1].VNind = expr.VNarg2;
                goto _exprlabe;
            } else {
            _exprlabe:
                ptr = FindExpr(fnc, state, &expr);
                if (!ptr) {
                    if (isSwap(inst->op)) {
                        uint32_t tmp = expr.VNarg1;
                        expr.VNarg1 = expr.VNarg2;
                        expr.VNarg2 = tmp;
                        ptr = FindExpr(fnc, state, &expr);
                        if (ptr) goto _foundexpr;
                    }
                    uint32_t ind = createValueNumber(state, &inst->ret);
                    ptr = createExpr(fnc, state, &expr, ind);
                    inst->ret.VNind = ind;
                } else {
                _foundexpr:
                    setValueNumber(fnc, state, &inst->ret, ptr->ind);
                    inst->ret.VNind = ptr->ind;
                    // 标志冗余
                    inst->redundance = true;
                }
            }
            break;
            case IR_OP_GETITEMPTR: {
                arrayAddr *ptr = inst->args[1].arrayAddr;
                Value     *index;
                size_t     j;
                vector_each_address(&ptr->index, j, index) {
                    index->VNind = createValueNumber(state, index);
                }
            } break;

            // 这个表达式需要入吗?
            case IR_OP_MOV: {
                uint32_t ind = createValueNumber(state, &inst->args[0]);
                inst->ret.VNind = setValueNumber(fnc, state, &inst->ret, ind);
                inst->args[0].VNind = ind;
                // 没有记录&inst->args[0],作用域要消除的
            } break;
            case IR_OP_I2F:
            case IR_OP_F2I:
            case IR_OP_NOT:
            case IR_OP_NOTF:
            case IR_OP_POS:
            case IR_OP_POSF:
            case IR_OP_NEG:
            case IR_OP_NEGF: {
                inst->args[0].VNind = createValueNumber(state, &inst->args[0]);
                expr.op = inst->op;
                expr.VNarg1 = inst->args[0].VNind;
                expr.VNarg2 = 0;
                int t = simplifiedExpr(fnc, state, &expr, inst->args[0].ty,
                                       inst->args[1].ty);
                if (t == 1) {
                    setValueNumber(fnc, state, &inst->ret, expr.VNarg1);
                    // 标志冗余
                    inst->redundance = true;
                } else {
                    ptr = FindExpr(fnc, state, &expr);
                    if (ptr) {
                        setValueNumber(fnc, state, &inst->ret, ptr->ind);
                        inst->ret.VNind = ptr->ind;
                        // 标志冗余
                        inst->redundance = true;
                    } else {
                        uint32_t ind = createValueNumber(state, &inst->ret);
                        ptr = createExpr(fnc, state, &expr, ind);
                        inst->ret.VNind = ind;
                    }
                }

            } break;

            case IR_OP_RETF:
            case IR_OP_RETI:
                inst->ret.VNind = createValueNumber(state, &inst->ret);
                break;
            case IR_OP_ADDPTR:
                inst->args[1].VNind = createValueNumber(state, &inst->args[1]);
                break;
            case IR_OP_GETPTR:
                inst->args[1].VNind = createValueNumber(state, &inst->args[1]);
                break;
            case IR_OP_SOTRE:
                inst->args[0].VNind = createValueNumber(state, &inst->args[0]);
                break;
            case IR_OP_RET:
            case IR_OP_LOAD:
            case IR_OP_CALL:
            case IR_OP_IF:
                break;
            case IR_OP_PARAM:
                if (!(inst->ret.ty & IS_ARRAY)) {
                    inst->ret.VNind = createValueNumber(state, &inst->ret);
                }
                break;
            case IR_OP_ARGUMENT:
                // inst->ret.VNind = createValueNumber(state, &inst->ret);
                break;
            case IR_OP_PHI:
                break;
            default:
                assert(0);
        }
    }
    BasicBlock *b;
    if (bb->nextblock) {
        vector_each3(&bb->nextblock->inst, inst) {
            if (inst->op == IR_OP_PHI) {
                phiFunction *phi = inst->args[0].phi;
                phiParam    *p;
                size_t       i;
                vector_each_address(&phi->param, i, p) {
                    uint32_t vn;
                    // if (findValueNumber(fnc, state, &p->v, &vn)) {
                    //     p->isVN = true;
                    //     p->v.ssaind = vn;
                    // }
                }
            }
        }
    }
    if (bb->branchblock) {
        vector_each3(&bb->branchblock->inst, inst) {
            if (inst->op == IR_OP_PHI) {
                phiFunction *phi = inst->args[0].phi;
                phiParam    *p;
                size_t       i;
                vector_each_address(&phi->param, i, p) {
                    uint32_t vn;
                    if (findValueNumber(fnc, state, &p->v, &vn)) {
                        p->isVN = true;
                        p->v.ssaind = vn;
                    }
                }
            }
        }
    }
    vector_each3(&fnc->BBs, b) {
        if (b->idom == b) continue;
        if (b->idom == bb) {
            DVNT(fnc, b);
        }
    }

    // free scope of bb
    // vector_each3(&exprList, ptr) { HASH_DEL(state->exprTable, ptr); }
    deleteScopeHH(state, bb);
}

/**
 * @brief 删除冗余代码
 * @param[in] *fnc
 */
static void deteleRedundance(Function *fnc) {
    size_t              i;
    BasicBlock         *bb = NULL;
    uint32_t            v0, v1, v2;
    struct instruction *inst = NULL;
    vector_template(struct instruction *, newinst);
    vector_each(&fnc->BBs, i, bb) {
        vector_init(&newinst);
        for (size_t i = 0;
             i < bb->inst.count ? (inst = bb->inst.data[i], 1) : 0;) {
            if (inst->redundance) {
            } else {
                vector_push_back(&newinst, inst);
            }
            i++;
        }
        if (bb->inst.data) {
            vector_free(&bb->inst);
        }
        bb->inst.capacity = newinst.capacity;
        bb->inst.count = newinst.count;
        bb->inst.data = newinst.data;
    }
}

static void replaceIR(Function *fnc) {
    size_t              i;
    BasicBlock         *bb = NULL;
    uint32_t            v0, v1, v2;
    struct instruction *inst = NULL;
    vector_each(&fnc->BBs, i, bb) vector_each3(&bb->inst, inst) {
        switch (inst->op) {
            // 不可以消除判断,和后端实现有关
            OP3_3AC_COND
            v1 = inst->args[0].VNind;
            v2 = inst->args[1].VNind;
            valueNumberToValue(fnc, state, v1, inst->args[0].ty,
                               &inst->args[0]);
            valueNumberToValue(fnc, state, v2, inst->args[1].ty,
                               &inst->args[1]);
            break;
            OP3_3AC_OPER
            v0 = inst->ret.VNind;
            v1 = inst->args[0].VNind;
            v2 = inst->args[1].VNind;
            valueNumberToValue(fnc, state, v0, inst->ret.ty, &inst->ret);
            valueNumberToValue(fnc, state, v1, inst->args[0].ty,
                               &inst->args[0]);
            valueNumberToValue(fnc, state, v2, inst->args[1].ty,
                               &inst->args[1]);
            break;
            case IR_OP_GETITEMPTR: {
                arrayAddr *ptr = inst->args[1].arrayAddr;
                Value     *index;
                size_t     j;
                vector_each_address(&ptr->index, j, index) {
                    valueNumberToValue(fnc, state, index->VNind, index->ty,
                                       index);
                }
            } break;

            case IR_OP_MOV:
            case IR_OP_I2F:
            case IR_OP_F2I:
            case IR_OP_NOT:
            case IR_OP_NOTF:
            case IR_OP_POS:
            case IR_OP_POSF:
            case IR_OP_NEG:
            case IR_OP_NEGF:
                v0 = inst->ret.VNind;
                v1 = inst->args[0].VNind;
                valueNumberToValue(fnc, state, v0, inst->ret.ty, &inst->ret);
                valueNumberToValue(fnc, state, v1, inst->args[0].ty,
                                   &inst->args[0]);

                break;

            case IR_OP_RETF:
            case IR_OP_RETI:

                v0 = inst->ret.VNind;
                valueNumberToValue(fnc, state, v0, inst->ret.ty, &inst->ret);
                break;
            case IR_OP_ADDPTR:
            case IR_OP_GETPTR:
                v2 = inst->args[1].VNind;
                valueNumberToValue(fnc, state, v2, inst->args[1].ty,
                                   &inst->args[1]);
                break;
            case IR_OP_SOTRE:
                v1 = inst->args[0].VNind;
                valueNumberToValue(fnc, state, v1, inst->args[0].ty,
                                   &inst->args[0]);
                break;
            case IR_OP_PARAM:
                if (!(inst->ret.ty & IS_ARRAY)) {
                    v0 = inst->ret.VNind;
                    valueNumberToValue(fnc, state, v0, inst->ret.ty,
                                       &inst->ret);
                }
                break;
            case IR_OP_RET:
            case IR_OP_LOAD:
            case IR_OP_CALL:
            case IR_OP_IF:

            case IR_OP_ARGUMENT:
                break;
            case IR_OP_PHI: {
                // phiFunction *phi = inst->args[0].phi;
                // phiParam    *p;
                // size_t       i;
                // vector_each_address(&phi->param, i, p) {
                //     if (p->isVN) {
                //         v1 = p->v.ssaind;
                //         valueNumberToValue(fnc, state, v1, p->v.ty,
                //         &p->v);
                //     }
                // }
            } break;
            default:
                assert(0);
        }
    }
}
/**
 * @brief 初始化该pass要用到的信息
 * @param[in] *fnc
 */
static void initDVNT(Function *fnc) {
    state = sy_malloc(sizeof(valueNumberState));
    LOOPALLBB({LOOPBBALLINST({ inst->redundance = false; })})
    vector_init(&state->hhlist);
    vector_init(&state->VNPools);
    LOOPALLBB({ bb->visited = 0; })
}
static void clearDVNT(Function *fnc) {
    vector_free(&state->hhlist);
    valueNumberTable *vn;
    vector_each3(&state->VNPools, vn) { free(vn); }
    vector_free(&state->VNPools);
    sy_free(state);
}

bool DVNTPass(Function *fnc) {
    // TransformArrayPass(fnc);

    FILE *fp = fopen("d1.txt", "w");
    printBasicAllBlock(fp, fnc);
    fclose(fp);

    initDVNT(fnc);
    // 内存泄漏

    DVNT(fnc, fnc->firstBB);
    deteleRedundance(fnc);
    replaceIR(fnc);
    clearDVNT(fnc);
    fp = fopen("d2.txt", "w");
    printBasicAllBlock(fp, fnc);
    fclose(fp);
}
Symbol *copySymbol(Symbol *s) {
    Symbol *ptr = sy_malloc(sizeof(Symbol));
    memcpy(ptr, s, sizeof(Symbol));
    return ptr;
}
ssaSymbol *copyssaSymbol(ssaSymbol *s) {
    ssaSymbol *ptr = sy_malloc(sizeof(ssaSymbol));
    memcpy(ptr, s, sizeof(ssaSymbol));
    return ptr;
}
instruction *copyinst(instruction *i) {
    instruction *ptr = sy_malloc(sizeof(instruction));
    memcpy(ptr, i, sizeof(instruction));
    if (i->op == IR_OP_GETITEMPTR) assert(0);
    if (i->op == IR_OP_PHI) {
        phiFunction *phi = i->args[0].phi;

        phiFunction *tmpphi = sy_malloc(sizeof(phiFunction));
        ptr->args[0].phi = tmpphi;
        phiParam tmp;
        memset(&tmp, 0, sizeof(phiParam));

        phiParam p;
        size_t   j;
        vector_each(&phi->param, j, p) {
            tmp = p;
            tmp.copybb = p.bb->bbName;
            vector_push_back(&tmpphi->param, tmp);
        }
    }
    return ptr;
}
vector_template(BasicBlock *, copyBBs);

BasicBlock *copyBasicBlock(BasicBlock **copy, BasicBlock *bb) {
    if (!bb) return NULL;
    if (!bb->visited) {
        bb->visited = 1;
        BasicBlock *ptr = sy_malloc(sizeof(BasicBlock));
        vector_push_back(&copyBBs, ptr);
        *copy = ptr;
        memcpy(ptr, bb, sizeof(BasicBlock));
        ptr->idom = bb->idom->bbName;

        vector_copy(&ptr->inst, &bb->inst);
        vector_clear(&ptr->inst);
        instruction *inst;
        vector_each3(&bb->inst, inst) {
            vector_push_back(&ptr->inst, copyinst(inst));
        }
        copyBasicBlock(&ptr->nextblock, bb->nextblock);
        copyBasicBlock(&ptr->branchblock, bb->branchblock);
    }
}
// 该函数名字不能一样,还要加入stacksymbol,数组和phi信息要注意用到了地址,建议如果函数用到了数组就不使用此pass
Function *copyFunction(Function *dest, Symbol **s) {
    vector_clear(&copyBBs);
    Function *fnc = sy_malloc(sizeof(Function));
    vector_copy(&fnc->symPool, &dest->symPool);
    vector_clear(&fnc->symPool);
    Symbol *sptr;
    vector_each3(&dest->symPool, sptr) {
        vector_push_back(&fnc->symPool, copySymbol(sptr));
    }

    vector_copy(&fnc->SSAValuePool, &dest->SSAValuePool);
    vector_clear(&fnc->SSAValuePool);
    ssaSymbol *ssaptr;
    vector_each3(&dest->SSAValuePool, ssaptr) {
        vector_push_back(&fnc->SSAValuePool, copyssaSymbol(ssaptr));
    }
    char name[1024];
    sprintf(name, "%s_copy_%x", dest->name, fnc);
    fnc->name = strdup(name);
    assert(scope == 0);
    Type    t = {IS_FNC};
    Symbol *sym = pushSymbol(defineToken(name, strlen(name))->tok, &t);
    *s = sym;
    sym->fnc = fnc;
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&dest->BBs, i, bb) { bb->visited = 0; }

    copyBasicBlock(&fnc->firstBB, dest->firstBB);
    vector_clear(&fnc->BBs);
    vector_each3(&copyBBs, bb) {
        BasicBlock *b = NULL;
        vector_each3(&copyBBs, b) {
            if (b->bbName == vector_len(&fnc->BBs)) {
                vector_push_back(&fnc->BBs, b);
                break;
            };
        }
    }

    // 后支配性质

    vector_each3(&dest->BBs, bb) {
        BasicBlock *postb = NULL;
        BasicBlock *copybb = vector_get(&fnc->BBs, bb->bbName);
        vector_each3(&bb->postdf, postb) {
            vector_push_back(&copybb->postdf,
                             vector_get(&fnc->BBs, postb->bbName));
        }
    }
    vector_each3(&dest->BBs, bb) {
        if (bb->nextblock) {
            BasicBlock *copybb = vector_get(&fnc->BBs, bb->bbName);
            copybb->nextblock = vector_get(&fnc->BBs, bb->nextblock->bbName);
        }
        if (bb->branchblock) {
            BasicBlock *copybb = vector_get(&fnc->BBs, bb->bbName);
            copybb->branchblock =
                vector_get(&fnc->BBs, bb->branchblock->bbName);
        }
    }
    // vector_copy(&fnc->BBs, &dest->BBs);
    // vector_clear(&fnc->BBs);
    // vector_each3(&dest->BBs, bb) {
    //     vector_push_back(&fnc->BBs, vector_get(&fnc->BBs, bb->bbName));
    // }

    vector_copy(&fnc->reverse_bb_list, &dest->reverse_bb_list);
    vector_clear(&fnc->reverse_bb_list);
    vector_each3(&dest->reverse_bb_list, bb) {
        vector_push_back(&fnc->reverse_bb_list,
                         vector_get(&fnc->BBs, bb->bbName));
    }

    vector_each(&fnc->BBs, i, bb) {
        bb->idom = vector_get(&fnc->BBs, ((size_t)bb->idom));
    }
    vector_each(&fnc->BBs, i, bb) {
        size_t       i;
        instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_PHI) {
                phiFunction *phi = inst->args[0].phi;

                phiParam    *p;
                size_t       j;
                vector_each_address(&phi->param, j, p) {
                    p->bb = vector_get(&fnc->BBs, p->copybb);
                }
            }
        }
    }
    fnc->firstBB = vector_get(&fnc->BBs, dest->firstBB->bbName);
    fnc->finalBB = vector_get(&fnc->BBs, dest->finalBB->bbName);
    fnc->isCopy = 1;

    vector_each3(&fnc->SSAValuePool, ssaptr) {
        instruction *inst = NULL;
        vector_each3(&fnc->BBs, bb) {
            vector_each3(&bb->inst, inst) {
                switch (inst->op) {
                    OP3_3AC_COND
                    OP3_3AC_OPER
                    case IR_OP_MOV:
                    case IR_OP_POS:
                    case IR_OP_POSF:
                    case IR_OP_NOT:
                    case IR_OP_NOTF:
                    case IR_OP_NEG:
                    case IR_OP_NEGF:
                    case IR_OP_I2F:
                    case IR_OP_F2I:
                    case IR_OP_CALL:
                    case IR_OP_ARGUMENT:
                    case IR_OP_LOAD:
                    case IR_OP_PHI:
                        if (inst->ret.ty & IS_SSA &&
                            inst->ret.ssaind == ssaptr->ssaind) {
                            ssaptr->def = inst;
                            goto next;
                        }
                        break;
                    case IR_OP_RET:
                    case IR_OP_RETF:
                    case IR_OP_RETI:
                    case IR_OP_PARAM:
                    case IR_OP_SOTRE:
                        break;
                    case IR_OP_GETITEMPTR:
                        assert(0);
                    default:
                        assert(0);
                }
            }
        }
    next:
        NULL;
    }

    return fnc;

    // 还有idom信息要恢复,finalbb
}
// 用于复制函数传播,先处理参数
bool copy_DVNTPass(Function *fnc) {
    // TransformArrayPass(fnc);

    FILE *fp = fopen("copy1.txt", "w");
    printBasicAllBlock(fp, fnc);
    fclose(fp);

    initDVNT(fnc);
    // 内存泄漏

    DVNT(fnc, fnc->firstBB);
    deteleRedundance(fnc);
    replaceIR(fnc);
    clearDVNT(fnc);
    fp = fopen("copy2.txt", "w");
    printBasicAllBlock(fp, fnc);
    fclose(fp);
}
// 函数是否可以进行过程间优化
bool isCopy(Function *fnc) {
    LOOPALLBB({LOOPBBALLINST({
        if (inst->op == IR_OP_GETITEMPTR) return false;
    })})
    return true;
}
bool DeadCodeEliminationPass(Function *fnc);
vector_template(instruction *, constantParamInsts);

/**
 * @brief 过程间常量传播
 * @param[in] *fnc
 */
bool functionCopy(Function *fnc) {
    vector_clear(&constantParams);
    vector_clear(&constantParamInsts);
    size_t      i;
    BasicBlock *bb = NULL;

    bool        isConstant = true;
    vector_each(&fnc->BBs, i, bb) {
        size_t       i;
        instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_PARAM) {
                // 是否全是常量
                if (ValueIsImm(&inst->ret)) {
                    vector_push_back(&constantParams, inst->ret);
                    vector_push_back(&constantParamInsts, inst);
                } else {
                    isConstant = false;
                }
            } else if (inst->op == IR_OP_CALL) {
                // 有常量参数并且全是
                Function *ptr = getGlobalSymbol(inst->args[0].symbol)->fnc;
                if (vector_len(&constantParams) && isConstant &&
                    (!ptr->isExtern) && isCopy(ptr)) {
                    Symbol   *s;
                    Function *copyFnc = copyFunction(ptr, &s);
                    DVNTPass(copyFnc);
                    DeadCodeEliminationPass(copyFnc);
                    inst->args[0].symbol = s->ind;
                    instruction *_inst;
                    vector_each3(&constantParamInsts, _inst) {
                        _inst->op = IR_OP_NOP;
                    }
                }
                vector_clear(&constantParams);
                vector_clear(&constantParamInsts);
            }
        }
    }
}