#include "Instruction.h"
#include "Loop.h"
// #include <utility>
//// NOLINTBEGIN
//
// Loop::Loop(shared_ptr<BasicBlock> header, int id)
//    : header(header), id(id), parent(header->belongfunc), parentLoop(nullptr), indCondVar(nullptr), indEnd(nullptr),
//      indPhi(nullptr), tripCount(0) {
//    // bbSet.insert(header);
//    addBasicBlock(header);
//}
//
// bool Loop::isADominatorB(BasicBlockPtr A, BasicBlockPtr B, BasicBlockPtr entry) {
//    if(A == entry) {
//        return true;
//    }
//    while(B != entry) {
//        if(B == A) {
//            return true;
//        }
//        B = B->directDominator;
//    }
//    return false;
//}
//

void Loop::dumpCfg() {
    // header
    cout << "Loop Header: " << header->getName() << endl;
    // latch
    cout << "Loop Latch: " << latch->getName() << endl;
    // tripCount
    cout << "Trip Count: " << getTripCount() << endl;
}


bool Loop::isSimpleLoopInvariant(Value* val) {
    if(auto inst = dynamic_cast<Instruction*>(val)) {
        return !this->contains(inst->getBasicBlock());
    }
    return true;
}
//
//// BasicBlockPtr Loop::getPreheader() {
////     if(preHeader != nullptr) return preHeader;
////     for(auto predecessor: header->predBasicBlocks) {
////         if(!contains(predecessor)) {
////             // assert preheader has only one successor
////             assert(predecessor->succBasicBlocks.size() == 1);
////             preHeader = predecessor;
////         }
////     }
////     // assert preheader always exists
////     assert(preHeader != nullptr);
////     return preHeader;
//// }
//
//// vector<BasicBlockPtr> Loop::getExitBlocks() {
////     vector<BasicBlockPtr> exitBlocks;
////     for(auto bb: bbSet) {
////         for(auto succPair: bb->succBasicBlocks) {
////             auto succ = succPair;
////             if(!contains(succ)) {
////                 exitBlocks.emplace_back(bb);
////                 break;
////             }
////         }
////     }
////     return exitBlocks;
//// }
//
//// void Loop::addInnerLoop(shared_ptr<Loop> innerLoop) {
////     innerLoops.insert(innerLoop);
//// }
//
//// bool Loop::hasInnerLoop() {
////     return innerLoops.size() > 0;
//// }
//
// SCEV::SCEV(ValuePtr initial, ValuePtr step) {
//    scevVal.push_back(initial);
//    scevVal.push_back(step);
//}
//
// SCEV::SCEV(ValuePtr initial, const SCEV& innerSCEV) {
//    scevVal.push_back(initial);
//
//    for(auto iter = innerSCEV.scevVal.begin(); iter != innerSCEV.scevVal.end(); iter++)
//
//        for(const auto& val : innerSCEV.scevVal)
//            scevVal.push_back(val);
//
//    std::set_union(instructionsHasBeenCaculated.begin(), instructionsHasBeenCaculated.end(),
//                   innerSCEV.instructionsHasBeenCaculated.begin(), innerSCEV.instructionsHasBeenCaculated.end(),
//                   std::inserter(instructionsHasBeenCaculated, instructionsHasBeenCaculated.begin()));
//}
//
// SCEV::SCEV(const vector<ValuePtr>& vec) {
//    for(const auto& val : vec)
//        scevVal.push_back(val);
//}
//
// SCEV::SCEV(ValuePtr initial, ValuePtr step, std::set<BinaryInstruction*>&& binarys) : instructionsHasBeenCaculated(binarys) {
//    scevVal.push_back(initial);
//    scevVal.push_back(step);
//}
//
// SCEV operator+(ValuePtr lhs, const SCEV& rhs) {
//    // auto add = new BinaryInstruction* (lhs->getType(), Kind::Add, lhs, rhs.at(0), nullptr, "scevtmpadd");
//    // cerr << "Start to Add" << endl;
//    auto tmp = rhs.at(0);
//    // cerr << "[passed] at " << endl;
//    // cerr << "lhs is " << lhs->getStr() << endl;
//    // cerr << "rhs is " << rhs.at(0)->getStr() << endl;
//    auto add = new BinaryInstruction(lhs, rhs.at(0), '+', BasicBlockPtr(nullptr));
//    // cerr << "[passed] create instr" << endl;
//    std::vector<ValuePtr> initialVals;
//    initialVals.push_back(add->reg);
//    for(int i = 1; i < rhs.size(); i++)
//        initialVals.push_back(rhs.at(i));
//    // cerr << "[passed] first loop" << endl;
//    auto res = SCEV(initialVals);
//    res.instructionsHasBeenCaculated = rhs.instructionsHasBeenCaculated;
//    res.instructionsHasBeenCaculated.insert(add);
//    // cerr << "[passed] union" << endl;
//    return res;
//}
//
// SCEV operator+(const SCEV& lhs, const SCEV& rhs) {
//    // cerr << "Start to Add" << endl;
//    auto maxSize = std::max(lhs.size(), rhs.size());
//    vector<BinaryInstruction*> binaryHasBeenCreated;
//    vector<ValuePtr> initialVals;
//    for(auto i = 0; i < maxSize; i++) {
//        if(i < lhs.size() && i < rhs.size()) {
//            auto lhsval = lhs.at(i);
//            auto rhsval = rhs.at(i);
//            if(lhsval->isConst) {
//                auto clhs = dynamic_cast<Const*>(lhs.at(i).get());
//                if(rhsval->isConst) {
//                    auto crhs = dynamic_cast<Const*>(rhs.at(i).get());
//                    auto nval = ValuePtr(new Const(clhs->intVal + crhs->intVal));
//                    initialVals.push_back(nval);
//                    continue;
//                }
//            }
//            auto add = new BinaryInstruction(lhs.at(i), rhs.at(i), '+', BasicBlockPtr(nullptr));
//            binaryHasBeenCreated.push_back(add);
//            initialVals.push_back(add->reg);
//        } else if(i < lhs.size())
//            initialVals.push_back(lhs.at(i));
//        else if(i < rhs.size())
//            initialVals.push_back(rhs.at(i));
//    }
//
//    // cerr << "[passed] first loop" << endl;
//
//    auto res = SCEV(initialVals);
//    std::set_union(lhs.instructionsHasBeenCaculated.begin(), lhs.instructionsHasBeenCaculated.end(),
//                   rhs.instructionsHasBeenCaculated.begin(), rhs.instructionsHasBeenCaculated.end(),
//                   std::inserter(res.instructionsHasBeenCaculated, res.instructionsHasBeenCaculated.begin()));
//    for(auto binary : binaryHasBeenCreated)
//        res.instructionsHasBeenCaculated.insert(binary);
//
//    // cerr << "[passed] union" << endl;
//
//    return res;
//}
//
// SCEV operator-(const SCEV& lhs, ValuePtr rhs) {
//    auto sub = new BinaryInstruction(lhs.at(0), rhs, '-', BasicBlockPtr(nullptr));
//    std::vector<ValuePtr> initialVals;
//    initialVals.push_back(sub->reg);
//    for(int i = 1; i < lhs.size(); i++)
//        initialVals.push_back(lhs.at(i));
//    auto res = SCEV(initialVals);
//    res.instructionsHasBeenCaculated = lhs.instructionsHasBeenCaculated;
//    res.instructionsHasBeenCaculated.insert(sub);
//    return res;
//}
//
// SCEV operator-(const SCEV& lhs, const SCEV& rhs) {
//    auto maxSize = std::max(lhs.size(), rhs.size());
//    std::vector<BinaryInstruction*> binaryHasBeenCreated;
//    std::vector<ValuePtr> initialVals;
//    for(auto i = 0; i < maxSize; i++) {
//        if(i < lhs.size() && i < rhs.size()) {
//            auto lhsval = lhs.at(i);
//            auto rhsval = rhs.at(i);
//            if(lhsval->isConst) {
//                auto clhs = dynamic_cast<Const*>(lhs.at(i).get());
//                if(rhsval->isConst) {
//                    auto crhs = dynamic_cast<Const*>(rhs.at(i).get());
//                    auto nval = ValuePtr(new Const(clhs->intVal - crhs->intVal));
//                    initialVals.push_back(nval);
//                    continue;
//                }
//            }
//            auto add = new BinaryInstruction(lhs.at(i), rhs.at(i), '-', BasicBlockPtr(nullptr));
//            binaryHasBeenCreated.push_back(add);
//            initialVals.push_back(add->reg);
//        } else if(i < lhs.size())
//            initialVals.push_back(lhs.at(i));
//        else if(i < rhs.size())
//            initialVals.push_back(rhs.at(i));
//    }
//
//    auto res = SCEV(initialVals);
//    std::set_union(lhs.instructionsHasBeenCaculated.begin(), lhs.instructionsHasBeenCaculated.end(),
//                   rhs.instructionsHasBeenCaculated.begin(), rhs.instructionsHasBeenCaculated.end(),
//                   std::inserter(res.instructionsHasBeenCaculated, res.instructionsHasBeenCaculated.begin()));
//    for(auto binary : binaryHasBeenCreated)
//        res.instructionsHasBeenCaculated.insert(binary);
//
//    return res;
//}
//
// SCEV operator*(ValuePtr lhs, const SCEV& rhs) {
//    std::vector<BinaryInstruction*> binaryHasBeenCreated;
//    std::vector<ValuePtr> initialVals;
//    for(const auto& initem : rhs.scevVal) {
//        const auto& lhsval = lhs;
//        const auto& rhsval = initem;
//        if(lhsval->isConst) {
//            auto clhs = dynamic_cast<Const*>(lhs.get());
//            if(rhsval->isConst) {
//                auto crhs = dynamic_cast<Const*>(initem.get());
//                auto nval = ValuePtr(new Const(clhs->intVal * crhs->intVal));
//                initialVals.push_back(nval);
//                continue;
//            }
//        }
//        auto mul = new BinaryInstruction(lhs, initem, '*', BasicBlockPtr(nullptr));
//        binaryHasBeenCreated.push_back(mul);
//        initialVals.push_back(mul->reg);
//    }
//
//    auto res = SCEV(initialVals);
//    res.instructionsHasBeenCaculated = rhs.instructionsHasBeenCaculated;
//
//    for(auto binary : binaryHasBeenCreated)
//        res.instructionsHasBeenCaculated.insert(binary);
//
//    return res;
//}
//// TODO DO the SCEV at the higher dimension
//// SCEV operator*(const SCEV& lhs, const SCEV& rhs)
//// {
//
//// }
//
// void SCEV::getItemChains(unsigned i, std::vector<BinaryInstruction*>& instrChain) {
//    auto value = scevVal.at(i);
//    auto instr = value->I;
//    if(instr && instr->type == Binary) {
//        auto bi = dynamic_cast<BinaryInstruction*>(instr);
//        if(instructionsHasBeenCaculated.count(bi)) {
//            std::function<void(BinaryInstruction*)> postTravel = [&](BinaryInstruction* binary) -> void {
//                // PrintMsg(binary->getSignature());
//                auto lhs = binary->a->I;
//                if(lhs && lhs->type == Binary && instructionsHasBeenCaculated.count(dynamic_cast<BinaryInstruction*>(lhs)))
//                    postTravel(dynamic_cast<BinaryInstruction*>(lhs));
//
//                auto rhs = binary->b->I;
//                if(rhs && rhs->type == Binary && instructionsHasBeenCaculated.count(dynamic_cast<BinaryInstruction*>(rhs)))
//                    postTravel(dynamic_cast<BinaryInstruction*>(rhs));
//
//                instrChain.push_back(binary);
//            };
//
//            postTravel(bi);
//        }
//    }
//}
//
// void unuseall(Instruction* ins) {
//    auto operandsNum = ins->getNumOperands();
//    for(int i = 0; i < operandsNum; i++) {
//        auto operand = ins->getOperand(i);
//        if(operand) {
//            deleteUser(operand);
//        }
//    }
//}
//
// void SCEV::clear() {
//    for(auto bin : instructionsHasBeenCaculated) {
//        if(!bin->basicblock)
//            unuseall(bin);
//    }
//    instructionsHasBeenCaculated.clear();
//    scevVal.clear();
//}
//
// SCEV& Loop::getSCEV(Instruction* instr) {
//    return SCEVCheck[instr];
//}
//
// bool Loop::hasSCEV(Instruction* instr) {
//    return SCEVCheck.count(instr);
//}
//

//void Loop::registerSCEV(Instruction* instr, SCEV scev) {
//    SCEVCheck[instr] = std::move(scev);
//}
//// NOLINTEND
//