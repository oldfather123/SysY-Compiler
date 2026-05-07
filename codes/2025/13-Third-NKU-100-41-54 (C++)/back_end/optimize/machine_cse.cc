// #include "machine_cse.h"
// #include <functional>

// // #define MachineCSE_DEBUG

// void RiscV64CSE::Execute() {
//     for (auto func : unit->functions) {
//         current_func = func;
//         CSEInCurrFunc();
//     }
// }

// static bool IsNeedCSE(MachineBaseInstruction *I) {
//     for (auto preg : I->GetReadReg()) {
//             if (!preg->is_virtual) {
//                 return false;
//             }
//         }
//         for (auto preg : I->GetWriteReg()) {
//             if (!preg->is_virtual) {
//                 return false;
//             }
//         }

//     // if (I->arch == MachineBaseInstruction::COPY) {
//     //     return true;
//     // }
//     if (I->arch == MachineBaseInstruction::RiscV) {
//         auto riscv_ins = (RiscV64Instruction *)I;
//         auto op=riscv_ins->getOpcode();
//         if (op == RISCV_LUI || op == RISCV_ADDI || op == RISCV_ADD ||
//             op == RISCV_MUL || op == RISCV_ADDIW) {
//             return true;
//         }
//         auto rs1=riscv_ins->getRs1();
//         auto rs2=riscv_ins->getRs2();
//         auto rd=riscv_ins->getRd();
//         //对应SysY的COPY部分
//         if(op ==RISCV_ADDW||op ==RISCV_ADD)
//         {
//             if(rs1.reg_no==RISCV_x0){return true;}
//             else if(rs2.reg_no==RISCV_x0){return true;}
//         }
//         else if(op==RISCV_FMV_S)
//         {
//             return true;
//         }
//     }
//     return false;
// }

// static std::string GetCSEInstInfo(MachineBaseInstruction *I) {
//     // if (I->arch == MachineBaseInstruction::COPY) {
//     //     auto cpI = (MachineCopyInstruction *)I;
//     //     return cpI->GetDst()->toString() + " = COPY " + cpI->GetSrc()->toString();
//     // } else if (I->arch == MachineBaseInstruction::RiscV) {
//     if (I->arch == MachineBaseInstruction::RiscV) {
//         auto riscv_ins = (RiscV64Instruction *)I;
//         if (riscv_ins->getOpcode() == RISCV_ADD) {
//             return "%" + std::to_string(riscv_ins->getRd().reg_no) + " = %" + std::to_string(riscv_ins->getRs1().reg_no) + " + " +
//                    "%" + std::to_string(riscv_ins->getRs2().reg_no);
//         } else if (riscv_ins->getOpcode() == RISCV_ADDI) {
//             if (riscv_ins->getUseLabel()) {
//                 return "%" + std::to_string(riscv_ins->getRd().reg_no) + " = %" + std::to_string(riscv_ins->getRs1().reg_no) +
//                        " + " + riscv_ins->getLabel().get_data_name();
//             } else {
//                 return "%" + std::to_string(riscv_ins->getRd().reg_no) + " = %" + std::to_string(riscv_ins->getRs1().reg_no) +
//                        " + " + std::to_string(riscv_ins->getImm());
//             }
//         } else if (riscv_ins->getOpcode() == RISCV_ADDIW) {
//             if (riscv_ins->getUseLabel()) {
//                 return "%" + std::to_string(riscv_ins->getRd().reg_no) + " = %" + std::to_string(riscv_ins->getRs1().reg_no) +
//                        " + " + riscv_ins->getLabel().get_data_name();
//             } else {
//                 return "%" + std::to_string(riscv_ins->getRd().reg_no) + " = %" + std::to_string(riscv_ins->getRs1().reg_no) +
//                        " + " + std::to_string(riscv_ins->getImm());
//             }
//         } else if (riscv_ins->getOpcode() == RISCV_LUI) {
//             if (riscv_ins->getUseLabel()) {
//                 return "%" + std::to_string(riscv_ins->getRd().reg_no) + " = lui " + riscv_ins->getLabel().get_data_name();
//             } else {
//                 return "%" + std::to_string(riscv_ins->getRd().reg_no) + " = lui " + std::to_string(riscv_ins->getImm());
//             }
//         } else if (riscv_ins->getOpcode() == RISCV_MUL) {
//             return "%" + std::to_string(riscv_ins->getRd().reg_no) + " = %" + std::to_string(riscv_ins->getRs1().reg_no) + " * " +
//                    "%" + std::to_string(riscv_ins->getRs2().reg_no);
//         } else {
//             ERROR("Unexpected Opcode");
//         }
//     } else {
//         ERROR("Unexpected Arch");
//     }
//     return "";
// }

// // if the value I1 calculated is same as I2, return true
// static bool isSameInst(MachineBaseInstruction *I1, MachineBaseInstruction *I2) {
//     if (I1->arch != I2->arch) {
//         return false;
//     }
//     // if (I1->arch == MachineBaseInstruction::COPY) {
//     //     auto cpI1 = (MachineCopyInstruction *)I1;
//     //     auto cpI2 = (MachineCopyInstruction *)I2;
//     //     return cpI1->GetSrc()->toString() == cpI2->GetSrc()->toString() && cpI1->GetCopyType() == cpI2->GetCopyType();

//     // } else if (I1->arch == MachineBaseInstruction::RiscV) {
//     if (I1->arch == MachineBaseInstruction::RiscV) {
//         auto riscv_ins1 = (RiscV64Instruction *)I1;
//         auto riscv_ins2 = (RiscV64Instruction *)I2;
//         if (riscv_ins1->getOpcode() != riscv_ins2->getOpcode()) {
//             return false;
//         }
//         if (riscv_ins1->getOpcode() == RISCV_ADD) {
//             return riscv_ins1->getRs1() == riscv_ins2->getRs1() && riscv_ins1->getRs2() == riscv_ins2->getRs2();
//         } else if (riscv_ins1->getOpcode() == RISCV_ADDI) {
//             if (riscv_ins1->getUseLabel()) {
//                 return riscv_ins1->getRs1() == riscv_ins2->getRs1() && riscv_ins1->getLabel() == riscv_ins2->getLabel();
//             } else {
//                 return riscv_ins1->getRs1() == riscv_ins2->getRs1() && riscv_ins1->getImm() == riscv_ins2->getImm();
//             }
//         } else if (riscv_ins1->getOpcode() == RISCV_ADDIW) {
//             if (riscv_ins1->getUseLabel()) {
//                 return riscv_ins1->getRs1() == riscv_ins2->getRs1() && riscv_ins1->getLabel() == riscv_ins2->getLabel();
//             } else {
//                 return riscv_ins1->getRs1() == riscv_ins2->getRs1() && riscv_ins1->getImm() == riscv_ins2->getImm();
//             }
//         } else if (riscv_ins1->getOpcode() == RISCV_LUI) {
//             if (riscv_ins1->getUseLabel()) {
//                 return riscv_ins1->getLabel() == riscv_ins2->getLabel();
//             } else {
//                 return riscv_ins1->getImm() == riscv_ins2->getImm();
//             }
//         } else if (riscv_ins1->getOpcode() == RISCV_MUL) {
//             return riscv_ins1->getRs1() == riscv_ins2->getRs1() && riscv_ins1->getRs2() == riscv_ins2->getRs2();
//         //COPY相关（排除ADD，ADD用上面的即可）
//         } else if (riscv_ins1->getOpcode() == RISCV_ADDW) {
//             return riscv_ins1->getRs1() == riscv_ins2->getRs1() && riscv_ins1->getRs2() == riscv_ins2->getRs2();
//         } else if (riscv_ins1->getOpcode() == RISCV_FMV_S) {
//             return riscv_ins1->getRs1() == riscv_ins2->getRs1() && riscv_ins1->getRs2() == riscv_ins2->getRs2();
//         } else {
//             ERROR("Unexpected Opcode");
//         }
//     } else {
//         ERROR("Unexpected Arch");
//     }
//     return false;
// }
// void RiscV64CSE::dfs(int bbid)
// {
//     std::set<MachineBaseInstruction *> tmpcse_set;

//     auto C = current_func->getMachineCFG();
//     MachineBlock *now = C->GetNodeByBlockId(bbid)->Mblock;
//     for (auto it = now->begin(); it != now->end(); ++it) {
//         auto I = *it;
//         //1.跳过不需要cse的指令
//         if (!IsNeedCSE(I)) {
//             continue;
//         }
//         bool is_cse = false;
//         //2.在CSE_SET中查找相同指令
//         for (auto oldI : CSESet) {
//             if (isSameInst(I, oldI)) {
// #ifdef MachineCSE_DEBUG
//                 std::cerr << GetCSEInstInfo(oldI) << "\n";
//                 std::cerr << GetCSEInstInfo(I) << "\n\n";
// #endif

//                 is_cse = true;
//                 is_changed = true;
//                 auto vw1 = oldI->GetWriteReg();
//                 auto vw2 = I->GetWriteReg();
//                 assert(vw1.size() == 1 && vw2.size() == 1);
//                 regreplace_map[vw2[0]->reg_no] = vw1[0]->reg_no;
//                 break;
//             }
//         }
//         if (!is_cse) {
//             tmpcse_set.insert(I);
//             CSESet.insert(I);
//         }
//     }
//     for (auto v : C->DomTree->dom_tree[bbid]) {
//         dfs(v->getLabelId());
//     }

//     for (auto I : tmpcse_set) {
//         CSESet.erase(I);
//     }
// }
// void RiscV64CSE::CSEInCurrFunc() {
//     bool is_changed = true;
//     std::set<MachineBaseInstruction *> CSESet;
//     std::map<int, int> regreplace_map;
//     // 1.循环优化直到无变化
//     while (is_changed) {
//         CSESet.clear();
//         regreplace_map.clear();

//         is_changed = false;
//         dfs(0); // 从入口块开始DFS（BlockID=0）

//         // === 寄存器替换阶段 ===
//         //2.遍历所有函数应用寄存器替换
//         for (auto func : unit->functions) {
//             auto C = func->getMachineCFG();
//             C->seqscan_open();
//             while (C->seqscan_hasNext()) {
//                 auto block = C->seqscan_next()->Mblock;
//                 for (auto it = block->begin(); it != block->end(); ++it) {
//                     auto I = *it;
//                     for (auto preg : I->GetReadReg()) {
//                         if (preg->is_virtual) {
//                             if (regreplace_map.find(preg->reg_no) != regreplace_map.end()) {
//                                 preg->reg_no = regreplace_map[preg->reg_no];
//                             }
//                         }
//                     }
//                 }
//             }
//         }
//         // === 删除无用指令阶段 ===
//         // erase dead def
//         std::map<int, int> vreg_refcnt;
//         //3.统计当前函数中虚拟寄存器的引用次数
//         for (auto block : current_func->blocks) {
//             cur_block = block;
//             for (auto it = block->begin(); it != block->end(); ++it) {
//                 auto cur_ins = *it;
//                 for (auto reg : cur_ins->GetReadReg()) {
//                     if (reg->is_virtual) {
//                         vreg_refcnt[reg->reg_no] = vreg_refcnt[reg->reg_no] + 1;
//                     }
//                 }
//             }
//         }
//         //4.删除无引用的指令
//         for (auto block : current_func->blocks) {
//             cur_block = block;
//             for (auto it = block->begin(); it != block->end(); ++it) {
//                 auto cur_ins = *it;
//                 if (cur_ins->GetWriteReg().size() == 1) {
//                     auto rd = cur_ins->GetWriteReg()[0];
//                     if (rd->is_virtual) {
//                         if (vreg_refcnt[rd->reg_no] == 0) {
//                             it = block->erase(it);
//                             --it;
//                         }
//                     }
//                 }
//             }
//         }
//         // next iteration
//     }
// }