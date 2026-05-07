#ifndef __INSTRUCTION_COMBINATION_HPP__
#define __INSTRUCTION_COMBINATION_HPP__

#include "opt.hpp"

class InstCombine: public Optimization {
private:
    // 常量表达式的删除：a + 0 -> a, a * 1 -> a
    bool identity_simplification(Function* func);
    // 链式表达式的合并：a + 1 + 2 -> a + 3
    bool chain_combination(Function* func);
    bool strength_reduction(Function* func);
public:
    explicit InstCombine(Module* m) : Optimization(m) {}
    void execute() override;
};

#endif // __INSTRUCTION_COMBINATION_HPP__