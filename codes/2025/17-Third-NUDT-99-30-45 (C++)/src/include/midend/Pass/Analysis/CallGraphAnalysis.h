#pragma once

#include "IR.h"
#include "Pass.h"
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_set>

namespace sysy {

// 前向声明
class CallGraphAnalysisResult;

/**
 * @brief 调用图节点信息
 * 存储单个函数在调用图中的信息
 */
struct CallGraphNode {
    Function* function;                    // 关联的函数
    std::set<Function*> callers;           // 调用此函数的函数集合
    std::set<Function*> callees;           // 此函数调用的函数集合
    
    // 递归信息
    bool isRecursive;                      // 是否参与递归调用
    bool isSelfRecursive;                  // 是否自递归
    int recursiveDepth;                    // 递归深度(-1表示无限递归)
    
    // 调用统计
    size_t totalCallers;                   // 调用者总数
    size_t totalCallees;                   // 被调用函数总数
    size_t callSiteCount;                  // 调用点总数
    
    CallGraphNode(Function* f) : function(f), isRecursive(false), 
        isSelfRecursive(false), recursiveDepth(0), totalCallers(0), 
        totalCallees(0), callSiteCount(0) {}
};

/**
 * @brief 调用图分析结果类
 * 包含整个模块的调用图信息和查询接口
 */
class CallGraphAnalysisResult : public AnalysisResultBase {
public:
    CallGraphAnalysisResult(Module* M) : AssociatedModule(M) {}
    ~CallGraphAnalysisResult() override = default;

    // ========== 基础查询接口 ==========
    
    /**
     * 获取函数的调用图节点
     */
    const CallGraphNode* getNode(Function* F) const {
        auto it = nodes.find(F);
        return (it != nodes.end()) ? it->second.get() : nullptr;
    }
    
    /**
     * 获取函数的调用图节点（非const版本）
     */
    CallGraphNode* getMutableNode(Function* F) {
        auto it = nodes.find(F);
        return (it != nodes.end()) ? it->second.get() : nullptr;
    }
    
    /**
     * 获取所有函数节点
     */
    const std::map<Function*, std::unique_ptr<CallGraphNode>>& getAllNodes() const {
        return nodes;
    }
    
    /**
     * 检查函数是否存在于调用图中
     */
    bool hasFunction(Function* F) const {
        return nodes.find(F) != nodes.end();
    }

    // ========== 调用关系查询 ==========
    
    /**
     * 检查是否存在从caller到callee的调用
     */
    bool hasCallEdge(Function* caller, Function* callee) const {
        auto node = getNode(caller);
        return node && node->callees.count(callee) > 0;
    }
    
    /**
     * 获取函数的所有调用者
     */
    std::vector<Function*> getCallers(Function* F) const {
        auto node = getNode(F);
        if (!node) return {};
        return std::vector<Function*>(node->callers.begin(), node->callers.end());
    }
    
    /**
     * 获取函数的所有被调用函数
     */
    std::vector<Function*> getCallees(Function* F) const {
        auto node = getNode(F);
        if (!node) return {};
        return std::vector<Function*>(node->callees.begin(), node->callees.end());
    }

    // ========== 递归分析查询 ==========
    
    /**
     * 检查函数是否参与递归调用
     */
    bool isRecursive(Function* F) const {
        auto node = getNode(F);
        return node && node->isRecursive;
    }
    
    /**
     * 检查函数是否自递归
     */
    bool isSelfRecursive(Function* F) const {
        auto node = getNode(F);
        return node && node->isSelfRecursive;
    }
    
    /**
     * 获取递归深度
     */
    int getRecursiveDepth(Function* F) const {
        auto node = getNode(F);
        return node ? node->recursiveDepth : 0;
    }

    // ========== 拓扑排序和SCC ==========
    
    /**
     * 获取函数的拓扑排序结果
     * 保证被调用函数在调用函数之前
     */
    const std::vector<Function*>& getTopologicalOrder() const {
        return topologicalOrder;
    }
    
    /**
     * 获取强连通分量列表
     * 每个SCC表示一个递归函数群
     */
    const std::vector<std::vector<Function*>>& getStronglyConnectedComponents() const {
        return sccs;
    }
    
    /**
     * 获取函数所在的SCC索引
     */
    int getSCCIndex(Function* F) const {
        auto it = functionToSCC.find(F);
        return (it != functionToSCC.end()) ? it->second : -1;
    }

    // ========== 统计信息 ==========
    
    struct Statistics {
        size_t totalFunctions;
        size_t totalCallEdges;
        size_t recursiveFunctions;
        size_t selfRecursiveFunctions;
        size_t stronglyConnectedComponents;
        size_t maxSCCSize;
        double avgCallersPerFunction;
        double avgCalleesPerFunction;
    };
    
    Statistics getStatistics() const;
    
    /**
     * 打印调用图分析结果
     */
    void print() const;

    // ========== 内部构建接口 ==========
    
    void addNode(Function* F);
    void addCallEdge(Function* caller, Function* callee);
    void computeTopologicalOrder();
    void computeStronglyConnectedComponents();
    void analyzeRecursion();

private:
    Module* AssociatedModule;                                      // 关联的模块
    std::map<Function*, std::unique_ptr<CallGraphNode>> nodes;     // 调用图节点
    std::vector<Function*> topologicalOrder;                      // 拓扑排序结果
    std::vector<std::vector<Function*>> sccs;                     // 强连通分量
    std::map<Function*, int> functionToSCC;                       // 函数到SCC的映射
    
    // 内部辅助方法
    void dfsTopological(Function* F, std::unordered_set<Function*>& visited, 
                       std::vector<Function*>& result);
    void tarjanSCC();
    void tarjanDFS(Function* F, int& index, std::vector<int>& indices, 
                  std::vector<int>& lowlinks, std::vector<Function*>& stack, 
                  std::unordered_set<Function*>& onStack);
};

/**
 * @brief SysY调用图分析Pass
 * Module级别的分析Pass，构建整个模块的函数调用图
 */
class CallGraphAnalysisPass : public AnalysisPass {
public:
    // 唯一的 Pass ID
    static void* ID;

    CallGraphAnalysisPass() : AnalysisPass("CallGraphAnalysis", Pass::Granularity::Module) {}

    // 实现 getPassID
    void* getPassID() const override { return &ID; }

    // 核心运行方法
    bool runOnModule(Module* M, AnalysisManager& AM) override;

    // 获取分析结果
    std::unique_ptr<AnalysisResultBase> getResult() override { return std::move(CurrentResult); }

private:
    std::unique_ptr<CallGraphAnalysisResult> CurrentResult;  // 当前模块的分析结果
    
    // ========== 主要分析流程 ==========
    
    void buildCallGraph(Module* M);                          // 构建调用图
    void scanFunctionCalls(Function* F);                     // 扫描函数的调用
    void processCallInstruction(CallInst* call, Function* caller);  // 处理调用指令
    
    // ========== 辅助方法 ==========
    
    bool isLibraryFunction(Function* F) const;               // 判断是否为标准库函数
    bool isIntrinsicFunction(Function* F) const;             // 判断是否为内置函数
    void printStatistics() const;                            // 打印统计信息
};

} // namespace sysy
