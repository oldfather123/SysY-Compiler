#pragma once

#include "Block.h"
#include "StringTable.h"

struct Block;
struct BasicBlock;
struct Function
{
    ValuePtr retVal;
    shared_ptr<BasicBlock> returnBasicBlock;
    string name;
    vector<ValuePtr> formArguments;
    shared_ptr<Block> block;
    bool isLib;
    bool isReenterable;

    // 定义
    Function(TypePtr returnType, string name, vector<ValuePtr> formArguments, shared_ptr<Block> block);
    Function(TypePtr returnType, string name, bool isReenterable, vector<ValuePtr> formArguments=vector<ValuePtr>()) : retVal{ValuePtr(new Reg(returnType, "retval"))}, name{name}, formArguments{formArguments}, block{nullptr}, isLib{true} , isReenterable{isReenterable} {};
    void solveReturnBasicBlock();

    string getTypeStr() { return retVal->type->getStr() + " @" + name; }
    void print();
};
typedef shared_ptr<Function> FunctionPtr;