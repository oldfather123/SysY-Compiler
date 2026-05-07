//#include "Instruction.h"
//#include "SCEV.h"
//#include <cassert>

//void BIV2SCEV(LoopPtr loop, PhiInstruction* phi, std::set<ValuePtr>& adds, std::set<ValuePtr>& subs) {
//    ValuePtr initial = nullptr;
//    for(auto itr : phi->incomings()) {
//        auto val = itr.second->val;
//        if(loop->isSimpleLoopInvariant(val))
//            initial = val;
//    }
//
//    if(!initial)
//        return;
//
//    if(!adds.empty()) {
//        auto step = *adds.begin();
//        adds.erase(step);
//
//        std::set<BinaryInstruction*> binaryStore;
//        for(auto sub : subs) {
//            auto binary = new BinaryInstruction(BinaryInstructionOps::Sub, step, sub);
//            binaryStore.insert(binary);
//            step = binary;
//        }
//
//        for(auto add : adds) {
//            auto binary = new BinaryInstruction(BinaryInstructionOps::Add, step, add);
//            binaryStore.insert(binary);
//            step = binary;
//        }
//
//        SCEV scev(initial, step, std::move(binaryStore));
//        loop->registerSCEV((Instruction*)phi, scev);
//        return;
//    }
//
//    if(subs.empty()) {
//        return;
//    }
//
//    auto sub = *subs.begin();
//    subs.erase(sub);
//    std::set<BinaryInstruction*> binaryStore;
//    int myval = 0;
//    auto step = new BinaryInstruction(ValuePtr(new Const(myval, "scevsubtmp")), sub, '-', BasicBlockPtr(nullptr));
//
//    binaryStore.insert(step);
//
//    for(auto sub : subs) {
//        step = new BinaryInstruction(ValuePtr(new Const(myval, "scevsubtmp")), sub, '-', BasicBlockPtr(nullptr));
//        binaryStore.insert(step);
//    }
//
//    SCEV scev(initial, step->reg, std::move(binaryStore));
//    loop->registerSCEV(phi, scev);
//}
//
//void registerBIV(LoopPtr loop, PhiInstruction* phi) {
//
//    stack<BinaryInstruction*> workStk;
//    for(auto incoming : phi->incomings()) {
//        // FIXME: check isConst
//        if(incoming.second->val->isConst)
//            continue;
//        auto ptr = incoming.second;
//        // assert(use.first->I !=nullptr);
//        if(!incoming.second->val->is<Instruction>())
//            continue;
//        auto inst = incoming.second->val->as<BinaryInstruction>();
//        if(inst->getBasicBlock()) {
//            workStk.push(inst);
//        }
//    }
//
//    std::set<ValuePtr> addInSCEV;
//    std::set<ValuePtr> subInSCEV;
//
//    while(!workStk.empty()) {
//        auto binary = workStk.top();
//        workStk.pop();
//
//        // not in loop
//        if(binary->getOp() == "+") {
//            if(loop->isSimpleLoopInvariant(binary->getLHS()))
//                addInSCEV.insert(binary->getLHS());
//            else if(loop->isSimpleLoopInvariant(binary->getRHS()))
//                addInSCEV.insert(binary->getRHS());
//            else
//                return;
//        } else if(binary->getOp() == "-") {
//            assert(false);
//            // if(loop->isSimpleLoopInvariant(binary->getRHS()))
//            //     subInSCEV.insert(binary->getRHS());
//            // else
//            //     return;
//        }
//
//        // useHead is self
//        auto ptr = binary->users().useHead.next;
//        while(ptr != nullptr) {
//            auto user = ptr->user;
//            if(user->insId == InsID::Phi) {
//                auto instr = dynamic_cast<PhiInstruction*>(user);
//                // to be fixed
//                if(user == phi) {
//                    BIV2SCEV(loop, phi, addInSCEV, subInSCEV);
//                    return;
//                }
//            } else if(user->insId == InsID::Binary) {
//                auto instr = dynamic_cast<BinaryInstruction*>(user);
//                if(instr->getOp() == "+" || instr->getOp() == "-") {
//                    workStk.push(instr);
//                }
//            }
//            ptr = ptr->next;
//        }
//    }
//}
//
//void findAndRegisterBIV(LoopPtr loop) {
//    auto header = loop->getHeader();
//    std::vector<PhiInstruction*> phiList;
//    // get phi inst
//    for(auto& instr : header->instructionsRef()) {
//        if(instr->insId == InsID::Phi) {
//            auto phi = dynamic_cast<PhiInstruction*>(instr.get());
//            phiList.push_back(phi);
//        } else
//            break;
//    }
//
//    for(auto phi : phiList) {
//        std::stack<Instruction*> workStk;
//        for(auto& incoming : phi->incomings()) {
//            auto ptr = incoming.second;
//            while(ptr != nullptr) {
//                auto user = ptr->user;
//                assert(user != nullptr);
//                workStk.push(user);
//                ptr = ptr->next;
//            }
//        }
//
//        while(!workStk.empty()) {
//            auto instr = workStk.top();
//            workStk.pop();
//            if(instr == phi) {
//                // cerr << "Registering" << phi->reg->getStr() << endl;
//                registerBIV(loop, phi);
//                break;
//            }
//
//            if(auto phi = dynamic_cast<PhiInstruction*>(instr))
//                break;
//
//            // unfinished
//            // if(loop->isSimpleLoopInvariant(instr))
//            //     continue;
//
//            auto val = instr->reg;
//            if(val == nullptr)
//                continue;
//            auto ptr = val->useHead;
//            while(ptr != nullptr) {
//                auto user = ptr->user;
//                assert(user != nullptr);
//                workStk.push(user);
//                ptr = ptr->next;
//            }
//        }
//    }
//}
//
//void linkInstructionToSCEV(LoopPtr loop, BinaryInstruction* binary, bool& fixed) {
//
//    // cerr << "Linking to SCEV" << endl;
//    auto lhs = binary->a;
//    auto rhs = binary->b;
//
//    auto lhsInstr = lhs->I;
//    auto rhsInstr = rhs->I;
//
//    if(loop->hasSCEV(lhsInstr)) {
//        if(loop->isSimpleLoopInvariant(rhs)) {
//            if(binary->op == '+') {
//                auto scev = rhs + loop->getSCEV(lhsInstr);
//                loop->registerSCEV(binary, scev);
//                fixed = false;
//                return;
//            } else if(binary->op == '*') {
//                auto scev = rhs * loop->getSCEV(lhsInstr);
//                loop->registerSCEV(binary, scev);
//                fixed = false;
//                return;
//            } else if(binary->op == '-') {
//                auto scev = loop->getSCEV(lhsInstr) - rhs;
//                loop->registerSCEV(binary, scev);
//                fixed = false;
//                return;
//            }
//        } else if(loop->hasSCEV(rhsInstr)) {
//            if(binary->op == '+') {
//                auto scev = loop->getSCEV(lhsInstr) + loop->getSCEV(rhsInstr);
//                loop->registerSCEV(binary, scev);
//                fixed = false;
//                return;
//            }  // TODO solve the Higher SCEV like {0, +, 1} * {0, +, 1} = {0, +, 1, +, 2}
//            else if(binary->op == '-') {
//                auto scev = loop->getSCEV(lhsInstr) - loop->getSCEV(rhsInstr);
//                loop->registerSCEV(binary, scev);
//                fixed = false;
//                return;
//            }
//        }
//    } else if(loop->hasSCEV(rhsInstr)) {
//        if(loop->isSimpleLoopInvariant(lhs)) {
//            if(binary->op == '+') {
//                auto scev = lhs + loop->getSCEV(rhsInstr);
//                loop->registerSCEV(binary, scev);
//                fixed = false;
//                return;
//            } else if(binary->op == '*') {
//                auto scev = lhs * loop->getSCEV(rhsInstr);
//                loop->registerSCEV(binary, scev);
//                fixed = false;
//                return;
//            }
//        }
//    }
//}
//
//void scalarEvolution(LoopPtr loop) {
//    if(loop == nullptr)
//        return;
//
//    for(auto subLoop : loop->getSubLoops())
//        ScalarEvolution(subLoop);
//
//    loop->cleanSCEV();
//    // Step1. Find the BIV.
//    findAndRegisterBIV(loop);
//    bool fixed = false;
//
//    do {
//        fixed = true;
//
//        for(auto bb : loop->getBasicBlocks()) {
//            for(auto instr : bb->instructions) {
//                if(loop->hasSCEV(instr.get()))
//                    continue;
//                if(instr->type == Binary) {
//                    // cerr << instr->getName()<< endl;
//                    auto binary = dynamic_cast<BinaryInstruction*>(instr.get());
//                    if(binary->op == '+' || binary->op == '-' || binary->op == '*') {
//                        linkInstructionToSCEV(loop, binary, fixed);
//                    }
//                }
//            }
//        }
//
//    } while(!fixed);
//
//    if(loop->indPhi && loop->hasSCEV(loop->indPhi)) {
//        auto phi = loop->indPhi;
//        auto& scev = loop->getSCEV(phi);
//        auto endval = loop->indEnd;
//        auto initval = scev.at(0);
//        auto stepval = scev.at(1);
//        if(endval && endval->isConst && initval && initval->isConst && stepval && stepval->isConst) {
//            auto cEnd = dynamic_cast<Const*>(endval.get());
//            auto cInitial = dynamic_cast<Const*>(initval.get());
//            auto cStep = dynamic_cast<Const*>(stepval.get());
//            int c;
//            if(loop->icmpKind == ICmpEQ || loop->icmpKind == ICmpSGE || loop->icmpKind == ICmpSLE)
//                c = (cEnd->intVal + 1 - cInitial->intVal) / cStep->intVal;
//            else
//                c = (cEnd->intVal - cInitial->intVal) / cStep->intVal;
//            loop->tripCount = c;
//        }
//    }
//}
//