// #include "LICM.h"

// using namespace ir;

// /*
// This pass uses alias analysis for two purposes:

// 1. Moving loop invariant loads and calls out of loops.
// If we can determine that a load or call inside of a loop never aliases anything stored to,
// we can hoist it or sink it like any other instruction.

// 2. Scalar Promotion of Memory.
// If there is a store instruction inside of the loop, we try to move the store to happen AFTER the loop instead of inside of the loop.
// This can only happen if a few conditions are true:

// 2.1 The pointer stored through is loop invariant.

// 2.2 There are no stores or loads in the loop which may alias the pointer.
// There are no calls in the loop which mod/ref the pointer.

// If these conditions are true, we can promote the loads and stores in the loop of the pointer to use a temporary alloca’d variable.
// We then use the mem2reg functionality to construct the appropriate SSA form for the variable.
// */

// Set<RegPtr> regsInLoadUse; // all registers that in uses() of load instruction
// Set<RegPtr> regsInStoreWr; // all registers that in defs() of use instruction

// std::pair<RegPtr, InstPtr> regToInst;

// Vector<ir::InstPtr> hoistLoadInsts;
// Vector<ir::InstPtr> sinkStoreInsts;

// Map<Ptr<Loop>, bool> isDone; // store information of if a loop has been handled

// // test Tool : print information of a loop
// void printLoop(Ptr<ir::Loop> loop);

// // the entry of whole algorithm
// void ir::runLICM(ir::FuncPtr func) {
//     LoopDetection dect;
//     dect.detectLoops(func);
//     Vector<Ptr<Loop>> loops = dect.getLoops();
//     std::cout << "size" << loops.size() << std::endl;
//     std::cout << "before LICM" << std::endl;
//     for (auto loop : loops) {
//         isDone[loop] = false;
//         // for test
//         printLoop(loop);
//     }
//     for (auto loop : loops) {
//         traverseLoop(loop, func);
//     }
//     // for test
//     std::cout << "after LICM" << std::endl;
//     for (auto loop : loops) {
//         printLoop(loop);
//     }
// }

// void ir::traverseLoop(Ptr<Loop> loop, ir::FuncPtr func) {
//     if (isDone[loop])
//         return;
//     for (auto subLoop : loop->getSubLoops()) {
//         traverseLoop(subLoop, func);
//     }
//     codeMotionHandler(loop, func);
//     isDone[loop] = true;
// }

// // if count times in Map is greater than 1, that means the register is changed in the loop

// // Set<ir::RegPtr> globalReg;
// // Set<ir::RegPtr> unChangedGlobal;
// // while(){
// //     a
// //     while(){

// //     }
// //     store b to a
// // }
// void scanLoop(Ptr<Loop> loop) {
//     for (auto subLoop : loop->getSubLoops()) {
//         scanLoop(subLoop);
//     }
//     for (auto bb : loop->getLoopBBs()) {
//         for (auto inst : bb->getInstSet()) {
//             // store all register used in each instrucrion(load/store/call)
//             ir::Instruction::Type instType = inst->instType();
//             if (instType == ir::Instruction::Load) {
//                 for (auto reg : inst->uses()) {
//                     regsInLoadUse.insert(reg);
//                 }
//                 // for (auto reg : inst->defs()) {
//                 //     regsInDef.insert(reg);
//                 // }

//                 // if (castPtr<ir::DefInst>(inst) != nullptr) {
//                 //     auto dest = *castPtr<ir::Instruction>(inst)->defs().begin();
//                 //     regChangeCountInLoop[dest]++;
//                 //     if (regChangeCountInLoop[dest] > 1) {
//                 //         regChangedInLoop.insert(dest);
//                 //     }
//                 // }
//             } else if (instType == ir::Instruction::Store) {
//                 auto storeInst = castPtr<ir::StoreInst>(inst);
//                 // if (storeInst->srcVal().isRegister()) {
//                 //     regsInUse.insert(storeInst->srcVal().getRegister());
//                 // }
//                 regsInStoreWr.insert(storeInst->destAddrReg());
//             } else if (instType == ir::Instruction::Call) {
//                 auto callInst = castPtr<ir::CallInst>(inst);
//                 for (auto reg : callInst->uses()) {
//                     regsInLoadUse.insert(reg);
//                 }
//             }
//         }
//     }
// }
// /*
// while(a>1){
//     int c = exgcd(a,5);
//     a = a -1;
// }
// loop_body:
//     %c = alloca i32*
//     %3 = load i32, i32* %a
//     br caller
// caller:
//     %4 = call i32 @exgcd(%3, 5)
//     br call_ret
// call_ret:
//     store i32 %4, i32* %c
//     %5 = load i32, i32* %a
//     %6 = sub i32 %5, 1
//     store i32 %6, i32* %a
//     br loop_cond

//     regsInLoadUse : %a %3
//     regsInStoreWr : %c %a
// */

// /*
//     %d = alloca i32*
//     %6 = load i32, i32* %c
//     store i32 %6, i32* %d

//     %7 = load i32, i32* %a
//     %8 = sub i32 %7, 1
//     store i32 %8, i32* %a

//     regsInLoadUse : %a %c
//     regsInStoreWr : %a %d
//  */
// // the function that decides which instruction should be moved outside of the loop
// // the mapping result will be stored in the map doMoveInst
// void aliasExit(ir::InstPtr inst) {
//     if (auto loadInst = castPtr<ir::LoadInst>(inst)) {
//         // the load instuction's srcaddrReg has not been modified
//         if (regsInStoreWr.find(loadInst->srcAddrReg()) == regsInStoreWr.end()) {
//             hoistLoadInsts.push_back(inst);
//         }
//     } else if (auto storeInst = castPtr<ir::StoreInst>(inst)) {
//         if (regsInLoadUse.find(storeInst->destAddrReg()) == regsInLoadUse.end()) {
//             sinkStoreInsts.push_back(inst);
//         }
//     } else if (auto callInst = castPtr<ir::CallInst>(inst)) {
//         for (auto argReg : callInst->argList()) {
//             if (argReg.isRegister()) {
//                 if (regsInStoreWr.find(argReg.getRegister()) == regsInStoreWr.end()) {
//                     hoistLoadInsts.push_back(inst);
//                 }
//             }
//         }
//     }
//     return;
// }

// void ir::codeMotionHandler(Ptr<Loop> loop, ir::FuncPtr func) {
//     scanLoop(loop);

//     bool changed;

//     for (auto bb : loop->getLoopBBs()) {
//         for (auto inst : bb->getInstSet()) {
//             auto type = inst->instType();
//             if (type == ir::Instruction::Load || type == ir::Instruction::Store || type == ir::Instruction::Call) {
//                 aliasExit(inst);
//             }
//         }
//     }

//     if (hoistLoadInsts.empty() && sinkStoreInsts.empty()) {
//         return;
//     }

//     if (loop->getPreheaderBB() == nullptr) {
//         loop->setPreheaderBB(ir::BasicBlock::create(func, "licm_hoist"));
//     }

//     auto addedNewPreBB = loop->getPreheaderBB();

//     // Vector<Ptr<ir::BasicBlock>> OldPreBBToRemove;
//     for (auto &oldPreBB : loop->getHeaderBB()->preds()) {
//         if (loop->getBackEdgesBlocksSet().find(oldPreBB) != loop->getBackEdgesBlocksSet().end())
//             continue;

//         for (auto inedge : loop->getHeaderBB()->inEdges()) {
//             auto backEdgeBlockSet = loop->getBackEdgesBlocksSet();
//             // 只对非回边进行split操作
//             if (backEdgeBlockSet.find(inedge->src()) == backEdgeBlockSet.end()) {
//                 auto [e1, e2] = BasicBlock::splitEdge(inedge, addedNewPreBB);
//                 e2->setUncondBr();
//             }
//         }
//         // oldPreBB->removeEdge(oldPreBB, loop->getHeaderBB());
//         // oldPreBB->addEdge(oldPreBB, addedNewPreBB);
//         // // addedNewPreBB->addEdge(oldPreBB, addedNewPreBB);
//         // ir::ExitInst::create(addedNewPreBB);
//         // OldPreBBToRemove.push_back(oldPreBB);
//         // }

//         // for (auto pred : OldPreBBToRemove) {
//         //     loop->getHeaderBB()->removeEdge(pred, loop->getHeaderBB());
//     }

//     if (!hoistLoadInsts.empty()) {
//         // hoist invariant load/call code
//         for (auto inst : hoistLoadInsts) {
//             // std::cout << inst->toString() << std::endl;
//             if (inst->parentBB() != nullptr) {
//                 Instruction::remove(inst);
//             }
//             auto newInst = ir::Instruction::clone(inst, addedNewPreBB);
//             Instruction::insertBefore(addedNewPreBB->exitInst(), newInst);
//         }
//     }

//     auto addedNewSucBB = ir::BasicBlock::create(func, "licm_sink");
//     auto brInst = loop->getHeaderBB()->exitInst();
//     auto endBB = brInst->condBrFalseEdge()->dest();
//     loop->setEndBB(endBB);
//     auto inedge = loop->getHeaderBB()->exitInst()->condBrFalseEdge();
//     auto [e1, e2] = BasicBlock::splitEdge(inedge, addedNewSucBB);
//     e2->setUncondBr();
//     // for (auto inedge : endBB->inEdges()) {
//     //     if (inedge == loop->getHeaderBB()->exitInst()->condBrTrueEdge()) {
//     //         std::cout << "inedge comes from header" << std::endl;
//     //     }
//     //     if (inedge != loop->getHeaderBB()->exitInst()->condBrTrueEdge()) {
//     //         auto [e1, e2] = BasicBlock::splitEdge(inedge, addedNewSucBB);
//     //         e2->setUncondBr();
//     //     }
//     // }

//     if (!sinkStoreInsts.empty()) {
//         // sink invariant store code
//         for (auto inst : sinkStoreInsts) {
//             // std::cout << inst->toString() << std::endl;
//             if (inst->parentBB() != nullptr) {
//                 Instruction::remove(inst);
//             }
//             auto newInst = ir::Instruction::clone(inst, addedNewSucBB);
//             Instruction::insertBefore(addedNewSucBB->exitInst(), newInst);
//         }
//     }
//     loop->setEndBB(addedNewSucBB);
// }

// void printLoop(Ptr<ir::Loop> loop) {
//     std::cout << "Loop preheader: " << std::endl;
//     if (loop->getPreheaderBB() != nullptr) {
//         std::cout << loop->getPreheaderBB()->toString() << std::endl;
//     }
//     std::cout << std::endl;

//     std::cout << "Loop header: " << std::endl;
//     std::cout << loop->getHeaderBB()->toString() << std::endl;
//     std::cout << std::endl;

//     std::cout << "Loop blocks: " << std::endl;
//     for (auto &bb : loop->getLoopBBs()) {
//         std::cout << bb->toString() << std::endl;
//     }
//     std::cout << std::endl;

//     std::cout << "Sub loops: " << std::endl;
//     std::cout << "Sub Loops Size: " << loop->getSubLoops().size() << std::endl;
//     for (auto sub_loop : loop->getSubLoops()) {
//         printLoop(sub_loop);
//     }
//     std::cout << std::endl;

//     std::cout << "loop end: " << std::endl;
//     if (loop->getEndBB() != nullptr) {
//         std::cout << loop->getEndBB()->toString() << std::endl;
//     }
//     std::cout << std::endl;
// }
