// #include "ssa.h"
// #include "DFA.h"

// using namespace ir;
// #pragma region PreProcess
// /// @brief PreProcess  Functions
// void SSA::preprocess() {
//     // Scope *globalscope = symbolTable->getGlobalScope();
//     // std::vector<Symbol *> paravec = globalscope->Symbols[name]->paramSymbols;
//     // Scope *funcscope = symbolTable->getFuncScope(name);
//     // Paras = static_cast<ir::BB>(new ir::BasicBlock("entry"));
//     // basicBlocks.push_back(Paras);
// }

// int SSA::findBlockIndex(ir::BBPtr &block) {

//     for (int n = 0; n < basicBlocks.size(); n++) {
//         if (block == basicBlocks[n])
//             return n;
//     }

//     // can not find the block
//     return -1;
// }

// void SSA::addPreedge() {
//     for (int i = 0; i < basicBlocks.size(); i++) {
//         auto label = basicBlocks[i]->label();
//         if (label == "loop_body") {
//             // basicBlocks[i]->postBlock[0]->preBlock.push_back(basicBlocks[i]);
//         }
//     }
// }

// void SSA::delPreedge() {
//     for (int b_index = 0; b_index < basicBlocks.size(); b_index++) {
//         auto label = basicBlocks[b_index]->label();
//         if (label == "loop_body") {
//             // BasicBlock *temp_whilecondition = basicBlocks[b_index]->postBlock[0];
//             // temp_whilecondition->preBlock.erase(find(temp_whilecondition->preBlock.begin(), temp_whilecondition->preBlock.end(), basicBlocks[b_index]));
//         }
//     }
// }

// void SSA::topoSSA() {
//     // 初始化isUsed数组
//     isUsed.clear();
//     isUsed = std::vector<int>(basicBlocks.size());

//     // 删除未使用的块
//     findUnusedBlock(entry);
//     deleteUnusedBlock();

//     // while
//     delPreedge();

//     isUsed.clear();
//     isUsed = std::vector<int>(basicBlocks.size());
//     Vector<ir::BBPtr> topoRes;
//     toposorting(entry, topoRes);

//     // while
//     addPreedge();

//     basicBlocks.clear();
//     basicBlocks = topoRes;
// }

// void SSA::toposorting(ir::BBPtr bb, Vector<ir::BBPtr> &topoRes) {
//     // 已访问
//     if (isUsed[findBlockIndex(bb)] == 1) {
//         return;
//     }

//     // 拓扑排序结果修改
//     topoRes.push_back(bb);
//     isUsed[findBlockIndex(bb)] = 1;

//     // 对while的特殊处理
//     // if (bb->label == BasicBlock::WHILE_CONDITION) {
//     //     if (bb->succs().size() == 1) {
//     //         bb->label = BasicBlock::NONE;
//     //     }
//     // }

//     // 递归访问后继
//     for (ir::BBPtr next : bb->succs()) {
//         int do_next = 1;
//         for (ir::BBPtr temp_pre : next->preds()) {
//             if (isUsed[findBlockIndex(temp_pre)] == 0) {
//                 do_next = 0;
//                 break;
//             }
//         }
//         if (do_next) {
//             toposorting(next, topoRes);
//         }
//     }
// }

// // 查找未使用块
// void SSA::findUnusedBlock(ir::BBPtr entry) {

//     // 若已访问则直接return
//     if (isUsed[findBlockIndex(entry)] == 1) {
//         return;
//     }

//     Set<ir::InstPtr> tset = entry->getInstSet();
//     Set<ir::BBPtr> bset = entry->succs();

//     // 空块
//     if (tset.empty() && bset.empty()) {
//         ir::Value *nl = new ir::Value(nullptr);
// #if false // 不要用 new
//         ir::InstPtr retnode = static_cast<ir::InstPtr>(new ir::RetInst(*nl));
// #endif
//         // entry->_addInst(entry, retnode);
//     }

//     isUsed[findBlockIndex(entry)] = 1;

//     // 递归调用进行标记
//     for (auto iter : entry->succs()) {
//         findUnusedBlock(iter);
//     }
// }

// // 删除未标记的块
// void SSA::deleteUnusedBlock() {

//     for (int i = 0; i < basicBlocks.size(); ++i) {

//         if (isUsed[i] == 0) {

//             // 当前不可达的基本块
//             ir::BBPtr tempB = basicBlocks[i];

//             // 移除前驱块中的后继引用
//             for (const auto &pre_iter : tempB->preds()) {

//                 if (pre_iter->succs().find(tempB) != pre_iter->succs().end()) {
//                     ir::BasicBlock::removeEdge(pre_iter, tempB);
//                 }
//             }

//             // 移除后继块中的前驱引用
//             for (const auto &post_iter : tempB->succs()) {

//                 if (post_iter->preds().find(tempB) != post_iter->preds().end()) {
//                     ir::BasicBlock::removeEdge(tempB, post_iter);
//                 }
//             }

//             // 从 basicBlocks 列表中删除当前基本块
//             basicBlocks.erase(basicBlocks.begin() + i);
//             i--; // 调整索引以确保遍历正确
//         }
//     }
// }
// #pragma endregion

// #pragma region Dominance
// ///@brief Dominance Calculator Functions
// // 计算支配集
// void SSA::calculateDominance() {

//     // 初始化DOM：指向块本身
//     for (ir::BBPtr block : basicBlocks) {
//         CFG_Dom[block].insert(block);
//     }

//     // 迭代计算方法：对于每个基本块，通过对前驱节点的支配集求交集，再加上当前基本块自身。
//     for (int i = 0; i < basicBlocks.size(); i++) {
//         // 获取前驱节点
//         Set<ir::BBPtr> preBlock = basicBlocks[i]->preds();
//         Set<ir::BBPtr> domSet;

//         if (!preBlock.empty()) {
//             // 使用迭代器获取 preBlock 中的第一个元素
//             auto it = preBlock.begin();
//             domSet = CFG_Dom[*it];

//             for (ir::BBPtr pre : preBlock) {
//                 Set<ir::BBPtr> temp;
//                 set_intersection(
//                     CFG_Dom[pre].begin(), CFG_Dom[pre].end(),
//                     domSet.begin(), domSet.end(),
//                     inserter(temp, temp.begin()));
//                 domSet = temp;
//             }
//         }
//         domSet.insert(basicBlocks[i]);

//         CFG_Dom[basicBlocks[i]] = domSet;
//     }
// }

// // 计算直接支配者
// // 向上迭代，找到最近的支配者
// void SSA::calculateImmediateDominators() {
//     for (int i = 0; i < basicBlocks.size(); ++i) {

//         bool findTag = false;
//         std::vector<ir::BBPtr> currentBlock;
//         std::vector<ir::BBPtr> preBlock;

//         // 当前Block作为查找起点
//         preBlock.push_back(basicBlocks[i]);
//         while (!findTag) {
//             // 无前驱
//             if (preBlock.empty()) {
//                 CFG_IDom[i] = nullptr;
//                 break;
//             }

//             currentBlock.assign(preBlock.begin(), preBlock.end());
//             preBlock.clear();

//             for (auto j : preBlock) {
//                 Set<ir::BBPtr> temp = j->preds();
//                 preBlock.insert(preBlock.end(), temp.begin(), temp.end());
//             }

//             for (auto it : CFG_Dom[basicBlocks[i]]) {
//                 if (std::find(preBlock.begin(), preBlock.end(), it) != preBlock.end()) {
//                     findTag = true;
//                     CFG_IDom[i] = it;
//                     Domtree_sons[it].push_back(basicBlocks[i]);
//                     break;
//                 }
//             }
//         }
//     }
// }

// // 计算支配边界
// void SSA::calculateDominanceFrontier() {
//     for (int i = 0; i < basicBlocks.size(); i++) {
//         ir::BBPtr currentBlock = basicBlocks[i];
//         Set<ir::BBPtr> preblocks = currentBlock->preds();
//         // 具有多个前驱
//         if (preblocks.size() > 1) {
//             for (ir::BBPtr blockJ : preblocks) {

//                 while (blockJ != CFG_IDom[i]) {
//                     int df_index = findBlockIndex(blockJ);
//                     CFG_DF[blockJ].insert(currentBlock);
//                     blockJ = CFG_IDom[df_index];
//                 }
//             }
//         }
//     }
// }

// #pragma endregion

// #pragma region Basics

// // Copy
// struct Copy {
//     Value src;
//     RegPtr dest;
//     BBPtr srcBB;

//     // 默认构造函数
//     Copy() : src(nullptr), dest(nullptr) { }

//     // 其他构造函数可以在这里定义，如需要显式初始化的构造函数
//     Copy(Value s, RegPtr d, BBPtr bb) : src(s), dest(d), srcBB(bb) { }

//     bool operator==(const Copy &other) const {
//         return src == other.src && dest == other.dest;
//     }

//     bool operator!=(const Copy &other) const {
//         return !(*this == other);
//     }
// };

// namespace std {
//     template <>
//     struct hash<Copy> {
//         std::size_t operator()(const Copy &copy) const {
//             std::size_t result = 0;
//             hashCombine(result, copy.src);
//             hashCombine(result, copy.dest);
//             return result;
//         }
//     };
// }

// // PCI，由单个块所拥有
// Set<Copy> pcopy;

// // 保存每个块拥有的PCI

// std::unordered_map<BBPtr, Set<Copy>> pci_bb;
// std::unordered_map<BBPtr, Vector<Copy>> seq_bb;

// #pragma endregion

// #pragma region Rename
// ///@brief Functions that are used to rename blocks and variables

// void SSA::rename() {
//     for (auto bb : basicBlocks) {
//         varScanner(bb);
//     }
// }

// // 扫描块内变量
// void SSA::varScanner(ir::BBPtr bb) {

//     for (auto iter : bb->getInstSet()) {
//         // 只需要查找到allocInst
//         if (auto loadInstPtr = std::dynamic_pointer_cast<ir::AllocInst>(iter)) {

//             // 重命名
//             // auto regRename = loadInstPtr->destReg();
//             auto newInst = renameVariables(loadInstPtr);

//             ir::Instruction::replace(iter, newInst);
//         }
//         // if (auto loadInstPtr = std::dynamic_pointer_cast<ir::LoadInst>(iter)) {
//         //     // iter 是 LoadInst 类型

//         // } else if (auto storeInstPtr = std::dynamic_pointer_cast<ir::StoreInst>(iter)) {
//         //     // iter 是 StoreInst 类型

//         // } else if (auto opInstPtr = std::dynamic_pointer_cast<ir::OperInst>(iter)) {
//         //     // iter 是 OperInst 类型
//         // }
//     }
// }

// ir::InstPtr SSA::renameVariables(Ptr<ir::AllocInst> inst) {

//     // 目标寄存器
//     ir::RegPtr reg = inst->destReg();

//     // 获取原有名字
//     std::string newName = reg->name();

//     // 重新标号
//     newName = newName + std::to_string(renameCounter[reg]);
//     renameCounter[reg]++;

//     // 重命名后的寄存器
//     ir::RegPtr regRename = static_cast<ir::RegPtr>(new ir::Register(reg->dataType(), newName));

//     // 重命名Inst
//     Ptr<ir::AllocInst> instR = AllocInst::create(inst->parentBB(), regRename);

//     return static_cast<ir::InstPtr>(instR);
// }

// #pragma endregion

// #pragma region Phi-Web

// // 并查集数据结构用于管理phi-web
// Map<RegPtr, RegPtr> phiWebParent;

// // 并查集查找函数
// RegPtr find(RegPtr reg) {
//     if (phiWebParent.find(reg) == phiWebParent.end()) {
//         phiWebParent[reg] = reg; // 自引用初始化
//     }
//     if (phiWebParent[reg] != reg) {
//         phiWebParent[reg] = find(phiWebParent[reg]); // 路径压缩
//     }
//     return phiWebParent[reg];
// }

// // 并查集合并函数
// void unionRegs(RegPtr reg1, RegPtr reg2) {
//     auto root1 = find(reg1);
//     auto root2 = find(reg2);
//     if (root1 != root2) {
//         phiWebParent[root2] = root1; // 合并集合
//     }
// }

// void SSA::phiwebSearch() {
//     for (auto bb : basicBlocks) {
//         for (auto phiInst : bb->getPhiNodes()) {

//             // 获取φ指令的目标寄存器
//             RegPtr destReg = phiInst->destReg();
//             // 遍历φ指令中的所有源寄存器
//             auto uses = phiInst->uses();
//             for (auto use : uses) {
//                 RegPtr useReg = use;
//                 unionRegs(destReg, useReg);
//                 // use.replace(ir::Register::create(PrimaryDataType::Int));
//             }
//         }
//     }
// }

// #pragma endregion

// #pragma region Edge Splitting

// /*
// Previous:
//       +----+
//       | B1 |
//       +----+
//         |a = 3
//         |
//       /     \
//   +----+   +----+
//   | B2 |   | B3 |
//   +----+   +----+
//     |a1 = a+1   |a2 = a*2
//     |           |
//   \ /         / \
//     +----+   +----+
//     | B4 |
//     +----+
//       |c = φ(a1 from B2, a2 from B3)
//       |b = c

// After:                                                             Modified Algorithm:
//       +----+                                                            +----+
//       | B1 |                                                            | B1 |
//       +----+                                                            +----+
//         |a = 3                                                             |a = 3
//         |                                                                  |
//       /     \                                                            /     \
//   +----+   +----+                                                      +----+   +----+
//   | B2 |   | B3 |                                                      | B2 |   | B3 |
//   +----+   +----+                                                      +----+   +----+
//     |a1 = a+1   |a2 = a*2                                                 |a1 = a+1   |a2 = a*2
//     |           |                                                          |           |
//     v           v                                                          v           v
//   +----+   +----+                                                      +----+   +----+
//   | B2'|   | B3'|                                                      | B2'|   | B3'|
//   +----+   +----+                                                      +----+   +----+
//     |PCI: a1' <- a1   |PCI: a2' <- a2                                     |PCI: c <- a1   |PCI: c <- a2
//     |                 |                                                    |                 |
//   \ /               / \                                                \ /               / \
//     +----+   +----+                                                      +----+   +----+
//     | B4 |                                                               | B4 |
//     +----+                                                               +----+
//       |c = φ(a1' from B2', a2' from B3')                                   | // 修改后的算法直接删除PhiInst
//       |b = c                                                               |b = c

// //附注：PCI的处理交由算法3.6实现简单序列化
// */

// // 插入关键边缘拆分
// void insertCriticalEdgeSplitting(FuncPtr func) {

//     // 遍历所有基本块，检查并处理所有出边
//     for (auto bb : func->bbSet()) {

//         // step 1 : 处理关键边
//         auto outEdges = bb->outEdges();
//         if (outEdges.size() > 1) { // 检查多个出边的情况
//             for (auto outEdge : outEdges) {
//                 auto succ = outEdge->dest();
//                 auto succInEdges = succ->inEdges();
//                 if (succInEdges.size() > 1) { // 检查后继有多个入边的情况，即关键边
//                     // outEdge 是关键边
//                     // 创建新的基本块
//                     auto splitBB = BasicBlock::create(func, "split");
//                     // 连接splitBB与原有俩基本块
//                     outEdge->redirectDest(splitBB);
// #if false // BrInst::createUnconditional() is deprecated
//                     Instruction::insertAfter(splitBB->entryInst(), BrInst::createUnconditional(splitBB, succ));
// #endif

//                     // newBlocks[succ] = splitBB; // Store new block for later use

//                     // 插入空的并行拷贝指令
//                     // 实现逻辑，维护一个map，BBPtr----->该BasicBlock的PCI
//                     // 对新基本块插入一个空的Copy集合
//                     // 使用.find()而不是[]操作符，以避免创建不存在的键
//                     auto it = pci_bb.find(bb);
//                     if (it == pci_bb.end()) {
//                         pci_bb.insert({bb, Set<Copy>()});
//                     }
//                 }
//             }
//         }
//         // 如果只有一个出边，则直接在当前Block中的末尾增加一条PciInst
//         else {
//             auto it = pci_bb.find(bb);
//             if (it == pci_bb.end()) {
//                 pci_bb.insert({bb, Set<Copy>()});
//             }
//         }
//     }

//     for (auto bb : func->bbSet()) {
//         // step 2 : 处理φ函数中的变量
//         auto phiNodes = bb->getPhiNodes();
//         for (auto phiInst : phiNodes) {

//             auto destReg = phiInst->destReg(); // phi函数目标寄存器

//             // if (newBlocks.find(bb) != newBlocks.end()) {

//             // Get the Set of instructions
//             auto &pc = pci_bb[bb];

//             for (auto tuple : phiInst->getTuples()) {
//                 // Algorithm 3.5:
//                 // auto freshReg = Register::create(use->dataType(), use->name() + "'");
//                 // pci->addCopy(freshReg, useReg);            // Add copy to the parallel copy instruction
//                 // phiInst->replaceReg(useRef, freshReg); // Replace source in phiInst

//                 // Modified Algorithm:
//                 auto newCopy = Copy(tuple.value(), destReg, tuple.bb());
//                 pc.insert(newCopy);
//             }

//             //}

//             // Modified Algorithm:删除PhiInst
//             phiInst->remove(phiInst);
//         }
//     }
// }

// #pragma endregion

// #pragma region Replacement of parallel copies

// // 判断一个拷贝指令是否存在于并行拷贝中
// bool isInPcopy(const std::unordered_set<Copy> &pcopy, const Copy &copy) {
//     return pcopy.find(copy) != pcopy.end();
// }

// // 查找并行拷贝中是否存在从a到b的拷贝
// bool hasCopy(const std::unordered_set<Copy> &pcopy, const Value &a, const RegPtr &b) {
//     for (const auto &c : pcopy) {
//         if (c.src == a && c.dest == b) {
//             return true;
//         }
//     }
//     return false;
// }

// // 并行拷贝指令序列化
// std::vector<Copy> replaceParallelCopiesWithSequential(std::unordered_set<Copy> &pcopy) {

//     std::vector<Copy> seq; // 用来存储顺序拷贝指令序列

//     while (!pcopy.empty()) {
//         bool progress = false;

//         for (auto it = pcopy.begin(); it != pcopy.end();) {
//             auto copy = *it;
//             // 如果不存在从dest到src的拷贝，这个拷贝可以直接执行
//             if (!(copy.src.isRegister() && hasCopy(pcopy, copy.dest, copy.src.getRegister()))) {
//                 seq.push_back(copy);
//                 it = pcopy.erase(it); // 从并行拷贝中移除
//                 progress = true;
//             } else {
//                 ++it;
//             }
//         }

//         if (!progress) { // 如果所有剩余的拷贝都形成循环
//             // 选择一个拷贝来打破循环
//             auto it = pcopy.begin();
//             auto copy = *it;
//             dbgassert(copy.src.isRegister(), "Copy source should be a register to form a cycle");
//             auto srcReg = copy.src.getRegister();

//             RegPtr freshVar = Register::create(srcReg->dataType(), srcReg->name() + "'"); // 创建新的临时变量

//             seq.push_back({copy.src, freshVar, copy.srcBB}); // 添加新的拷贝指令
//             pcopy.insert({freshVar, copy.dest, copy.srcBB}); // 替换原有拷贝
//             pcopy.erase(it);                                 // 移除原有的拷贝
//         }
//     }

//     return seq;
// }

// void sequentialize(FuncPtr func) {
//     for (auto bb : func->bbSet()) {
//         seq_bb[bb] = replaceParallelCopiesWithSequential(pci_bb[bb]);
//     }
// }
// // // 全局并行拷贝指令序列化
// // void replaceParallelCopiesWithSequential() {
// //     Set<Copy> global_pcopy;

// //     // 整合所有基本块的PCI到一个全局集合
// //     for (const auto &bb_pci : pci_bb) {
// //         global_pcopy.insert(bb_pci.second.begin(), bb_pci.second.end());
// //     }

// //     std::vector<Copy> seq; // 用来存储顺序拷贝指令序列

// //     while (!global_pcopy.empty()) {
// //         bool progress = false;

// //         for (auto it = global_pcopy.begin(); it != global_pcopy.end();) {
// //             auto copy = *it;
// //             // 如果不存在从dest到src的拷贝，这个拷贝可以直接执行
// //             if (!hasCopy(global_pcopy, copy.dest, copy.src)) {
// //                 seq.push_back(copy);
// //                 it = global_pcopy.erase(it); // 从并行拷贝中移除
// //                 progress = true;
// //             } else {
// //                 ++it;
// //             }
// //         }

// //         if (!progress) { // 如果所有剩余的拷贝都形成循环
// //             // 选择一个拷贝来打破循环
// //             auto it = global_pcopy.begin();
// //             auto copy = *it;

// //             RegPtr freshVar = Register::create(copy.src->dataType(), copy.src->name() + "'"); // 创建新的临时变量

// //             seq.push_back({copy.src, freshVar});        // 添加新的拷贝指令
// //             global_pcopy.insert({freshVar, copy.dest}); // 替换原有拷贝
// //             global_pcopy.erase(it);                     // 移除原有的拷贝
// //         }
// //     }
// // }

// void copyToAssign(FuncPtr func) {
//     for (auto bb : func->bbSet()) {
//         auto seq = seq_bb[bb];
//         for (auto &copy : seq) {
//             auto src = copy.src;
//             auto dest = copy.dest;
//             auto srcBB = copy.srcBB;
//             auto assign = ir::MoveInst::create(srcBB, dest, src);
//             ir::Instruction::insertBefore(srcBB->exitInst(), assign);
//         }
//     }
// }
// #pragma endregion

// #pragma region Entry

// void ir::ssaDestruction(FuncPtr func) {
//     dbgout << std::endl
//            << "SSADestruction pass started (" << func->name() << ")." << std::endl;

//     insertCriticalEdgeSplitting(func);
//     sequentialize(func);
//     copyToAssign(func);

//     dbgout << "└── SSADestruction pass done." << std::endl;
// }
// #pragma endregion
