#pragma once

#include "Module.hpp"

#include <deque>
#include <unordered_map>

class PureFuncDect {
private:
    Module *m_;
    std::deque<Function *> worklist;
    std::unordered_map<Function *, bool> is_pure_func;

    void mark_pure_func(Function *func);
    void propagate_non_pure_func(Function *func);
    bool is_side_effect_inst(Instruction *inst);
    bool is_local_load(LoadInst *inst);
    bool is_local_store(StoreInst *inst);
    Value *get_first_addr(Value *val);

public:
    PureFuncDect(Module *m) : m_(m) {}
    void run();
    std::unordered_map<Function *, bool> &get_pure_func_map() {
        return is_pure_func;
    }
};


