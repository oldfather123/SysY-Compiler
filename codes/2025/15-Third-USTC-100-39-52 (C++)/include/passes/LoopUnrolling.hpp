#ifndef LOOPUNROLLING_HPP
#define LOOPUNROLLING_HPP

#include "PassManager.hpp"
#include "Module.hpp"
#include "Function.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"
#include "IRBuilder.hpp"
#include "logging.hpp"
#include "ilist.hpp"
#include "Looplook.hpp"
class  LoopUnrolling : public Pass {
public:
    LoopUnrolling(Module *m) : Pass(m) {}
    void run() override;

private:
    bool Loop_unrolling(Module* m);
};

// 前插
BasicBlock *insert_before_blockz(BasicBlock *orig_bb, Module *m);

BasicBlock *insert_after_blockz(BasicBlock *orig_bb, Module *m);
void move_instrsz(BasicBlock *insert_block,BasicBlock *inst_block,
    std::vector<Instruction *> to_move,Instruction *insert_before_inst);

void replace_phi_predecessor(BasicBlock* bb1, BasicBlock* bb2);


#endif