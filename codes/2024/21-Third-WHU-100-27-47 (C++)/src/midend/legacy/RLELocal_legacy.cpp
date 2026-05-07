// #include "RLELocal.h"
// #include "UseDefAnalysis.h"

// using namespace ir;

// void ir::rleLocal(FuncPtr func) {
//     dbgout << std::endl
//            << "RLELocal pass started (" << func->name() << ")." << std::endl;

//     UseDefAnalysisContext useDefCtx{func};

//     unsigned replacedCount = 0;
//     // Load: srcReg -> destReg
//     Map<RegPtr, RegPtr> loadMap;

//     // GEP: (srcReg, offset) -> destReg
//     Map<Expr, RegPtr> gepCache;        // 缓存 GEP 结果
//     Map<RegPtr, Set<Expr>> arrayCache; // 缓存与数组名有关的 GEP 表达式
//     for (auto bb : func->bbSet()) {
//         for (auto inst : bb->getInstTopoList()) {
//             // 处理 GEP 操作
//             if (auto gepInst = castPtr<GEPInst>(inst)) {
//                 auto expr = Expr{gepInst};
//                 auto it = gepCache.find(expr);

//                 if (it != gepCache.end()) {
//                     // 找到缓存的地址，构建Move指令替换当前 GEP 指令
//                     auto cachedReg = it->second;
//                     auto newMoveInst = MoveInst::create(bb, gepInst->destReg(), cachedReg);
//                     Instruction::replace(inst, newMoveInst);
//                     dbgout << "├── GEP instruction replaced: " << gepInst->toString(false) << " => " << newMoveInst->toString(false) << std::endl;
//                     ++replacedCount;
//                     continue;
//                 } else {
//                     // 缓存新的 GEP 结果
//                     gepCache[expr] = gepInst->destReg();
//                     // 数组名 => GEP 表达式
//                     arrayCache[gepInst->arrPtrReg()].insert(expr);
//                     continue;
//                 }
//             }

//             // TODO：这里的处理逻辑太过保守，可以进一步优化
//             // 处理 store 操作，清空 loadMap
//             if (auto storeInst = castPtr<StoreInst>(inst)) {
//                 auto targetAddr = storeInst->destAddrReg();
//                 // 如果更改了数组的元素，这里需要进一步处理
//                 auto defInst = useDefCtx.getDefInst(storeInst->destAddrReg());
//                 if (auto gepInst = castPtr<GEPInst>(defInst)) {
//                     auto array = gepInst->arrPtrReg();
//                     for (auto expr : arrayCache[array]) {
//                         gepCache.erase(expr);
//                         dbgout << "├── GEP cache cleared for store instruction: " << storeInst->toString(false) << std::endl;
//                     }
//                 }
//                 // 更新 loadMap
//                 auto it = loadMap.find(targetAddr);
//                 if (it != loadMap.end()) {
//                     loadMap.erase(it);
//                     dbgout << "├── Load map cleared for store instruction: " << storeInst->toString(false) << std::endl;
//                 }
//                 continue; // 继续处理其他指令
//             }

//             // 处理 load 操作
//             if (auto loadInst = castPtr<LoadInst>(inst)) {
//                 auto srcReg = loadInst->srcAddrReg();
//                 auto it = loadMap.find(srcReg);

//                 if (it != loadMap.end()) {
//                     // 找到之前的 load 结果，替换为move指令
//                     auto cachedReg = it->second;
//                     // 构造move指令
//                     auto newMoveInst = MoveInst::create(bb, loadInst->destReg(), cachedReg);
//                     Instruction::replace(inst, newMoveInst);
//                     dbgout << "├── Load instruction replaced: " << loadInst->toString(false) << " => " << newMoveInst->toString(false) << std::endl;
//                     ++replacedCount;
//                     continue;
//                 } else {
//                     // 记录新的 load 结果
//                     loadMap[srcReg] = loadInst->destReg();
//                     dbgout << "├── Load instruction recorded: " << loadInst->toString(false) << std::endl;
//                     continue;
//                 }
//             }
//         }
//     }

//     dbgout << "└── RLELocal pass done." << std::endl;
// }
