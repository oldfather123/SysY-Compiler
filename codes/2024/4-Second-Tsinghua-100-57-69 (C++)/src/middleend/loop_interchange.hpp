#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/reverse_postorder.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/loop_analysis.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class LoopInterchange {
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
    void loop_interchange(TightlyNestedLoopInfo &nested_info);
    InterchangeLoopInfo get_interchange_info(Loop *loop);

public:
    ~LoopInterchange() {
        delete rpo_;
        delete dt_;
        delete la_;
        delete uda_;
    }

    LoopInterchange(ir::Function *func_) : func(func_) {
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