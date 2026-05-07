#pragma once

#include "Block.h"
#include "Function.h"
#include "StringTable.h"
#include "Variable.h"
#include <memory>
#include <stack>

using std::move;
using std::stack;

struct Module {
    // use auto&
    vector<Variable*> globalVariables;
    vector<unique_ptr<Function>> globalFunctions;
    // use auto&
    // use auto&
    // vector<unique_ptr<Function>> globalFunctions;
    // use auto&
    unique_ptr<StringTableNode> globalStringTable;
    StringTableNode* currStringTable = nullptr;
    unique_ptr<StringTableNode> paramStringTable;
    TypePtr declType;
    stack<BasicBlockPtr> trueBlockStack;
    stack<BasicBlockPtr> falseBlockStack;
    stack<BasicBlockPtr> whileEndBlockStack;
    stack<BasicBlockPtr> whileCondBlockStack;
    bool shouldAddCacheLookup = false;

    Module();
    ~Module() = default;
    Module(const Module&) = delete;
    Module(Module&&) = delete;
    Module& operator=(const Module&) = delete;
    Module& operator=(Module&&) = delete;

    void pushFunction(std::unique_ptr<Function> function);
    void eraseFunction(Function* function);
    void pushVariable(Variable* globalVariable, string trueName);
    void pushBackStringTable();
    StringTableNodePtr popStringTable();
    FunctionPtr getFunction(string name);
    vector<Variable*> getGlobalVariables();
    vector<Function*> getGlobalFunctions();

    void dump(std::ostream& out = std::cout);
};
