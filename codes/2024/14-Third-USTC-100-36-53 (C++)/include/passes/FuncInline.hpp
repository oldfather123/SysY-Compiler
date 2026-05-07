#pragma once

#include "FuncInfo.hpp"
#include "Module.hpp"
#include "PassManager.hpp"

class FuncInline : public Pass<Module> {
  public:
    ~FuncInline() = default;

    void run(Module *module, AnalysisPassManager &APM) override;

  private:
    // std::unique_ptr<FuncInfo> func_info_;
    int inlinecnt;
    bool should_inline(Function *func, const FuncInfo *func_info_);

    void inline_func(Function *caller, BasicBlock *parent_bb,
                     Instruction *inst, Module *m_);
};
