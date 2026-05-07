#pragma once

#include "PassManager.hpp"

// 简单的表达式恒等变换
class InstCombine final : public Pass<Function> {
public:
    ~InstCombine() override = default;
    void run(Function *func, AnalysisPassManager &APM) override;

private:
    bool try_optimize(Instruction *instr);
    std::vector<Instruction *> instr_to_delete_;
};