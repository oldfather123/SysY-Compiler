#pragma once
#include <unordered_set>
#include <iostream>
#include <memory>
#include <functional>
#include "Instruction.h"
#include "Label.h"
#include <algorithm>
#include <map>

using namespace std;

struct Instruction;
struct Function;
struct Loop;

struct BasicBlock
{
    LabelPtr label;
    vector<shared_ptr<Instruction>> instructions;
    map<Instruction*, int> instructionOrder;
    shared_ptr<Instruction> terminator= nullptr;
    shared_ptr<Function> parentFunction;

    float propagatedFloatValue = 0.0;
    int propagatedIntValue = 0;

    bool isEntryBlock = false;
    bool isPreHeader = false;
    int loopDepth = 0;
    shared_ptr<Loop> loop = nullptr;

    unordered_set<shared_ptr<BasicBlock>> predBasicBlocks;
    unordered_set<shared_ptr<BasicBlock>> succBasicBlocks;
    
    unordered_set<shared_ptr<BasicBlock>> dominatedBlocks;
    unordered_set<shared_ptr<BasicBlock>> dominanceFrontier;
    shared_ptr<BasicBlock> immediateDominator = nullptr;
    unordered_set<shared_ptr<BasicBlock>> allDominators;

    BasicBlock(LabelPtr label = LabelPtr(new Label())): label{label} ,parentFunction{nullptr}{};

    void appendInstruction(shared_ptr<Instruction> instruction);
    void appendInstruction(vector<shared_ptr<Instruction>> instructionsToInsert);
    void insertInstructionBefore(shared_ptr<Instruction> instruction, shared_ptr<Instruction> before);
    void setTerminator(shared_ptr<Instruction> instruction);
    void prependInstructions(vector<shared_ptr<Instruction>> instructionsToInsert);
    void removeInstruction(shared_ptr<Instruction> instruction);

    void addPredecessor(shared_ptr<BasicBlock> bb)  { predBasicBlocks.insert(bb); }
    void addSuccessor(shared_ptr<BasicBlock> bb)    { succBasicBlocks.insert(bb); }
    void removePredecessor(shared_ptr<BasicBlock> bb)   { predBasicBlocks.erase(bb); }
    void removeSuccessor(shared_ptr<BasicBlock> bb) { succBasicBlocks.erase(bb); }
    void print();

};
typedef shared_ptr<BasicBlock> BasicBlockPtr;
