#include "riscv.h"

static bool maybe_unused(VI_Tag kind) {
    return kind == VI_BINARY || kind == VI_LOAD || kind == VI_MOVE;
}

static bool f_maybe_unused(VI_Tag kind) {
    return kind == VI_VBINARY || kind == VI_VLOAD || kind == VI_FMOVE;
}

// yyy: 定义的def是否有被用过
void remove_unused_instruction(Program_Asm *prog) {
    int cnt = 0;
    cerr << "begin remove_unused_instruction:\n";
    for(auto f : prog->functions) {
        for(auto mb : f->mbs) {
            VI *next_instr = mb->inst;
            for(auto I = mb->inst; I; I=next_instr) {
                next_instr = I->next;
                if(maybe_unused(I->tag)) {
                    auto defs = get_defs(I);
                    bool unused = defs.size() > 0;
                    for(auto def: defs) {
                        // yyy: 是REG就设置为false
                        if(def.get_use_head(f) != nullptr || def.tag == REG) {
                            unused = false;
                            break;
                        }
                    }

                    if(unused) {
                        auto tmp = I->prev;
                        I->erase_from_parent();
                        I = tmp;
                        cnt ++;
                    }
                }
                else if(f_maybe_unused(I->tag)) {
                    auto defs = s_get_defs(I);
                    bool unused = defs.size() > 0;
                    for(auto def: defs) {
                        if(def.f_get_use_head(f) != nullptr || def.tag == FREG) {
                            unused = false;
                            break;
                        }
                    }

                    if(unused) {
                        auto tmp = I->prev;
                        I->erase_from_parent();
                        I = tmp;
                        cnt ++;
                    }
                }
            }
        }
    }
    cerr << "[opt]: ereased " << cnt << " unused instructions\n";
}
