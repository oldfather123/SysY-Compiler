#pragma once

#include "IR.h"
#include "IRBuilder.h" // 你的 IR Builder
#include "Liveness.h"
#include "Dom.h"
#include "Pass.h"      // 你的 Pass 框架基类
#include <iostream>    // 调试用
#include <map>         // 用于 Value 到 AllocaInst 的映射
#include <set>         // 可能用于其他辅助集合
#include <string>
#include <vector>

namespace sysy {

class Reg2MemContext {
public:
  Reg2MemContext(IRBuilder *b) : builder(b) {}

  // 运行 Reg2Mem 优化
  void run(Function *func);

private:
  IRBuilder *builder; // IR 构建器

  // 存储 SSA Value 到对应的 AllocaInst 的映射
  // 只有那些需要被"溢出"到内存的 SSA 值才会被记录在这里
  std::map<Value *, AllocaInst *> valueToAllocaMap;

  // 辅助函数：
  // 1. 识别并为 SSA Value 分配 AllocaInst
  void allocateMemoryForSSAValues(Function *func);

  // 2. 将 SSA 值的使用替换为 Load/Store
  void insertLoadsAndStores(Function *func);

  // 3. 处理 Phi 指令，将其转换为 Load/Store
  void rewritePhis(Function *func);

  // 4. 清理 (例如，可能删除不再需要的 Phi 指令)
  void cleanup(Function *func);

  // 判断一个 Value 是否是 AllocaInst 可以为其分配内存的目标
  // 通常指非指针类型的Instruction结果和Argument
  bool isPromotableToMemory(Value *val);
};

class Reg2Mem : public OptimizationPass {
private:
  IRBuilder *builder; ///< IR构建器，用于插入指令
public:
  static void *ID; ///< Pass的唯一标识符
  Reg2Mem(IRBuilder* builder) : OptimizationPass("Reg2Mem", Pass::Granularity::Function), builder(builder) {}
  bool runOnFunction(Function *F, AnalysisManager &AM) override;
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override;
  void *getPassID() const override { return &ID; } ///< 获取 Pass ID
};

} // namespace sysy