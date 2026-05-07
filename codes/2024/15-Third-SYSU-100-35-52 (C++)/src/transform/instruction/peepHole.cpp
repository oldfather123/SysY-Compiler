#include "peepHole.h"
#include "Instruction.h"
#include "Value.h"
#include "Variable.h"

static bool storeValNoUseInBB(StoreInstruction* SI, BasicBlock* bb) {
    auto ptr = SI->getPtr();
    auto addr = ptr->as<User>(); 
    for(auto user : addr->users()) {
        if(auto LI = user->as<LoadInstruction>()) {
            if(LI->getBasicBlock() == bb) {
                return false;
            }
        }
    }
    return true;
}

bool runPeepHole(FunctionPtr func) {
    bool Changed = false;
    std::vector<Instruction*> toDelete;

    for(auto& BB : func->basicBlocks) {
        for(auto I = BB->begin(), E = BB->end(); I != E; ++I) {
            // Check if the current instruction is a store
            if(auto* SI = dynamic_cast<StoreInstruction*>(I->get())) {
                // Get the next instruction
                auto Next = std::next(I);
                if(Next == E)
                    continue;
                // Check if the next instruction is a load
                if(auto* LI = dynamic_cast<LoadInstruction*>(Next->get())) {
                    // Check if the load is from the same address as the store
                    if(SI->getPtr() == LI->getPtr()) {
                        // Replace all uses of the loaded value with the stored value
                        LI->replaceWith(SI->getValue());
                        // Erase the load instruction
                        toDelete.push_back(Next->get());
                        Changed = true;
                    }
                } else if(auto* SI2 = dynamic_cast<StoreInstruction*>(Next->get())) {
                    // Check if the next instruction is a store
                    // Check if the store is to the same address as the current store
                    if(SI->getPtr() == SI2->getPtr()) {
                        // Replace the stored value of the next store with the stored value
                        // of the current store
                        // SI2->setOperand(0, SI->getValueOperand());
                        // Erase the current store instruction
                        toDelete.push_back(I->get());
                        Changed = true;
                    }
                }
            }
            // if (auto LI = dynamic_cast<LoadInst>(&*I)) {
            //   // it the next is load, and the load is from the same address as the
            //   // load replace the load with the next load
            //   auto Next = std::next(I);
            //   if (Next == E)
            //     continue;
            //   if (auto LI2 = dynamic_cast<LoadInst>(&*Next)) {
            //     if (LI->getPointerOperand() == LI2->getPointerOperand()) {
            //       errs() << "replace load" << *LI << " with " << *LI2 << "\n";
            //       LI->replaceAllUsesWith(LI2);
            //       //LI->eraseFromParent();
            //       toDelete.push_back(&*LI);
            //       Changed = true;
            //       Count++;
            //       continue;
            //     }
            //   }
            // }

            // binary operation  cse
            if(auto BI = dynamic_cast<BinaryInstruction*>(I->get())) {
                if (BI->op == BinaryInstructionOps::Not) continue;  
                auto Next = std::next(I);
                if(Next == E)
                    continue;
                if(auto BI2 = dynamic_cast<BinaryInstruction*>(Next->get())) {
                    if(BI->getOp() == BI2->getOp() && BI->getOperand(0) == BI2->getOperand(0) &&
                       BI->getOperand(1) == BI2->getOperand(1)) {
                        BI->replaceWith(BI2);
                        toDelete.push_back(BI);
                        Changed = true;
                    }
                }
            }
            // for gep
            if(auto GEP = dynamic_cast<GetElementPtrInstruction*>(I->get())) {
                auto Next = std::next(I);
                if(Next == E)
                    continue;
                if(auto GEP2 = dynamic_cast<GetElementPtrInstruction*>(Next->get())) {
                    if(GEP->getBase() == GEP2->getBase() && GEP->getNumOperands() == GEP2->getNumOperands()) {
                        bool same = true;
                        for(int i = 0; i < GEP->getNumOperands(); i++) {
                            if(GEP->getOperand(i) != GEP2->getOperand(i)) {
                                same = false;
                                break;
                            }
                        }
                        if(same) {
                            GEP->replaceWith(GEP2);
                            toDelete.push_back(GEP);
                            Changed = true;
                        }
                    }
                }
            }
        }
    }

    for(auto* I : toDelete) {
        I->eraseFromParent();
    }
    toDelete.clear();

//TODO: fix bug
//int a = 0;
//void fun(int a) {
//    return a;
//}
//
//int main() {
//    return fun(a);
//}

    //std::set<Instruction*> visited;
    //// 对于store指令 在同一个基本块中 如果store的地址相同 而且没有use
    //// 那么只保留最后一条store
    //for(auto& bb : func->basicBlocks)
    //    for(auto I = bb->begin(), E = bb->end(); I != E; ++I) {
    //        if(auto SI = dynamic_cast<StoreInstruction*>(I->get())) {
    //            if(!visited.insert(SI).second)
    //                continue;
    //            if(storeValNoUseInBB(SI, bb.get())) {
    //                // 遍历之后的store指令
    //                toDelete.push_back(&*SI);
    //                for(auto J = std::next(I); J != E; ++J) {
    //                    if(auto* SI2 = dynamic_cast<StoreInstruction*>(J->get())) {
    //                        if(SI->getPtr() == SI2->getPtr()) {
    //                            visited.insert(SI2);
    //                            toDelete.push_back(SI2);
    //                            Changed = true;
    //                        }
    //                    }
    //                }
    //                toDelete.pop_back();
    //            }
    //        }
    //    }
    //for(auto* I : toDelete) {
    //    I->eraseFromParent();
    //}

    //toDelete.clear();

    // 优化sext 指令 replace the const to val
    //for(auto& bb : func->basicBlocks) {
    //    for(auto I = bb->begin(), E = bb->end(); I != E; ++I) {
    //        if(auto SI = dynamic_cast<ExtInstruction*>(I->get())) {
    //            if(auto CI = dynamic_cast<Const*>(SI->getOperand(0))) {
    //                auto val = Const::getConst(, CI->getValue().sext(SI->getType()->getIntegerBitWidth()));
    //                SI->replaceAllUsesWith(val);
    //                toDelete.push_back(&*SI);
    //                Changed = true;
    //                Count++;
    //            }
    //            // TODO: just test 不是常数也可以
    //            else {
    //                auto val = SI->getOperand(0);
    //                SI->replaceAllUsesWith(val);
    //                toDelete.push_back(&*SI);
    //                Changed = true;
    //                Count++;
    //            }
    //        }
    //    }
    //}

    //for(auto* I : toDelete) {
    //    I->eraseFromParent();
    //}

    //toDelete.clear();

    //TODO: fix bug
    // if the store inst's ptr has no use, then delete the store inst
    //for(auto& bb : func->basicBlocks) {
    //    for(auto I = bb->begin(), E = bb->end(); I != E; ++I) {
    //        if(auto SI = dynamic_cast<StoreInstruction*>(I->get())) {
    //            if(!SI->getPtr()->as<User>()->isUsed() && SI->getPtr()->valueId() == ValueID::Variable && SI->getPtr()->as<Variable>()->isGlobal) {
    //                toDelete.push_back(SI);
    //                Changed = true;
    //            }
    //        }
    //    }
    //}

    //for(auto* I : toDelete) {
    //    I->eraseFromParent();
    //}

    //toDelete.clear();

    // 对于phi 节点 如果 incoming block 不是前缀block，那么删除这个incoming
    //for(auto& bb : func->basicBlocks) {
    //    auto preds = predecessors(&bb);
    //    for(auto I = bb.begin(), E = bb.end(); I != E; ++I) {
    //        if(auto PI = dynamic_cast<PHINode>(&*I)) {
    //            for(int i = 0; i < PI->getNumIncomingValues(); i++) {
    //                auto incomingBlock = PI->getIncomingBlock(i);
    //                if(std::find(preds.begin(), preds.end(), incomingBlock) == preds.end()) {
    //                    PI->removeIncomingValue(i);
    //                    Changed = true;
    //                }
    //            }
    //        }
    //    }
    //}

    //// 对于单独的phi节点，如果phi节点的值只有一个，那么将phi节点替换为这个值
    //for(auto& bb : F) {
    //    for(auto I = bb.begin(), E = bb.end(); I != E; ++I) {
    //        if(auto PI = dynamic_cast<PHINode>(&*I)) {
    //            if(PI->getNumIncomingValues() == 1) {
    //                PI->replaceAllUsesWith(PI->getIncomingValue(0));
    //                toDelete.push_back(&*PI);
    //                Changed = true;
    //            }
    //        }
    //    }
    //}

    //for(auto* I : toDelete) {
    //    I->eraseFromParent();
    //}
    return Changed;
}