#pragma once

#include "IR.h"
#include "IRBuilder.h"
#include "Pass.h"

namespace sysy {

// 优化前对SysY IR的预处理，也可以视作部分CFG优化
// 主要包括删除无用指令、合并基本块、删除空块等
// 这些操作可以在SysY IR生成时就完成，但为了简化IR生成过程，
// 这里将其放在SysY IR生成后进行预处理
// 同时兼容phi节点的处理，可以再mem2reg后再次调用优化

//TODO: 可增加的CFG优化和方法
// - 检查基本块跳转关系正确性
// - 简化条件分支（Branch Simplification），如条件恒真/恒假转为直接跳转
// - 合并连续的跳转指令（Jump Threading）在合并不可达块中似乎已经实现了
// - 基本块重排序（Block Reordering），提升局部性

// 辅助工具类，包含实际的CFG优化逻辑
// 这些方法可以被独立的Pass调用
class SysYCFGOptUtils {
public:
  static bool SysYDelInstAfterBr(Function *func); // 删除br后面的指令
  static bool SysYDelEmptyBlock(Function *func, IRBuilder* pBuilder); // 空块删除
  static bool SysYDelNoPreBLock(Function *func); // 删除无前驱块（不可达块）
  static bool SysYBlockMerge(Function *func);    // 合并基本块
  static bool SysYAddReturn(Function *func, IRBuilder* pBuilder);     // 添加return指令
  static bool SysYCondBr2Br(Function *func, IRBuilder* pBuilder);  // 条件分支转换为无条件分支
};

// ======================================================================
// 独立的CFG优化遍
// ======================================================================

class SysYDelInstAfterBrPass : public OptimizationPass {
public:
  static void *ID; // 唯一ID
  SysYDelInstAfterBrPass() : OptimizationPass("SysYDelInstAfterBrPass", Granularity::Function) {}
  bool runOnFunction(Function *F, AnalysisManager& AM) override;
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override {
    // 这个优化可能改变CFG结构，使一些CFG相关的分析失效
    // 可以在这里指定哪些分析会失效，例如支配树、活跃变量等
    // analysisInvalidations.insert(DominatorTreeAnalysisPass::ID); // 示例
  }
  void *getPassID() const override { return &ID; }
};

class SysYDelEmptyBlockPass : public OptimizationPass {
private:
  IRBuilder *pBuilder;
public:
  static void *ID;
  SysYDelEmptyBlockPass(IRBuilder *builder) : OptimizationPass("SysYDelEmptyBlockPass", Granularity::Function), pBuilder(builder) {}
  bool runOnFunction(Function *F, AnalysisManager& AM) override;
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override {};
  void *getPassID() const override { return &ID; }
};

class SysYDelNoPreBLockPass : public OptimizationPass {
public:
  static void *ID;
  SysYDelNoPreBLockPass() : OptimizationPass("SysYDelNoPreBLockPass", Granularity::Function) {}
  bool runOnFunction(Function *F, AnalysisManager& AM) override;
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override {};
  void *getPassID() const override { return &ID; }
};

class SysYBlockMergePass : public OptimizationPass {
public:
  static void *ID;
  SysYBlockMergePass() : OptimizationPass("SysYBlockMergePass", Granularity::Function) {}
  bool runOnFunction(Function *F, AnalysisManager& AM) override;
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override {};
  void *getPassID() const override { return &ID; }
};

class SysYAddReturnPass : public OptimizationPass {
private:
  IRBuilder *pBuilder;
public:
  static void *ID;
  SysYAddReturnPass(IRBuilder *builder) : OptimizationPass("SysYAddReturnPass", Granularity::Function), pBuilder(builder) {}
  bool runOnFunction(Function *F, AnalysisManager& AM) override;
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override {};
  void *getPassID() const override { return &ID; }
};

class SysYCondBr2BrPass : public OptimizationPass {
private:
  IRBuilder *pBuilder;
public:
  static void *ID;
  SysYCondBr2BrPass(IRBuilder *builder) : OptimizationPass("SysYCondBr2BrPass", Granularity::Function), pBuilder(builder) {}
  bool runOnFunction(Function *F, AnalysisManager& AM) override;
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override {};
  void *getPassID() const override { return &ID; }
};

}  // namespace sysy