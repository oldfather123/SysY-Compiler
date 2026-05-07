#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/bitset.h"
#include "../include/function.h"
#include "../include/ir.h"
#include "../include/m-bitset.h"
#include "../include/pass.h"
#include "../include/symbol.h"
#include "../include/sysy.h"
#include "../include/uthash.h"
#include "../include/util.h"
#include "../include/utlist.h"
#include "../include/value.h"
#include "../include/vector.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

static size_t nextNum;

static void   DFS(Function *fnc, ssaSymbol *curV);
static bool   isRegionConstant(Function *fnc, Value *v, BasicBlock *bb);
static Value  Replace(Function *fnc, instruction *ins, Value *iv, Value *rc);

vector_template(BasicBlock *, loopHeaders);
struct LoopNestingForest {
    BasicBlock *head;
    BasicBlock *back;
    vector_template(BasicBlock *, bb);
};
static vector_template(BasicBlock *, loopstack);
static vector_template(struct LoopNestingForest, LNFs);
static void LFTR(Function *fnc) {
    BasicBlock               *b;
    struct instruction       *inst = NULL;
    struct LoopNestingForest *looptmp;
    int                       i;
    vector_each_address(&LNFs, i, looptmp) {
        int _i;
        vector_each(&looptmp->bb, _i, b) {
            vector_each3(&b->inst, inst) {
                switch (inst->op) {
                    OP3_3AC_COND
                    if (inst->args[0].ty & IS_SSA) {
                        ssaSymbol *v =
                            getSSAlVariable(fnc, inst->args[0].ssaind);
                        if (v->Header && v->LFTR.v) {
                            Value val;
                            val = inst->args[1];
                            do {
                                int op = v->LFTR.op;
                                v = getSSAlVariable(fnc, v->LFTR.v);
                                // iv op rc,区域常量一直都是args[1]
                                // 折叠一下inst->args[1] op rc
                                // 区域常量也有可能是一个变量
                                uint32_t ssa = v->ssaind;
                                if (v->def->op == IR_OP_MOV) {
                                    v = getSSAlVariable(fnc,
                                                        v->def->args[0].ssaind);
                                }
                                Value tmpv;
                                if (v->def->op == IR_OP_PHI) {
                                    assert(0);
                                    tmpv = v->def->ret;
                                } else {
                                    tmpv = v->def->args[1];
                                }

                                if (!foldValue(op, &val, &tmpv, &val)) {
                                    instruction *ins =
                                        sy_malloc(sizeof(instruction));
                                    ins->args[0] = tmpv;
                                    ins->args[1] = val;
                                    ins->ret = createSSATempVariable(
                                        fnc, inst->args[0].ty);
                                    ins->op = op;
                                    val = ins->ret;
                                    // vector_insert(&b->inst, i, ins);
                                    DL_PREPEND_ELEM(b->instlist, inst, ins);
                                }
                                inst->args[1] = val;
                                inst->args[0].ssaind = ssa;
                            } while (v->LFTR.v != 0);

                            goto _end;
                        }
                    }
                    if (inst->args[1].ty & IS_SSA) {
                        ssaSymbol *v =
                            getSSAlVariable(fnc, inst->args[1].ssaind);
                        if (v->Header && v->LFTR.v) {
                            Value val;
                            val = inst->args[0];
                            do {
                                int op = v->LFTR.op;
                                v = getSSAlVariable(fnc, v->LFTR.v);
                                // iv op rc,区域常量一直都是args[1]
                                // 折叠一下inst->args[1] op rc
                                // 区域常量也有可能是一个变量
                                uint32_t ssa = v->ssaind;
                                if (v->def->op == IR_OP_MOV) {
                                    v = getSSAlVariable(fnc,
                                                        v->def->args[1].ssaind);
                                }

                                if (!foldValue(op, &val, &v->def->args[1],
                                               &val)) {
                                    instruction *ins =
                                        sy_malloc(sizeof(instruction));
                                    ins->args[0] = val;
                                    ins->args[1] = v->def->args[1];
                                    ins->ret = createSSATempVariable(
                                        fnc, inst->args[0].ty);
                                    ins->op = op;
                                    val = ins->ret;
                                    // vector_insert(&b->inst, i, ins);
                                    DL_PREPEND_ELEM(b->instlist, inst, ins);
                                }
                                inst->args[0] = val;
                                inst->args[1].ssaind = ssa;
                            } while (v->LFTR.v != 0);

                            goto _end;
                        }
                        // ssaSymbol *v = getSSAlVariable(fnc,
                        // inst->args[1].ssaind); if (v->Header && v->LFTR) {
                        //     do {
                        //         v = getSSAlVariable(fnc, v->LFTR);
                        //         // iv op rc,区域常量一直都是args[1]
                        //         // 折叠一下inst->args[1] op rc
                        //         Value con;
                        //         // 区域常量也有可能是一个变量
                        //         foldValue(v->def->op, &inst->args[0],
                        //                   &v->def->args[1], &con);
                        //         inst->args[0] = con;
                        //         inst->args[1].ssaind = v->ssaind;
                        //     } while (v->LFTR != 0);
                        // }
                    }
                _end:
                    break;
                }
            }
        }
    }

    // if (o1->ty & IS_SSA) {
    //     s1 = getSSAlVariable(fnc, o1->ssaind);
    // }
}
static void LFTR1(Function *fnc) {
    BasicBlock         *b;
    struct instruction *inst = NULL;
    vector_each3(&loopHeaders, b) {
        BasicBlock *ptr;
        vector_each(&b->inst, i, inst) {
            switch (inst->op) {
                OP3_3AC_COND
                if (inst->args[0].ty & IS_SSA) {
                    ssaSymbol *v = getSSAlVariable(fnc, inst->args[0].ssaind);
                    if (v->Header && v->LFTR.v) {
                        Value val;
                        val = inst->args[1];
                        do {
                            int op = v->LFTR.op;
                            v = getSSAlVariable(fnc, v->LFTR.v);
                            // iv op rc,区域常量一直都是args[1]
                            // 折叠一下inst->args[1] op rc
                            // 区域常量也有可能是一个变量
                            uint32_t ssa = v->ssaind;
                            if (v->def->op == IR_OP_MOV) {
                                v = getSSAlVariable(fnc,
                                                    v->def->args[0].ssaind);
                            }
                            Value tmpv;
                            if (v->def->op == IR_OP_PHI) {
                                assert(0);
                                tmpv = v->def->ret;
                            } else {
                                tmpv = v->def->args[1];
                            }

                            if (!foldValue(op, &val, &tmpv, &val)) {
                                instruction *ins =
                                    sy_malloc(sizeof(instruction));
                                ins->args[0] = tmpv;
                                ins->args[1] = val;
                                ins->ret = createSSATempVariable(
                                    fnc, inst->args[0].ty);
                                ins->op = op;
                                val = ins->ret;
                                // vector_insert(&b->inst, i, ins);
                                DL_PREPEND_ELEM(b->instlist, inst, ins);
                            }
                            inst->args[1] = val;
                            inst->args[0].ssaind = ssa;
                        } while (v->LFTR.v != 0);

                        goto _end;
                    }
                }
                if (inst->args[1].ty & IS_SSA) {
                    ssaSymbol *v = getSSAlVariable(fnc, inst->args[1].ssaind);
                    if (v->Header && v->LFTR.v) {
                        Value val;
                        val = inst->args[0];
                        do {
                            int op = v->LFTR.op;
                            v = getSSAlVariable(fnc, v->LFTR.v);
                            // iv op rc,区域常量一直都是args[1]
                            // 折叠一下inst->args[1] op rc
                            // 区域常量也有可能是一个变量
                            uint32_t ssa = v->ssaind;
                            if (v->def->op == IR_OP_MOV) {
                                v = getSSAlVariable(fnc,
                                                    v->def->args[1].ssaind);
                            }

                            if (!foldValue(op, &val, &v->def->args[1], &val)) {
                                instruction *ins =
                                    sy_malloc(sizeof(instruction));
                                ins->args[0] = val;
                                ins->args[1] = v->def->args[1];
                                ins->ret = createSSATempVariable(
                                    fnc, inst->args[0].ty);
                                ins->op = op;
                                val = ins->ret;
                                // vector_insert(&b->inst, i, ins);
                                DL_PREPEND_ELEM(b->instlist, inst, ins);
                            }
                            inst->args[0] = val;
                            inst->args[1].ssaind = ssa;
                        } while (v->LFTR.v != 0);

                        goto _end;
                    }
                    // ssaSymbol *v = getSSAlVariable(fnc,
                    // inst->args[1].ssaind); if (v->Header && v->LFTR) {
                    //     do {
                    //         v = getSSAlVariable(fnc, v->LFTR);
                    //         // iv op rc,区域常量一直都是args[1]
                    //         // 折叠一下inst->args[1] op rc
                    //         Value con;
                    //         // 区域常量也有可能是一个变量
                    //         foldValue(v->def->op, &inst->args[0],
                    //                   &v->def->args[1], &con);
                    //         inst->args[0] = con;
                    //         inst->args[1].ssaind = v->ssaind;
                    //     } while (v->LFTR != 0);
                    // }
                }
            _end:
                break;
            }
        }
        vector_each3(&b->doms, ptr) {
            vector_each(&ptr->inst, i, inst) {
                switch (inst->op) {
                    OP3_3AC_COND
                    // if (inst->args[0].ty & IS_SSA) {
                    //     ssaSymbol *v =
                    //         getSSAlVariable(fnc, inst->args[0].ssaind);
                    //     if (v->Header && v->_LFTR) {
                    //         getSSAlVariable(fnc, inst->args[0].ssaind);
                    //     }
                    // }
                    // if (inst->args[1].ty & IS_SSA) {
                    //     ssaSymbol *v =
                    //         getSSAlVariable(fnc, inst->args[1].ssaind);
                    //     if (v->Header && v->_LFTR) {
                    //         getSSAlVariable(fnc, inst->args[0].ssaind);
                    //     }
                    // }
                    break;
                }
            }
        }
    }
    // if (o1->ty & IS_SSA) {
    //     s1 = getSSAlVariable(fnc, o1->ssaind);
    // }
}
static bool isloopnode(BasicBlock *bb) {
    struct LoopNestingForest *looptmp;
    int                       i;
    vector_each_address(&LNFs, i, looptmp) {
        int         i;
        BasicBlock *b;
        vector_each3(&looptmp->bb, b) {
            if (bb == b) return true;
        }
    }
    return false;
}
bool        onepass;
static void OSR(Function *fnc) {
    onepass = true;
    nextNum = 0;
    BasicBlock         *bb = vector_get(&fnc->BBs, 3);
    size_t              i;
    struct instruction *inst = NULL;
    vector_template(struct instruction *, temp);
    // LOOPALLBB({
    BasicBlock               *b;
    struct LoopNestingForest *looptmp;
    vector_each_address(&LNFs, i, looptmp) {
        int i;
        if (onepass) {
            vector_copy(&temp, &looptmp->back->inst);
            vector_each(&temp, i, inst) {
                if (inst->ret.ty & IS_SSA) {
                    ssaSymbol *v = getSSAlVariable(fnc, inst->ret.ssaind);
                    if (!v->visited) {
                        DFS(fnc, v);
                    }
                }
            }
            onepass = false;
        }

        ssaSymbol *sym;

        vector_each3(&fnc->SSAValuePool, sym) {
            sym->Header = NULL;
            sym->visited = 0;
        }
        // vector_each_reverse(&looptmp->bb, i, b) {
        //     vector_copy(&temp, &b->inst);
        //     int i;
        //     vector_each(&temp, i, inst) {
        //         if (inst->ret.ty & IS_SSA) {
        //             ssaSymbol *v = getSSAlVariable(fnc, inst->ret.ssaind);
        //             if (!v->visited) {
        //                 DFS(fnc, v);
        //             }
        //         }
        //     }
        // }
    }
    // vector_each3(&loopHeaders, b) {
    //     BasicBlock *ptr;

    //     vector_each3(&b->doms, ptr) {
    //         if (!(isloopnode(ptr))) {
    //             continue;
    //         }
    //         vector_copy(&temp, &ptr->inst);
    //         vector_each(&temp, i, inst) {
    //             if (inst->ret.ty & IS_SSA) {
    //                 ssaSymbol *v = getSSAlVariable(fnc, inst->ret.ssaind);
    //                 if (!v->visited) {
    //                     DFS(fnc, v);
    //                 }
    //             }
    //         }
    //         vector_free(&temp);
    //     }
    // }
    //   })
}
/**
 * @brief OSR建立在Tarjan算法基础上的的强连接区域查找器
 * @param[in] *fnc
 */
static void OSR1(Function *fnc) {
    nextNum = 0;
    BasicBlock         *bb = vector_get(&fnc->BBs, 3);
    size_t              i;
    struct instruction *inst = NULL;
    vector_template(struct instruction *, temp);
    // LOOPALLBB({
    BasicBlock *b;
    vector_each3(&loopHeaders, b) {
        BasicBlock *ptr;

        vector_each3(&b->doms, ptr) {
            if (!(isloopnode(ptr))) {
                continue;
            }
            vector_copy(&temp, &ptr->inst);
            vector_each(&temp, i, inst) {
                if (inst->ret.ty & IS_SSA) {
                    ssaSymbol *v = getSSAlVariable(fnc, inst->ret.ssaind);
                    if (!v->visited) {
                        DFS(fnc, v);
                    }
                }
            }
            vector_free(&temp);
        }
        ssaSymbol *sym;
        vector_each3(&fnc->SSAValuePool, sym) { sym->Header = NULL; }
    }
    //   })
}
static vector_template(ssaSymbol *, stack);
static inline void       push(ssaSymbol *ins) { vector_push_back(&stack, ins); }
static inline ssaSymbol *pop() {
    ssaSymbol *ins;
    vector_pop_back(&stack, ins);
    return ins;
}
static inline bool isOnStack(ssaSymbol *ins) {
    ssaSymbol *ptr;
    vector_each3(&stack, ptr) {
        if (ptr == ins) {
            return true;
        }
    }
    return false;
}

// static bool isInductionVariable(Function *fnc, Value *v) {
//     if (!(v->ty & IS_SSA)) return false;
//     ssaSymbol *s = vector_get(&fnc->SSAValuePool, v->ssaind);
//     assert(s);
//     assert(s->def);
//     if (s->def->op == IR_OP_PHI) {
//         s->isIV = 1;
//         return true;
//     } else if (s->def->op == IR_OP_ADD || s->def->op == IR_OP_ADDF) {
//         if (isInductionVariable(fnc, &s->def->args[0]) &&
//             isRegionConstant(fnc, &s->def->args[1])) {
//             return true;
//         }
//         if (isInductionVariable(fnc, &s->def->args[1]) &&
//             isRegionConstant(fnc, &s->def->args[0])) {
//             return true;
//         }
//     } else if (s->def->op == IR_OP_SUB || s->def->op == IR_OP_SUBF) {
//         if (isInductionVariable(fnc, &s->def->args[0]) &&
//             isRegionConstant(fnc, &s->def->args[1])) {
//             return true;
//         }
//     } else if (s->def->op == IR_OP_MOV) {
//         if (isInductionVariable(fnc, &s->def->args[0])) {
//             return true;
//         }
//     }
//     return false;
// }
// 如果包含值定义的 cfg 节点严格支配由值使用的标头标签给出的 cfg
// 节点，则该值是循环不变的
static bool isRegionConstant(Function *fnc, Value *v, BasicBlock *bb) {
    BasicBlock *defBB = NULL;
    if (v->ty & IS_IMM) {
        return true;
    } else if (v->ty & IS_SSA) {
        ssaSymbol *s = getSSAlVariable(fnc, v->ssaind);
        LOOPALLBB({LOOPBBALLINST({
            if (inst == s->def) {
                defBB = bb;
            }
        })})
        assert(defBB);
        // 严格支配
        if (defBB == bb) return false;
        if (bb->idom == defBB) return true;
        return isDominetor(fnc, defBB, bb);
        BasicBlock *ptr;
        vector_each3(&defBB->doms, ptr) {
            if (ptr == bb) {
                return true;
            }
        }
    }
    return false;
}
/* x ← c × i
x ← i × c
x ← c + i
x ← i + c
x ← i-c
Candidate Operations
 */
// static bool isCandidateOperation(Function *fnc, instruction *ins) {
//     switch (ins->op) {
//             // case IR_OP_MUL:
//             // case IR_OP_MULF:
//             // case IR_OP_ADD:
//             // case IR_OP_ADDF:
//             //     if (isInductionVariable(fnc, &ins->args[0]) &&
//             //         isRegionConstant(fnc, &ins->args[1])) {
//             //         return true;
//             //     } else if (isInductionVariable(fnc, &ins->args[1]) &&
//             //                isRegionConstant(fnc, &ins->args[0])) {
//             //         return true;
//             //     }
//             //     break;
//             // case IR_OP_SUB:
//             // case IR_OP_SUBF:
//             //     if (isInductionVariable(fnc, &ins->args[0]) &&
//             //         isRegionConstant(fnc, &ins->args[1])) {
//             //         return true;
//             //     }
//             //     break;
//         case IR_OP_MUL:
//         case IR_OP_MULF:
//         case IR_OP_ADD:
//         case IR_OP_ADDF:
//             if (isInductionVariable(fnc, &ins->args[0]) &&
//                 isRegionConstant(fnc, &ins->args[1])) {
//                 return true;
//             } else if (isInductionVariable(fnc, &ins->args[1]) &&
//                        isRegionConstant(fnc, &ins->args[0])) {
//                 return true;
//             }
//             break;
//         case IR_OP_SUB:
//         case IR_OP_SUBF:
//             if (isInductionVariable(fnc, &ins->args[0]) &&
//                 isRegionConstant(fnc, &ins->args[1])) {
//                 return true;
//             }
//             break;
//     }
//     return false;
// }
static bool isInductionVariable1(Function *fnc, bitset_t SCC, Value *v) {
    if (!(v->ty & IS_SSA)) return false;
    ssaSymbol *s = vector_get(&fnc->SSAValuePool, v->ssaind);
    return bitset_get(SCC, s->ssaind);
}
// ClassifyIV(N)
// IsIV ← true
// for each node n ∈ N do
// if n is not a valid update for
// an induction variable then
// IsIV ← false
static void __ClassifyIV1(Function *fnc, bitset_t N) {
    // SCC 中深度优先编号最低的节点
    ssaSymbol  *header;
    bool        first = true;
    bitset_it_t it;
    for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            ssaSymbol *v = getSSAlVariable(fnc, it->index);
            if (first) {
                first = false;
                header = v;
            }
            if (v->Num < header->Num) {
                header = v;
            }
        }
    }
    BasicBlock *headerBB = NULL;
    LOOPALLBB({LOOPBBALLINST({
        if (inst == header->def) {
            headerBB = bb;
        }
    })})

    bool   isIV = true;
    size_t n;
    // 这里保证SCC中所有更新都有效
    for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            ssaSymbol *v = getSSAlVariable(fnc, it->index);
            /* 其中更新其值的每个操作都是
            (1) 归纳变量之一 加上一个区域常数，
            (2)
             * 一个归纳变量减去一个区域常数，
             (3) 一个 φ 函数，或
             (4)来自另一个归纳变量的寄存器到寄存器的副本 */
            switch (v->def->op) {
                case IR_OP_PHI:
                    // {
                    //     phiFunction *phi = v->def->args[0].phi;
                    //     phiParam    *p;
                    //     size_t       i;
                    //     vector_each_address(&phi->param, i, p) {
                    //         if (!isInductionVariable1(fnc, N, &p->v) &&
                    //             !isRegionConstant(fnc, &p->v, headerBB)) {
                    //             isIV = false;
                    //         }
                    //     }
                    // }
                    break;
                case IR_OP_ADD:
                case IR_OP_ADDF:
                case IR_OP_SUB:
                case IR_OP_SUBF:
                    if (!isInductionVariable1(fnc, N, &v->def->args[0]) &&
                        !isRegionConstant(fnc, &v->def->args[0], headerBB)) {
                        isIV = false;
                    }
                    if (!isInductionVariable1(fnc, N, &v->def->args[1]) &&
                        !isRegionConstant(fnc, &v->def->args[1], headerBB)) {
                        isIV = false;
                    }
                    break;
                case IR_OP_MOV:
                    if (!isInductionVariable1(fnc, N, &v->def->args[0])) {
                        isIV = false;
                    }
                    break;
            }
        }
    }
    if (isIV) {
        for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
            if (*bitset_cref(it)) {
                ssaSymbol *v = getSSAlVariable(fnc, it->index);
                v->Header = header->def;
            }
        }
    } else {
        for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
            if (*bitset_cref(it)) {
                ssaSymbol *v = getSSAlVariable(fnc, it->index);
                // if (isCandidateOperation(fnc, v->def)) {
                //     Replace(fnc, v->def, iv, rc);
                // }
                switch (v->def->op) {
                    case IR_OP_ADD:
                    case IR_OP_ADDF:
                    case IR_OP_MUL:
                    case IR_OP_MULF:
                        if (isInductionVariable1(fnc, N, &v->def->args[1]) &&
                            isRegionConstant(fnc, &v->def->args[0], headerBB)) {
                            Replace(fnc, v->def, &v->def->args[1],
                                    &v->def->args[0]);
                        } else if (isInductionVariable1(fnc, N,
                                                        &v->def->args[0]) &&
                                   isRegionConstant(fnc, &v->def->args[1],
                                                    headerBB)) {
                            Replace(fnc, v->def, &v->def->args[0],
                                    &v->def->args[1]);
                        } else {
                            v->Header = NULL;
                        }
                        break;
                    case IR_OP_SUB:
                    case IR_OP_SUBF:
                        if (isInductionVariable1(fnc, N, &v->def->args[0]) &&
                            isRegionConstant(fnc, &v->def->args[1], headerBB)) {
                            Replace(fnc, v->def, &v->def->args[0],
                                    &v->def->args[1]);
                        } else {
                            v->Header = NULL;
                        }
                        break;
                    default:
                        v->Header = NULL;
                }
            }
        }
    }
}
static void ClassifyIV12(Function *fnc, bitset_t N) {
    // SCC 中深度优先编号最低的节点
    ssaSymbol  *header;
    bool        first = true;
    bitset_it_t it;
    for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            ssaSymbol *v = getSSAlVariable(fnc, it->index);
            if (first) {
                first = false;
                header = v;
            }
            if (v->Num < header->Num) {
                header = v;
            }
        }
    }
    BasicBlock *headerBB = NULL;
    LOOPALLBB({LOOPBBALLINST({
        if (inst == header->def) {
            headerBB = bb;
        }
    })})

    bool   isIV = true;
    size_t n;
    // 这里保证SCC中所有更新都有效
    for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            ssaSymbol *v = getSSAlVariable(fnc, it->index);
            /* 其中更新其值的每个操作都是
            (1) 归纳变量之一 加上一个区域常数，
            (2)
             * 一个归纳变量减去一个区域常数，
             (3) 一个 φ 函数，或
             (4)来自另一个归纳变量的寄存器到寄存器的副本 */
            switch (v->def->op) {
                case IR_OP_PHI:
                    break;
                case IR_OP_ADD:
                case IR_OP_ADDF:
                case IR_OP_SUB:
                case IR_OP_SUBF:
                    if (!bitset_get(N, v->def->args[0].ssaind) &&
                        !isRegionConstant(fnc, &v->def->args[0], headerBB)) {
                        isIV = false;
                    }
                    if (!bitset_get(N, v->def->args[1].ssaind) &&
                        !isRegionConstant(fnc, &v->def->args[1], headerBB)) {
                        isIV = false;
                    }
                    break;
                case IR_OP_MOV:
                    if (!bitset_get(N, v->def->args[0].ssaind) &&
                        !isRegionConstant(fnc, &v->def->args[0], headerBB)) {
                        isIV = false;
                    }
                    break;
                    // 不是这4种操作就不是归纳变量
                default:
                    isIV = false;
            }
        }
    }
    if (isIV) {
        for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
            if (*bitset_cref(it)) {
                ssaSymbol *v = getSSAlVariable(fnc, it->index);
                v->Header = header->def;
            }
        }
    } else {
        ssaSymbol *r1, *r2;
        for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
            if (*bitset_cref(it)) {
                r1 = NULL;
                r2 = NULL;
                ssaSymbol *v = getSSAlVariable(fnc, it->index);
                // if (isCandidateOperation(fnc, v->def)) {
                //     Replace(fnc, v->def, iv, rc);
                // }
                switch (v->def->op) {
                    case IR_OP_ADD:
                    case IR_OP_ADDF:
                    case IR_OP_MUL:
                    case IR_OP_MULF:
                    case IR_OP_ADDPTR:
                        if (v->def->args[0].ty & IS_SSA) {
                            r1 = getSSAlVariable(fnc, v->def->args[0].ssaind);
                        }
                        if (v->def->args[1].ty & IS_SSA) {
                            r2 = getSSAlVariable(fnc, v->def->args[1].ssaind);
                        }
                        if (r2 && r2->Header &&
                            isRegionConstant(fnc, &v->def->args[0], headerBB)) {
                            Replace(fnc, v->def, &v->def->args[1],
                                    &v->def->args[0]);
                        } else if (r1 && r1->Header &&
                                   isRegionConstant(fnc, &v->def->args[1],
                                                    headerBB)) {
                            Replace(fnc, v->def, &v->def->args[0],
                                    &v->def->args[1]);
                        } else {
                            v->Header = NULL;
                            // 用值 NULL 标记它以指示它不是归纳变量
                        }
                        break;
                    case IR_OP_SUB:
                    case IR_OP_SUBF:
                        if (bitset_get(N, v->def->args[0].ssaind) &&
                            isRegionConstant(fnc, &v->def->args[1], headerBB)) {
                            Replace(fnc, v->def, &v->def->args[0],
                                    &v->def->args[1]);
                        } else {
                            v->Header = NULL;
                        }
                        break;
                    default:
                        v->Header = NULL;
                }
            }
        }
    }
}
static void ClassifyIV1(Function *fnc, bitset_t N) {
    // SCC 中深度优先编号最低的节点
    ssaSymbol  *header;
    bool        first = true;
    bitset_it_t it;
    for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            ssaSymbol *v = getSSAlVariable(fnc, it->index);
            if (first) {
                first = false;
                header = v;
            }
            if (v->Num < header->Num) {
                header = v;
            }
        }
    }
    BasicBlock *headerBB = NULL;
    LOOPALLBB({LOOPBBALLINST({
        if (inst == header->def) {
            headerBB = bb;
        }
    })})

    bool   isIV = true;
    size_t n;
    // 这里保证SCC中所有更新都有效
    for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
        if (*bitset_cref(it)) {
            ssaSymbol *v = getSSAlVariable(fnc, it->index);
            /* 其中更新其值的每个操作都是
            (1) 归纳变量之一 加上一个区域常数，
            (2)
             * 一个归纳变量减去一个区域常数，
             (3) 一个 φ 函数，或
             (4)来自另一个归纳变量的寄存器到寄存器的副本 */
            switch (v->def->op) {
                case IR_OP_PHI:
                    v->Header = header->def;
                    break;
                case IR_OP_ADD:
                case IR_OP_ADDF:
                case IR_OP_SUB:
                case IR_OP_SUBF: {
                    if (v->def->args[0].ty & IS_SSA) {
                        ssaSymbol *s =
                            getSSAlVariable(fnc, v->def->args[0].ssaind);
                        if (s->Header || s->def->op == IR_OP_PHI) {
                            if (isRegionConstant(fnc, &v->def->args[1],
                                                 headerBB)) {
                                v->Header = header->def;
                                break;
                            }
                        }
                    }
                    if (v->def->args[1].ty & IS_SSA) {
                        ssaSymbol *s =
                            getSSAlVariable(fnc, v->def->args[1].ssaind);
                        if (s->Header || s->def->op == IR_OP_PHI) {
                            if (isRegionConstant(fnc, &v->def->args[0],
                                                 headerBB)) {
                                v->Header = header->def;
                                break;
                            }
                        }
                    }
                    isIV = false;
                    break;
                }

                case IR_OP_MOV: {
                    ssaSymbol *s = getSSAlVariable(fnc, v->def->args[0].ssaind);
                    if (!s->Header || s->def->op == IR_OP_PHI) {
                        isIV = false;
                    } else {
                        v->Header = header->def;
                    }
                } break;
                    // 不是这4种操作就不是归纳变量
                default:
                    isIV = false;
            }
        }
    }
    if (isIV) {
        for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
            if (*bitset_cref(it)) {
                ssaSymbol *v = getSSAlVariable(fnc, it->index);
                v->Header = header->def;
            }
        }
    } else {
        ssaSymbol *r1, *r2;
        for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
            if (*bitset_cref(it)) {
                r1 = NULL;
                r2 = NULL;
                ssaSymbol *v = getSSAlVariable(fnc, it->index);
                // if (isCandidateOperation(fnc, v->def)) {
                //     Replace(fnc, v->def, iv, rc);
                // }
                switch (v->def->op) {
                    case IR_OP_ADD:
                    case IR_OP_ADDF:
                    case IR_OP_MUL:
                    case IR_OP_MULF:
                    case IR_OP_ADDPTR:
                        if (v->def->args[0].ty & IS_SSA) {
                            r1 = getSSAlVariable(fnc, v->def->args[0].ssaind);
                        }
                        if (v->def->args[1].ty & IS_SSA) {
                            r2 = getSSAlVariable(fnc, v->def->args[1].ssaind);
                        }
                        if (r2 && r2->Header &&
                            isRegionConstant(fnc, &v->def->args[0], headerBB)) {
                            Replace(fnc, v->def, &v->def->args[1],
                                    &v->def->args[0]);
                        } else if (r1 && r1->Header &&
                                   isRegionConstant(fnc, &v->def->args[1],
                                                    headerBB)) {
                            Replace(fnc, v->def, &v->def->args[0],
                                    &v->def->args[1]);
                        } else {
                            v->Header = NULL;
                            // 用值 NULL 标记它以指示它不是归纳变量
                        }
                        break;
                    case IR_OP_SUB:
                    case IR_OP_SUBF:
                        if (bitset_get(N, v->def->args[0].ssaind) &&
                            isRegionConstant(fnc, &v->def->args[1], headerBB)) {
                            Replace(fnc, v->def, &v->def->args[0],
                                    &v->def->args[1]);
                        } else {
                            v->Header = NULL;
                        }
                        break;
                    default:
                        v->Header = NULL;
                }
            }
        }
    }
}
// static void ClassifyIV(Function *fnc, bitset_t N) {
//     bool        isIV = true;
//     size_t      n;
//     bitset_it_t it;
//     // 这里保证SCC中所有更新都有效
//     for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
//         if (*bitset_cref(it)) {
//             ssaSymbol *v = getSSAlVariable(fnc, it->index);
//             /* 其中更新其值的每个操作都是
//             (1) 归纳变量之一 加上一个区域常数，
//             (2)
//              * 一个归纳变量减去一个区域常数，
//              (3) 一个 φ 函数，或
//              (4)来自另一个归纳变量的寄存器到寄存器的副本 */
//             bool       validupdate = false;

//             if (v->def->op == IR_OP_PHI) {
//                 validupdate = true;
//             } else if (v->def->op == IR_OP_ADD || v->def->op ==
// IR_OP_ADDF) {
//                 if (isInductionVariable(fnc, &v->def->args[0]) &&
//                     isRegionConstant(fnc, &v->def->args[1])) {
//                     validupdate = true;
//                 }
//                 if (isInductionVariable(fnc, &v->def->args[1]) &&
//                     isRegionConstant(fnc, &v->def->args[0])) {
//                     validupdate = true;
//                 }
//             } else if (v->def->op == IR_OP_SUB || v->def->op ==
// IR_OP_SUBF) {
//                 if (isInductionVariable(fnc, &v->def->args[0]) &&
//                     isRegionConstant(fnc, &v->def->args[1])) {
//                     validupdate = true;
//                 }
//             } else if (v->def->op == IR_OP_MOV) {
//                 if (isInductionVariable(fnc, &v->def->args[0])) {
//                     validupdate = true;
//                 }
//             }
//             if (!validupdate) isIV = false;
//         }
//     }
//     if (isIV) {
//         // SCC 中深度优先编号最低的节点
//         ssaSymbol *header;
//         bool       first = true;
//         for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
//             if (*bitset_cref(it)) {
//                 ssaSymbol *v = getSSAlVariable(fnc, it->index);
//                 if (first) {
//                     first = false;
//                     header = v;
//                 }
//                 if (v->def->Num < header->def->Num) {
//                     header = v;
//                 }
//             }
//         }
//         for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
//             if (*bitset_cref(it)) {
//                 ssaSymbol *v = getSSAlVariable(fnc, it->index);
//                 v->Header = header->def;
//             }
//         }
//     } else {
//         for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
//             if (*bitset_cref(it)) {
//                 ssaSymbol *v = getSSAlVariable(fnc, it->index);
//                 if (isCandidateOperation(fnc, v->def)) {
//                     // Replace(n, iv, rc)
//                 }
//             }
//         }
//     }
// }

// static void Reduce(Function *fnc, TAC op, Value *iv, Value *rc) {
//     // createSSATempVariable(fnc, );
// }
static instruction *clone(Function *fnc, instruction *i, Value v) {
    instruction *copy = (instruction *)sy_malloc(sizeof(instruction));
    memcpy(copy, i, sizeof(instruction));
    copy->ret = v;
    return copy;
}
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
typedef struct valueItem {
    valueNumber args[2];
    TAC         op;
} valueItem;
typedef struct valueTable {
    valueItem      key;
    Value          value;
    UT_hash_handle hh;
} valueTable;
valueTable        *valueHH;
static valueTable *insert(Function *fnc, TAC op, Value *iv, Value *rc,
                          Value v) {
    valueItem key;
    key.op = op;
    if (iv->ty & IS_IMM) {
        if (iv->ty & TY_FLOAT) {
            key.args[0].ty = VN_FLOAT;
            key.args[0].f = getImmFloatValue(iv);
        } else if (iv->ty & TY_INT) {
            key.args[0].ty = VN_INT;
            key.args[0].i = getImmIntValue(iv);
        }
    } else if (iv->ty & IS_SSA) {
        key.args[0].ty = VN_VAR;
        key.args[0].var = iv->ssaind;
    } else {
        assert(0);
    }
    if (rc->ty & IS_IMM) {
        if (rc->ty & TY_FLOAT) {
            key.args[1].ty = VN_FLOAT;
            key.args[1].f = getImmFloatValue(rc);
        } else if (rc->ty & TY_INT) {
            key.args[1].ty = VN_INT;
            key.args[1].i = getImmIntValue(rc);
        }
    } else if (rc->ty & IS_SSA) {
        key.args[1].ty = VN_VAR;
        key.args[1].var = rc->ssaind;
    } else {
        assert(0);
    }
    valueTable *ptr = (valueTable *)sy_malloc(sizeof(valueTable));
    ptr->key = key;
    ptr->value = v;
    HASH_ADD(hh, valueHH, key, sizeof(valueItem), ptr);
}
static valueTable *lookUp(Function *fnc, TAC op, Value *iv, Value *rc) {
    valueItem key;
    key.op = op;
    if (iv->ty & IS_IMM) {
        if (iv->ty & TY_FLOAT) {
            key.args[0].ty = VN_FLOAT;
            key.args[0].f = getImmFloatValue(iv);
        } else if (iv->ty & TY_INT) {
            key.args[0].ty = VN_INT;
            key.args[0].i = getImmIntValue(iv);
        }
    } else if (iv->ty & IS_SSA) {
        key.args[0].ty = VN_VAR;
        key.args[0].var = iv->ssaind;
    } else {
        assert(0);
    }
    if (rc->ty & IS_IMM) {
        if (rc->ty & TY_FLOAT) {
            key.args[1].ty = VN_FLOAT;
            key.args[1].f = getImmFloatValue(rc);
        } else if (rc->ty & TY_INT) {
            key.args[1].ty = VN_INT;
            key.args[1].i = getImmIntValue(rc);
        }
    } else if (rc->ty & IS_SSA) {
        key.args[1].ty = VN_VAR;
        key.args[1].var = rc->ssaind;
    } else {
        assert(0);
    }
    valueTable *ptr;
    valueTable  vn;
    vn.key = key;
    HASH_FIND(hh, valueHH, &vn.key, sizeof(valueItem), ptr);
    return ptr;
}
static Value Reduce1(Function *fnc, TAC op, Value *iv, Value *rc);
static Value Apply1(Function *fnc, TAC op, Value *o1, Value *o2);
instruction *__replace;
instruction *copyinst(instruction *i);
static void  osrinsertIR(Function *fnc, instruction *ins, instruction *insert);
static Value Replace(Function *fnc, instruction *ins, Value *iv, Value *rc) {
    printf("强度削弱成功！");
    __replace = ins;
    Value        result = Reduce1(fnc, ins->op, iv, rc);
    instruction *ptr;
    ptr = copyinst(ins);
    osrinsertIR(fnc, ins, ptr);
    ins = ptr;

    // 重写该指令
    //  instruction *copy = (instruction *)sy_malloc(sizeof(instruction));
    ins->op = IR_OP_MOV;
    ins->args[0] = result;
    ssaSymbol *v = getSSAlVariable(fnc, result.ssaind);
    ssaSymbol *iv_s = getSSAlVariable(fnc, iv->ssaind);
    // copy->op = IR_OP_MOV;
    // copy->args[0] = result;
    // copy->ret = ins->ret;
    // ssaSymbol *v = getSSAlVariable(fnc, result.ssaind);
    // ssaSymbol *iv_s = getSSAlVariable(fnc, iv->ssaind);
    // LOOPALLBB({LOOPBBALLINST({
    //     if (inst == ins) {
    //         vector_insert(&bb->inst, i + 1, copy);
    //     }
    // })})
    v->Header = iv_s->Header;
}
static Value Reduce12(Function *fnc, TAC op, Value *iv, Value *rc) {
    valueTable *result = lookUp(fnc, op, iv, rc);
    uint32_t    newValue;
    if (!result) {
        // 创建归纳变量的副本
        Value v = createSSATempVariable(fnc, iv->ty);
        // 将该指令进行处理

        // TODO new name的def未写入,还有指令插入那里

        insert(fnc, op, iv, rc, v);
        ssaSymbol *iv_s = vector_get(&fnc->SSAValuePool, iv->ssaind);

        assert(iv_s->def);
        // 克隆iv的定义,并用new name替换
        // 用new name替换定义,并对操作数进行操作
        // instruction *copy = clone(fnc, iv_s->def, v);
        // copy->marked = true;

        instruction *copy = iv_s->def;

        // LOOPALLBB({LOOPBBALLINST({
        //     if (inst == iv_s->def) {
        //         vector_insert(&bb->inst, i + 1, copy);
        //     }
        // })})
        // instruction *copy = clone(fnc, iv_s->def, v);
        iv_s->def = NULL;

        // 此归纳变量的定义已经被删除
        copy->ret.ssaind = v.ssaind;
        ssaSymbol *new_s = vector_get(&fnc->SSAValuePool, v.ssaind);

        new_s->def = copy;
        new_s->Header = iv_s->Header;

        // instruction *copy = clone(fnc, iv_s->def, v);
        // ssaSymbol   *new_s = vector_get(&fnc->SSAValuePool, v.ssaind);
        // new_s->Header = iv_s->Header;
        switch (copy->op) {
            case IR_OP_ADDPTR:
                OP3_3AC_COND
                OP3_3AC_OPER
                for (size_t i = 0; i < 2; i++) {
                    if (copy->args[i].ty & IS_SSA ||
                        copy->args[i].ty & IS_IMM) {
                        ssaSymbol *o_s = vector_get(&fnc->SSAValuePool,
                                                    copy->args[i].ssaind);
                        if (o_s->Header != NULL &&
                            o_s->Header == iv_s->Header) {
                            copy->args[i] =
                                Reduce1(fnc, op, &copy->args[i], rc);
                        } else if (op == IR_OP_MUL || op == IR_OP_MULF) {
                            copy->args[i] = Apply1(fnc, op, &copy->args[i], rc);
                            // iv_s->LFTR = copy->ret.ssaind;
                        }
                    }
                }
                break;
            case IR_OP_MOV:
                if (copy->args[0].ty & IS_SSA || copy->args[0].ty & IS_IMM) {
                    ssaSymbol *o_s =
                        vector_get(&fnc->SSAValuePool, copy->args[0].ssaind);
                    if (o_s->Header != NULL && o_s->Header == iv_s->Header) {
                        copy->args[0] = Reduce1(fnc, op, &copy->args[0], rc);
                    } else if (op == IR_OP_MUL || op == IR_OP_MULF) {
                        copy->args[0] = Apply1(fnc, op, &copy->args[0], rc);
                        // iv_s->LFTR = copy->ret.ssaind;
                    }
                }
                break;
            case IR_OP_PHI: {
                phiFunction *phi = copy->args[0].phi;
                phiParam    *p;
                size_t       i;
                vector_each_address(&phi->param, i, p) {
                    ssaSymbol *o_s = getSSAlVariable(fnc, p->v.ssaind);
                    assert(o_s->def);
                    if (o_s->Header != NULL && o_s->Header == iv_s->Header) {
                        p->v = Reduce1(fnc, op, &p->v, rc);
                    } else {
                        p->v = Apply1(fnc, op, &p->v, rc);
                        //  iv_s->LFTR = p->v.ssaind;
                    }
                }
            } break;
            default:
                assert(0);
        }
        return v;
    }
    return result->value;
}

static Value Reduce1(Function *fnc, TAC op, Value *iv, Value *rc) {
    valueTable *result = lookUp(fnc, op, iv, rc);
    uint32_t    newValue;
    if (!result) {
        // 创建归纳变量的副本
        Value v = createSSATempVariable(fnc, iv->ty);
        if (v.ssaind == 72) {
            int i = 1;
        }
        // 将该指令进行处理

        // TODO new name的def未写入,还有指令插入那里

        insert(fnc, op, iv, rc, v);
        ssaSymbol *iv_s = vector_get(&fnc->SSAValuePool, iv->ssaind);

        assert(iv_s->def);
        // 克隆iv的定义,并用new name替换
        // 用new name替换定义,并对操作数进行操作
        instruction *copy = copyinst(iv_s->def);
        // BUG这里我不知道该不该标记
        copy->marked = true;

        osrinsertIR(fnc, iv_s->def, copy);

        // LOOPALLBB({LOOPBBALLINST({
        //     if (inst == iv_s->def) {
        //         vector_insert(&bb->inst, i + 1, copy);
        //     }
        // })})
        // instruction *copy = clone(fnc, iv_s->def, v);

        // 此归纳变量的定义已经被删除
        copy->ret.ssaind = v.ssaind;
        ssaSymbol *new_s = vector_get(&fnc->SSAValuePool, v.ssaind);
        new_s->visited = true;

        new_s->def = copy;
        new_s->Header = iv_s->Header;

        // instruction *copy = clone(fnc, iv_s->def, v);
        // ssaSymbol   *new_s = vector_get(&fnc->SSAValuePool, v.ssaind);
        // new_s->Header = iv_s->Header;
        switch (copy->op) {
            case IR_OP_ADDPTR:
                OP3_3AC_COND
                OP3_3AC_OPER
                for (size_t i = 0; i < 2; i++) {
                    if (copy->args[i].ty & IS_SSA ||
                        copy->args[i].ty & IS_IMM) {
                        ssaSymbol *o_s = vector_get(&fnc->SSAValuePool,
                                                    copy->args[i].ssaind);
                        if (o_s->Header != NULL &&
                            o_s->Header == iv_s->Header) {
                            copy->args[i] =
                                Reduce1(fnc, op, &copy->args[i], rc);
                            // iv_s->LFTR.v = copy->args[0].ssaind;
                            // iv_s->LFTR.op = op;

                        } else if (op == IR_OP_MUL || op == IR_OP_MULF) {
                            copy->args[i] = Apply1(fnc, op, &copy->args[i], rc);
                            iv_s->LFTR.v = copy->args[i].ssaind;
                            iv_s->LFTR.op = op;
                            // iv_s->LFTR = copy->args[i].ssaind;
                        }
                    }
                }
                break;
            case IR_OP_MOV:
                if (copy->args[0].ty & IS_SSA || copy->args[0].ty & IS_IMM) {
                    ssaSymbol *o_s =
                        vector_get(&fnc->SSAValuePool, copy->args[0].ssaind);
                    if (o_s->Header != NULL && o_s->Header == iv_s->Header) {
                        copy->args[0] = Reduce1(fnc, op, &copy->args[0], rc);
                        // iv_s->LFTR.v = copy->args[0].ssaind;
                        // iv_s->LFTR.op = op;
                    } else if (op == IR_OP_MUL || op == IR_OP_MULF) {
                        copy->args[0] = Apply1(fnc, op, &copy->args[0], rc);
                        iv_s->LFTR.v = copy->args[0].ssaind;
                        iv_s->LFTR.op = op;
                        // iv_s->LFTR = copy->args[0].ssaind;
                    }
                }
                break;
            case IR_OP_PHI: {
                phiFunction *phi = copy->args[0].phi;
                phiParam    *p;
                size_t       i;
                vector_each_address(&phi->param, i, p) {
                    ssaSymbol *o_s = getSSAlVariable(fnc, p->v.ssaind);
                    if (o_s->ty & TY_INT) {
                        p->v.ty |= TY_INT;
                    } else if (o_s->ty & TY_FLOAT) {
                        p->v.ty |= TY_FLOAT;
                    }
                    assert(o_s->def);
                    if (o_s->Header != NULL && o_s->Header == iv_s->Header) {
                        p->v = Reduce1(fnc, op, &p->v, rc);
                        // iv_s->LFTR.v = p->v.ssaind;
                        // iv_s->LFTR.op = op;

                    } else {
                        p->v = Apply1(fnc, op, &p->v, rc);
                        if (o_s->Header != NULL) {
                            printValue(stdout, fnc, &p->v);
                            // iv_s->LFTR.v = p->v.ssaind;
                            // iv_s->LFTR.op = op;
                        }
                    }
                }
            } break;
            default:
                assert(0);
        }
        return v;
    }
    return result->value;
}

// static Value Reduce(Function *fnc, TAC op, Value *iv, Value *rc) {
//     valueTable *result = lookUp(fnc, op, iv, rc);
//     uint32_t    newValue;
//     if (!result) {
//         // 创建归纳变量的副本
//         Value v = createSSATempVariable(fnc, iv->ty);
//         // TODO new name的def未写入,还有指令插入那里

//         insert(fnc, op, iv, rc, v);
//         ssaSymbol *iv_s = vector_get(&fnc->SSAValuePool, iv->ssaind);
//         assert(iv_s->def);
//         // 用new name替换定义,并对操作数进行操作
//         // instruction *copy = clone(fnc, iv_s->def, v);
//         instruction *copy = iv_s->def;
//         iv_s->def = NULL;
//         // 此归纳变量的定义已经被删除
//         copy->ret.ssaind = v.ssaind;
//         ssaSymbol *new_s = vector_get(&fnc->SSAValuePool, v.ssaind);
//         new_s->def = copy;
//         new_s->Header = iv_s->Header;
//         switch (copy->op) {
//             OP3_3AC_OPER
//             for (size_t i = 0; i < 2; i++) {
//                 if (copy->args[i].ty & IS_SSA) {
//                     ssaSymbol *o_s =
//                         vector_get(&fnc->SSAValuePool,
//  copy->args[i].ssaind);
//                     if (o_s->Header == iv_s->Header) {
//                         copy->args[i] = Reduce(fnc, op, &copy->args[i],
//  rc);
//                     } else if (op == IR_OP_MUL || op == IR_OP_MULF) {
//                         copy->args[i] = Apply(fnc, op, &copy->args[i],
//  rc);
//                     }
//                 }
//             }
//             break;
//             case IR_OP_MOV:
//                 if (copy->args[0].ty & IS_SSA) {
//                     ssaSymbol *o_s =
//                         vector_get(&fnc->SSAValuePool,
//  copy->args[0].ssaind);
//                     if (o_s->Header == iv_s->Header) {
//                         copy->args[0] = Reduce(fnc, op, &copy->args[0],
//  rc);
//                     } else if (op == IR_OP_MUL || op == IR_OP_MULF) {
//                         copy->args[0] = Apply(fnc, op, &copy->args[0],
//  rc);
//                     }
//                 }
//                 break;
//             case IR_OP_PHI: {
//                 phiFunction *phi = copy->args[0].phi;
//                 phiParam    *p;
//                 size_t       i;
//                 vector_each_address(&phi->param, i, p) {
//                     ssaSymbol *o_s = getSSAlVariable(fnc, p->v.ssaind);
//                     assert(o_s->def);
//                     if (o_s->Header == iv_s->Header) {
//                         p->v = Reduce(fnc, op, &p->v, rc);
//                     } else {
//                         // replace o with Apply(op, o, rc)
//                         p->v = Apply1(fnc, op, &p->v, rc);
//                     }
//                 }
//             } break;
//             default:
//                 assert(0);
//         }
//         return v;
//     }
//     return result->value;
// }

static BasicBlock *findDominator(Function *fnc, Value *v) {
    assert(v->ty & IS_SSA);
    ssaSymbol *ssa = getSSAlVariable(fnc, v->ssaind);
    assert(ssa->def);
    LOOPALLBB({LOOPBBALLINST({
        if (ssa->def == inst) {
            return bb->idom;
        }
    })})
}
static BasicBlock *findDefintionBB(Function *fnc, Value *v) {
    assert(v->ty & IS_SSA);
    ssaSymbol *ssa = getSSAlVariable(fnc, v->ssaind);
    assert(ssa->def);
    LOOPALLBB({LOOPBBALLINST({
        if (ssa->def == inst) {
            return bb;
        }
    })})
}
// Apply(op, o1, o2)
// static Value Apply(Function *fnc, TAC op, Value *o1, Value *o2) {
//     valueTable *result = lookUp(fnc, op, o1, o2);
//     BasicBlock *bb;
//     if (!result) {
//         if (isInductionVariable(fnc, o1) && isRegionConstant(fnc, o2)) {
//             // result ← Reduce(op, o1, o2)
//             return Reduce(fnc, op, o1, o2);
//         } else if (isInductionVariable(fnc, o2) && isRegionConstant(fnc,
// o1))
//         {
//             return Reduce(fnc, op, o2, o1);
//         } else {
//             // TODO 这里可能BUG
//             Value v = createSSATempVariable(fnc, o1->ty);
//             // TODO new name的def未写入,还有指令插入那里

//             insert(fnc, op, o1, o2, v);
//             if (o1->ty & IS_SSA && o2->ty & IS_SSA) {
//                 assert(0);
//             } else if (o1->ty & IS_SSA && o2->ty & IS_IMM) {
//                 bb = findDominator(fnc, o1);
//             } else if (o1->ty & IS_IMM && o2->ty & IS_SSA) {
//                 bb = findDominator(fnc, o2);
//             } else {
//                 assert(0);
//             }
//             instruction *ins = (instruction
// *)sy_malloc(sizeof(instruction));
//             ins->op = op;
//             ins->args[0] = *o1;
//             ins->args[1] = *o2;
//             ins->ret = v;
//             vector_push_back(&bb->inst, ins);
//             ssaSymbol *s = getSSAlVariable(fnc, v.ssaind);
//             s->Header = NULL;
//             s->def = ins;
//             return v;
//         }
//     }
//     return result->value;
// }
static Value Apply1(Function *fnc, TAC op, Value *o1, Value *o2) {
    valueTable *result = lookUp(fnc, op, o1, o2);
    BasicBlock *bb;
    ssaSymbol  *s1 = NULL, *s2 = NULL;
    if (!result) {
        if (o1->ty & IS_SSA) {
            s1 = getSSAlVariable(fnc, o1->ssaind);
            BasicBlock *defbb;
            LOOPALLBB({LOOPBBALLINST({
                if (inst == s1->Header) {
                    defbb = bb;
                }
            })})
            if (s1->Header != NULL && isRegionConstant(fnc, o2, defbb)) {
                return Reduce1(fnc, op, o1, o2);
            }
        } else if (o2->ty & IS_SSA) {
            s2 = getSSAlVariable(fnc, o2->ssaind);
            BasicBlock *defbb;
            LOOPALLBB({LOOPBBALLINST({
                if (inst == s2->Header) {
                    defbb = bb;
                }
            })})
            if (s2->Header != NULL && isRegionConstant(fnc, o1, defbb)) {
                return Reduce1(fnc, op, o1, o2);
            }
        }

        // TODO 这里可能BUG
        Value v = createSSATempVariable(fnc, o1->ty);
        // TODO new name的def未写入,还有指令插入那里

        insert(fnc, op, o1, o2, v);
        if (o1->ty & IS_SSA && o2->ty & IS_SSA) {
            BasicBlock *b1 = findDefintionBB(fnc, o1);
            BasicBlock *b2 = findDefintionBB(fnc, o2);
            if (isDominetor(fnc, b1, b2)) {
                bb = b1;
            } else {
                bb = b2;
            }

        } else if (o1->ty & IS_SSA && o2->ty & IS_IMM) {
            bb = findDefintionBB(fnc, o1);
        } else if (o1->ty & IS_IMM && o2->ty & IS_SSA) {
            bb = findDefintionBB(fnc, o2);
        } else if (o1->ty & IS_IMM && o2->ty & IS_IMM) {
            assert(o1->ty == o2->ty);
            if (op == IR_OP_MUL) {
                return SET_IMM_INT(o1->imm.i * o2->imm.i);
            } else if (op == IR_OP_MULF) {
                return SET_IMM_FLOAT(o1->imm.f * o2->imm.f);
            } else {
                assert(0);
            }
        } else {
            assert(0);
        }
        instruction *ins = (instruction *)sy_malloc(sizeof(instruction));

        // if (o1->ty & IS_SSA) {
        //     ssaSymbol *s1 = getSSAlVariable(fnc, o1->ssaind);
        //     s1->
        // }
        // if (o2->ty & IS_SSA) {
        //     ssaSymbol *s2 = getSSAlVariable(fnc, o2->ssaind);
        //     s2->
        // }
        ins->op = op;
        ins->args[0] = *o1;
        ins->args[1] = *o2;
        ins->ret = v;
        vector_push_back(&bb->inst, ins);
        DL_APPEND(bb->instlist, ins);
        ssaSymbol *s = getSSAlVariable(fnc, v.ssaind);
        s->visited = true;

        s->Header = NULL;
        s->def = ins;
        return v;
    }
    return result->value;
}
// Process(N) if N has only one member n then if n is a “candidate” then
//     Replace(n, iv, rc) else n.Header ← null else ClassifyIV(N)

// 当 Process 将 n 识别为候选操作时，它会找到归纳变量 iv 和区域常数 rc。
static void Process(Function *fnc, bitset_t N) {
    size_t      ind;
    bitset_it_t it;
    if (bitset_popcount(N) == 1) {
        //
        // 如果SCC由单个结点组成，且结点已经具备候选操作的形式，那么process会将n及其归纳变量和区域常量传递Replcae
        for (bitset_it(it, N); !bitset_end_p(it); bitset_next(it)) {
            if (*bitset_cref(it)) {
                ssaSymbol *s = vector_get(&fnc->SSAValuePool, it->index);
                assert(s);
                assert(s->def);

                ssaSymbol *s1 = NULL, *s2 = NULL;
                if (s->def->args[0].ty & IS_SSA) {
                    s1 = vector_get(&fnc->SSAValuePool, s->def->args[0].ssaind);
                }
                if (s->def->args[1].ty & IS_SSA) {
                    s2 = vector_get(&fnc->SSAValuePool, s->def->args[1].ssaind);
                }

                if (s->def->op == IR_OP_ADD || s->def->op == IR_OP_SUB ||
                    s->def->op == IR_OP_MUL) {
                    if (s1 && s1->Header)
                        Replace(fnc, s->def, &s->def->args[0],
                                &s->def->args[1]);
                    else if (s2 && s2->Header)
                        Replace(fnc, s->def, &s->def->args[1],
                                &s->def->args[0]);
                } else {
                    s->Header = NULL;
                }
            }
        }
    } else {
        ClassifyIV1(fnc, N);
    }
}
static void osrrepalce(Function *fnc);
static void processIR(Function *fnc);
/**
 * @brief
 * DFS对SSA图执行深度优先搜索。它为每个节点分配一个数字，该数字对应于DFS访问节点的顺序。
 * 它将每个节点推送到堆栈上，并在节点上标记可从其子节点访问的节点上具有最低深度优先数字的节点。
 当 DFS 从处理子节点返回时，如果从 n 访问的最低节点有 n 个编号， 则 n 是
 SCC的标头。DFS 从堆栈中弹出节点，直到达到 n;所有这些节点都是 SCC 的成员。
 * @param[in] *fnc
 * @param[in] *ins
*/
static void DFS(Function *fnc, ssaSymbol *curV) {
    switch (curV->def->op) {
        OP3_3AC_COND
        curV->visited = true;
        return;
    }
    // 加载常量指令不分析
    // switch (curV->op) {
    //     case IR_OP_MOV:
    //         if (curV->args[0].ty & IS_IMM) returv->def->Numn;
    // }
    curV->Num = nextNum++;
    curV->visited = true;
    curV->Low = curV->Num;
    push(curV);

    // 遍历操作数的定义
    switch (curV->def->op) {
        OP3_3AC_COND
        break;
        case IR_OP_ADDPTR:
            OP3_3AC_OPER

            for (size_t i = 0; i < 2; i++) {
                if (curV->def->args[i].ty & IS_SSA) {
                    ssaSymbol *v =
                        getSSAlVariable(fnc, curV->def->args[i].ssaind);
                    if (!v->visited) {
                        DFS(fnc, v);
                        curV->Low = min(curV->Low, v->Low);
                    }
                    if ((v->Num < curV->Num) && isOnStack(v)) {
                        curV->Low = min(curV->Low, v->Num);
                    }
                }
            }
            break;

        case IR_OP_GETPTR: {
            assert(0);
        } break;
        case IR_OP_LOAD:
            if (curV->def->args[0].ty & IS_SSA) {
                ssaSymbol *v = getSSAlVariable(fnc, curV->def->args[0].ssaind);
                if (!v->visited) {
                    DFS(fnc, v);
                    curV->Low = min(curV->Low, v->Low);
                }
                if ((v->Num < curV->Num) && isOnStack(v)) {
                    curV->Low = min(curV->Low, v->Num);
                }
            }
            break;
        case IR_OP_MOV:
        case IR_OP_I2F:
        case IR_OP_F2I:
            if (curV->def->args[0].ty & IS_SSA) {
                ssaSymbol *v = getSSAlVariable(fnc, curV->def->args[0].ssaind);
                if (!v->visited) {
                    DFS(fnc, v);
                    curV->Low = min(curV->Low, v->Low);
                }
                if ((v->Num < curV->Num) && isOnStack(v)) {
                    curV->Low = min(curV->Low, v->Num);
                }
            } else if ((curV->def->args[0].ty & IS_IMM)) {
                // 立即数则退出
                break;
            } else {
                assert(0);
            }
            break;
        case IR_OP_PHI: {
            phiFunction *phi = curV->def->args[0].phi;
            phiParam    *p;
            size_t       i;
            vector_each_address(&phi->param, i, p) {
                ssaSymbol *v = getSSAlVariable(fnc, p->v.ssaind);
                if (!v->visited) {
                    DFS(fnc, v);
                    curV->Low = min(curV->Low, v->Low);
                }
                if ((v->Num < curV->Num) && isOnStack(v)) {
                    curV->Low = min(curV->Low, v->Num);
                }
            }
        } break;
        case IR_OP_ARGUMENT:
            break;
        case IR_OP_CALL:
            break;
        default:
            assert(0);
    }
    if (curV->Low == curV->Num) {
        bitset_t SCC;
        bitset_init(SCC);
        bitset_resize(SCC, vector_len(&fnc->SSAValuePool));
        bitset_init_empty(SCC);
        ssaSymbol *x;
        do {
            x = pop();
            bitset_set_at(SCC, x->ssaind, true);
        } while (x != curV);
        // bitset_it_t it;
        // for (bitset_it(it, SCC); !bitset_end_p(it); bitset_next(it)) {
        //     if (*bitset_cref(it)) {
        //         ssaSymbol *s = vector_get(&fnc->SSAValuePool, it->index);
        //         printSSAValue(stdout, fnc, s);
        //         printf("\n");
        //     }
        // }
        processIR(fnc);
        Process(fnc, SCC);
        LFTR(fnc);
        osrrepalce(fnc);
        ssaSymbol *sym;
        //
        bitset_clear(SCC);
    }
}

static instruction *createMul2(Function *fnc, BasicBlock *bb, Value *arg1,
                               Value *arg2) {
    assert(ValueIsInt(arg1) && ValueIsInt(arg2));
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;

    inst->ret = createSSATempVariable(fnc, arg1->ty);
    ssaSymbol *s = getSSAlVariable(fnc, inst->ret.ssaind);
    s->def = inst;
    inst->op = IR_OP_MUL;

    return inst;
}
static instruction *createAddptr(Function *fnc, BasicBlock *bb,
                                 struct ArrayType *arrayTy, Value *arg1,
                                 Value *arg2) {
    assert(ValueIsInt(arg2));
    assert(arg1->ty & IS_POINTER);
    instruction *inst = sy_malloc(sizeof(instruction));
    inst->args[0] = *arg1;
    inst->args[1] = *arg2;

    Type ty = {0};
    ty |= IS_SSA;
    if (arrayTy->next) {
        ty |= IS_POINTER | IS_ARRAY;
    }
    if (arg1->ty & TY_INT) {
        ty |= TY_INT;
    } else {
        ty |= TY_FLOAT;
    }
    inst->ret = createSSATempVariable(fnc, ty);
    ssaSymbol *s = getSSAlVariable(fnc, inst->ret.ssaind);
    s->def = inst;
    inst->op = IR_OP_ADDPTR;

    return inst;
}
size_t getArraySize(ArrayType *ty);

// 将getptr转换为addptr
void   TransformArrayPass(Function *fnc) {
    size_t       i;
    instruction *inst;
    ssaSymbol   *ssa;
    BasicBlock  *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        vector_template(BasicBlock *, tmpBBptr);
        vector_init(&tmpBBptr);
        size_t i;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_GETITEMPTR) {
                struct ArrayType *arrayTy;
                if (inst->args[0].ty & IS_SSA) {
                    ssaSymbol *ssa = getSSAlVariable(fnc, inst->args[0].ssaind);
                    assert(ssa->arrayTy);
                    arrayTy = ssa->arrayTy;
                } else {
                    Symbol *s;
                    if (inst->args[0].ty & IS_GLOBAL) {
                        s = getGlobalSymbol(inst->args[0].symbol);
                    } else {
                        s = getLocalVariable(fnc, inst->args[0].symbol);
                    }
                    arrayTy = s->arrayTy;
                }
                assert(arrayTy);
                arrayAddr *ptr = inst->args[1].arrayAddr;
                Value     *index;
                size_t     j;
                int        sum;
                Value      pointer = inst->args[0];
                vector_each_address(&ptr->index, j, index) {
                    if (arrayTy->next) {
                        size_t n = getArraySize(arrayTy->next);
                        sum = n * 4;
                    } else {
                        sum = 4;
                    }
                    Value        imm = SET_IMM_INT(sum);
                    instruction *tmpinst;
                    Value        tmp;
                    // TODO fold
                    if (!foldValue(IR_OP_MUL, index, &imm, &tmp)) {
                        tmpinst = createMul2(fnc, bb, index, &imm);
                        tmp = tmpinst->ret;
                        vector_push_back(&tmpBBptr, tmpinst);
                    } else {
                    }

                    tmpinst = createAddptr(fnc, bb, arrayTy, &pointer, &tmp);
                    vector_push_back(&tmpBBptr, tmpinst);
                    if (arrayTy->next && ptr->index.count - 1 != j) {
                        pointer = tmpinst->ret;
                    } else {
                        tmpinst->ret = inst->ret;
                        ssaSymbol *s =
                            getSSAlVariable(fnc, tmpinst->ret.ssaind);
                        s->def = tmpinst;
                    }
                    arrayTy = arrayTy->next;
                }
            } else {
                vector_push_back(&tmpBBptr, inst);
            }
        }
        if (bb->inst.data) {
            vector_free(&bb->inst);
        }
        bb->inst.capacity = tmpBBptr.capacity;
        bb->inst.data = tmpBBptr.data;
        bb->inst.count = tmpBBptr.count;
    }
}

/**
 * @brief 找函数的每个循环块
 * @param[in] *fnc
 */
static void findLoopHeader(Function *fnc) {
    vector_init(&loopHeaders);
    LOOPALLBB({
        if (bb->nextblock) {
            if (isDominetor(fnc, bb->nextblock, bb)) {
                printf("loop:%d\n", bb->nextblock->bbName);
                vector_push_back(&loopHeaders, bb->nextblock);
            }
        } else if (bb->branchblock) {
            if (isDominetor(fnc, bb->branchblock, bb)) {
                printf("loop:%d\n", bb->branchblock->bbName);
                vector_push_back(&loopHeaders, bb->branchblock);
            }
        }
    })
}

static void processIR(Function *fnc) {
    LOOPALLBB({
        bb->instlist = NULL;
        instruction *ins;
        vector_each3(&bb->inst, ins) { DL_APPEND(bb->instlist, ins); }
    })
}
static BasicBlock *findBB(Function *fnc, instruction *ptr) {
    LOOPALLBB({
        instruction *ins;
        vector_each3(&bb->inst, ins) {
            if (ins == ptr) {
                return bb;
            }
        }
    })
    assert(0);
}
static void osrinsertIR(Function *fnc, instruction *ins, instruction *insert) {
    // APPEND_ELEM
    BasicBlock *bb = findBB(fnc, ins);
    DL_APPEND_ELEM(bb->instlist, ins, insert);
}
static void osrrepalce(Function *fnc) {
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        vector_free(&bb->inst);
        vector_init(&bb->inst);
        instruction *head, *ptr, *tmp;
        head = bb->instlist;
        DL_FOREACH_SAFE(head, ptr, tmp) { vector_push_back(&bb->inst, ptr); }
    }
}
// 循环寻找回边
static void Loopnestingforest2(Function *fnc, BasicBlock *bb,
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
static void initOSR(Function *fnc) {
    // 内存泄漏,我已经忘了这个函数怎么用了
    valueHH = NULL;
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
}

/**
 * @brief 强度削弱,采用OSR算法,参考自EAC
 * @param[in] *fnc
 */
bool StrengthReductionPass(Function *fnc) {
    // 需要先将数组寻址指令转为低级指令
    TransformArrayPass(fnc);
    initOSR(fnc);
    // processIR(fnc);
    printBasicAllBlock(stdout, fnc);
    findLoopHeader(fnc);
    ssaSymbol *v;
    vector_each3(&fnc->SSAValuePool, v) { v->visited = false; }
    FILE *fp1 = fopen("osr1.txt", "w");
    printBasicAllBlock(fp1, fnc);
    fclose(fp1);
    OSR(fnc);
    // osrrepalce(fnc);
    FILE *fp2 = fopen("osr2.txt", "w");
    printBasicAllBlock(fp2, fnc);
    fclose(fp2);
}
