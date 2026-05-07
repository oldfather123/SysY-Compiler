#include "EarlyExitLoopElimination.h"

static int earlyExitRegCount = 0;

bool checkIndexPhi(LoopPtr loop){
    for(auto inst: loop->header->instructions){
        if(inst->type == Icmp){
            auto icmpInst = dynamic_cast<IcmpInstruction*>(inst.get());
            auto phiInst = icmpInst->a->I;
            if(phiInst){
                if(phiInst->type != Phi) return false;
                auto phiNode = dynamic_cast<PhiInstruction*>(phiInst);
                for(auto it:phiNode->from){
                    if(it.first->isConst) return false;
                }
                return true;
            }
        }
    }
    return false;
}

bool checkSubLoop(LoopPtr loop){
    if(loop->exitBlocks.size() != 1 || loop->exitingBlocks.size() != 1 || loop->header != *loop->exitingBlocks.begin()){
        return false;
    }
    if(!checkIndexPhi(loop)) return false;
    if(loop->header->instructions.size()>=15 ||loop->latchBlock->instructions.size()>=15) return false;
    
    auto exitBlock = *loop->exitBlocks.begin();
    vector<ValuePtr> latchModifiedGEPBase;

    for(auto inst:loop->latchBlock->instructions){
        if(inst->type == GEP){
            auto gepInst = dynamic_cast<GetElementPtrInstruction*>(inst.get());
            latchModifiedGEPBase.push_back(gepInst->base);
        }
    }
    vector<ValuePtr> exitModifiedGEPBase;
    for(auto inst :exitBlock->instructions){
        if(inst->type == GEP){
            auto gepInst = dynamic_cast<GetElementPtrInstruction*>(inst.get());
            exitModifiedGEPBase.push_back(gepInst->base);
        }
    }
    if(latchModifiedGEPBase.size()!=exitModifiedGEPBase.size() || exitModifiedGEPBase.size()==0)return false;
    for(int i=0;i<latchModifiedGEPBase.size();i++){
        if(latchModifiedGEPBase[i]!=exitModifiedGEPBase[i])return false;
    }
    return true;
}

// bool processOuterLoop(LoopPtr loop){
//     auto header = loop->header;
//     ValuePtr loopIndexEnd = nullptr;
//     for(auto inst:header->instructions){
//         if(inst->type == Icmp){
//             auto icmpInst = dynamic_cast<IcmpInstruction*>(inst.get());
//             if(!icmpInst->b->isConst) return false;
//             loopIndexEnd = icmpInst->b;
//             icmpInst->b = Const::createConstant(Type::getInt(), int(15625));
//         }
//     }
//     if(!loopIndexEnd) return false;
//     auto exitBlock = *loop->subLoops[0]->exitBlocks.begin();
//     int n=exitBlock->instructions.size();
//     for(int i=0;i<n;i++){
//         auto inst = exitBlock->instructions[i];
//         if(inst->type == Call){
//             auto binaryInst = shared_ptr<BinaryInstruction>(new BinaryInstruction(inst->reg, Const::createConstant(Type::getInt(), int(32)), '*', exitBlock));
//             exitBlock->insertInstructionBefore(binaryInst, exitBlock->instructions[i+1]);
//             replaceVarByVar(inst->reg, binaryInst->reg);
//             binaryInst->a = inst->reg;
//             return true;
//         }
//     }
//     return false;
// }

void deconstructLoop(LoopPtr loop){
    auto header = loop->header;
    int n=header->instructions.size();
    for(int i=0;i<n;i++){
        auto inst = header->instructions[i];
        if(inst->type == Phi){
            auto phiNode = dynamic_cast<PhiInstruction*>(inst.get());
            for(auto it:phiNode->from){
                if(it.first->isConst){
                    replaceVarByVar(phiNode->reg, it.first);
                    break;
                }
            }
        }
        else if(inst->type == Br){
            auto brInst = dynamic_cast<BrInstruction*>(inst.get());
            brInst->exp = nullptr;
            brInst->label2 = nullptr;
            header->instructions = {inst};
            return;
        }
    }
    
}

void earlyDetach(LoopPtr loop){
    for(auto bb:loop->basicBlocks){
        for(auto inst : bb->instructions){
            if(inst->type == Binary){
                auto binaryInst = dynamic_cast<BinaryInstruction*>(inst.get());
                auto lhs = dynamic_cast<Const *>(binaryInst->a.get());
                auto rhs = dynamic_cast<Const *>(binaryInst->b.get());
                if(lhs && rhs){
                    int myres = 0.0;
                    int lhs_value, rhs_value;
                    lhs_value = lhs->type->isBool() ? lhs->boolVal : lhs->intVal;
                    rhs_value = rhs->type->isBool() ? rhs->boolVal : rhs->intVal;
                    switch (binaryInst->op)
                    {
                    case '+':
                        myres = lhs_value + rhs_value;
                        break;
                    case '-':
                        myres = lhs_value - rhs_value;
                        break;
                    case '*':
                        myres = lhs_value * rhs_value;
                        break;
                    case '/':
                        myres = lhs_value / rhs_value;
                        break;
                    case '%':
                        myres = lhs_value % rhs_value;
                        break;
                    case '!':
                        myres = !lhs_value;
                        break;
                    case ',':
                        myres = lhs_value << rhs_value;
                        break;
                    case '.':
                        myres = lhs_value >> rhs_value;
                        break;
                    }

                    auto res = Const::createConstant(Type::getInt(),(int)myres, "earlyExitconstPropogation%" + to_string(earlyExitRegCount++));
                    replaceVarByVar(binaryInst->reg, res);
                    deleteUser(binaryInst->reg);
                }
            }
            else if(inst->type == Icmp){
                auto Ii = dynamic_cast<IcmpInstruction*>(inst.get());
                if(Ii->a->isConst && Ii->b->isConst){
                    bool res = false;
                    assert(!Ii->a->type->isFloat() && !Ii->b->type->isFloat()&&"Icmp with float value");
                    int aVal = Ii->a->type->isInt()?dynamic_cast<Const*>(Ii->a.get())->intVal:dynamic_cast<Const*>(Ii->a.get())->boolVal;
                    int bVal = Ii->b->type->isInt()?dynamic_cast<Const*>(Ii->b.get())->intVal:dynamic_cast<Const*>(Ii->b.get())->boolVal;
                    string op = Ii->op;

                    if (op == "!=")
                    {
                        res = aVal!=bVal;
                    }
                    else if (op == ">")
                    {
                        res = aVal>bVal;
                    }
                    else if (op == ">=")
                    {
                        res = aVal>=bVal;
                    }
                    else if (op == "<")
                    {
                        res = aVal<bVal;
                    }
                    else if (op == "<=")
                    {
                        res = aVal<=bVal;
                    }
                    else if (op == "==")
                    {
                        res = aVal==bVal;
                    }
                    else{
                        assert(false&&"wrong op");
                    }
                    auto newConst = Const::createConstant(Type::getBool(),res);

                    replaceVarByVar(Ii->reg, newConst);
                    deleteUser(Ii->reg);
                }
            }
            else if(inst->type == Br){
                auto brInst = dynamic_cast<BrInstruction*>(inst.get());
                
                auto brExp = brInst->exp;
                if(!brExp) break;;
                if(brExp->isConst){
                    bool brExpVal = dynamic_cast<Const*>(brExp.get())->boolVal;
                    brInst->exp = nullptr;
                    
                    if(!brExpVal){
                        brInst->label1 = brInst->label2;
                    }
                    brInst->label2 = nullptr;

                    auto thenBlock = bb->parentFunction->LabelToBBMap[brInst->label1];
                    auto thenBr = dynamic_cast<BrInstruction*>(thenBlock->instructions.back().get());
                    auto sameBlock = bb->parentFunction->LabelToBBMap[thenBr->label1];

                    for(auto inst2:sameBlock->instructions){
                        if(inst2->type == Phi){
                            auto phiNode = dynamic_cast<PhiInstruction*>(inst2.get());
                            for(auto itt:phiNode->from){
                                if(itt.second == thenBlock){
                                    // replaceVarByVar(phiNode->reg, itt.first);
                                    auto accUseHead = phiNode->reg->useHead;
                                    while(accUseHead){
                                        auto accUse = accUseHead->user;
                                        accUse->replaceOperand(phiNode->reg, itt.first);
                                        accUseHead = accUseHead->next;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    sameBlock->instructions = {sameBlock->instructions.back()};
                    
                }
            }
        }
    }
}

bool processOuterLoop(LoopPtr loop){
    auto header = loop->header;
    ValuePtr loopIndexEnd = nullptr;
    IcmpInstruction* loopIcmp = nullptr;
    BrInstruction* headerBr = nullptr;
    ValuePtr loopIndexStart = nullptr;

    auto exitBlock = *loop->subLoops[0]->exitBlocks.begin();
    int n=exitBlock->instructions.size();

    for(int i=0;i<n;i++){
        auto inst = exitBlock->instructions[i];
        if(inst->type == Call){
            auto callInst = dynamic_cast<CallInstruction*>(inst.get());
            if(!callInst->func->returnValue->type->isFloat()) return false;
        }
    }


    for(auto inst:header->instructions){
        if(inst->type == Icmp){
            auto icmpInst = dynamic_cast<IcmpInstruction*>(inst.get());
            loopIcmp = icmpInst;
            if(!icmpInst->b->isConst) return false;
            loopIndexEnd = icmpInst->b;
            auto indexPhi = dynamic_cast<PhiInstruction*>(icmpInst->a->I);
            
            for(auto it:indexPhi->from){
                if(it.first->isConst){
                    loopIndexStart = it.first;
                }
            }
            loopIndexEnd = icmpInst->b;
            icmpInst->b = Const::createConstant(Type::getInt(), int(1));
            if(!icmpInst->b->isConst) return false;
        }
        if(inst->type == Br) headerBr = dynamic_cast<BrInstruction*>(inst.get());
    }
    if(!loopIndexEnd) return false;
    int loopCnt = dynamic_cast<Const*>(loopIndexEnd.get())->intVal;
    
    
    BrInstruction* exitBr = dynamic_cast<BrInstruction*>(exitBlock->instructions.back().get());
    for(int i=0;i<n;i++){
        auto inst = exitBlock->instructions[i];
        if(inst->type == Call){
            
            InstructionPtr lastInst = nullptr;
            // header->insertInstructionBefore(inst, header->instructions[header->instructions.size()-1]);
            // exitBlock->removeInstruction(inst);
            
            auto earlyExit = make_shared<BasicBlock>(std::make_shared<Label>("earlyExit"));
            auto phiNodeIndex = make_shared<PhiInstruction>(earlyExit, Type::getInt());
            earlyExit->appendInstruction(phiNodeIndex);
            auto phiNodeAcc = make_shared<PhiInstruction>(earlyExit, inst->reg->type);
            earlyExit->appendInstruction(phiNodeAcc);

            auto valAddInst = shared_ptr<BinaryInstruction>(new BinaryInstruction(phiNodeAcc->reg, inst->reg, '+', exitBlock));

            replaceVarByVar(inst->reg, phiNodeAcc->reg);
            auto nextBinary = dynamic_cast<BinaryInstruction*>(exitBlock->instructions[i+1].get());
            if(nextBinary->b == phiNodeAcc->reg) nextBinary->b = inst->reg;
            auto useHead = nextBinary->reg->useHead;
            while (useHead) {
                auto use = useHead->user;
                if(use->type == Phi){
                    auto accUseHead = use->reg->useHead;
                    while(accUseHead){
                        auto accUse = accUseHead->user;
                        if(loop->contains(accUse->basicblock)) {
                            accUseHead = accUseHead->next;
                            continue;
                        }
                        accUse->replaceOperand(use->reg, phiNodeAcc->reg);
                        accUseHead = accUseHead->next;
                    }
                    break;
                }
                useHead = useHead->next;
            }

            valAddInst->a = inst->reg;
            earlyExit->appendInstruction(valAddInst);
            auto indexAddInst = shared_ptr<BinaryInstruction>(new BinaryInstruction(phiNodeIndex->reg, Const::createConstant(Type::getInt(), int(1)), '+', exitBlock));
            earlyExit->appendInstruction(indexAddInst);
            auto icmpInst = shared_ptr<IcmpInstruction>(new IcmpInstruction(earlyExit, indexAddInst->reg, Const::createConstant(Type::getInt(), int(loopCnt+1)), loopIcmp->op));
            earlyExit->appendInstruction(icmpInst);
            auto brInst = shared_ptr<BrInstruction>(new BrInstruction(icmpInst->reg, earlyExit->label, headerBr->label2, earlyExit));
            earlyExit->appendInstruction(brInst);
            earlyExit->setTerminator(brInst);
            headerBr->label2 = earlyExit->label;

            phiNodeIndex->addFrom(loopIndexStart, exitBlock);
            phiNodeIndex->addFrom(indexAddInst->reg, earlyExit);
            
            phiNodeAcc->addFrom(Const::createConstant(Type::getFloat(), float(0)), exitBlock);
            phiNodeAcc->addFrom(valAddInst->reg, earlyExit);

            header->parentFunction->addBasicBlock(earlyExit);

            exitBr->label1 = earlyExit->label;

            deconstructLoop(loop);
            earlyDetach(loop);
            return true;
        }
    }
    return false;
}

bool EarlyExitLoopElimination(FunctionPtr func){
    bool returnFlag = false;
    for(auto loop:func->loopList){
        if(loop->subLoops.size() == 1 && loop->loopDepth==1){
            if(checkSubLoop(loop->subLoops[0])){
                returnFlag |= processOuterLoop(loop);
            }
        }
    }
    return returnFlag;
}