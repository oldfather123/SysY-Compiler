#pragma once

#include<stack>
#include"Function.h"
#include"StringTable.h"

using std::stack;

struct Module
{
    vector<FunctionPtr> globalFunctions;
    vector<VariablePtr> globalVariables;
    stack<FunctionPtr> functionStack;
    StringTableNodePtr globalStringTable;
    StringTableNodePtr currentScopeStringTable;
    StringTableNodePtr parameterStringTable;
    TypePtr declaredType;
    stack<LabelPtr> trueLabelStack;
    stack<LabelPtr> falseLabelStack;
    stack<LabelPtr> loopEndLabelStack;
    stack<LabelPtr> loopConditionLabelStack;

    Module();

    void addFunction(FunctionPtr Function);
    void addGlobalVariable(VariablePtr globalVariable);
    void pushFunctionToStack(FunctionPtr func);

    FunctionPtr popFunctionFromStack();
    void restorePreviousStringTable();
    StringTableNodePtr popStringTable();
    FunctionPtr getCurrentFunction();

    shared_ptr<BasicBlock> getCurrentBasicBlock();
    FunctionPtr findFunctionByName(string name);

    void recalculateCallerAndCallee();

    void registerGlobalVariable(VariablePtr variable);
    bool isFunctionStackEmpty();
    void print();
};