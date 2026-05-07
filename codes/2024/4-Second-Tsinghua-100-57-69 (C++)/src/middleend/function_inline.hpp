#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"

namespace middleend {

class FunctionInline {
private:
    ir::Module *module_;
    ir::Function *inline_func;
    std::vector<int> func_arg_temp;
    std::vector<Temp*> call_arg_temp;
    std::unordered_set<std::string> can_inline_func;
    std::unordered_map<int, ir::BasicBlock*> bb_map;
    std::unordered_map<int, Temp*> temp_map;
    std::vector<Temp*> next_bb_values;
    std::vector<ir::BasicBlock*> next_bb_bbs;
    void run();
    Temp *new_temp(Temp *temp, int now_temp_used);
    void new_bb(ir::BasicBlock *bb, ir::BasicBlock *new_bb, ir::BasicBlock *next_bb, int now_temp_used);
    void inline_function(ir::Function *now_func);
    void recursive_inline();
public:
    FunctionInline(ir::Module *module) : module_(module) { run(); }
};

}