#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "../include/bitset.h"
#include "../include/function.h"
#include "../include/ir.h"
#include "../include/m-bitset.h"
#include "../include/pass.h"
#include "../include/symbol.h"
#include "../include/sysy.h"
#include "../include/util.h"
#include "../include/value.h"
#include "../include/vector.h"

typedef struct CFGedge {
    BasicBlock *bb, *in;
} CFGedge;
vector_template(CFGedge, CFGworklist);
vector_template(CFGedge, SSAworklist);

static void SCCP(Function *fnc) {
    vector_clear(&CFGworklist);
    BasicBlock *bb = fnc->firstBB;
    LOOPALLBBNEXT({
        CFGedge tmp;
        tmp.bb = parent;
        tmp.in = bb;
    })
    LOOPALLBB({
        bb->bb1unexecuted = true;
        bb->bb2unexecuted = true;
    })
    ssaSymbol *v;
    vector_each3(&fnc->SSAValuePool, v) {
        // ssaSymbol *v = getSSAlVariable(fnc, it->index);
        v->lattice = lattice_TOP;
    }
    while (vector_len(&CFGworklist) != 0 || vector_len(&SSAworklist) != 0) {
        if (vector_len(&CFGworklist) != 0) {
            CFGedge edge;
            vector_pop_back(&CFGworklist, edge);
            bool unexecuted = false;
            if (edge.bb->nextblock == edge.in) {
                if (edge.bb->bb1unexecuted) unexecuted = true;
            } else if (edge.bb->branchblock == edge.in) {
                if (edge.bb->bb2unexecuted) unexecuted = true;
            } else
                assert(0);
            if (unexecuted) {
                if (edge.bb->nextblock == edge.in)
                    edge.bb->bb1unexecuted = false;
                else if (edge.bb->branchblock == edge.in)
                    edge.bb->bb2unexecuted = false;
                // EvaluateAllPhisInBlock();
                bool executed = false;
                bb = edge.in;
                // 除了edge.bb,没有其他块进入
                LOOPALLBBPREV({
                    if (bb == edge.bb) continue;
                    if (bb->nextblock == edge.in) {
                        if (bb->bb1unexecuted) executed = true;
                    } else if (bb->branchblock == edge.in) {
                        if (bb->bb2unexecuted) executed = true;
                    }
                })
                if (!executed) {
                    BasicBlock *bb = edge.in;
                    if (vector_len(&bb->inst) == 0) {
                        CFGedge tmp;
                        tmp.bb = bb;
                        tmp.in = bb->nextblock;
                        vector_push_back(&CFGworklist, tmp);
                        // 折叠指令
                    }
                }
            }
        }
    }
}

void SCCPPass(Function *fnc) {}