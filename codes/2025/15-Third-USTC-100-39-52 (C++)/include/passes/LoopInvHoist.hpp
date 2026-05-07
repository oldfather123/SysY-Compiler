#pragma once

#include "BasicBlock.hpp"
#include "Function.hpp"
#include "LoopSearch.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "Dominators.hpp"

#include <unordered_map>
#include <unordered_set>
class LoopInvHoist : public Pass {
  public:
    LoopInvHoist(Module *m) : Pass(m) {}

    void run() override;

  private:
    std::unordered_map<Value *, bool> info_;

    using LoopTree = std::unordered_map<BBset_t *, std::unordered_set<BBset_t *>>;

    void hoist(BBset_t *loop, LoopTree &loop_tree, LoopSearch &loop_searcher, BBset_t &vis);
    bool is_inv(Value *value, BBset_t *loop);
    bool is_inv(Value *value, BBset_t *loop, std::set<Value*> &visited);
    bool is_self_increment(Instruction *instr, BBset_t *loop);
    bool is_modified_in_loop(Value *addr, BBset_t *loop); 
    bool can_move(Instruction *instr, BBset_t *loop);
    bool function_writes_to_global(Function *f, GlobalVariable *gv);
    bool function_writes_to_global(Function *f, GlobalVariable *gv,std::set<Function *> &visited);
};
std::map<BasicBlock*, BBset_t> build_loop_dom_tree(Dominators &DT, const BBset_t &loopBlocks);
BasicBlock *find_loop_header(BBset_t *loop);