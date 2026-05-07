#pragma once

#include "PassManager.hpp"
#include "Module.hpp"
#include "Function.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"
#include "funusedef.hpp"

#include <iostream>
#include <unordered_map>
#include <vector>

class FunctionInlineFindPass : public Pass {
    public:
        explicit FunctionInlineFindPass(Module *m);
    
        void run() override;
        void find_replace_calls();
        void replace_call();  
        void round_replace();
        void remove_useless_func();
    private:
        Module* m_;  // 你可以根据实际情况保存 Module 的指针
        std::unordered_map<Function*, int> line;
        std::unordered_map<Function*, std::vector<CallInst*>> function_calls;
        std::unordered_map<Function*, std::vector<CallInst*>> func_use;
        std::unordered_map<Function*, bool> is_simple_fun;
        std::vector<CallInst*> replace_calls;
        std::list<Argument> args;
        std::unordered_map<Value*, Value*> inst_map;
        std::unordered_map<BasicBlock *, BasicBlock *> bb_map;
    };