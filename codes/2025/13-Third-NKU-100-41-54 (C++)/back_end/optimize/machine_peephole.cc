#include "machine_peephole.h"


void MachinePeepholePass::Execute() {
    EliminateRedundantInstructions();
#if USE_FMA
    FloatCompFusion();
#endif
    ConstantReplacement();
}

void MachinePeepholePass::EliminateRedundantInstructions() {
    // remove instructions like:
    // fmv x, x     √
    // add x, x, 0  √
    // mul x, x, 1  √
    // sub x, x, 0  √
    // div x, x, 1  √
    // li  tx,k     --->直接用k替换
    for (auto &func : unit->functions) {
        for (auto &block : func->blocks) {
            for (auto it = block->instructions.begin(); it != block->instructions.end();) {
                auto inst = (RiscV64Instruction*)(*it);
                //(1) fmv x, x  ---> 直接删除此指令
                if (inst->getOpcode() == RISCV_FMV_S || inst->getOpcode() == RISCV_FMV_D ) {
                    if(inst->getRd().reg_no == inst->getRs1().reg_no) {
                        it = block->instructions.erase(it);
                        continue;
                    }
                }

                //(2) add x, x, 0  ---> 直接删除此指令
                if(inst ->getOpcode() == RISCV_ADDI || inst ->getOpcode() == RISCV_ADDIW) {
                    if (inst->getRd().reg_no == inst->getRs1().reg_no && inst->getImm() == 0) {
                        it = block->instructions.erase(it); 
                        continue;
                    }
                }else if( inst ->getOpcode() == RISCV_ADD || inst ->getOpcode() == RISCV_SUB ||
                          inst ->getOpcode() == RISCV_ADDW || inst ->getOpcode() == RISCV_SUBW ||
                          inst ->getOpcode() == RISCV_FADD_D || inst ->getOpcode() == RISCV_FADD_S ||
                          inst ->getOpcode() == RISCV_FSUB_D || inst ->getOpcode() == RISCV_FSUB_S) {
                    if (inst->getRd().reg_no == inst->getRs1().reg_no ) {
                        auto rs2 = inst->getRs2();
                        if(!rs2.is_virtual && rs2.reg_no == 0) {
                            it = block->instructions.erase(it); 
                            continue;
                        }
                    }
                }

                //（3）add t0, x0, 1
                //    mul xxx, xxx,t0     ---> 自身乘1，直接删除两条指令

                //    add t0, x0, 1
                //    mul xxx, xx, t0      ---> 乘1，改为加0（强度削弱）
                if(inst ->getOpcode() == RISCV_ADDI || inst ->getOpcode() == RISCV_ADDIW) {
                    auto rs1 = inst->getRs1();
                    auto rs2 = inst->getRs2();
                    if (!rs1.is_virtual && rs1.reg_no == 0 && inst->getImm() == 1){// add t0, x0, 1型
                        auto next_it = std::next(it);
                        if (next_it != block->instructions.end()) {
                            auto next_inst = (RiscV64Instruction*)(*next_it);
                            // （2.1）int 型
                            if (next_inst->getOpcode() == RISCV_MUL || next_inst->getOpcode() == RISCV_MULW ||
                                next_inst->getOpcode() == RISCV_MULH || next_inst->getOpcode() == RISCV_MULHSU ||
                                next_inst->getOpcode() == RISCV_MULHU||
                                next_inst->getOpcode() == RISCV_DIV || next_inst->getOpcode() == RISCV_DIVU ||
                                next_inst->getOpcode() == RISCV_DIVW) {
                                if(next_inst->getRd().reg_no == next_inst->getRs1().reg_no &&
                                   next_inst->getRs2().reg_no == inst->getRd().reg_no) {//mul xxx, xxx,t0型
                                    //删除两条指令
                                    it = block->instructions.erase(it);
                                    it = block->instructions.erase(it);
                                    continue;
                                }else if(next_inst->getRd().reg_no != next_inst->getRs1().reg_no &&
                                         next_inst->getRs2().reg_no == inst->getRd().reg_no){
                                    //删除第一条add指令，将mul 1 改为add 0
                                    it = block->instructions.erase(it);
                                    if(next_inst->getOpcode() == RISCV_MUL) {
                                        next_inst->setOpcode(RISCV_ADDI,false);
                                    } else if(next_inst->getOpcode() == RISCV_MULW) {
                                        next_inst->setOpcode(RISCV_ADDIW,false);
                                    }
                                    next_inst->setImm(0);
                                    ++it;
                                    continue;
                                }
                            }
                        }
                    } 
                // (4) addi(w) t0, x0, k
                //     add(w)sub xxx, xx, t0  --->直接使用立即数k
                    if(!rs1.is_virtual && rs1.reg_no == 0 ){// addi t0, x0, k型，t0被使用，可直接用k替换
                        auto next_it = std::next(it);
                        if (next_it != block->instructions.end()) {
                            auto next_inst = (RiscV64Instruction*)(*next_it);
                            if(next_inst->getOpcode() == RISCV_ADD || next_inst->getOpcode() == RISCV_ADDW||
                               next_inst->getOpcode() == RISCV_SUB || next_inst->getOpcode() == RISCV_SUBW) {
                                if(next_inst->getRs2().reg_no== inst->getRd().reg_no) {
                                    if(next_inst->getOpcode() == RISCV_ADD) {
                                        next_inst->setOpcode(RISCV_ADDI,false);
                                        next_inst->setImm(inst->getImm());
                                    } else if(next_inst->getOpcode() == RISCV_ADDW) {
                                        next_inst->setOpcode(RISCV_ADDIW,false);
                                        next_inst->setImm(inst->getImm());
                                    } else if(next_inst->getOpcode() == RISCV_SUB) {
                                        next_inst->setOpcode(RISCV_ADDI,false);
                                        next_inst->setImm(-(inst->getImm()));
                                    } else if(next_inst->getOpcode() == RISCV_SUBW) {
                                        next_inst->setOpcode(RISCV_ADDIW,false);
                                        next_inst->setImm(-(inst->getImm()));
                                    }
                                    
                                    it = block->instructions.erase(it);
                                    ++it;
                                    continue;
                                }
                            }// store 0型： addi tx, x0, 0;  sw tx, imm(reg)  ---> sw x0, imm(reg)
                            else if(next_inst->getOpcode() == RISCV_SW && next_inst->getRs1().reg_no == inst->getRd().reg_no
                                     && inst->getImm()==0){
                                    next_inst->setRs1(inst->getRs1());
                                    it = block->instructions.erase(it);
                                    ++it;
                                    continue;
                            }

                        }
                    }
                }
                
                ++it;
            }
        }
    }
}

void MachinePeepholePass::FloatCompFusion(){
    for (auto &func : unit->functions) {
        for (auto &block : func->blocks) {
            for (auto it = block->instructions.begin(); it != block->instructions.end();) {
                auto inst = (RiscV64Instruction*)(*it);

                //（5）fma /fnma
                if(inst ->getOpcode() == RISCV_FMUL_S){
                    auto next_it = std::next(it);
                    if (next_it != block->instructions.end()) {
                        auto next_inst = (RiscV64Instruction*)(*next_it);
						// FMA 指令单次舍入，精度较高，所以导致了浮点数输出结果错误，暂时关闭
                        if(next_inst->getOpcode() == RISCV_FADD_S || next_inst->getOpcode() == RISCV_FSUB_S) {
						    if(next_inst->getRs2().reg_no == inst->getRd().reg_no) {
                                RiscV64Instruction *fma_inst = new RiscV64Instruction();
                                if(next_inst->getOpcode() == RISCV_FADD_S) {
                                    fma_inst->setOpcode(RISCV_FMADD_S,false);
                                } else {
                                    fma_inst->setOpcode(RISCV_FNMADD_S,false);
                                }
                                fma_inst->setRd(next_inst->getRd());
                                fma_inst->setRs1(inst->getRs1());
                                fma_inst->setRs2(inst->getRs2());
                                fma_inst->setRs3(next_inst->getRs1());

                                it = block->instructions.erase(it);
                                *it = fma_inst;
                                ++it;
                                continue;
                            }else if(next_inst->getRs1().reg_no == inst->getRd().reg_no){
                                RiscV64Instruction *fma_inst = new RiscV64Instruction();
                                if(next_inst->getOpcode() == RISCV_FADD_S) {
                                    fma_inst->setOpcode(RISCV_FMADD_S,false);
                                } else {
                                    fma_inst->setOpcode(RISCV_FMSUB_S,false);
                                }
                                fma_inst->setRd(next_inst->getRd());
                                fma_inst->setRs1(inst->getRs1());
                                fma_inst->setRs2(inst->getRs2());
                                fma_inst->setRs3(next_inst->getRs1());

                                it = block->instructions.erase(it);
                                *it = fma_inst;
                                ++it;
                                continue;
                            }
                        }
                    }
                }
                ++it;
            }
        }
    }
}


void MachinePeepholePass::ConstantReplacement() {
    for (auto &func : unit->functions) {
        for (auto &block : func->blocks) {
            for (auto it = block->instructions.begin(); it != block->instructions.end();) {
                auto inst = (RiscV64Instruction*)(*it); 
                // (6)   li tx, k 
                //     add, ty, xx, tx ---> 直接用k替换tx
                if(inst ->getOpcode() == RISCV_LI) {
                    int imm = inst->getImm();
                    // addi 的 12 位有符号立即数范围为 [-2048, 2047]
                    if (imm >= -2048 && imm <= 2047) {
                        auto next_it = std::next(it);
                        if (next_it != block->instructions.end()) {
                            auto next_inst = (RiscV64Instruction*)(*next_it);
                            if(next_inst->getOpcode() == RISCV_ADD) {
                                if(next_inst->getRs2().reg_no == inst->getRd().reg_no) {
                                    next_inst->setOpcode(RISCV_ADDI,false);
                                    next_inst->setImm(inst->getImm());
                                    it = block->instructions.erase(it);
                                    ++it;
                                    continue;
                                }
                            }
                        }
                    }
                }
                // (7) addi tx, ty, 0
                //     add  xx, tx, xx ---> 直接用ty替换tx
                else if(inst ->getOpcode() == RISCV_ADDI){
                    if(inst->getImm()==0){
                        auto next_it = std::next(it);
                        if (next_it != block->instructions.end()) {
                            auto next_inst = (RiscV64Instruction*)(*next_it);
                            if(next_inst->getOpcode() == RISCV_ADD  || next_inst->getOpcode() == RISCV_ADDI||
                               next_inst->getOpcode() == RISCV_SLLI || next_inst->getOpcode() == RISCV_MUL ||
                               next_inst->getOpcode() == RISCV_DIV  || next_inst->getOpcode() == RISCV_REM ||
                               next_inst->getOpcode() == RISCV_SW   || next_inst->getOpcode() == RISCV_LW ) {
                                
                                if(next_inst->getRs1().reg_no == inst->getRd().reg_no) {
                                    if(inst->getRs1().reg_no == RISCV_sp){
                                    //if(inst->getRd().reg_no == RISCV_t1 && inst->getRs1().reg_no == RISCV_sp) {
                                        // addi t1, sp, 0
                                        // add t0, t1, t0
                                        // 为什么捕捉不到呢？？？？？？
                                        std::cout<< "Occur! The regno of sp is " << inst->getRs1().reg_no << std::endl;
                                    } 
                                    next_inst->setRs1(inst->getRs1());
                                    it = block->instructions.erase(it);
                                    ++it;
                                    continue;
                                }
                            }
                        }
                    }
                    
                }
                ++it;
            }
        }
    }
}

