// #include <assert.h>
// #include <bitset.h>
// #include <function.h>
// #include <ir.h>
// #include <value.h>
// #include <lexer.h>
// #include <m-bitset.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include <util.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../include/bitset.h"
#include "../include/function.h"
#include "../include/ir.h"
#include "../include/lexer.h"
#include "../include/m-bitset.h"
#include "../include/pass.h"
#include "../include/symbol.h"
#include "../include/sysy.h"
#include "../include/util.h"
#include "../include/value.h"
#include "../include/vector.h"

// TODO 删除
int gdfnum = 1;
vector_template(BasicBlock *, BBvertex);

void dfsDom(Function *fnc, BasicBlock *bb) {
    if (!bb->visited) {
        bb->visited = 1;
        vector_push_back(&fnc->reverse_bb_list, bb);
        BasicBlock *parnet = bb;
        LOOPALLBB({
            if (bb->idom == parnet) dfsDom(fnc, bb);
        })
    }
}

// TODO 删除
void dfs(BasicBlock *parent, BasicBlock *bb) {
    if (bb->dfnum == 0) {
        bb->dfnum = gdfnum++;
        bb->parent = parent;
        if (bb->branchblock) dfs(bb, bb->branchblock);
        if (bb->nextblock) dfs(bb, bb->nextblock);
        vector_push_back(&BBvertex, bb);
    }
}
static BasicBlock *postintersect(BasicBlock *b1, BasicBlock *b2) {
    BasicBlock *finger1 = b1;
    BasicBlock *finger2 = b2;
    while (finger1 != finger2) {
        while (finger1->postdomind > finger2->postdomind)
            finger1 = finger1->postidom;
        while (finger2->postdomind > finger1->postdomind)
            finger2 = finger2->postidom;
    }
    return finger1;
}
static BasicBlock *intersect(BasicBlock *b1, BasicBlock *b2) {
    BasicBlock *finger1 = b1;
    BasicBlock *finger2 = b2;
    while (finger1 != finger2) {
        while (finger1->domind > finger2->domind) finger1 = finger1->idom;
        while (finger2->domind > finger1->domind) finger2 = finger2->idom;
    }
    return finger1;
}
void initInedges(Function *fnc) {
    LOOPALLBB({
        bb->visited = 0;
        vector_clear(&bb->inedges);
    })
}
/**
 * @brief 构建入边,为反支配树做准备,其他pass可能破坏inedges,每次需要重新构建
 * @param[in] *fn
 * @param[in] *root
 */
void buildInedges(BasicBlock *parent, BasicBlock *bb) {
    if (!bb) return;
    if (parent) {
        vector_push_back(&bb->inedges, parent);
    }
    if (!bb->visited) {
        bb->visited = 1;
        if (bb->nextblock) buildInedges(bb, bb->nextblock);
        if (bb->branchblock) buildInedges(bb, bb->branchblock);
    }
}
// 反支配
static void reverse_postorder1(Function *fn, BasicBlock *root, size_t *ind) {
    if (!root) return;
    if (root->visited) return;
    root->visited = 1;
    size_t      i;
    BasicBlock *bb;
    vector_each(&root->inedges, i, bb) { reverse_postorder1(fn, bb, ind); }
    root->postdomind = *ind;
    fn->reverse_bb_list.data[--(*ind)] = root;
}
static void reverse_postorder(Function *fn, BasicBlock *root, size_t *ind) {
    if (!root) return;
    if (root->visited) return;
    root->visited = 1;
    reverse_postorder(fn, root->branchblock, ind);
    reverse_postorder(fn, root->nextblock, ind);
    root->domind = *ind;
    fn->reverse_bb_list.data[--(*ind)] = root;
}
/**
 * @brief 构建反支配树,反立即支配点,反支配dom集合,
 * @param[in] *fn
 */
void BuildPostorderDominetTree(Function *fnc) {
    bool        changed = true;
    size_t      bbLenth, oldLenth;
    BasicBlock *bb = NULL;
    oldLenth = bbLenth = vector_len(&fnc->BBs);

    LOOPALLBB({
        bb->visited = 0;
        bb->postidom = NULL;
    })
    buildInedges(NULL, vector_get(&fnc->BBs, 0));

    LOOPALLBB({ bb->visited = 0; })
    vector_clear(&fnc->reverse_bb_list);

    BasicBlock *finalBB = fnc->finalBB;

    // 是不应该包含finalBB的,循环中需要处理下
    size_t      i;
    vector_each(&finalBB->inedges, i, bb) {
        reverse_postorder1(fnc, bb, &oldLenth);
    }
    // reverse_postorder1(fnc, finalBB, &oldLenth);
    finalBB->postidom = finalBB;
    finalBB->postdomind = 1;

    BasicBlock *new_idom = NULL, *b = NULL;
    while (changed) {
        changed = false;
        size_t i;
        for (i = oldLenth; i < bbLenth; i++) {
            bb = vector_get(&fnc->reverse_bb_list, i);
            b = bb;
            // 处理finalBB
            if (finalBB == b) continue;
            assert(finalBB != b);
            if (bb->nextblock && bb->nextblock->postidom) {
                new_idom = bb->nextblock;
            } else if (bb->branchblock && bb->branchblock->postidom) {
                new_idom = bb->branchblock;
            } else {
                assert(0);
                new_idom = NULL;
            }
            {
                // 后前驱
                if (bb->nextblock) {
                    if (bb->nextblock->postidom != NULL) {
                        new_idom = postintersect(bb->nextblock, new_idom);
                    }
                }
                if (bb->branchblock) {
                    if (bb->branchblock->postidom != NULL) {
                        new_idom = postintersect(bb->branchblock, new_idom);
                    }
                }
            }
            assert(new_idom);
            if (bb->postidom != new_idom) {
                bb->postidom = new_idom;
                changed = true;
            }
        }
    }

    // 构建postdom集合
    // a->b , b->c ; a->c
    LOOPALLBB({
        // 跳过第一个基本块
        if (bb->postidom == bb) continue;
        if (bb->postidom) {
            vector_push_back(&bb->postidom->postdoms, bb);
        }
    })
}
static BasicBlock *_tempBB;
/**
 * @brief 构建每个基本快的doms
 * @param[in] *fnc
 * @param[in] bb
 */
static void        buildDoms(Function *fnc, BasicBlock *bb) {
    if (!bb) return;

    BasicBlock *b;
    vector_each3(&bb->doms, b) {
        if (b->dom == bb) {
            vector_push_back(&_tempBB->doms, bb);
            buildDoms(fnc, b);
        }
    }
}
static void dfsDominetor(Function *fnc, BasicBlock *bb1, BasicBlock *bb2,
                         bool *isDom) {
    if (!bb1) return;
    BasicBlock *ptr;
    vector_each3(&bb1->doms, ptr) {
        if (ptr == bb2) {
            *isDom = true;
            return;
        } else {
            dfsDominetor(fnc, ptr, bb2, isDom);
        }
    }
}
/**
 * @brief bb1是否支配bb2
 * @param[in] *fnc
 * @param[in] *bb1
 * @param[in] *bb2
 */
bool isDominetor(Function *fnc, BasicBlock *bb1, BasicBlock *bb2) {
    bool isDom = false;
    dfsDominetor(fnc, bb1, bb2, &isDom);
    return isDom;
}
/**
 * @brief 构建支配树,立即支配点,dom集合,参考自dom.pdf
 * @param[in] *fn
 */
void BuildDominetTree(Function *fnc) {
    bool        changed = true;
    size_t      bbLenth, oldLenth;
    BasicBlock *first = NULL, *bb = NULL;
    oldLenth = bbLenth = vector_len(&fnc->BBs);

    initInedges(fnc);
    buildInedges(NULL, fnc->firstBB);
    LOOPALLBB({ bb->visited = 0; })
    first = fnc->firstBB;
    LOOPALLBB({ bb->idom = NULL; })

    vector_reserve(&fnc->reverse_bb_list, vector_len(&fnc->BBs));
    reverse_postorder(fnc, first->nextblock, &oldLenth);
    reverse_postorder(fnc, first->branchblock, &oldLenth);

    // dfs(first, first->nextblock);
    BasicBlock *new_idom = NULL, *b = NULL;
    size_t      i;

    first->idom = first;
    first->domind = 1;

    BasicBlock *p;
    while (changed) {
        changed = false;
        int i;
        for (i = oldLenth; i < bbLenth; i++) {
            b = bb = vector_get(&fnc->reverse_bb_list, i);
            assert(first != b);
            new_idom = NULL;
            {
                int i;
                vector_each3(&bb->inedges, new_idom) {
                    if (new_idom->idom != NULL) {
                        goto _found;
                    }
                }
                assert(0);
            _found:
                p = NULL;
                vector_each3(&bb->inedges, p) {
                    if (p->idom != NULL) {
                        new_idom = intersect(p, new_idom);
                    }
                }
                // 其前驱

                // vector_each(&fnc->BBs, i, pred) {
                //     if ((pred->nextblock == bb) || (pred->branchblock == bb))
                //     {
                //         if (pred->idom != NULL) {
                //             if (new_idom)
                //                 intersect(pred, new_idom);
                //             else {
                //                 new_idom = pred;
                //             }
                //         }
                //     }
                // }
            }
            assert(new_idom);
            if (b->idom != new_idom) {
                b->idom = new_idom;
                changed = true;
            }
        }
    }

    // 构建dom集合
    LOOPALLBB({
        // 跳过第一个基本块
        if (bb->idom == bb) continue;
        if (bb->idom) {
            vector_push_back(&bb->idom->doms, bb);
        }
    })
    // LOOPALLBB({
    //     // 跳过第一个基本块
    //     if (bb->idom == bb) continue;
    //     if (bb->idom && vector_len(&bb->doms)) {
    //         size_t      i;
    //         BasicBlock *tmp;
    //         vector_each(&bb->doms, i, tmp) {
    //             vector_push_back(&bb->idom->doms, tmp);
    //         };
    //     }
    // })
    //     LOOPALLBB({
    //     // 跳过第一个基本块
    //     if (bb->idom == bb) continue;
    //     if (bb->idom) {
    //         vector_push_back(&bb->idom->doms, bb);
    //     }
    //     // buildDoms(fnc,bb);

    // })
    // LOOPALLBB({
    //     size_t      i;
    //     BasicBlock *tmp;
    //     printf("bb_%d:", bb->bbName);
    //     vector_each(&bb->doms, i, tmp) { printf("bb_%d,", tmp->bbName); };
    //     printf("\n");
    // })
}

vector_template(BasicBlock *, parents);

/**
 * @brief 构造支配边界,参考自eac
 * @param[in] *fnc
 */
// void buildDF(Function *fnc) {
//     BasicBlock *runner = NULL, *ptr = NULL;
//     vector_clear(&parents);
//     initInedges(fnc);
//     buildInedges(NULL, fnc->firstBB);
//     LOOPALLBB({
//         // vector_clear(&parents);

//         LOOPALLBBPREV({ vector_push_back(&parents, bb); })
//         if (parents.count >= 2) {
//             int i;
//             vector_each(&parents, i, runner) {
//                 assert(runner);
//                 while (runner != bb->idom) {
//                     vector_push_back(&runner->df, bb);
//                     runner = runner->idom;
//                     if (runner == runner->idom) break;
//                 }
//             }
//         }
//     })
// }
/**
 * @brief
 * @param[in] *fnc
 */
void buildDF(Function *fnc) {
    BasicBlock *runner = NULL, *ptr = NULL;
    vector_clear(&parents);
    initInedges(fnc);
    buildInedges(NULL, fnc->firstBB);
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        // vector_clear(&parents);
        if (bb->inedges.count >= 2) {
            size_t i;
            vector_each(&bb->inedges, i, runner) {
                assert(runner);
                while (runner != bb->idom) {
                    vector_push_back(&runner->df, bb);
                    runner = runner->idom;
                    // if (runner == runner->idom) break;
                }
            }
        }
    }
}
/**
 * @brief 构造反向支配边界,参考自eac
 * @param[in] *fnc
 */
void buildPostDF(Function *fnc) {
    BasicBlock *runner = NULL, *ptr = NULL;
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        if (bb->branchblock && bb->nextblock) {
            runner = bb->nextblock;
            while (runner != bb->postidom) {
                vector_push_back(&runner->postdf, bb);
                runner = runner->postidom;
                if (runner == runner->postidom) break;
            }
            runner = bb->branchblock;
            while (runner != bb->postidom) {
                vector_push_back(&runner->postdf, bb);
                runner = runner->postidom;
                if (runner == runner->postidom) break;
            }
        }
    }
}
// TODO free
static bitset_t Wset;
static bitset_t Fset;
static bitset_t variableSet;

/**
 * @brief 得到局部变量
 * @param[in] *fnc
 * @param[in] i
 */
Symbol         *getLocalVariable(Function *fnc, size_t i) {
    return vector_get(&fnc->symPool, i);
}
/**
 * @brief 得到ssa变量
 * @param[in] *fnc
 * @param[in] i
 */
ssaSymbol *getSSAlVariable(Function *fnc, size_t i) {
    return vector_get(&fnc->SSAValuePool, i);
}
static void variable_phi_function1(Function *fnc, Symbol *sym) {
    Symbol *s = NULL;
    size_t  bb_index;
    bitset_init_empty(Wset);
    bitset_init_empty(Fset);
    LOOPALLBB({LOOPBBALLINST({
        switch (inst->op) {
            OP3_3AC_COND
            OP3_3AC_OPER
            OP2_3AC
            // 将所有被定义的变量的基本块加入到集合中

            // TODO 没有考虑数组
            if (!(inst->ret.ty & IS_GLOBAL) && (inst->ret.ty & IS_VAR)) {
                s = getLocalVariable(fnc, inst->ret.symbol);
                if (s == sym) {
                    m_bitset_set_at(Wset, bb->bbName, 1);
                }
            }
        }
    })})

    bitset_t defSet;
    bitset_init_set(defSet, Wset);
    // 如果集合不为NULL
    while (!bitset_is_empty(Wset)) {
        bitset_pop_index(Wset, &bb_index);

        BasicBlock *bb = vector_get(&fnc->BBs, bb_index);
        BasicBlock *y;
        size_t      i;
        vector_each(&bb->df, i, y) {
            if (!bitset_get(Fset, y->bbName)) {
                // 这里可能会插入较多的phi,应该查看变量是否在这里插入过函数
                printf("insert phi function in <bb %d>,symbol \"%s\"",
                       y->bbName, getToken(sym->tok)->str);
                setCurrentBB(fnc, y);
                instruction *phi = createPHI(fnc);
                phi->args[0].phi->symbol = sym;
                phi->ret.ty = sym->type | IS_VAR;
                phi->ret.symbol = sym->ind;
                bitset_set_at(Fset, y->bbName, 1);
                if (!bitset_get(defSet, y->bbName)) {
                    bitset_set_at(Wset, y->bbName, 1);
                }
            }
        }
    }
    bitset_clear(defSet);
}
static void variable_phi_function(Function *fnc, Symbol *sym) {
    Symbol *s = NULL;
    size_t  bb_index;
    bitset_init_empty(Wset);
    bitset_init_empty(Fset);
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        instruction *head, *inst, *tmp;
        head = bb->instlist;
        DL_FOREACH_SAFE(head, inst, tmp) {
            switch (inst->op) {
                OP3_3AC_COND
                OP3_3AC_OPER
                OP2_3AC
                // 将所有被定义的变量的基本块加入到集合中

                // TODO 没有考虑数组
                if (!(inst->ret.ty & IS_GLOBAL) && (inst->ret.ty & IS_VAR)) {
                    s = getLocalVariable(fnc, inst->ret.symbol);
                    if (s == sym) {
                        m_bitset_set_at(Wset, bb->bbName, 1);
                    }
                }
            }
        }
    }

    bitset_t defSet;
    bitset_init_set(defSet, Wset);
    // 如果集合不为NULL
    while (!bitset_is_empty(Wset)) {
        bitset_pop_index(Wset, &bb_index);

        BasicBlock *bb = vector_get(&fnc->BBs, bb_index);
        BasicBlock *y;
        size_t      i;
        vector_each(&bb->df, i, y) {
            if (!bitset_get(Fset, y->bbName)) {
                // 这里可能会插入较多的phi,应该查看变量是否在这里插入过函数
                printf("insert phi function in <bb %d>,symbol \"%s\"",
                       y->bbName, getToken(sym->tok)->str);
                setCurrentBB(fnc, y);
                instruction *phi = createPHI(fnc);
                phi->args[0].phi->symbol = sym;
                phi->ret.ty = sym->type | IS_VAR;
                phi->ret.symbol = sym->ind;
                bitset_set_at(Fset, y->bbName, 1);
                if (!bitset_get(defSet, y->bbName)) {
                    bitset_set_at(Wset, y->bbName, 1);
                }
            }
        }
    }
    bitset_clear(defSet);
}
/**
 * @brief 插入PHI函数,参考自SSA book
 * @param[in] *fnc
 */
void insertPhi1(Function *fnc) {
    bitset_reset(Wset);
    bitset_reset(Fset);
    bitset_reset(variableSet);
    bitset_resize(Wset, vector_len(&fnc->BBs));
    bitset_resize(Fset, vector_len(&fnc->BBs));
    bitset_resize(variableSet, vector_len(&fnc->symPool));
    Symbol *s = NULL;
    LOOPALLBB({LOOPBBALLINST({
        switch (inst->op) {
            OP3_3AC_OPER
            OP3_3AC_COND
            OP2_3AC
            // 将所有被定义的变量加入到集合中,全局变量和临时变量除外
            // TODO 没有考虑数组
            if (!(inst->ret.ty & IS_GLOBAL) && (inst->ret.ty & IS_VAR)) {
                s = getLocalVariable(fnc, inst->ret.symbol);
                if (!s->isTemp) {
                    bitset_set_at(variableSet, s->ind, 1);
                }
            }
        }
    })})
    size_t ind;
    while (bitset_pop_index(variableSet, &ind)) {
        s = vector_get(&fnc->symPool, ind);
        variable_phi_function(fnc, s);
    }
}
void insertPhi(Function *fnc) {
    bitset_reset(Wset);
    bitset_reset(Fset);
    bitset_reset(variableSet);
    bitset_resize(Wset, vector_len(&fnc->BBs));
    bitset_resize(Fset, vector_len(&fnc->BBs));
    bitset_resize(variableSet, vector_len(&fnc->symPool));
    Symbol     *s = NULL;
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        instruction *head, *inst, *tmp;
        head = bb->instlist;
        DL_FOREACH_SAFE(head, inst, tmp) {
            switch (inst->op) {
                OP3_3AC_OPER
                OP3_3AC_COND
                OP2_3AC
                // 将所有被定义的变量加入到集合中,全局变量和临时变量除外
                // TODO 没有考虑数组
                if (!(inst->ret.ty & IS_GLOBAL) && (inst->ret.ty & IS_VAR)) {
                    s = getLocalVariable(fnc, inst->ret.symbol);
                    if (!s->isTemp) {
                        bitset_set_at(variableSet, s->ind, 1);
                    }
                }
            }
        }
    }
    size_t ind;
    while (bitset_pop_index(variableSet, &ind)) {
        s = vector_get(&fnc->symPool, ind);
        variable_phi_function(fnc, s);
    }
}
static bool dominatorDefinition(Function *fnc, ssaSymbol *v, BasicBlock *_bb) {
    // printf("%d\n", _bb->idom->bbName);
    LOOPALLBB({LOOPBBALLINST({
        if (inst == v->def) {
            if (bb == _bb) return true;
            size_t      i;
            BasicBlock *tmp;
            if (isDominetor(fnc, bb, _bb)) {
                return true;
            }
            // vector_each(&bb->doms, i, tmp) {
            //     if (tmp == _bb) return true;
            // }
            return false;
        };
    })});
    assert(0);
};
/**
 * @brief 参考自ssa book,更新值,
 * @param[in] *fn
 * @param[in] *v
 * @param[in] *i
 * @param[in] *bb 此指令所在的基本块
 */
ssaSymbol *updateReachingDef(Function *fnc, Symbol *v, instruction *i,
                             BasicBlock *bb) {
    ssaSymbol *r = v->ReachingDef;
    while (!((r == NULL) || dominatorDefinition(fnc, r, bb))) {
        r = r->ReachingDef;
    };
    // ssaSymbol *tmp = NULL;
    // // 指令i是否定义了v
    // while (!((r == NULL) || (r && r->valueind == 0) ||
    //          r->sym->ind == i->ret.symbol)) {
    //     if (r->ReachingDef && r->ReachingDef->valueind == 0) tmp = r;
    // }
    v->ReachingDef = r;
    // if (r && r->valueind == 0) v->ReachingDef = tmp;
}
// ssaSymbol *updateReachingDef(Function *fnc, Symbol *v, instruction *i,
//                              BasicBlock *bb) {
//     ssaSymbol *r = v->ReachingDef;
//     while (!((r == NULL) || i->ret.symbol == v->ind)) {
//         r = r->ReachingDef;
//     };
//     // ssaSymbol *tmp = NULL;
//     // // 指令i是否定义了v
//     // while (!((r == NULL) || (r && r->valueind == 0) ||
//     //          r->sym->ind == i->ret.symbol)) {
//     //     if (r->ReachingDef && r->ReachingDef->valueind == 0) tmp = r;
//     // }
//     v->ReachingDef = r;
//     // if (r && r->valueind == 0) v->ReachingDef = tmp;
// }

ssaSymbol *createSSAValue(Function *fnc, Symbol *sym) {
    ssaSymbol *ssa = sy_malloc(sizeof(ssaSymbol));
    ssa->ssaind = vector_len(&fnc->SSAValuePool);
    vector_push_back(&fnc->SSAValuePool, ssa);

    ssa->sym = sym;
    ssa->valueind = sym->count;
    sym->count++;
    ssa->ty = sym->type | IS_SSA;
    return ssa;
}
void setVariableDef(Function *fnc, uint32_t ind, instruction *inst) {
    ssaSymbol *ssa = vector_get(&fnc->SSAValuePool, ind);
    ssa->def = inst;
}
/**
 * @brief 变量重命名,该算法参考自ssa book
 * @param[in] *fn
 */
void variableRenaming(Function *fnc) {
    Symbol     *sym = NULL;
    ssaSymbol  *ssa = NULL, *new_ssa = NULL;
    BasicBlock *bb, *bbking;

    Symbol     *s;
    size_t      i;
    // vector_each(&fnc->symPool, i, s) {
    //     if (!(s->type & IS_ARRAY) && !s->isTemp) {
    //         s->ReachingDef = createSSAValue(fnc, s);
    //         setVariableDef(fnc, s->ReachingDef->ssaind, s->ReachingDef);
    //     }
    // }
    // LOOPALLBB({
    //     printf("%d:", bb->bbName);
    //     size_t i;
    //     vector_each(&bb->doms, i, bbking) { printf(",%d", bbking->bbName); };
    //     printf("\n");
    // })

    vector_clear(&fnc->reverse_bb_list);
    LOOPALLBB({ bb->visited = 0; })
    dfsDom(fnc, fnc->firstBB);

    vector_each(&fnc->reverse_bb_list, i, bb) {
        bbking = bb;

        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            if (inst->op == IR_OP_GETPTR) {
                assert(0);
            } else if (inst->op == IR_OP_LOAD) {
                if (!(inst->args[0].ty & IS_GLOBAL)) {
                    sym = getLocalVariable(fnc, inst->args[0].symbol);
                    if (sym->ReachingDef) {
                        inst->args[0].ssaind = sym->ReachingDef->ssaind;
                        inst->args[0].ty |= IS_SSA;
                    } else {
                        assert(0);
                    }
                }
            }
            switch (inst->op) {
                OP3_3AC_COND
                OP3_3AC_OPER
                // 替换used(被使用的变量),variable renaming
                if (!(inst->args[1].ty & IS_GLOBAL) &&
                    !(inst->args[1].ty & IS_POINTER) &&
                    (inst->args[1].ty & IS_VAR)) {
                    sym = getLocalVariable(fnc, inst->args[1].symbol);
                    if (!sym->isTemp) {
                        updateReachingDef(fnc, sym, inst, bb);
                        inst->args[1].ssaind = sym->ReachingDef->ssaind;
                        inst->args[1].ty |= IS_SSA;
                    } else {
                        inst->args[1].ssaind = sym->ReachingDef->ssaind;
                        inst->args[1].ty |= IS_SSA;
                    }
                }
                __attribute__((fallthrough));
                OP2_3AC
                if (inst->op == IR_OP_CALL) break;
                // 全局变量和数组不会被命名
                if (!(inst->args[0].ty & IS_GLOBAL) &&
                    !(inst->args[0].ty & IS_ARRAY) &&
                    (inst->args[0].ty & IS_VAR)) {
                    sym = getLocalVariable(fnc, inst->args[0].symbol);
                    // if (!sym->isTemp) {
                    updateReachingDef(fnc, sym, inst, bb);
                    // printf("更新 %s的定义", getToken(sym->tok)->str);
                    inst->args[0].ssaind = sym->ReachingDef->ssaind;
                    inst->args[0].ty |= IS_SSA;
                }
                if (inst->op == IR_OP_SOTRE) {
                    // 指针
                    if (!(inst->ret.ty & IS_GLOBAL) &&
                        (inst->ret.ty & IS_POINTER)) {
                        sym = getLocalVariable(fnc, inst->ret.symbol);
                        if (sym->ReachingDef) {
                            inst->ret.ssaind = sym->ReachingDef->ssaind;
                            inst->ret.ty |= IS_SSA;
                        } else {
                            assert(0);
                        }
                    }
                }
                break;  // 这里不是bug
                OP1_3AC
                if (inst->op == IR_OP_ARGUMENT) {
                    break;
                }
                if (!(inst->ret.ty & IS_GLOBAL) && (inst->ret.ty & IS_VAR)) {
                    sym = getLocalVariable(fnc, inst->ret.symbol);
                    if (sym->ReachingDef) {
                        updateReachingDef(fnc, sym, inst, bb);
                        inst->ret.ssaind = sym->ReachingDef->ssaind;
                        inst->ret.ty |= IS_SSA;
                    }
                }
                break;
                case IR_OP_GETPTR:
                    assert(0);
                    break;
                case IR_OP_GETITEMPTR: {
                    if (!(inst->args[0].ty & IS_GLOBAL)) {
                        sym = getLocalVariable(fnc, inst->args[0].symbol);
                        // 函数参数数组指针
                        if (sym->isPointer) {
                            updateReachingDef(fnc, sym, inst, bb);
                            inst->args[0].ssaind = sym->ReachingDef->ssaind;
                            inst->args[0].ty |= IS_SSA;
                            ssaSymbol *ssa =
                                getSSAlVariable(fnc, inst->args[0].ssaind);
                            ssa->arrayTy = sym->arrayTy;
                        }
                    }

                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     i;
                    vector_each_address(&ptr->index, i, index) {
                        if (index->ty & IS_GLOBAL) assert(0);
                        if (index->ty & IS_POINTER) assert(0);
                        if (index->ty & IS_ARRAY) assert(0);
                        if (!(index->ty & IS_GLOBAL) && (index->ty & IS_VAR)) {
                            sym = getLocalVariable(fnc, index->symbol);
                            if (sym->ReachingDef) {
                                updateReachingDef(fnc, sym, inst, bb);
                                index->ssaind = sym->ReachingDef->ssaind;
                                index->ty |= IS_SSA;
                            }
                        }
                    }
                } break;
                case IR_OP_PHI:
                case IR_OP_RET:
                    break;
                default:
                    assert(0);
            }
            switch (inst->op) {
                OP3_3AC_COND
                OP3_3AC_OPER
                OP2_3AC
                case IR_OP_PHI:
                case IR_OP_GETITEMPTR:
                    if (inst->ret.ty & IS_ARRAY) break;
                    if (inst->ret.ty & IS_GLOBAL) break;
                    if (inst->op == IR_OP_SOTRE && inst->ret.ty & IS_POINTER)
                        break;

                case IR_OP_ARGUMENT:
                    // 替换used(被使用的变量),variable renaming
                    if (!(inst->ret.ty & IS_GLOBAL) &&
                        (inst->ret.ty & IS_VAR)) {
                        if (inst->op == IR_OP_SOTRE &&
                            !(inst->ret.ty & IS_POINTER)) {
                            inst->op = IR_OP_MOV;
                        }

                        sym = getLocalVariable(fnc, inst->ret.symbol);
                        // if (!sym->isTemp) {
                        updateReachingDef(fnc, sym, inst, bb);
                        new_ssa = createSSAValue(fnc, sym);
                        inst->ret.ssaind = new_ssa->ssaind;
                        inst->ret.ty |= IS_SSA;
                        new_ssa->ReachingDef = sym->ReachingDef;
                        // printf("更新 %s的定义", getToken(sym->tok)->str);
                        // if (new_ssa == NULL) {
                        //     printf("更新 %s的定义", getToken(sym->tok)->str);
                        // }
                        sym->ReachingDef = new_ssa;
                        // } else {
                        //     inst->ret.ssaind = sym->ReachingDef->ssaind;
                        //     inst->ret.ty |= IS_SSA;
                        // }
                        setVariableDef(fnc, inst->ret.ssaind, inst);
                    }
                    break;
                case IR_OP_GETPTR: {
                    assert(0);
                    // sym = getLocalVariable(fnc, inst->ret.symbol);
                    // updateReachingDef(fnc, sym, inst, bb);
                    // new_ssa = createSSAValue(fnc, sym);
                    // inst->ret.ssaind = new_ssa->ssaind;
                    // inst->ret.ty |= IS_SSA;
                    // new_ssa->arrayTy = sym->arrayTy;
                    // new_ssa->ReachingDef = sym->ReachingDef;

                    // sym->ReachingDef = new_ssa;

                    // setVariableDef(fnc, inst->ret.ssaind, inst);
                } break;
            }
        }
        {
            BasicBlock *parent = bb;
            if (parent->nextblock) {
                BasicBlock  *bb = parent->nextblock;
                size_t       i;
                instruction *inst = NULL;
                vector_each(&bb->inst, i, inst) {
                    if (inst->op == IR_OP_PHI) {
                        phiFunction *phi = inst->args[0].phi;
                        sym = phi->symbol;
                        updateReachingDef(fnc, sym, inst, parent);
                        // updateReachingDef(fnc, sym, inst, bb);
                        phiParam tmp;
                        memset(&tmp, 0, sizeof(phiParam));
                        if (!sym->ReachingDef) continue;

                        tmp.v.ssaind = sym->ReachingDef->ssaind;
                        tmp.v.ty = IS_VAR | IS_SSA;
                        tmp.bb = bbking;
                        vector_push_back(&phi->param, tmp);
                    }
                }
            }
            if (parent->branchblock) {
                BasicBlock  *bb = parent->branchblock;
                size_t       i;
                instruction *inst = NULL;
                vector_each(&bb->inst, i, inst) {
                    if (inst->op == IR_OP_PHI) {
                        phiFunction *phi = inst->args[0].phi;
                        sym = phi->symbol;
                        updateReachingDef(fnc, sym, inst, parent);
                        // updateReachingDef(fnc, sym, inst, bb);
                        phiParam tmp;
                        memset(&tmp, 0, sizeof(phiParam));
                        if (!sym->ReachingDef) continue;
                        tmp.v.ssaind = sym->ReachingDef->ssaind;
                        tmp.v.ty = IS_VAR | IS_SSA;
                        tmp.bb = bbking;
                        vector_push_back(&phi->param, tmp);
                    }
                }
            }
        }
        // LOOPALLBBNEXT({LOOPBBALLPHI({
        //     phiFunction *phi = inst->args[0].phi;
        //     sym = phi->symbol;
        //     updateReachingDef(fnc, sym, inst, parent);
        //     phiParam tmp;
        //     memset(&tmp, 0, sizeof(phiParam));

        //     tmp.v.ssaind = sym->ReachingDef->ssaind;
        //     tmp.v.ty = IS_VAR | IS_SSA;
        //     tmp.bb = bbking;
        //     vector_push_back(&phi->param, tmp);
        // })})
    }
}

static bool insertphi(Function *fnc, Symbol *s, BasicBlock *bb) {
    LOOPBBALLPHI({
        if (inst->ret.symbol == s->ind) {
            return true;
        }
    })
    setCurrentBB(fnc, bb);
    instruction *phi = createPHI(fnc);
    phi->args[0].phi->symbol = s;
    phi->ret.ty = s->type | IS_VAR;
    phi->ret.symbol = s->ind;
    return false;
}
static void addBB(Function *fnc, uint32_t s, BasicBlock *bb) {
    Symbol *sym = getLocalVariable(fnc, s);
    if (!sym->setinit) {
        bitset_init(sym->bbset);
        sym->setinit = 1;
        bitset_resize(sym->bbset, vector_len(&fnc->BBs));
    }
    bitset_set_at(sym->bbset, bb->bbName, 1);
}
bitset_t globalname;
void     insertPhiFunction(Function *fnc) {
    // init
    bitset_t varkill;
    bitset_init(globalname);
    bitset_init(varkill);

    bitset_resize(globalname, vector_len(&fnc->symPool));
    bitset_resize(varkill, vector_len(&fnc->symPool));

    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        bitset_init_empty(varkill);
        size_t              i;
        struct instruction *inst = NULL;
        vector_each(&bb->inst, i, inst) {
            switch (inst->op) {
                OP3_3AC_COND
                if (inst->args[0].ty & IS_VAR) {
                    if (!bitset_get(varkill, inst->args[0].symbol)) {
                        bitset_set_at(globalname, inst->args[0].symbol, 1);
                    }
                }
                if (inst->args[1].ty & IS_VAR) {
                    if (!bitset_get(varkill, inst->args[1].symbol)) {
                        bitset_set_at(globalname, inst->args[1].symbol, 1);
                    }
                }
                break;
                OP3_3AC_OPER
                if (inst->args[0].ty & IS_VAR) {
                    if (!bitset_get(varkill, inst->args[0].symbol)) {
                        bitset_set_at(globalname, inst->args[0].symbol, 1);
                    }
                }
                if (inst->args[1].ty & IS_VAR) {
                    if (!bitset_get(varkill, inst->args[1].symbol)) {
                        bitset_set_at(globalname, inst->args[1].symbol, 1);
                    }
                }
                bitset_set_at(varkill, inst->ret.symbol, 1);
                addBB(fnc, inst->ret.symbol, bb);
                break;
                case IR_OP_MOV:
                    assert(0);
                case IR_OP_SOTRE:
                    if (inst->args[0].ty & IS_VAR) {
                        if (!bitset_get(varkill, inst->args[0].symbol)) {
                            bitset_set_at(globalname, inst->args[0].symbol, 1);
                        }
                    }
                    if (!(inst->ret.ty & IS_GLOBAL) &&
                        inst->ret.ty & IS_POINTER) {
                        if (!bitset_get(varkill, inst->ret.symbol)) {
                            bitset_set_at(globalname, inst->ret.symbol, 1);
                        }
                    } else if (!(inst->ret.ty & IS_GLOBAL) &&
                               inst->ret.ty & IS_VAR) {
                        addBB(fnc, inst->ret.symbol, bb);
                    }
                    break;
                case IR_OP_LOAD:
                    bitset_set_at(varkill, inst->ret.symbol, 1);
                    addBB(fnc, inst->ret.symbol, bb);
                    break;
                case IR_OP_I2F:
                case IR_OP_F2I:
                case IR_OP_CALL:
                case IR_OP_POS:
                case IR_OP_POSF:
                case IR_OP_NEG:
                case IR_OP_NEGF:
                case IR_OP_NOT:
                case IR_OP_NOTF:
                    if (inst->args[0].ty & IS_VAR) {
                        if (!bitset_get(varkill, inst->args[0].symbol)) {
                            bitset_set_at(globalname, inst->args[0].symbol, 1);
                        }
                    }
                    bitset_set_at(varkill, inst->ret.symbol, 1);
                    addBB(fnc, inst->ret.symbol, bb);
                    break;
                case IR_OP_RET:
                    break;
                case IR_OP_ARGUMENT:
                    break;
                case IR_OP_RETI:
                case IR_OP_RETF:
                case IR_OP_PARAM:
                    if (inst->ret.ty & IS_VAR) {
                        if (!bitset_get(varkill, inst->ret.symbol)) {
                            bitset_set_at(globalname, inst->ret.symbol, 1);
                        }
                    }
                    break;
                case IR_OP_GETITEMPTR: {
                    arrayAddr *ptr = inst->args[1].arrayAddr;
                    Value     *index;
                    size_t     i;
                    vector_each_address(&ptr->index, i, index) {
                        if (index->ty & IS_VAR) {
                            if (!bitset_get(varkill, index->symbol)) {
                                bitset_set_at(globalname, index->symbol, 1);
                            }
                        }
                    }
                    bitset_set_at(varkill, inst->ret.symbol, 1);
                    addBB(fnc, inst->ret.symbol, bb);
                }

                break;
                default:
                    assert(0);
            }
        }
    }
    bitset_it_t it;
    bitset_t    worklist;
    bitset_init(worklist);
    bitset_resize(worklist, vector_len(&fnc->BBs));
    for (bitset_it(it, globalname); !bitset_end_p(it); bitset_next(it)) {
        bool c = *bitset_cref(it);
        if (c) {
            Symbol     *sym = getLocalVariable(fnc, it->index);
            bitset_it_t bbit;
            bitset_init_empty(worklist);
            for (bitset_it(bbit, sym->bbset); !bitset_end_p(bbit);
                 bitset_next(bbit)) {
                bool c = *bitset_cref(bbit);
                if (c) {
                    bitset_set_at(worklist, bbit->index, 1);
                    // BasicBlock *ptr = vector_get(&fnc->BBs, bbit->index);
                    // vector_push_back(&worklist, ptr);
                }
            }
            while (1) {
                bool        changed = false;
                bitset_it_t it;
                for (bitset_it(it, worklist); !bitset_end_p(it);
                     bitset_next(it)) {
                    bool c = *bitset_cref(it);
                    if (c) {
                        BasicBlock *ptr = vector_get(&fnc->BBs, it->index);
                        BasicBlock *df;
                        vector_each3(&ptr->df, df) {
                            // 要检查是否已经插入过
                            if (!insertphi(fnc, sym, df)) {
                                bitset_set_at(worklist, df->bbName, 1);
                                changed = true;
                            }
                        }
                    }
                }
                if (!changed) break;
            }
        }
    }
}
ssaSymbol *newName(Function *fnc, Symbol *sym) {
    ssaSymbol *ssa = sy_malloc(sizeof(ssaSymbol));
    ssa->ssaind = vector_len(&fnc->SSAValuePool);
    vector_push_back(&fnc->SSAValuePool, ssa);

    ssa->sym = sym;
    ssa->valueind = sym->count;
    sym->count++;
    ssa->ty = sym->type | IS_SSA;
    if (sym->ReachingDef) {
        ssa->prev = sym->ReachingDef;
        sym->ReachingDef = ssa;
    } else {
        sym->ReachingDef = ssa;
    }
    return ssa;
}
void Rename(Function *fnc, BasicBlock *bb) {
    size_t       i;
    instruction *inst = NULL;
    vector_each(&bb->inst, i, inst) {
        if (inst->op == IR_OP_PHI) {
            Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
            inst->ret.ssaind = newName(fnc, s)->ssaind;
            inst->ret.ty |= IS_SSA;
        }
    }
    vector_each(&bb->inst, i, inst) {
        Symbol *s;
        switch (inst->op) {
            OP3_3AC_COND
            if (inst->args[0].ty & IS_VAR) {
                Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                inst->args[0].ssaind = s->ReachingDef->ssaind;
                inst->args[0].ty |= IS_SSA;
            }
            if (inst->args[1].ty & IS_VAR) {
                Symbol *s = getLocalVariable(fnc, inst->args[1].symbol);
                inst->args[1].ssaind = s->ReachingDef->ssaind;
                inst->args[1].ty |= IS_SSA;
            }
            s = getLocalVariable(fnc, inst->ret.symbol);
            inst->ret.ssaind = newName(fnc, s)->ssaind;
            inst->ret.ty |= IS_SSA;
            break;
            OP3_3AC_OPER
            if (inst->args[0].ty & IS_VAR) {
                Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                inst->args[0].ssaind = s->ReachingDef->ssaind;
                inst->args[0].ty |= IS_SSA;
            }
            if (inst->args[1].ty & IS_VAR) {
                Symbol *s = getLocalVariable(fnc, inst->args[1].symbol);
                inst->args[1].ssaind = s->ReachingDef->ssaind;
                inst->args[1].ty |= IS_SSA;
            }
            s = getLocalVariable(fnc, inst->ret.symbol);
            inst->ret.ssaind = newName(fnc, s)->ssaind;
            inst->ret.ty |= IS_SSA;
            break;
            case IR_OP_MOV:
                assert(0);
            case IR_OP_SOTRE:
                // 2种情况如果
                if (inst->args[0].ty & IS_VAR) {
                    Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                    inst->args[0].ssaind = s->ReachingDef->ssaind;
                    inst->args[0].ty |= IS_SSA;
                }
                if (!(inst->ret.ty & IS_GLOBAL) && inst->ret.ty & IS_POINTER) {
                    Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
                    inst->ret.ssaind = s->ReachingDef->ssaind;
                    inst->ret.ty |= IS_SSA;
                } else if (!(inst->ret.ty & IS_GLOBAL) &&
                           inst->ret.ty & IS_VAR) {
                    s = getLocalVariable(fnc, inst->ret.symbol);
                    inst->ret.ssaind = newName(fnc, s)->ssaind;
                    inst->ret.ty |= IS_SSA;
                }
                if (!(inst->ret.ty & IS_POINTER)) inst->op = IR_OP_MOV;
                break;
            case IR_OP_LOAD:
                if (!(inst->args[0].ty & IS_GLOBAL) &&
                    inst->args[0].ty & IS_VAR) {
                    Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                    inst->args[0].ssaind = s->ReachingDef->ssaind;
                    inst->args[0].ty = s->ReachingDef->ty | IS_SSA;
                }
                s = getLocalVariable(fnc, inst->ret.symbol);
                inst->ret.ssaind = newName(fnc, s)->ssaind;
                inst->ret.ty |= IS_SSA;
                break;
            case IR_OP_CALL:
                s = getLocalVariable(fnc, inst->ret.symbol);
                inst->ret.ssaind = newName(fnc, s)->ssaind;
                inst->ret.ty |= IS_SSA;
                break;
                break;
            case IR_OP_I2F:
            case IR_OP_F2I:
            case IR_OP_POS:
            case IR_OP_POSF:
            case IR_OP_NEG:
            case IR_OP_NEGF:
            case IR_OP_NOT:
            case IR_OP_NOTF:
                if (inst->args[0].ty & IS_VAR) {
                    Symbol *s = getLocalVariable(fnc, inst->args[1].symbol);
                    inst->args[0].ssaind = s->ReachingDef->ssaind;
                    inst->args[0].ty |= IS_SSA;
                }
                s = getLocalVariable(fnc, inst->ret.symbol);
                inst->ret.ssaind = newName(fnc, s)->ssaind;
                inst->ret.ty |= IS_SSA;
                break;
            case IR_OP_ARGUMENT:
                s = getLocalVariable(fnc, inst->ret.symbol);
                inst->ret.ssaind = newName(fnc, s)->ssaind;
                inst->ret.ty |= IS_SSA;

            case IR_OP_RET:
                break;
            case IR_OP_RETI:
            case IR_OP_RETF:
            case IR_OP_PARAM:
                if (inst->ret.ty & IS_VAR) {
                    Symbol *s = getLocalVariable(fnc, inst->ret.symbol);
                    if (!s->ReachingDef) break;
                    inst->ret.ssaind = s->ReachingDef->ssaind;
                    inst->ret.ty = s->ReachingDef->ty | IS_SSA;
                }
                break;
            case IR_OP_GETITEMPTR:
                if (inst->args[0].ty & IS_VAR) {
                    Symbol *s = getLocalVariable(fnc, inst->args[0].symbol);
                    if (s->ReachingDef) {
                        inst->args[0].ssaind = s->ReachingDef->ssaind;
                        inst->args[0].ty |= IS_SSA;
                    }
                }
                arrayAddr *ptr = inst->args[1].arrayAddr;
                Value     *index;
                size_t     i;
                vector_each_address(&ptr->index, i, index) {
                    if (index->ty & IS_VAR) {
                        Symbol *s = getLocalVariable(fnc, index->symbol);
                        index->ssaind = s->ReachingDef->ssaind;
                        index->ty |= IS_SSA;
                    }
                }
                s = getLocalVariable(fnc, inst->ret.symbol);
                inst->ret.ssaind = newName(fnc, s)->ssaind;
                inst->ret.ty |= IS_SSA;
                break;
            case IR_OP_PHI:
                break;
            default:
                assert(0);
        }
    }
    if (bb->nextblock) {
        size_t       i;
        instruction *inst = NULL;
        vector_each(&bb->nextblock->inst, i, inst) {
            if (inst->op == IR_OP_PHI) {
                phiFunction *phi = inst->args[0].phi;
                phiParam     tmp;
                memset(&tmp, 0, sizeof(phiParam));
                if (phi->symbol->ReachingDef) {
                    tmp.v.ssaind = phi->symbol->ReachingDef->ssaind;
                    tmp.v.ty = IS_VAR | IS_SSA;
                    tmp.bb = bb;
                    vector_push_back(&phi->param, tmp);
                }
            }
        }
    }
    if (bb->branchblock) {
        size_t       i;
        instruction *inst = NULL;
        vector_each(&bb->branchblock->inst, i, inst) {
            if (inst->op == IR_OP_PHI) {
                phiFunction *phi = inst->args[0].phi;
                phiParam     tmp;
                memset(&tmp, 0, sizeof(phiParam));
                if (phi->symbol->ReachingDef) {
                    tmp.v.ssaind = phi->symbol->ReachingDef->ssaind;
                    tmp.v.ty = IS_VAR | IS_SSA;
                    tmp.bb = bb;
                    vector_push_back(&phi->param, tmp);
                }
            }
        }
    }
    BasicBlock *ptr;
    vector_each3(&fnc->BBs, ptr) {
        if (ptr->idom == ptr) continue;
        if (ptr->idom == bb) {
            Rename(fnc, ptr);
        }
    }
    vector_each(&bb->inst, i, inst) {
        ssaSymbol *s;
        switch (inst->op) {
            OP3_3AC_COND
            break;
            OP3_3AC_OPER
            s = getSSAlVariable(fnc, inst->ret.ssaind);
            s->sym->ReachingDef = s->sym->ReachingDef->prev;
            break;
            case IR_OP_MOV:
                s = getSSAlVariable(fnc, inst->ret.ssaind);
                s->sym->ReachingDef = s->sym->ReachingDef->prev;
                break;
            case IR_OP_SOTRE:
                // IR_OP_SOTRE并没有定义
                break;
            case IR_OP_LOAD:
                s = getSSAlVariable(fnc, inst->ret.ssaind);
                s->sym->ReachingDef = s->sym->ReachingDef->prev;
                break;
            case IR_OP_I2F:
            case IR_OP_F2I:
            case IR_OP_CALL:
            case IR_OP_POS:
            case IR_OP_POSF:
            case IR_OP_NEG:
            case IR_OP_NEGF:
            case IR_OP_NOT:
            case IR_OP_NOTF:
                s = getSSAlVariable(fnc, inst->ret.ssaind);
                s->sym->ReachingDef = s->sym->ReachingDef->prev;
                break;
            case IR_OP_PHI:
            case IR_OP_ARGUMENT:
                s = getSSAlVariable(fnc, inst->ret.ssaind);
                s->sym->ReachingDef = s->sym->ReachingDef->prev;
                break;

            case IR_OP_RET:
            case IR_OP_RETI:
            case IR_OP_RETF:
            case IR_OP_PARAM:
                break;
            case IR_OP_GETITEMPTR:
                s = getSSAlVariable(fnc, inst->ret.ssaind);
                s->sym->ReachingDef = s->sym->ReachingDef->prev;
                break;
            default:
                assert(0);
        }
    }
}

// 废案
static void _dce(Function *fnc) {
    Symbol *s;
    if (vector_len(&fnc->BBs) != 2) return;
    size_t      i;
    BasicBlock *bb = NULL;
    vector_each(&fnc->BBs, i, bb) {
        size_t       count;
        instruction *head, *inst, *tmp;
        head = bb->instlist;
        DL_FOREACH_SAFE(head, inst, tmp) {
            switch (inst->op) {
                case IR_OP_ARGUMENT:
                    vector_push_back(&bb->tmplist, inst);
                    inst->marked = true;
                    break;
                case IR_OP_SOTRE:
                    if (inst->ret.ty & IS_GLOBAL) {
                        vector_push_back(&bb->tmplist, inst);
                        inst->marked = true;
                    }
                    break;
                case IR_OP_RET:
                    vector_push_back(&bb->tmplist, inst);
                    inst->marked = true;
                    break;
                default:
                    return;
            }
        }
    }
    vector_each(&fnc->BBs, i, bb) {
        size_t       count;
        instruction *head, *inst, *tmp;
        bb->instlist = NULL;
        vector_each3(&bb->tmplist, inst) {
            inst->next = NULL;
            inst->prev = NULL;
            DL_APPEND(bb->instlist, inst);
        }
        // DL_FOREACH_SAFE(head, inst, tmp) {
        //     if (inst->marked) {
        //         inst->next = NULL;
        //         inst->prev = NULL;
        //         DL_APPEND(bb->instlist, inst);
        //         // TODO 内存泄漏
        //     }
        // }
    }
}
/**
 * @brief 构建ssa
 * @param[in] *fnc
 */
bool ssaPass(Function *fnc) {
    _dce(fnc);
    size_t      i;
    BasicBlock *bb = NULL;

    BuildDominetTree(fnc);
    BuildPostorderDominetTree(fnc);

    buildDF(fnc);
    // LOOPALLBB({
    //     size_t      i;
    //     BasicBlock *tmp;
    //     printf("bb_%d:", bb->bbName);
    //     vector_each(&bb->df, i, tmp) { printf("bb_%d,", tmp->bbName); };
    //     printf("\n");
    // })
    buildPostDF(fnc);

    insertPhi(fnc);
    fnc->insert = 0;
    vector_each(&fnc->BBs, i, bb) {
        size_t       count;
        instruction *head, *inst, *tmp;
        DL_COUNT(bb->instlist, inst, count);
        vector_reserve(&bb->inst, count);
        head = bb->instlist;
        DL_FOREACH_SAFE(head, inst, tmp) { vector_push_back(&bb->inst, inst); }
    }
    variableRenaming(fnc);
    fnc->vState = VAL_SSA;
    printBasicAllBlock(stdout, fnc);
}