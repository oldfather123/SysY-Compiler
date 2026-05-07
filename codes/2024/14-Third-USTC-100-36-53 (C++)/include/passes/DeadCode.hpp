#pragma once

#include "FuncInfo.hpp"
#include "Function.hpp"
#include "PassManager.hpp"

#include <unordered_set>

/**
 * 死代码消除：参见
 *https://www.clear.rice.edu/comp512/Lectures/10Dead-Clean-SCCP.pdf
 **/
class DeadCode : public Pass<Module>{
  public:

    void run(Module *module, AnalysisPassManager &APM) override;

  private:
    // std::shared_ptr<FuncInfo> func_info;
    int ins_count{0}; // 用以衡量死代码消除的性能
    std::deque<Instruction *> work_list{};
    std::unordered_map<Instruction *, bool> marked{};

    void mark(Function *func, const FuncInfo *func_info);
    void mark(Instruction *ins);
    bool sweep(Function *func);
    bool clear_basic_blocks(Function *func);
    bool is_critical(Instruction *ins, const FuncInfo *func_info);

    // clear write only alloca
    // better algorithm?
    bool clear_dead_store(Function *func);
};
