#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "StringTable.h"
#include "Label.h"
#include "Loop.h"

struct Loop;
struct BasicBlock;
struct Instruction;

struct Function:public std::enable_shared_from_this<Function>
{
    ValuePtr returnValue;
    string name;
    bool isLibraryFunction;
    bool isReentrant;
    int registerCount;
    shared_ptr<BasicBlock> returnBasicBlock;
    vector<ValuePtr> formalArguments;

    unordered_map<shared_ptr<Function>,int> callerMap;
    unordered_map<shared_ptr<Function>,int> calleeMap;
    std::unordered_set<shared_ptr<Instruction>> callerInstructions;

    vector<shared_ptr<BasicBlock>> basicBlocks;
    std::unordered_map<string, VariablePtr> variableMap;
    unordered_map<LabelPtr, shared_ptr<BasicBlock>> LabelToBBMap;
    vector<shared_ptr<Loop>> loopList;
    unordered_map<shared_ptr<BasicBlock>, shared_ptr<Loop>> bbToLoopMap;
    vector<shared_ptr<Loop>> outerLoops;

    Function();
    Function(TypePtr returnType, string name, vector<ValuePtr> formalArguments);
    Function(TypePtr returnType, string name, bool isReenterable, vector<ValuePtr> formArguments=vector<ValuePtr>()) : returnValue{ValuePtr(new Reg(returnType, "retval"))}, name{name}, formalArguments{formArguments}, isLibraryFunction{true} , isReentrant{isReenterable} {};

    void initializeReturnBlock();
    void processReturnBlock();

    string returnTypeToString() { return returnValue->type->toString() + " @" + name; }
    void print();
    void clearCallerAndCalleeInfo();
    void calculateCallerAndCalleeInfo();

    ValuePtr createRegister(TypePtr type){return ValuePtr(new Reg(type, registerCount++));}

    void initializeLabelToBlockMap();

    vector<shared_ptr<Loop>> getLoopsInPostorder();
    shared_ptr<Loop> getLoopOfBB(shared_ptr<BasicBlock> bb) { return bbToLoopMap.count(bb)? bbToLoopMap[bb]: nullptr; }
    void clearBBToLoopMap();
    void clearLoopList();

    void assignBlocksToFunction();
    void addBasicBlock(shared_ptr<BasicBlock> block);
    void addGlobalVariable(VariablePtr variable);
    VariablePtr findVariable(string name);

    void allocateLocalVariables();

    void processEndInstructions();
    shared_ptr<BasicBlock> getEntryBlock();
};
typedef shared_ptr<Function> FunctionPtr;