#ifndef __DEAD_CODE_ELIMINATION_H__
#define __DEAD_CODE_ELIMINATION_H__

#include "opt.hpp"

class DeadCodeElimination: public Optimization {
private:
    // 是否存在副作用：影响控制流、改变内存
    bool has_side_effect(Instruction* inst);
    // 死代码判断：无副作用、无引用、无操作数的Phi指令
    bool is_dead_code(Instruction* inst);
    // 找到死代码
    vector<Instruction*> select_dead_code(Function* func);
public:
    explicit DeadCodeElimination(Module* m): Optimization(m) {}
    void execute() override;
};

#endif // __DEAD_CODE_ELIMINATION_H__