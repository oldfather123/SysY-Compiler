#ifndef __LICM_H__
#define __LICM_H__

#include "Loop.hpp"

// Loop Invariant Code Motion (LICM): 将循环不变量代码外提，减少循环内的计算量，提高性能
class LICM: public Optimization {
public:
    explicit LICM(Module* m, unordered_map<Function*, LoopVecPtr>& loops): Optimization(m), loops(loops) {}
    void execute() override;

private:
    unordered_map<Function*, LoopVecPtr>& loops; // 函数->循环
    void init_loop_invar(Function* func, LoopPtr loop);
};


#endif // __LICM_H__