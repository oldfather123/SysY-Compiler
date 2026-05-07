#pragma once

#include <set>
#include <vector>
#include "Instruction.h"
#include "Block.h"
#include "utils.h"
using namespace std;

struct BasicBlock;
struct Function;
struct Instruction;
struct PhiInstruction;
struct BinaryInstruction;
struct Value;

class SCEV  
{
private:
    vector<ValuePtr> scevValues;
    set<BinaryInstruction*> calculatedInstructions;
public:
    SCEV() {};
    SCEV(ValuePtr initial, ValuePtr step);
    SCEV(ValuePtr initial, const SCEV& innerSCEV);
    SCEV(const vector<ValuePtr>& vec);
    SCEV(ValuePtr initial, ValuePtr step, std::set<BinaryInstruction*>&& binarys);

    friend SCEV operator+(ValuePtr lhs, const SCEV& rhs);
    friend SCEV operator+(const SCEV& lhs, const SCEV& rhs);

    friend SCEV operator-(const SCEV& lhs, ValuePtr rhs);
    friend SCEV operator-(const SCEV& lhs, const SCEV& rhs);

    friend SCEV operator*(ValuePtr lhs, const SCEV& rhs);

    ValuePtr& at(unsigned index) { return scevValues.at(index); }
    size_t size() { return scevValues.size(); }
    ValuePtr at(unsigned index) const { return scevValues.at(index); }
    size_t size() const { return scevValues.size(); }

    void clear();
};
typedef shared_ptr<SCEV> SCEVPtr;


struct Loop
{
    int id;
    int loopDepth;

    PhiInstruction* inductionPhi = nullptr;
    shared_ptr<Instruction> inductionCondition;
    shared_ptr<Value> inductionEnd;
    IcmpKind icmpKind;
    int iterationCount;

    set<shared_ptr<BasicBlock>> basicBlockSet;
    vector<shared_ptr<BasicBlock>> basicBlocks;
    shared_ptr<BasicBlock> header = nullptr;
    shared_ptr<BasicBlock> preHeader = nullptr;
    shared_ptr<BasicBlock> latchBlock = nullptr;

    shared_ptr<Loop> parentLoop = nullptr;
    vector<shared_ptr<Loop>> subLoops;
    unordered_set<shared_ptr<BasicBlock>> exitingBlocks; 
    unordered_set<shared_ptr<BasicBlock>> exitBlocks;
    shared_ptr<Function> parentFunction = nullptr;

    shared_ptr<Instruction> indexBinary = nullptr;
    Instruction* indexPhi = nullptr;
    bool hasPartiallyUnroll = false;
    int step = 0;
    shared_ptr<Value> loopEnd = nullptr;
    shared_ptr<Loop> friendLoop = nullptr;

    unordered_map<Instruction*, SCEV> scevMap;

    Loop() {}
    Loop(shared_ptr<BasicBlock> header, int id);

    bool contains(shared_ptr<BasicBlock> bb) { return basicBlockSet.count(bb); }
    void addBasicBlock(shared_ptr<BasicBlock> bb)        { basicBlocks.push_back(bb); basicBlockSet.insert(bb);}

    SCEV& getSCEV(Instruction* instr);
    bool hasSCEV(Instruction* instr);
    void registerSCEV(Instruction* instr, SCEV scev);

    static bool dominates(shared_ptr<BasicBlock> dominator, shared_ptr<BasicBlock> dominated, shared_ptr<BasicBlock> entry);
    bool isLoopInvariant(ValuePtr value);
};
typedef shared_ptr<Loop> LoopPtr;