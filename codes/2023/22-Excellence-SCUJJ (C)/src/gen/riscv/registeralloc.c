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

void printIntervals(FILE *fp, Function *fnc);
void porcessloop(Function *fnc);
#define _X(op) #op,
const char *regName[] = {__R "t0", "t1",  "t2",  "t3",   __FR "sp", "ra",
                         "zero",   "ft8", "ft9", "ft10", "ft11"};

Reg         functionArgument[] = {REG_a0, REG_a1, REG_a2, REG_a3,
                                  REG_a4, REG_a5, REG_a6, REG_a7};
Reg         FPfunctionArgument[] = {REG_fa0, REG_fa1, REG_fa2, REG_fa3,
                                    REG_fa4, REG_fa5, REG_fa6, REG_fa7};
Reg         RVfpcaller[] = {REG_fa0, REG_fa1, REG_fa2, REG_fa3,
                            REG_fa4, REG_fa5, REG_fa6, REG_fa7};
Reg RVfpcallee[] = {REG_fs0, REG_fs1, REG_fs2, REG_fs3, REG_fs4,  REG_fs5,
                    REG_fs6, REG_fs7, REG_fs8, REG_fs9, REG_fs10, REG_fs11};
Reg RVcaller[] = {REG_a0, REG_a1, REG_a2, REG_a3,
                  REG_a4, REG_a5, REG_a6, REG_a7};
Reg RVcallee[] = {REG_s0, REG_s1, REG_s2, REG_s3, REG_s4,  REG_s5,
                  REG_s6, REG_s7, REG_s8, REG_s9, REG_s10, REG_s11};

#undef _X
#define _X(op) {.reg = REG_##op},

Register iRegisterPools[] = {__R};
Register FPRegisterPools[] = {__FR};

vector_template(liveIntervals, intervals);

// void addIntervals(Value v,liveRange r){

// }

static inline void initIntervals(Function *fnc) {
    vector_free(&intervals);
    vector_reserve(&intervals, vector_len(&fnc->SSAValuePool));
    intervals.count = intervals.capacity;
}

static inline void destroyIntervals(Function *fnc) {
    vector_free(&intervals);
    vector_init(&intervals);
}

static instruction *REP(instruction *x) {
    if (x->join == x) {
        return x;
    } else {
        return REP(x->join);
    }
}
bool intervalsIsUnion(liveIntervals *x, liveIntervals *y) {
    size_t     i;
    liveRange *yrange;
    vector_each(&y->LRs, i, yrange) {
        size_t     i;
        liveRange *xrange;
        vector_each(&x->LRs, i, xrange) {
            // printf("[%d,%d]      [%d,%d]\n", yrange->begin, yrange->end,
            //        xrange->begin, xrange->end);

            if (yrange->begin <= xrange->begin && yrange->end >= xrange->begin)
                return true;
            if (yrange->begin <= xrange->end && yrange->end >= xrange->end)
                return true;
        }
    }
    return false;
}

bool intervalsIsUnion1(liveIntervals *x, liveIntervals *y, bool isdest) {
    size_t     i;
    liveRange *yrange;
    vector_each(&y->LRs, i, yrange) {
        size_t     i;
        liveRange *xrange;
        vector_each(&x->LRs, i, xrange) {
            // printf("[%d,%d]      [%d,%d]\n", yrange->begin, yrange->end,
            //        xrange->begin, xrange->end);

            if (yrange->begin < xrange->begin && yrange->end > xrange->begin)
                return true;
            if (yrange->begin < xrange->end && yrange->end > xrange->end)
                return true;
        }
    }
    return false;
}
static void join(Function *fnc, instruction *x, instruction *y) {
    liveIntervals *i = vector_get_address(&intervals, REP(x)->ret.ssaind);
    ;
    liveIntervals *j = vector_get_address(&intervals, REP(y)->ret.ssaind);
    if (vector_len(&j->LRs) == 0) return;
    if (vector_len(&i->LRs) == 0) return;
    if (!intervalsIsUnion1(i, j, 1) && i != j) {
        size_t     n;
        liveRange *range;
        uint32_t   v;
        size_t     m;
        vector_each(&i->ssaind, m, v) { vector_push_back(&j->ssaind, v); }
        vector_free(&(i->ssaind));
        vector_init(&(i->ssaind));

        // vector_each(i, n, range) {
        //     vector_each(&range->ssaind, m, v) {
        //         vector_push_back(&vector_first(j)->ssaind, v);
        //     }
        //     vector_init(&range->ssaind);
        // }
        vector_each(&i->LRs, n, range) {
            // if (vector_len(&range->ssaind) != 0) {
            //     printf("%d", vector_len(&range->ssaind));
            //     assert(0);
            // }
            // vector_push_back(j, range);
            size_t     i;
            liveRange *ptr;
            vector_each(&j->LRs, i, ptr) {
                if (ptr->end == range->begin) {
                    ptr->end = range->end;
                } else if (ptr->end == range->end) {
                    if (ptr->begin > range->begin) {
                        ptr->begin = range->begin;
                    }
                } else if (ptr->begin == range->end) {
                    ptr->begin = range->begin;
                } else if (range->begin < ptr->begin) {
                    vector_insert(&j->LRs, i, range);
                } else {
                    continue;
                }
                goto end;
            }
            vector_push_back(&j->LRs, range);
        end:
            NULL;
        }
        vector_free(&i->LRs);
        vector_init(&i->LRs);
        x->join = REP(y);
    }
}

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
                                setCurrentBB(fnc, n);
                                // TODO 因为多了一些指令，需要之后计算指令总数
                                //  p->join = createMOV(fnc,&v,&p->v);
                                instruction *tmp = createMOV(fnc, &v, &p->v);
                                getSSAlVariable(fnc, v.ssaind)->def = tmp;
                                // tmp->join = inst;
                                tmp->join = tmp;
                                setVariableDef(fnc, v.ssaind, tmp);
                                p->v = v;
                                p->bb = n;
                            } else if (p->bb == n) {
                                Value v = createSSATempVariable(fnc, p->v.ty);
                                setCurrentBB(fnc, n);
                                // p->join = createMOV(fnc,&v,&p->v);
                                instruction *tmp = createMOV(fnc, &v, &p->v);
                                getSSAlVariable(fnc, v.ssaind)->def = tmp;
                                // tmp->join = inst;
                                tmp->join = tmp;
                                setVariableDef(fnc, v.ssaind, tmp);
                                p->v = v;
                            }
                        }
                    }
                }
            }
        }
    }
}

static void _instructionNumbering(size_t *ind, BasicBlock *bb) {
    size_t tmp = *ind;
    LOOPBBALLINST({
        inst->n = tmp;
        tmp++;
    })
    *ind = tmp;
}

// 对指令编号,拓扑顺序遍历所有基本块
// BUG循环可能有大问isLoop
// TODO重写
static void instructionNumbering(Function *fnc) {
    size_t ind = 0;

    initInedges(fnc);
    buildInedges(NULL, fnc->firstBB);
    LOOPALLBB({
        bb->visited = 0;
        bb->inNum = vector_len(&bb->inedges);
    })
    vector_template(BasicBlock *, tmpbb);
    vector_init(&tmpbb);

    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        size_t      i;
        BasicBlock *bb = NULL;
        vector_each(&fnc->BBs, i, bb) {
            if (bb->visited) continue;
            // 注意bb->isLoop,死代码删除可能导致编号错误
            // if (bb->inNum == 0 || bb->isLoop) {
            if (bb->inNum == 0 || bb->idom->visited) {
                bb->visited = 1;
                LOOPALLBBNEXT({ vector_push_back(&tmpbb, bb); })
                printf("bb%d:%d\n", bb->bbName, bb->inNum);
                _instructionNumbering(&ind, bb);
                continue;
            }
        }
        BasicBlock *ptr;
        vector_each(&tmpbb, i, ptr) { ptr->inNum--; }
        vector_clear(&tmpbb);
    }
    fnc->instNumbering = true;
    // printBasicAllBlock(stdout, fnc);
}

// 区间是否重叠
static bool rangeIsOverlaps(liveRange *x, liveRange *y) {
    if (y->begin >= x->begin && y->begin <= x->end) return true;
    if (y->end >= x->begin && y->end <= x->end) return true;
}

static void addRange(ssaSymbol *v, BasicBlock *bb, size_t end) {
    // b.first.n • i.n • b.last.n, i.n是否在这2者之间
    liveRange    range = {0};
    instruction *frist = vector_get(&bb->inst, 0);
    instruction *last = vector_get(&bb->inst, vector_len(&bb->inst) - 1);
    if (frist->n <= v->def->n && v->def->n <= last->n) {
        range.begin = v->def->n;
        range.end = end;
    } else {
        range.begin = frist->n;
        range.end = end;
    }
    liveIntervals *ll = vector_get_address(&intervals, v->ssaind);
    if (vector_len(&ll->LRs) == 0) {
        liveRange *tmp = sy_malloc(sizeof(liveRange));
        *tmp = range;
        ll->reg = -1;
        vector_push_back(&ll->ssaind, v->ssaind);
        vector_push_back(&ll->LRs, tmp);
        return;
    }
    // TODO
    liveRange *tmp = vector_get(&ll->LRs, vector_len(&ll->LRs) - 1);
    // 合并区间
    if (tmp->end == range.begin) {
        tmp->end = range.end;
    } else if (tmp->end == range.end) {
        if (tmp->begin > range.begin) {
            tmp->begin = range.begin;
        }
    } else if (tmp->begin == range.end) {
        tmp->begin = range.begin;
    } else {
        //     liveRange *tmp = sy_malloc(sizeof(liveRange));
        //     *tmp = range;
        //     tmp->reg = -1;
        //     liveRange *item;
        //     if (tmp->begin < vector_first(ll)->begin) {
        //         tmp->ssaind = vector_first(ll)->ssaind;
        //         vector_init(&vector_first(ll)->ssaind);
        //         vector_insert(ll, 0, tmp);
        //         return;
        //     }
        //     vector_each3(ll, item) {
        //         if (tmp->begin < item->begin) {
        //             tmp->ssaind = item->ssaind;
        //             vector_init(&item->ssaind);
        //             vector_insert(ll, i, tmp);
        //             goto _end;
        //         }
        //     }
        //     vector_push_back(ll, tmp);
        // _end:
        //     NULL;
        liveRange *tmp = sy_malloc(sizeof(liveRange));
        *tmp = range;
        liveRange *item;
        vector_each3(&ll->LRs, item) {
            if (tmp->begin < item->begin) {
                vector_insert(&ll->LRs, i, tmp);
                goto _end;
            }
        }
        vector_push_back(&ll->LRs, tmp);
    _end:
        NULL;
    }
}

// 以任意顺序遍历控制流图，找出每个块末尾的活值，并计算这些值的范围。
void buildIntervals(Function *fnc) {
    BasicBlock  *g_b = NULL;
    m_bitset_ptr live;
    // bitset_init(live);
    // TODO 初始化所有指令的集合

    // TODO 指令size
    LOOPALLBB({
        bitset_init(bb->live);
        bitset_resize(bb->live, vector_len(&fnc->SSAValuePool));
        bitset_init_empty(bb->live);
    })

    size_t      i;
    BasicBlock *bb = NULL;

    // vector_each_reverse(&fnc->BBs, i, bb) {
    //     g_b = bb;
    //     live = bb->live;
    //     // bitset_or(live, bb->live);
    //     LOOPALLBBNEXT({
    //         bitset_or(live, bb->live);
    //         LOOPBBALLPHI({
    //             phiFunction *phi = inst->args[0].phi;
    //             phiParam    *p;
    //             size_t       i;
    //             vector_each_address(&phi->param, i, p) {
    //                 if (p->bb == g_b) {
    //                     bitset_set_at(live, p->v.ssaind, 1);
    //                 }
    //             }
    //             bitset_set_at(live, inst->ret.ssaind, 0);
    //         })
    //     })
    // }

    vector_each_reverse(&fnc->BBs, i, bb) {
        g_b = bb;
        live = bb->live;
        bitset_init_empty(live);
        // bitset_or(live, bb->live);
        LOOPALLBBNEXT({
            bitset_or(live, bb->live);
            LOOPBBALLPHI({
                phiFunction *phi = inst->args[0].phi;
                phiParam    *p;
                size_t       i;
                vector_each_address(&phi->param, i, p) {
                    // assert(p->join);
                    if (p->bb == g_b) {
                        bitset_set_at(live, p->v.ssaind, 1);
                    }
                }
                bitset_set_at(live, inst->ret.ssaind, 0);
            })
        })
        // LOOP inst in live
        if (vector_len(&bb->inst) == 0) continue;
        bitset_it_t it;
        for (bitset_it(it, live); !bitset_end_p(it); bitset_next(it)) {
            bool c = *bitset_cref(it);
            if (c) {
                addRange(
                    getSSAlVariable(fnc, it->index), bb,
                    vector_get(&bb->inst, vector_len(&bb->inst) - 1)->n + 1);
            }
        }

        // LOOP inst in b
        instruction *inst;
        size_t       i;
        instruction *prevInst = NULL;
        vector_each_reverse(&bb->inst, i, inst) {
            switch (inst->op) {
                OP3_3AC_COND
                OP3_3AC_OPER
                OP2_3AC
                case IR_OP_ADDPTR:
                case IR_OP_GETITEMPTR:
                    if (inst->ret.ty & IS_SSA) {
                        bitset_set_at(live, inst->ret.ssaind, 0);
                    }
                    break;  // 这里不是bug
                    OP1_3AC
                case IR_OP_PHI:
                    if (inst->op == IR_OP_ARGUMENT) {
                        if (inst->ret.ty & IS_SSA) {
                            bitset_set_at(live, inst->ret.ssaind, 0);
                        }
                    }
                    break;
                default:
                    assert(0);
            }

            switch (inst->op) {
                OP3_3AC_COND
                OP3_3AC_OPER
                if (inst->args[1].ty & IS_SSA) {
                    if (!bitset_get(live, inst->args[1].ssaind)) {
                        bitset_set_at(live, inst->args[1].ssaind, 1);
                        addRange(getSSAlVariable(fnc, inst->args[1].ssaind), bb,
                                 inst->n);
                    }
                }
                __attribute__((fallthrough));
                OP2_3AC
                // 替换used(被使用的变量),variable renaming
                if (inst->args[0].ty & IS_SSA) {
                    if (!bitset_get(live, inst->args[0].ssaind)) {
                        bitset_set_at(live, inst->args[0].ssaind, 1);
                        addRange(getSSAlVariable(fnc, inst->args[0].ssaind), bb,
                                 inst->n);
                    }
                }
                if (inst->op == IR_OP_SOTRE) {
                    if (inst->ret.ty & IS_SSA) {
                        if (!bitset_get(live, inst->ret.ssaind)) {
                            bitset_set_at(live, inst->ret.ssaind, 1);
                            addRange(getSSAlVariable(fnc, inst->ret.ssaind), bb,
                                     inst->n);
                        }
                    }
                }

                break;  // 这里不是bug
                case IR_OP_GETPTR:
                case IR_OP_ADDPTR:
                    assert(0);
                    if (inst->args[1].ty & IS_SSA) {
                        if (!bitset_get(live, inst->args[1].ssaind)) {
                            bitset_set_at(live, inst->args[1].ssaind, 1);
                            addRange(getSSAlVariable(fnc, inst->args[1].ssaind),
                                     bb, inst->n);
                        }
                    }
                    // 只记录数组,数组产生的局部变量地址不计算
                    if (inst->args[0].ty & IS_SSA) {
                        if (!bitset_get(live, inst->args[0].ssaind)) {
                            bitset_set_at(live, inst->args[0].ssaind, 1);
                            addRange(getSSAlVariable(fnc, inst->args[0].ssaind),
                                     bb, inst->n);
                        }
                    }
                    break;
                case IR_OP_RETI:
                case IR_OP_RETF:
                    // 替换used(被使用的变量),variable renaming
                    if (inst->ret.ty & IS_SSA) {
                        if (!bitset_get(live, inst->ret.ssaind)) {
                            bitset_set_at(live, inst->ret.ssaind, 1);
                            addRange(getSSAlVariable(fnc, inst->ret.ssaind), bb,
                                     inst->n);
                        }
                    }
                    break;
                case IR_OP_GETITEMPTR: {
                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     j;
                    if (inst->args[0].ty & IS_SSA) {
                        if (!bitset_get(live, inst->args[0].ssaind)) {
                            bitset_set_at(live, inst->args[0].ssaind, 1);
                            addRange(getSSAlVariable(fnc, inst->args[0].ssaind),
                                     bb, inst->n);
                        }
                    }
                    vector_each_address(&ptr->index, j, index) {
                        if (index->ty & IS_SSA) {
                            if (!bitset_get(live, index->ssaind)) {
                                bitset_set_at(live, index->ssaind, 1);
                                addRange(getSSAlVariable(fnc, index->ssaind),
                                         bb, inst->n);
                            }
                        }
                    }
                } break;
                case IR_OP_PHI:
                    if (inst->ret.ty & IS_SSA) {
                        if (!bitset_get(live, inst->ret.ssaind)) {
                            bitset_set_at(live, inst->ret.ssaind, 1);
                            addRange(getSSAlVariable(fnc, inst->ret.ssaind), bb,
                                     inst->n);
                        }
                    }
                case IR_OP_IF:
                    break;
                case IR_OP_ARGUMENT:
                case IR_OP_PARAM:
                    if (inst->ret.ty & IS_SSA) {
                        if (!bitset_get(live, inst->ret.ssaind)) {
                            bitset_set_at(live, inst->ret.ssaind, 1);
                            addRange(getSSAlVariable(fnc, inst->ret.ssaind), bb,
                                     inst->n);
                        }
                    }
                    break;

                default:
                    assert(0);
            }
            prevInst = inst;
        }
    }
}

vector_template(liveIntervals *, allIntervals);

Register *getFPregister(Reg r) {
    for (size_t i = 0; i < sizeof(FPRegisterPools) / sizeof(FPRegisterPools[0]);
         i++) {
        if (FPRegisterPools[i].reg == r) {
            return &FPRegisterPools[i];
        }
    }
    assert(0);
}
static int intervalcmp(const void *a, const void *b) {
    liveIntervals *a1 = *(liveIntervals **)a;
    liveIntervals *b1 = *(liveIntervals **)b;
    if (vector_get(&a1->LRs, 0)->begin > vector_get(&b1->LRs, 0)->begin) {
        return 1;
    }
    return 0;
}
void initRegister(Function *fnc) {
    for (size_t i = 0; i < sizeof(iRegisterPools) / sizeof(iRegisterPools[0]);
         i++) {
        iRegisterPools[i].used = 0;
        iRegisterPools[i].isused = 0;
        iRegisterPools[i].w = 0;
    }
    for (size_t i = 0; i < sizeof(FPRegisterPools) / sizeof(FPRegisterPools[0]);
         i++) {
        FPRegisterPools[i].used = 0;
        FPRegisterPools[i].isused = 0;
        FPRegisterPools[i].w = 0;
    }
    vector_free(&allIntervals);
    vector_init(&allIntervals);
    liveIntervals *ll;
    size_t         i;
    vector_each_address(&intervals, i, ll) {
        if (vector_len(&ll->LRs) > 0) vector_push_back(&allIntervals, ll);
    }
    qsort(allIntervals.data, vector_len(&allIntervals), sizeof(liveIntervals *),
          intervalcmp);
    // qsort(vector_data(&allIntervals),vector_len(&allIntervals),sizeof(void
    // *),intervalcmp);
    vector_each(&allIntervals, i, ll) {
        printf("\n%d", vector_get(&ll->LRs, 0)->begin);
    }
}

bool registerIsNull(Register *R) {
    for (size_t i = 0; i < sizeof(iRegisterPools) / sizeof(iRegisterPools[0]);
         i++) {
        if (R[i].used == 0) {
            return false;
        }
    }
    return true;
}

static bool llIsFP(Function *fnc, liveIntervals *ll) {
    ssaSymbol *s = getSSAlVariable(fnc, vector_get(&ll->ssaind, 0));
    if ((s->ty & TY_FLOAT) && (!(s->ty & IS_POINTER))) {
        return true;
    }
    return false;
}

int8_t registerPop(Register *R) {
    for (size_t i = 0; i < sizeof(iRegisterPools) / sizeof(iRegisterPools[0]);
         i++) {
        if (R[i].used == 0) {
            return R[i].reg;
        }
    }
}
bitset_t unhandled, active, inactive, handled;

// 不统计phi参数
size_t   getRangeWeight(Function *fnc, liveIntervals *allssa, liveRange *range,
                        size_t curBeg) {
    size_t   weight = 0;
    size_t   num;
    uint32_t v;
    size_t   i;
    size_t   begin, end;
    begin = range->begin;
    end = range->end;
    vector_each(&allssa->ssaind, i, v) {
        num = 0;
        LOOPALLBB({LOOPBBALLINST({
            if (!(inst->n >= begin && inst->n <= end)) {
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
                case IR_OP_GETPTR:
                    if (inst->args[1].ty & IS_SSA) {
                        if (inst->args[1].ssaind == v) {
                            num++;
                        }
                    }
                    break;
            }
        })})
        weight += num;
    }
    return weight;
}

static inline int allocStack(Function *fnc) {
    fnc->loc1 += 8;
    return fnc->loc1 - 8;
}
// 为了传入参数,不允许分配
static inline int allocStack2(Function *fnc, int size) {
    fnc->loc1 += size;
    if (fnc->loc1 >= 540000) {
        assert(0);
    }
    return fnc->loc1 - size;
}

typedef struct spillInfo {
    size_t         n;
    liveIntervals *ll;
    int            loc;
    bool           isSpill;
    bool           isMem;
    bool           isMoveMem;
    int            r;
    bool           isalloc;  // 调试,是否已经分配了
} spillInfo;
vector_template(spillInfo, allSpill);
// void printAtice(void *fnc, liveIntervals *cur) {
//     liveIntervals *range;
//     bitset_it_t    it;
//     for (bitset_it(it, active); !bitset_end_p(it); bitset_next(it)) {
//         if (*bitset_cref(it)) {
//             range = vector_get(&allIntervals, it->index);
//             size_t i;
//             int    v;

//             vector_each(&range->ssaind, i, v) {
//                 ssaSymbol *sym = getSSAlVariable(fnc, v);
//                 if (!sym->sym) {
//                     printf("_%d", sym);
//                 } else if (sym->sym->isTemp) {
//                     printf("_%d", sym->sym->tmpInd);
//                 } else {
//                     printf("%s_%d", getToken(sym->sym->tok)->str,
//                            sym->valueind);
//                 }
//                 printf(",");
//             }
//             printf("    [%d,%d]\n", vector_get(range, 0)->begin,
//                    vector_get(range, 0)->end);
//         }
//     }
//     size_t i;
//     int    v;
//     vector_each(&cur->ssaind, i, v) {
//         ssaSymbol *sym = getSSAlVariable(fnc, v);
//         if (!sym->sym) {
//             printf("_%d", sym);
//         } else if (sym->sym->isTemp) {
//             printf("_%d", sym->sym->tmpInd);
//         } else {
//             printf("%s_%d", getToken(sym->sym->tok)->str, sym->valueind);
//         }
//         printf(",");
//     }
//     printf("    [%d,%d]\n", vector_get(&cur->LRs, 0)->begin,
//            vector_get(&cur->LRs, 0)->end);
// }
/**
 * @brief 根据权重溢出寄存器
 * @param[in] *fnc
 * @param[in] *cur
 * @param[in] index
 */
void assignMemLoc(Function *fnc, liveIntervals *cur, size_t index) {
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
            if (intervalsIsUnion(cur, range)) {
                // w[i.reg] ← w[i.reg] + i.weight
                size_t     tmp = w[range->reg];
                liveRange *Ra;
                vector_each3(&range->LRs, Ra) {
                    tmp += getRangeWeight(fnc, range, Ra, 0);
                }
                w[range->reg] = tmp;
            }
        }
    }
    for (bitset_it(it, inactive); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            range = vector_get(&allIntervals, it->index);
            if (intervalsIsUnion(cur, range)) {
                size_t     tmp = w[range->reg];
                liveRange *Ra;
                vector_each3(&range->LRs, Ra) {
                    tmp += getRangeWeight(fnc, range, Ra, 0);
                }
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

    size_t     curweight = 0;
    liveRange *Ra;
    vector_each3(&cur->LRs, Ra) {
        curweight += getRangeWeight(fnc, cur, Ra, 0);
    }

    // 该声明区间未分配过寄存器
    if (curweight < w[r]) {
        bitset_set_at(handled, index, 1);
        spillInfo sp;
        sp.ll = cur;
        sp.n = vector_get(&cur->LRs, 0)->begin;
        // 每次溢出都会分配一次,过于浪费stack?
        sp.loc = allocStack(fnc);
        cur->loc = sp.loc;
        vector_push_back(&allSpill, sp);
    } else {
        // 将寄存器溢出
        //  还有active和in中
        //  assert(0);
        assert(iRegisterPools[r].ll);
        range = iRegisterPools[r].ll;
        // printAtice(fnc, range);
        if (vector_len(&range->LRs) == 1) {
            if ((vector_get(&range->LRs, 0)->end -
                 vector_get(&range->LRs, 0)->begin) <= 3) {
                w[r] += 10;
                goto _repo;
            }
        }
        spillInfo sp;
        sp.ll = iRegisterPools[r].ll;
        sp.n = vector_get(&cur->LRs, 0)->begin;
        // 每次溢出都会分配一次,过于浪费stack?
        sp.loc = allocStack(fnc);
        sp.r = r;
        sp.isMoveMem = true;
        vector_push_back(&allSpill, sp);
        bitset_set_at(handled, iRegisterPools[r].llind, 1);
        bitset_set_at(active, iRegisterPools[r].llind, 0);
        // 只有寄存器中的溢出，其他全部mem

        // 共享同一寄存器的所有生命周期,活动和未活动全部溢出
        for (bitset_it(it, inactive); !bitset_end_p(it); bitset_next(it)) {
            if (*bitset_cref(it)) {
                range = vector_get(&allIntervals, it->index);
                if (cur->reg == r) {
                    bitset_set_at(handled, it->index, 1);
                    bitset_set_at(inactive, it->index, 0);
                    sp.ll = range;
                    sp.n = vector_get(&cur->LRs, 0)->begin;
                    // 每次溢出都会分配一次,过于浪费stack?
                    sp.loc = allocStack(fnc);
                    sp.r = r;
                    vector_push_back(&allSpill, sp);
                }
            }
        }

        cur->reg = r;
        iRegisterPools[r].ll = cur;
        iRegisterPools[r].llind = index;
    }
}

static inline void freeRegister(int reg){};
// 检查过期或重新激活的非活动间隔

void               checkInactiveIntervals(Function *fnc, liveIntervals *cur) {
    bitset_it_t it;
    for (bitset_it(it, inactive); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            liveIntervals *ll = vector_get(&allIntervals, it->index);
            bool           isFree = true;
            size_t         i;
            liveRange     *R;
            if (vector_final(&ll->LRs)->end < vector_first(&cur->LRs)->begin) {
                // move i to handled
                bitset_set_at(inactive, it->index, 0);
                bitset_set_at(handled, it->index, 1);
            } else {
                // 重新激活该寄存器
                size_t begin = vector_get(&cur->LRs, 0)->begin;
                vector_each(&ll->LRs, i, R) {
                    if (begin > R->begin && begin < R->end) {
                        bitset_set_at(active, it->index, 1);
                        bitset_set_at(inactive, it->index, 0);
                        iRegisterPools[ll->reg].used = 1;
                        iRegisterPools[ll->reg].ll = ll;
                    }
                }
            }
        }
    }
}
// 检查过期的活动间隔
void checkActiveIntervals(Function *fnc, liveIntervals *cur) {
    bitset_it_t it;
    for (bitset_it(it, active); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            liveIntervals *ll = vector_get(&allIntervals, it->index);
            size_t         i;
            liveRange     *R;
            // printf("%d,%d\n", vector_final(ll)->end,
            // vector_first(cur)->begin);

            if (vector_final(&ll->LRs)->end < vector_first(&cur->LRs)->begin) {
                // move i to handled and add i.reg to free
                printf("\n活动间隔过期\n");
                bitset_set_at(active, it->index, 0);
                bitset_set_at(handled, it->index, 1);
                iRegisterPools[ll->reg].used = 0;
                iRegisterPools[ll->reg].ll = NULL;
                iRegisterPools[ll->reg].llind = 0;
            } else {
                // move i to inactive and add i.reg to free
                // 不相交,可能有bug
                if (!intervalsIsUnion(cur, ll)) {
                    // new
                    //  bitset_set_at(active, it->index, 0);
                    //  bitset_set_at(inactive, it->index, 1);
                    //  iRegisterPools[ll->reg].used = 0;
                    //  iRegisterPools[ll->reg].ll = NULL;
                }
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

vector_template(functionCallInfo, _calls);
functionCallInfo CurFncCall;
bool             callNum;
vector_template(Reg, usedSavee);

typedef struct saveRegister {
    Reg r;
    int saved;  //-1代表了stack地址
    int loc;
} saveRegister;
vector_template(saveRegister, savelist);

// 别忘记了标记callee寄存器

// 没有区分指令中是否已经分配了寄存器
bool allocCallee(Reg *r) {
    Register *R = iRegisterPools;
    for (size_t i = 0; i < sizeof(RVcallee) / sizeof(RVcallee[0]); i++) {
        if (R[RVcallee[i]].used == 0) {
            R[RVcallee[i]].used = 1;
            *r = R[RVcallee[i]].reg;
            R[RVcallee[i]].isused = 1;
            return true;
        }
    }
    return false;
}
bool allocfpCallee(Reg *r) {
    Register *R = FPRegisterPools;
    for (size_t i = 0; i < sizeof(RVfpcallee) / sizeof(RVfpcallee[0]); i++) {
        Register *r1 = getFPregister(RVfpcaller[i]);
        if (r1->used == 0) {
            r1->used = 1;
            *r = r1->reg;
            r1->isused = 1;
            return true;
        }
    }
    return false;
}
// 每次进入该函数都要清除信息
//  保存caller寄存器
void saveCaller(Function *fnc) {
    Register *R = iRegisterPools;
    for (size_t i = 0; i < sizeof(RVcaller) / sizeof(RVcaller[0]); i++) {
        if (R[RVcaller[i]].used == 1) {
            Reg tmp;
            if (allocCallee(&tmp)) {
                saveRegister save;
                save.r = R[RVcaller[i]].reg;
                save.saved = tmp;
                vector_push_back(&savelist, save);
                instruction *i = sy_malloc(sizeof(instruction));
                i->op = IR_OP_MOV;
                i->args[0].reg = save.r;
                i->ret.reg = save.saved;
                vector_push_back(&CurFncCall.insts1, i);
            } else {
                // 没有多余了寄存器进行保存了,分配到栈中
                saveRegister save;
                save.r = R[RVcaller[i]].reg;
                save.saved = -1;
                save.loc = allocStack(fnc);
                vector_push_back(&savelist, save);
                instruction *i = sy_malloc(sizeof(instruction));
                i->op = IR_OP_MOV;
                i->args[0].reg = save.r;

                // 这里如何判定是局部地址？
                i->ret.loc = save.loc;
                i->ret.ty |= IS_POINTER;
                vector_push_back(&CurFncCall.insts1, i);
            }
        }
    }
    R = FPRegisterPools;
    for (size_t i = 0; i < sizeof(RVfpcaller) / sizeof(RVfpcaller[0]); i++) {
        Register *r = getFPregister(RVfpcaller[i]);
        if (r->used == 1) {
            Reg tmp;
            if (allocfpCallee(&tmp)) {
                saveRegister save;
                save.r = r->reg;
                save.saved = tmp;
                vector_push_back(&savelist, save);
                instruction *i = sy_malloc(sizeof(instruction));
                i->op = IR_OP_MOV;
                i->args[0].reg = save.r;
                i->ret.reg = save.saved;
                vector_push_back(&CurFncCall.insts1, i);
            } else {
                // 没有多余了寄存器进行保存了,分配到栈中
                saveRegister save;
                save.r = r->reg;
                save.saved = -1;
                save.loc = allocStack(fnc);
                vector_push_back(&savelist, save);
                instruction *i = sy_malloc(sizeof(instruction));
                i->op = IR_OP_MOV;
                i->args[0].reg = save.r;

                // 这里如何判定是局部地址？
                i->ret.loc = save.loc;
                i->ret.ty |= IS_POINTER;
                vector_push_back(&CurFncCall.insts1, i);
            }
        }
    }
}
// 恢复caller寄存器
void restoreCaller(Function *fnc, bool isret) {
    saveRegister save;
    vector_each3(&savelist, save) {
        if (save.saved != -1) {
            if (isret && (save.r == REG_RET || save.r == FPREG_RET)) continue;
            // move reg,reg
            instruction *i = sy_malloc(sizeof(instruction));
            i->op = IR_OP_MOV;
            i->args[0].reg = save.saved;
            i->ret.reg = save.r;
            vector_push_back(&CurFncCall.insts2, i);
        } else {
            if (isret && (save.r == REG_RET || save.r == FPREG_RET)) continue;
            // move reg,mem
            instruction *i = sy_malloc(sizeof(instruction));
            i->op = IR_OP_MOV;
            i->args[0].loc = save.loc;
            i->args[0].ty |= IS_POINTER;
            // 这里如何判定是局部地址？
            i->ret.reg = save.r;
            vector_push_back(&CurFncCall.insts2, i);
        }
    }
}
void porcessCalleeSize(Function *fnc) {
    Register *r;
    for (size_t i = 0; i < sizeof(RVcallee) / sizeof(RVcallee[0]); i++) {
        r = &iRegisterPools[RVcallee[i]];
        if (r->isused) {
            r->loc = allocStack(fnc);
        }
    }
    for (size_t i = 0; i < sizeof(RVfpcallee) / sizeof(RVfpcallee[0]); i++) {
        Register *r = getFPregister(RVfpcallee[i]);
        if (r->isused) {
            r->loc = allocStack(fnc);
        }
    }
}
// 保存callee寄存器
void saveCallee(Function *fnc, FILE *fp) {
    Register *r;
    for (size_t i = 0; i < sizeof(RVcallee) / sizeof(RVcallee[0]); i++) {
        r = &iRegisterPools[RVcallee[i]];
        if (r->isused) {
            //  sd      ra, 8(sp)
            fprintf(fp, "sd      %s, %d(sp)\n", regName[r->reg], r->loc);
        }
    }
    for (size_t i = 0; i < sizeof(RVfpcallee) / sizeof(RVfpcallee[0]); i++) {
        Register *r = getFPregister(RVfpcallee[i]);
        if (r->isused) {
            //  sd      ra, 8(sp)
            fprintf(fp, "sd      %s, %d(sp)\n", regName[r->reg], r->loc);
        }
    }
}
// 恢复callee寄存器
void restoreCallee(Function *fnc, FILE *fp) {
    Register *r;
    for (size_t i = 0; i < sizeof(RVcallee) / sizeof(RVcallee[0]); i++) {
        r = &iRegisterPools[RVcallee[i]];
        if (r->isused) {
            // ld      s0, 32(sp)
            // r->loc = allocStack(fnc);
            fprintf(fp, "ld      %s, %d(sp)\n", regName[r->reg], r->loc);
        }
    }
    for (size_t i = 0; i < sizeof(RVfpcallee) / sizeof(RVfpcallee[0]); i++) {
        Register *r = getFPregister(RVfpcallee[i]);
        if (r->isused) {
            // ld      s0, 32(sp)
            // r->loc = allocStack(fnc);
            fprintf(fp, "ld      %s, %d(sp)\n", regName[r->reg], r->loc);
        }
    }
}

int findsaveRegister(int r) {
    saveRegister save;
    vector_each3(&savelist, save) {
        if (save.r == r) {
            return i;
        }
    }
    return -1;
}
void createFunctionCall(Function *fnc, int ind) {
    CurFncCall = vector_get(&_calls, ind);
    vector_clear(&savelist);
    saveCaller(fnc);
    saveRegister save;
    // 再进行ir
    // param的时候，要另一种读取寄存器,这里将被移动的寄存器保存下来,并在ir
    // param中写入被保存的信息

    // 收回为了保护caller用到的寄存器
    vector_each3(&savelist, save) {
        // saved为-1是栈地址
        if (save.saved >= REG_fa0) {
            FPRegisterPools[save.saved].used = 0;
        } else {
            iRegisterPools[save.saved].used = 0;
        }
    }
    // vector_each3(&savelist, save) {
    //     if (save.saved >= REG_fa0) {
    //         FPRegisterPools[save.saved].isused = 0;
    //     } else {
    //         instruction *inst;
    //         vector_each3(&CurFncCall.params, inst) {
    //             if (inst->ret.ty & IS_SSA) {
    //                 Register *R = iRegisterPools;
    //                 for (size_t i = 0;
    //                      i < sizeof(iRegisterPools) /
    //                      sizeof(iRegisterPools[0]); i++) {
    //                     int            v;
    //                     size_t         j;
    //                     liveIntervals *ll = R[i].ll;
    //                     if (!ll) continue;
    //                     vector_each(&vector_first(ll)->ssaind, j, v) {
    //                         if (v == inst->ret.ssaind) {
    //                             int tmp = findsaveRegister(i);
    //                             if (tmp != -1) {
    //                                 if (savelist.data[tmp].saved == -1) {
    //                                     inst->isMem0 = true;
    //                                     inst->ret.loc =
    //                                     savelist.data[tmp].loc;
    //                                 } else {
    //                                     inst->ret.reg =
    //                                         savelist.data[tmp].saved;
    //                                 }
    //                             }
    //                         }
    //                     }
    //                 }
    //             }
    //         }
    //         iRegisterPools[save.saved].ll = 0;
    //     }
    // }
    restoreCaller(fnc, CurFncCall.isret);
    *vector_get_address(&_calls, ind) = CurFncCall;
}
void porcessFunctionCall(Function *fnc) {
    liveIntervals    *ll;
    size_t            _i;

    functionCallInfo *call;
    vector_each_address(&_calls, _i, call) {
        size_t i;
        vector_each_reverse(&allIntervals, i, ll) {
            if (vector_len(&ll->LRs) == 0) continue;
            if (vector_get(&ll->LRs, 0)->begin <= call->num) {
                // printInterval(stdout, fnc, ll);
                ll->callNum++;
                ll->iscall = true;
                ll->callind = _i;
                vector_push_back(&ll->callinds, _i);
                break;
            }
        }
    }

    // saveCaller(fnc);
    // restoreCaller(fnc);
    // printf("%d", vector_get(live, 0)->begin);
    // if (vector_get(live, 0)->begin > CurFncCall.num) {
    // }
}

// 函数调用信息,必须在分配之前调用
void initFunctionCall(Function *fnc) {
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
    // if (vector_len(&_calls) == 0) {
    //     callsEnd = false;
    // } else {
    //     callsEnd = true;
    //     CurFncCall = vector_first(&_calls);
    //     vector_remove(&_calls, 0);
    // }
}

// 寄存器分配
void linearScan(Function *fnc) {
    initRegister(fnc);
    // unhandled ← all intervals in increasing order of their start points
    // active ← {}; inactive ← {}; handled ← {}

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

    porcessloop(fnc);  // 必须这里执行
    qsort(allIntervals.data, vector_len(&allIntervals), sizeof(liveIntervals *),
          intervalcmp);
    porcessFunctionCall(fnc);
    printIntervals(stderr, fnc);
    bitset_it_t it;
    for (bitset_it(it, unhandled); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            bitset_set_at(unhandled, it->index, 0);
            cur = vector_get(&allIntervals, it->index);
            // if (cur->LRs.data[0]->begin == 21) {
            //     printf("");
            // }

            // if (callsEnd) {
            //     porcessFunctionCall(fnc, cur);
            //     if (vector_len(&_calls) == 0) {
            //         callsEnd = false;
            //     } else {
            //         callsEnd = true;
            //         CurFncCall = vector_first(&_calls);
            //         vector_remove(&_calls, 0);
            //     }
            // }

            // active
            checkActiveIntervals(fnc, cur);

            // inactive
            checkInactiveIntervals(fnc, cur);
            memcpy(f, iRegisterPools, sizeof(iRegisterPools));
            memcpy(fp, FPRegisterPools, sizeof(FPRegisterPools));
            // inactive
            {
                bitset_it_t it;
                for (bitset_it(it, inactive); !bitset_end_p(it);
                     bitset_next(it)) {
                    if (*bitset_cref(it)) {
                        liveIntervals *ll =
                            vector_get(&allIntervals, it->index);
                        if (intervalsIsUnion(ll, cur)) {
                            f[ll->reg].used = 1;
                        }
                    }
                }
            }
            // unhandled
            {
                bitset_it_t it;
                for (bitset_it(it, unhandled); !bitset_end_p(it);
                     bitset_next(it)) {
                    if (*bitset_cref(it)) {
                        liveIntervals *ll =
                            vector_get(&allIntervals, it->index);
                        if (intervalsIsUnion(ll, cur) && ll->reg >= 0) {
                            f[ll->reg].used = 1;
                        }
                    }
                }
            }
            // BUG:未处理：unhandled的每个固定间隔 i 与 cur 重叠
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
            if (cur->iscall) {
                size_t call;

                printf("\n%d", cur->LRs.data[0]->begin);
                ;
                vector_each3(&cur->callinds, call) {
                    createFunctionCall(fnc, call);
                }
            }
        }
    }
}

void printInterval(FILE *fp, Function *fnc, liveIntervals *ll) {
    int        i;
    liveRange *range;
    uint32_t   v;
    vector_each(&ll->ssaind, i, v) {
        ssaSymbol *sym = getSSAlVariable(fnc, v);
        if (!sym->sym) {
            fprintf(fp, "_%d", sym->valueind);
        } else if (sym->sym->isTemp) {
            fprintf(fp, "_%d", sym->sym->tmpInd);
        } else {
            fprintf(fp, "%s_%d", getToken(sym->sym->tok)->str, sym->valueind);
        }
        fprintf(fp, ",");
    }
    fprintf(fp, ":");
    vector_each(&ll->LRs, i, range) {
        size_t   i;
        uint32_t v;
        fprintf(fp, "[%d,%d] ,", range->begin, range->end);
    }
    fprintf(fp, "\n");
}
void printIntervals(FILE *fp, Function *fnc) {
    size_t        i;
    liveIntervals live;
    vector_each(&intervals, i, live) {
        size_t     i;
        liveRange *range;
        int8_t     reg = -1;
        if (vector_len(&live.LRs) == 0) continue;
        reg = live.reg;
        uint32_t v;
        vector_each(&live.ssaind, i, v) {
            ssaSymbol *sym = getSSAlVariable(fnc, v);
            if (!sym->sym) {
                fprintf(fp, "_%d", sym->valueind);
            } else if (sym->sym->isTemp) {
                fprintf(fp, "_%d", sym->sym->tmpInd);
            } else {
                fprintf(fp, "%s_%d", getToken(sym->sym->tok)->str,
                        sym->valueind);
            }
            fprintf(fp, ",");
        }

        fprintf(fp, ":");
        vector_each(&live.LRs, i, range) {
            size_t   i;
            uint32_t v;
            fprintf(fp, "[%d,%d] ,", range->begin, range->end);
        }
        fprintf(fp, "reg:%d", reg);
        fprintf(fp, "\n");
    }
}

void printIntervals2(FILE *fp, Function *fnc) {
    size_t         i;
    liveIntervals *ll;
    vector_each(&allIntervals, i, ll) {
        liveIntervals live = *ll;
        size_t        i;
        liveRange    *range;
        int8_t        reg = -1;
        if (vector_len(&live.LRs) == 0) continue;
        liveRange *allssa = vector_first(&live.LRs);
        reg = live.reg;
        uint32_t v;
        vector_each(&live.ssaind, i, v) {
            ssaSymbol *sym = getSSAlVariable(fnc, v);
            if (!sym->sym) {
                fprintf(fp, "_%d", sym->valueind);
            } else if (sym->sym->isTemp) {
                fprintf(fp, "_%d", sym->sym->tmpInd);
            } else {
                fprintf(fp, "%s_%d", getToken(sym->sym->tok)->str,
                        sym->valueind);
            }
            fprintf(fp, ",");
        }

        fprintf(fp, ":");
        vector_each(&live.LRs, i, range) {
            size_t   i;
            uint32_t v;
            fprintf(fp, "[%d,%d] ,", range->begin, range->end);
        }
        fprintf(fp, "reg:%d", reg);
        fprintf(fp, "\n");
    }
}

void joinValue(Function *fnc) {
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_MOV && inst->args[0].ty & IS_SSA) {
                instruction *_i =
                    getSSAlVariable(fnc, inst->args[0].ssaind)->def;
                join(fnc, _i, inst);
            } else if (inst->op == IR_OP_PHI) {
                phiFunction *phi = inst->args[0].phi;
                phiParam    *p;
                size_t       i;
                vector_each_address(&phi->param, i, p) {
                    // printValue(stdout, fnc, &inst->ret);

                    join(fnc, getSSAlVariable(fnc, p->v.ssaind)->def, inst);
                }
            }
            // else if (inst->op == IR_OP_GETPTR && inst->args[0].ty & IS_SSA) {
            //     assert(0);
            //     instruction *_i = getSSAlVariable(fnc,
            //     inst->args[0].ssaind)->def; join(fnc, _i, inst);
            // }
        }
    }
}

static void mappingRegister(Function *fnc, liveIntervals *allssa,
                            liveRange *range, int8_t reg) {
    size_t n;
    size_t i;
    // uint32_t ssaind;
    // vector_each(&range->ssaind, i, ssaind) { printf("%d\n", ssaind); }
    for (i = range->begin; i <= range->end; i++) {
        n = i;
        size_t      i;
        BasicBlock *bb = NULL;
        vector_each(&fnc->BBs, i, bb) {
            size_t              i;
            struct instruction *inst = NULL;
            vector_each(&bb->inst, i, inst) {
                if (inst->n == n) {
                    size_t   i;
                    uint32_t ssaind;
                    if (inst->op == IR_OP_PHI) break;

                    switch (inst->op) {
                        OP3_3AC_OPER
                        OP3_3AC_COND
                        case IR_OP_ADDPTR:
                            vector_each(&allssa->ssaind, i, ssaind) {
                                if (inst->args[1].ty & IS_SSA &&
                                    inst->args[1].ssaind == ssaind)
                                    inst->args[1].reg = reg;
                            }
                            OP2_3AC
                            vector_each(&allssa->ssaind, i, ssaind) {
                                if (inst->args[0].ty & IS_SSA &&
                                    inst->args[0].ssaind == ssaind)
                                    inst->args[0].reg = reg;
                            }
                            OP1_3AC
                            vector_each(&allssa->ssaind, i, ssaind) {
                                if (inst->ret.ty & IS_SSA &&
                                    inst->ret.ssaind == ssaind)
                                    inst->ret.reg = reg;
                            }
                            break;
                        case IR_OP_GETITEMPTR: {
                            arrayAddr *ptr = inst->args[1].arrayAddr;
                            Value     *index;
                            size_t     j;
                            size_t     i;
                            vector_each(&allssa->ssaind, i, ssaind) {
                                if (inst->ret.ssaind == ssaind)
                                    inst->ret.reg = reg;
                            }

                            vector_each_address(&ptr->index, j, index) {
                                size_t i;
                                vector_each(&allssa->ssaind, i, ssaind) {
                                    if (index->ty & IS_SSA &&
                                        index->ssaind == ssaind)
                                        index->reg = reg;
                                }
                            }
                        } break;
                        default:
                            assert(0);
                    }
                }
            }
        }
    }
}

static inline spillInfo *isSpill(liveIntervals *ll) {
    spillInfo *sp;
    size_t     i;
    vector_each_address(&allSpill, i, sp) {
        if (sp->ll == ll) return sp;
    }
    return NULL;
}
// 处理溢出情况
static void mappingSpillRegister(Function *fnc, spillInfo *sp,
                                 liveIntervals *allsaa, liveRange *range,
                                 int8_t reg) {
    size_t n;
    size_t i;
    for (i = range->begin; i <= range->end; i++) {
        n = i;
        bool isSpill = false;
        bool isMem = false;
        if (i >= sp->n) isMem = true;
        size_t      i;
        BasicBlock *bb = NULL;
        vector_each(&fnc->BBs, i, bb) {
            size_t              i;
            struct instruction *inst = NULL;
            vector_each(&bb->inst, i, inst) {
                if (inst->n == n) {
                    size_t   i;
                    uint32_t ssaind;
                    if (inst->op == IR_OP_PHI) break;
                    switch (inst->op) {
                        OP3_3AC_OPER
                        OP3_3AC_COND

                        vector_each(&allsaa->ssaind, i, ssaind) {
                            if (inst->args[1].ty & IS_SSA &&
                                inst->args[1].ssaind == ssaind) {
                                if (isMem) {
                                    inst->args[1].loc = sp->loc;
                                    inst->isMem2 = true;
                                } else {
                                    inst->args[1].reg = reg;
                                }
                            }
                        }
                        OP2_3AC
                        vector_each(&allsaa->ssaind, i, ssaind) {
                            if (inst->args[0].ty & IS_SSA &&
                                inst->args[0].ssaind == ssaind) {
                                if (isMem) {
                                    inst->args[0].loc = sp->loc;
                                    inst->isMem1 = true;
                                } else {
                                    inst->args[0].reg = reg;
                                }
                            }
                        }
                        OP1_3AC
                        vector_each(&allsaa->ssaind, i, ssaind) {
                            if (inst->ret.ty & IS_SSA &&
                                inst->ret.ssaind == ssaind) {
                                if (isMem) {
                                    inst->ret.loc = sp->loc;
                                    inst->isMem0 = true;
                                } else {
                                    inst->ret.reg = reg;
                                }
                            }
                        }
                        break;
                        case IR_OP_GETITEMPTR: {
                            arrayAddr *ptr = inst->args[1].arrayAddr;
                            Value     *index;
                            size_t     j;
                            if (inst->ret.ty & IS_SSA &&
                                inst->ret.ssaind == ssaind) {
                                if (isMem) {
                                    inst->ret.loc = sp->loc;
                                    inst->isMem0 = true;
                                } else {
                                    inst->ret.reg = reg;
                                }
                            }
                            // vector_each_address(&ptr->index, j, index) {
                            //     if (index->ty & IS_SSA &&
                            //         index->ssaind == ssaind)
                            //         index->reg = reg;
                            // }
                        } break;
                    }
                }
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
        if (vector_len(&live->LRs) == 0) continue;
        spillInfo *sp = isSpill(live);

        size_t     i;
        liveRange *range;
        if (sp) {
            // 处理寄存器溢出
            reg = live->reg;
            vector_each(&live->LRs, i, range) {
                mappingSpillRegister(fnc, sp, live, range, reg);
            }
        } else {
            reg = live->reg;
            vector_each(&live->LRs, i, range) {
                mappingRegister(fnc, live, range, reg);
            }
        }
    }
}
// TODO
/**
 * @brief 得到区间的权重,用于寄存器分配的溢出,忽略φ-函数中的访问
 * @param[in] *fnc
 * @param[in] live
 */
uint32_t    getLiveIntervalsWeight(Function *fnc, liveIntervals live) {}
/**
 * @brief 预先分配寄存器
 * @param[in] *fnc
 * @param[in] ssaind
 * @param[in] reg
 */
static void mappingRegister2(Function *fnc, uint32_t ssaind, int8_t reg) {
    size_t         i;
    liveIntervals *live;
    vector_each_address(&intervals, i, live) {
        size_t     i;
        liveRange *range;
        uint32_t   ssa;
        vector_each(&live->ssaind, i, ssa) {
            if (ssa == ssaind) {
                live->reg = reg;
                return;
            }
        }
        // vector_each(&live->LRs, i, range) {
        //     size_t i;
        //     vector_each(&range->ssaind, i, ssa) {
        //         if (ssa == ssaind) {
        //             vector_first(&live)->reg = reg;
        //         }
        //     }
        // }
    }
    return;
}

static liveIntervals *getssaIntervals(uint32_t ssaind) {
    size_t         i;
    liveIntervals *live;
    vector_each_address(&intervals, i, live) {
        size_t     i;
        liveRange *range;
        uint32_t   ssa;
        vector_each(&live->ssaind, i, ssa) {
            if (ssa == ssaind) {
                return live;
            }
        }
    }
    return NULL;
}

struct overParams {
    size_t   begin;
    int      loc;
    uint32_t v;
    bool     ispointer;
};
vector_template(struct overParams, _overp);
/**
 * @brief 寄存器预分配
 */
void preRegisterAllocation(Function *fnc) {
    vector_clear(&_overp);
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
                    paramNum++;
                    fnc->overParam = 1;
                    inst->args[0].imm.i = loc;
                    liveIntervals    *ll = getssaIntervals(inst->ret.ssaind);
                    struct overParams p = {0};
                    p.begin = vector_get(&ll->LRs, 0)->begin;
                    p.loc = loc;
                    p.v = inst->ret.ssaind;
                    if (inst->ret.ty & IS_POINTER) {
                        p.ispointer = true;
                        loc += 8;
                    } else {
                        p.ispointer = false;
                        loc += 4;
                    }
                    vector_push_back(&_overp, p);

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

int          m1111(Function *fnc, FILE *fp);
void         gen_function(Function *fnc, FILE *fp);

extern char *outputFile;

// void processCond(Function *fnc){
//     LOOPALLBB({
//         LOOPBBALLINST({
//             switch
//         })
//     })
// }
// 函数有返回值但是没有被用到
static void  processRet(Function *fnc) {
    BasicBlock *bb;
    int         i;
    vector_each(&fnc->BBs, i, bb) {
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_CALL && !(inst->ret.ty & TY_VOID)) {
                if (!getssaIntervals(inst->ret.ssaind)) {
                    inst->ret.ty = TY_VOID;
                }
            }
        }
    }
}

void processsArrayPointer(Function *fnc) {
    LOOPALLBB({LOOPBBALLINST({
        if (inst->op == IR_OP_ARGUMENT && inst->ret.ty & IS_ARRAY) {
            uint32_t   symbol = inst->ret.symbol;
            Value      v = createSSATempVariable(fnc, inst->ret.ty);
            ssaSymbol *ssa = getSSAlVariable(fnc, v.ssaind);
            Symbol    *s = getLocalVariable(fnc, symbol);
            ssa->sym = s;
            ssa->arrayTy = s->arrayTy;
            ssa->def = inst;
            inst->ret.ty |= IS_SSA;
            // inst->ret = v;
            LOOPALLBB({LOOPBBALLINST({
                if (inst->op == IR_OP_GETITEMPTR &&
                    inst->args[0].symbol == symbol) {
                    inst->args[0].ty |= IS_SSA;
                    // inst->args[0].ssaind = v.ssaind;
                } else if (inst->op == IR_OP_PARAM &&
                           inst->ret.symbol == symbol) {
                    inst->ret.ty |= IS_SSA;
                }
            })})
        }
    })})
}

struct LoopNestingForest {
    BasicBlock *head;
    BasicBlock *back;
    vector_template(BasicBlock *, bb);
};
vector_template(struct LoopNestingForest, LNFs);
// 寻找回边
void Loopnestingforest2(Function *fnc, BasicBlock *bb, BasicBlock *root) {
    if (!bb) return;
    BasicBlock *ptr;
    vector_each3(&bb->doms, ptr) {
        if (ptr->nextblock == root || ptr->branchblock == root) {
            struct LoopNestingForest tmp = {0};
            tmp.head = root;
            tmp.back = ptr;
            vector_push_back(&LNFs, tmp);
            return;
        } else {
            Loopnestingforest2(fnc, ptr, root);
        }
    }
}
void Loopnestingforest(Function *fnc) {
    BasicBlock *ptr;
    LOOPALLBB({
        vector_each3(&bb->doms, ptr) {
            Loopnestingforest2(fnc,ptr,ptr);
}
})
}

vector_template(BasicBlock *, loopstack);
void findLoop(Function *fnc, m_bitset_ptr live, BasicBlock *head,
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

bool processLoopssa2(Function *fnc, uint32_t v, struct LoopNestingForest *ptr) {
    struct LoopNestingForest *looptmp;
    int                       i;
    vector_each_address(&LNFs, i, looptmp) {
        BasicBlock *bb;
        vector_each3(&looptmp->bb, bb) {
            size_t              i;
            struct instruction *inst = NULL;
            // 条件不用加入
            vector_each(&bb->inst, i, inst) {
                switch (inst->op) {
                    OP3_3AC_OPER
                    OP2_3AC
                    case IR_OP_ADDPTR:
                    case IR_OP_GETITEMPTR:
                    case IR_OP_GETPTR:
                        if (inst->ret.ty & IS_SSA && inst->ret.ssaind == v) {
                            *ptr = *looptmp;
                            return true;
                        }
                }
            }
        }
    }
    return false;
}
void porcessloop(Function *fnc) {
    size_t         i;
    liveIntervals *ll;
    vector_each(&allIntervals, i, ll) {
        size_t i;
        if (vector_len(&ll->LRs) == 0) continue;
        uint32_t v;
        vector_each(&ll->ssaind, i, v) {
            struct LoopNestingForest ptr;
            if (processLoopssa2(fnc, v, &ptr)) {
                ll->isLoop = true;
                instruction *ins = vector_first(&ptr.head->inst);
                instruction *ins1 = vector_get(&ptr.head->inst, 1);
                instruction *ins12 = vector_get(&ptr.head->inst, 2);
                size_t       begin = ins->n;
                size_t       end = vector_final(&ptr.back->inst)->n;
                if (vector_first(&ll->LRs)->begin > begin) {
                    vector_first(&ll->LRs)->begin = begin - 1;
                }
                if (vector_final(&ll->LRs)->end < end) {
                    vector_final(&ll->LRs)->end = end;
                }
            }
        }
    }
}
/**
 * @brief 线性寄存器分配
 * @param[in] *fnc
 */
void linearScanRegisterAllocPass(Function *fnc) {
    processsArrayPointer(fnc);

    return;
    Loopnestingforest(fnc);
    printBasicAllBlock(stdout, fnc);

    destroyIntervals(fnc);
    printBasicAllBlock(stdout, fnc);
    // BUG 数组参数ssa

    printBasicAllBlock(stdout, fnc);
    LOOPALLBB({LOOPBBALLINST({ inst->join = inst; })})
    vector_clear(&_calls);
    vector_clear(&allSpill);
    // replacePointer(fnc);
    genMoves(fnc);

    instructionNumbering(fnc);
    LOOPALLBB({ bitset_resize(bb->live, vector_len(&fnc->SSAValuePool)); })
    initIntervals(fnc);
    // printBasicAllBlock(stdout, fnc);

    buildIntervals(fnc);
    printBasicAllBlock(stdout, fnc);
    printIntervals(stderr, fnc);

    joinValue(fnc);
    printIntervals(stderr, fnc);
    // printBasicAllBlock(stdout, fnc);

    // printIntervals(stderr, fnc);
    LOOPALLBB({LOOPBBALLINST({
        inst->isSpill0 = 0;
        inst->isSpill1 = 0;
        inst->isSpill2 = 0;
        inst->isMem0 = 0;
        inst->isMem1 = 0;
        inst->isMem2 = 0;
    })})
    // printIntervals(stderr, fnc);
    // printCFG(fnc);
    preRegisterAllocation(fnc);
    printIntervals(stderr, fnc);
    processRet(fnc);

    initRegister(fnc);
    initFunctionCall(fnc);
    printIntervals(stderr, fnc);
    printIntervals2(stdout, fnc);

    __loop(fnc);
    instructionNumbering(fnc);
    ;
    linearScan(fnc);
    printIntervals(stderr, fnc);
    mappingAllRegister(fnc);
    printBasicAllBlock(stdout, fnc);

    // gen_function(fnc, fp);
    // processCon(fnc, fp);
}

typedef struct RVcon {
    size_t num;
    Value  v;
} RVcon;
vector_template(RVcon, _con);
// 处理字符串和小数字面量
void processCon(FILE *fp) {
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
extern int testNum;
void       gen_functionProlog(Function *fnc, FILE *fp) {
    // 多于参数
    size_t size;
    size_t rasize;
    if (1) {
        size = bytealigned(540000, 16);
        rasize = 53580;
    } else {
        size = 500;
        rasize = 400;
    }
    fprintf(fp, "mv      s0,sp\n");
    fprintf(fp, "li    t1, -%d\n", bytealigned(540000, 16));
    fprintf(fp, "add    sp, sp,t1\n");
    fprintf(fp, " mv    t3, sp\n");
    fprintf(fp, "li    t1, %d\n", bytealigned(535800, 16));
    fprintf(fp, "add    t1, sp,t1\n");
    fprintf(fp, "sd       ra, 0(t1)\n");
}
void gen_functionEpilogue(Function *fnc, FILE *fp) {
    size_t size;
    size_t rasize;
    if (1) {
        size = bytealigned(540000, 16);
        rasize = 535800;
    } else {
        size = 500;
        rasize = 400;
    }
    fprintf(fp, "li    t1, %d\n", bytealigned(535800, 16));
    fprintf(fp, "add    t1, sp,t1\n");
    fprintf(fp, "ld       ra, 0(t1)\n");
    fprintf(fp, "li    t1, %d\n", bytealigned(540000, 16));
    fprintf(fp, "add    sp, sp,t1\n");
    fprintf(fp, "ret\n");
}
asminstruction *asmlist;
void            asmgen_functionProlog(Function *fnc, FILE *fp) {
    asminstruction *asmptr;
    // 多于参数
    size_t          size;
    size_t          rasize;

    // fprintf(fp, "mv      s0,sp\n");
    asmptr = createAsm5(ASM_mv, REG_s0, REG_sp);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "li    t1, -%d\n", bytealigned(540000, 16));
    asmptr = createAsm4(ASM_li, REG_t1, 0);
    fnc->stackSize = &asmptr->src[0].imm;
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add    sp, sp,t1\n");
    asmptr = createAsm2(ASM_add, REG_sp, REG_sp, REG_t1);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, " mv    t3, sp\n");
    asmptr = createAsm5(ASM_mv, REG_t3, REG_sp);
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
}
void asmgen_functionEpilogue(Function *fnc, FILE *fp) {
    asminstruction *asmptr;
    size_t          size;
    size_t          rasize;

    fnc->loc1 += 1000;
    *fnc->raAddr = fnc->loc1 - 8;
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

typedef struct saveCallerList {
    int src, dest;
    int loc;
} saveCallerList;
vector_template(saveCallerList, _saveList);
static void genSaveCaller(Function *fnc, FILE *fp) {
    struct instruction *inst = NULL;
    vector_clear(&_saveList);
    vector_each3(&CurFncCall.insts1, inst) {
        if (inst->op != IR_OP_MOV) {
            assert(0);
        }
        // 这里是移入内存,没有实现
        if (inst->ret.ty & IS_POINTER || inst->args[0].ty & IS_POINTER)
            assert(0);
        fprintf(fp, "mv  %s, %s\n", regName[inst->ret.reg],
                regName[inst->args[0].reg]);
        saveCallerList tmp;
        tmp.dest = inst->ret.reg;
        tmp.src = inst->args[0].reg;
        vector_push_back(&_saveList, tmp);
    }
}
static void genrestoreCaller(Function *fnc, FILE *fp) {
    struct instruction *inst = NULL;
    vector_each3(&CurFncCall.insts2, inst) {
        if (inst->op != IR_OP_MOV) {
            assert(0);
        }
        if (inst->ret.ty & IS_POINTER || inst->args[0].ty & IS_POINTER)
            assert(0);
        fprintf(fp, "mv  %s, %s\n", regName[inst->ret.reg],
                regName[inst->args[0].reg]);
    }
}
// 处理溢出的寄存器
void processMemoryLoad(Function *fnc, FILE *fp, Reg r, Value *v) {
    fprintf(fp, "ld      %s, %d(t3)\n", regName[r], v->loc);
}
void processMemoryStore(Function *fnc, FILE *fp, Reg r, Value *v) {
    fprintf(fp, "sd      %s, %d(t3)\n", regName[r], v->loc);
}

void asmprocessMemoryLoad1(Function *fnc, FILE *fp, Reg r, Symbol *s1) {
    asminstruction *asmptr;
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    assert(s1->loc != 0);

    // fprintf(fp, "li      t2, %d\n", s1->loc);
    asmptr = createAsm4(ASM_li, REG_t2, s1->loc);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add      t2, t2,t3\n");
    asmptr = createAsm2(ASM_add, REG_t2, REG_t2, REG_t3);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "ld      %s, 0(t2)\n", regName[r]);
    if (s1->type & IS_POINTER || s1->type & IS_ARRAY) {
        asmptr = createAsm7(ASM_ld, r, REG_t2, 0);
    } else {
        asmptr = createAsm7(ASM_lw, r, REG_t2, 0);
    }
    DL_APPEND(asmlist, asmptr);
}

void asmloadImm(Function *fnc, FILE *fp, Reg r, Value *v) {
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
void asmprocessMemoryStore1(Function *fnc, FILE *fp, Reg r, Symbol *s1) {
    asminstruction *asmptr;
    // ssaSymbol *s1 = getSSAlVariable(fnc, v->ssaind);
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    // fprintf(fp, "li      t2, %d\n", s1->loc);
    asmptr = createAsm4(ASM_li, REG_t2, s1->loc);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add      t2, t2,t3\n");
    asmptr = createAsm2(ASM_add, REG_t2, REG_t2, REG_t3);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "sd      %s, 0(t2)\n", regName[r]);
    if (s1->type & IS_POINTER || s1->type & IS_ARRAY) {
        asmptr = createAsm7(ASM_sd, r, REG_t2, 0);

    } else {
        asmptr = createAsm7(ASM_sw, r, REG_t2, 0);
    }
    DL_APPEND(asmlist, asmptr);
}
void asmprocessFPMemoryLoad1(Function *fnc, FILE *fp, Reg r, Symbol *s1) {
    asminstruction *asmptr;
    if (s1->loc == 0) {
        assert(s1->loc != 0);
    }
    assert(s1->loc != 0);
    assert(r >= REG_fa0);
    // fprintf(fp, "li      t2, %d\n", s1->loc);
    asmptr = createAsm4(ASM_li, REG_t2, s1->loc);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add      t2, t2,t3\n");
    asmptr = createAsm2(ASM_add, REG_t2, REG_t2, REG_t3);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "flw      %s, 0(t2)\n", regName[r]);
    asmptr = createAsm7(ASM_flw, r, REG_t2, 0);
    DL_APPEND(asmlist, asmptr);
}
void asmprocessFPMemoryStore1(Function *fnc, FILE *fp, Reg r, Symbol *s1) {
    asminstruction *asmptr;
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    assert(r >= REG_fa0);
    // fprintf(fp, "li      t2, %d\n", s1->loc);
    asmptr = createAsm4(ASM_li, REG_t2, s1->loc);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "add      t2, t2,t3\n");
    asmptr = createAsm2(ASM_add, REG_t2, REG_t2, REG_t3);
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "fsw      %s, 0(t2)\n", regName[r]);
    asmptr = createAsm7(ASM_fsw, r, REG_t2, 0);
    DL_APPEND(asmlist, asmptr);
}
void processMemoryLoad1(Function *fnc, FILE *fp, Reg r, Symbol *s1) {
    // ssaSymbol *s1 = getSSAlVariable(fnc, v->ssaind);
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    assert(s1->loc != 0);
    fprintf(fp, "li t2,%d\n", s1->loc);
    fprintf(fp, "add t2,t2,t3\n");
    fprintf(fp, "ld %s,0(t2)\n", regName[r]);
}
void processMemoryStore1(Function *fnc, FILE *fp, Reg r, Symbol *s1) {
    // ssaSymbol *s1 = getSSAlVariable(fnc, v->ssaind);
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    fprintf(fp, "li t2,%d\n", s1->loc);
    fprintf(fp, "add t2,t2,t3\n");
    fprintf(fp, "sd %s,0(t2)\n", regName[r]);
    // fprintf(fp, "sd      %s, %d(t3)\n", regName[r], s1->loc);
}
void processFPMemoryLoad1(Function *fnc, FILE *fp, Reg r, Symbol *s1) {
    // ssaSymbol *s1 = getSSAlVariable(fnc, v->ssaind);
    if (s1->loc == 0) {
        assert(s1->loc != 0);
    }
    assert(s1->loc != 0);
    assert(r >= REG_fa0);
    fprintf(fp, "li      t2, %d\n", s1->loc);
    fprintf(fp, "add      t2, t2,t3\n");
    fprintf(fp, "flw      %s, 0(t2)\n", regName[r]);
}
void processFPMemoryStore1(Function *fnc, FILE *fp, Reg r, Symbol *s1) {
    // ssaSymbol *s1 = getSSAlVariable(fnc, v->ssaind);
    if (s1->loc == 0) {
        s1->loc = allocStack2(fnc, 8);
    }
    assert(r >= REG_fa0);
    fprintf(fp, "li      t2, %d\n", s1->loc);
    fprintf(fp, "add      t2, t2,t3\n");
    fprintf(fp, "fsw      %s, 0(t2)\n", regName[r]);
    // fprintf(fp, "sd      %s, %d(t3)\n", regName[r], s1->loc);
}
void asmloadSymbol(Function *fnc, FILE *fp, Reg r, Value *v) {
    asminstruction *asmptr;
    char           *label = getToken(getGlobalSymbol(v->symbol)->tok)->str;
    // fprintf(fp, "la     %s, .%s\n", regName[r], label);

    char            buff[1024];
    sprintf(buff, ".%s", label);
    asmptr = createAsm8(ASM_la, r, strdup(buff));
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp,
    //         "lui     %s, %%hi(%s)\n"
    //         "addi      %s, %s,%%lo(%s)\n",
    //         regName[r], label, regName[r], regName[r], label);
}
void loadSymbol(Function *fnc, FILE *fp, Reg r, Value *v) {
    char *label = getToken(getGlobalSymbol(v->symbol)->tok)->str;
    fprintf(fp, "la %s, .%s\n", regName[r], label);
    // fprintf(fp,
    //         "lui     %s, %%hi(%s)\n"
    //         "addi      %s, %s,%%lo(%s)\n",
    //         regName[r], label, regName[r], regName[r], label);
}

int m111122(Function *fnc, FILE *fp) {
    if (vector_len(&_calls) == 0) {
        callNum = false;
        CurFncCall.num = 0;
    } else {
        callNum = true;
        CurFncCall = vector_first(&_calls);
        vector_remove(&_calls, 0);
    }
    // 指令很有可能溢出了多个参数,因此统计溢出个数,3个溢出 add t0,t0,t1
    int         memNUM = 0;
    char       *str;
    size_t      i, paramNum = 0;
    int         overparammem = 0;
    BasicBlock *bb = NULL;
    bool        isRestore = false;
    bool        swap = false;

    // size_t             j;
    // spillInfo         *sp;
    // struct overParams *p;
    // vector_each_address(&_overp, j, p) {

    // }

    vector_each(&fnc->BBs, i, bb) {
        fprintf(fp, ".%s_bb_%d:\n", fnc->name, bb->bbName);
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->n == 19) {
                printf("");
            }
            fprintf(fp, "#");
            printIns(fp, fnc, inst);
            fprintf(fp, "\n");
            size_t             j;
            spillInfo         *sp;
            struct overParams *p;
            vector_each_address(&_overp, j, p) {
                liveIntervals *ll = getssaIntervals(p->v);
                if (ll->reg == -1) {
                    // 不曾获得过寄存器
                    assert(0);
                }
                if (inst->n == p->begin) {
                    if (p->ispointer) {
                        fprintf(fp, "ld      %s, %d(t3)\n", regName[ll->reg],
                                p->loc);
                    } else {
                        fprintf(fp, "lw      %s, %d(t3)\n", regName[ll->reg],
                                p->loc);
                    }
                }
            }
            vector_each_address(&allSpill, j, sp) {
                if (sp->isMoveMem && sp->n == inst->n) {
                    uint32_t v;
                    size_t   i;
                    fprintf(fp, "#spill reg:");
                    vector_each(&sp->ll->ssaind, i, v) {
                        ssaSymbol *sym = getSSAlVariable(fnc, v);
                        if (!sym->sym) {
                            fprintf(fp, "_%d", sym->valueind);
                        } else if (sym->sym->isTemp) {
                            fprintf(fp, "_%d", sym->sym->tmpInd);
                        } else {
                            fprintf(fp, "%s_%d", getToken(sym->sym->tok)->str,
                                    sym->valueind);
                        }
                        fprintf(fp, ",");
                    }
                    fprintf(fp, "\n");
                    fprintf(fp, "sd      %s, %d(sp)\n", regName[sp->r],
                            sp->loc);
                }
            }
            if (callNum && CurFncCall.num == inst->n) {
                genSaveCaller(fnc, fp);
                isRestore = true;
            }
            swap = false;
            fprintf(fp, "");
            switch (inst->op) {
                case IR_OP_ARGUMENT:
                    break;
                case IR_OP_NOT:
                    // seqz    a0, a0
                    if (inst->args[0].ty & IS_SSA) {
                        fprintf(fp, "seqz   %s, %s\n", regName[inst->ret.reg],
                                regName[inst->args[0].reg]);
                    } else if (ValueIsImm(&inst->args[0])) {
                        assert(0);
                    } else {
                        assert(0);
                    }
                    break;
                case IR_OP_NOTF:
                    assert(0);
                    break;
                    // // seqz    a0, a0
                    // if (inst->args[0].ty & IS_SSA) {
                    //     fprintf(fp, "seqz   %s, %s\n",
                    //     regName[inst->ret.reg],
                    //             regName[inst->args[0].reg]);
                    // } else if (ValueIsImm(&inst->args[0])) {
                    //     assert(0);
                    // } else {
                    //     assert(0);
                    // }
                    // break;
                    // negw    a0, a0
                case IR_OP_NEG:
                    if (inst->args[0].ty & IS_SSA) {
                        fprintf(fp, "negw   %s, %s\n", regName[inst->ret.reg],
                                regName[inst->args[0].reg]);
                    } else if (ValueIsImm(&inst->args[0])) {
                        assert(0);
                    } else {
                        assert(0);
                    }
                    break;
                //  fneg.s  fa0, fa0
                case IR_OP_NEGF:
                    if (inst->args[0].ty & IS_SSA) {
                        fprintf(fp, "fneg.s   %s, %s\n", regName[inst->ret.reg],
                                regName[inst->args[0].reg]);
                    } else if (ValueIsImm(&inst->args[0])) {
                        assert(0);
                    } else {
                        assert(0);
                    }
                    break;

                case IR_OP_MOV:
                    if (ValueIsImm(&inst->args[0])) {
                        assert(inst->args[0].ty & TY_INT);
                        // BUG 小数
                        if (inst->isMem0) {
                            loadImm(fnc, fp, REG_CON, &inst->args[0]);
                            processMemoryStore(fnc, fp, REG_CON, &inst->ret);
                        } else {
                            loadImm(fnc, fp, inst->ret.reg, &inst->args[0]);
                        }
                    } else if (inst->args[0].ty & IS_SSA) {
                        if (inst->isMem1 && inst->isMem0) {
                            processMemoryLoad(fnc, fp, REG_SPILL,
                                              &inst->args[0]);
                            processMemoryStore(fnc, fp, REG_SPILL, &inst->ret);
                        } else {
                            int r1, r0;
                            if (inst->isMem1) {
                                processMemoryLoad(fnc, fp, REG_SPILL,
                                                  &inst->args[0]);
                                r1 = REG_SPILL;
                            } else {
                                r1 = inst->args[0].reg;
                            }
                            if (inst->isMem0) {
                                processMemoryStore(fnc, fp, r1, &inst->ret);
                            } else {
                                r0 = inst->ret.reg;
                                fprintf(fp, "mv  %s, %s\n", regName[r0],
                                        regName[r1]);
                            }
                        }
                    } else {
                        assert(0);
                    }
                    break;
                    // src操作数：ssa,立即数;dest操作数：ssa,立即数
                case IR_OP_NE:
                    str = "beq";
                    swap = false;
                    goto _cond;
                case IR_OP_EQ:
                    str = "bne";
                    swap = false;
                    goto _cond;
                case IR_OP_GT:
                    str = "blt";
                    // str = "bge";
                    // swap = true;
                    goto _cond;
                case IR_OP_GE:
                    str = "ble";
                    swap = false;
                    goto _cond;
                case IR_OP_LT:
                    str = "bgt";
                    swap = false;
                    goto _cond;
                case IR_OP_LE:
                    str = "bge";
                    swap = false;
                _cond : {
                    int l = 0, r = 1;
                    int r1, r2;
                    if (swap) {
                        r = 0;
                        l = 1;
                    }
                    memNUM = 0;
                    if (inst->args[0].ty & IS_IMM &&
                        inst->args[1].ty & IS_IMM) {
                        loadImm(fnc, fp, REG_SPILL, &inst->args[0]);
                        loadImm(fnc, fp, REG_SPILL2, &inst->args[1]);
                        r1 = REG_SPILL;
                        r2 = REG_SPILL2;
                        goto _cond1;
                    }
                    if (inst->isMem1) {
                        processMemoryLoad(fnc, fp, REG_SPILL, &inst->args[0]);
                        memNUM++;
                        r1 = REG_SPILL;
                    } else if (ValueIsImm(&inst->args[0])) {
                        r1 = REG_CON;
                        loadImm(fnc, fp, REG_CON, &inst->args[0]);

                    } else {
                        r1 = inst->args[0].reg;
                    }
                    if (inst->isMem2) {
                        if (memNUM == 0) {
                            processMemoryLoad(fnc, fp, REG_SPILL,
                                              &inst->args[1]);
                            r2 = REG_SPILL;
                        } else {
                            processMemoryLoad(fnc, fp, REG_SPILL2,
                                              &inst->args[1]);
                            r2 = REG_SPILL2;
                        }
                        memNUM++;
                    } else if (ValueIsImm(&inst->args[1])) {
                        r2 = REG_CON;
                        loadImm(fnc, fp, REG_CON, &inst->args[1]);
                    } else {
                        r2 = inst->args[1].reg;
                    }
                    if (swap) {
                        int tmp = r1;
                        r1 = r2;
                        r2 = tmp;
                    }
                _cond1:
                    fprintf(fp, "%s   %s, %s,.%s_bb_%d\n", str, regName[r1],
                            regName[r2], fnc->name, bb->branchblock->bbName);
                    fprintf(fp, "J .%s_bb_%d\n", fnc->name,
                            bb->nextblock->bbName);
                    memNUM = 0;
                }
                // if (inst->args[1].ty & IS_SSA &&
                //     inst->args[0].ty & IS_SSA) {
                //     fprintf(fp, "bne   %s, %s,.%s_bb_%d\n",
                //             regName[inst->args[0].reg],
                //             regName[inst->args[1].reg]);
                // } else if (ValueIsImm(&inst->args[0]) &&
                //            ValueIsImm(&inst->args[1])) {
                //     assert(0);
                // } else if (ValueIsImm(&inst->args[0]) ||
                //            ValueIsImm(&inst->args[1])) {
                //     if (ValueIsImm(&inst->args[0])) {
                //         fprintf(fp, "addi   %s, zero, %d\n",
                //                 regName[inst->ret.reg],
                //                 getImmIntValue(&inst->args[0]));
                //         fprintf(fp, "bne   %s, %s,.%s_bb_%d\n",
                //                 regName[inst->ret.reg],
                //                 regName[inst->args[1].reg], fnc->name,
                //                 bb->nextblock->bbName);
                //     } else if (ValueIsImm(&inst->args[1])) {
                //         fprintf(fp, "addi   %s, zero, %d\n",
                //                 regName[inst->ret.reg],
                //                 getImmIntValue(&inst->args[1]));
                //         fprintf(fp, "bne   %s, %s,.%s_bb_%d\n",
                //                 regName[inst->args[0].reg],
                //                 regName[inst->ret.reg], fnc->name,
                //                 bb->nextblock->bbName);
                //     } else {
                //         assert(0);
                //     }
                //     fprintf(fp, "J .%s_bb_%d\n", fnc->name,
                //             bb->branchblock->bbName);
                // } else {
                //     assert(0);
                // }
                break;
                case IR_OP_ADD:
                    str = "add";
                    goto _binary1;
                case IR_OP_SUB:
                    str = "sub";
                    goto _binary1;
                case IR_OP_MUL:
                    str = "mul";
                    goto _binary1;
                case IR_OP_DIV:
                    str = "div";
                    goto _binary1;
                case IR_OP_MOD:
                    str = "rem";
                _binary1:
                    if (inst->args[0].ty & IS_IMM && inst->args[1].ty & IS_IMM)
                        assert(0);
                    {
                        int r1, r2, r0;
                        memNUM = 0;
                        if (inst->isMem1) {
                            processMemoryLoad(fnc, fp, REG_SPILL,
                                              &inst->args[0]);
                            memNUM++;
                            r1 = REG_SPILL;
                        } else if (ValueIsImm(&inst->args[0])) {
                            r1 = REG_CON;
                            loadImm(fnc, fp, REG_CON, &inst->args[0]);

                        } else {
                            r1 = inst->args[0].reg;
                        }
                        if (inst->isMem2) {
                            if (memNUM == 0) {
                                processMemoryLoad(fnc, fp, REG_SPILL,
                                                  &inst->args[1]);
                                r2 = REG_SPILL;
                            } else {
                                processMemoryLoad(fnc, fp, REG_SPILL2,
                                                  &inst->args[1]);
                                r2 = REG_SPILL2;
                            }
                            memNUM++;
                        } else if (ValueIsImm(&inst->args[1])) {
                            r2 = REG_CON;
                            loadImm(fnc, fp, REG_CON, &inst->args[1]);
                        } else {
                            r2 = inst->args[1].reg;
                        }

                        if (inst->isMem0) {
                            r0 = REG_SPILL;
                        } else {
                            r0 = inst->ret.reg;
                        }

                        // 考虑ret溢出
                        fprintf(fp, "%s   %s, %s, %s\n", str, regName[r0],
                                regName[r1], regName[r2]);
                        if (inst->isMem0) {
                            processMemoryStore(fnc, fp, REG_SPILL, &inst->ret);
                        }
                        memNUM = 0;
                    }
                    break;
                case IR_OP_RET:
                    fprintf(fp, "J .%s_ret\n", fnc->name);
                    break;
                case IR_OP_RETI:
                    // ret
                    // 指令会在寄存器分配中分配a0,如果被溢出了需要重新加载
                    if (inst->isMem0) {
                        processMemoryLoad(fnc, fp, REG_RET, &inst->ret);
                        // assert(0);
                    } else if (ValueIsImm(&inst->ret)) {
                        loadImm(fnc, fp, REG_a0, &inst->ret);
                    } else if (inst->ret.ty & IS_SSA) {
                        fprintf(fp, "mv    a0, %s\n", regName[inst->ret.reg]);
                    } else {
                        assert(0);
                    }
                    fprintf(fp, "J .%s_ret\n", fnc->name);
                    break;
                case IR_OP_RETF:
                    // BUG 没有生成函数序言结语
                    if (inst->ret.ty & IS_SSA) {
                        assert(0);
                    } else if (ValueIsImm(&inst->ret)) {
                        assert(0);
                    } else {
                        assert(0);
                    }
                    fprintf(fp, "J .%s_ret\n", fnc->name);
                    break;
                case IR_OP_ADDPTR: {
                    int r0, r1, r2, memNUM;
                    memNUM = 0;
                    if (inst->isMem2) {
                        processMemoryLoad(fnc, fp, REG_SPILL, &inst->args[1]);
                        memNUM++;
                        r2 = REG_SPILL;
                    } else if (ValueIsImm(&inst->args[1])) {
                        r2 = REG_CON;
                        loadImm(fnc, fp, REG_CON, &inst->args[1]);
                    } else {
                        r2 = inst->args[1].reg;
                    }

                    if (inst->isMem1) {
                        if (memNUM == 0) {
                            processMemoryLoad(fnc, fp, REG_SPILL,
                                              &inst->args[0]);
                            r1 = REG_SPILL;
                        } else {
                            processMemoryLoad(fnc, fp, REG_SPILL2,
                                              &inst->args[0]);
                            r1 = REG_SPILL2;
                        }
                        memNUM++;
                    } else if (ValueIsImm(&inst->args[0])) {
                        assert(0);
                    } else {
                        r1 = inst->args[0].reg;
                    }

                    if (inst->isMem0) {
                        r0 = REG_SPILL;
                    } else {
                        r0 = inst->ret.reg;
                    }
                    // 全局符号
                    if (inst->args[0].ty & IS_GLOBAL &&
                        inst->args[0].ty & IS_ARRAY) {
                        char *label;
                        label =
                            getToken(getGlobalSymbol(inst->args[0].symbol)->tok)
                                ->str;
                        fprintf(fp, "la     %s, .%s\n", regName[r0], label);
                        // fprintf(fp,
                        //         "lui     %s, %%hi(%s)\n"
                        //         "addi      %s, %s,%%lo(%s)\n",
                        //         // addi    a0,a0,%lo(a1)
                        //         regName[r0], label, regName[r0], regName[r0],
                        //         label);
                        fprintf(fp, "add   %s, %s, %s\n", regName[r0],
                                regName[r0], regName[r2]);
                    } else if (inst->args[0].ty & IS_ARRAY) {
                        // 局部数组
                        Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                        if (s->loc == 0) {
                            s->loc =
                                allocStack2(fnc, getArraySize2(s->arrayTy));
                        }
                        // BUG
                        // addi    a5,sp,-16
                        fprintf(fp, "addi   %s, sp, -%d\n", regName[r0],
                                s->loc);
                        fprintf(fp, "add   %s, %s, %s\n", regName[r0],
                                regName[r0], regName[r2]);
                    } else if (inst->args[0].ty & IS_POINTER &&
                               inst->args[0].ty & IS_SSA) {
                        // 数组寻址的临时指针
                        fprintf(fp, "add   %s, %s, %s\n", regName[r0],
                                regName[r1], regName[r2]);
                    } else {
                        assert(0);
                    }
                    if (inst->isMem0) {
                        processMemoryStore(fnc, fp, REG_SPILL, &inst->ret);
                    }
                }

                break;
                case IR_OP_SOTRE: {
                    // lui     a0,%hi(b)
                    // lw      a0, %lo(b)(a0)
                    // 加载了一个32位的值,int类型
                    int r0, r1, m;
                    m = 0;
                    if (inst->isMem1) {
                        processMemoryLoad(fnc, fp, REG_SPILL, &inst->args[0]);
                        r1 = REG_SPILL;
                        m++;
                    } else if (ValueIsImm(&inst->args[0])) {
                        r1 = REG_CON;
                        loadImm(fnc, fp, REG_CON, &inst->args[0]);
                    } else {
                        r1 = inst->args[0].reg;
                    }
                    if (inst->isMem0) {
                        if (m != 0) {
                            r0 = REG_SPILL2;

                        } else {
                            r0 = REG_SPILL;
                        }
                    } else {
                        r0 = inst->ret.reg;
                    }
                    // 全局标量
                    if (inst->ret.ty & IS_GLOBAL) {
                        loadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                        fprintf(fp, "sw   %s, 0(%s)\n", regName[r1],
                                regName[REG_SPILL]);
                        break;
                    }
                    if (!(inst->ret.ty & IS_ARRAY) &&
                        (inst->ret.ty & IS_POINTER) &&
                        (inst->ret.ty & IS_GLOBAL)) {
                        loadSymbol(fnc, fp, r0, &inst->ret);
                        fprintf(fp, "sw   %s, 0(%s)\n", regName[r1],
                                regName[r0]);
                    } else {
                        fprintf(fp, "sw   %s, 0(%s)\n", regName[r1],
                                regName[r0]);
                    }
                    // 考虑ret溢出
                }

                break;
                case IR_OP_LOAD:
                    // 局部地址和全局地址的加载貌似是不一样的
                    {
                        // lui     a0,%hi(b)
                        // lw      a0, %lo(b)(a0)
                        // 加载了一个32位的值,int类型
                        int r0, r1;

                        if (inst->isMem1) {
                            processMemoryLoad(fnc, fp, REG_SPILL,
                                              &inst->args[0]);
                            r1 = REG_SPILL;
                        } else {
                            r1 = inst->args[0].reg;
                        }
                        if (inst->isMem0) {
                            r0 = REG_SPILL;
                        } else {
                            r0 = inst->ret.reg;
                        }
                        // 全局标量,int a;
                        if (!(inst->args[0].ty & IS_ARRAY) &&
                            (inst->args[0].ty & IS_POINTER) &&
                            (inst->args[0].ty & IS_GLOBAL)) {
                            loadSymbol(fnc, fp, r0, &inst->args[0]);
                            fprintf(fp, "lw   %s, 0(%s)\n", regName[r0],
                                    regName[r0]);
                        } else {
                            fprintf(fp, "lw   %s, 0(%s)\n", regName[r0],
                                    regName[r1]);
                        }

                        // 考虑ret溢出

                        if (inst->isMem0) {
                            processMemoryStore(fnc, fp, REG_SPILL, &inst->ret);
                        }
                    }
                    break;
                case IR_OP_IF:
                case IR_OP_PHI:
                    break;
                case IR_OP_GETITEMPTR: {
                    ArrayType *arrayTy;
                    // 数组指针int fnc(int a[]);
                    if (inst->args[0].ty & IS_SSA) {
                        ssaSymbol *s =
                            getSSAlVariable(fnc, inst->args[0].ssaind);
                        if (inst->args[0].ty & IS_GLOBAL) assert(0);
                        assert(s->arrayTy);
                        fprintf(fp, "mv    %s, %s\n", regName[REG_SPILL],
                                regName[inst->args[0].reg]);
                        arrayTy = s->arrayTy;
                    } else {
                        Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                        if (inst->args[0].ty & IS_GLOBAL) assert(0);
                        if (!s->isalloc) {
                            s->isalloc = 1;
                            s->loc =
                                allocStack2(fnc, getArraySize2(s->arrayTy));
                            if (s->loc > 1000) {
                                allocStack2(fnc, getArraySize2(s->arrayTy));
                                printf("\n%d", getArraySize2(s->arrayTy));
                            }
                        }
                        fprintf(fp, "addi    %s, t3,%d\n", regName[REG_SPILL],
                                s->loc);
                        arrayTy = s->arrayTy;
                    }
                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     j;

                    // 考虑常量溢出

                    int        sum = 0;
                    vector_each_address(&ptr->index, j, index) {
                        if (arrayTy->next) {
                            sum = arrayTy->next->size * 4;
                        } else {
                            sum = 4;
                        }
                        arrayTy = arrayTy->next;
                        if (index->ty & IS_IMM) {
                            fprintf(fp, "#%d:[%d] \n", j, index->imm.i);
                            sum *= index->imm.i;
                            Value v = SET_IMM_INT(sum);
                            loadImm(fnc, fp, REG_CON, &v);
                            fprintf(fp, "add    %s,%s,%s\n", regName[REG_SPILL],
                                    regName[REG_SPILL], regName[REG_CON]);
                        } else {
                            // 没有考虑溢出啊
                            fprintf(fp, "#%d:[v] \n", j);
                            Value v = SET_IMM_INT(sum);
                            loadImm(fnc, fp, REG_CON, &v);
                            fprintf(fp, "mul    %s,%s,%s\n", regName[REG_CON],
                                    regName[index->reg], regName[REG_CON]);
                            fprintf(fp, "add    %s,%s,%s\n", regName[REG_SPILL],
                                    regName[REG_SPILL], regName[REG_CON]);
                        }
                    }
                    if (inst->isMem0) {
                        assert(0);
                    } else {
                        fprintf(fp, "mv    %s,%s\n", regName[inst->ret.reg],
                                regName[REG_SPILL]);
                    }
                } break;

                case IR_OP_PARAM:
                    // 数组没有做
                    if (inst->ret.ty & IS_SSA) {
                        if (inst->isMem0) {
                            loadReg(fnc, fp, functionArgument[paramNum],
                                    inst->isMem0, &inst->ret);
                        } else {
                            saveCallerList list;
                            vector_each3(&_saveList, list) {
                                // 没有实现内存
                                if (list.src == inst->ret.reg) {
                                    inst->ret.reg = list.dest;
                                    loadReg(fnc, fp, functionArgument[paramNum],
                                            inst->isMem0, &inst->ret);
                                    goto _succ;
                                }
                            }
                            assert(0);
                        _succ:
                            NULL;
                        }

                        paramNum++;
                        // ssa 标量已经被分配过寄存器了
                    } else if (inst->ret.ty & TY_VOID &&
                               inst->ret.ty & IS_POINTER) {
                        int   reg = functionArgument[paramNum];
                        // TY_VOID | IS_POINTER是字符串
                        RVcon con;
                        con.num = vector_len(&_con);
                        con.v = inst->ret;
                        vector_push_back(&_con, con);
                        // lui     a0, %hi(.L.str)
                        // addi    a0, a0, %lo(.L.str)
                        fprintf(fp, "la     %s, ._str%d\n", regName[reg],
                                con.num);
                        // fprintf(fp,
                        //         "lui     %s, %%hi(._str%d)\n"
                        //         "addi      %s,%s, %%lo(._str%d)\n",
                        //         regName[reg], con.num, regName[reg],
                        //         regName[reg], con.num);
                        //, _con
                    } else if (ValueIsImm(&inst->ret)) {
                        // 立即数参数
                        if (paramNum >= 8) {
                            if (ValueIsInt(&inst->ret)) {
                                loadImm(fnc, fp, REG_CON, &inst->ret);
                                fprintf(fp, "sd      %s,%d(sp)\n",
                                        regName[REG_CON], overparammem);
                                overparammem += 4;
                            } else {
                                assert(0);
                            }
                        } else if (ValueIsInt(&inst->ret)) {
                            Reg reg = functionArgument[paramNum];
                            loadImm(fnc, fp, reg, &inst->ret);
                        } else if (ValueIsFloat(&inst->ret)) {
                            Reg reg = FPfunctionArgument[paramNum];
                            loadFPImm(fnc, fp, reg, &inst->ret);
                        }
                    } else if ((inst->ret.ty & IS_ARRAY) &&
                               !(inst->ret.ty & IS_GLOBAL)) {
                        // 处理数组,但处理数组指针是上面
                        Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
                        fprintf(fp, "addi    %s, t3,%d\n",
                                regName[functionArgument[paramNum]], s->loc);
                    } else {
                        assert(0);
                    }
                    paramNum++;
                    break;
                case IR_OP_CALL: {
                    // 这里会导致局部申请很大,因为多余传入的参数没有考虑回收
                    allocStack2(fnc, overparammem);
                    overparammem = 0;
                    paramNum = 0;
                    Symbol *s = getGlobalSymbol(inst->args[0].symbol);
                    fprintf(fp, "call	%s\n", getToken(s->tok)->str);
                    if (isRestore) {
                        isRestore = false;
                        genrestoreCaller(fnc, fp);
                        if (vector_len(&_calls) == 0) {
                            CurFncCall.num = 0;
                        } else {
                            CurFncCall = vector_first(&_calls);
                            vector_remove(&_calls, 0);
                        }
                    }
                    if (inst->ret.ty & TY_VOID) break;
                    // return value
                    if (inst->isMem0) {
                        assert(0);
                    } else if (inst->isSpill0) {
                        assert(0);

                    } else {
                        if (ValueIsFloat(&inst->ret)) {
                            assert(0);
                        } else {
                            fprintf(fp, "mv  %s, a0\n", regName[inst->ret.reg]);
                        }
                    }
                    fprintf(fp, "mv  t3, sp\n", regName[inst->ret.reg]);
                    break;
                }

                default:
                    assert(0);
            };
        };
        if (bb->nextblock && bb->branchblock) {
        } else if (bb->nextblock) {
            fprintf(fp, "");
            fprintf(fp, "J .%s_bb_%d\n", fnc->name, bb->nextblock->bbName);
        } else if ((bb->nextblock == NULL) && (bb->branchblock == NULL)) {
            fprintf(fp, "J .%s_ret\n", fnc->name);
        } else {
            assert(0);
            // BUG 多写入了一次ret
            //  function end
        }
    }
    fprintf(fp, ".%s_ret:\n", fnc->name);
    restoreCallee(fnc, fp);
    gen_functionEpilogue(fnc, fp);
    fprintf(fp, "\n");
}
// 函数的返回值如果不是void，就可以分配rax, a= fnc()
// 优化叶子函数

// phi参数和值也有可能是其他phi函数的值
void spreadPhi(Function *fnc, Symbol *s, int loc);
void processPhi(Function *fnc) {
    ssaSymbol *s0, *s1, *s2, *s4;
    LOOPALLBB({LOOPBBALLINST({
        if (inst->op == IR_OP_PHI) {
            phiFunction *phi = inst->args[0].phi;
            phiParam    *p;
            size_t       i;

            s1 = getSSAlVariable(fnc, inst->ret.ssaind);
            if (s1->loc != 0) continue;
            s1->loc = allocStack2(fnc, 8);
            spreadPhi(fnc, s1->sym, s1->loc);
            int loc = s1->loc;
            vector_each_address(&phi->param, i, p) {
                s1 = getSSAlVariable(fnc, p->v.ssaind);
                s1->loc = loc;
            }
        }
    })})
}

void spreadPhi(Function *fnc, Symbol *s, int loc) {
    ssaSymbol *s0, *s1, *s2, *s4;
    LOOPALLBB({LOOPBBALLINST({
        if (inst->op == IR_OP_PHI) {
            phiFunction *phi = inst->args[0].phi;
            phiParam    *p;
            size_t       i;

            s1 = getSSAlVariable(fnc, inst->ret.ssaind);
            if (s1->sym == s) {
                s1->loc = loc;
                vector_each_address(&phi->param, i, p) {
                    s1 = getSSAlVariable(fnc, p->v.ssaind);
                    if (s1->loc == 0) {
                        s1->loc = loc;
                    }
                }
            }
        }
    })})
}

int m1111SSA(Function *fnc, FILE *fp) {
    fnc->loc1 += 4;
    processPhi(fnc);
    // 指令很有可能溢出了多个参数,因此统计溢出个数,3个溢出 add t0,t0,t1
    int         memNUM = 0;
    char       *str;
    size_t      i, paramNum = 0;
    int         overparammem = 0;
    BasicBlock *bb = NULL;
    bool        isRestore = false;
    bool        swap = false;
    Value       tmpv;
    int         argumentCount = 0;

    vector_each(&fnc->BBs, i, bb) {
        fprintf(fp, ".%s_bb_%d:\n", fnc->name, bb->bbName);
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->n == 19) {
                printf("");
            }
            fprintf(fp, "#");
            printIns(fp, fnc, inst);
            fprintf(fp, "\n");

            if (callNum && CurFncCall.num == inst->n) {
                genSaveCaller(fnc, fp);
                isRestore = true;
            }
            swap = false;
            fprintf(fp, "");
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
            switch (inst->op) {
                case IR_OP_ARGUMENT: {
                    if (argumentCount >= 8) assert(0);
                    if (inst->ret.ty & IS_POINTER) {
                        s0->loc = allocStack2(fnc, 8);
                        functionArgument[argumentCount];
                        processMemoryStore1(
                            fnc, fp, functionArgument[argumentCount], s0);
                    } else {
                        s0->loc = allocStack2(fnc, 8);
                        functionArgument[argumentCount];
                        processMemoryStore1(
                            fnc, fp, functionArgument[argumentCount], s0);
                    }
                    argumentCount++;
                } break;
                case IR_OP_NOT:
                    assert(0);
                    // seqz    a0, a0
                    if (inst->args[0].ty & IS_SSA) {
                        fprintf(fp, "seqz   %s, %s\n", regName[inst->ret.reg],
                                regName[inst->args[0].reg]);
                    } else if (ValueIsImm(&inst->args[0])) {
                        assert(0);
                    } else {
                        assert(0);
                    }
                    break;
                case IR_OP_NOTF:
                    assert(0);
                    break;
                    // // seqz    a0, a0
                    // if (inst->args[0].ty & IS_SSA) {
                    //     fprintf(fp, "seqz   %s, %s\n",
                    //     regName[inst->ret.reg],
                    //             regName[inst->args[0].reg]);
                    // } else if (ValueIsImm(&inst->args[0])) {
                    //     assert(0);
                    // } else {
                    //     assert(0);
                    // }
                    // break;
                    // negw    a0, a0
                case IR_OP_NEG:
                    assert(0);
                    if (inst->args[0].ty & IS_SSA) {
                        fprintf(fp, "negw   %s, %s\n", regName[inst->ret.reg],
                                regName[inst->args[0].reg]);
                    } else if (ValueIsImm(&inst->args[0])) {
                        assert(0);
                    } else {
                        assert(0);
                    }
                    break;
                //  fneg.s  fa0, fa0
                case IR_OP_NEGF:
                    assert(0);
                    if (inst->args[0].ty & IS_SSA) {
                        fprintf(fp, "fneg.s   %s, %s\n", regName[inst->ret.reg],
                                regName[inst->args[0].reg]);
                    } else if (ValueIsImm(&inst->args[0])) {
                        assert(0);
                    } else {
                        assert(0);
                    }
                    break;

                case IR_OP_MOV:
                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_SSA) {
                        if (isPHIparam1(fnc, &inst->args[0], &tmpv)) {
                            ssaSymbol *s4 = getSSAlVariable(fnc, tmpv.ssaind);
                            processMemoryLoad1(fnc, fp, REG_a0, s4);
                        } else {
                            if (s1->loc == 0) {
                                s1->loc = allocStack2(fnc, 8);
                            }
                            processMemoryLoad1(fnc, fp, REG_a0, s1);
                        }
                        // processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }

                    if (isPHIparam1(fnc, &inst->ret, &tmpv)) {
                        s4 = getSSAlVariable(fnc, tmpv.ssaind);
                        processMemoryStore1(fnc, fp, REG_a0, s4);
                    } else {
                        processMemoryStore1(fnc, fp, REG_a0, s0);
                    }
                    break;
                    // src操作数：ssa,立即数;dest操作数：ssa,立即数
                case IR_OP_NE:
                    str = "beq";
                    swap = false;
                    goto _cond;
                case IR_OP_EQ:
                    str = "bne";
                    swap = false;
                    goto _cond;
                case IR_OP_GT:
                    str = "blt";
                    // str = "bge";
                    // swap = true;
                    goto _cond;
                case IR_OP_GE:
                    str = "ble";
                    swap = false;
                    goto _cond;
                case IR_OP_LT:
                    str = "bgt";
                    swap = false;
                    goto _cond;
                case IR_OP_LE:
                    str = "bge";
                    swap = false;
                _cond : {
                    int l = 0, r = 1;
                    int r1, r2;
                    if (swap) {
                        r = 0;
                        l = 1;
                    }
                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_SSA) {
                        processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        loadImm(fnc, fp, REG_a1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_SSA) {
                        processMemoryLoad1(fnc, fp, REG_a1, s2);
                    }
                    r1 = REG_a0;
                    r2 = REG_a1;
                _cond1:
                    fprintf(fp, "%s   %s, %s,.%s_bb_%d\n", str, regName[r1],
                            regName[r2], fnc->name, bb->branchblock->bbName);
                    fprintf(fp, "J .%s_bb_%d\n", fnc->name,
                            bb->nextblock->bbName);
                } break;
                case IR_OP_ADD:
                    str = "add";
                    goto _binary1;
                case IR_OP_SUB:
                    str = "sub";
                    goto _binary1;
                case IR_OP_MUL:
                    str = "mul";
                    goto _binary1;
                case IR_OP_DIV:
                    str = "div";
                    goto _binary1;
                case IR_OP_MOD:
                    str = "rem";
                _binary1 : {
                    int r1, r2;
                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_SSA) {
                        processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        loadImm(fnc, fp, REG_a1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_SSA) {
                        processMemoryLoad1(fnc, fp, REG_a1, s2);
                    }
                    r1 = REG_a0;
                    r2 = REG_a1;

                    fprintf(fp, "%s   %s, %s, %s\n", str, regName[r1],
                            regName[r1], regName[r2]);

                    if (isPHIparam1(fnc, &inst->ret, &tmpv)) {
                        s4 = getSSAlVariable(fnc, tmpv.ssaind);
                        processMemoryStore1(fnc, fp, REG_a0, s4);
                    } else {
                        processMemoryStore1(fnc, fp, REG_a0, s0);
                    }
                } break;
                case IR_OP_RET:
                    fprintf(fp, "J .%s_ret\n", fnc->name);
                    break;
                case IR_OP_RETI:
                    // ret
                    // 指令会在寄存器分配中分配a0,如果被溢出了需要重新加载

                    if (ValueIsImm(&inst->ret)) {
                        loadImm(fnc, fp, REG_a0, &inst->ret);
                    } else if (inst->ret.ty & IS_SSA) {
                        processMemoryLoad1(fnc, fp, REG_a0, s0);
                    }
                    fprintf(fp, "mv    a0, %s\n", regName[REG_a0]);
                    fprintf(fp, "J .%s_ret\n", fnc->name);
                    break;
                case IR_OP_RETF:
                    // BUG 没有生成函数序言结语
                    assert(0);
                    if (inst->ret.ty & IS_SSA) {
                        assert(0);
                    } else if (ValueIsImm(&inst->ret)) {
                        assert(0);
                    } else {
                        assert(0);
                    }
                    fprintf(fp, "J .%s_ret\n", fnc->name);
                    break;
                case IR_OP_ADDPTR: {
                    assert(0);
                    int r0, r1, r2, memNUM;
                    memNUM = 0;
                    if (inst->isMem2) {
                        processMemoryLoad(fnc, fp, REG_SPILL, &inst->args[1]);
                        memNUM++;
                        r2 = REG_SPILL;
                    } else if (ValueIsImm(&inst->args[1])) {
                        r2 = REG_CON;
                        loadImm(fnc, fp, REG_CON, &inst->args[1]);
                    } else {
                        r2 = inst->args[1].reg;
                    }

                    if (inst->isMem1) {
                        if (memNUM == 0) {
                            processMemoryLoad(fnc, fp, REG_SPILL,
                                              &inst->args[0]);
                            r1 = REG_SPILL;
                        } else {
                            processMemoryLoad(fnc, fp, REG_SPILL2,
                                              &inst->args[0]);
                            r1 = REG_SPILL2;
                        }
                        memNUM++;
                    } else if (ValueIsImm(&inst->args[0])) {
                        assert(0);
                    } else {
                        r1 = inst->args[0].reg;
                    }

                    if (inst->isMem0) {
                        r0 = REG_SPILL;
                    } else {
                        r0 = inst->ret.reg;
                    }
                    // 全局符号
                    if (inst->args[0].ty & IS_GLOBAL &&
                        inst->args[0].ty & IS_ARRAY) {
                        char *label;
                        label =
                            getToken(getGlobalSymbol(inst->args[0].symbol)->tok)
                                ->str;
                        fprintf(fp,
                                "lui     %s, %%hi(%s)\n"
                                "addi      %s, %s,%%lo(%s)\n",
                                // addi    a0,a0,%lo(a1)
                                regName[r0], label, regName[r0], regName[r0],
                                label);
                        fprintf(fp, "add   %s, %s, %s\n", regName[r0],
                                regName[r0], regName[r2]);
                    } else if (inst->args[0].ty & IS_ARRAY) {
                        // 局部数组
                        Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                        if (s->loc == 0) {
                            s->loc =
                                allocStack2(fnc, getArraySize2(s->arrayTy) * 2);
                        }
                        // BUG
                        // addi    a5,sp,-16
                        fprintf(fp, "addi   %s, sp, -%d\n", regName[r0],
                                s->loc);
                        fprintf(fp, "add   %s, %s, %s\n", regName[r0],
                                regName[r0], regName[r2]);
                    } else if (inst->args[0].ty & IS_POINTER &&
                               inst->args[0].ty & IS_SSA) {
                        // 数组寻址的临时指针
                        fprintf(fp, "add   %s, %s, %s\n", regName[r0],
                                regName[r1], regName[r2]);
                    } else {
                        assert(0);
                    }
                    if (inst->isMem0) {
                        processMemoryStore(fnc, fp, REG_SPILL, &inst->ret);
                    }
                }

                break;
                case IR_OP_SOTRE: {
                    // lui     a0,%hi(b)
                    // lw      a0, %lo(b)(a0)
                    // 加载了一个32位的值,int类型

                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_SSA) {
                        processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }

                    // 全局标量
                    if (inst->ret.ty & IS_GLOBAL) {
                        loadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                        fprintf(fp, "sw   %s, 0(%s)\n", regName[REG_a0],
                                regName[REG_SPILL]);
                        break;
                    }
                    if (!(inst->ret.ty & IS_ARRAY) &&
                        (inst->ret.ty & IS_POINTER) &&
                        (inst->ret.ty & IS_GLOBAL)) {
                        assert(0);
                        // loadSymbol(fnc, fp, r0, &inst->ret);
                        // fprintf(fp, "sw   %s, 0(%s)\n", regName[r1],
                        //         regName[r0]);
                    } else {
                        // 地址
                        processMemoryLoad1(fnc, fp, REG_a1, s0);
                        fprintf(fp, "sd   %s, 0(%s)\n", regName[REG_a0],
                                regName[REG_a1]);
                    }
                    // 考虑ret溢出
                }

                break;
                case IR_OP_LOAD:
                    // 局部地址和全局地址的加载貌似是不一样的
                    {
                        // if (ValueIsImm(&inst->args[0])) {
                        //     assert(0);
                        //     loadImm(fnc, fp, REG_a0, &inst->args[0]);
                        // } else if (inst->args[0].ty & IS_SSA) {
                        //     processMemoryLoad1(fnc, fp, REG_a0, s1);
                        // }
                        // lui     a0,%hi(b)
                        // lw      a0, %lo(b)(a0)
                        // 加载了一个32位的值,int类型
                        int r0, r1;
                        // 全局标量,int a;
                        if (!(inst->args[0].ty & IS_ARRAY) &&
                            (inst->args[0].ty & IS_POINTER) &&
                            (inst->args[0].ty & IS_GLOBAL)) {
                            loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                            fprintf(fp, "lw   %s, 0(%s)\n", regName[REG_a0],
                                    regName[REG_a0]);
                        } else if (ValueIsImm(&inst->args[0])) {
                            loadImm(fnc, fp, REG_a0, &inst->args[0]);
                            assert(0);
                        } else if (inst->args[0].ty & IS_SSA) {
                            processMemoryLoad1(fnc, fp, REG_a0, s1);
                            fprintf(fp, "ld   %s, 0(%s)\n", regName[REG_a0],
                                    regName[REG_a0]);
                        }

                        if (isPHIparam1(fnc, &inst->ret, &tmpv)) {
                            s4 = getSSAlVariable(fnc, tmpv.ssaind);
                            processMemoryStore1(fnc, fp, REG_a0, s4);
                        } else {
                            processMemoryStore1(fnc, fp, REG_a0, s0);
                        }
                    }
                    break;
                case IR_OP_IF:
                case IR_OP_PHI:
                    break;
                case IR_OP_GETITEMPTR: {
                    ArrayType *arrayTy;
                    // 数组指针int fnc(int a[]);
                    if (inst->args[0].ty & IS_SSA) {
                        ssaSymbol *s =
                            getSSAlVariable(fnc, inst->args[0].ssaind);
                        if (inst->args[0].ty & IS_GLOBAL) assert(0);
                        assert(s->arrayTy);
                        processMemoryLoad1(fnc, fp, REG_SPILL, s1);
                        arrayTy = s->arrayTy;
                    } else {
                        Symbol *s;
                        if (inst->args[0].ty & IS_GLOBAL) {
                            // assert(0);
                            s = getGlobalSymbol(inst->args[0].symbol);
                            loadSymbol(fnc, fp, REG_SPILL, &inst->args[0]);
                        } else {
                            s = getLocalVariable(fnc, inst->args[0].symbol);
                            if (!s->isalloc) {
                                s->isalloc = 1;
                                s->loc =
                                    allocStack2(fnc, getArraySize2(s->arrayTy));
                                if (s->loc > 535800) {
                                    assert(0);
                                    allocStack2(fnc,
                                                getArraySize2(s->arrayTy) * 2);
                                    printf("\n%d", getArraySize2(s->arrayTy));
                                }
                            }
                            fprintf(fp, "addi    %s, t3,%d\n",
                                    regName[REG_SPILL], s->loc);
                        }

                        arrayTy = s->arrayTy;
                    }
                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     j;

                    // 考虑常量溢出

                    int        sum = 0;
                    vector_each_address(&ptr->index, j, index) {
                        if (arrayTy->next) {
                            sum = arrayTy->next->size * 8;
                        } else {
                            sum = 8;
                        }
                        arrayTy = arrayTy->next;
                        if (index->ty & IS_IMM) {
                            fprintf(fp, "#%d:[%d] \n", j, index->imm.i);
                            sum *= index->imm.i;
                            Value v = SET_IMM_INT(sum);
                            loadImm(fnc, fp, REG_CON, &v);
                            fprintf(fp, "add    %s,%s,%s\n", regName[REG_SPILL],
                                    regName[REG_SPILL], regName[REG_CON]);
                        } else {
                            // 没有考虑溢出啊
                            fprintf(fp, "#%d:[v] \n", j);
                            Value v = SET_IMM_INT(sum);
                            loadImm(fnc, fp, REG_CON, &v);
                            ssaSymbol *tmp =
                                getSSAlVariable(fnc, index->ssaind);
                            processMemoryLoad1(fnc, fp, REG_a0, tmp);
                            fprintf(fp, "mul    %s,%s,%s\n", regName[REG_CON],
                                    regName[REG_a0], regName[REG_CON]);
                            fprintf(fp, "add    %s,%s,%s\n", regName[REG_SPILL],
                                    regName[REG_SPILL], regName[REG_CON]);
                        }
                    }
                    // instruction *ni = vector_get(&&bb->inst, i + 1);
                    // if(ni->op == IR_OP_SOTRE){

                    // }
                    if (inst->isMem0) {
                        assert(0);
                    } else {
                        if (isPHIparam1(fnc, &inst->ret, &tmpv)) {
                            s4 = getSSAlVariable(fnc, tmpv.ssaind);
                            processMemoryStore1(fnc, fp, REG_SPILL, s4);
                        } else {
                            processMemoryStore1(fnc, fp, REG_SPILL, s0);
                        }
                    }
                } break;

                case IR_OP_PARAM:
                    // 数组没有做
                    if (ValueIsImm(&inst->ret)) {
                        if (!ValueIsInt(&inst->ret)) assert(0);
                        if (paramNum >= 8) {
                            loadImm(fnc, fp, REG_CON, &inst->ret);
                            fprintf(fp, "sd      %s,%d(sp)\n", regName[REG_CON],
                                    overparammem);
                            overparammem += 8;

                        } else {
                            loadImm(fnc, fp, functionArgument[paramNum],
                                    &inst->ret);
                        }
                    } else if (inst->ret.ty & IS_SSA) {
                        if (!ValueIsInt(&inst->ret)) assert(0);
                        if (paramNum >= 8) {
                            processMemoryLoad1(fnc, fp, REG_CON, s0);
                            fprintf(fp, "sd      %s,%d(sp)\n", regName[REG_CON],
                                    overparammem);
                            overparammem += 8;

                        } else {
                            if (isPHIparam1(fnc, &inst->ret, &tmpv)) {
                                s4 = getSSAlVariable(fnc, tmpv.ssaind);
                                processMemoryLoad1(
                                    fnc, fp, functionArgument[paramNum], s4);
                            } else {
                                processMemoryLoad1(
                                    fnc, fp, functionArgument[paramNum], s0);
                            }
                        }

                    } else if (inst->ret.ty & TY_VOID &&
                               inst->ret.ty & IS_POINTER) {
                        int reg = functionArgument[paramNum];
                        // TY_VOID | IS_POINTER是字符串
                        if (paramNum >= 8) assert(0);
                        RVcon con;
                        con.num = vector_len(&_con);
                        con.v = inst->ret;
                        vector_push_back(&_con, con);
                        // lui     a0, %hi(.L.str)
                        // addi    a0, a0, %lo(.L.str)
                        fprintf(fp,
                                "lui     %s, %%hi(._str%d)\n"
                                "addi      %s,%s, %%lo(._str%d)\n",
                                regName[reg], con.num, regName[reg],
                                regName[reg], con.num);
                        //, _con
                    } else if ((inst->ret.ty & IS_ARRAY) &&
                               !(inst->ret.ty & IS_GLOBAL)) {
                        if (paramNum >= 8) assert(0);
                        Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
                        fprintf(fp, "addi    %s, t3,%d\n",
                                regName[functionArgument[paramNum]], s->loc);
                    }
                    paramNum++;
                    break;
                case IR_OP_CALL: {
                    // 这里会导致局部申请很大,因为多余传入的参数没有考虑回收
                    overparammem = 0;
                    paramNum = 0;
                    Symbol *s = getGlobalSymbol(inst->args[0].symbol);
                    fprintf(fp, "call	%s\n", getToken(s->tok)->str);
                    if (isRestore) {
                        isRestore = false;
                        // genrestoreCaller(fnc, fp);
                        if (vector_len(&_calls) == 0) {
                            CurFncCall.num = 0;
                        } else {
                            CurFncCall = vector_first(&_calls);
                            vector_remove(&_calls, 0);
                        }
                    }
                    fprintf(fp, "mv  t3, sp\n");
                    if (inst->ret.ty & TY_VOID) break;
                    // return value
                    if (inst->isMem0) {
                        assert(0);
                    } else if (inst->isSpill0) {
                        assert(0);

                    } else {
                        if (ValueIsFloat(&inst->ret)) {
                            assert(0);
                        } else {
                            processMemoryStore1(fnc, fp, REG_a0, s0);
                        }
                    }
                    break;
                }

                default:
                    assert(0);
            };
        };
        if (bb->nextblock && bb->branchblock) {
        } else if (bb->nextblock) {
            fprintf(fp, "");
            fprintf(fp, "J .%s_bb_%d\n", fnc->name, bb->nextblock->bbName);
        } else if ((bb->nextblock == NULL) && (bb->branchblock == NULL)) {
            fprintf(fp, "J .%s_ret\n", fnc->name);
        } else {
            assert(0);
            // BUG 多写入了一次ret
            //  function end
        }
    }
    fprintf(fp, ".%s_ret:\n", fnc->name);
    restoreCallee(fnc, fp);
    gen_functionEpilogue(fnc, fp);
    fprintf(fp, "\n");
}
void loadFPImm(Function *fnc, FILE *fp, Reg r, Value *v) {
    assert(ValueIsFloat(v));

    RVcon con;
    con.num = vector_len(&_con);
    con.v = *v;
    vector_push_back(&_con, con);
    fprintf(fp, "la     %s, ._float%d\n", regName[REG_a4], con.num);
    fprintf(fp, "flw     %s, 0(%s)\n", regName[r], regName[REG_a4]);
}
void asmloadFPImm(Function *fnc, FILE *fp, Reg r, Value *v) {
    assert(ValueIsFloat(v));
    asminstruction *asmptr;
    RVcon           con;
    con.num = vector_len(&_con);
    con.v = *v;
    vector_push_back(&_con, con);
    // fprintf(fp, "la     %s, ._float%d\n", regName[REG_a4], con.num);
    char buff[1024];
    sprintf(buff, "._float%d", con.num);
    asmptr = createAsm8(ASM_la, REG_a4, strdup(buff));
    DL_APPEND(asmlist, asmptr);
    // fprintf(fp, "flw     %s, 0(%s)\n", regName[r], regName[REG_a4]);
    asmptr = createAsm7(ASM_flw, r, REG_a4, 0);
    DL_APPEND(asmlist, asmptr);
}
vector_template(Symbol *, zeorArrays);
// 函数的返回值如果不是void，就可以分配rax, a= fnc()
// 优化叶子函数
bool isprintf;
// 此函数不用于ssa优化
int  m1111(Function *fnc, FILE *fp) {
    fnc->loc1 += 8;

    // processPhi(fnc);
    // 指令很有可能溢出了多个参数,因此统计溢出个数,3个溢出 add t0,t0,t1
    int         memNUM = 0;
    char       *str;
    size_t      i, paramNum = 0;
    int         overparammem = 0;
    BasicBlock *bb = NULL;
    bool        isRestore = false;
    bool        swap = false;
    Value       tmpv;
    int         argumentCount = 0;

    vector_each(&fnc->BBs, i, bb) {
        fprintf(fp, ".%s_bb_%d:\n", fnc->name, bb->bbName);
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            // fprintf(fp, "#");
            // printIns(stdout, fnc, inst);
            // printIns(fp, fnc, inst);
            // fprintf(fp, "\n");

            swap = false;
            Symbol *s0, *s1, *s2, *s4;
            if (inst->args[0].ty & IS_VAR || inst->args[0].ty & IS_POINTER) {
                s1 = getLocalVariable(fnc, inst->args[0].symbol);
            }
            if (inst->args[1].ty & IS_VAR || inst->args[1].ty & IS_POINTER) {
                s2 = getLocalVariable(fnc, inst->args[1].symbol);
            }
            if (!(inst->ret.ty & TY_VOID) &&
                (inst->ret.ty & IS_VAR || inst->ret.ty & IS_POINTER)) {
                s0 = getLocalVariable(fnc, inst->ret.symbol);
            }
            switch (inst->op) {
                case IR_OP_ARGUMENT: {
                    if (argumentCount >= 8) {
                        if (ValueIsInt(&inst->ret)) {
                            fprintf(fp, "li s1,%d\n", overparammem);
                            fprintf(fp, "add s1,s1,s0\n");
                            fprintf(fp, "ld %s,0(s1)\n", regName[REG_CON],
                                     overparammem);
                            processMemoryStore1(fnc, fp, REG_CON, s0);
                        } else if (ValueIsFloat(&inst->ret)) {
                            fprintf(fp, "flw %s,%d(s0)\n", regName[REG_fa0],
                                     overparammem);
                            processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                        } else if (inst->ret.ty & IS_SSA) {
                            // 数组指针
                            fprintf(fp, "ld %s,%d(s0)\n", regName[REG_CON],
                                     overparammem);
                            processMemoryStore1(fnc, fp, REG_CON, s0);
                        } else {
                            assert(0);
                        }
                        overparammem += 8;
                    } else if (inst->ret.ty & IS_POINTER) {
                        s0->loc = allocStack2(fnc, 8);
                        functionArgument[argumentCount];
                        processMemoryStore1(
                            fnc, fp, functionArgument[argumentCount], s0);
                    } else {
                        s0->loc = allocStack2(fnc, 8);
                        functionArgument[argumentCount];
                        processMemoryStore1(
                            fnc, fp, functionArgument[argumentCount], s0);
                    }
                    argumentCount++;
                } break;
                case IR_OP_NOT:
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsInt(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    fprintf(fp, "seqz a0, a0\n");
                    processMemoryStore1(fnc, fp, REG_a0, s0);

                    break;
                case IR_OP_NOTF:
                    assert(0);
                    break;
                    // // seqz    a0, a0
                    // if (inst->args[0].ty & IS_SSA) {
                    //     fprintf(fp, "seqz   %s, %s\n",
                    //     regName[inst->ret.reg],
                    //             regName[inst->args[0].reg]);
                    // } else if (ValueIsImm(&inst->args[0])) {
                    //     assert(0);
                    // } else {
                    //     assert(0);
                    // }
                    // break;
                    // negw    a0, a0
                case IR_OP_NEG:
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsInt(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    fprintf(fp, "negw a0, a0\n");
                    processMemoryStore1(fnc, fp, REG_a0, s0);
                    break;
                //  fneg.s  fa0, fa0
                case IR_OP_NEGF:
                    assert(ValueIsFloat(&inst->args[0]));
                    assert(ValueIsFloat(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                    }
                    fprintf(fp, "fneg.s fa0, fa0\n");
                    processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                    break;

                case IR_OP_MOV:
                    assert(0);
                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_SSA) {
                        if (s1->loc == 0) {
                            s1->loc = allocStack2(fnc, 8);
                        }
                        processMemoryLoad1(fnc, fp, REG_a0, s1);
                        // processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }

                    if (isPHIparam1(fnc, &inst->ret, &tmpv)) {
                        s4 = getSSAlVariable(fnc, tmpv.ssaind);
                        processMemoryStore1(fnc, fp, REG_a0, s4);
                    } else {
                        processMemoryStore1(fnc, fp, REG_a0, s0);
                    }
                    break;
                    // src操作数：ssa,立即数;dest操作数：ssa,立即数
                case IR_OP_NE:
                    str = "beq";
                    swap = false;
                    goto _cond;
                case IR_OP_EQ:
                    str = "bne";
                    swap = false;
                    goto _cond;
                case IR_OP_GT:
                    str = "blt";
                    // str = "bge";
                    // swap = true;
                    goto _cond;
                case IR_OP_GE:
                    str = "ble";
                    swap = false;
                    goto _cond;
                case IR_OP_LT:
                    str = "bgt";
                    swap = false;
                    goto _cond;
                case IR_OP_LE:
                    str = "bge";
                    swap = false;
                _cond : {
                    int l = 0, r = 1;
                    int r1, r2;
                    if (swap) {
                        r = 0;
                        l = 1;
                    }
                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        loadImm(fnc, fp, REG_a1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_VAR) {
                        processMemoryLoad1(fnc, fp, REG_a1, s2);
                    }
                    r1 = REG_a0;
                    r2 = REG_a1;
                _cond1:
                    fprintf(fp, "%s %s, %s,.%s_bb_%d\n", str, regName[r1],
                             regName[r2], fnc->name, bb->branchblock->bbName);
                    fprintf(fp, "J .%s_bb_%d\n", fnc->name,
                             bb->nextblock->bbName);
                } break;
                case IR_OP_I2F:
                    // fmv.s.x rd, rs1 f[rd] = x[rs1][31:0]
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsFloat(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    fprintf(fp, "fcvt.s.w fa0,a0\n");
                    processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                    break;
                case IR_OP_F2I:
                    // fmv.x.s rd, rs1 x[rd] = f[rs1][31:0]
                    assert(ValueIsFloat(&inst->args[0]));
                    assert(ValueIsInt(&inst->ret));

                    if (ValueIsImm(&inst->args[0])) {
                        loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                    }
                    fprintf(fp, "fcvt.w.s a0,fa0\n");
                    processMemoryStore1(fnc, fp, REG_a0, s0);
                    break;
                case IR_OP_NEF:
                    str = "feq.s";
                    swap = false;
                    goto _fpcond;
                case IR_OP_EQF:
                    str = "fne.s";
                    swap = false;
                    goto _fpcond;
                case IR_OP_GTF:
                    str = "flt.s";
                    // swap = true;
                    goto _fpcond;
                case IR_OP_GEF:
                    str = "fle.s";
                    swap = false;
                    goto _fpcond;
                case IR_OP_LTF:
                    str = "fgt.s";
                    swap = false;
                    goto _fpcond;
                case IR_OP_LEF:
                    str = "fge.s";
                    swap = false;
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
                        loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        loadFPImm(fnc, fp, REG_fa1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_VAR) {
                        processFPMemoryLoad1(fnc, fp, REG_fa1, s2);
                    }
                    r1 = REG_fa0;
                    r2 = REG_fa1;
                    // feq.s   a5,fa5,fa4
                    // beq     a5,zero,.L4
                    fprintf(fp, "%s a0, %s,%s\n", str, regName[r1],
                             regName[r2]);
                    fprintf(fp, "beq a0, zero,.%s_bb_%d\n", fnc->name,
                             bb->branchblock->bbName);
                    fprintf(fp, "J .%s_bb_%d\n", fnc->name,
                             bb->nextblock->bbName);
                } break;
                case IR_OP_ADDF:
                    str = "fadd.s";
                    goto _fpbinary1;
                case IR_OP_SUBF:
                    str = "fsub.s";
                    goto _fpbinary1;
                case IR_OP_MULF:
                    str = "fmul.s";
                    goto _fpbinary1;
                case IR_OP_DIVF:
                    str = "fdiv.s";
                    goto _fpbinary1;
                _fpbinary1 : {
                    int r1, r2;
                    assert(ValueIsFloat(&inst->ret));
                    assert(ValueIsFloat(&inst->args[0]));
                    assert(ValueIsFloat(&inst->args[1]));
                    if (ValueIsImm(&inst->args[0])) {
                        loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        loadFPImm(fnc, fp, REG_fa1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_VAR) {
                        processFPMemoryLoad1(fnc, fp, REG_fa1, s2);
                    }
                    r1 = REG_fa0;
                    r2 = REG_fa1;

                    fprintf(fp, "%s %s, %s, %s\n", str, regName[r1],
                             regName[r1], regName[r2]);

                    processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                } break;
                case IR_OP_ADD:
                    str = "add";
                    goto _binary1;
                case IR_OP_SUB:
                    str = "sub";
                    goto _binary1;
                case IR_OP_MUL:
                    str = "mul";
                    goto _binary1;
                case IR_OP_DIV:
                    str = "div";
                    goto _binary1;
                case IR_OP_MOD:
                    str = "rem";
                _binary1 : {
                    int r1, r2;
                    assert(ValueIsInt(&inst->ret));
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsInt(&inst->args[1]));
                    if (ValueIsImm(&inst->args[0])) {
                        loadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        processMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        loadImm(fnc, fp, REG_a1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_VAR) {
                        processMemoryLoad1(fnc, fp, REG_a1, s2);
                    }
                    r1 = REG_a0;
                    r2 = REG_a1;

                    fprintf(fp, "%s %s,%s,%s\n", str, regName[r1], regName[r1],
                             regName[r2]);

                    processMemoryStore1(fnc, fp, REG_a0, s0);
                } break;
                case IR_OP_RET:
                    fprintf(fp, "J .%s_ret\n", fnc->name);
                    break;
                case IR_OP_RETI:
                    // ret
                    // 指令会在寄存器分配中分配a0,如果被溢出了需要重新加载

                    if (ValueIsImm(&inst->ret)) {
                        loadImm(fnc, fp, REG_a0, &inst->ret);
                    } else if (inst->ret.ty & IS_VAR) {
                        processMemoryLoad1(fnc, fp, REG_a0, s0);
                    }
                    fprintf(fp, "mv a0,%s\n", regName[REG_a0]);
                    fprintf(fp, "J .%s_ret\n", fnc->name);
                    break;
                case IR_OP_RETF:
                    // BUG 没有生成函数序言结语
                    assert((inst->ret.ty & TY_FLOAT));
                    if (inst->ret.ty & IS_VAR) {
                        processFPMemoryLoad1(fnc, fp, REG_fa0, s0);
                    } else if (ValueIsImm(&inst->ret)) {
                        loadFPImm(fnc, fp, REG_fa0, &inst->ret);
                    } else {
                        assert(0);
                    }
                    fprintf(fp, "J .%s_ret\n", fnc->name);
                    break;
                case IR_OP_ADDPTR: {
                    assert(0);
                    int r0, r1, r2, memNUM;
                    memNUM = 0;
                    if (inst->isMem2) {
                        processMemoryLoad(fnc, fp, REG_SPILL, &inst->args[1]);
                        memNUM++;
                        r2 = REG_SPILL;
                    } else if (ValueIsImm(&inst->args[1])) {
                        r2 = REG_CON;
                        loadImm(fnc, fp, REG_CON, &inst->args[1]);
                    } else {
                        r2 = inst->args[1].reg;
                    }

                    if (inst->isMem1) {
                        if (memNUM == 0) {
                            processMemoryLoad(fnc, fp, REG_SPILL,
                                               &inst->args[0]);
                            r1 = REG_SPILL;
                        } else {
                            processMemoryLoad(fnc, fp, REG_SPILL2,
                                               &inst->args[0]);
                            r1 = REG_SPILL2;
                        }
                        memNUM++;
                    } else if (ValueIsImm(&inst->args[0])) {
                        assert(0);
                    } else {
                        r1 = inst->args[0].reg;
                    }

                    if (inst->isMem0) {
                        r0 = REG_SPILL;
                    } else {
                        r0 = inst->ret.reg;
                    }
                    // 全局符号
                    if (inst->args[0].ty & IS_GLOBAL &&
                        inst->args[0].ty & IS_ARRAY) {
                        char *label;
                        label =
                            getToken(getGlobalSymbol(inst->args[0].symbol)->tok)
                                ->str;
                        fprintf(fp, "la %s,.%s\n", regName[r0], label);
                        // fprintf(fp,
                        //          "lui     %s, %%hi(%s)\n"
                        //           "addi      %s, %s,%%lo(%s)\n",
                        //          // addi    a0,a0,%lo(a1)
                        //          regName[r0], label, regName[r0],
                        //          regName[r0], label);
                        fprintf(fp, "add %s,%s,%s\n", regName[r0], regName[r0],
                                 regName[r2]);
                    } else if (inst->args[0].ty & IS_ARRAY) {
                        // 局部数组
                        Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                        if (s->loc == 0) {
                            s->loc =
                                allocStack2(fnc, getArraySize2(s->arrayTy) * 2);
                        }
                        // BUG
                        // addi    a5,sp,-16
                        fprintf(fp, "addi %s,sp,-%d\n", regName[r0], s->loc);
                        fprintf(fp, "add %s,%s,%s\n", regName[r0], regName[r0],
                                 regName[r2]);
                    } else if (inst->args[0].ty & IS_POINTER &&
                               inst->args[0].ty & IS_SSA) {
                        assert(0);
                        // 数组寻址的临时指针
                        fprintf(fp, "add %s,%s,%s\n", regName[r0], regName[r1],
                                 regName[r2]);
                    } else {
                        assert(0);
                    }
                    if (inst->isMem0) {
                        processMemoryStore(fnc, fp, REG_SPILL, &inst->ret);
                    }
                }

                break;
                case IR_OP_SOTRE: {
                    // lui     a0,%hi(b)
                    // lw      a0, %lo(b)(a0)
                    // 加载了一个32位的值,int类型
                    if (inst->args[0].ty & TY_INT) {
                    } else if (inst->args[0].ty & TY_FLOAT) {
                    } else {
                        assert(0);
                    }
                    if (inst->args[0].ty & TY_INT && inst->ret.ty & TY_INT) {
                        if (ValueIsImm(&inst->args[0])) {
                            loadImm(fnc, fp, REG_a0, &inst->args[0]);
                        } else if (inst->args[0].ty & IS_VAR) {
                            processMemoryLoad1(fnc, fp, REG_a0, s1);
                        } else {
                            assert(0);
                        }

                        // 全局标量
                        if (inst->ret.ty & IS_GLOBAL) {
                            loadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                            fprintf(fp, "sd %s,0(%s)\n", regName[REG_a0],
                                     regName[REG_SPILL]);
                            break;
                        }
                        if (!(inst->ret.ty & IS_ARRAY) &&
                            (inst->ret.ty & IS_POINTER) &&
                            (inst->ret.ty & IS_GLOBAL)) {
                            assert(0);
                            // loadSymbol(fnc, fp, r0, &inst->ret);
                            // fprintf(fp, "sw   %s, 0(%s)\n", regName[r1],
                            //         regName[r0]);
                        } else if (inst->ret.ty & IS_VAR &&
                                   !(inst->ret.ty & IS_POINTER)) {
                            processMemoryStore1(fnc, fp, REG_a0, s0);
                        } else {
                            // 地址
                            processMemoryLoad1(fnc, fp, REG_a1, s0);
                            fprintf(fp, "sd %s,0(%s)\n", regName[REG_a0],
                                     regName[REG_a1]);
                        }
                    } else if (inst->args[0].ty & TY_FLOAT &&
                               inst->ret.ty & TY_FLOAT) {
                        // 处理浮点数
                        if (ValueIsImm(&inst->args[0])) {
                            loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                        } else if (inst->args[0].ty & IS_VAR) {
                            processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                        } else {
                            assert(0);
                        }

                        // 全局标量
                        if (inst->ret.ty & IS_GLOBAL) {
                            loadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                            fprintf(fp, "fsw  %s,0(%s)\n", regName[REG_a0],
                                     regName[REG_SPILL]);
                            break;
                        }
                        if (!(inst->ret.ty & IS_ARRAY) &&
                            (inst->ret.ty & IS_POINTER) &&
                            (inst->ret.ty & IS_GLOBAL)) {
                            assert(0);
                            // loadSymbol(fnc, fp, r0, &inst->ret);
                            // fprintf(fp, "sw   %s, 0(%s)\n", regName[r1],
                            //         regName[r0]);
                        } else if (inst->ret.ty & IS_VAR &&
                                   !(inst->ret.ty & IS_POINTER)) {
                            processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                        } else {
                            // 地址
                            processMemoryLoad1(fnc, fp, REG_a1, s0);
                            fprintf(fp, "fsw   %s, 0(%s)\n", regName[REG_fa0],
                                     regName[REG_a1]);
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
                        if (inst->args[0].ty & TY_INT) {
                        } else if (inst->args[0].ty & TY_FLOAT) {
                        } else {
                            assert(0);
                        }
                        if (inst->args[0].ty & TY_INT &&
                            inst->ret.ty & TY_INT) {
                            // 加载了一个32位的值,int类型
                            int r0, r1;
                            // 全局标量,int a;
                            if (!(inst->args[0].ty & IS_ARRAY) &&
                                (inst->args[0].ty & IS_POINTER) &&
                                (inst->args[0].ty & IS_GLOBAL)) {
                                loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                fprintf(fp, "ld %s,0(%s)\n", regName[REG_a0],
                                         regName[REG_a0]);
                            } else if (ValueIsImm(&inst->args[0])) {
                                loadImm(fnc, fp, REG_a0, &inst->args[0]);
                                assert(0);
                            } else if (inst->args[0].ty & IS_GLOBAL) {
                                // 加载数组,调试打印
                                loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                            } else if (inst->args[0].ty & IS_VAR) {
                                processMemoryLoad1(fnc, fp, REG_a0, s1);
                                fprintf(fp, "ld %s,0(%s)\n", regName[REG_a0],
                                         regName[REG_a0]);
                            } else {
                                assert(0);
                            }

                            processMemoryStore1(fnc, fp, REG_a0, s0);
                        } else if (inst->args[0].ty & TY_FLOAT &&
                                   inst->ret.ty & TY_FLOAT) {
                            int r0, r1;
                            // 全局标量,int a;
                            if (!(inst->args[0].ty & IS_ARRAY) &&
                                (inst->args[0].ty & IS_POINTER) &&
                                (inst->args[0].ty & IS_GLOBAL)) {
                                loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                fprintf(fp, "flw   %s, 0(%s)\n",
                                         regName[REG_fa0], regName[REG_a0]);
                                processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                            } else if (ValueIsImm(&inst->args[0])) {
                                loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                                assert(0);
                            } else if (inst->args[0].ty & IS_GLOBAL) {
                                // 加载数组,调试打印
                                loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                processMemoryStore1(fnc, fp, REG_a0, s0);
                            } else if (inst->args[0].ty & IS_VAR) {
                                assert(!(inst->args[0].ty & IS_GLOBAL));
                                processMemoryLoad1(fnc, fp, REG_a0, s1);
                                fprintf(fp, "flw %s, 0(%s)\n", regName[REG_fa0],
                                         regName[REG_a0]);
                                processFPMemoryStore1(fnc, fp, REG_fa0, s0);
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
                case IR_OP_GETITEMPTR: {
                    ArrayType *arrayTy;
                    if (inst->args[0].ty & TY_INT) {
                    } else if (inst->args[0].ty & TY_FLOAT) {
                    } else {
                        assert(0);
                    }
                    // 数组指针int fnc(int a[]);
                    // ssa标识数组指针
                    if (inst->args[0].ty & IS_SSA) {
                        Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                        if (inst->args[0].ty & IS_GLOBAL) assert(0);
                        assert(s->arrayTy);
                        processMemoryLoad1(fnc, fp, REG_SPILL, s1);
                        arrayTy = s->arrayTy;
                    } else {
                        Symbol *s;
                        if (inst->args[0].ty & IS_GLOBAL) {
                            s = getGlobalSymbol(inst->args[0].symbol);
                            loadSymbol(fnc, fp, REG_SPILL, &inst->args[0]);
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
                                fprintf(fp, "la %s,.zeroarray_%x\n",
                                         regName[REG_SPILL], s);
                            } else {
                                fprintf(fp, "li t2,%d\n", s->loc);
                                fprintf(fp, "add %s,t3,t2\n",
                                         regName[REG_SPILL]);
                            }
                        }

                        arrayTy = s->arrayTy;
                    }
                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     j;

                    // 考虑常量溢出

                    int        sum = 0;
                    vector_each_address(&ptr->index, j, index) {
                        if (arrayTy->next) {
                            size_t n = getArraySize(arrayTy->next);
                            sum = n * 8;
                        } else {
                            sum = 8;
                        }
                        // if (arrayTy)
                        arrayTy = arrayTy->next;
                        if (index->ty & IS_IMM) {
                            fprintf(fp, "#%d:[%d] \n", j, index->imm.i);
                            sum *= index->imm.i;
                            Value v = SET_IMM_INT(sum);
                            loadImm(fnc, fp, REG_CON, &v);
                            fprintf(fp, "add %s,%s,%s\n", regName[REG_SPILL],
                                     regName[REG_SPILL], regName[REG_CON]);
                        } else {
                            // 没有考虑溢出啊
                            fprintf(fp, "#%d:[v] \n", j);
                            Value v = SET_IMM_INT(sum);
                            loadImm(fnc, fp, REG_CON, &v);
                            Symbol *tmp = getLocalVariable(fnc, index->symbol);
                            processMemoryLoad1(fnc, fp, REG_a0, tmp);
                            fprintf(fp, "mul %s,%s,%s\n", regName[REG_CON],
                                     regName[REG_a0], regName[REG_CON]);
                            fprintf(fp, "add %s,%s,%s\n", regName[REG_SPILL],
                                     regName[REG_SPILL], regName[REG_CON]);
                        }
                    }
                    if (inst->isMem0) {
                        assert(0);
                    } else {
                        processMemoryStore1(fnc, fp, REG_SPILL, s0);
                    }
                } break;

                case IR_OP_PARAM:
                    if (ValueIsImm(&inst->ret)) {
                        if (ValueIsInt(&inst->ret)) {
                            if (paramNum >= 8) {
                                loadImm(fnc, fp, REG_CON, &inst->ret);
                                fprintf(fp, "li      s1,%d\n", overparammem);
                                fprintf(fp, "add      s1,s1,sp\n");
                                fprintf(fp, "sd      %s,0(s1)\n",
                                         regName[REG_CON], overparammem);
                                overparammem += 8;
                            } else {
                                loadImm(fnc, fp, functionArgument[paramNum],
                                         &inst->ret);
                            }
                        } else if (ValueIsFloat(&inst->ret)) {
                            if (paramNum >= 8) {
                                loadFPImm(fnc, fp, REG_fs0, &inst->ret);
                                fprintf(fp, "fsw      %s,%d(sp)\n",
                                         regName[REG_fs0], overparammem);
                                overparammem += 8;
                            } else {
                                if (isprintf) {
                                    loadFPImm(fnc, fp,
                                               FPfunctionArgument[paramNum],
                                               &inst->ret);
                                    fprintf(
                                        fp, "fcvt.d.s    %s,%s\n",
                                        regName[FPfunctionArgument[paramNum]],
                                        regName[FPfunctionArgument[paramNum]]);
                                    fprintf(

                                        fp, "fmv.x.d    %s,%s\n",
                                        regName[functionArgument[paramNum]],
                                        regName[FPfunctionArgument[paramNum]]);

                                } else {
                                    loadFPImm(fnc, fp,
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
                        if (paramNum >= 8) {
                        } else {
                            Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
                            fprintf(fp, "li t2,%d\n", s->loc);
                            fprintf(fp, "add %s,t3,t2\n",
                                     regName[functionArgument[paramNum]]);
                            fprintf(fp, "ld %s,0(%s)\n",
                                     regName[functionArgument[paramNum]],
                                     regName[functionArgument[paramNum]]);
                        }

                    } else if ((inst->ret.ty & IS_ARRAY) &&
                               !(inst->ret.ty & IS_GLOBAL)) {
                        // 局部数组
                        // 没有使用的局部数组没有分配值
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
                            fprintf(fp, "la %s,.zeroarray_%x\n",
                                     regName[REG_SPILL], s);
                        } else {
                            fprintf(fp, "li t2,%d\n", s->loc);
                            fprintf(fp, "add %s,t3,t2\n", regName[REG_SPILL]);
                        }

                        if (paramNum >= 8) {
                            fprintf(fp, "sd %s,%d(sp)\n", regName[REG_SPILL],
                                     overparammem);
                            overparammem += 8;
                        } else {
                            fprintf(fp, "mv %s,%s\n",
                                     regName[functionArgument[paramNum]],
                                     regName[REG_SPILL]);
                            // fprintf(fp, "li    t2, %d\n", s->loc);
                            // fprintf(fp, "add    %s, t3,t2\n",
                            //        regName[functionArgument[paramNum]]);
                        }

                    } else if ((inst->ret.ty & IS_ARRAY) &&
                               (inst->ret.ty & IS_GLOBAL)) {
                        // 全局数组
                        // Symbol *s = getGlobalSymbol(inst->ret.symbol);
                        loadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                        fprintf(fp, "sd %s,%d(sp)\n", regName[REG_SPILL],
                                 overparammem);
                        overparammem += 8;

                    } else if (inst->ret.ty & IS_POINTER &&
                               (!(inst->ret.ty & IS_SSA)) &&
                               (!(inst->ret.ty & IS_ARRAY)) &&
                               (!(inst->ret.ty & TY_VOID))) {
                        // int a[122][12];a[33]
                        if (paramNum >= 8) {
                            processMemoryLoad1(fnc, fp, REG_CON, s0);
                            fprintf(fp, "sd %s,%d(sp)\n", regName[REG_CON],
                                     overparammem);
                            overparammem += 8;

                        } else {
                            processMemoryLoad1(fnc, fp,
                                                functionArgument[paramNum], s0);
                        }
                        printf("1");
                    } else if (inst->ret.ty & IS_VAR || inst->ret.ty & IS_SSA) {
                        if (!ValueIsInt(&inst->ret) &&
                            !inst->ret.ty & IS_POINTER)
                            assert(0);
                        if (ValueIsInt(&inst->ret)) {
                            if (paramNum >= 8) {
                                processMemoryLoad1(fnc, fp, REG_CON, s0);
                                fprintf(fp, "li s1,%d\n", overparammem);
                                fprintf(fp, "add s1,s1,sp\n");
                                fprintf(fp, "sd %s,0(s1)\n", regName[REG_CON],
                                         overparammem);
                                overparammem += 8;

                            } else {
                                processMemoryLoad1(
                                    fnc, fp, functionArgument[paramNum], s0);
                            }
                        } else if (ValueIsFloat(&inst->ret)) {
                            if (paramNum >= 8) {
                                processFPMemoryLoad1(fnc, fp, REG_fs0, s0);
                                fprintf(fp, "li s1,%d\n", overparammem);
                                fprintf(fp, "fsw %s,%d(sp)\n", regName[REG_fs0],
                                         overparammem);
                                overparammem += 8;

                            } else {
                                if (isprintf) {
                                    processFPMemoryLoad1(
                                        fnc, fp, FPfunctionArgument[paramNum],
                                        s0);
                                    fprintf(
                                        fp, "fcvt.d.s %s,%s\n",
                                        regName[FPfunctionArgument[paramNum]],
                                        regName[FPfunctionArgument[paramNum]]);
                                    fprintf(

                                        fp, "fmv.x.d %s,%s\n",
                                        regName[functionArgument[paramNum]],
                                        regName[FPfunctionArgument[paramNum]]);
                                    // fprintf(fp, "fmv.x.s    %s,%s\n",
                                    //          functionArgument[paramNum],
                                    //          FPfunctionArgument[paramNum], );
                                } else {
                                    processFPMemoryLoad1(
                                        fnc, fp, FPfunctionArgument[paramNum],
                                        s0);
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
                        // lui     a0, %hi(.L.str)
                        // addi    a0, a0, %lo(.L.str)
                        fprintf(fp, "la %s,._str%d\n", regName[reg], con.num);
                        // fprintf(fp,
                        //          "lui     %s, %%hi(._str%d)\n"
                        //           "addi      %s,%s, %%lo(._str%d)\n",
                        //          regName[reg], con.num, regName[reg],
                        //          regName[reg], con.num);
                        //, _con
                    } else {
                        assert(0);
                    }
                    paramNum++;
                    break;
                case IR_OP_CALL: {
                    isprintf = false;
                    // 这里会导致局部申请很大,因为多余传入的参数没有考虑回收
                    overparammem = 0;
                    paramNum = 0;
                    Symbol *s = getGlobalSymbol(inst->args[0].symbol);
                    fprintf(fp, "call %s\n", getToken(s->tok)->str);
                    fprintf(fp, "mv t3,sp\n");
                    if (inst->ret.ty & TY_VOID) break;
                    // return value

                    if (ValueIsFloat(&inst->ret)) {
                        processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                    } else {
                        processMemoryStore1(fnc, fp, REG_a0, s0);
                    }
                    break;
                }

                default:
                    assert(0);
            };
        };
        if (bb->nextblock && bb->branchblock) {
        } else if (bb->nextblock) {
            fprintf(fp, "");
            fprintf(fp, "J .%s_bb_%d\n", fnc->name, bb->nextblock->bbName);
        } else if ((bb->nextblock == NULL) && (bb->branchblock == NULL)) {
            fprintf(fp, "J .%s_ret\n", fnc->name);
        } else {
            assert(0);
            // BUG 多写入了一次ret
            //  function end
        }
        instruction *tmpIns;
        // vector_each3(&bb->inst, tmpIns) { free(tmpIns); }
        // vector_free(&bb->inst);
        // vector_free(&bb->df);
        // vector_free(&bb->postdf);
        // vector_free(&bb->inedges);
        // vector_free(&bb->doms);
    }
    fprintf(fp, ".%s_ret:\n", fnc->name);
    restoreCallee(fnc, fp);
    gen_functionEpilogue(fnc, fp);
    fprintf(fp, "\n");
}
// 函数的返回值如果不是void，就可以分配rax, a= fnc()
// 优化叶子函数

void processZeroArray(FILE *fp) {
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

int asmm1111(Function *fnc, FILE *fp) {
    fnc->loc1 += 8;

    processPhi(fnc);
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
            Symbol *s0, *s1, *s2, *s4;
            if (inst->args[0].ty & IS_VAR || inst->args[0].ty & IS_POINTER) {
                s1 = getLocalVariable(fnc, inst->args[0].symbol);
            }
            if (inst->args[1].ty & IS_VAR || inst->args[1].ty & IS_POINTER) {
                s2 = getLocalVariable(fnc, inst->args[1].symbol);
            }
            if (!(inst->ret.ty & TY_VOID) &&
                (inst->ret.ty & IS_VAR || inst->ret.ty & IS_POINTER)) {
                s0 = getLocalVariable(fnc, inst->ret.symbol);
            }
            int feq = 0;
            switch (inst->op) {
                case IR_OP_ARGUMENT: {
                    if (argumentCount >= 8) {
                        if (ValueIsInt(&inst->ret)) {
                            // fprintf(fp, "li        s1,%d\n", overparammem);
                            asmptr = createAsm4(ASM_li, REG_s1, overparammem);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "add      s1,s1,s0\n");
                            asmptr =
                                createAsm2(ASM_add, REG_s1, REG_s1, REG_s0);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "ld      %s,0(s1)\n",
                            // regName[REG_CON],
                            //                     overparammem);
                            asmptr = createAsm7(ASM_ld, REG_CON, REG_s1,
                                                overparammem);
                            DL_APPEND(asmlist, asmptr);
                            // processMemoryStore1(fnc, fp, REG_CON, s0);
                            asmprocessMemoryStore1(fnc, fp, REG_CON, s0);
                        } else if (ValueIsFloat(&inst->ret)) {
                            // fprintf(fp, "flw      %s,%d(s0)\n",
                            //         regName[REG_fa0], overparammem);
                            asmptr = createAsm7(ASM_flw, REG_fa0, REG_s0,
                                                overparammem);
                            DL_APPEND(asmlist, asmptr);
                            // processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                            asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
                        } else if (inst->ret.ty & IS_SSA) {
                            // 数组指针
                            // fprintf(fp, "ld      %s,%d(s0)\n",
                            // regName[REG_CON],
                            //         overparammem);
                            asmptr = createAsm7(ASM_ld, REG_CON, REG_s0,
                                                overparammem);
                            DL_APPEND(asmlist, asmptr);
                            // processMemoryStore1(fnc, fp, REG_CON, s0);
                            asmprocessMemoryStore1(fnc, fp, REG_CON, s0);
                        } else {
                            assert(0);
                        }
                        overparammem += 8;
                    } else if (inst->ret.ty & IS_POINTER) {
                        s0->loc = allocStack2(fnc, 8);
                        // processMemoryStore1(
                        //     fnc, fp, functionArgument[argumentCount], s0);
                        asmprocessMemoryStore1(
                            fnc, fp, functionArgument[argumentCount], s0);
                    } else {
                        if (ValueIsInt(&inst->ret)) {
                            s0->loc = allocStack2(fnc, 8);
                            // processMemoryStore1(
                            //     fnc, fp, functionArgument[argumentCount],
                            //     s0);
                            asmprocessMemoryStore1(
                                fnc, fp, functionArgument[argumentCount], s0);

                        } else {
                            s0->loc = allocStack2(fnc, 8);
                            // processMemoryStore1(
                            //     fnc, fp, functionArgument[argumentCount],
                            //     s0);
                            asmprocessFPMemoryStore1(
                                fnc, fp, FPfunctionArgument[argumentCount], s0);
                        }
                    }
                    argumentCount++;
                } break;
                case IR_OP_NOT:
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsInt(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        // loadImm(fnc, fp, REG_a0, &inst->args[0]);
                        asmloadImm(fnc, fp, REG_a0, &inst->args[0]);

                    } else if (inst->args[0].ty & IS_VAR) {
                        // processMemoryLoad1(fnc, fp, REG_a0, s1);
                        asmprocessMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    // fprintf(fp, "seqz    a0, a0\n");
                    asmptr = createAsm5(ASM_seqz, REG_a0, REG_a0);
                    DL_APPEND(asmlist, asmptr);
                    // processMemoryStore1(fnc, fp, REG_a0, s0);
                    asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                    break;
                case IR_OP_NOTF:
                    assert(0);
                    break;
                    // negw    a0, a0
                case IR_OP_NEG:
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsInt(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        // loadImm(fnc, fp, REG_a0, &inst->args[0]);
                        asmloadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        // processMemoryLoad1(fnc, fp, REG_a0, s1);
                        asmprocessMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    // fprintf(fp, "negw    a0, a0\n");
                    asmptr = createAsm5(ASM_negw, REG_a0, REG_a0);
                    DL_APPEND(asmlist, asmptr);
                    // processMemoryStore1(fnc, fp, REG_a0, s0);
                    asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                    break;
                //  fneg.s  fa0, fa0
                case IR_OP_NEGF:
                    assert(ValueIsFloat(&inst->args[0]));
                    assert(ValueIsFloat(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        // loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                        asmloadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        // processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                        asmprocessFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                    }
                    // fprintf(fp, "fneg.s    fa0, fa0\n");
                    asmptr = createAsm11(ASM_fneg, REG_fa0, REG_fa0);
                    DL_APPEND(asmlist, asmptr);
                    // processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                    asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
                    break;
                case IR_OP_MOV:
                    assert(0);
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
                    swap = false;
                _cond : {
                    int l = 0, r = 1;
                    int r1, r2;
                    if (swap) {
                        r = 0;
                        l = 1;
                    }
                    if (ValueIsImm(&inst->args[0])) {
                        // loadImm(fnc, fp, REG_a0, &inst->args[0]);
                        asmloadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        // processMemoryLoad1(fnc, fp, REG_a0, s1);
                        asmprocessMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        // loadImm(fnc, fp, REG_a1, &inst->args[1]);
                        asmloadImm(fnc, fp, REG_a1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_VAR) {
                        // processMemoryLoad1(fnc, fp, REG_a1, s2);
                        asmprocessMemoryLoad1(fnc, fp, REG_a1, s2);
                    }
                    r1 = REG_a0;
                    r2 = REG_a1;
                _cond1:
                    // fprintf(fp, "%s   %s, %s,.%s_bb_%d\n", str, regName[r1],
                    //         regName[r2], fnc->name, bb->branchblock->bbName);
                    {
                        char buff[1024];
                        sprintf(buff, ".%s_bb_%d", fnc->name,
                                bb->branchblock->bbName);
                        asmptr = createAsm6(asmop, r1, r2, strdup(buff));
                        DL_APPEND(asmlist, asmptr);

                        // fprintf(fp, "J .%s_bb_%d\n", fnc->name,
                        //         bb->nextblock->bbName);
                        sprintf(buff, ".%s_bb_%d", fnc->name,
                                bb->nextblock->bbName);
                        asmptr = createAsm1(ASM_J, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    }
                } break;
                case IR_OP_I2F:
                    // fmv.s.x rd, rs1 f[rd] = x[rs1][31:0]
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsFloat(&inst->ret));
                    if (ValueIsImm(&inst->args[0])) {
                        // loadImm(fnc, fp, REG_a0, &inst->args[0]);
                        asmloadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        // processMemoryLoad1(fnc, fp, REG_a0, s1);
                        asmprocessMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    // fprintf(fp, "fcvt.s.w    fa0,a0\n");
                    asmptr = createAsm11(ASM_fcvtsw, REG_fa0, REG_a0);
                    DL_APPEND(asmlist, asmptr);
                    // processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                    asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
                    break;
                case IR_OP_F2I:
                    // fmv.x.s rd, rs1 x[rd] = f[rs1][31:0]
                    assert(ValueIsFloat(&inst->args[0]));
                    assert(ValueIsInt(&inst->ret));

                    if (ValueIsImm(&inst->args[0])) {
                        // loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                        asmloadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        // processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                        asmprocessFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                    }
                    // fprintf(fp, "fcvt.w.s     a0,fa0\n");
                    asmptr = createAsm11(ASM_fcvtws, REG_a0, REG_fa0);
                    DL_APPEND(asmlist, asmptr);
                    // processMemoryStore1(fnc, fp, REG_a0, s0);
                    asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
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
                        // loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                        asmloadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        // processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                        asmprocessFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        // loadFPImm(fnc, fp, REG_fa1, &inst->args[1]);
                        asmloadFPImm(fnc, fp, REG_fa1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_VAR) {
                        // processFPMemoryLoad1(fnc, fp, REG_fa1, s2);
                        asmprocessFPMemoryLoad1(fnc, fp, REG_fa1, s2);
                    }
                    r1 = REG_fa0;
                    r2 = REG_fa1;
                    // feq.s   a5,fa5,fa4
                    // beq     a5,zero,.L4
                    // fprintf(fp, "%s   a0, %s,%s\n", str, regName[r1],
                    //         regName[r2]);
                    asmptr = createAsm12(asmop, REG_a0, r1, r2);
                    DL_APPEND(asmlist, asmptr);

                    // fprintf(fp, "beq   a0, zero,.%s_bb_%d\n", fnc->name,
                    //         bb->branchblock->bbName);
                    if (feq) {
                        char buff[1024];
                        sprintf(buff, ".%s_bb_%d", fnc->name,
                                bb->nextblock->bbName);
                        asmptr =
                            createAsm6(ASM_beq, REG_a0, REG_zero, strdup(buff));
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
                        asmptr =
                            createAsm6(ASM_beq, REG_a0, REG_zero, strdup(buff));
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
                        // loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                        asmloadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        // processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                        asmprocessFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        // loadFPImm(fnc, fp, REG_fa1, &inst->args[1]);
                        asmloadFPImm(fnc, fp, REG_fa1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_VAR) {
                        // processFPMemoryLoad1(fnc, fp, REG_fa1, s2);
                        asmprocessFPMemoryLoad1(fnc, fp, REG_fa1, s2);
                    }
                    r1 = REG_fa0;
                    r2 = REG_fa1;

                    // fprintf(fp, "%s   %s, %s, %s\n", str, regName[r1],
                    //         regName[r1], regName[r2]);
                    asmptr = createAsm12(asmop, r1, r1, r2);
                    DL_APPEND(asmlist, asmptr);

                    // processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                    asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
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
                    int r1, r2;
                    assert(ValueIsInt(&inst->ret));
                    assert(ValueIsInt(&inst->args[0]));
                    assert(ValueIsInt(&inst->args[1]));
                    if (ValueIsImm(&inst->args[0])) {
                        // loadImm(fnc, fp, REG_a0, &inst->args[0]);
                        asmloadImm(fnc, fp, REG_a0, &inst->args[0]);
                    } else if (inst->args[0].ty & IS_VAR) {
                        // processMemoryLoad1(fnc, fp, REG_a0, s1);
                        asmprocessMemoryLoad1(fnc, fp, REG_a0, s1);
                    }
                    if (ValueIsImm(&inst->args[1])) {
                        // loadImm(fnc, fp, REG_a1, &inst->args[1]);
                        asmloadImm(fnc, fp, REG_a1, &inst->args[1]);
                    } else if (inst->args[1].ty & IS_VAR) {
                        // processMemoryLoad1(fnc, fp, REG_a1, s2);
                        asmprocessMemoryLoad1(fnc, fp, REG_a1, s2);
                    }
                    r1 = REG_a0;
                    r2 = REG_a1;

                    // fprintf(fp, "%s   %s, %s, %s\n", str, regName[r1],
                    //         regName[r1], regName[r2]);
                    asmptr = createAsm2(asmop, r1, r1, r2);
                    DL_APPEND(asmlist, asmptr);

                    // processMemoryStore1(fnc, fp, REG_a0, s0);
                    asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                } break;
                case IR_OP_RET: {
                    // fprintf(fp, "J .%s_ret\n", fnc->name);
                    char buff[1024];
                    sprintf(buff, ".%s_ret", fnc->name);
                    asmptr = createAsm1(ASM_J, strdup(buff));
                    DL_APPEND(asmlist, asmptr);
                } break;
                case IR_OP_RETI:
                    // ret
                    // 指令会在寄存器分配中分配a0,如果被溢出了需要重新加载

                    if (ValueIsImm(&inst->ret)) {
                        // loadImm(fnc, fp, REG_a0, &inst->ret);
                        asmloadImm(fnc, fp, REG_a0, &inst->ret);
                    } else if (inst->ret.ty & IS_VAR) {
                        // processMemoryLoad1(fnc, fp, REG_a0, s0);
                        asmprocessMemoryLoad1(fnc, fp, REG_a0, s0);
                    }
                    // fprintf(fp, "mv    a0, %s\n", regName[REG_a0]);
                    asmptr = createAsm5(ASM_mv, REG_a0, REG_a0);
                    DL_APPEND(asmlist, asmptr);
                    // fprintf(fp, "J .%s_ret\n", fnc->name);
                    {
                        char buff[1024];
                        sprintf(buff, ".%s_ret", fnc->name);
                        asmptr = createAsm1(ASM_J, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    }
                    break;
                case IR_OP_RETF:
                    // BUG 没有生成函数序言结语
                    assert((inst->ret.ty & TY_FLOAT));
                    if (inst->ret.ty & IS_VAR) {
                        // processFPMemoryLoad1(fnc, fp, REG_fa0, s0);
                        asmprocessFPMemoryLoad1(fnc, fp, REG_fa0, s0);
                    } else if (ValueIsImm(&inst->ret)) {
                        // loadFPImm(fnc, fp, REG_fa0, &inst->ret);
                        asmloadFPImm(fnc, fp, REG_fa0, &inst->ret);
                    } else {
                        assert(0);
                    }
                    // fprintf(fp, "J .%s_ret\n", fnc->name);
                    {
                        char buff[1024];
                        sprintf(buff, ".%s_ret", fnc->name);
                        asmptr = createAsm1(ASM_J, strdup(buff));
                        DL_APPEND(asmlist, asmptr);
                    }
                    break;
                case IR_OP_ADDPTR:
                    assert(0);
                case IR_OP_SOTRE: {
                    if (inst->args[0].ty & TY_INT) {
                    } else if (inst->args[0].ty & TY_FLOAT) {
                    } else {
                        assert(0);
                    }
                    if (inst->args[0].ty & TY_INT && inst->ret.ty & TY_INT) {
                        if (ValueIsImm(&inst->args[0])) {
                            // loadImm(fnc, fp, REG_a0, &inst->args[0]);
                            asmloadImm(fnc, fp, REG_a0, &inst->args[0]);
                        } else if (inst->args[0].ty & IS_VAR) {
                            // processMemoryLoad1(fnc, fp, REG_a0, s1);
                            asmprocessMemoryLoad1(fnc, fp, REG_a0, s1);
                        } else {
                            assert(0);
                        }

                        // 全局标量
                        if (inst->ret.ty & IS_GLOBAL) {
                            // loadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                            asmloadSymbol(fnc, fp, REG_SPILL, &inst->ret);

                            // fprintf(fp, "sd   %s, 0(%s)\n", regName[REG_a0],
                            //         regName[REG_SPILL]);
                            asmptr = createAsm7(ASM_sw, REG_a0, REG_SPILL, 0);
                            DL_APPEND(asmlist, asmptr);
                            break;
                        }
                        if (!(inst->ret.ty & IS_ARRAY) &&
                            (inst->ret.ty & IS_POINTER) &&
                            (inst->ret.ty & IS_GLOBAL)) {
                            assert(0);
                        } else if (inst->ret.ty & IS_VAR &&
                                   !(inst->ret.ty & IS_POINTER)) {
                            // processMemoryStore1(fnc, fp, REG_a0, s0);
                            asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                        } else {
                            // 地址
                            // processMemoryLoad1(fnc, fp, REG_a1, s0);
                            asmprocessMemoryLoad1(fnc, fp, REG_a1, s0);
                            // fprintf(fp, "sd   %s, 0(%s)\n", regName[REG_a0],
                            //         regName[REG_a1]);
                            asmptr = createAsm7(ASM_sw, REG_a0, REG_a1, 0);
                            DL_APPEND(asmlist, asmptr);
                        }
                    } else if (inst->args[0].ty & TY_FLOAT &&
                               inst->ret.ty & TY_FLOAT) {
                        // 处理浮点数
                        if (ValueIsImm(&inst->args[0])) {
                            // loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                            asmloadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                        } else if (inst->args[0].ty & IS_VAR) {
                            // processFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                            asmprocessFPMemoryLoad1(fnc, fp, REG_fa0, s1);
                        } else {
                            assert(0);
                        }

                        // 全局标量
                        if (inst->ret.ty & IS_GLOBAL) {
                            // loadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                            asmloadSymbol(fnc, fp, REG_SPILL, &inst->ret);
                            // fprintf(fp, "fsw   %s, 0(%s)\n", regName[REG_a0],
                            //         regName[REG_SPILL]);
                            // fprintf(fp, "fsw   %s, 0(%s)\n", regName[REG_a0],
                            //         regName[REG_SPILL]);
                            asmptr = createAsm7(ASM_fsw, REG_fa0, REG_SPILL, 0);
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
                            asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
                        } else {
                            // 地址
                            // processMemoryLoad1(fnc, fp, REG_a1, s0);
                            asmprocessMemoryLoad1(fnc, fp, REG_a1, s0);
                            // fprintf(fp, "fsw   %s, 0(%s)\n",
                            // regName[REG_fa0],
                            //         regName[REG_a1]);
                            asmptr = createAsm7(ASM_fsw, REG_fa0, REG_a1, 0);
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
                        if (inst->args[0].ty & TY_INT) {
                        } else if (inst->args[0].ty & TY_FLOAT) {
                        } else {
                            assert(0);
                        }
                        if (inst->args[0].ty & TY_INT &&
                            inst->ret.ty & TY_INT) {
                            // 加载了一个32位的值,int类型
                            int r0, r1;
                            // 全局标量,int a;
                            if (!(inst->args[0].ty & IS_ARRAY) &&
                                (inst->args[0].ty & IS_POINTER) &&
                                (inst->args[0].ty & IS_GLOBAL)) {
                                // loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                asmloadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                // fprintf(fp, "ld   %s, 0(%s)\n",
                                // regName[REG_a0],
                                //         regName[REG_a0]);
                                asmptr = createAsm7(ASM_lw, REG_a0, REG_a0, 0);
                                DL_APPEND(asmlist, asmptr);
                            } else if (ValueIsImm(&inst->args[0])) {
                                loadImm(fnc, fp, REG_a0, &inst->args[0]);
                                assert(0);
                            } else if (inst->args[0].ty & IS_GLOBAL) {
                                // 加载数组,调试打印
                                // loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                asmloadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                            } else if (inst->args[0].ty & IS_VAR) {
                                // processMemoryLoad1(fnc, fp, REG_a0, s1);
                                asmprocessMemoryLoad1(fnc, fp, REG_a0, s1);
                                // fprintf(fp, "ld   %s, 0(%s)\n",
                                // regName[REG_a0],
                                //         regName[REG_a0]);
                                asmptr = createAsm7(ASM_lw, REG_a0, REG_a0, 0);
                                DL_APPEND(asmlist, asmptr);
                            } else {
                                assert(0);
                            }

                            // processMemoryStore1(fnc, fp, REG_a0, s0);
                            asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                        } else if (inst->args[0].ty & TY_FLOAT &&
                                   inst->ret.ty & TY_FLOAT) {
                            int r0, r1;
                            // 全局标量,int a;
                            if (!(inst->args[0].ty & IS_ARRAY) &&
                                (inst->args[0].ty & IS_POINTER) &&
                                (inst->args[0].ty & IS_GLOBAL)) {
                                // loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                asmloadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                // fprintf(fp, "flw   %s, 0(%s)\n",
                                //         regName[REG_fa0], regName[REG_a0]);
                                asmptr =
                                    createAsm7(ASM_flw, REG_fa0, REG_a0, 0);
                                DL_APPEND(asmlist, asmptr);
                                // processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                                asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
                            } else if (ValueIsImm(&inst->args[0])) {
                                loadFPImm(fnc, fp, REG_fa0, &inst->args[0]);
                                assert(0);
                            } else if (inst->args[0].ty & IS_GLOBAL) {
                                // 加载数组,调试打印
                                // loadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                asmloadSymbol(fnc, fp, REG_a0, &inst->args[0]);
                                // processMemoryStore1(fnc, fp, REG_a0, s0);
                                asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                            } else if (inst->args[0].ty & IS_VAR) {
                                assert(!(inst->args[0].ty & IS_GLOBAL));
                                // processMemoryLoad1(fnc, fp, REG_a0, s1);
                                asmprocessMemoryLoad1(fnc, fp, REG_a0, s1);
                                // fprintf(fp, "flw   %s, 0(%s)\n",
                                //         regName[REG_fa0], regName[REG_a0]);
                                asmptr =
                                    createAsm7(ASM_flw, REG_fa0, REG_a0, 0);
                                DL_APPEND(asmlist, asmptr);
                                // processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                                asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
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
                case IR_OP_GETITEMPTR: {
                    ArrayType *arrayTy;
                    if (inst->args[0].ty & TY_INT) {
                    } else if (inst->args[0].ty & TY_FLOAT) {
                    } else {
                        assert(0);
                    }
                    // 数组指针int fnc(int a[]);
                    // ssa标识数组指针
                    if (inst->args[0].ty & IS_SSA) {
                        Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                        if (inst->args[0].ty & IS_GLOBAL) assert(0);
                        assert(s->arrayTy);
                        // processMemoryLoad1(fnc, fp, REG_SPILL, s1);
                        asmprocessMemoryLoad1(fnc, fp, REG_SPILL, s1);
                        arrayTy = s->arrayTy;
                    } else {
                        Symbol *s;
                        if (inst->args[0].ty & IS_GLOBAL) {
                            s = getGlobalSymbol(inst->args[0].symbol);
                            // loadSymbol(fnc, fp, REG_SPILL, &inst->args[0]);
                            asmloadSymbol(fnc, fp, REG_SPILL, &inst->args[0]);
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
                                asmptr =
                                    createAsm8(ASM_la, REG_SPILL, strdup(buff));
                                DL_APPEND(asmlist, asmptr);
                            } else {
                                // fprintf(fp, "li    t2, %d\n", s->loc);
                                asmptr = createAsm4(ASM_li, REG_t2, s->loc);
                                DL_APPEND(asmlist, asmptr);
                                // fprintf(fp, "add    %s, t3,t2\n",
                                //         regName[REG_SPILL]);
                                asmptr = createAsm2(ASM_add, REG_SPILL, REG_t3,
                                                    REG_t2);
                                DL_APPEND(asmlist, asmptr);
                            }
                        }

                        arrayTy = s->arrayTy;
                    }
                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     j;

                    // 考虑常量溢出

                    int        sum = 0;
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
                            // 没有考虑溢出啊
                            fprintf(fp, "#%d:[v] \n", j);
                            Value v = SET_IMM_INT(sum);
                            // loadImm(fnc, fp, REG_CON, &v);
                            asmloadImm(fnc, fp, REG_CON, &v);
                            Symbol *tmp = getLocalVariable(fnc, index->symbol);
                            // processMemoryLoad1(fnc, fp, REG_a0, tmp);
                            asmprocessMemoryLoad1(fnc, fp, REG_a0, tmp);
                            // fprintf(fp, "mul    %s,%s,%s\n",
                            //         regName[REG_CON], regName[REG_a0],
                            //         regName[REG_CON]);
                            asmptr =
                                createAsm2(ASM_mul, REG_CON, REG_a0, REG_CON);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "add    %s,%s,%s\n",
                            //         regName[REG_SPILL], regName[REG_SPILL],
                            //         regName[REG_CON]);
                            asmptr = createAsm2(ASM_add, REG_SPILL, REG_SPILL,
                                                REG_CON);
                            DL_APPEND(asmlist, asmptr);
                        }
                    }
                    // instruction *ni = vector_get(&&bb->inst, i + 1);
                    // if(ni->op == IR_OP_SOTRE){

                    // }
                    if (inst->isMem0) {
                        assert(0);
                    } else {
                        // processMemoryStore1(fnc, fp, REG_SPILL, s0);
                        asmprocessMemoryStore1(fnc, fp, REG_SPILL, s0);
                    }
                } break;

                case IR_OP_PARAM:
                    if (paramNum < 8) {
                        overparammem = 0;
                    }
                    if (ValueIsImm(&inst->ret)) {
                        if (ValueIsInt(&inst->ret)) {
                            if (paramNum >= 8) {
                                // loadImm(fnc, fp, REG_CON, &inst->ret);
                                asmloadImm(fnc, fp, REG_CON, &inst->ret);

                                // fprintf(fp, "li      s1,%d\n", overparammem);
                                asmptr =
                                    createAsm4(ASM_li, REG_s1, overparammem);
                                DL_APPEND(asmlist, asmptr);
                                // fprintf(fp, "add      s1,s1,sp\n");
                                asmptr =
                                    createAsm2(ASM_add, REG_s1, REG_s1, REG_sp);
                                DL_APPEND(asmlist, asmptr);
                                // fprintf(fp, "sd      %s,0(s1)\n",
                                //         regName[REG_CON], overparammem);
                                asmptr = createAsm7(ASM_sd, REG_CON, REG_s1,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                overparammem += 8;
                            } else {
                                // loadImm(fnc, fp, functionArgument[paramNum],
                                //         &inst->ret);
                                asmloadImm(fnc, fp, functionArgument[paramNum],
                                           &inst->ret);
                            }
                        } else if (ValueIsFloat(&inst->ret)) {
                            if (paramNum >= 8) {
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
                                    // loadFPImm(fnc, fp,
                                    //           FPfunctionArgument[paramNum],
                                    //           &inst->ret);
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
                        if (paramNum >= 8) {
                            Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
                            // fprintf(fp, "li    t2, %d\n", s->loc);
                            asmptr = createAsm4(ASM_li, REG_t2, s->loc);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "add    %s, t3,t2\n",
                            //         regName[functionArgument[paramNum]]);
                            asmptr =
                                createAsm2(ASM_add, REG_t2, REG_t3, REG_t2);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "ld    %s, 0(%s)\n",
                            //         regName[functionArgument[paramNum]],
                            //         regName[functionArgument[paramNum]]);

                            asmptr = createAsm7(ASM_ld, REG_t2, REG_t2, 0);
                            DL_APPEND(asmlist, asmptr);
                            asmptr = createAsm7(ASM_sd, REG_t2, REG_sp,
                                                overparammem);
                            DL_APPEND(asmlist, asmptr);
                            overparammem += 8;

                        } else {
                            Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
                            // fprintf(fp, "li    t2, %d\n", s->loc);
                            asmptr = createAsm4(ASM_li, REG_t2, s->loc);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "add    %s, t3,t2\n",
                            //         regName[functionArgument[paramNum]]);
                            asmptr =
                                createAsm2(ASM_add, functionArgument[paramNum],
                                           REG_t3, REG_t2);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "ld    %s, 0(%s)\n",
                            //         regName[functionArgument[paramNum]],
                            //         regName[functionArgument[paramNum]]);
                            asmptr =
                                createAsm7(ASM_ld, functionArgument[paramNum],
                                           functionArgument[paramNum], 0);
                            DL_APPEND(asmlist, asmptr);
                        }

                    } else if ((inst->ret.ty & IS_ARRAY) &&
                               !(inst->ret.ty & IS_GLOBAL)) {
                        // 局部数组
                        // 没有使用的局部数组没有分配值
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
                            asmptr = createAsm4(ASM_li, REG_t2, s->loc);
                            DL_APPEND(asmlist, asmptr);
                            // fprintf(fp, "add    %s, t3,t2\n",
                            //         regName[REG_SPILL]);
                            asmptr =
                                createAsm2(ASM_add, REG_SPILL, REG_t3, REG_t2);
                            DL_APPEND(asmlist, asmptr);
                        }

                        if (paramNum >= 8) {
                            // fprintf(fp, "sd      %s,%d(sp)\n",
                            //         regName[REG_SPILL], overparammem);
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
                        // fprintf(fp, "sd      %s,%d(sp)\n",
                        // regName[REG_SPILL],
                        //         overparammem);
                        asmptr =
                            createAsm7(ASM_sd, REG_SPILL, REG_sp, overparammem);
                        DL_APPEND(asmlist, asmptr);
                        overparammem += 8;

                    } else if (inst->ret.ty & IS_POINTER &&
                               (!(inst->ret.ty & IS_SSA)) &&
                               (!(inst->ret.ty & IS_ARRAY)) &&
                               (!(inst->ret.ty & TY_VOID))) {
                        // int a[122][12];a[33]
                        if (paramNum >= 8) {
                            // processMemoryLoad1(fnc, fp, REG_CON, s0);
                            asmprocessMemoryLoad1(fnc, fp, REG_CON, s0);
                            // fprintf(fp, "sd      %s,%d(sp)\n",
                            // regName[REG_CON],
                            //         overparammem);
                            asmptr = createAsm7(ASM_sd, REG_CON, REG_sp,
                                                overparammem);
                            DL_APPEND(asmlist, asmptr);
                            overparammem += 8;

                        } else {
                            // processMemoryLoad1(fnc, fp,
                            //                    functionArgument[paramNum],
                            //                    s0);
                            asmprocessMemoryLoad1(
                                fnc, fp, functionArgument[paramNum], s0);
                        }
                        printf("1");
                    } else if (inst->ret.ty & IS_VAR || inst->ret.ty & IS_SSA) {
                        if (!ValueIsInt(&inst->ret) &&
                            !inst->ret.ty & IS_POINTER)
                            assert(0);
                        if (ValueIsInt(&inst->ret)) {
                            if (paramNum >= 8) {
                                // processMemoryLoad1(fnc, fp, REG_CON, s0);
                                asmprocessMemoryLoad1(fnc, fp, REG_CON, s0);

                                // fprintf(fp, "li      s1,%d\n", overparammem);
                                asmptr =
                                    createAsm4(ASM_li, REG_s1, overparammem);
                                DL_APPEND(asmlist, asmptr);

                                // fprintf(fp, "add      s1,s1,sp\n");
                                asmptr =
                                    createAsm2(ASM_add, REG_s1, REG_s1, REG_sp);
                                DL_APPEND(asmlist, asmptr);
                                fprintf(fp, "sd      %s,0(s1)\n",
                                        regName[REG_CON], overparammem);
                                asmptr = createAsm7(ASM_sd, REG_CON, REG_s1,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                overparammem += 8;

                            } else {
                                // processMemoryLoad1(
                                //     fnc, fp, functionArgument[paramNum], s0);
                                asmprocessMemoryLoad1(
                                    fnc, fp, functionArgument[paramNum], s0);
                            }
                        } else if (ValueIsFloat(&inst->ret)) {
                            if (paramNum >= 8) {
                                // processMemoryLoad1(fnc, fp, REG_CON, s0);
                                asmprocessMemoryLoad1(fnc, fp, REG_fs0, s0);
                                // fprintf(fp, "li      s1,%d\n", overparammem);
                                asmptr =
                                    createAsm4(ASM_li, REG_s1, overparammem);
                                DL_APPEND(asmlist, asmptr);
                                // fprintf(fp, "sd      %s,%d(sp)\n",
                                //         regName[REG_CON], overparammem);
                                asmptr = createAsm7(ASM_fsw, REG_fs0, REG_sp,
                                                    overparammem);
                                DL_APPEND(asmlist, asmptr);
                                overparammem += 8;

                            } else {
                                if (isprintf) {
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
                                    // processFPMemoryLoad1(
                                    //     fnc, fp,
                                    //     FPfunctionArgument[paramNum], s0);
                                    asmprocessFPMemoryLoad1(
                                        fnc, fp, FPfunctionArgument[paramNum],
                                        s0);
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
                    break;
                case IR_OP_CALL: {
                    isprintf = false;
                    // 这里会导致局部申请很大,因为多余传入的参数没有考虑回收
                    overparammem = 0;
                    paramNum = 0;
                    Symbol *s = getGlobalSymbol(inst->args[0].symbol);
                    // fprintf(fp, "call	%s\n", getToken(s->tok)->str);
                    asmptr = createAsm1(ASM_call, getToken(s->tok)->str);
                    DL_APPEND(asmlist, asmptr);
                    // fprintf(fp, "mv  t3, sp\n");
                    asmptr = createAsm5(ASM_mv, REG_t3, REG_sp);
                    DL_APPEND(asmlist, asmptr);
                    if (inst->ret.ty & TY_VOID) break;
                    if (ValueIsFloat(&inst->ret)) {
                        // processFPMemoryStore1(fnc, fp, REG_fa0, s0);
                        asmprocessFPMemoryStore1(fnc, fp, REG_fa0, s0);
                    } else {
                        // processMemoryStore1(fnc, fp, REG_a0, s0);
                        asmprocessMemoryStore1(fnc, fp, REG_a0, s0);
                    }
                    break;
                }

                default:
                    assert(0);
            };
        };
        if (bb->nextblock && bb->branchblock) {
        } else if (bb->nextblock) {
            // fprintf(fp, "");
            char buff[1024];
            // fprintf(fp, "J .%s_bb_%d\n", fnc->name, bb->nextblock->bbName);
            sprintf(buff, ".%s_bb_%d", fnc->name, bb->nextblock->bbName);
            asmptr = createAsm1(ASM_J, strdup(buff));
            DL_APPEND(asmlist, asmptr);
        } else if ((bb->nextblock == NULL) && (bb->branchblock == NULL)) {
            // fprintf(fp, "J .%s_ret\n", fnc->name);
            char buff[1024];
            sprintf(buff, ".%s_ret", fnc->name);
            asmptr = createAsm1(ASM_J, strdup(buff));
            DL_APPEND(asmlist, asmptr);
        } else {
            assert(0);
        }
        instruction *tmpIns;
        vector_each3(&bb->inst, tmpIns) { free(tmpIns); }
        vector_free(&bb->inst);
        vector_free(&bb->df);
        vector_free(&bb->postdf);
        vector_free(&bb->inedges);
        vector_free(&bb->doms);
    }
    // fprintf(fp, ".%s_ret:\n", fnc->name);
    char buff[1024];
    sprintf(buff, ".%s_ret", fnc->name);
    asmptr = createAsm9(ASM_label, strdup(buff));
    DL_APPEND(asmlist, asmptr);
    restoreCallee(fnc, fp);
    asmgen_functionEpilogue(fnc, fp);
    fprintf(fp, "\n");
}