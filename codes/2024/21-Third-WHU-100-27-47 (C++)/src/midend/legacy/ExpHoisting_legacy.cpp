// #include "ExpHoisting.h"

// void ir::runHosit(ir::FuncPtr func) {
// }

// void ir::ifElseDetector::getIfElse(ir::FuncPtr) {
// }

// Vector<ir::ifElse> ir::ifElseDetector::getSubIfElse(Ptr<ir::ifElse> entry) {
//     Vector<ir::ifElse> res;
//     auto condition = entry->getCondtion();
//     auto thenBB = entry->getThenBB();
//     auto elseBB = entry->getElseBB();

//     for (auto inst : thenBB->getInstSet()) {
//         if (auto subIfElse = std::dynamic_pointer_cast<ir::ExitInst>(inst)) {
//             if (subIfElse->isCondBr()) {
//             }
//         }
//     }
//     return res;
// }
