#pragma once

#include "PassManager.hpp"

// 将尾递归转化为循环，实际例子参考 h_performance/h-1
// 通过将 call+ret 转化为 br 实现，目前仅能够处理 call 和 ret 相邻的情况
// 详见 TailRecurEliminate.cpp
class TailRecurEliminate : public Pass<Function> {
  public:
    ~TailRecurEliminate() final = default;
    void run(Function *func, AnalysisPassManager &APM) override;
};
