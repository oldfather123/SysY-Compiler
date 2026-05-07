#pragma once

#include<stack>
#include"Function.h"
#include"StringTable.h"

using std::stack;
using std::move;

struct Module
{
    vector<FunctionPtr> globalFunctions;
    vector<VariablePtr> globalVariables;
    stack<BlockPtr> blockStack;
    StringTableNodePtr globalStringTable;
    StringTableNodePtr currStringTable;
    StringTableNodePtr paramStringTable;
    TypePtr declType;
    stack<LabelPtr> trueLabelStack;
    stack<LabelPtr> falseLabelStack;
    stack<LabelPtr> whileEndStack;
    stack<LabelPtr> whileCondStack;
    Module();

    void pushFunction(FunctionPtr Function);
    void pushVariable(VariablePtr globalVariable);
    void pushBlock(BlockPtr block);
    BlockPtr popBlock();
    void pushBackStringTable();
    StringTableNodePtr popStringTable();
    BlockPtr getBlock();
    shared_ptr<BasicBlock> getBasicBlock();
    FunctionPtr getFunction(string name);

    void registerVariable(VariablePtr variable);
    bool blockStackEmpty();
    void print();
};
