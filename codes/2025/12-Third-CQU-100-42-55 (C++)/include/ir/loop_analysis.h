#ifndef LLVM_IR_LOOP_ANALYSIS_H
#define LLVM_IR_LOOP_ANALYSIS_H

#include "llvm_ir.h"
#include "cfg.h"
#include <set>
#include <vector>
#include <memory>
#include <optional>
#include <map>

namespace llvm_ir {

class Loop {
public:
    BasicBlock* header = nullptr;
    BasicBlock* preheader = nullptr; // 只有在运行ensurePreheaders后才有值
    std::set<BasicBlock*> blocks; // 包含header, latches, exitblocks
    std::set<BasicBlock*> exitBlocks;
    std::set<BasicBlock*> latches;
    std::set<BasicBlock*> exitingBlocks;
    Loop* parent = nullptr;
    std::vector<Loop*> children;
    
    Loop(BasicBlock* h) : header(h) {}
    BasicBlock* getHeader() const { return header; }
    BasicBlock* getPreheader() const { return preheader; }
    const std::set<BasicBlock*>& getBlocks() const { return blocks; }
    const std::set<BasicBlock*>& getExitBlocks() const { return exitBlocks; }
    const std::set<BasicBlock*>& getLatches() const { return latches; }
    const std::set<BasicBlock*>& getExitingNodes() const { return exitingBlocks; }
    
    void addBlock(BasicBlock* bb) { blocks.insert(bb); }
    void setPreheader(BasicBlock* bb) { preheader = bb; }
    void addLatch(BasicBlock* bb) { latches.insert(bb); }
    void addExitBlock(BasicBlock* bb) { exitBlocks.insert(bb); }
    void addExitingNode(BasicBlock* bb) { exitingBlocks.insert(bb); }
    
    bool contains(BasicBlock* bb) const { return blocks.count(bb); }
    bool contains(Instruction* inst) const { return inst && inst->parent && contains(inst->parent); }
    
    // 嵌套相关
    void setParent(Loop* p) { parent = p; }
    Loop* getParent() const { return parent; }
    void addChild(Loop* c) { children.push_back(c); }
    const std::vector<Loop*>& getChildren() const { return children; }
    
    void findLatches();
    void findExitNodes(Function& F);

    // loop规范化
    void SingleLatchInsert(Function& F);
    void ExitInsert(Function& F); // 确保exit被循环内块支配，也就是exit块所有的前驱都是循环内的块
    void LCSSA(Function& F);

    // debug
    void printLoopInfo();
};

class LoopAnalysis {
public:
    std::vector<std::unique_ptr<Loop>> loops;
    
    // 默认构造函数
    LoopAnalysis() = default;
    
    // 移动构造函数
    LoopAnalysis(LoopAnalysis&& other) noexcept = default;
    
    // 移动赋值运算符
    LoopAnalysis& operator=(LoopAnalysis&& other) noexcept = default;
    
    // 删除复制构造函数和复制赋值运算符
    LoopAnalysis(const LoopAnalysis&) = delete;
    LoopAnalysis& operator=(const LoopAnalysis&) = delete;
    
    void runOnFunction(Function& F, const cfg::DominatorTree& DT);
    void commentLoops();

    // 循环规范化方法
    void ensurePreheaders(Function& F, const cfg::DominatorTree& DT);
    void normalizeLoops(Function& F);
    void runLCSSA(Function& F);
    
    // 重新分析单个循环
    void reAnalysisLoop(Loop* loop, Function& F, const cfg::DominatorTree& DT);
};

static int s_lcssa_temp_phi_counter = 0;

} // namespace llvm_ir

#endif // LLVM_IR_LOOP_ANALYSIS_H 