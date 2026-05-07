#pragma once

#include "FuncInfo.hpp"
#include "PassManager.hpp"

#include <Dominators.hpp>
#include <unordered_map>
#include <vector>

// 基于控制流分析，可用表达式分析的 CSE 实现
// 能够处理简单的 load-store
// TODO: alias analysis?
class CSE final : public Pass<Function> {
public:
    ~CSE() override = default;
    void run(Function *func, AnalysisPassManager &APM) override;

private:
    static bool trivial_common_expr(Instruction *instr1, Instruction *instr2);
    bool can_delete(Instruction *instr);

    // is a load killed by instr with side effected?
    bool may_kill(Instruction *instr, Instruction *load);
    static Value *get_base_addr(Value *val);

    // insert extra load for each store
    void insert_forward_load(Function *func);
    void remove_forward_load(Function *func);

    bool local_cse(BasicBlock *bb);
    bool global_cse(Function *func);

    std::unordered_map<LoadInst *, StoreInst *> forward_loads_;
    std::vector<Instruction *> inst_to_delete_;
    const FuncInfo *func_info_ = nullptr;
    const Dominators *dom_info_ = nullptr;
};