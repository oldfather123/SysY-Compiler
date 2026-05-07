#ifndef __LOOP_UNROLLING_H__
#define __LOOP_UNROLLING_H__

#include "Loop.hpp"

class LoopUnrolling: public Optimization {
public:
    explicit LoopUnrolling(Module* m, unordered_map<Function*, LoopVecPtr>& loops): Optimization(m), loops(loops) {}
    void execute() override;

private:
    unordered_map<Function*, LoopVecPtr>& loops; // 函数->循环

    bool can_unroll(LoopPtr loop); // 检查循环是否可以展开
    void do_unroll(LoopPtr loop);
};

#endif // __LOOP_UNROLLING_H__