#pragma once

#include "Dominators.hpp"
#include "Instruction.hpp"
#include "Value.hpp"

#include <map>
#include <memory>
#include <stack>

class Mem2Reg : public Pass<Function>{
  private:
    std::map<Value *, std::stack<Value *>> var_stack_;
    std::map<PhiInst *, Value *>phi_map_;

  public:
    ~Mem2Reg() = default;

    void run(Function *f, AnalysisPassManager &APM) override;

    void generate_phi(Function *f, const Dominators *dominators_);
    void rename(BasicBlock *bb, const Dominators *dominators_);

    static inline bool is_global_variable(Value *l_val) {
        return dynamic_cast<GlobalVariable *>(l_val) != nullptr;
    }
    static inline bool is_gep_instr(Value *l_val) {
        return dynamic_cast<GetElementPtrInst *>(l_val) != nullptr;
    }

    static inline bool is_valid_ptr(Value *l_val) {
        return not is_global_variable(l_val) and not is_gep_instr(l_val);
    }
};
