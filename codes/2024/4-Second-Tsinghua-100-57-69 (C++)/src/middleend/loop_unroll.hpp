#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/reverse_postorder.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/loop_analysis.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class LoopUnroll {
private:
    std::unordered_map<int, ConstValue> constant_map; // temp -> constant value
    CFG *cfg_;
    ReversePostOrder *rpo_;
    DominatorTree_ *dt_;
    LoopAnalysis *la_;
    UseDefAnalysis *uda_;
    ir::BasicBlock *root;
    ir::Function *func;
    void run();
    SimpleLoopInfo get_loop_info(Loop *loop, const std::unordered_set<ir::BasicBlock *> &loop_bbs, ir::BasicBlock *exit);
    void loop_unroll(Loop *loop, SimpleLoopInfo info, const std::unordered_set<ir::BasicBlock *> &loop_bbs, const int unroll_cnt);
    void copy_bb(Loop* loop, SimpleLoopInfo& info, bool last_turn, ir::BasicBlock *bb, ir::BasicBlock *new_bb, ir::BasicBlock *entry, ir::BasicBlock *exit);
    ir::Instruction* copy_inst(Loop* loop, SimpleLoopInfo& info, ir::Instruction *inst, ir::BasicBlock *entry, ir::BasicBlock *exit, bool is_last = false, ir::BasicBlock* for_br = nullptr, bool is_br = false); 
    void copy_bb_last(SimpleLoopInfo& info, ir::BasicBlock *bb, ir::BasicBlock *new_bb, ir::BasicBlock *entry, ir::BasicBlock *exit, ir::BasicBlock* br);
    void copy_bb_br(SimpleLoopInfo& info, ir::BasicBlock *bb, ir::BasicBlock *new_bb, ir::BasicBlock *entry, ir::BasicBlock *exit, ir::BasicBlock* br);
   
public:
    ~LoopUnroll() {
        delete rpo_;
        delete dt_;
        delete la_;
        delete uda_;
    }

    bool success = false;

    LoopUnroll(ir::Function *func_) : func(func_) {
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
