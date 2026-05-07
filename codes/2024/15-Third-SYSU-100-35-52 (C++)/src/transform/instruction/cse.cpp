#include "cse.h"
#include "CFGAnalysis.h"
#include "domTreeAnalysis.h"
#include <iostream>
#include <queue>
#include <stack>
#include <unordered_map>
#include <vector>
bool isMemoryRelated(InsID type) {
    return type == InsID::Load || type == InsID::Store || type == InsID::Call;
}

bool impossibleToCSE(InsID type) {
    return (type == InsID::Br || type == InsID::Return || type == InsID::Alloca || type == InsID::Phi || type == InsID::Icmp ||
            type == InsID::Fcmp)||isMemoryRelated(type) ;
}

bool rmInstructionUse(Instruction* I, ValuePtr v) {
    auto user = dynamic_cast<User*>(v);
    if(!user)
        return false;
    auto useIt = user->mUsers.begin();
    bool flag = false;
    while(useIt != user->mUsers.end()) {
        if(useIt.use->user == I) {
            user->mUsers.removeUse(*(useIt.use));
            flag = true;
        }
        ++useIt;
    }
    return flag;
    // auto useH = v->use_head;
    // bool flag = false;
    // while(useH!=nullptr){
    //     if(useH->user == I){
    //         useH->rmUse();
    //         // return true;
    //         flag = true;
    //     }
    //     useH=useH->next;
    // }
    // return flag;
}

bool maybeSameGEP(Instruction* ii, Instruction* ij) {
    auto gep1 = dynamic_cast<GetElementPtrInstruction*>(ii);
    auto gep2 = dynamic_cast<GetElementPtrInstruction*>(ij);
    // @BUG: two 'from' may be two 'GEP->reg' from the same array base
    if(gep1 && gep2) {
        // if(gep1->base == nullptr || gep2->base == nullptr || gep1->allIndex.size() != gep2->allIndex.size())
        auto gep1Base = gep1->getBase();
        auto gep2Base = gep2->getBase();
        auto gep1idx = gep1->getIdx();
        auto gep2idx = gep2->getIdx();
        if(gep1->getIdx().size() != gep2->getIdx().size())
            return false;
        if(gep1->getBase() != gep2->getBase())
            return false;

        bool matched = true, mayMatched = false;
        for(int i = 0; i < gep1->getIdx().size(); i++) {
            if(gep1idx[i] == gep2idx[i])
                continue;

            // same constant
            auto const1 = dynamic_cast<Const*>(gep1idx[i]);
            auto const2 = dynamic_cast<Const*>(gep2idx[i]);
            if(const1 == nullptr || const2 == nullptr) {
                mayMatched = true;
            } else if(const1->intVal != const2->intVal) {
                matched = false;
                break;
            }
        }
        if(!matched)
            return false;
        if(mayMatched)
            return true;
    }
    return false;
}

bool cse(FunctionPtr func) {  // NOLINT
    bool changed = false;
    int instrCount = 0;
    auto cfg = runCFGAnalysis(func);
    auto dom = runDominateTreeAnalysis(func, cfg);
    for(const auto& bbptr : func->getBasicBlocks()) {
        auto bb = bbptr.get();
        unordered_map<InstructionPtr, BasicBlockPtr> del;
        vector<BasicBlockPtr> succBlocks{ bb };
        unordered_map<BrInstruction*, bool> canBrContinue;
        unordered_map<BasicBlockPtr, bool> canBBContinue;
        for(auto succBB : cfg.successors(bb)) {
            succBlocks.emplace_back(succBB);
        }

        for(int i = 0; i < bb->instructionsRef().size(); i++) {
            // 此时是ssa，所以不需要考虑值会不会改变
            auto& iaPtr = bb->instructionsRef()[i];
            auto ia = iaPtr.get();
            bool flag = false;

            // impossible to do CSE
            if(impossibleToCSE(ia->insId) || del.count(ia))
                continue;

            for(const auto& succBB : succBlocks) {
                int j = (succBB == bb ? i + 1 : 0);

                // // if CSE for succBB may incur error, skip it
                if(!dom.dominate(bb, succBB))
                    continue;

                for(; j < succBB->instructionsRef().size(); j++) {
                    auto& ibPtr = succBB->instructionsRef()[j];
                    auto ib = ibPtr.get();
                    // destination of Br
                    // if(ib->insId == InsID::Br) {
                    //     auto brInst = dynamic_cast<BrInstruction*>(ib);
                    //     bool canContinue;
                    //     if(canBrContinue.find(brInst) != canBrContinue.end()) {
                    //         canContinue = canBrContinue[brInst];
                    //     } else {
                    //         // determinate whether succBB is reached by 'bb' only
                    //         auto nextBB = brInst->trueTarget;
                    //         // canBBContinue[nextBB] = (nextBB->predBasicBlocks.size() == 1);
                    //         canContinue = cfg.predecessors(nextBB).size() == 1;
                    //         canBBContinue[nextBB] = canContinue;
                    //         if(brInst->getFalseTarget()) {
                    //             // nextBB = func->labelBbMap[brInst->label2];
                    //             // canBBContinue[nextBB] = (nextBB->predBasicBlocks.size() == 1);
                    //             // canContinue = canContinue || canBBContinue[nextBB];
                    //             nextBB = brInst->falseTarget;
                    //             canBBContinue[nextBB] = cfg.predecessors(nextBB).size() == 1;
                    //             canContinue = canContinue || canBBContinue[nextBB];
                    //         }
                    //         canBrContinue[brInst] = canContinue;
                    //     }
                    //     if(!canContinue) {
                    //         if(succBB == bb)
                    //             flag = true;
                    //         else
                    //             break;
                    //     }
                    // }
                    if(flag)
                        break;

                    if(del.count(ib))
                        continue;

                    if(ia->insId == ib->insId) {
                        if(ia->insId == InsID::Binary) {
                            auto binaryInstA = dynamic_cast<BinaryInstruction*>(ia);
                            auto binaryInstB = dynamic_cast<BinaryInstruction*>(ib);
                            if(binaryInstA->op != binaryInstB->op) {
                                continue;
                            }
                            if(binaryInstA->getLHS() == binaryInstB->getLHS() && binaryInstA->getRHS() == binaryInstB->getRHS()) {
                                binaryInstB->replaceWith(binaryInstA);
                                changed = true;
                            } else if(binaryInstA->getLHS()->isConst && binaryInstB->getLHS()->isConst &&
                                      binaryInstA->getRHS() == binaryInstB->getRHS()) {
                                auto newA1 = dynamic_cast<Const*>(binaryInstA->getLHS());
                                auto newA2 = dynamic_cast<Const*>(binaryInstB->getLHS());
                                if(newA1->type->isInt() && newA2->type->isInt()) {
                                    if(newA1->intVal == newA2->intVal) {
                                        binaryInstB->replaceWith(binaryInstA);
                                        changed = true;
                                    }
                                } else if(newA1->type->isFloat() && newA2->type->isFloat()) {
                                    if(newA1->floatVal == newA2->floatVal) {
                                        binaryInstB->replaceWith(binaryInstA);
                                        changed = true;
                                    }
                                } else if(newA1->type->isBool() && newA2->type->isBool()) {
                                    if(newA1->boolVal == newA2->boolVal) {
                                        binaryInstB->replaceWith(binaryInstA);
                                        changed = true;
                                    }
                                }
                            } else if(binaryInstA->getRHS()->isConst && binaryInstB->getRHS()->isConst &&
                                      binaryInstA->getLHS() == binaryInstB->getLHS()) {
                                auto newB1 = dynamic_cast<Const*>(binaryInstA->getRHS());
                                auto newB2 = dynamic_cast<Const*>(binaryInstB->getRHS());
                                if(newB1->type->isInt() && newB2->type->isInt()) {
                                    if(newB1->intVal == newB2->intVal) {
                                        binaryInstB->replaceWith(binaryInstA);
                                        changed = true;
                                    }
                                } else if(newB1->type->isFloat() && newB2->type->isFloat()) {
                                    if(newB1->floatVal == newB2->floatVal) {
                                        binaryInstB->replaceWith(binaryInstA);
                                        changed = true;
                                    }
                                } else if(newB1->type->isBool() && newB2->type->isBool()) {
                                    if(newB1->boolVal == newB2->boolVal) {
                                        binaryInstB->replaceWith(binaryInstA);
                                        changed = true;
                                    }
                                }
                            }
                            // 两个都是常量
                            else {
                            }
                        }
                        // pointer alias
                        else if(ia->insId == InsID::GEP) {
                            auto gepInstA = dynamic_cast<GetElementPtrInstruction*>(ia);
                            auto gepInstB = dynamic_cast<GetElementPtrInstruction*>(ib);
                            if(gepInstA->getBase() == gepInstB->getBase() &&
                               gepInstA->getNumOperands() == gepInstB->getNumOperands()) {
                                bool matched = true;
                                auto idxA = gepInstA->getIdx();
                                auto idxB = gepInstB->getIdx();
                                for(int i = 0; i < gepInstA->getIdx().size(); i++) {
                                    // same SSA variable
                                    if(idxA[i] == idxB[i])
                                        continue;
                                    // same constant
                                    auto const0 = dynamic_cast<Const*>(idxA[i]);
                                    auto const1 = dynamic_cast<Const*>(idxB[i]);
                                    if(const0 == nullptr || const1 == nullptr || const0->intVal != const1->intVal) {
                                        matched = false;
                                        break;
                                    }
                                }
                                if(matched) {
                                    // FIXME
                                    gepInstB->replaceWith(gepInstA);
                                    changed = true;
                                }
                            }
                            
                        } else if(ia->insId == InsID::Ext) {
                            auto extInstA = dynamic_cast<ExtInstruction*>(ia);
                            auto extInstB = dynamic_cast<ExtInstruction*>(ib);
                            if(extInstA->getFrom() == extInstB->getFrom() &&
                               extInstA->getToType()->id == extInstB->getToType()->id) {
                                extInstB->replaceWith(extInstA);
                                changed = true;
                            }
                        }
                    } 
                    if(flag)
                        break;
                }
            }
        }
        // for(const auto& newBB : changedBB) {
        //     vector<InstructionPtr> newIns;
        //     for(const auto& instruction : newBB->instructions) {
        //         if(del.count(instruction) == 0) {
        //             newIns.push_back(instruction);
        //         }
        //     }
        //     newBB->instructions = newIns;
        // }
        instrCount += del.size();  // NOLINT
    

        // if(del.size()) {
        //     cout << bb->label->name << " eliminated:" << endl;
        //     for(auto instrPair: del) {
        //         instrPair.first->print();
        //     }
        // }
    }
    cout << "[frontend opt]: CSE eliminated " << instrCount << " instructions" << endl;
    return changed;
}

void runCSE(Module& mod) {
    for(auto& func : mod.getGlobalFunctions()) {
        if(func->isLib)
            continue;
        cse(func);
    }
}