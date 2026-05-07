#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

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
vector_template(instruction *, instWorkList);

/**
 * @brief 该操作是否关键,数组无法转为ssa标量,因此对数组的存储全部标记
 * @param[in] *inst
 */
static bool isCritical(instruction *inst) {
    switch (inst->op) {
        case IR_OP_SOTRE:
            if (inst->ret.ty & IS_GLOBAL) return true;
            break;
        case IR_OP_ARGUMENT:
            return true;
        case IR_OP_PARAM:
        case IR_OP_RETF:
        case IR_OP_RETI:
        case IR_OP_RET:
        case IR_OP_CALL:
            return true;
    }
    return false;
}

/**
 * @brief 标记代码,IR_OP_CALL和IR_OP_PARAM是绝对会标记且不会被删除
 * @param[in] *fnc
 */

bool isJump(instruction *ins) {
    switch (ins->op) {
        OP3_3AC_COND
        return true;
        case IR_OP_IF:
            return true;
    }
    return false;
}
// 被用到过的数组
// 单独扫描一遍存储,如果存储的指针是来自已经标记的数组则搞定他
static void codeMarkPass(Function *fnc) {
    vector_clear(&instWorkList);
    LOOPALLBB({LOOPBBALLINST({
        inst->marked = false;
        if (isCritical(inst)) {
            inst->marked = true;
            vector_push_back(&instWorkList, inst);
        }
    })})
    instruction *i;
    ssaSymbol   *ssa;
    int          changed = 0;

repo:
    while (vector_len(&instWorkList)) {
        vector_pop_back(&instWorkList, i);
        fprintf(stderr, "i%d", i->op);
        switch (i->op) {
            OP3_3AC_OPER
            OP3_3AC_COND
            if (i->args[1].ty & IS_SSA) {
                ssa = getSSAlVariable(fnc, i->args[1].ssaind);
                if (!ssa->def) assert(0);
                if (!ssa->def->marked) {
                    ssa->def->marked = true;
                    vector_push_back(&instWorkList, ssa->def);
                }
            }
            if (i->args[0].ty & IS_SSA) {
                ssa = getSSAlVariable(fnc, i->args[0].ssaind);
                if (!ssa->def) assert(0);
                if (!ssa->def->marked) {
                    ssa->def->marked = true;
                    vector_push_back(&instWorkList, ssa->def);
                }
            }
            break;
            // IR_OP_SOTRE有可能是指针、数组
            case IR_OP_GETPTR:
                assert(0);
                break;
            case IR_OP_GETITEMPTR: {
                fprintf(stderr, "e1");
                if (!(i->args[0].ty & IS_GLOBAL)) {
                    if (!(i->args[0].ty & IS_SSA)) {
                        fprintf(stderr, "e2");
                        Symbol *s = getLocalVariable(fnc, i->args[0].symbol);
                        s->arraymarked = 1;
                    }
                }
                size_t     j = 0;
                Value      v;
                arrayAddr *ptr = i->args[1].arrayAddr;
                fprintf(stderr, "e3");
                vector_each(&ptr->index, j, v) {
                    fprintf(stderr, "e6");
                    ssa = getSSAlVariable(fnc, v.ssaind);
                    if (!ssa->def) assert(0);
                    if (!ssa->def->marked) {
                        ssa->def->marked = true;
                        fprintf(stderr, "e4");
                        vector_push_back(&instWorkList, ssa->def);
                    }
                }
                fprintf(stderr, "e5");
                break;
            }
            case IR_OP_SOTRE:
                if (!((i->ret.ty & IS_ARRAY)) || !((i->ret.ty & IS_GLOBAL))) {
                    ssa = getSSAlVariable(fnc, i->ret.ssaind);
                    if (!ssa->def) assert(0);
                    if (!ssa->def->marked) {
                        ssa->def->marked = true;
                        vector_push_back(&instWorkList, ssa->def);
                    }
                }
                if (i->args[0].ty & IS_SSA) {
                    ssa = getSSAlVariable(fnc, i->args[0].ssaind);
                    if (!ssa->def) assert(0);
                    if (!ssa->def->marked) {
                        ssa->def->marked = true;
                        vector_push_back(&instWorkList, ssa->def);
                    }
                }
                break;

            case IR_OP_MOV:
            case IR_OP_POS:
            case IR_OP_POSF:
            case IR_OP_NOT:
            case IR_OP_NOTF:
            case IR_OP_NEG:
            case IR_OP_NEGF:
            case IR_OP_I2F:
            case IR_OP_F2I:
            // call必须标记,但是函数返回值不能强制标记,需要看是否用到了
            case IR_OP_CALL:
                if (i->args[0].ty & IS_SSA) {
                    ssa = getSSAlVariable(fnc, i->args[0].ssaind);
                    if (!ssa->def) assert(0);
                    if (!ssa->def->marked) {
                        ssa->def->marked = true;
                        vector_push_back(&instWorkList, ssa->def);
                    }
                }
                break;
            case IR_OP_LOAD:
                if (i->args[0].ty & IS_GLOBAL) break;
                if (i->args[0].ty & IS_SSA) {
                    ssaSymbol *s = getSSAlVariable(fnc, i->args[0].ssaind);
                    if (!s->def) {
                        assert(0);
                    }
                    if (!s->def->marked) {
                        s->def->marked = true;
                        // s->def->
                        vector_push_back(&instWorkList, s->def);
                    }
                }
                break;
            case IR_OP_PARAM:
                if (i->ret.ty & IS_SSA) {
                    ssa = getSSAlVariable(fnc, i->ret.ssaind);
                    if (!ssa->def) assert(0);
                    if (!ssa->def->marked) {
                        ssa->def->marked = true;
                        vector_push_back(&instWorkList, ssa->def);
                    }
                } else if (i->ret.ty & IS_ARRAY) {
                    if (!(i->ret.ty & IS_SSA)) {
                        Symbol *s = getLocalVariable(fnc, i->ret.symbol);
                        s->arraymarked = 1;
                    }
                }
                break;
            case IR_OP_IF:
                if (i->ret.ty & IS_SSA) {
                    ssa = getSSAlVariable(fnc, i->ret.ssaind);
                    if (!ssa->def) assert(0);
                    if (!ssa->def->marked) {
                        ssa->def->marked = true;
                        vector_push_back(&instWorkList, ssa->def);
                    }
                }
                break;
            case IR_OP_RETF:
            case IR_OP_RETI:
                if (i->ret.ty & IS_SSA) {
                    ssa = getSSAlVariable(fnc, i->ret.ssaind);
                    if (!ssa->def) assert(0);
                    if (!ssa->def->marked) {
                        ssa->def->marked = true;
                        vector_push_back(&instWorkList, ssa->def);
                    }
                }
                break;
            case IR_OP_PHI: {
                phiFunction *phi = i->args[0].phi;
                phiParam    *p;
                size_t       i;
                vector_each_address(&phi->param, i, p) {
                    ssa = getSSAlVariable(fnc, p->v.ssaind);
                    if (!ssa->def) assert(0);
                    if (!ssa->def->marked) {
                        ssa->def->marked = true;
                        vector_push_back(&instWorkList, ssa->def);
                    }
                }
            } break;
            case IR_OP_RET:
                break;
            case IR_OP_ARGUMENT:
                i->marked = true;
                break;

            default:
                assert(0);
        }
        BasicBlock  *b;
        instruction *_i = i;
        /* 防止此类情况,以免删除i有关的指令
        int main() {
            int sum = 0;
            int i = 1;
            while (i < 100) {
                i = i + 1;
                sum = sum * 4;
            }
            return sum;
        } */

        LOOPALLBB({LOOPBBALLINST({
            if (inst == _i) {
                vector_each3(&bb->postdf, b) {
                    if (vector_len(&b->inst) == 0) continue;
                    _i = vector_final(&b->inst);
                    if (isJump(_i))
                        if (!_i->marked) {
                            _i->marked = true;
                            vector_push_back(&instWorkList, _i);
                        }
                }
            }
        })})
    }
    fprintf(stderr, "e2");

    changed = 0;
    {
        size_t      i;
        BasicBlock *bb = NULL;
        vector_each(&fnc->BBs, i, bb) {
            size_t              i;
            struct instruction *inst = NULL;
            vector_each(&bb->inst, i, inst) {
                if (inst->op == IR_OP_SOTRE) {
                    if (inst->ret.ty & IS_GLOBAL) {
                        if (!inst->marked) {
                            inst->marked = 1;
                        }
                        if (inst->args[0].ty & IS_SSA) {
                            ssaSymbol *ssa =
                                getSSAlVariable(fnc, inst->args[0].ssaind);
                            if (!ssa->def->marked) {
                                ssa->def->marked = 1;
                                changed = 1;
                                vector_push_back(&instWorkList, ssa->def);
                            }
                        }
                        continue;
                    }
                    ssaSymbol   *ssa = getSSAlVariable(fnc, inst->ret.ssaind);
                    instruction *def = ssa->def;
                    if (def->op == IR_OP_GETITEMPTR) {
                        // 全局数组
                        if (def->args[0].ty & IS_GLOBAL) {
                            inst->marked = 1;
                            if (!def->marked) {
                                def->marked = 1;
                                changed = 1;
                                vector_push_back(&instWorkList, def);
                            }
                            if (inst->args[0].ty & IS_SSA) {
                                ssaSymbol *ssa =
                                    getSSAlVariable(fnc, inst->args[0].ssaind);
                                if (!ssa->def->marked) {
                                    ssa->def->marked = 1;
                                    changed = 1;
                                    vector_push_back(&instWorkList, ssa->def);
                                }
                            }
                        } else if (def->args[0].ty & IS_SSA) {
                            // 数组指针
                            inst->marked = 1;
                            if (!def->marked) {
                                def->marked = 1;
                                changed = 1;
                                vector_push_back(&instWorkList, def);
                            }
                            if (inst->args[0].ty & IS_SSA) {
                                ssaSymbol *ssa =
                                    getSSAlVariable(fnc, inst->args[0].ssaind);
                                if (!ssa->def->marked) {
                                    ssa->def->marked = 1;
                                    changed = 1;
                                    vector_push_back(&instWorkList, ssa->def);
                                }
                            }
                        } else {
                            // 局部数组
                            if (!(def->args[0].ty & IS_SSA)) {
                                Symbol *s =
                                    getLocalVariable(fnc, def->args[0].symbol);
                                if (s->arraymarked) {
                                    inst->marked = 1;
                                    if (!def->marked) {
                                        def->marked = 1;
                                        changed = 1;
                                        vector_push_back(&instWorkList, def);
                                    }
                                    if (inst->args[0].ty & IS_SSA) {
                                        ssaSymbol *ssa = getSSAlVariable(
                                            fnc, inst->args[0].ssaind);
                                        if (!ssa->def->marked) {
                                            ssa->def->marked = 1;
                                            changed = 1;
                                            vector_push_back(&instWorkList,
                                                             ssa->def);
                                        }
                                    }
                                }
                            } else {
                                assert(0);
                            }
                        }
                    }
                }
            }
        }
    }
    if (changed) goto repo;
}

/**
 * @brief 删除未标记的代码
 * @param[in] *fnc
 */
static void sweepCodePass(Function *fnc) {
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        size_t              i;
        struct instruction *inst = NULL;
        vector_template(struct instruction *, newinst);
        vector_init(&newinst);
        vector_each(&bb->inst, i, inst) {
            if (!inst->marked) {
            } else {
                vector_push_back(&newinst, inst);
            }
        }
        bb->inst.capacity = newinst.capacity;
        bb->inst.count = newinst.count;
        bb->inst.data = newinst.data;
    }
}

static void dfsbb(BasicBlock *bb) {
    if (!bb) return;
    if (!bb->visited) return;
    bb->visited = true;
    bb->marked = true;
    dfsbb(bb->nextblock);
    dfsbb(bb->branchblock);
    return;
}
/**
 * @brief 清除死基本快,好像破坏了SSA属性?
 * @param[in] *fnc
 */
void clearDeadBB(Function *fnc) {
    LOOPALLBB({ bb->visited = 0; })
    fnc->finalBB->marked = 1;
    fnc->firstBB->marked = 1;
    dfsbb(fnc->firstBB->nextblock);
    dfsbb(fnc->firstBB->branchblock);
    size_t      i;
    BasicBlock *bb = NULL;
    for (i = 0; i < fnc->BBs.count ? (bb = fnc->BBs.data[i], 1) : 0;) {
        if (!bb->marked) {
            vector_remove(&fnc->BBs, i);
            continue;
        }
        i++;
    }
}

/**
 * @brief 消除无用代码
 * @param[in] *fnc
 */
void EliminationUselessCodePass(Function *fnc) {
    fprintf(stderr, "codeMarkPass\n");
    codeMarkPass(fnc);
    fprintf(stderr, "sweepCodePass\n");
    sweepCodePass(fnc);
}
/**
 * @brief 消除无用控制流
 * @param[in] *fnc
 */
void        EliminatingUselessControlFlow(Function *fnc) {}
static void initDCE(Function *fnc) {
    LOOPALLBB({LOOPBBALLINST({ inst->marked = false; })})
}
/**
 * @brief 死代码消除pass
 * @param[in] *fnc
 */
bool DeadCodeEliminationPass(Function *fnc) {
    FILE *fp = fopen("dec1.txt", "w");
    printBasicAllBlock(fp, fnc);
    fclose(fp);
    initDCE(fnc);
    EliminationUselessCodePass(fnc);
    // clearDeadBB(fnc);
    FILE *fp1 = fopen("dec3.txt", "w");
    printBasicAllBlock(fp1, fnc);
    fclose(fp1);
}
