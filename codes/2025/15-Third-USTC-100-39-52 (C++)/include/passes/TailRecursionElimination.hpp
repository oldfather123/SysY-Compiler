#ifndef TAILRECURSIONELIMINATION_HPP
#define TAILRECURSIONELIMINATION_HPP

#include "PassManager.hpp"
#include "Module.hpp"
#include "Function.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"
#include "IRBuilder.hpp"
#include "logging.hpp"
#include "ilist.hpp"
class TailRecursionElimination : public Pass {
public:
    TailRecursionElimination(Module *m) : Pass(m) {}
    void run() override;

private:
    bool eliminate_tail_recursion(Function *f);
};

// 前插
BasicBlock *insert_before_blockx(BasicBlock *orig_bb, Module *m);

BasicBlock *insert_after_blockx(BasicBlock *orig_bb, Module *m);
void move_instrsx(BasicBlock *insert_block,BasicBlock *inst_block,
    std::vector<Instruction *> to_move,Instruction *insert_before_inst);




#endif
