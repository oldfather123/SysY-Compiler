#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"

namespace middleend {

class Global2Local {
private:
    ir::Module *module_;
    std::vector<int> func_arg_temp;
    std::vector<Temp*> call_arg_temp;
    std::unordered_set<std::string> can_inline_func;
    std::unordered_map<int, ir::BasicBlock*> bb_map;
    std::unordered_map<int, Temp*> temp_map;
    std::vector<Temp*> next_bb_values;
    std::vector<ir::BasicBlock*> next_bb_bbs;
    void run();
    void mark_global_temp();
    Temp *new_temp(Temp *temp);
    void new_bb(ir::BasicBlock *bb, ir::BasicBlock *new_bb, ir::BasicBlock *next_bb);
public:
    bool has_new_alloca = false;
    Global2Local(ir::Module *module) : module_(module) { run(); }
};

}