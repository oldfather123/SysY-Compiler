// Referrence from Compiler-2023-YatCC: https://gitlab.eduxiji.net/educg-group-17291-1894922/202310558201558-3109/-/blob/main/src/backEnd/asm/remove_identical_moves.h

#include "riscv.h"

void removeIdenticalMoves(MProg* prog) {
    for(auto func : prog->mFuncs) {
        for(auto mb : func->mBlocks) {
            for(auto I = mb->firInst; I; I = I->next) {
                if(I->tag == MI_MOVE) {
                    auto mvMI = (MI_Move*)I;
                    if(mvMI->dst == mvMI->src) {
                        I->deleteFromParent(func);
                    }
                } else if(I->tag == MI_FMOVE) {
                    auto vmvMI = (MI_FMove*)I;
                    if(!vmvMI->fromS32 && vmvMI->dst == vmvMI->src) {
                        I->deleteFromParent(func);
                    }
                }
            }
        }
    }
}
