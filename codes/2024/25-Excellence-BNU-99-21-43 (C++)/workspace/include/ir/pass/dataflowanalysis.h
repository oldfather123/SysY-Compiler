#pragma once

#include "IR.h"

namespace IR
{
    struct DataFlowAnalysis
    {
        std::map<BasicBlock *, std::set<Value *>> gen;
        std::map<BasicBlock *, std::set<Value *>> kill;
        std::map<BasicBlock *, std::map<Value *, std::set<Value *>>> in;
        std::map<BasicBlock *, std::map<Value *, std::set<Value *>>> out;
        std::map<Value *, std::set<Value *>> def;

        DataFlowAnalysis() = default;

        void runOnFunction(Function *func);
        void calculateInAndOut(Function *func);
    };
}