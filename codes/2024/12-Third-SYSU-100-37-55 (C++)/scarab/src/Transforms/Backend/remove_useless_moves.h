#include "riscv.h"

void remove_useless_moves(Program_Asm *prog) {
    for(auto f : prog->functions) {
        for(auto mb : f->mbs) {
            for(auto I = mb->inst; I; I=I->next) {
                if (I->tag == VI_MOVE) {
                    auto mv = (VI_Move *) I;
                    if (mv->dst == mv->src) {
                        I->mark();
                    }
                }
                else if (I->tag == VI_FMOVE) {
                    auto mv = (VI_FMove *) I;
                    if (!(mv->from_f32) && !(mv->from_s32) && mv->dst == mv->src) {
                        I->mark();
                    }
                }
            }
            mb->erase_marked_values();
        }
    }
}
