#include "riscv.h"

void remove_redundant_moves(Program_Asm *prog) {
    int cnt = 0;
    for(auto f : prog->functions) {
        for(auto mb : f->mbs) {
            for(auto I = mb->inst; I; I=I->next) {
                if(I->tag == VI_MOVE) {
                    auto mv = (VI_Move *)I;
                    if(mv->src.tag == IMM || mv->dst.tag == REG)
                        continue;
                    replace_operand_by_operand(&mv->dst, &mv->src, f);
                    mv->erase_from_parent();
                    cnt ++;
                }
            }
        }
    }
    cerr << "[opt]: remove_redundant_moves ereased " << cnt << " instructions\n";
}
