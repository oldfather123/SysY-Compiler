#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/reverse_postorder.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/loop_analysis.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class LoopParallel {
private:
    std::unordered_map<int, ConstValue> constant_map; // temp -> constant value
    CFG *cfg_;
    ReversePostOrder *rpo_;
    DominatorTree_ *dt_;
    LoopAnalysis *la_;
    UseDefAnalysis *uda_;
    ir::BasicBlock *root;
    ir::Function *func;
    ir::Module* mod;
    ir::Function* create_threads;
    ir::Function* join_threads;
    void run();
    ParallelLoopInfo get_parallel_info(Loop *loop, const std::unordered_set<BasicBlock *> &loop_bbs, BasicBlock *exit);
    void loop_parallel(ParallelLoopInfo &info, Loop *loop, const std::unordered_set<BasicBlock *> &loop_bbs);
    void copy_bb(ParallelLoopInfo info, BasicBlock *bb, BasicBlock *new_bb, const std::unordered_map<BasicBlock *, BasicBlock *> &bb_map, const std::unordered_map<Temp*, Temp*> &reg_map);
    ir::Instruction *copy_inst(ParallelLoopInfo info, ir::Instruction *inst, const std::unordered_map<BasicBlock *, BasicBlock *> &bb_map, const std::unordered_map<Temp*, Temp*> &reg_map);

public:
    ~LoopParallel() {
        delete rpo_;
        delete dt_;
        delete la_;
        delete uda_;
    }
    bool success = false;
    int cnt = 0;

    LoopParallel(ir::Function *func_, ir::Module* mod_) : func(func_), mod(mod_) {
        create_threads = new ir::Function(mod_, Type(Void), {}, "__create_threads");
        join_threads = new ir::Function(mod_, Type(Void), {}, "__join_threads");
        cfg_ = new CFG(func);
        rpo_ = new ReversePostOrder(cfg_);
        dt_ = new DominatorTree_(cfg_);
        la_ = new LoopAnalysis(cfg_);
        uda_ = new UseDefAnalysis(cfg_);
        func = cfg_->get_func();
        root = func->get_entry();
        run(); 
    }
};

}
