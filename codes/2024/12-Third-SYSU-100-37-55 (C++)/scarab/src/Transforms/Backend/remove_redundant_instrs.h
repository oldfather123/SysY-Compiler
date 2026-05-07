#include "riscv.h"

void remove_redundant_instrs(Program_Asm *prog) {
	for (auto f : prog->functions) {
		for (auto mb : f->mbs) {
			for (auto I = mb->inst; I; I = I->next) 
			{
				if (I->tag == VI_STORE && I->next)
				{
					if (I->next->tag == VI_LOAD)
					{ 
						auto str = (VI_Store *)I;
						auto ldr = (VI_Load *)I->next;
                        if (str->base == ldr->base) {
                            if (str->offset.tag == ERRORTYPE || (str->offset == ldr->offset))
                            {
                                if (!(str->reg == ldr->reg)) {
                                    auto mv = new VI_Move(ldr->reg, str->reg);
                                    insert(mv, ldr);
                                    I = mv;
                                }
                                ldr->mark();
                            }
                        }
					}
				}else if(I->tag == VI_BINARY && I->next)
				{
					auto vi_b = (VI_Binary *)I;
					if(vi_b->op == BINARY_XOR && vi_b->rhs==create_imm(0) &&I->next->tag == VI_SNEZ){
						auto vi_snez = (VI_Snez *)(I->next);
						if(vi_snez->src == vi_b->dst){
							auto sltu = new VI_Slt(vi_snez->dst, create_reg(zero), vi_b->lhs, SLT_U);
							insert(sltu, vi_snez);
							I = sltu;
						}
						vi_b->mark();
						vi_snez->mark();
					}
				}
			}
            mb->erase_marked_values();
		}
	}
}
