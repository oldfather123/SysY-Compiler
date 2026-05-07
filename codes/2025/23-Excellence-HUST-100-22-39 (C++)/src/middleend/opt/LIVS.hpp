#ifndef __LOOP_INDUCTION_VARIABLE_SIMPLIFICATION_H__
#define __LOOP_INDUCTION_VARIABLE_SIMPLIFICATION_H__

#include "Loop.hpp"

class LIVS: public Optimization {
public:
    explicit LIVS(Module* m, unordered_map<Function*, LoopVecPtr>& loops): Optimization(m), loops(loops) {}
    void execute() override;

private:
    unordered_map<Function*, LoopVecPtr>& loops; // 函数->循环

    // Induction Variable Strength Reduction: 归纳变量强度削弱
    bool indu_vars_strength_reduction(LoopPtr loop);
    // Basic Induction Variable Elimination: 基本归纳变量消除
    bool basic_indu_vars_elimination(LoopPtr loop);
};

#endif // __LOOP_INDUCTION_VARIABLE_SIMPLIFICATION_H__