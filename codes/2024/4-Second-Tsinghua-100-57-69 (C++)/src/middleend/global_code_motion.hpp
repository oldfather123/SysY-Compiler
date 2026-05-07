#pragma once

#include "middleend/dominator_tree.hpp"
#include "middleend/loop_analysis.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class GlobalCodeMotion {
private:
    CFG *cfg_;
    ReversePostOrder *rpo_;
    DominatorTree_ *dt_;
    LoopAnalysis *la_;
    UseDefAnalysis *uda_;
    
    std::unordered_map<ir::Instruction *, bool> visit;
    ir::BasicBlock *root;
    ir::Function *func;

    std::vector<ir::Instruction *> inst_list; // 因为需要移动指令的位置所以预先记录所有的指令

    void run();
    void schedule_early(ir::Instruction *inst);
    void schedule_late(ir::Instruction *inst);
    void clear_visit();
    bool is_pinned(ir::Instruction *inst);

public:
    ~GlobalCodeMotion() {
        delete rpo_;
        delete dt_;
        delete la_;
        delete uda_;
    }

    GlobalCodeMotion(CFG *cfg) : cfg_(cfg) {
        rpo_ = new ReversePostOrder(cfg_);
        dt_ = new DominatorTree_(cfg_);
        la_ = new LoopAnalysis(cfg_);
        uda_ = new UseDefAnalysis(cfg_);
        func = cfg_->get_func();
        root = func->get_entry();
        for (auto &bb : *func->get_basic_blocks()) {
            for (auto &inst : *bb->get_instructions()) {
                inst_list.push_back(inst);
                inst->set_parent(bb);
            }
        }
        run();
    }
};

} // namespace middleend