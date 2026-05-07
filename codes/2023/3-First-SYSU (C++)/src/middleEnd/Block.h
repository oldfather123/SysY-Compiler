#pragma once
#include"Instruction.h"
#include "Variable.h"
#include"Label.h"
#include <unordered_map>
#include <iostream>
using namespace std;
struct BasicBlock;
struct Instruction;

struct Block
{
    int regNum;
    ValuePtr getReg(TypePtr type){return ValuePtr(new Reg(type, regNum++));}
    vector<shared_ptr<BasicBlock>> basicBlocks;
    std::unordered_map<string, VariablePtr> variables;

    unordered_map<LabelPtr, shared_ptr<BasicBlock>> LabelBBMap;
    void getLabelBBMap();

    Block();
    void pushBasicBlock(shared_ptr<BasicBlock> block);
    void pushVariable(VariablePtr variable);
    VariablePtr findVariable(string name);
    void allocLocalVariable();
    void solveEndInstruction();

    void print();
};
typedef shared_ptr<Block> BlockPtr;

struct BasicBlock
{
    LabelPtr label;
    vector<shared_ptr<Instruction>> instructions;
    shared_ptr<Instruction> endInstruction;

    BasicBlock(LabelPtr label=LabelPtr(new Label())): label{label} {};

    void pushInstruction(shared_ptr<Instruction> instruction);
    void pushEndInstruction(shared_ptr<Instruction> instruction);

    //用于后续优化阶段的标记
    bool isDeadBB=false;
    bool isWhileCond = false;
    bool isIfCond = false;

    int loopDepth = 0;

    //前继后继基本块
    unordered_map<shared_ptr<BasicBlock>, bool> predBasicBlocks;
    unordered_map<shared_ptr<BasicBlock>, bool> succBasicBlocks;
    
    //该基本块直接支配的子基本块
    unordered_map<shared_ptr<BasicBlock>, bool> dominatorSon;
    //支配边界
    unordered_map<shared_ptr<BasicBlock>, bool> DF;
     //用来存储直接支配该基本块的基本块
    shared_ptr<BasicBlock> directDominator = nullptr;

    void print();
};
typedef shared_ptr<BasicBlock> BasicBlockPtr;