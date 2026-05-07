#include "riscv.h"

void use_analysis(Program_Asm *prog) {
    for(auto f : prog->functions) {
        f->clear_use_head_map();
        f->clear_def_I_map();
        for(auto mb : f->mbs) {
            for(auto I = mb->inst; I; I=I->next) {
                auto defs = get_defs_ptr(I);
                auto uses = get_uses_ptr(I);
                if(defs.size() == 0)
                    defs.push_back(nullptr);
                for(auto def: defs) {
                    for(auto use: uses) {
                        // def 用上之前use的值
                        auto mi_use = new VI_Use(use, def, I);
                        // yyy: 别弄混了，这个use和use表没关系，是src operand，即被use的
                        use->add_use(mi_use, f);
                    }
                    if(def != nullptr)
                        def->set_def_I(I, f);
                }

                auto s_defs = s_get_defs_ptr(I);
                auto s_uses = s_get_uses_ptr(I);
                if(s_defs.size() == 0)
                    s_defs.push_back(nullptr);
                for(auto def: s_defs) {
                    for(auto use: s_uses) {
                        auto mi_use = new VI_Use(use, def, I);
                        use->s_add_use(mi_use, f);
                    }
                    if(def != nullptr)
                        def->set_def_I(I, f);
                }
            }
        }
    }
    cerr << "[opt]: finish use analysis\n";
}