#include <limits.h>
#include <math.h>
#include <stdio.h>

#include "../../include/bitset.h"
#include "../../include/function.h"
#include "../../include/ir.h"
#include "../../include/lexer.h"
#include "../../include/pass.h"
#include "../../include/util.h"
#include "../../include/utlist.h"
#include "../../include/value.h"
#include "../../include/vector.h"
#include "asm.h"
#include "riscv.h"

static Reg functionArgument[] = {REG_a0, REG_a1, REG_a2, REG_a3,
                                 REG_a4, REG_a5, REG_a6, REG_a7};
static Reg FPfunctionArgument[] = {REG_fa0, REG_fa1, REG_fa2, REG_fa3,
                                   REG_fa4, REG_fa5, REG_fa6, REG_fa7};
static Reg RVfpcaller[] = {REG_fa0, REG_fa1, REG_fa2, REG_fa3,
                           REG_fa4, REG_fa5, REG_fa6, REG_fa7};
static Reg RVfpcallee[] = {REG_fs0, REG_fs1, REG_fs2,  REG_fs3,
                           REG_fs4, REG_fs5, REG_fs6,  REG_fs7,
                           REG_fs8, REG_fs9, REG_fs10, REG_fs11};
static Reg RVcaller[] = {REG_a0, REG_a1, REG_a2, REG_a3,
                         REG_a4, REG_a5, REG_a6, REG_a7};
static Reg RVcallee[] = {REG_s0, REG_s1, REG_s2, REG_s3, REG_s4,  REG_s5,
                         REG_s6, REG_s7, REG_s8, REG_s9, REG_s10, REG_s11};

#undef _X
#define _X(op) {.reg = REG_##op},

static Register iRegisterPools[] = {__R};
static Register FPRegisterPools[] = {__R{.reg = REG_a0},
                                     {.reg = REG_a0},
                                     {.reg = REG_a0},
                                     {.reg = REG_a0},
                                     __FR};
void            optasmgen_functionProlog(Function *fnc, FILE *fp);
void            optasmgen_functionEpilogue(Function *fnc, FILE *fp);
// 循环寻找回边
static void     Loopnestingforest2(Function *fnc, BasicBlock *bb,
                                   BasicBlock *root) {
    if (!bb) return;
    BasicBlock *ptr;
    vector_each3(&bb->doms, ptr) {
        if (ptr->nextblock == root || ptr->branchblock == root) {
            root->isloop = true;
            root->backblock = ptr;
            return;
        } else {
            Loopnestingforest2(fnc, ptr, root);
        }
    }
}
static void Loopnestingforest(Function *fnc) {
    BasicBlock *ptr;
    LOOPALLBB({
        vector_each3(&bb->doms, ptr) {
            Loopnestingforest2(fnc,ptr,ptr);
}
})
}

struct LoopNestingForest {
    BasicBlock *head;
    BasicBlock *back;
    vector_template(BasicBlock *, bb);
};
static vector_template(BasicBlock *, loopstack);
static vector_template(struct LoopNestingForest, LNFs);
static void findLoop(Function *fnc, m_bitset_ptr live, BasicBlock *head,
                     BasicBlock *bb) {
    bitset_init(live);
    bitset_resize(live, fnc->BBs.count);
    bitset_init_empty(live);
    vector_clear(&loopstack);
    initInedges(fnc);
    buildInedges(NULL, fnc->firstBB);
    vector_push_back(&loopstack, bb);
    bitset_set_at(live, head->bbName, 1);
    bitset_set_at(live, bb->bbName, 1);
    while (vector_len(&loopstack)) {
        BasicBlock *m = vector_final(&loopstack);
        loopstack.count--;
        BasicBlock *p;
        vector_each3(&m->inedges, p) {
            if (!bitset_get(live, p->bbName)) {
                bitset_set_at(live, p->bbName, 1);
                vector_push_back(&loopstack, p);
            }
        }
    }
}

static void __loop(Function *fnc) {
    vector_clear(&LNFs);
    Loopnestingforest(fnc);
    bitset_t                  live;
    struct LoopNestingForest *looptmp;
    int                       i;
    struct LoopNestingForest  tmp = {0};
    LOOPALLBB({
        if (bb->backblock) {
            tmp.head = bb;
            tmp.back = bb->backblock;
            vector_push_back(&LNFs, tmp);
        }
    })
    vector_each_address(&LNFs, i, looptmp) {
        findLoop(fnc, live, looptmp->head, looptmp->back);
        bitset_it_t it;
        for (bitset_it(it, live); !bitset_end_p(it); bitset_next(it)) {
            bool c = *bitset_cref(it);
            if (c) {
                BasicBlock *ptr = vector_get(&fnc->BBs, it->index);
                vector_push_back(&looptmp->bb, ptr);
            }
        }
    }
}

static vector_template(liveIntervals, intervals);
static inline void destroyIntervals(Function *fnc) {
    if (intervals.data != NULL) {
        vector_free(&intervals);
    }
    vector_init(&intervals);
}
// 消除phi函数
static void genMoves(Function *fnc) {
    BasicBlock *n = NULL, *joker = NULL;

    initInedges(fnc);
    buildInedges(NULL, fnc->firstBB);
    size_t      i;
    BasicBlock *_bb = NULL;
    vector_each(&fnc->BBs, i, _bb) {
        BasicBlock *bb;
        vector_each3(&_bb->inedges, bb) {
            BasicBlock *p = bb;
            BasicBlock *b = _bb;
            bool        b1 = bb->nextblock && bb->branchblock;
            bool        b2 = vector_len(&_bb->inedges) > 1;
            joker = NULL;
            if (b1 && b2) {
                n = createBasicBlock(fnc);
                if (p->nextblock == b) {
                    p->nextblock = n;
                    joker = p->nextblock;
                } else {
                    p->branchblock = n;
                    joker = p->branchblock;
                }
                n->nextblock = b;
            } else {
                n = p;
            }
            {
                BasicBlock  *bb = b;
                size_t       i;
                instruction *inst = NULL;
                vector_each(&bb->inst, i, inst) {
                    if (inst->op == IR_OP_PHI) {
                        phiFunction *phi = inst->args[0].phi;
                        phiParam    *p;
                        size_t       i;
                        vector_each_address(&phi->param, i, p) {
                            assert(p->bb);
                            if (p->bb == joker) {
                                Value v = createSSATempVariable(fnc, p->v.ty);
                                assert(!ValueIsImm(&v));
                                setCurrentBB(fnc, n);
                                instruction *tmp =
                                    createMOV(fnc, &inst->ret, &v);
                                tmp->join = tmp;
                            } else if (p->bb == n) {
                                Value v = createSSATempVariable(fnc, p->v.ty);
                                assert(!ValueIsImm(&v));
                                setCurrentBB(fnc, n);
                                instruction *tmp =
                                    createMOV(fnc, &inst->ret, &p->v);
                            }
                        }
                    }
                }
            }
        }
    }
}
// 指令编号
static void _instructionNumbering(size_t *ind, BasicBlock *bb) {
    size_t tmp = *ind;
    LOOPBBALLINST({
        inst->n = tmp;
        tmp++;
    })
    *ind = tmp;
}
static void _DepthFirstOrderingNum(Function *fnc, BasicBlock *bb, size_t *ind) {
    if (!bb) return;
    if (bb->visited) return;
    bb->visited = 1;
    _instructionNumbering(ind, bb);
    _DepthFirstOrderingNum(fnc, bb->nextblock, ind);
    _DepthFirstOrderingNum(fnc, bb->branchblock, ind);
}
static void DepthFirstOrderingNum(Function *fnc) {
    size_t ind = 0;
    LOOPALLBB({ bb->visited = 0; })
    _DepthFirstOrderingNum(fnc, fnc->firstBB, &ind);
    fnc->instNumbering = 1;
}
// 活跃性分析
static void initLiveness(Function *fnc) {
    LOOPALLBB({
        bitset_init(bb->liveOut);
        bitset_init(bb->liveIn);
        bitset_init(bb->LiveUse);
        bitset_init(bb->LiveDef);
        bitset_resize(bb->liveOut, vector_len(&fnc->SSAValuePool));
        bitset_resize(bb->liveIn, vector_len(&fnc->SSAValuePool));
        bitset_resize(bb->LiveUse, vector_len(&fnc->SSAValuePool));
        bitset_resize(bb->LiveDef, vector_len(&fnc->SSAValuePool));
    })
}
// 初始化声明区间
static inline void initIntervals(Function *fnc) {
    vector_init(&intervals);
    vector_reserve(&intervals, vector_len(&fnc->SSAValuePool));
    intervals.count = intervals.capacity;
}
static void setFrom(ssaSymbol *v, size_t start) {
    liveIntervals *ll = vector_get_address(&intervals, v->ssaind);

    if (start == 50) {
        printf("1");
    }
    ll->from = start;
    ll->v = v->ssaind;
    ll->use = 1;
    ll->reg = -1;
}
static void addRange3(ssaSymbol *v, size_t start, size_t end) {
    liveIntervals *ll = vector_get_address(&intervals, v->ssaind);
    if (ll->from > start) {
        ll->from = start;
    }
    if (ll->to < end) {
        ll->to = end;
    }

    ll->v = v->ssaind;
    ll->reg = -1;
    ll->use = 1;
}
static int _maxNumber;
static int findMaxNumber(Function *fnc, BasicBlock *bb) {
    struct LoopNestingForest *looptmp;
    int                       i;
    _maxNumber = -1;
    vector_each_address(&LNFs, i, looptmp) {
        if (looptmp->head == bb) {
            BasicBlock *b;
            vector_each3(&looptmp->bb, b) {
                if (vector_len(&b->inst) == 0) continue;

                int n = vector_get(&b->inst, vector_len(&b->inst) - 1)->n + 1;
                if (n > _maxNumber) {
                    _maxNumber = n;
                }
            }
            break;
        }
    }
    return _maxNumber;
}
// 构建生命区间
static void buildIntervals2(Function *fnc) {
    BasicBlock *g_b = NULL;
    bitset_t    live;

    size_t      i;
    BasicBlock *bb = NULL;

    vector_each_reverse(&fnc->reverse_bb_list, i, bb) {
        g_b = bb;
        bitset_init(live);
        bitset_set(live, bb->liveIn);

        LOOPALLBBNEXT({
            bitset_or(live, bb->liveIn);
            LOOPBBALLPHI({
                phiFunction *phi = inst->args[0].phi;
                phiParam    *p;
                size_t       i;
                vector_each_address(&phi->param, i, p) {
                    if (p->bb == g_b) {
                        bitset_set_at(live, p->v.ssaind, 1);
                    }
                }
            })
        })
        if (vector_len(&bb->inst) == 0) {
            bitset_set(bb->liveIn, live);
            continue;
        }
        bitset_it_t it;
        for (bitset_it(it, live); !bitset_end_p(it); bitset_next(it)) {
            bool c = *bitset_cref(it);
            if (c) {
                size_t b = vector_get(&bb->inst, 0)->n;
                size_t e = vector_get(&bb->inst, vector_len(&bb->inst) - 1)->n;
                addRange3(getSSAlVariable(fnc, it->index), b, e);
            }
        }

        // LOOP inst in b
        instruction *inst;
        size_t       i;
        instruction *prevInst = NULL;
        BasicBlock  *back;
        size_t       beg = vector_get(&bb->inst, 0)->n;
        vector_each_reverse(&bb->inst, i, inst) {
            switch (inst->op) {
                OP3_3AC_COND
                if (inst->args[1].ty & IS_SSA) {
                    bitset_set_at(live, inst->args[1].ssaind, 1);
                    addRange3(getSSAlVariable(fnc, inst->args[1].ssaind), beg,
                              inst->n);
                }
                if (inst->args[0].ty & IS_SSA) {
                    bitset_set_at(live, inst->args[0].ssaind, 1);
                    addRange3(getSSAlVariable(fnc, inst->args[0].ssaind), beg,
                              inst->n);
                }
                break;
                OP3_3AC_OPER
                bitset_set_at(live, inst->ret.ssaind, 0);
                setFrom(getSSAlVariable(fnc, inst->ret.ssaind), inst->n);
                if (inst->args[1].ty & IS_SSA) {
                    bitset_set_at(live, inst->args[1].ssaind, 1);
                    addRange3(getSSAlVariable(fnc, inst->args[1].ssaind), beg,
                              inst->n);
                }
                if (inst->args[0].ty & IS_SSA) {
                    bitset_set_at(live, inst->args[0].ssaind, 1);
                    addRange3(getSSAlVariable(fnc, inst->args[0].ssaind), beg,
                              inst->n);
                }
                break;
                OP2_3AC
                if (inst->op != IR_OP_SOTRE) {
                    setFrom(getSSAlVariable(fnc, inst->ret.ssaind), inst->n);
                    if (inst->ret.ty & IS_SSA)
                        bitset_set_at(live, inst->ret.ssaind, 0);
                }
                if (inst->args[0].ty & IS_SSA) {
                    bitset_set_at(live, inst->args[0].ssaind, 1);
                    addRange3(getSSAlVariable(fnc, inst->args[0].ssaind), beg,
                              inst->n);
                }
                // 退出ssa后,mov也相当于使用了,考虑删除phi函数的操作
                if (inst->op == IR_OP_MOV) {
                    if (inst->ret.ty & IS_SSA) {
                        if (inst->ret.ssaind == 22) {
                            printf("1");
                        }
                        liveIntervals *ll =
                            vector_get_address(&intervals, inst->ret.ssaind);
                        if (ll->from == 0) {
                            bitset_set_at(live, inst->ret.ssaind, 1);
                            setFrom(getSSAlVariable(fnc, inst->ret.ssaind),
                                    inst->n);
                        } else {
                            addRange3(getSSAlVariable(fnc, inst->ret.ssaind),
                                      inst->n, inst->n);
                        }
                    }
                    if (inst->args[0].ty & IS_SSA) {
                        bitset_set_at(live, inst->args[0].ssaind, 1);
                        if (inst->args[0].ssaind == 22) {
                            printf("1");
                        }
                        addRange3(getSSAlVariable(fnc, inst->args[0].ssaind),
                                  beg, inst->n);
                    }
                }
                if (inst->op == IR_OP_SOTRE) {
                    if (inst->ret.ty & IS_SSA) {
                        bitset_set_at(live, inst->ret.ssaind, 1);
                        addRange3(getSSAlVariable(fnc, inst->ret.ssaind), beg,
                                  inst->n);
                    }
                }
                break;  // 这里不是bug
                case IR_OP_GETPTR:
                    // assert(0);
                    // if (inst->args[1].ty & IS_SSA) {
                    //     if (!bitset_get(live, inst->args[1].ssaind)) {
                    //         bitset_set_at(live, inst->args[1].ssaind, 1);
                    //         addRange(getSSAlVariable(fnc,
                    //         inst->args[1].ssaind),
                    //                  bb, inst->n);
                    //     }
                    // }
                    // // 只记录数组,数组产生的局部变量地址不计算
                    // if (inst->args[0].ty & IS_SSA) {
                    //     if (!bitset_get(live, inst->args[0].ssaind)) {
                    //         bitset_set_at(live, inst->args[0].ssaind, 1);
                    //         addRange(getSSAlVariable(fnc,
                    //         inst->args[0].ssaind),
                    //                  bb, inst->n);
                    //     }
                    // }
                    break;
                case IR_OP_RETI:
                case IR_OP_RETF:
                    // 替换used(被使用的变量),variable renaming
                    if (inst->ret.ty & IS_SSA) {
                        bitset_set_at(live, inst->ret.ssaind, 1);
                        addRange3(getSSAlVariable(fnc, inst->ret.ssaind), beg,
                                  inst->n);
                    }
                    break;
                case IR_OP_ADDPTR: {
                    setFrom(getSSAlVariable(fnc, inst->ret.ssaind), inst->n);
                    bitset_set_at(live, inst->ret.ssaind, 0);
                    if (inst->args[0].ty & IS_SSA) {
                        bitset_set_at(live, inst->args[0].ssaind, 1);
                        addRange3(getSSAlVariable(fnc, inst->args[0].ssaind),
                                  beg, inst->n);
                    }
                    if (inst->args[1].ty & IS_SSA) {
                        bitset_set_at(live, inst->args[1].ssaind, 1);
                        addRange3(getSSAlVariable(fnc, inst->args[1].ssaind),
                                  beg, inst->n);
                    }
                } break;
                case IR_OP_GETITEMPTR: {
                    setFrom(getSSAlVariable(fnc, inst->ret.ssaind), inst->n);
                    bitset_set_at(live, inst->ret.ssaind, 0);
                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     j;
                    if (inst->args[0].ty & IS_SSA) {
                        bitset_set_at(live, inst->args[0].ssaind, 1);
                        addRange3(getSSAlVariable(fnc, inst->args[0].ssaind),
                                  beg, inst->n);
                    }
                    vector_each_address(&ptr->index, j, index) {
                        if (index->ty & IS_SSA) {
                            bitset_set_at(live, index->ssaind, 1);
                            addRange3(getSSAlVariable(fnc, index->ssaind), beg,
                                      inst->n);
                        }
                    }
                } break;
                case IR_OP_PHI:
                    if (inst->ret.ssaind == 22) {
                        printf("1");
                    }
                    setFrom(getSSAlVariable(fnc, inst->ret.ssaind), inst->n);
                    break;
                case IR_OP_IF:
                case IR_OP_NOP:
                    break;
                case IR_OP_ARGUMENT:
                    if (inst->ret.ty & IS_SSA) {
                        bitset_set_at(live, inst->ret.ssaind, 1);
                        addRange3(getSSAlVariable(fnc, inst->ret.ssaind), beg,
                                  inst->n);
                    }
                    break;
                case IR_OP_PARAM:
                    if (inst->ret.ty & IS_SSA) {
                        bitset_set_at(live, inst->ret.ssaind, 1);
                        addRange3(getSSAlVariable(fnc, inst->ret.ssaind), beg,
                                  inst->n);
                    }
                    break;
                case IR_OP_RET:
                    break;
                default:
                    assert(0);
            }
            prevInst = inst;
        }
        if (bb->isloop) {
            bitset_it_t it;

            for (bitset_it(it, live); !bitset_end_p(it); bitset_next(it)) {
                bool c = *bitset_cref(it);
                if (c) {
                    back = bb->backblock;
                    int tmp = findMaxNumber(fnc, bb);
                    addRange3(getSSAlVariable(fnc, it->index), beg, tmp);
                }
            }
        }
        bitset_set(bb->liveIn, live);
    }
}
static void printIntervals1(FILE *fp, Function *fnc) {
    size_t        i;
    liveIntervals live;
    vector_each(&intervals, i, live) {
        if (live.use == 0) continue;
        ssaSymbol *sym = getSSAlVariable(fnc, live.v);
        if (!sym->sym) {
            fprintf(fp, "_%d", sym->valueind);
        } else if (sym->sym->isTemp) {
            fprintf(fp, "_%d", sym->sym->tmpInd);
        } else {
            fprintf(fp, "%s_%d", getToken(sym->sym->tok)->str, sym->valueind);
        }
        fprintf(fp, "(%d)", sym->ssaind);
        uint32_t ssa;
        vector_each3(&live.ssaind, ssa) {
            sym = getSSAlVariable(fnc, ssa);
            if (!sym->sym) {
                fprintf(fp, ",_%d", sym->valueind);
            } else if (sym->sym->isTemp) {
                fprintf(fp, ",_%d", sym->sym->tmpInd);
            } else {
                fprintf(fp, ",%s_%d", getToken(sym->sym->tok)->str,
                        sym->valueind);
            }
            fprintf(fp, "(%d)", sym->ssaind);
        }

        fprintf(fp, ":");
        fprintf(fp, "[%d,%d] ,", live.from, live.to);
        fprintf(fp, "  reg:%d", live.reg);
        fprintf(fp, "\n");
        fprintf(fp, "\n");
    }
}
static liveIntervals *getssaIntervals(uint32_t ssaind) {
    size_t         i;
    liveIntervals *live;
    vector_each_address(&intervals, i, live) {
        if (live->v == ssaind) {
            return live;
        }
    }
    return NULL;
}
// 函数有返回值但是没有被用到
static void processRet(Function *fnc) {
    BasicBlock *bb;
    int         i;
    vector_each(&fnc->BBs, i, bb) {
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_CALL && !(inst->ret.ty & TY_VOID)) {
                liveIntervals *tmp = getssaIntervals(inst->ret.ssaind);
                if (!tmp || tmp->to == 0) {
                    inst->ret.ty = TY_VOID;
                }
            }
        }
    }
}

static int intervalcmp(const void *a, const void *b) {
    liveIntervals *a1 = *(liveIntervals **)a;
    liveIntervals *b1 = *(liveIntervals **)b;
    return a1->from - b1->from;
}
static vector_template(liveIntervals *, allIntervals);
// 初始化寄存器
static void initRegister2(Function *fnc) {
    for (size_t i = 0; i < sizeof(iRegisterPools) / sizeof(iRegisterPools[0]);
         i++) {
        iRegisterPools[i].used = 0;
        iRegisterPools[i].isused = 0;
        iRegisterPools[i].w = 0;
        iRegisterPools[i].addr = 0;
        iRegisterPools[i].ll = NULL;
    }
    for (size_t i = 0; i < sizeof(FPRegisterPools) / sizeof(FPRegisterPools[0]);
         i++) {
        FPRegisterPools[i].used = 0;
        FPRegisterPools[i].isused = 0;
        FPRegisterPools[i].w = 0;
        FPRegisterPools[i].addr = 0;
        FPRegisterPools[i].ll = NULL;
    }
    vector_init(&allIntervals);
    liveIntervals *ll;
    size_t         i;
    vector_each_address(&intervals, i, ll) {
        if (ll->use) {
            if (ll->to != 0) vector_push_back(&allIntervals, ll);
        }
    }
    // vector_each(&allIntervals, i, ll) { printf("\n%d:%d", ll->v, ll->from); }
    qsort(allIntervals.data, vector_len(&allIntervals), sizeof(liveIntervals *),
          intervalcmp);
    vector_each(&allIntervals, i, ll) { printf("\n%d:%d", ll->v, ll->from); }
}

/* 寄存器分配 */
static bitset_t unhandled, active, inactive, handled;
// 检查过期的活动间隔
static void     checkActiveIntervals2(Function *fnc, liveIntervals *cur) {
    bitset_it_t it;
    for (bitset_it(it, active); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            liveIntervals *ll = vector_get(&allIntervals, it->index);
            size_t         i;
            liveRange     *R;
            if (ll->to < cur->from) {
                // move i to handled and add i.reg to free
                // printf("\n活动间隔过期\n");
                bitset_set_at(active, it->index, 0);
                bitset_set_at(handled, it->index, 1);
                iRegisterPools[ll->reg].used = 0;
                iRegisterPools[ll->reg].ll = NULL;
                iRegisterPools[ll->reg].llind = 0;
            } else {
                // move i to inactive and add i.reg to free
                // 不相交,可能有bug
                // if (!intervalsIsUnion(cur, ll)) {
                // new
                //  bitset_set_at(active, it->index, 0);
                //  bitset_set_at(inactive, it->index, 1);
                //  iRegisterPools[ll->reg].used = 0;
                //  iRegisterPools[ll->reg].ll = NULL;
                // }
                // size_t begin = vector_get(cur, 0)->begin;
                // vector_each(ll, i, R) {
                //     if (begin < R->begin || begin > R->end) {
                //         bitset_set_at(active, it->index, 0);
                //         bitset_set_at(inactive, it->index, 1);
                //         iRegisterPools[vector_get(ll, 0)->reg].used = 0;
                //         iRegisterPools[vector_get(ll, 0)->reg].ll = NULL;
                //     }
                // }
            }
        }
    }
}
// 生命区间是否重叠
static bool intervalsIsUnion2(liveIntervals *x, liveIntervals *y) {
    size_t     i;
    liveRange *yrange;

    if (y->from <= x->from && y->to >= x->from) return true;
    if (y->from <= x->to && y->to >= x->to) return true;

    return false;
}
static bool registerIsNull(Register *R) {
    for (size_t i = 0; i < sizeof(iRegisterPools) / sizeof(iRegisterPools[0]);
         i++) {
        if (R[i].used == 0) {
            return false;
        }
    }
    return true;
}
static bool FPregisterIsNull(Register *R) {
    for (size_t i = REG_t3 + 1;
         i < sizeof(FPRegisterPools) / sizeof(FPRegisterPools[0]); i++) {
        if (R[i].used == 0) {
            return false;
        }
    }
    return true;
}
// 计算权重
static size_t getRangeWeight2(Function *fnc, uint32_t v, size_t from,
                              size_t to) {
    size_t weight = 0;
    size_t num;
    size_t i;
    num = 0;
    LOOPALLBB({LOOPBBALLINST({
        if (!(inst->n >= from && inst->n <= to)) {
            continue;
        }
        switch (inst->op) {
            OP3_3AC_COND
            OP3_3AC_OPER
            // 替换used(被使用的变量),variable renaming
            if (inst->args[1].ty & IS_SSA) {
                if (inst->args[1].ssaind == v) {
                    num++;
                }
            }
            __attribute__((fallthrough));
            OP2_3AC
            // 替换used(被使用的变量),variable renaming
            if (inst->args[0].ty & IS_SSA) {
                if (inst->args[0].ssaind == v) {
                    num += 1;
                }
            }
            OP1_3AC
            case IR_OP_PHI:
                if (inst->ret.ty & IS_SSA) {
                    if (inst->ret.ssaind == v) {
                        num += 1;
                    }
                }
                break;  // 这里不是bug
            case IR_OP_GETITEMPTR: {
                arrayAddr *ptr = inst->args[1].arrayAddr;
                Value     *index;
                size_t     j;

                vector_each_address(&ptr->index, j, index) {
                    size_t i;
                    if (index->ty & IS_SSA && index->ssaind == v) {
                        num += 1;
                    }
                }
            } break;
        }
    })})
    return num;
}
static int8_t registerPop(Register *R) {
    for (size_t i = 0; i < sizeof(iRegisterPools) / sizeof(iRegisterPools[0]);
         i++) {
        if (R[i].used == 0) {
            return R[i].reg;
        }
    }
}
static int8_t FPregisterPop(Register *R) {
    for (size_t i = REG_t3 + 1;
         i < sizeof(FPRegisterPools) / sizeof(FPRegisterPools[0]); i++) {
        if (R[i].used == 0) {
            return R[i].reg;
        }
    }
}
/**
 * @brief 根据权重溢出寄存器
 * @param[in] *fnc
 * @param[in] *cur
 * @param[in] index
 */
static void assignMemLoc(Function *fnc, liveIntervals *cur, size_t index) {
    size_t w[100];
    for (size_t i = 0; i < 100; i++) {
        w[i] = 0;
    }

    // 初始化权重信息
    for (size_t i = 0; i < sizeof(iRegisterPools) / sizeof(iRegisterPools[0]);
         i++) {
        w[i] = 0;
    }
    liveIntervals *range;
    bitset_it_t    it;
    for (bitset_it(it, active); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            range = vector_get(&allIntervals, it->index);
            if (intervalsIsUnion2(cur, range)) {
                // w[i.reg] ← w[i.reg] + i.weight
                size_t tmp = w[range->reg];
                tmp += getRangeWeight2(fnc, range->v, range->from, range->to);
                w[range->reg] = tmp;
            }
        }
    }
    // for (bitset_it(it, unhandled); !bitset_end_p(it); bitset_next(it)) {
    //     if (*bitset_cref(it)) {
    //         range = vector_get(&allIntervals, it->index);

    //         // 固定间隔
    //         if (vector_get(range, 0)->reg > 0) {
    //             if (intervalsIsUnion(cur, range)) {
    //                 size_t     tmp = w[vector_get(range, 0)->reg];
    //                 liveRange *Ra;
    //                 vector_each3(&range->LRs, Ra) {
    //                     tmp += getRangeWeight(fnc, Ra, 0);
    //                 }
    //                 w[vector_get(range, 0)->reg] = tmp;
    //             }
    //         }
    //     }
    // }
    int    r;
    size_t weight;
_repo:
    // 找到权重最小的寄存器
    r = 0;
    weight = w[0];
    for (size_t i = 0; i < sizeof(iRegisterPools) / sizeof(iRegisterPools[0]);
         i++) {
        if (w[i] < weight) {
            r = i;
            weight = w[i];
        }
    }

    size_t curweight = 0;
    curweight = getRangeWeight2(fnc, cur->v, cur->from, cur->to);

    // if (curweight < w[r]) {
    if (1) {
        // 该区间未分配过寄存器
        cur->reg = -1;
        bitset_set_at(handled, index, 1);
        bitset_set_at(active, index, 0);
    } else {
        // // 将寄存器溢出
        // //  还有active和in中
        // //  assert(0);
        // assert(iRegisterPools[r].ll);
        // range = iRegisterPools[r].ll;
        // // printAtice(fnc, range);
        // if (vector_len(&range->LRs) == 1) {
        //     if ((vector_get(&range->LRs, 0)->end -
        //          vector_get(&range->LRs, 0)->begin) <= 3) {
        //         w[r] += 10;
        //         goto _repo;
        //     }
        // }
        // spillInfo sp;
        // sp.ll = iRegisterPools[r].ll;
        // sp.n = vector_get(&cur->LRs, 0)->begin;
        // // 每次溢出都会分配一次,过于浪费stack?
        // // sp.loc = allocStack(fnc);
        // sp.r = r;
        // sp.isMoveMem = true;
        // vector_push_back(&allSpill, sp);
        // bitset_set_at(handled, iRegisterPools[r].llind, 1);
        // bitset_set_at(active, iRegisterPools[r].llind, 0);
        // // 只有寄存器中的溢出，其他全部mem

        // // 共享同一寄存器的所有生命周期,活动和未活动全部溢出
        // for (bitset_it(it, inactive); !bitset_end_p(it); bitset_next(it)) {
        //     if (*bitset_cref(it)) {
        //         range = vector_get(&allIntervals, it->index);
        //         if (cur->reg == r) {
        //             bitset_set_at(handled, it->index, 1);
        //             bitset_set_at(inactive, it->index, 0);
        //             sp.ll = range;
        //             sp.n = vector_get(&cur->LRs, 0)->begin;
        //             // 每次溢出都会分配一次,过于浪费stack?
        //             // sp.loc = allocStack(fnc);
        //             sp.r = r;
        //             vector_push_back(&allSpill, sp);
        //         }
        //     }
        // }

        // cur->reg = r;
        // iRegisterPools[r].ll = cur;
        // iRegisterPools[r].llind = index;
    }
}
static void FPassignMemLoc(Function *fnc, liveIntervals *cur, size_t index) {
    size_t w[100];
    for (size_t i = 0; i < 100; i++) {
        w[i] = 0;
    }

    // 初始化权重信息
    for (size_t i = 0; i < sizeof(FPRegisterPools) / sizeof(FPRegisterPools[0]);
         i++) {
        w[i] = 0;
    }
    liveIntervals *range;
    bitset_it_t    it;
    for (bitset_it(it, active); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            range = vector_get(&allIntervals, it->index);
            if (intervalsIsUnion2(cur, range)) {
                // w[i.reg] ← w[i.reg] + i.weight
                size_t tmp = w[range->reg];
                tmp += getRangeWeight2(fnc, range->v, range->from, range->to);
                w[range->reg] = tmp;
            }
        }
    }
    // for (bitset_it(it, unhandled); !bitset_end_p(it); bitset_next(it)) {
    //     if (*bitset_cref(it)) {
    //         range = vector_get(&allIntervals, it->index);

    //         // 固定间隔
    //         if (vector_get(range, 0)->reg > 0) {
    //             if (intervalsIsUnion(cur, range)) {
    //                 size_t     tmp = w[vector_get(range, 0)->reg];
    //                 liveRange *Ra;
    //                 vector_each3(&range->LRs, Ra) {
    //                     tmp += getRangeWeight(fnc, Ra, 0);
    //                 }
    //                 w[vector_get(range, 0)->reg] = tmp;
    //             }
    //         }
    //     }
    // }
    int    r;
    size_t weight;
_repo:
    // 找到权重最小的寄存器
    r = 0;
    weight = w[0];
    for (size_t i = 0; i < sizeof(FPRegisterPools) / sizeof(FPRegisterPools[0]);
         i++) {
        if (w[i] < weight) {
            r = i;
            weight = w[i];
        }
    }

    size_t curweight = 0;
    curweight = getRangeWeight2(fnc, cur->v, cur->from, cur->to);

    // if (curweight < w[r]) {
    if (1) {
        // 该区间未分配过寄存器
        cur->reg = -1;
        bitset_set_at(handled, index, 1);
        bitset_set_at(active, index, 0);
    } else {
        // // 将寄存器溢出
        // //  还有active和in中
        // //  assert(0);
        // assert(iRegisterPools[r].ll);
        // range = iRegisterPools[r].ll;
        // // printAtice(fnc, range);
        // if (vector_len(&range->LRs) == 1) {
        //     if ((vector_get(&range->LRs, 0)->end -
        //          vector_get(&range->LRs, 0)->begin) <= 3) {
        //         w[r] += 10;
        //         goto _repo;
        //     }
        // }
        // spillInfo sp;
        // sp.ll = iRegisterPools[r].ll;
        // sp.n = vector_get(&cur->LRs, 0)->begin;
        // // 每次溢出都会分配一次,过于浪费stack?
        // // sp.loc = allocStack(fnc);
        // sp.r = r;
        // sp.isMoveMem = true;
        // vector_push_back(&allSpill, sp);
        // bitset_set_at(handled, iRegisterPools[r].llind, 1);
        // bitset_set_at(active, iRegisterPools[r].llind, 0);
        // // 只有寄存器中的溢出，其他全部mem

        // // 共享同一寄存器的所有生命周期,活动和未活动全部溢出
        // for (bitset_it(it, inactive); !bitset_end_p(it); bitset_next(it)) {
        //     if (*bitset_cref(it)) {
        //         range = vector_get(&allIntervals, it->index);
        //         if (cur->reg == r) {
        //             bitset_set_at(handled, it->index, 1);
        //             bitset_set_at(inactive, it->index, 0);
        //             sp.ll = range;
        //             sp.n = vector_get(&cur->LRs, 0)->begin;
        //             // 每次溢出都会分配一次,过于浪费stack?
        //             // sp.loc = allocStack(fnc);
        //             sp.r = r;
        //             vector_push_back(&allSpill, sp);
        //         }
        //     }
        // }

        // cur->reg = r;
        // iRegisterPools[r].ll = cur;
        // iRegisterPools[r].llind = index;
    }
}
static vector_template(functionCallInfo, _calls);
static void porcessFunctionCall(Function *fnc) {
    liveIntervals    *ll;
    size_t            _i;

    functionCallInfo *call;
    vector_each_address(&_calls, _i, call) {
        size_t i;
        vector_each_reverse(&allIntervals, i, ll) {
            if (ll->from <= call->num) {
                ll->callNum++;
                ll->iscall = true;
                ll->callind = _i;
                vector_push_back(&ll->callinds, _i);
                break;
            }
        }
    }
}
static functionCallInfo *findFunctionCall(Function *fnc, int index) {
    liveIntervals    *ll;
    size_t            _i;

    functionCallInfo *call;
    vector_each_address(&_calls, _i, call) {
        if (call->num == index) {
            return call;
        }
    }
    return NULL;
}
static void createFunctionCall(Function *fnc, int ind) {
    functionCallInfo CurFncCall = vector_get(&_calls, ind);
    // 再进行ir
    // param的时候，要另一种读取寄存器,这里将被移动的寄存器保存下来,并在ir
    // param中写入被保存的信息

    Register        *R = iRegisterPools;
    for (size_t i = 0; i < sizeof(RVcaller) / sizeof(RVcaller[0]); i++) {
        // if (R[RVcaller[i]].used == 1) {
        vector_push_back(&CurFncCall.reg, RVcaller[i]);
        // }
    }
    *vector_get_address(&_calls, ind) = CurFncCall;
}
// 寄存器分配
static void linearScan(Function *fnc) {
    Register *f = sy_malloc(sizeof(iRegisterPools));
    Register *fp = sy_malloc(sizeof(FPRegisterPools));

    bitset_resize(unhandled, vector_len(&allIntervals));
    bitset_resize(active, vector_len(&allIntervals));
    bitset_resize(inactive, vector_len(&allIntervals));
    bitset_resize(handled, vector_len(&allIntervals));

    bitset_init_empty(unhandled);
    bitset_init_empty(active);
    bitset_init_empty(inactive);
    bitset_init_empty(handled);

    bitset_init_true(unhandled);
    liveIntervals *cur = NULL;

    porcessFunctionCall(fnc);
    bitset_it_t it;
    for (bitset_it(it, unhandled); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            bitset_set_at(unhandled, it->index, 0);
            cur = vector_get(&allIntervals, it->index);

            // active
            checkActiveIntervals2(fnc, cur);

            // inactive
            // checkInactiveIntervals(fnc, cur);
            memcpy(f, iRegisterPools, sizeof(iRegisterPools));
            memcpy(fp, FPRegisterPools, sizeof(FPRegisterPools));
            // unhandled
            {  // 寄存器预先分配

                bitset_it_t it;
                for (bitset_it(it, unhandled); !bitset_end_p(it);
                     bitset_next(it)) {
                    if (*bitset_cref(it)) {
                        liveIntervals *ll =
                            vector_get(&allIntervals, it->index);
                        if (intervalsIsUnion2(ll, cur) && ll->reg >= 0) {
                            f[ll->reg].used = 1;
                        }
                    }
                }
            }
            // BUG:未处理：unhandled的每个固定间隔 i 与 cur 重叠
            ssaSymbol *ssa = getSSAlVariable(fnc, cur->v);
            if (ssa->ty & TY_FLOAT && !(ssa->ty & IS_ARRAY) &&
                !(ssa->ty & IS_POINTER)) {
                if (FPregisterIsNull(fp)) {
                    FPassignMemLoc(fnc, cur, it->index);  //
                } else {
                    int reg;
                    if (cur->reg < 0) {
                        reg = FPregisterPop(fp);
                        cur->reg = reg;
                    } else {
                        reg = cur->reg;
                    }
                    FPRegisterPools[reg].used = 1;
                    FPRegisterPools[reg].isused = 1;
                    FPRegisterPools[reg].ll = cur;
                    FPRegisterPools[reg].llind = it->index;

                    bitset_set_at(active, it->index, 1);
                }
            } else {
                if (registerIsNull(f)) {
                    assignMemLoc(fnc, cur, it->index);  //
                } else {
                    int reg;
                    if (cur->reg < 0) {
                        reg = registerPop(f);
                        cur->reg = reg;
                    } else {
                        reg = cur->reg;
                    }
                    iRegisterPools[reg].used = 1;
                    iRegisterPools[reg].isused = 1;
                    iRegisterPools[reg].ll = cur;
                    iRegisterPools[reg].llind = it->index;

                    bitset_set_at(active, it->index, 1);
                }
            }

            if (cur->iscall) {
                size_t call;

                printf("call :\n%d", cur->from);
                vector_each3(&cur->callinds, call) {
                    createFunctionCall(fnc, call);
                }
            }
        }
    }
}
static void mappingRegister3(Function *fnc, uint32_t ssaind, liveRange *range,
                             int8_t reg) {
    size_t n;
    size_t i;
    n = i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            switch (inst->op) {
                OP3_3AC_OPER
                OP3_3AC_COND
                if (inst->args[1].ty & IS_SSA &&
                    inst->args[1].ssaind == ssaind) {
                    if (reg >= 0) {
                        inst->args[1].ty |= IS_REG;
                    }
                    inst->args[1].reg = reg;
                }
                if (inst->args[0].ty & IS_SSA &&
                    inst->args[0].ssaind == ssaind) {
                    if (reg >= 0) {
                        inst->args[0].ty |= IS_REG;
                    }
                    inst->args[0].reg = reg;
                }
                if (inst->ret.ty & IS_SSA && inst->ret.ssaind == ssaind) {
                    if (reg >= 0) {
                        inst->ret.ty |= IS_REG;
                    }
                    inst->ret.reg = reg;
                }
                break;
                OP2_3AC
                if ((inst->args[0].ty & IS_SSA) &&
                    (inst->args[0].ssaind == ssaind)) {
                    if (reg >= 0) {
                        inst->args[0].ty |= IS_REG;
                    }
                    inst->args[0].reg = reg;
                }
                if ((inst->ret.ty & IS_SSA) && (inst->ret.ssaind == ssaind)) {
                    if (reg >= 0) {
                        inst->ret.ty |= IS_REG;
                    }
                    inst->ret.reg = reg;
                }
                break;
                OP1_3AC
                if ((inst->ret.ty & IS_SSA) && (inst->ret.ssaind == ssaind)) {
                    if (reg >= 0) {
                        inst->ret.ty |= IS_REG;
                    }
                    inst->ret.reg = reg;
                }
                break;
                case IR_OP_ADDPTR: {
                    if (inst->ret.ssaind == ssaind) {
                        if (reg >= 0) {
                            inst->ret.ty |= IS_REG;
                        }
                        inst->ret.reg = reg;
                    }
                    // 数组指针
                    if ((inst->args[0].ty & IS_SSA) &&
                        (inst->args[0].ssaind == ssaind)) {
                        if (reg >= 0) {
                            inst->args[0].ty |= IS_REG;
                        }
                        inst->args[0].reg = reg;
                    }
                    if (inst->args[1].ty & IS_SSA &&
                        inst->args[1].ssaind == ssaind) {
                        if (reg >= 0) {
                            inst->args[1].ty |= IS_REG;
                        }
                        inst->args[1].reg = reg;
                    }
                } break;
                case IR_OP_GETITEMPTR: {
                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     j;
                    size_t     i;
                    if (inst->ret.ssaind == ssaind) {
                        if (reg >= 0) {
                            inst->ret.ty |= IS_REG;
                        }
                        inst->ret.reg = reg;
                    }
                    // 数组指针
                    if ((inst->args[0].ty & IS_SSA) &&
                        (inst->args[0].ssaind == ssaind)) {
                        if (reg >= 0) {
                            inst->args[0].ty |= IS_REG;
                        }
                        inst->args[0].reg = reg;
                    }
                    vector_each_address(&ptr->index, j, index) {
                        size_t i;
                        if (index->ty & IS_SSA && index->ssaind == ssaind) {
                            if (reg >= 0) {
                                index->ty |= IS_REG;
                            }
                            index->reg = reg;
                        }
                    }
                } break;
                case IR_OP_PHI:
                case IR_OP_RET:
                case IR_OP_NOP:
                    break;
                default:
                    assert(0);
            }
        }
    }
}
/**
 * @brief 将生命周期分配的寄存器映射到指令中
 * @param[in] *fnc
 */
static void mappingAllRegister(Function *fnc) {
    size_t         i;
    liveIntervals *live;
    int            reg;
    vector_each(&allIntervals, i, live) {
        // 跳过被优化消除的变量
        size_t    i;
        liveRange range;
        {
            reg = live->reg;
            range.begin = live->from;
            range.end = live->to;
            reg = live->reg;
            mappingRegister3(fnc, live->v, &range, reg);
            uint32_t tmpv;
            vector_each3(&live->ssaind, tmpv) {
                mappingRegister3(fnc, tmpv, &range, reg);
            }
        }
    }
}

static void initFunctionCall(Function *fnc) {
    vector_clear(&_calls);
    bool              one = true;
    size_t            i;
    BasicBlock       *bb = NULL;
    functionCallInfo *call = NULL;

    functionCallInfo  f = {0};
    vector_init(&f.params);

    vector_each(&fnc->BBs, i, bb) {
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_CALL) {
                if (inst->ret.ty & TY_VOID)
                    f.isret = false;
                else
                    f.isret = true;
                if (one) {
                    f.num = inst->n;
                    vector_push_back(&_calls, f);
                } else {
                    vector_push_back(&_calls, f);
                }
                // init
                f.num = 0;
                f.isret = false;
                vector_init(&f.params);
                vector_init(&f.insts1);
                vector_init(&f.insts2);
                one = true;
            } else if (inst->op == IR_OP_PARAM) {
                // 防止多个参数识别为多个调用
                if (one) {
                    one = false;
                    f.num = inst->n;
                    vector_push_back(&f.params, inst);
                } else {
                    vector_push_back(&f.params, inst);
                }
            }
        }
    }
}
/**
 * @brief 预先分配寄存器
 * @param[in] *fnc
 * @param[in] ssaind
 * @param[in] reg
 */
static void mappingRegister2(Function *fnc, uint32_t ssaind, int8_t reg) {
    size_t         i;
    liveIntervals *live;

    vector_each(&allIntervals, i, live) {
        if (live->v == ssaind) {
            live->reg = reg;
            return;
        }
        uint32_t tmp;
        vector_each3(&live->ssaind, tmp) {
            if (live->from != 0 && live->to != 0) {
                assert(0);
            }
            if (tmp == ssaind) {
                live->reg = reg;
                return;
            }
        }
    }
    assert(0);
    return;
}
// 寄存器预分配
static void preRegisterAllocation(Function *fnc) {
    size_t      paramNum = 0;
    size_t      i;
    BasicBlock *bb = NULL;
    int         loc = 0;
    vector_each(&fnc->BBs, i, bb) {
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_ARGUMENT) {
                // 参数可以传入8个进入寄存器a0-a7
                // 剩下得参数放入栈,还有考虑加载
                if (paramNum >= 8) {
                } else if (inst->ret.ty & IS_POINTER ||
                           inst->ret.ty & IS_ARRAY) {
                    mappingRegister2(fnc, inst->ret.ssaind,
                                     functionArgument[paramNum]);
                } else if (inst->ret.ty & TY_FLOAT) {
                    mappingRegister2(fnc, inst->ret.ssaind,
                                     FPfunctionArgument[paramNum]);
                } else if (inst->ret.ty & TY_INT) {
                    mappingRegister2(fnc, inst->ret.ssaind,
                                     functionArgument[paramNum]);
                } else {
                    assert(0);
                }
                paramNum++;
            } else {
                paramNum = 0;
            }
            switch (inst->op) {
                // ret 可能与IR_OP_PARAM起到冲突,不要用
                // case IR_OP_RETI:
                //     if (inst->ret.ty & IS_SSA && inst->ret.ty & TY_INT)
                //         mappingRegister2(fnc, inst->ret.ssaind, REG_a0);
                //     else if (inst->ret.ty & IS_SSA && inst->ret.ty &
                //     TY_FLOAT)
                //         assert(0);
                //     break;
                case IR_OP_CALL:
                    paramNum = 0;
                    // if (inst->ret.ty & IS_SSA && inst->ret.ty & TY_INT)
                    //     mappingRegister2(fnc, inst->ret.ssaind, REG_a0);
                    // else if (inst->ret.ty & IS_SSA && inst->ret.ty &
                    // TY_FLOAT)
                    //     assert(0);
                    // else if (inst->ret.ty & TY_VOID)
                    //     ;
                    // else
                    //     assert(0);
                    break;
                    // 值有2种状态,一种是由于溢出,值在内存种,还有就是函数调用时保护caller寄存器被移动了
                    // case IR_OP_PARAM:
                    //     if (paramNum >= 7) {
                    //         assert(0);  // 剩下得参数放入栈
                    //     }
                    //     // 指针地址
                    //     if (inst->ret.ty & TY_FLOAT && inst->ret.ty & IS_SSA)
                    //     {
                    //         mappingRegister2(fnc, inst->ret.ssaind,
                    //                          FPfunctionArgument[paramNum]);
                    //     } else if (inst->ret.ty & TY_INT && inst->ret.ty &
                    //     IS_SSA) {
                    //         mappingRegister2(fnc, inst->ret.ssaind,
                    //                          functionArgument[paramNum]);
                    //     }
                    //     paramNum++;
                    //     break;
            }
        }
    }
}
FILE        *llfp;
static FILE *irfp;
static void  joinValue(Function *fnc) {
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_PHI) {
                liveIntervals *ll =
                    vector_get_address(&intervals, inst->ret.ssaind);
                if (ll->to == 0) {
                    ll = ll->join;
                }
                phiFunction *phi = inst->args[0].phi;
                phiParam    *p;
                size_t       i;
                vector_each_address(&phi->param, i, p) {
                    assert(!(p->v.ty & IS_IMM));
                    liveIntervals *ptr =
                        vector_get_address(&intervals, p->v.ssaind);
                    ssaSymbol *ssa = getSSAlVariable(fnc, ll->v);
                    addRange3(ssa, ptr->from, ptr->to);
                    if (ptr != ll) {
                        ptr->from = 0;
                        ptr->to = 0;
                    }
                    ptr->join = ll;
                    vector_push_back(&ll->ssaind, p->v.ssaind);
                    uint32_t tmpv;
                    if (ptr != ll) {
                        vector_each3(&ptr->ssaind, tmpv) {
                            vector_push_back(&ll->ssaind, tmpv);
                        }
                    }
                }
            }
        }
    }
}
/**
 * @brief 线性寄存器分配
 * @param[in] *fnc
 */
void optlinearScanRegisterAllocPass(Function *fnc) {
    Loopnestingforest(fnc);
    __loop(fnc);
    struct LoopNestingForest *looptmp;
    int                       i;
    vector_each_address(&LNFs, i, looptmp) {
        printf("head bb:%d:", looptmp->head->bbName);
        int         i;
        BasicBlock *b;
        vector_each3(&looptmp->bb, b) { printf("%d\t", b->bbName); }
        printf("\n");
    }
    destroyIntervals(fnc);

    LOOPALLBB({LOOPBBALLINST({ inst->join = inst; })})
    // vector_clear(&_calls);
    // vector_clear(&allSpill);
    genMoves(fnc);

    // instructionNumbering(fnc);
    DepthFirstOrderingNum(fnc);
    if (!irfp) {
        irfp = fopen("ir.txt", "w");
    } else {
        irfp = fopen("ir.txt", "a");
    }
    printBasicAllBlock(irfp, fnc);
    fclose(irfp);
    // printBasicAllBlock(stdout, fnc);
    BasicBlock *bb;
    // vector_each_reverse(&fnc->reverse_bb_list, i, bb) {
    //     printf("%d\n", bb->bbName);
    // }
    LOOPALLBB({ bitset_resize(bb->live, vector_len(&fnc->SSAValuePool)); })
    initIntervals(fnc);
    // printBasicAllBlock(stdout, fnc);
    initLiveness(fnc);

    // livenessAnalysis(fnc);
    LOOPALLBB({ bb->visited = 0; })
    // vector_clear(&fnc->reverse_bb_list);
    // reverserCFG(fnc, fnc->finalBB);
    // vector_each(&fnc->reverse_bb_list, i, bb) { printf("%d\n", bb->bbName); }
    buildIntervals2(fnc);
    joinValue(fnc);
    // printIntervals1(stderr, fnc);

    // printBasicAllBlock(stdout, fnc);

    // printIntervals(stderr, fnc);
    // printCFG(fnc);
    processRet(fnc);

    initRegister2(fnc);
    initFunctionCall(fnc);
    printIntervals1(stdout, fnc);
    preRegisterAllocation(fnc);

    // __loop(fnc);
    // instructionNumbering(fnc);
    if (llfp) {
        llfp = fopen("ll.txt", "a");
    } else {
        llfp = fopen("ll.txt", "w");
    }
    linearScan(fnc);
    fprintf(llfp, "%s\n", fnc->name);
    printIntervals1(llfp, fnc);
    fclose(llfp);
    mappingAllRegister(fnc);
}

// 生成指令
// 用于优化版本
// 为了传入参数,不允许分配
static inline int allocStack2(Function *fnc, int size) {
    fnc->loc1 += size;
    if (fnc->loc1 >= 540000) {
        assert(0);
    }
    return fnc->loc1 - size;
}
extern asminstruction *asmlist;
static void            asmprocessFPMemoryLoad1(Function *fnc, FILE *fp, Reg r,
                                               ssaSymbol *s1) {
    asminstruction *asmptr;
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    assert(s1->loc != 0);
    assert(r >= REG_fa0);
    // fprintf(fp, "li      t2, %d\n", s1->loc);
    asmptr = createAsm4(ASM_li, REG_t2, s1->loc);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add      t2, t2,t3\n");
    asmptr = createAsm2(ASM_add, REG_t2, REG_t2, REG_sp);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "flw      %s, 0(t2)\n", regName[r]);
    asmptr = createAsm7(ASM_flw, r, REG_t2, 0);
    DL_APPEND(asmlist, asmptr);
}
static void asmprocessFPMemoryStore1(Function *fnc, FILE *fp, Reg r,
                                     ssaSymbol *s1) {
    asminstruction *asmptr;
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    assert(r >= REG_fa0);
    // fprintf(fp, "li      t2, %d\n", s1->loc);
    asmptr = createAsm4(ASM_li, REG_t2, s1->loc);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add      t2, t2,t3\n");
    asmptr = createAsm2(ASM_add, REG_t2, REG_t2, REG_sp);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "fsw      %s, 0(t2)\n", regName[r]);
    asmptr = createAsm7(ASM_fsw, r, REG_t2, 0);
    DL_APPEND(asmlist, asmptr);
}
static void asmprocessMemoryLoad1(Function *fnc, FILE *fp, Reg r,
                                  ssaSymbol *s1) {
    asminstruction *asmptr;
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    assert(s1->loc != 0);

    // fprintf(fp, "li      t2, %d\n", s1->loc);
    asmptr = createAsm4(ASM_li, REG_t2, s1->loc);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add      t2, t2,t3\n");
    asmptr = createAsm2(ASM_add, REG_t2, REG_t2, REG_sp);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "ld      %s, 0(t2)\n", regName[r]);
    if (s1->ty & IS_POINTER || s1->ty & IS_ARRAY) {
        asmptr = createAsm7(ASM_ld, r, REG_t2, 0);
    } else {
        asmptr = createAsm7(ASM_lw, r, REG_t2, 0);
    }
    DL_APPEND(asmlist, asmptr);
}

static void asmloadImm(Function *fnc, FILE *fp, Reg r, Value *v) {
    asminstruction *asmptr;
    if (!ValueIsInt(v)) {
        ValueIsInt(v);
    }
    assert(ValueIsInt(v));
    int i = getImmIntValue(v);
    // riscv 支持 12bit 立即数
    // li 伪指令
    // fprintf(fp, "li   %s, %d\n", regName[r], i);
    asmptr = createAsm4(ASM_li, r, i);
    DL_APPEND(asmlist, asmptr);
}
// SSATODO
static void asmprocessMemoryStore1(Function *fnc, FILE *fp, Reg r,
                                   ssaSymbol *s1) {
    asminstruction *asmptr;
    // ssaSymbol *s1 = getSSAlVariable(fnc, v->ssaind);
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    // fprintf(fp, "li      t2, %d\n", s1->loc);
    asmptr = createAsm4(ASM_li, REG_t2, s1->loc);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add      t2, t2,t3\n");
    asmptr = createAsm2(ASM_add, REG_t2, REG_t2, REG_sp);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "sd      %s, 0(t2)\n", regName[r]);
    if (s1->ty & IS_POINTER || s1->ty & IS_ARRAY) {
        asmptr = createAsm7(ASM_sd, r, REG_t2, 0);
    } else {
        asmptr = createAsm7(ASM_sw, r, REG_t2, 0);
    }
    DL_APPEND(asmlist, asmptr);
}
static inline bool isSpill(Value *v) {
    if (v->ty & IS_SSA && v->ty & IS_REG) {
        return false;
    }
    return true;
}
static vector_template(Symbol *, zeorArrays);
// 函数的返回值如果不是void，就可以分配rax, a= fnc()
// 优化叶子函数
static bool isprintf;
size_t      getSymbolSize(Symbol *sym);
void        optprocessZeroArray(FILE *fp) {
    Symbol *s;
    vector_each3(&zeorArrays, s) {
        size_t size = getSymbolSize(s) * 2;
        size_t size1 = getSymbolSize(s);
        fprintf(fp, ".bss\n");
        fprintf(fp, ".align 2\n");
        fprintf(fp, ".type zeroarray_%x,@object\n", s);
        fprintf(fp, ".size zeroarray_%x,%lld\n", s, size);

        fprintf(fp, ".zeroarray_%x:\n", s);

        // 用于指定一个占据4个字节的空数据项,该数据项的值始终为02。这个伪指令通常用于初始化内存中的数据
        fprintf(fp, ".zero %lld\n", size);

        fprintf(fp, "\n");
    }
}
typedef struct RVcon {
    size_t num;
    Value  v;
} RVcon;
static vector_template(RVcon, _con);
static void asmloadFPImm(Function *fnc, FILE *fp, Reg r, Value *v) {
    assert(ValueIsFloat(v));
    asminstruction *asmptr;
    RVcon           con;
    con.num = vector_len(&_con);
    con.v = *v;
    vector_push_back(&_con, con);
    // fprintf(fp, "la     %s, ._float%d\n", regName[REG_a4], con.num);
    char buff[1024];
    sprintf(buff, "._float%d", con.num);
    asmptr = createAsm8(ASM_la, REG_CON, strdup(buff));
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "flw     %s, 0(%s)\n", regName[r], regName[REG_a4]);
    asmptr = createAsm7(ASM_flw, r, REG_CON, 0);
    DL_APPEND(asmlist, asmptr);
}
// 处理字符串和小数字面量
void optprocessCon(FILE *fp) {
    RVcon v;
    vector_each3(&_con, v) {
        if (v.v.ty & TY_FLOAT) {
            fprintf(fp, "\n.data\n._float%d:\n.word	%d\n", v.num,
                    *((int *)&v.v.imm.f));
        } else if (v.v.ty & TY_VOID && v.v.ty & IS_POINTER) {
            fprintf(fp, "\n.data\n._str%d:\n.string	\"%s\"\n", v.num,
                    v.v.imm.str);
        } else {
            assert(0);
        }
    }

    fprintf(fp, "\n");
}

bool issavereg(functionCallInfo *curCall, int r) {
    int reg;
    vector_each3(&curCall->reg, reg) {
        if (r == reg) return true;
    }
    return false;
}
bool isfpsavereg(functionCallInfo *curCall, int r) {
    int reg;
    for (size_t j = REG_t3 + 1;
         j < sizeof(FPRegisterPools) / sizeof(FPRegisterPools[0]); j++) {
        if (FPRegisterPools[j].isused == 1) {
            if (r == j) return true;
        }
    }
    return false;
}
int getsaveregaddr(functionCallInfo *curCall, int r) {
    int reg;
    vector_each3(&curCall->reg, reg) {
        if (r == reg) return i * 8 + 320;
    }
    assert(0);
}
int getfpsaveregaddr(functionCallInfo *curCall, int r) {
    int reg;
    int i;
    vector_each(&curCall->reg, i, reg) { i++; }
    for (size_t j = REG_t3 + 1;
         j < sizeof(FPRegisterPools) / sizeof(FPRegisterPools[0]); j++) {
        if (FPRegisterPools[j].isused == 1) {
            if (r == j) return i * 8 + 320;
            i++;
        }
    }
    assert(0);
}
size_t getArraySize2(ArrayType *ty);
void   asmloadFPImm(Function *fnc, FILE *fp, Reg r, Value *v);
void   asmloadSymbol1(Function *fnc, FILE *fp, Reg r, Value *v) {
    asminstruction *asmptr;
    Symbol         *s = getGlobalSymbol(v->symbol);
    char           *label = getToken(s->tok)->str;
    // fprintf(fp, "la     %s, .%s\n", regName[r], label);
    char            buff[1024];
    // if (s->type & IS_ARRAY && s->iszero) {
    //     sprintf(buff, ".zeroarray_%x", s);
    // } else {
    sprintf(buff, ".%s", label);
    // }
    asmptr = createAsm8(ASM_la, r, strdup(buff));
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp,
    //         "lui     %s, %%hi(%s)\n"
    //         "addi      %s, %s,%%lo(%s)\n",
    //         regName[r], label, regName[r], regName[r], label);
}
void optasmm1111(Function *fnc, FILE *fp) {
    int               saveSize = 0;  // 保护寄存器的大小
    functionCallInfo *curCall = NULL;

    fnc->loc1 += 8;

    // 指令很有可能溢出了多个参数,因此统计溢出个数,3个溢出 add t0,t0,t1
    int             memNUM = 0;
    char           *str;
    int             asmop;
    size_t          i, paramNum = 0;
    int             overparammem = 0;
    BasicBlock     *bb = NULL;
    bool            isRestore = false;
    bool            swap = false;
    Value           tmpv;
    int             argumentCount = 0;

    asminstruction *asmptr;

    vector_each(&fnc->BBs, i, bb) {
        {
            char buff[1024];
            sprintf(buff, ".%s_bb_%d", fnc->name, bb->bbName);
            asmptr = createAsm9(ASM_label, strdup(buff));
            DL_APPEND(asmlist, asmptr);
        }
        // fprintf(fp, ".%s_bb_%d:\n", fnc->name, bb->bbName);
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            ssaSymbol *s0, *s1, *s2, *s4;
            if (inst->args[0].ty & IS_SSA) {
                s1 = getSSAlVariable(fnc, inst->args[0].ssaind);
            }
            if (inst->args[1].ty & IS_SSA) {
                s2 = getSSAlVariable(fnc, inst->args[1].ssaind);
            }
            if (inst->ret.ty & IS_SSA) {
                s0 = getSSAlVariable(fnc, inst->ret.ssaind);
            }
            int feq = 0;
            switch (inst->op) {
                case IR_OP_ARGUMENT: {
                    if (argumentCount >= 8) {
                        // 根本不能调用这个函数,严重bug
                        if (ValueIsInt(&inst->ret)) {
                            if (isSpill(&inst->ret)) {
                                asmptr = createAsm7(ASM_ld, REG_t1, REG_t0,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                asmprocessMemoryStore1(fnc, fp, REG_t1, s0);
                            } else {
                                asmptr = createAsm7(ASM_ld, inst->ret.reg,
                                                    REG_t0, overparammem);
                                DL_APPEND(asmlist, asmptr);
                            }
                            // asmptr = createAsm4(ASM_li, REG_t1,
                            // overparammem); DL_APPEND(asmlist, asmptr); asmptr
                            // =
                            //     createAsm2(ASM_add, REG_t1, REG_t1, REG_t0);
                            // DL_APPEND(asmlist, asmptr);

                            // asmprocessMemoryStore1(fnc, fp, REG_CON, s0);
                        } else if (ValueIsFloat(&inst->ret)) {
                            assert(0);
                            if (isSpill(&inst->ret)) {
                                asmptr = createAsm7(ASM_flw, REG_fpt8, REG_t0,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                asmprocessFPMemoryStore1(fnc, fp, REG_fpt8, s0);
                            } else {
                                asmptr = createAsm7(ASM_flw, inst->ret.reg,
                                                    REG_t0, overparammem);
                                DL_APPEND(asmlist, asmptr);
                            }
                        } else if (inst->ret.ty & IS_SSA) {
                            // 数组指针
                            // fprintf(fp, "ld      %s,%d(s0)\n",
                            // regName[REG_CON],
                            //         overparammem);
                            if (isSpill(&inst->ret)) {
                                asmptr = createAsm7(ASM_ld, REG_t1, REG_t0,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                asmprocessMemoryStore1(fnc, fp, REG_t1, s0);
                            } else {
                                asmptr = createAsm7(ASM_ld, inst->ret.reg,
                                                    REG_t0, overparammem);
                                DL_APPEND(asmlist, asmptr);
                            }
                        } else {
                            assert(0);
                        }
                        overparammem += 8;
                    }
                    argumentCount++;
                } break;
                case IR_OP_NOT: {
                    int reg, r1;
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsInt(&inst->ret));

                    if (ValueIsImm(&inst->args[0])) {
                        reg = REG_CON;
                        asmloadImm(fnc, fp, REG_CON, &inst->args[0]);
                    } else if (isSpill(&inst->args[0])) {
                        reg = REG_SPILL;
                        asmprocessMemoryLoad1(fnc, fp, REG_SPILL, s1);
                    } else {
                        assert((inst->args[0].ty & IS_REG));
                        assert((inst->args[0].ty & IS_SSA));
                        reg = inst->args[0].reg;
                    }

                    if (isSpill(&inst->ret)) {
                        r1 = REG_SPILL;
                        asmptr = createAsm5(ASM_seqz, r1, reg);
                        DL_APPEND(asmlist, asmptr);
                        asmprocessMemoryStore1(fnc, fp, r1, s0);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));
                        r1 = inst->ret.reg;
                        asmptr = createAsm5(ASM_seqz, r1, reg);
                        DL_APPEND(asmlist, asmptr);
                    }

                } break;
                case IR_OP_NOTF:
                    assert(0);
                    break;
                    // negw    a0, a0
                case IR_OP_NEG: {
                    int reg, r1;
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsInt(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        reg = REG_CON;
                        asmloadImm(fnc, fp, reg, &inst->args[0]);
                    } else if (isSpill(&inst->args[0])) {
                        reg = REG_SPILL;
                        asmprocessMemoryLoad1(fnc, fp, REG_SPILL, s1);
                    } else {
                        assert((inst->args[0].ty & IS_REG));
                        assert((inst->args[0].ty & IS_SSA));
                        reg = inst->args[0].reg;
                    }

                    if (isSpill(&inst->ret)) {
                        r1 = REG_SPILL;
                        asmptr = createAsm5(ASM_negw, r1, reg);
                        DL_APPEND(asmlist, asmptr);
                        asmprocessMemoryStore1(fnc, fp, REG_SPILL, s0);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));
                        r1 = inst->ret.reg;
                        asmptr = createAsm5(ASM_negw, r1, reg);
                        DL_APPEND(asmlist, asmptr);
                    }
                } break;
                //  fneg.s  fa0, fa0
                case IR_OP_NEGF: {
                    assert(ValueIsFloat(&inst->args[0]));
                    assert(ValueIsFloat(&inst->ret));
                    int reg, r1;
                    if (ValueIsImm(&inst->args[0])) {
                        reg = FPREG_SPILL;
                        asmloadFPImm(fnc, fp, reg, &inst->args[0]);
                    } else if (isSpill(&inst->args[0])) {
                        reg = FPREG_SPILL;
                        asmprocessFPMemoryLoad1(fnc, fp, reg, s1);
                    } else {
                        assert((inst->args[0].ty & IS_REG));
                        assert((inst->args[0].ty & IS_SSA));
                        reg = inst->args[0].reg;
                    }

                    if (isSpill(&inst->ret)) {
                        r1 = FPREG_SPILL;
                        asmptr = createAsm11(ASM_fneg, r1, reg);
                        DL_APPEND(asmlist, asmptr);
                        asmprocessFPMemoryStore1(fnc, fp, FPREG_SPILL, s0);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));
                        r1 = inst->ret.reg;
                        asmptr = createAsm11(ASM_fneg, r1, reg);
                        DL_APPEND(asmlist, asmptr);
                    }
                    // fprintf(fp, "fneg.s    fa0, fa0\n");
                    // processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                }

                break;
                case IR_OP_MOV: {
                    int r1, r2;
                    if (ValueIsInt(&inst->ret)) {
                        if (ValueIsImm(&inst->args[0])) {
                            r2 = REG_CON;
                            asmloadImm(fnc, fp, r2, &inst->args[0]);
                        } else if (isSpill(&inst->args[0])) {
                            r2 = REG_SPILL;
                            asmprocessMemoryLoad1(fnc, fp, REG_SPILL, s1);
                        } else {
                            assert((inst->args[0].ty & IS_REG));
                            assert((inst->args[0].ty & IS_SSA));
                            r2 = inst->args[0].reg;
                        }

                        if (isSpill(&inst->ret)) {
                            asmprocessMemoryStore1(fnc, fp, r2, s0);
                        } else {
                            assert((inst->ret.ty & IS_REG));
                            assert((inst->ret.ty & IS_SSA));
                            r1 = inst->ret.reg;
                            asmptr = createAsm5(ASM_mv, r1, r2);
                            DL_APPEND(asmlist, asmptr);
                        }
                    } else if (ValueIsFloat(&inst->ret)) {
                        if (ValueIsImm(&inst->args[0])) {
                            r2 = FPREG_SPILL2;
                            asmloadFPImm(fnc, fp, r2, &inst->args[0]);
                        } else if (isSpill(&inst->args[0])) {
                            r2 = FPREG_SPILL2;
                            asmprocessFPMemoryLoad1(fnc, fp, FPREG_SPILL2, s1);
                        } else {
                            assert((inst->args[0].ty & IS_REG));
                            assert((inst->args[0].ty & IS_SSA));
                            r2 = inst->args[0].reg;
                        }

                        if (isSpill(&inst->ret)) {
                            asmprocessFPMemoryStore1(fnc, fp, r2, s0);
                        } else {
                            assert((inst->ret.ty & IS_REG));
                            assert((inst->ret.ty & IS_SSA));
                            r1 = inst->ret.reg;
                            asmptr = createAsm11(ASM_fmvs, r1, r2);
                            DL_APPEND(asmlist, asmptr);
                        }
                    } else {
                        assert(0);
                    }

                } break;
                    // src操作数：ssa,立即数;dest操作数：ssa,立即数
                case IR_OP_NE:
                    asmop = ASM_beq;
                    str = "beq";
                    goto _cond;
                case IR_OP_EQ:
                    asmop = ASM_bne;
                    str = "bne";
                    goto _cond;
                case IR_OP_GT:
                    asmop = ASM_blt;
                    str = "blt";
                    goto _cond;
                case IR_OP_GE:
                    asmop = ASM_ble;
                    str = "ble";
                    goto _cond;
                case IR_OP_LT:
                    asmop = ASM_bgt;
                    str = "bgt";
                    goto _cond;
                case IR_OP_LE:
                    asmop = ASM_bge;
                    str = "bge";
                _cond : {
                    int reg = REG_CON;
                    int l = 0, r = 1;
                    int r1, r2;
                    if (ValueIsImm(&inst->args[0])) {
                        r1 = REG_SPILL;
                        asmloadImm(fnc, fp, r1, &inst->args[0]);
                    } else if (isSpill(&inst->args[0])) {
                        r1 = REG_SPILL;
                        asmprocessMemoryLoad1(fnc, fp, REG_SPILL, s1);
                    } else {
                        assert((inst->args[0].ty & IS_REG));
                        assert((inst->args[0].ty & IS_SSA));
                        r1 = inst->args[0].reg;
                        asmptr = createAsm5(ASM_sextw, REG_SPILL, r1);
                        r1 = REG_SPILL;
                        DL_APPEND(asmlist, asmptr);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        r2 = REG_SPILL2;
                        asmloadImm(fnc, fp, r2, &inst->args[1]);
                    } else if (isSpill(&inst->args[1])) {
                        r2 = REG_SPILL2;
                        asmprocessMemoryLoad1(fnc, fp, REG_SPILL2, s2);
                    } else {
                        assert((inst->args[1].ty & IS_REG));
                        assert((inst->args[1].ty & IS_SSA));
                        r2 = inst->args[1].reg;
                        asmptr = createAsm5(ASM_sextw, REG_SPILL2, r2);
                        r2 = REG_SPILL2;
                        DL_APPEND(asmlist, asmptr);
                    }
                    {
                        char buff[1024];
                        sprintf(buff, ".%s_bb_%d", fnc->name,
                                bb->branchblock->bbName);
                        asmptr = createAsm6(asmop, r1, r2, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                        sprintf(buff, ".%s_bb_%d", fnc->name,
                                bb->nextblock->bbName);
                        asmptr = createAsm1(ASM_J, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    }
                } break;
                case IR_OP_I2F: {
                    int reg;
                    // fmv.s.x rd, rs1 f[rd] = x[rs1][31:0]
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsFloat(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        reg = REG_SPILL;
                        asmloadImm(fnc, fp, reg, &inst->args[0]);
                    } else if (isSpill(&inst->args[0])) {
                        reg = REG_SPILL;
                        asmprocessMemoryLoad1(fnc, fp, REG_SPILL, s1);
                    } else {
                        assert((inst->args[0].ty & IS_REG));
                        assert((inst->args[0].ty & IS_SSA));
                        reg = inst->args[0].reg;
                    }
                    if (isSpill(&inst->ret)) {
                        asmptr = createAsm11(ASM_fcvtsw, FPREG_SPILL, reg);
                        DL_APPEND(asmlist, asmptr);
                        asmprocessFPMemoryStore1(fnc, fp, FPREG_SPILL, s0);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));

                        asmptr = createAsm11(ASM_fcvtsw, inst->ret.reg, reg);
                        DL_APPEND(asmlist, asmptr);
                    }
                } break;
                case IR_OP_F2I: {
                    // fmv.x.s rd, rs1 x[rd] = f[rs1][31:0]
                    assert(ValueIsFloat(&inst->args[0]));
                    assert(ValueIsInt(&inst->ret));
                    int r1, r2;

                    if (ValueIsImm(&inst->args[0])) {
                        r2 = FPREG_SPILL2;
                        asmloadFPImm(fnc, fp, r2, &inst->args[0]);
                    } else if (isSpill(&inst->args[0])) {
                        r2 = FPREG_SPILL2;
                        asmprocessFPMemoryLoad1(fnc, fp, FPREG_SPILL2, s1);
                    } else {
                        assert((inst->args[0].ty & IS_REG));
                        assert((inst->args[0].ty & IS_SSA));
                        r2 = inst->args[0].reg;
                    }

                    // fprintf(fp, "fcvt.w.s     a0,fa0\n");
                    asmptr = createAsm11(ASM_fcvtws, REG_CON, r2);
                    DL_APPEND(asmlist, asmptr);
                    if (isSpill(&inst->ret)) {
                        asmprocessMemoryStore1(fnc, fp, REG_CON, s0);
                    } else {
                        asmptr = createAsm5(ASM_mv, inst->ret.reg, REG_CON);
                        DL_APPEND(asmlist, asmptr);
                    }
                }

                break;
                case IR_OP_NEF:
                    str = "feq.s";
                    asmop = ASM_fne;
                    goto _fpcond;
                case IR_OP_EQF:
                    str = "fne.s";
                    asmop = ASM_feq;
                    feq = 1;
                    goto _fpcond;
                case IR_OP_GTF:
                    str = "flt.s";
                    asmop = ASM_fgt;
                    goto _fpcond;
                case IR_OP_GEF:
                    str = "fle.s";
                    asmop = ASM_fge;
                    goto _fpcond;
                case IR_OP_LTF:
                    str = "fgt.s";
                    asmop = ASM_flt;
                    goto _fpcond;
                case IR_OP_LEF:
                    str = "fge.s";
                    asmop = ASM_fle;
                _fpcond : {
                    assert(ValueIsFloat(&inst->args[0]));
                    assert(ValueIsFloat(&inst->args[1]));
                    int l = 0, r = 1;
                    int r1, r2;
                    if (swap) {
                        r = 0;
                        l = 1;
                    }
                    if (ValueIsImm(&inst->args[0])) {
                        r1 = FPREG_SPILL;
                        asmloadFPImm(fnc, fp, FPREG_SPILL, &inst->args[0]);
                    } else if (isSpill(&inst->args[0])) {
                        r1 = FPREG_SPILL;
                        asmprocessFPMemoryLoad1(fnc, fp, FPREG_SPILL, s1);
                    } else {
                        assert((inst->args[0].ty & IS_REG));
                        assert((inst->args[0].ty & IS_SSA));
                        r1 = inst->args[0].reg;
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        r2 = FPREG_SPILL2;
                        asmloadFPImm(fnc, fp, FPREG_SPILL2, &inst->args[1]);
                    } else if (isSpill(&inst->args[1])) {
                        r2 = FPREG_SPILL2;
                        asmprocessFPMemoryLoad1(fnc, fp, FPREG_SPILL2, s2);
                    } else {
                        assert((inst->args[1].ty & IS_REG));
                        assert((inst->args[1].ty & IS_SSA));
                        r2 = inst->args[1].reg;
                    }
                    // feq.s   a5,fa5,fa4
                    // beq     a5,zero,.L4
                    // fprintf(fp, "%s   a0, %s,%s\n", str, regName[r1],
                    //         regName[r2]);
                    asmptr = createAsm12(asmop, REG_CON, r1, r2);
                    DL_APPEND(asmlist, asmptr);

                    // fprintf(fp, "beq   a0, zero,.%s_bb_%d\n", fnc->name,
                    //         bb->branchblock->bbName);
                    if (feq) {
                        char buff[1024];
                        sprintf(buff, ".%s_bb_%d", fnc->name,
                                bb->nextblock->bbName);
                        asmptr = createAsm6(ASM_beq, REG_CON, REG_zero,
                                            strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                        // fprintf(fp, "J .%s_bb_%d\n", fnc->name,
                        //         bb->nextblock->bbName);
                        sprintf(buff, ".%s_bb_%d", fnc->name,
                                bb->branchblock->bbName);
                        asmptr = createAsm1(ASM_J, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    } else {
                        char buff[1024];
                        sprintf(buff, ".%s_bb_%d", fnc->name,
                                bb->branchblock->bbName);
                        asmptr = createAsm6(ASM_beq, REG_CON, REG_zero,
                                            strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                        // fprintf(fp, "J .%s_bb_%d\n", fnc->name,
                        //         bb->nextblock->bbName);
                        sprintf(buff, ".%s_bb_%d", fnc->name,
                                bb->nextblock->bbName);
                        asmptr = createAsm1(ASM_J, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    }

                } break;

                case IR_OP_ADDF:
                    asmop = ASM_fadd;
                    str = "fadd.s";
                    goto _fpbinary1;
                case IR_OP_SUBF:
                    asmop = ASM_fsub;
                    str = "fsub.s";
                    goto _fpbinary1;
                case IR_OP_MULF:
                    asmop = ASM_fmul;
                    str = "fmul.s";
                    goto _fpbinary1;
                case IR_OP_DIVF:
                    asmop = ASM_fdiv;
                    str = "fdiv.s";
                    goto _fpbinary1;
                _fpbinary1 : {
                    int r1, r2;
                    assert(ValueIsFloat(&inst->ret));
                    assert(ValueIsFloat(&inst->args[0]));
                    assert(ValueIsFloat(&inst->args[1]));
                    if (ValueIsImm(&inst->args[0])) {
                        r1 = FPREG_SPILL;
                        asmloadFPImm(fnc, fp, r1, &inst->args[0]);
                    } else if (isSpill(&inst->args[0])) {
                        r1 = FPREG_SPILL;
                        asmprocessFPMemoryLoad1(fnc, fp, r1, s1);
                    } else {
                        assert((inst->args[0].ty & IS_REG));
                        assert((inst->args[0].ty & IS_SSA));
                        r1 = inst->args[0].reg;
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        r2 = FPREG_SPILL2;
                        asmloadFPImm(fnc, fp, r2, &inst->args[1]);
                    } else if (isSpill(&inst->args[1])) {
                        r2 = FPREG_SPILL2;
                        asmprocessFPMemoryLoad1(fnc, fp, r2, s2);
                    } else {
                        assert((inst->args[1].ty & IS_REG));
                        assert((inst->args[1].ty & IS_SSA));
                        r2 = inst->args[1].reg;
                    }
                    int r0;
                    if (isSpill(&inst->ret)) {
                        asmptr = createAsm12(asmop, FPREG_SPILL, r1, r2);
                        DL_APPEND(asmlist, asmptr);
                        asmprocessFPMemoryStore1(fnc, fp, FPREG_SPILL, s0);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));
                        r0 = inst->ret.reg;
                        assert(inst->ret.reg != -1);
                        asmptr = createAsm12(asmop, r0, r1, r2);
                        DL_APPEND(asmlist, asmptr);
                    }
                } break;
                case IR_OP_ADD:
                    asmop = ASM_add;
                    str = "add";
                    goto _binary1;
                case IR_OP_SUB:
                    asmop = ASM_sub;
                    str = "sub";
                    goto _binary1;
                case IR_OP_MUL:
                    asmop = ASM_mul;
                    str = "mul";
                    goto _binary1;
                case IR_OP_DIV:
                    asmop = ASM_div;
                    str = "div";
                    goto _binary1;
                case IR_OP_MOD:
                    asmop = ASM_rem;
                    str = "rem";
                _binary1 : {
                    int r1, r2, r0;
                    assert(ValueIsInt(&inst->ret));
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsInt(&inst->args[1]));
                    if (ValueIsImm(&inst->args[0])) {
                        r1 = REG_SPILL;
                        asmloadImm(fnc, fp, r1, &inst->args[0]);
                    } else if (isSpill(&inst->args[0])) {
                        r1 = REG_SPILL;
                        asmprocessMemoryLoad1(fnc, fp, REG_SPILL, s1);
                    } else {
                        assert((inst->args[0].ty & IS_REG));
                        assert((inst->args[0].ty & IS_SSA));
                        r1 = inst->args[0].reg;
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        r2 = REG_SPILL2;
                        asmloadImm(fnc, fp, r2, &inst->args[1]);
                    } else if (isSpill(&inst->args[1])) {
                        r2 = REG_SPILL2;
                        asmprocessMemoryLoad1(fnc, fp, REG_SPILL2, s2);
                    } else {
                        assert((inst->args[1].ty & IS_REG));
                        assert((inst->args[1].ty & IS_SSA));
                        r2 = inst->args[1].reg;
                    }

                    if (isSpill(&inst->ret)) {
                        r0 = REG_SPILL;
                        asmptr = createAsm2(asmop, r0, r1, r2);
                        DL_APPEND(asmlist, asmptr);
                        asmprocessMemoryStore1(fnc, fp, r0, s0);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));
                        r0 = inst->ret.reg;
                        assert(inst->ret.reg != -1);
                        asmptr = createAsm2(asmop, r0, r1, r2);
                        DL_APPEND(asmlist, asmptr);
                    }
                } break;
                case IR_OP_RET: {
                    // fprintf(fp, "J .%s_ret\n", fnc->name);
                    char buff[1024];
                    sprintf(buff, ".%s_ret", fnc->name);
                    asmptr = createAsm1(ASM_J, strdup(buff));
                    DL_APPEND(asmlist, asmptr);
                } break;
                case IR_OP_RETI: {
                    int reg;
                    // ret
                    // 指令会在寄存器分配中分配a0,如果被溢出了需要重新加载
                    if (ValueIsImm(&inst->ret)) {
                        asmloadImm(fnc, fp, REG_a0, &inst->ret);
                    } else if (isSpill(&inst->ret)) {
                        asmprocessMemoryLoad1(fnc, fp, REG_a0, s2);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));
                        asmptr = createAsm5(ASM_mv, REG_a0, inst->ret.reg);
                        DL_APPEND(asmlist, asmptr);
                    }
                    {
                        char buff[1024];
                        sprintf(buff, ".%s_ret", fnc->name);
                        asmptr = createAsm1(ASM_J, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    }
                } break;
                case IR_OP_RETF: {
                    // BUG 没有生成函数序言结语
                    assert((inst->ret.ty & TY_FLOAT));
                    if (ValueIsImm(&inst->ret)) {
                        asmloadFPImm(fnc, fp, REG_fa0, &inst->ret);
                    } else if (isSpill(&inst->ret)) {
                        asmprocessFPMemoryLoad1(fnc, fp, REG_fa0, s0);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));
                        asmptr = createAsm11(ASM_fmvs, REG_fa0, inst->ret.reg);
                        DL_APPEND(asmlist, asmptr);
                    }
                    // fprintf(fp, "J .%s_ret\n", fnc->name);
                    {
                        char buff[1024];
                        sprintf(buff, ".%s_ret", fnc->name);
                        asmptr = createAsm1(ASM_J, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    }
                }

                break;
                case IR_OP_SOTRE: {
                    if (inst->args[0].ty & TY_INT) {
                    } else if (inst->args[0].ty & TY_FLOAT) {
                    } else {
                        assert(0);
                    }
                    int r1, r2;
                    if (inst->args[0].ty & TY_INT && inst->ret.ty & TY_INT) {
                        if (ValueIsImm(&inst->args[0])) {
                            // loadImm(fnc, fp, REG_a0, &inst->args[0]);
                            r2 = REG_SPILL;
                            asmloadImm(fnc, fp, r2, &inst->args[0]);
                        } else if (isSpill(&inst->args[0])) {
                            assert(inst->args[0].ty & IS_SSA);
                            r2 = REG_SPILL;
                            asmprocessMemoryLoad1(fnc, fp, r2, s1);
                        } else {
                            assert((inst->args[0].ty & IS_REG));
                            assert((inst->args[0].ty & IS_SSA));
                            r2 = inst->args[0].reg;
                        }

                        // 全局标量
                        if (inst->ret.ty & IS_GLOBAL) {
                            // loadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                            r1 = REG_SPILL2;
                            asmloadSymbol(fnc, fp, r1, &inst->ret);
                            // fprintf(fp, "sd   %s, 0(%s)\n", regName[REG_a0],
                            //         regName[REG_SPILL]);
                            asmptr = createAsm7(ASM_sw, r2, r1, 0);
                            DL_APPEND(asmlist, asmptr);
                            break;
                        }
                        if (!(inst->ret.ty & IS_ARRAY) &&
                            (inst->ret.ty & IS_POINTER) &&
                            (inst->ret.ty & IS_GLOBAL)) {
                            assert(0);
                        } else if (inst->ret.ty & IS_SSA &&
                                   !(inst->ret.ty & IS_POINTER)) {
                            assert(0);
                            // ssa是不可能的
                            //  processMemoryStore1(fnc, fp, REG_a0, s0);
                            asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                        } else {
                            // 地址
                            // processMemoryLoad1(fnc, fp, REG_a1, s0);
                            if (isSpill(&inst->ret)) {
                                r1 = REG_SPILL2;
                                asmprocessMemoryLoad1(fnc, fp, r1, s0);
                            } else {
                                assert((inst->ret.ty & IS_REG));
                                assert((inst->ret.ty & IS_SSA));
                                r1 = inst->ret.reg;
                            }
                            asmptr = createAsm7(ASM_sw, r2, r1, 0);
                            DL_APPEND(asmlist, asmptr);
                        }
                    } else if (inst->args[0].ty & TY_FLOAT &&
                               inst->ret.ty & TY_FLOAT) {
                        if (ValueIsImm(&inst->args[0])) {
                            // loadImm(fnc, fp, REG_a0, &inst->args[0]);
                            r2 = FPREG_SPILL;
                            asmloadFPImm(fnc, fp, r2, &inst->args[0]);
                        } else if (isSpill(&inst->args[0])) {
                            assert(inst->args[0].ty & IS_SSA);
                            r2 = FPREG_SPILL;
                            asmprocessFPMemoryLoad1(fnc, fp, r2, s1);
                        } else {
                            assert((inst->args[0].ty & IS_REG));
                            assert((inst->args[0].ty & IS_SSA));
                            r2 = inst->args[0].reg;
                        }
                        // 全局标量
                        if (inst->ret.ty & IS_GLOBAL) {
                            asmloadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                            asmptr = createAsm7(ASM_fsw, r2, REG_SPILL, 0);
                            DL_APPEND(asmlist, asmptr);
                            break;
                        }
                        if (!(inst->ret.ty & IS_ARRAY) &&
                            (inst->ret.ty & IS_POINTER) &&
                            (inst->ret.ty & IS_GLOBAL)) {
                            assert(0);
                        } else if (inst->ret.ty & IS_VAR &&
                                   !(inst->ret.ty & IS_POINTER)) {
                            // processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                            assert(0);
                            asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
                        } else {
                            // 地址
                            // processMemoryLoad1(fnc, fp, REG_a1, s0);
                            if (isSpill(&inst->ret)) {
                                r1 = REG_SPILL;
                                asmprocessMemoryLoad1(fnc, fp, r1, s0);
                            } else {
                                assert((inst->ret.ty & IS_REG));
                                assert((inst->ret.ty & IS_SSA));
                                r1 = inst->ret.reg;
                            }
                            // fprintf(fp, "fsw   %s, 0(%s)\n",
                            // regName[REG_fa0],
                            //         regName[REG_a1]);
                            asmptr = createAsm7(ASM_fsw, r2, r1, 0);
                            DL_APPEND(asmlist, asmptr);
                        }
                    } else {
                        assert(0);
                    }

                }

                // 考虑ret溢出

                break;
                case IR_OP_LOAD:
                    // 局部地址和全局地址的加载貌似是不一样的
                    {
                        int r0, r1;
                        if (inst->args[0].ty & TY_INT) {
                        } else if (inst->args[0].ty & TY_FLOAT) {
                        } else {
                            assert(0);
                        }

                        if (inst->args[0].ty & TY_INT &&
                            inst->ret.ty & TY_INT) {
                            // 加载了一个32位的值,int类型
                            // 全局标量,int a;
                            if (!(inst->args[0].ty & IS_ARRAY) &&
                                (inst->args[0].ty & IS_POINTER) &&
                                (inst->args[0].ty & IS_GLOBAL)) {
                                r1 = REG_SPILL;
                                // loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                asmloadSymbol(fnc, fp, r1, &inst->args[0]);
                                // fprintf(fp, "ld   %s, 0(%s)\n",
                                // regName[REG_a0],
                                //         regName[REG_a0]);
                                asmptr = createAsm7(ASM_lw, r1, r1, 0);
                                DL_APPEND(asmlist, asmptr);
                            } else if (ValueIsImm(&inst->args[0])) {
                                loadImm(fnc, fp, REG_a0, &inst->args[0]);
                                assert(0);
                            } else if (inst->args[0].ty & IS_GLOBAL) {
                                // 加载全局数组地址,作为传递参数
                                // loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                r1 = REG_SPILL;
                                asmloadSymbol1(fnc, fp, r1, &inst->args[0]);
                                s0->ty |= IS_POINTER;
                            } else if (inst->args[0].ty & IS_SSA) {
                                // processMemoryLoad1(fnc, fp, REG_a0, s1);
                                // 地址
                                if (isSpill(&inst->args[0])) {
                                    r1 = REG_SPILL;
                                    asmprocessMemoryLoad1(fnc, fp, r1, s1);
                                    asmptr = createAsm7(ASM_lw, r1, r1, 0);
                                    DL_APPEND(asmlist, asmptr);
                                } else {
                                    assert((inst->args[0].ty & IS_REG));
                                    assert((inst->args[0].ty & IS_SSA));
                                    r1 = inst->args[0].reg;
                                    asmptr =
                                        createAsm7(ASM_lw, REG_SPILL, r1, 0);
                                    DL_APPEND(asmlist, asmptr);
                                    r1 = REG_SPILL;
                                }
                            } else {
                                assert(0);
                            }

                            if (isSpill(&inst->ret)) {
                                asmprocessMemoryStore1(fnc, fp, r1, s0);
                            } else {
                                assert((inst->ret.ty & IS_REG));
                                assert((inst->ret.ty & IS_SSA));
                                r0 = inst->ret.reg;
                                asmptr = createAsm5(ASM_mv, r0, r1);
                                DL_APPEND(asmlist, asmptr);
                            }
                        } else if (inst->args[0].ty & TY_FLOAT &&
                                   inst->ret.ty & TY_FLOAT) {
                            int r0, r1;
                            // 全局标量,int a;
                            if (!(inst->args[0].ty & IS_ARRAY) &&
                                (inst->args[0].ty & IS_POINTER) &&
                                (inst->args[0].ty & IS_GLOBAL)) {
                                r1 = FPREG_SPILL;
                                asmloadSymbol(fnc, fp, REG_CON, &inst->args[0]);
                                asmptr = createAsm7(ASM_flw, FPREG_SPILL,
                                                    REG_CON, 0);
                                DL_APPEND(asmlist, asmptr);
                            } else if (ValueIsImm(&inst->args[0])) {
                                loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                                assert(0);
                            } else if (inst->args[0].ty & IS_GLOBAL) {
                                assert(0);
                                // 加载数组,调试打印
                                asmloadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                            } else if (inst->args[0].ty & IS_SSA) {
                                if (isSpill(&inst->args[0])) {
                                    r1 = FPREG_SPILL2;
                                    asmprocessMemoryLoad1(fnc, fp, REG_SPILL,
                                                          s1);
                                    asmptr =
                                        createAsm7(ASM_flw, r1, REG_SPILL, 0);
                                    DL_APPEND(asmlist, asmptr);
                                } else {
                                    assert((inst->args[0].ty & IS_REG));
                                    assert((inst->args[0].ty & IS_SSA));
                                    r1 = inst->args[0].reg;
                                    asmptr = createAsm7(ASM_flw, FPREG_SPILL2,
                                                        r1, 0);
                                    DL_APPEND(asmlist, asmptr);
                                    r1 = FPREG_SPILL2;
                                }

                                if (isSpill(&inst->ret)) {
                                    asmprocessFPMemoryStore1(fnc, fp, r1, s0);
                                } else {
                                    assert((inst->ret.ty & IS_REG));
                                    assert((inst->ret.ty & IS_SSA));
                                    r0 = inst->ret.reg;
                                    asmptr = createAsm11(ASM_fmvs, r0, r1);
                                    DL_APPEND(asmlist, asmptr);
                                }

                                assert(!(inst->args[0].ty & IS_GLOBAL));

                            } else {
                                assert(0);
                            }

                        } else {
                            assert(0);
                        }
                    }
                    break;
                case IR_OP_IF:
                case IR_OP_PHI:
                    break;
                case IR_OP_ADDPTR: {
                    if (inst->args[0].ty & TY_INT) {
                    } else if (inst->args[0].ty & TY_FLOAT) {
                    } else {
                        assert(0);
                    }
                    int arrayReg;
                    // 数组指针int fnc(int a[]);
                    // ssa标识数组指针
                    if (inst->args[0].ty & IS_SSA) {
                        if (isSpill(&inst->args[0])) {
                            arrayReg = REG_SPILL;
                            asmprocessMemoryLoad1(fnc, fp, REG_SPILL, s1);
                        } else {
                            assert((inst->args[0].ty & IS_REG));
                            assert((inst->args[0].ty & IS_SSA));
                            arrayReg = inst->args[0].reg;
                        }
                        if (inst->args[0].ty & IS_GLOBAL) assert(0);
                    } else {
                        Symbol *s;
                        if (inst->args[0].ty & IS_GLOBAL) {
                            s = getGlobalSymbol(inst->args[0].symbol);
                            arrayReg = REG_SPILL;
                            asmloadSymbol(fnc, fp, arrayReg, &inst->args[0]);
                        } else {
                            s = getLocalVariable(fnc, inst->args[0].symbol);
                            if (!s->isalloc) {
                                s->isalloc = 1;
                                if (s->iszero) {
                                    vector_push_back(&zeorArrays, s);
                                } else {
                                    s->loc = allocStack2(
                                        fnc, getArraySize2(s->arrayTy) * 2);
                                    if (s->loc > 535800) {
                                        assert(0);
                                        allocStack2(
                                            fnc, getArraySize2(s->arrayTy) * 2);
                                        printf("\n%d",
                                               getArraySize2(s->arrayTy));
                                    }
                                }
                            }
                            if (s->iszero) {
                                // fprintf(fp, "la    %s, .zeroarray_%x\n",
                                //         regName[REG_SPILL], s);
                                char buff[1024];
                                sprintf(buff, ".zeroarray_%x", s);
                                arrayReg = REG_SPILL;

                                asmptr =
                                    createAsm8(ASM_la, arrayReg, strdup(buff));
                                DL_APPEND(asmlist, asmptr);
                            } else {
                                // fprintf(fp, "li    t2, %d\n", s->loc);
                                asmptr = createAsm4(ASM_li, REG_t2, s->loc);
                                DL_APPEND(asmlist, asmptr);
                                // fprintf(fp, "add    %s, t3,t2\n",
                                //         regName[REG_SPILL]);
                                arrayReg = REG_SPILL;
                                asmptr = createAsm2(ASM_add, arrayReg, REG_sp,
                                                    REG_t2);
                                DL_APPEND(asmlist, asmptr);
                            }
                        }
                    }
                    int r2;
                    if (ValueIsImm(&inst->args[1])) {
                        r2 = REG_SPILL2;
                        asmloadImm(fnc, fp, r2, &inst->args[1]);
                    } else if (isSpill(&inst->args[1])) {
                        r2 = REG_SPILL2;
                        asmprocessMemoryLoad1(fnc, fp, REG_SPILL2, s2);
                    } else {
                        assert((inst->args[1].ty & IS_REG));
                        assert((inst->args[1].ty & IS_SSA));
                        r2 = inst->args[1].reg;
                    }

                    if (isSpill(&inst->ret)) {
                        asmptr = createAsm2(ASM_add, REG_SPILL, arrayReg, r2);
                        DL_APPEND(asmlist, asmptr);
                        asmprocessMemoryStore1(fnc, fp, REG_SPILL, s0);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));
                        asmptr =
                            createAsm2(ASM_add, inst->ret.reg, arrayReg, r2);
                        DL_APPEND(asmlist, asmptr);
                    }

                }; break;
                case IR_OP_GETITEMPTR: {
                    ArrayType *arrayTy;
                    if (inst->args[0].ty & TY_INT) {
                    } else if (inst->args[0].ty & TY_FLOAT) {
                    } else {
                        assert(0);
                    }
                    int arrayReg;
                    // 数组指针int fnc(int a[]);
                    // ssa标识数组指针
                    if (inst->args[0].ty & IS_SSA) {
                        if (isSpill(&inst->args[0])) {
                            arrayReg = REG_SPILL;
                            asmprocessMemoryLoad1(fnc, fp, REG_SPILL, s1);
                        } else {
                            assert((inst->args[0].ty & IS_REG));
                            assert((inst->args[0].ty & IS_SSA));
                            arrayReg = inst->args[0].reg;
                        }
                        if (inst->args[0].ty & IS_GLOBAL) assert(0);
                        assert(s1->arrayTy);
                        arrayTy = s1->arrayTy;
                    } else {
                        Symbol *s;
                        if (inst->args[0].ty & IS_GLOBAL) {
                            s = getGlobalSymbol(inst->args[0].symbol);
                            arrayReg = REG_SPILL;
                            asmloadSymbol(fnc, fp, arrayReg, &inst->args[0]);
                        } else {
                            s = getLocalVariable(fnc, inst->args[0].symbol);
                            if (!s->isalloc) {
                                s->isalloc = 1;
                                if (s->iszero) {
                                    vector_push_back(&zeorArrays, s);
                                } else {
                                    s->loc = allocStack2(
                                        fnc, getArraySize2(s->arrayTy) * 2);
                                    if (s->loc > 535800) {
                                        assert(0);
                                        allocStack2(
                                            fnc, getArraySize2(s->arrayTy) * 2);
                                        printf("\n%d",
                                               getArraySize2(s->arrayTy));
                                    }
                                }
                            }
                            if (s->iszero) {
                                // fprintf(fp, "la    %s, .zeroarray_%x\n",
                                //         regName[REG_SPILL], s);
                                char buff[1024];
                                sprintf(buff, ".zeroarray_%x", s);
                                arrayReg = REG_SPILL;

                                asmptr =
                                    createAsm8(ASM_la, arrayReg, strdup(buff));
                                DL_APPEND(asmlist, asmptr);
                            } else {
                                // fprintf(fp, "li    t2, %d\n", s->loc);
                                asmptr = createAsm4(ASM_li, REG_t2, s->loc);
                                DL_APPEND(asmlist, asmptr);
                                // fprintf(fp, "add    %s, t3,t2\n",
                                //         regName[REG_SPILL]);
                                arrayReg = REG_SPILL;
                                asmptr = createAsm2(ASM_add, arrayReg, REG_sp,
                                                    REG_t2);
                                DL_APPEND(asmlist, asmptr);
                            }
                        }

                        arrayTy = s->arrayTy;
                    }
                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     j;

                    asmptr = createAsm5(ASM_mv, REG_SPILL, arrayReg);
                    DL_APPEND(asmlist, asmptr);

                    // 考虑常量溢出

                    int sum = 0;
                    vector_each_address(&ptr->index, j, index) {
                        if (arrayTy->next) {
                            size_t n = getArraySize(arrayTy->next);
                            sum = n * 4;
                        } else {
                            sum = 4;
                        }
                        // if (arrayTy)
                        arrayTy = arrayTy->next;
                        if (index->ty & IS_IMM) {
                            fprintf(fp, "#%d:[%d] \n", j, index->imm.i);
                            sum *= index->imm.i;
                            Value v = SET_IMM_INT(sum);
                            // loadImm(fnc, fp, REG_CON, &v);
                            asmloadImm(fnc, fp, REG_CON, &v);
                            // fprintf(fp, "add    %s,%s,%s\n",
                            //         regName[REG_SPILL], regName[REG_SPILL],
                            //         regName[REG_CON]);
                            asmptr = createAsm2(ASM_add, REG_SPILL, REG_SPILL,
                                                REG_CON);
                            DL_APPEND(asmlist, asmptr);
                        } else {
                            int r2;
                            assert((index->ty & IS_SSA));
                            ssaSymbol *tmp =
                                getSSAlVariable(fnc, index->ssaind);
                            if (isSpill(index)) {
                                r2 = REG_SPILL2;
                                asmprocessMemoryLoad1(fnc, fp, r2, tmp);
                            } else {
                                assert((index->ty & IS_REG));
                                r2 = index->reg;
                            }
                            Value v = SET_IMM_INT(sum);
                            asmloadImm(fnc, fp, REG_CON, &v);
                            asmptr = createAsm2(ASM_mul, REG_CON, r2, REG_CON);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "add    %s,%s,%s\n",
                            //         regName[REG_SPILL], regName[REG_SPILL],
                            //         regName[REG_CON]);
                            asmptr = createAsm2(ASM_add, REG_SPILL, REG_SPILL,
                                                REG_CON);
                            DL_APPEND(asmlist, asmptr);
                        }
                    }
                    if (isSpill(&inst->ret)) {
                        asmprocessMemoryStore1(fnc, fp, REG_SPILL, s0);
                    } else {
                        assert((inst->ret.ty & IS_REG));
                        assert((inst->ret.ty & IS_SSA));
                        asmptr = createAsm5(ASM_mv, inst->ret.reg, REG_SPILL);
                        DL_APPEND(asmlist, asmptr);
                    }
                } break;

                case IR_OP_PARAM: {
                    if (overparammem >= 320) {
                        assert(0);
                    }
                    if (paramNum < 8) {
                        overparammem = 0;
                    }
                    functionCallInfo *callinfo;
                    callinfo = findFunctionCall(fnc, inst->n);
                    if (callinfo) {
                        curCall = callinfo;
                        // 没有浮点数寄存器
                        // 这个0是为了保存超过8个参数开辟的空间
                        int fpnum = 0;
                        for (size_t j = 0; j < sizeof(FPRegisterPools) /
                                                   sizeof(FPRegisterPools[0]);
                             j++) {
                            if (FPRegisterPools[j].isused == 1) {
                                fpnum++;
                            }
                        }
                        saveSize =
                            vector_len(&callinfo->reg) * 8 + 320 + fpnum * 8;
                        // 只预留了300
                        if (saveSize >= 600) assert(0);
                        asmptr =
                            createAsm3(ASM_addi, REG_sp, REG_sp, -saveSize);
                        DL_APPEND(asmlist, asmptr);
                        int reg;
                        int i = 0;
                        vector_each(&callinfo->reg, i, reg) {
                            asmptr =
                                createAsm7(ASM_sd, reg, REG_sp, i * 8 + 320);
                            DL_APPEND(asmlist, asmptr);
                        }
                        for (size_t j = 0; j < sizeof(FPRegisterPools) /
                                                   sizeof(FPRegisterPools[0]);
                             j++) {
                            if (FPRegisterPools[j].isused == 1) {
                                reg = FPRegisterPools[j].reg;
                                asmptr = createAsm7(ASM_fsw, reg, REG_sp,
                                                    i * 8 + 320);
                                DL_APPEND(asmlist, asmptr);
                                i++;
                            }
                        }
                    }
                    if (ValueIsImm(&inst->ret)) {
                        if (ValueIsInt(&inst->ret)) {
                            if (paramNum >= 8) {
                                // 多申请空间,我已经保存了caller寄存器,会被破坏
                                asmloadImm(fnc, fp, REG_CON, &inst->ret);
                                asmptr = createAsm7(ASM_sd, REG_CON, REG_sp,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                overparammem += 8;
                            } else {
                                asmloadImm(fnc, fp, functionArgument[paramNum],
                                           &inst->ret);
                            }
                        } else if (ValueIsFloat(&inst->ret)) {
                            if (paramNum >= 8) {
                                assert(0);
                                // loadFPImm(fnc, fp, REG_fs0, &inst->ret);
                                asmloadFPImm(fnc, fp, REG_fs0, &inst->ret);
                                // fprintf(fp, "fsw      %s,%d(sp)\n",
                                //         regName[REG_fs0], overparammem);
                                asmptr = createAsm7(ASM_fsw, REG_fs0, REG_sp,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                overparammem += 8;
                            } else {
                                if (isprintf) {
                                    assert(0);
                                    // loadFPImm(fnc, fp,
                                    //           FPfunctionArgument[paramNum],
                                    //           &inst->ret);
                                    asmloadFPImm(fnc, fp,
                                                 FPfunctionArgument[paramNum],
                                                 &inst->ret);
                                    // fprintf(
                                    //     fp, "fcvt.d.s    %s,%s\n",
                                    //     regName[FPfunctionArgument[paramNum]],
                                    //     regName[FPfunctionArgument[paramNum]]);
                                    asmptr = createAsm11(
                                        ASM_fcvtds,
                                        FPfunctionArgument[paramNum],
                                        FPfunctionArgument[paramNum]);
                                    DL_APPEND(asmlist, asmptr);
                                    // fprintf(

                                    //     fp, "fmv.x.d    %s,%s\n",
                                    //     regName[functionArgument[paramNum]],
                                    //     regName[FPfunctionArgument[paramNum]]);
                                    asmptr = createAsm11(
                                        ASM_fmv, functionArgument[paramNum],
                                        FPfunctionArgument[paramNum]);
                                    DL_APPEND(asmlist, asmptr);
                                } else {
                                    asmloadFPImm(fnc, fp,
                                                 FPfunctionArgument[paramNum],
                                                 &inst->ret);
                                }
                            }
                        } else {
                            assert(0);
                        }

                        // 数组指针
                    } else if ((inst->ret.ty & IS_ARRAY) &&
                               !(inst->ret.ty & IS_GLOBAL) &&
                               (inst->ret.ty & IS_SSA)) {
                        if (paramNum >= 8) assert(0);
                        // 数组指针
                        if (isSpill(&inst->ret)) {
                            s0->loc += saveSize;
                            asmprocessMemoryLoad1(
                                fnc, fp, functionArgument[paramNum], s0);
                            s0->loc -= saveSize;
                        } else {
                            assert((inst->ret.ty & IS_REG));
                            assert((inst->ret.ty & IS_SSA));
                            int reg = functionArgument[paramNum];
                            // 注意这里BUG,如果参数超过了8,sp被扩大,那么这里取值地址错误
                            if (issavereg(curCall, inst->ret.reg)) {
                                asmptr = createAsm7(
                                    ASM_ld, reg, REG_sp,
                                    getsaveregaddr(curCall, inst->ret.reg));
                                DL_APPEND(asmlist, asmptr);
                            } else {
                                asmptr = createAsm5(ASM_mv, reg, inst->ret.reg);
                                DL_APPEND(asmlist, asmptr);
                            }
                            // asmptr =
                            //     createAsm5(ASM_mv, inst->ret.reg, REG_SPILL);
                            // DL_APPEND(asmlist, asmptr);
                        }
                        // Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
                        // // fprintf(fp, "li    t2, %d\n", s->loc);
                        // asmptr = createAsm4(ASM_li, REG_t2, s->loc +
                        // saveSize); DL_APPEND(asmlist, asmptr);
                        // // fprintf(fp, "add    %s, t3,t2\n",
                        // //         regName[functionArgument[paramNum]]);
                        // asmptr = createAsm2(ASM_add,
                        // functionArgument[paramNum],
                        //                     REG_sp, REG_t2);
                        // DL_APPEND(asmlist, asmptr);
                        // // fprintf(fp, "ld    %s, 0(%s)\n",
                        // //         regName[functionArgument[paramNum]],
                        // //         regName[functionArgument[paramNum]]);
                        // asmptr = createAsm7(ASM_ld,
                        // functionArgument[paramNum],
                        //                     functionArgument[paramNum], 0);
                        // DL_APPEND(asmlist, asmptr);
                    } else if ((inst->ret.ty & IS_ARRAY) &&
                               !(inst->ret.ty & IS_GLOBAL)) {
                        // 局部数组
                        // 没有使用的局部数组没有分配值
                        assert(!(inst->ret.ty & IS_SSA));
                        Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
                        if (!s->isalloc) {
                            s->isalloc = 1;
                            if (s->iszero) {
                                vector_push_back(&zeorArrays, s);
                            } else {
                                s->loc = allocStack2(
                                    fnc, getArraySize2(s->arrayTy) * 2);
                                if (s->loc > 535800) {
                                    assert(0);
                                    allocStack2(fnc,
                                                getArraySize2(s->arrayTy) * 2);
                                    printf("\n%d", getArraySize2(s->arrayTy));
                                }
                            }
                        }
                        if (s->iszero) {
                            // fprintf(fp, "la    %s, .zeroarray_%x\n",
                            //         regName[REG_SPILL], s);
                            char buff[1024];
                            sprintf(buff, ".zeroarray_%x", s);
                            asmptr =
                                createAsm8(ASM_la, REG_SPILL, strdup(buff));
                            DL_APPEND(asmlist, asmptr);
                        } else {
                            // fprintf(fp, "li    t2, %d\n", s->loc);
                            asmptr =
                                createAsm4(ASM_li, REG_t2, s->loc + saveSize);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "add    %s, t3,t2\n",
                            //         regName[REG_SPILL]);
                            asmptr =
                                createAsm2(ASM_add, REG_SPILL, REG_sp, REG_t2);
                            DL_APPEND(asmlist, asmptr);
                        }

                        if (paramNum >= 8) {
                            asmptr = createAsm7(ASM_sd, REG_SPILL, REG_sp,
                                                overparammem);
                            DL_APPEND(asmlist, asmptr);
                            overparammem += 8;
                        } else {
                            // fprintf(fp, "mv    %s, %s\n",
                            //         regName[functionArgument[paramNum]],
                            //         regName[REG_SPILL]);
                            asmptr = createAsm5(
                                ASM_mv, functionArgument[paramNum], REG_SPILL);
                            DL_APPEND(asmlist, asmptr);
                        }

                    } else if ((inst->ret.ty & IS_ARRAY) &&
                               (inst->ret.ty & IS_GLOBAL)) {
                        // 全局数组
                        // loadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                        asmloadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                        assert(0);
                        // fprintf(fp, "sd      %s,%d(sp)\n",
                        // regName[REG_SPILL],
                        //         overparammem);
                        asmptr =
                            createAsm7(ASM_sd, REG_SPILL, REG_sp, overparammem);
                        DL_APPEND(asmlist, asmptr);
                        overparammem += 8;

                    } else if (inst->ret.ty & IS_POINTER &&
                               (inst->ret.ty & IS_SSA) &&
                               (!(inst->ret.ty & IS_ARRAY)) &&
                               (!(inst->ret.ty & TY_VOID))) {
                        // int a[122][12];a[33]
                        // 会被ssa对待,不可能这么实现
                        if (paramNum >= 8) {
                            if (isSpill(&inst->ret)) {
                                s0->loc += saveSize;
                                asmprocessMemoryLoad1(fnc, fp, REG_CON, s0);
                                s0->loc -= saveSize;
                                asmptr = createAsm7(ASM_sd, REG_CON, REG_sp,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                overparammem += 8;
                            } else {
                                assert((inst->ret.ty & IS_REG));
                                assert((inst->ret.ty & IS_SSA));
                                if (issavereg(curCall, inst->ret.reg)) {
                                    asmptr = createAsm7(
                                        ASM_ld, REG_CON, REG_sp,
                                        getsaveregaddr(curCall, inst->ret.reg));
                                    DL_APPEND(asmlist, asmptr);
                                    asmptr = createAsm7(ASM_sd, REG_CON, REG_sp,
                                                        overparammem);
                                    DL_APPEND(asmlist, asmptr);
                                    overparammem += 8;
                                } else {
                                    asmptr = createAsm7(ASM_sd, inst->ret.reg,
                                                        REG_sp, overparammem);
                                    DL_APPEND(asmlist, asmptr);
                                    overparammem += 8;
                                }
                            }

                        } else {
                            if (isSpill(&inst->ret)) {
                                s0->loc += saveSize;
                                asmprocessMemoryLoad1(
                                    fnc, fp, functionArgument[paramNum], s0);
                                s0->loc -= saveSize;
                            } else {
                                assert((inst->ret.ty & IS_REG));
                                assert((inst->ret.ty & IS_SSA));
                                int reg = functionArgument[paramNum];
                                // 注意这里BUG,如果参数超过了8,sp被扩大,那么这里取值地址错误
                                if (issavereg(curCall, inst->ret.reg)) {
                                    asmptr = createAsm7(
                                        ASM_ld, reg, REG_sp,
                                        getsaveregaddr(curCall, inst->ret.reg));
                                    DL_APPEND(asmlist, asmptr);
                                } else {
                                    asmptr =
                                        createAsm5(ASM_mv, reg, inst->ret.reg);
                                    DL_APPEND(asmlist, asmptr);
                                }
                                // asmptr =
                                //     createAsm5(ASM_mv, inst->ret.reg,
                                //     REG_SPILL);
                                // DL_APPEND(asmlist, asmptr);
                            }
                        }
                        printf("1");
                    } else if (inst->ret.ty & IS_VAR || inst->ret.ty & IS_SSA) {
                        if (!ValueIsInt(&inst->ret) &&
                            !inst->ret.ty & IS_POINTER)
                            assert(0);
                        // 全局数组地址传递
                        if (s0->ty & IS_POINTER) {
                        }
                        if (ValueIsInt(&inst->ret)) {
                            if (paramNum >= 8) {
                                if (isSpill(&inst->ret)) {
                                    s0->loc += saveSize;
                                    asmprocessMemoryLoad1(fnc, fp, REG_CON, s0);
                                    s0->loc -= saveSize;
                                    asmptr = createAsm7(ASM_sd, REG_CON, REG_sp,
                                                        overparammem);
                                    DL_APPEND(asmlist, asmptr);
                                    overparammem += 8;
                                } else {
                                    assert((inst->ret.ty & IS_REG));
                                    assert((inst->ret.ty & IS_SSA));
                                    // 注意这里BUG,如果参数超过了8,sp被扩大,那么这里取值地址错误
                                    if (issavereg(curCall, inst->ret.reg)) {
                                        asmptr = createAsm7(
                                            ASM_ld, REG_CON, REG_sp,
                                            getsaveregaddr(curCall,
                                                           inst->ret.reg));
                                        DL_APPEND(asmlist, asmptr);
                                        asmptr =
                                            createAsm7(ASM_sd, REG_CON, REG_sp,
                                                       overparammem);
                                        DL_APPEND(asmlist, asmptr);
                                        overparammem += 8;
                                    } else {
                                        asmptr =
                                            createAsm7(ASM_sd, inst->ret.reg,
                                                       REG_sp, overparammem);
                                        DL_APPEND(asmlist, asmptr);
                                        overparammem += 8;
                                    }
                                }
                                // assert(0);
                                // // processMemoryLoad1(fnc, fp, REG_CON, s0);
                                // s0->loc += saveSize;
                                // asmprocessMemoryLoad1(fnc, fp, REG_CON, s0);
                                // s0->loc -= saveSize;

                                // asmptr = createAsm7(ASM_sd, REG_CON, REG_sp,
                                //                     overparammem);
                                // DL_APPEND(asmlist, asmptr);
                                // overparammem += 8;

                            } else {
                                if (isSpill(&inst->ret)) {
                                    s0->loc += saveSize;
                                    asmprocessMemoryLoad1(
                                        fnc, fp, functionArgument[paramNum],
                                        s0);
                                    s0->loc -= saveSize;
                                } else {
                                    assert((inst->ret.ty & IS_REG));
                                    assert((inst->ret.ty & IS_SSA));
                                    int reg = functionArgument[paramNum];
                                    // 注意这里BUG,如果参数超过了8,sp被扩大,那么这里取值地址错误
                                    if (issavereg(curCall, inst->ret.reg)) {
                                        asmptr = createAsm7(
                                            ASM_ld, reg, REG_sp,
                                            getsaveregaddr(curCall,
                                                           inst->ret.reg));
                                        DL_APPEND(asmlist, asmptr);
                                    } else {
                                        asmptr = createAsm5(ASM_mv, reg,
                                                            inst->ret.reg);
                                        DL_APPEND(asmlist, asmptr);
                                    }
                                }
                            }
                        } else if (ValueIsFloat(&inst->ret)) {
                            if (paramNum >= 8) {
                                assert(0);
                                // processMemoryLoad1(fnc, fp, REG_CON, s0);
                                asmprocessMemoryLoad1(fnc, fp, REG_CON, s0);
                                // fprintf(fp, "li      s1,%d\n", overparammem);
                                asmptr =
                                    createAsm4(ASM_li, REG_s1, overparammem);
                                DL_APPEND(asmlist, asmptr);
                                // fprintf(fp, "sd      %s,%d(sp)\n",
                                //         regName[REG_CON], overparammem);
                                asmptr = createAsm7(ASM_sd, REG_CON, REG_sp,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                overparammem += 8;

                            } else {
                                if (isprintf) {
                                    assert(0);
                                    // processFPMemoryLoad1(
                                    //     fnc, fp,
                                    //     FPfunctionArgument[paramNum], s0);
                                    asmprocessFPMemoryLoad1(
                                        fnc, fp, FPfunctionArgument[paramNum],
                                        s0);
                                    // fprintf(
                                    //     fp, "fcvt.d.s    %s,%s\n",
                                    //     regName[FPfunctionArgument[paramNum]],
                                    //     regName[FPfunctionArgument[paramNum]]);
                                    asmptr = createAsm11(
                                        ASM_fcvtds,
                                        FPfunctionArgument[paramNum],
                                        FPfunctionArgument[paramNum]);
                                    DL_APPEND(asmlist, asmptr);
                                    // fprintf(

                                    //     fp, "fmv.x.d    %s,%s\n",
                                    //     regName[functionArgument[paramNum]],
                                    //     regName[FPfunctionArgument[paramNum]]);
                                    asmptr = createAsm11(
                                        ASM_fmv, functionArgument[paramNum],
                                        FPfunctionArgument[paramNum]);
                                    DL_APPEND(asmlist, asmptr);
                                } else {
                                    if (isSpill(&inst->ret)) {
                                        s0->loc += saveSize;
                                        asmprocessFPMemoryLoad1(
                                            fnc, fp,
                                            FPfunctionArgument[paramNum], s0);
                                        s0->loc -= saveSize;
                                    } else {
                                        assert((inst->ret.ty & IS_REG));
                                        assert((inst->ret.ty & IS_SSA));
                                        int reg = FPfunctionArgument[paramNum];
                                        // 注意这里BUG,如果参数超过了8,sp被扩大,那么这里取值地址错误
                                        if (isfpsavereg(curCall,
                                                        inst->ret.reg)) {
                                            asmptr = createAsm7(
                                                ASM_flw, reg, REG_sp,
                                                getfpsaveregaddr(
                                                    curCall, inst->ret.reg));
                                            DL_APPEND(asmlist, asmptr);
                                        } else {
                                            asmptr = createAsm11(ASM_fmvs, reg,
                                                                 inst->ret.reg);
                                            DL_APPEND(asmlist, asmptr);
                                        }
                                    }
                                }
                            }
                        } else {
                            assert(0);
                        }

                    } else if (inst->ret.ty & TY_VOID &&
                               inst->ret.ty & IS_POINTER) {
                        isprintf = true;
                        int reg = functionArgument[paramNum];
                        // TY_VOID | IS_POINTER是字符串
                        if (paramNum >= 8) assert(0);
                        RVcon con;
                        con.num = vector_len(&_con);
                        con.v = inst->ret;
                        vector_push_back(&_con, con);
                        // fprintf(fp, "la     %s, ._str%d\n", regName[reg],
                        //         con.num);
                        char buff[1024];
                        sprintf(buff, "._str%d", con.num);
                        asmptr = createAsm8(ASM_la, reg, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    } else {
                        assert(0);
                    }
                    paramNum++;

                }

                break;
                case IR_OP_CALL: {
                    functionCallInfo *callinfo;
                    callinfo = findFunctionCall(fnc, inst->n);
                    if (callinfo) {
                        // 没有浮点数寄存器
                        curCall = callinfo;
                        int fpnum = 0;
                        for (size_t j = 0; j < sizeof(FPRegisterPools) /
                                                   sizeof(FPRegisterPools[0]);
                             j++) {
                            if (FPRegisterPools[j].isused == 1) {
                                fpnum++;
                            }
                        }
                        saveSize =
                            vector_len(&callinfo->reg) * 8 + 320 + fpnum * 8;
                        asmptr =
                            createAsm3(ASM_addi, REG_sp, REG_sp, -saveSize);
                        DL_APPEND(asmlist, asmptr);
                        int reg;
                        int i = 0;

                        vector_each(&callinfo->reg, i, reg) {
                            asmptr =
                                createAsm7(ASM_sd, reg, REG_sp, i * 8 + 320);
                            DL_APPEND(asmlist, asmptr);
                        }
                        for (size_t j = 0; j < sizeof(FPRegisterPools) /
                                                   sizeof(FPRegisterPools[0]);
                             j++) {
                            if (FPRegisterPools[j].isused == 1) {
                                reg = FPRegisterPools[j].reg;
                                asmptr = createAsm7(ASM_fsw, reg, REG_sp,
                                                    i * 8 + 320);
                                DL_APPEND(asmlist, asmptr);
                                i++;
                            }
                        }
                    }
                    isprintf = false;
                    // 这里会导致局部申请很大,因为多余传入的参数没有考虑回收
                    overparammem = 0;
                    paramNum = 0;
                    Symbol *s = getGlobalSymbol(inst->args[0].symbol);
                    if (inst->args[1].isTailRec) {
                        asmptr = createAsm3(ASM_addi, REG_sp, REG_sp, saveSize);
                        DL_APPEND(asmlist, asmptr);

                        char buff[1024];
                        sprintf(buff, ".%s_bb_%d", fnc->name, 0);
                        asmptr = createAsm1(ASM_J, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    } else {
                        asmptr = createAsm1(ASM_call, getToken(s->tok)->str);
                        DL_APPEND(asmlist, asmptr);
                    }

                    if (inst->ret.ty & TY_VOID) {
                    } else if (ValueIsFloat(&inst->ret)) {
                        if (isSpill(&inst->ret)) {
                            if (s0->loc == 0) {
                                s0->loc = allocStack2(fnc, 8);
                            }
                            s0->loc += saveSize;
                            asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
                            s0->loc -= saveSize;
                        } else {
                            assert((inst->ret.ty & IS_REG));
                            assert((inst->ret.ty & IS_SSA));
                            asmptr =
                                createAsm11(ASM_fmvs, inst->ret.reg, REG_fa0);

                            DL_APPEND(asmlist, asmptr);
                        }
                    } else {
                        if (isSpill(&inst->ret)) {
                            if (s0->loc == 0) {
                                s0->loc = allocStack2(fnc, 8);
                            }
                            s0->loc += saveSize;
                            asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                            s0->loc -= saveSize;
                        } else {
                            assert((inst->ret.ty & IS_REG));
                            assert((inst->ret.ty & IS_SSA));
                            asmptr = createAsm5(ASM_mv, inst->ret.reg, REG_a0);
                            DL_APPEND(asmlist, asmptr);
                        }
                    }
                    // 恢复caller寄存器
                    int reg;
                    int i = 0;
                    vector_each(&curCall->reg, i, reg) {
                        if (inst->ret.ty & IS_REG && inst->ret.reg == reg) {
                            // 上一个a0寄存器的生命区间刚好结束
                            // 避免返回值被覆盖
                            if (reg == reg) {
                                continue;
                            }
                        }
                        asmptr = createAsm7(ASM_ld, reg, REG_sp, i * 8 + 320);
                        DL_APPEND(asmlist, asmptr);
                    }
                    for (size_t j = 0; j < sizeof(FPRegisterPools) /
                                               sizeof(FPRegisterPools[0]);
                         j++) {
                        if (FPRegisterPools[j].isused == 1) {
                            reg = FPRegisterPools[j].reg;
                            if (inst->ret.ty & IS_REG && inst->ret.reg == reg) {
                                // 上一个a0寄存器的生命区间刚好结束
                                // 避免返回值被覆盖
                                if (reg == reg) {
                                    continue;
                                }
                            }
                            asmptr =
                                createAsm7(ASM_flw, reg, REG_sp, i * 8 + 320);
                            DL_APPEND(asmlist, asmptr);
                            i++;
                        }
                    }
                    asmptr = createAsm3(ASM_addi, REG_sp, REG_sp, saveSize);
                    DL_APPEND(asmlist, asmptr);
                    saveSize = 0;
                    curCall = NULL;
                    break;
                }
                case IR_OP_NOP:
                    break;

                default:
                    assert(0);
            };
            asmptr = createAsm13(inst);
            DL_APPEND(asmlist, asmptr);
        }
        if (bb->nextblock && bb->branchblock) {
        } else if (bb->nextblock) {
            char buff[1024];
            sprintf(buff, ".%s_bb_%d", fnc->name, bb->nextblock->bbName);
            asmptr = createAsm1(ASM_J, strdup(buff));
            DL_APPEND(asmlist, asmptr);
        } else if ((bb->nextblock == NULL) && (bb->branchblock == NULL)) {
            char buff[1024];
            sprintf(buff, ".%s_ret", fnc->name);
            asmptr = createAsm1(ASM_J, strdup(buff));
            DL_APPEND(asmlist, asmptr);
        } else {
            assert(0);
        }
        instruction *tmpIns;
        // vector_each3(&bb->inst, tmpIns) { free(tmpIns); }
        // vector_free(&bb->inst);
        // vector_free(&bb->df);
        // vector_free(&bb->postdf);
        // vector_free(&bb->inedges);
        // vector_free(&bb->doms);
    }
    char buff[1024];
    sprintf(buff, ".%s_ret", fnc->name);
    asmptr = createAsm9(ASM_label, strdup(buff));
    DL_APPEND(asmlist, asmptr);
    // restoreCallee(fnc, fp);
    optasmgen_functionEpilogue(fnc, fp);
    fprintf(fp, "\n");
}

void optasmgen_functionProlog(Function *fnc, FILE *fp) {
    asminstruction *asmptr;
    // 多于参数
    size_t          size;
    size_t          rasize;

    asmptr = createAsm5(ASM_mv, REG_t0, REG_sp);
    DL_APPEND(asmlist, asmptr);

    // fprintf(fp, "li    t1, -%d\n", bytealigned(540000, 16));
    asmptr = createAsm4(ASM_li, REG_t1, 0);
    fnc->stackSize = &asmptr->src[0].imm;
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add    sp, sp,t1\n");
    asmptr = createAsm2(ASM_add, REG_sp, REG_sp, REG_t1);
    DL_APPEND(asmlist, asmptr);

    // fprintf(fp, "li    t1, %d\n", bytealigned(535800, 16));
    asmptr = createAsm4(ASM_li, REG_t1, 0);
    fnc->raAddr = &asmptr->src[0].imm;
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add    t1, sp,t1\n");
    asmptr = createAsm2(ASM_add, REG_t1, REG_sp, REG_t1);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "sd       ra, 0(t1)\n");
    asmptr = createAsm7(ASM_sd, REG_ra, REG_t1, 0);
    DL_APPEND(asmlist, asmptr);
    for (size_t i = 0; i < sizeof(RVcallee) / sizeof(RVcallee[0]); i++) {
        if (iRegisterPools[RVcallee[i]].isused) {
            asmptr = createAsm4(ASM_li, REG_t1, 0);
            iRegisterPools[RVcallee[i]].addr = &asmptr->src[0].imm;
            DL_APPEND(asmlist, asmptr);
            asmptr = createAsm2(ASM_add, REG_t1, REG_sp, REG_t1);
            DL_APPEND(asmlist, asmptr);
            // fprintf(fp, "sd       ra, 0(t1)\n");
            asmptr = createAsm7(ASM_sd, RVcallee[i], REG_t1, 0);
            DL_APPEND(asmlist, asmptr);
        }
    }
}
void optasmgen_functionEpilogue(Function *fnc, FILE *fp) {
    asminstruction *asmptr;
    size_t          size;
    size_t          rasize;

    fnc->loc1 += 1000;  // 以免ra和callee寄存器被覆盖
    *fnc->raAddr = fnc->loc1 - 8;
    int num = 0;
    for (size_t i = 0; i < sizeof(RVcallee) / sizeof(RVcallee[0]); i++) {
        if (iRegisterPools[RVcallee[i]].isused) {
            int tmp = fnc->loc1 - 16 - (num * 8);
            *iRegisterPools[RVcallee[i]].addr = tmp;
            num++;
            asmptr = createAsm4(ASM_li, REG_t1, tmp);
            DL_APPEND(asmlist, asmptr);
            asmptr = createAsm2(ASM_add, REG_t1, REG_sp, REG_t1);
            DL_APPEND(asmlist, asmptr);
            asmptr = createAsm7(ASM_ld, RVcallee[i], REG_t1, 0);
            DL_APPEND(asmlist, asmptr);
        }
    }
    // fprintf(fp, "li    t1, %d\n", bytealigned(535800, 16));
    asmptr = createAsm4(ASM_li, REG_t1, fnc->loc1 - 8);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add    t1, sp,t1\n");
    asmptr = createAsm2(ASM_add, REG_t1, REG_sp, REG_t1);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "ld       ra, 0(t1)\n");
    asmptr = createAsm7(ASM_ld, REG_ra, REG_t1, 0);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "li    t1, %d\n", bytealigned(540000, 16));
    *fnc->stackSize = -bytealigned(fnc->loc1, 16);
    asmptr = createAsm4(ASM_li, REG_t1, bytealigned(fnc->loc1, 16));
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add    sp, sp,t1\n");
    asmptr = createAsm2(ASM_add, REG_sp, REG_sp, REG_t1);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "ret\n");
    asmptr = createAsm10(ASM_ret);
    DL_APPEND(asmlist, asmptr);
}
AsmState getAsmState(riscvAsm asmop);
/*
li reg1,imm
add any,reg2,reg1

2:
mv reg1,reg1

3:
li t2,316
add t2,t2,sp
sw a1,0(t2)



 */
void     peepholeOptimizationPass(Function *fnc) {
    asminstruction *head, *ptr, *tmp;
    head = (asminstruction *)fnc->asmlist;
    DL_FOREACH_SAFE(head, ptr, tmp) {
        if (ptr->isPO) {
            DL_DELETE(head, ptr);
            free(ptr);
            continue;
        }
        if (ptr->op == ASM_li && ptr->src[0].imm < 2045 &&
            ptr->src[0].imm > -2045) {
            if (ptr->next && ptr->next->op == ASM_add) {
                asminstruction *p1 = ptr->next;
                if (p1->src[1].reg == ptr->dest) {
                    p1->isPO = 1;
                    int asmop;
                    switch (p1->op) {
                        case ASM_add:
                            asmop = ASM_addi;
                            break;
                        case ASM_sub:
                            asmop = ASM_subi;
                            break;
                        case ASM_mul:
                            asmop = ASM_muli;
                            break;
                        case ASM_div:
                            asmop = ASM_divi;
                            break;
                        default:
                            assert(0);
                    }
                    int imm = ptr->src[0].imm;
                    ptr->op = asmop;
                    ptr->dest = p1->dest;
                    ptr->src[0].reg = p1->src[0].reg;
                    ptr->src[1].imm = imm;
                }
            }
        }
        if ((ptr->op == ASM_mv) && (ptr->dest == ptr->src[0].reg)) {
            ptr->isPO = 1;
        }

        // mv reg2,reg1
        // lw any,0(reg2)
        // ASM_7
        // if (ptr->op == ASM_mv && (getAsmState(ptr->next->op) == ASM_7)) {
        //     asminstruction *p1 = ptr->next;
        //     if (ptr->dest == p1->src[0].reg) {
        //         if (p1->src[1].reg == ptr->dest) {
        //             p1->isPO = 1;
        //             int asmop;
        //             switch (p1->op) {
        //                 case ASM_add:
        //                     asmop = ASM_addi;
        //                     break;
        //                 case ASM_sub:
        //                     asmop = ASM_subi;
        //                     break;
        //                 case ASM_mul:
        //                     asmop = ASM_muli;
        //                     break;
        //                 case ASM_div:
        //                     asmop = ASM_divi;
        //                     break;
        //                 default:
        //                     assert(0);
        //             }
        //             int imm = ptr->src[0].imm;
        //             ptr->op = asmop;
        //             ptr->dest = p1->dest;
        //             ptr->src[0].reg = p1->src[0].reg;
        //             ptr->src[1].imm = imm;
        //         }
        //         ptr->
        //     }
        // }

        //  li t2,316
        // add t2,t2,sp
        // sw a1,0(t2)
        // = sw a1,316(sp)
        if (ptr->op == ASM_li) {
            asminstruction *p1 = ptr->next;
            if (p1->op == ASM_add && (p1->dest == p1->src[0].reg) &&
                (p1->dest == ptr->dest)) {
                asminstruction *p2 = p1->next;
                if ((getAsmState(p2->op) == ASM_7) &&
                    (p2->src[0].reg == ptr->dest)) {
                    int imm = ptr->src->imm;
                    p1->isPO = 1;
                    p2->isPO = 1;
                    ptr->op = p2->op;
                    ptr->dest = p2->dest;
                    ptr->src[0].reg = p1->src[1].reg;
                    ptr->src[0].index = imm;
                }
            }
        }
    }
    fnc->asmlist = head;
}

/**
 * @brief 该pass进行尾递归优化
 * @param[in] *fnc
 */
void TailRecursionPass(Function *fnc) {
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_RETI) {
                if (inst->ret.ty & IS_SSA) {
                    ssaSymbol *s = getSSAlVariable(fnc, inst->ret.ssaind);
                    if (s->def->op == IR_OP_CALL) {
                        Symbol *_f = getGlobalSymbol(s->def->args[0].symbol);
                        if (_f->fnc == fnc) {
                            s->def->args[1].isTailRec = true;
                        }
                    }
                }
            } else if (inst->op == IR_OP_RETF) {
                if (inst->ret.ty & IS_SSA) {
                    ssaSymbol *s = getSSAlVariable(fnc, inst->ret.ssaind);
                    if (s->def->op == IR_OP_CALL) {
                        Symbol *_f = getGlobalSymbol(s->def->args[0].symbol);
                        if (_f->fnc == fnc) {
                            s->def->args[1].isTailRec = true;
                        }
                    }
                }
            }
        }
    }
}
