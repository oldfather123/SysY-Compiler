#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"

namespace middleend {

class SplitCriticalEdges {
private:
    ir::Module *module_;
    std::unordered_map<ir::BasicBlock *, std::list<ir::instruction::Phi *>> phi_info;
    std::unordered_set<ir::BasicBlock *> incoming_bbs;
    std::list<std::pair<ir::BasicBlock *, ir::BasicBlock *>> new_bbs;
    
    void run();
    void split_critical_edges(ir::Function *func);
public:
    SplitCriticalEdges(ir::Module *module) : module_(module) { run(); }
};

}