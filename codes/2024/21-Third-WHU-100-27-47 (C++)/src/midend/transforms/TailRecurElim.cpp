// #include "TailRecurElim.h"

// using namespace ir;

// bool ir::tailRecurElim(FuncPtr func) {
//     bool changed = false;

//     dbgout << std::endl
//            << "TailRecurElim pass started (" << func->name() << ")." << std::endl;

//     bool isTailRecursive = false;
//     Set<Ptr<CallInst>> tailSelfCalls{};
//     for (auto bb : func->bbSet()) {
//         for (auto inst : bb->getInstSet()) {
//             if (auto callInst = inst->as<CallInst>()) {
//                 if (callInst->function() != func) {
//                     continue;
//                 }
//                 isTailRecursive |= callInst->isTailCall();
//                 if (!isTailRecursive) {
//                     break;
//                 }
//                 tailSelfCalls.insert(callInst);
//             }
//         }
//         if (!isTailRecursive) {
//             break;
//         }
//     }

//     if (isTailRecursive) {
//         for (auto tailSelfCall : tailSelfCalls) {
//             auto edge = tailSelfCall->parentBB()->exitInst()->uncondBrEdge();
//             edge->redirectDest(func->entryBB()->firstSucc());
//             auto args = tailSelfCall->argList();
//             auto params = func->paramList();
//             for (int i = 0; i < args.size(); ++i) {
//                 edge->addParallelCopy(args[i], params[i]);
//             }
//         }
//     }

//     dbgout << "└── TailRecurElim pass done." << std::endl;
//     return changed;
// }
