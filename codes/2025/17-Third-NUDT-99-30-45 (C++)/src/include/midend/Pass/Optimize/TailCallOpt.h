#pragma once

#include "Pass.h"
#include "Dom.h"
#include "Loop.h"

namespace sysy {

/**
 * @class TailCallOpt
 * @brief 优化尾调用的中端优化通道。
 *
 * 该类实现了一个针对函数级别的尾调用优化的优化通道（OptimizationPass）。
 * 通过分析和转换 IR（中间表示），将可优化的尾调用转换为更高效的形式，
 * 以减少函数调用的开销，提升程序性能。
 *
 * @note 需要传入 IRBuilder 指针用于 IR 构建和修改。
 *
 * @method runOnFunction
 * 对指定函数进行尾调用优化。
 *
 * @method getPassID
 * 获取当前优化通道的唯一标识符。
 *
 * @method getAnalysisUsage
 * 指定该优化通道所依赖和失效的分析集合。
 */
class TailCallOpt : public OptimizationPass {
private:
  IRBuilder* builder;
public:
  TailCallOpt(IRBuilder* builder) : OptimizationPass("TailCallOpt", Granularity::Function), builder(builder) {}
  static void *ID;
  bool runOnFunction(Function *F, AnalysisManager &AM) override;
  void *getPassID() const override { return &ID; }
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override;
};

} // namespace sysy
