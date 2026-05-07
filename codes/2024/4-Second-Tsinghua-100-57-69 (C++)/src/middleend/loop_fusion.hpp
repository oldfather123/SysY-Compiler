#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/loop_analysis.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/use_def_analysis.hpp"
#include "middleend/copy_propagation.hpp"

namespace middleend {

struct LoopCond {
    bool is_const_bound;
    Temp* start;
    Temp* end;
    Temp* step;
    Temp* var;
    BinaryOp cmp;
    BinaryOp update;

    void print() {
        std::cout << is_const_bound << " " << start->to_str() << " " << end->to_str() << " " << step->to_str() << " " << var->to_str() << std::endl;
    }
};

class LoopFusion {
private:
    ir::Module *module_;
    ir::Function *func_;
    void run();
    void loop_fusion(ir::Function *func);
    LoopCond get_loop_cond(UseDefAnalysis *uda, Loop* loop, std::unordered_set<ir::BasicBlock *> loop_bb);
    bool check_common_var(UseDefAnalysis* uda, ir::BasicBlock* b1, ir::BasicBlock* b2, ir::BasicBlock* mid_bb);
    void fuse_loops(CFG* cfg, DominatorTree_* dom, UseDefAnalysis* uda, Loop *to, Loop *from, LoopCond to_cond, LoopCond from_cond);
    
public:
    LoopFusion(ir::Module *module) : module_(module), func_(nullptr) { run(); }
};

}