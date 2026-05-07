#pragma once

#include "PassManager.hpp"
#include "Module.hpp"
#include "Function.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"

class ComputeSimplify : public Pass {
  public:
    ComputeSimplify(Module *m) : Pass(m) {}
    ~ComputeSimplify() = default;
    void run() override;
    void add_simplify(Instruction* inst,BasicBlock* putinstbb);
    void sub_simplify(Instruction* inst,BasicBlock* putinstbb);
    void mul_simplify(Instruction* inst,BasicBlock* putinstbb);
    void div_simplify(Instruction* inst,BasicBlock* putinstbb);
    void srem_simplify(Instruction* inst,BasicBlock* putinstbb);

    std::vector<Instruction *> to_move;
};
