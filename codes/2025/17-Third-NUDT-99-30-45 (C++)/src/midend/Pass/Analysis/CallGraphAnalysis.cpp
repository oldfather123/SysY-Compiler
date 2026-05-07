#include "CallGraphAnalysis.h"
#include "SysYIRPrinter.h"
#include <iostream>
#include <stack>
#include <unordered_set>

extern int DEBUG;

namespace sysy {

// 静态成员初始化
void* CallGraphAnalysisPass::ID = (void*)&CallGraphAnalysisPass::ID;

// ========== CallGraphAnalysisResult 实现 ==========

CallGraphAnalysisResult::Statistics CallGraphAnalysisResult::getStatistics() const {
    Statistics stats = {};
    stats.totalFunctions = nodes.size();
    
    size_t totalCallEdges = 0;
    size_t recursiveFunctions = 0;
    size_t selfRecursiveFunctions = 0;
    size_t totalCallers = 0;
    size_t totalCallees = 0;
    
    for (const auto& pair : nodes) {
        const auto& node = pair.second;
        totalCallEdges += node->callees.size();
        totalCallers += node->callers.size();
        totalCallees += node->callees.size();
        
        if (node->isRecursive) recursiveFunctions++;
        if (node->isSelfRecursive) selfRecursiveFunctions++;
    }
    
    stats.totalCallEdges = totalCallEdges;
    stats.recursiveFunctions = recursiveFunctions;
    stats.selfRecursiveFunctions = selfRecursiveFunctions;
    stats.stronglyConnectedComponents = sccs.size();
    
    // 计算最大SCC大小
    size_t maxSCCSize = 0;
    for (const auto& scc : sccs) {
        maxSCCSize = std::max(maxSCCSize, scc.size());
    }
    stats.maxSCCSize = maxSCCSize;
    
    // 计算平均值
    if (stats.totalFunctions > 0) {
        stats.avgCallersPerFunction = static_cast<double>(totalCallers) / stats.totalFunctions;
        stats.avgCalleesPerFunction = static_cast<double>(totalCallees) / stats.totalFunctions;
    }
    
    return stats;
}

void CallGraphAnalysisResult::print() const {
    std::cout << "---- Call Graph Analysis Results for Module ----\n";
    
    // 打印基本统计信息
    auto stats = getStatistics();
    std::cout << "  Statistics:\n";
    std::cout << "    Total Functions: " << stats.totalFunctions << "\n";
    std::cout << "    Total Call Edges: " << stats.totalCallEdges << "\n";
    std::cout << "    Recursive Functions: " << stats.recursiveFunctions << "\n";
    std::cout << "    Self-Recursive Functions: " << stats.selfRecursiveFunctions << "\n";
    std::cout << "    Strongly Connected Components: " << stats.stronglyConnectedComponents << "\n";
    std::cout << "    Max SCC Size: " << stats.maxSCCSize << "\n";
    std::cout << "    Avg Callers per Function: " << stats.avgCallersPerFunction << "\n";
    std::cout << "    Avg Callees per Function: " << stats.avgCalleesPerFunction << "\n";
    
    // 打印拓扑排序结果
    std::cout << "  Topological Order (" << topologicalOrder.size() << "):\n";
    for (size_t i = 0; i < topologicalOrder.size(); ++i) {
        std::cout << "    " << i << ": " << topologicalOrder[i]->getName() << "\n";
    }
    
    // 打印强连通分量
    if (!sccs.empty()) {
        std::cout << "  Strongly Connected Components:\n";
        for (size_t i = 0; i < sccs.size(); ++i) {
            std::cout << "    SCC " << i << " (size " << sccs[i].size() << "): ";
            for (size_t j = 0; j < sccs[i].size(); ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << sccs[i][j]->getName();
            }
            std::cout << "\n";
        }
    }
    
    // 打印每个函数的详细信息
    std::cout << "  Function Details:\n";
    for (const auto& pair : nodes) {
        const auto& node = pair.second;
        std::cout << "    Function: " << node->function->getName();
        
        if (node->isRecursive) {
            std::cout << " (Recursive";
            if (node->isSelfRecursive) std::cout << ", Self";
            if (node->recursiveDepth >= 0) std::cout << ", Depth=" << node->recursiveDepth;
            std::cout << ")";
        }
        std::cout << "\n";
        
        if (!node->callers.empty()) {
            std::cout << "      Callers (" << node->callers.size() << "): ";
            bool first = true;
            for (Function* caller : node->callers) {
                if (!first) std::cout << ", ";
                std::cout << caller->getName();
                first = false;
            }
            std::cout << "\n";
        }
        
        if (!node->callees.empty()) {
            std::cout << "      Callees (" << node->callees.size() << "): ";
            bool first = true;
            for (Function* callee : node->callees) {
                if (!first) std::cout << ", ";
                std::cout << callee->getName();
                first = false;
            }
            std::cout << "\n";
        }
    }
    
    std::cout << "--------------------------------------------------\n";
}

void CallGraphAnalysisResult::addNode(Function* F) {
    if (nodes.find(F) == nodes.end()) {
        nodes[F] = std::make_unique<CallGraphNode>(F);
    }
}

void CallGraphAnalysisResult::addCallEdge(Function* caller, Function* callee) {
    // 确保两个函数都有对应的节点
    addNode(caller);
    addNode(callee);
    
    // 添加调用边
    nodes[caller]->callees.insert(callee);
    nodes[callee]->callers.insert(caller);
    
    // 更新统计信息
    nodes[caller]->totalCallees = nodes[caller]->callees.size();
    nodes[callee]->totalCallers = nodes[callee]->callers.size();
    
    // 检查自递归
    if (caller == callee) {
        nodes[caller]->isSelfRecursive = true;
        nodes[caller]->isRecursive = true;
    }
}

void CallGraphAnalysisResult::computeTopologicalOrder() {
    topologicalOrder.clear();
    std::unordered_set<Function*> visited;
    
    // 对每个未访问的函数进行DFS
    for (const auto& pair : nodes) {
        Function* F = pair.first;
        if (visited.find(F) == visited.end()) {
            dfsTopological(F, visited, topologicalOrder);
        }
    }
    
    // 反转结果（因为我们在后序遍历中添加）
    std::reverse(topologicalOrder.begin(), topologicalOrder.end());
}

void CallGraphAnalysisResult::dfsTopological(Function* F, std::unordered_set<Function*>& visited, 
                                            std::vector<Function*>& result) {
    visited.insert(F);
    
    auto node = getNode(F);
    if (node) {
        // 先访问所有被调用的函数
        for (Function* callee : node->callees) {
            if (visited.find(callee) == visited.end()) {
                dfsTopological(callee, visited, result);
            }
        }
    }
    
    // 后序遍历：访问完所有子节点后添加当前节点
    result.push_back(F);
}

void CallGraphAnalysisResult::computeStronglyConnectedComponents() {
    tarjanSCC();
    
    // 为每个函数设置其所属的SCC
    functionToSCC.clear();
    for (size_t i = 0; i < sccs.size(); ++i) {
        for (Function* F : sccs[i]) {
            functionToSCC[F] = static_cast<int>(i);
        }
    }
}

void CallGraphAnalysisResult::tarjanSCC() {
    sccs.clear();
    
    std::vector<int> indices(nodes.size(), -1);
    std::vector<int> lowlinks(nodes.size(), -1);
    std::vector<Function*> stack;
    std::unordered_set<Function*> onStack;
    int index = 0;
    
    // 为函数分配索引
    std::map<Function*, int> functionIndex;
    int idx = 0;
    for (const auto& pair : nodes) {
        functionIndex[pair.first] = idx++;
    }
    
    // 对每个未访问的函数运行Tarjan算法
    for (const auto& pair : nodes) {
        Function* F = pair.first;
        int fIdx = functionIndex[F];
        if (indices[fIdx] == -1) {
            tarjanDFS(F, index, indices, lowlinks, stack, onStack);
        }
    }
}

void CallGraphAnalysisResult::tarjanDFS(Function* F, int& index, std::vector<int>& indices, 
                                      std::vector<int>& lowlinks, std::vector<Function*>& stack, 
                                      std::unordered_set<Function*>& onStack) {
    // 这里需要函数到索引的映射，简化实现
    // 在实际实现中应该维护一个全局的函数索引映射
    static std::map<Function*, int> functionIndex;
    static int nextIndex = 0;
    
    if (functionIndex.find(F) == functionIndex.end()) {
        functionIndex[F] = nextIndex++;
    }
    
    int fIdx = functionIndex[F];
    
    // 确保向量足够大
    if (fIdx >= static_cast<int>(indices.size())) {
        indices.resize(fIdx + 1, -1);
        lowlinks.resize(fIdx + 1, -1);
    }
    
    indices[fIdx] = index;
    lowlinks[fIdx] = index;
    index++;
    
    stack.push_back(F);
    onStack.insert(F);
    
    auto node = getNode(F);
    if (node) {
        for (Function* callee : node->callees) {
            int calleeIdx = functionIndex[callee];
            
            // 确保向量足够大
            if (calleeIdx >= static_cast<int>(indices.size())) {
                indices.resize(calleeIdx + 1, -1);
                lowlinks.resize(calleeIdx + 1, -1);
            }
            
            if (indices[calleeIdx] == -1) {
                // 递归访问
                tarjanDFS(callee, index, indices, lowlinks, stack, onStack);
                lowlinks[fIdx] = std::min(lowlinks[fIdx], lowlinks[calleeIdx]);
            } else if (onStack.find(callee) != onStack.end()) {
                // 后向边
                lowlinks[fIdx] = std::min(lowlinks[fIdx], indices[calleeIdx]);
            }
        }
    }
    
    // 如果F是SCC的根
    if (lowlinks[fIdx] == indices[fIdx]) {
        std::vector<Function*> scc;
        Function* w;
        do {
            w = stack.back();
            stack.pop_back();
            onStack.erase(w);
            scc.push_back(w);
        } while (w != F);
        
        sccs.push_back(std::move(scc));
    }
}

void CallGraphAnalysisResult::analyzeRecursion() {
    // 基于SCC分析递归
    for (const auto& scc : sccs) {
        if (scc.size() > 1) {
            // 多函数的SCC，标记为相互递归
            for (Function* F : scc) {
                auto* node = getMutableNode(F);
                if (node) {
                    node->isRecursive = true;
                    node->recursiveDepth = -1; // 相互递归，深度未定义
                }
            }
        } else if (scc.size() == 1) {
            // 单函数SCC，检查是否自递归
            Function* F = scc[0];
            auto* node = getMutableNode(F);
            if (node && node->callees.count(F) > 0) {
                node->isSelfRecursive = true;
                node->isRecursive = true;
                node->recursiveDepth = -1; // 简化：不计算递归深度
            }
        }
    }
}

// ========== CallGraphAnalysisPass 实现 ==========

bool CallGraphAnalysisPass::runOnModule(Module* M, AnalysisManager& AM) {
    if (DEBUG) {
        std::cout << "Running Call Graph Analysis on module\n";
    }
    
    // 创建分析结果
    CurrentResult = std::make_unique<CallGraphAnalysisResult>(M);
    
    // 执行主要分析步骤
    buildCallGraph(M);
    CurrentResult->computeTopologicalOrder();
    CurrentResult->computeStronglyConnectedComponents();
    CurrentResult->analyzeRecursion();
    
    if (DEBUG) {
        CurrentResult->print();
    }
    
    return false; // 分析遍不修改IR
}

void CallGraphAnalysisPass::buildCallGraph(Module* M) {
    // 1. 为所有函数创建节点（包括声明但未定义的函数）
    for (auto& pair : M->getFunctions()) {
        Function* F = pair.second.get();
        if (!isLibraryFunction(F) && !isIntrinsicFunction(F)) {
            CurrentResult->addNode(F);
        }
    }
    
    // 2. 扫描所有函数的调用关系
    for (auto& pair : M->getFunctions()) {
        Function* F = pair.second.get();
        if (!isLibraryFunction(F) && !isIntrinsicFunction(F)) {
            scanFunctionCalls(F);
        }
    }
}

void CallGraphAnalysisPass::scanFunctionCalls(Function* F) {
    // 遍历函数中的所有基本块和指令
    for (auto& BB : F->getBasicBlocks_NoRange()) {
        for (auto& I : BB->getInstructions()) {
            if (CallInst* call = dynamic_cast<CallInst*>(I.get())) {
                processCallInstruction(call, F);
            }
        }
    }
}

void CallGraphAnalysisPass::processCallInstruction(CallInst* call, Function* caller) {
    Function* callee = call->getCallee();
    
    if (!callee) {
        // 间接调用，无法静态确定目标函数
        return;
    }
    
    if (isLibraryFunction(callee) || isIntrinsicFunction(callee)) {
        // 跳过标准库函数和内置函数
        return;
    }
    
    // 添加调用边
    CurrentResult->addCallEdge(caller, callee);
    
    // 更新调用点统计
    auto* node = CurrentResult->getMutableNode(caller);
    if (node) {
        node->callSiteCount++;
    }
}

bool CallGraphAnalysisPass::isLibraryFunction(Function* F) const {
    std::string name = F->getName();
    
    // SysY标准库函数
    return name == "getint" || name == "getch" || name == "getfloat" ||
           name == "getarray" || name == "getfarray" ||
           name == "putint" || name == "putch" || name == "putfloat" ||
           name == "putarray" || name == "putfarray" ||
           name == "_sysy_starttime" || name == "_sysy_stoptime";
}

bool CallGraphAnalysisPass::isIntrinsicFunction(Function* F) const {
    std::string name = F->getName();
    
    // 编译器内置函数（后续可以增加某些内置函数）
    return name.substr(0, 5) == "llvm." || name.substr(0, 5) == "sysy.";
}

void CallGraphAnalysisPass::printStatistics() const {
    if (CurrentResult) {
        CurrentResult->print();
    }
}

} // namespace sysy
