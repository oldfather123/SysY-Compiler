// #pragma once

// #include "IR.h"
// #include "SymbolTable.h"
// #include "Common.h"
// #include <algorithm>
// #include <iostream>
// #include <map>
// #include <set>
// #include <string>

// class SSA {
// private:
//     std::string name;
//     ir::FuncPtr CFG;
//     ir::BBPtr entry; // CFG的入口节点
//     // Ptr<SymbolTable> symbolTable; // 符号表
//     ir::BBPtr Paras;
//     std::vector<int> isUsed;
//     std::vector<ir::BBPtr> basicBlocks; // 存储所有基本块的列表

//     std::vector<std::vector<ir::BBPtr>> CFG_Pre;              // 每个块的前驱
//     std::map<ir::BBPtr, Set<ir::BBPtr>> CFG_DF;               // 支配边界
//     std::vector<ir::BBPtr> CFG_IDom;                          // 直接支配者（IDom）
//     std::map<ir::BBPtr, Set<ir::BBPtr>> CFG_Dom;              // 支配者
//     std::map<ir::BBPtr, std::vector<ir::BBPtr>> Domtree_sons; // 支配者树后继节点

//     std::map<ir::BBPtr, std::vector<Ptr<Symbol>>> phiFunctions;      // 每个基本块的Phi函数
//     std::map<ir::RegPtr, int> renameCounter;                         // 重命名计数器
//     std::map<Ptr<Symbol>, std::vector<Ptr<Symbol>>> currentNames;    // 当前活动的名称栈
//     std::map<ir::BBPtr, std::vector<Ptr<Symbol>>> definedVariables;  // 每个基本块定义的变量列表
//     std::map<Ptr<Symbol>, std::vector<ir::BBPtr>> variableUseBlocks; // 变量被使用的基本块列表
//     std::map<ir::BBPtr, std::vector<Ptr<Symbol>>> liveOutVariables;  // 每个基本块的活跃外部变量
//     std::stack<Ptr<Symbol>> symbolStack;                             // 用于重命名的符号栈
//     std::map<ir::BBPtr, bool> visitedBlocks;                         // 标记已访问的基本块，用于DFS和其他遍历

// public:
//     SSA(ir::BBPtr entryNode /*, Ptr<SymbolTable> symTable*/) : entry(entryNode) /*, symbolTable(symTable)*/ {
//         // 预处理
//         preprocess();

//         // 获取basicBlocks
//         Set<ir::BBPtr> bbSet = CFG->bbSet();
//         for (auto bb : bbSet) {
//             basicBlocks.push_back(bb);
//         }

//         // 获取name标签
//         // name = CFG->name();

//         // 拓扑排序
//         topoSSA();

//         // 初始化支配集大小
//         CFG_IDom = std::vector<ir::BBPtr>(bbSet.size());
//     }
//     void preprocess();
//     int findBlockIndex(ir::BBPtr &block);
//     void topoSSA();
//     void toposorting(ir::BBPtr bb, Vector<ir::BBPtr> &topoRes);
//     void calculateDominance();
//     void calculateImmediateDominators(); // 计算直接支配集
//     void calculateDominanceFrontier();   // 计算支配前沿
//     // void insertPhiFunctions();           // 放置phi函数
//     void rename();
//     void varScanner(ir::BBPtr bb);
//     ir::InstPtr renameVariables(Ptr<ir::AllocInst> reg); // 变量重命名
//     // void renameBlocks();                                 // 块重命名
//     // void calculateLiveVariables();                       // 计算活跃变量
//     // void pruneUnusedPhiFunctions();                      // 删除未使用的Phi函数
//     // void optimizeSSA();                                  // 进行SSA的优化
//     void findUnusedBlock(ir::BBPtr entry); // 标记不可达的块
//     void deleteUnusedBlock();              // 删除不可达的块
//     void phiwebSearch();

//     // 辅助函数
//     void depthFirstSearch(Ptr<ir::GraphNode<ir::BasicBlock>> node, std::vector<Ptr<ir::GraphNode<ir::BasicBlock>>> &visited); // DFS遍历CFG
//     void initializeSymbolStack();                                                                                             // 初始化符号栈
//     void clearVisited();                                                                                                      // 清除访问标记
//     void addPreedge();
//     void delPreedge();
// };

// namespace ir {
//     void ssaDestruction(ir::FuncPtr func);
// }
