#pragma once
//
#include "Block.h"
#include "Instruction.h"
#include "Value.h"
#include <cassert>
#include <memory>
#include <set>
#include <utility>
#include <vector>
//// #include "Value.h"
//// #include "Function.h"
// using namespace std;
//
// struct Block;
// struct BasicBlock;
// struct Function;
// struct Instruction;
// struct PhiInstruction;
// struct IcmpInstruction;
// struct Value;
//
// scalar evolution
// class SCEV {
// private:
//    vector<ValuePtr> scevVal;                              // NOLINT
//    vector<InstructionPtr> scevIns;                        // NOLINT
//    set<BinaryInstruction*> instructionsHasBeenCaculated;  // NOLINT
//
// public:
//    SCEV() = default;
//    SCEV(ValuePtr initial, ValuePtr step);
//    SCEV(ValuePtr initial, const SCEV& innerSCEV);
//    SCEV(const vector<ValuePtr>& vec);
//    SCEV(ValuePtr initial, ValuePtr step, std::set<BinaryInstruction*>&& binarys);
//
//    friend SCEV operator+(ValuePtr lhs, const SCEV& rhs);
//    friend SCEV operator+(const SCEV& lhs, const SCEV& rhs);
//
//    friend SCEV operator-(const SCEV& lhs, ValuePtr rhs);
//    friend SCEV operator-(const SCEV& lhs, const SCEV& rhs);
//
//    friend SCEV operator*(ValuePtr lhs, const SCEV& rhs);
//    // friend SCEV operator*(const SCEV& lhs, const SCEV& rhs);
//
//    ValuePtr& at(unsigned i) {
//        return scevVal.at(i);
//    }
//
//    size_t size() {
//        return scevVal.size();
//    }
//
//    [[nodiscard]] ValuePtr at(unsigned i) const {
//        return scevVal.at(i);
//    }
//
//    [[nodiscard]] size_t size() const {
//        return scevVal.size();
//    }
//
//    void getItemChains(unsigned i, std::vector<BinaryInstruction*>& instrChain);
//
//    string getSignature();
//
//    void clear();
//};
// using SCEVPtr = SCEV*;
//
//
struct Loop {
    //    set<shared_ptr<BasicBlock>> bbSet;
    //    vector<shared_ptr<BasicBlock>> basicBlocks;
    BasicBlock* header = nullptr;
    BasicBlock* latch = nullptr;
    Loop* parentLoop = nullptr;
    Value* inductionVar;  // header's arg, 归纳变量 phi node
    Value* next;          // ind + step = next
    // [initial, bound)
    Value* initVal;  // int
    Value* bound;    // int
    int step;
    Loop() = default;
    Loop(BasicBlock* header, BasicBlock* latch, Value* inductionVar, Value* next, Value* initial, Value* bound, int step)
        : header(header), latch(latch), inductionVar(inductionVar), next(next), initVal(initial), bound(bound), step(step) {}

    set<Loop*> innerLoops;
    vector<Loop*> subLoops;
    // 指向出口
    unordered_set<BasicBlock*> exitingBlocks;
    // 出口
    unordered_set<BasicBlock*> exitBlocks;
    //    shared_ptr<Block> block = nullptr;
    //    shared_ptr<Function> parent = nullptr;
    // unordered_map<Instruction*, SCEV> SCEVCheck;  // NOLINT
    //
    //    int id;
    //    int loopDepth;
    //    bool isInner;
    //
    //    // SCEV
    //    // std::unordered_map<Instruction*, SCEV> SCEVCheck;
    //
    //    // induction var
    //    PhiInstruction* indPhi;   // reg 即 val
    //    // icmp instruction
    //    shared_ptr<Instruction> indCondVar;
    //    Value* next;
    //    // bound
    //    shared_ptr<Value> indEnd;
    //    IcmpKind icmpKind;
    //    int tripCount;
    bool contains(BasicBlock* bb) const {
        // return bbSet.count(bb);
        assert(false);
    }

    void dumpCfg();

    //
    BasicBlock* getHeader() const {
        return header;
    }

    int getTripCount() const {
        if (!initVal->is<Const>() || !bound->is<Const>()) return -1;

        int init = initVal->as<Const>()->intVal;    // NOLINT
        int boundVal = bound->as<Const>()->intVal;  // NOLINT

        auto br = dynamic_cast<BrInstruction*>(latch->getTerminator());
        auto cond = br->getCondition();
        auto icmp = dynamic_cast<IcmpInstruction*>(cond);
        auto icmpKind = icmp->kind;

        if(icmpKind == IcmpKind::ICmpSLT) {
            return (boundVal - init - 1) / step + 1;
        }
        if(icmpKind == IcmpKind::ICmpSLE) {
            return (boundVal - init) / step + 1;
        }
        if(icmpKind == IcmpKind::ICmpSGT) {
            return (init - boundVal - 1) / std::abs(step) + 1;
        }
        if(icmpKind == IcmpKind::ICmpSGE) {
            return (init - boundVal) / std::abs(step) + 1;
        }
        if (icmpKind == IcmpKind::ICmpNE) {
            return (boundVal - init) / step ;
        }


        assert(false);
    }
    //    shared_ptr<BasicBlock> getPreheader() const {
    //        return preHeader;
    //    }
    //    shared_ptr<Block> getBlock() const {
    //        return block;
    //    }
    //    set<shared_ptr<BasicBlock>> getBasicBlocks() const {
    //        return bbSet;
    //    }
    //    vector<shared_ptr<BasicBlock>> getLoopBasicBlocks() const {
    //        return basicBlocks;
    //    }
    //    shared_ptr<BasicBlock> getLatchBlock() const {
    //        return latchBlock;
    //    }
    //    shared_ptr<Loop> getParentLoop() const {
    //        return parentLoop;
    //    }
    //    vector<shared_ptr<Loop>> getSubLoops() const {
    //        return subLoops;
    //    }
    //    unordered_set<shared_ptr<BasicBlock>> getExitingBlocks() const {
    //        return exitingBlocks;
    //    }
    //    unordered_set<shared_ptr<BasicBlock>> getExitBlocks() const {
    //        return exitBlocks;
    //    }
    //    vector<shared_ptr<BasicBlock>>& getLoopBasicBlocksRef() {
    //        return basicBlocks;
    //    }
    //    vector<shared_ptr<Loop>>& getSubLoopsRef() {
    //        return subLoops;
    //    }
    //    int getLoopDepth() const {
    //        return loopDepth;
    //    }
    //
    //    IcmpKind getIcmpKind() const {
    //        return icmpKind;
    //    }
    //    int getTripCount() const {
    //        return tripCount;
    //    }
    //    void setLoopBasicBlocks(vector<shared_ptr<BasicBlock>> bbs) {
    //        basicBlocks = std::move(bbs);
    //    }
    //    void setPreheader(shared_ptr<BasicBlock> _preHeader) {  // NOLINT
    //        preHeader = std::move(_preHeader);
    //    }
    //    void setLatchBlock(shared_ptr<BasicBlock> bb) {
    //        latchBlock = std::move(bb);
    //    }
    //    void setParentLoop(shared_ptr<Loop> parent) {
    //        parentLoop = std::move(parent);
    //    }
    //    void setSubLoops(vector<shared_ptr<Loop>> loops) {
    //        subLoops = std::move(loops);
    //    }
    //
    //    // maintain basicBlocks and bbSets
    //    void addBasicBlock(const shared_ptr<BasicBlock>& bb) {
    //        basicBlocks.push_back(bb);
    //        bbSet.insert(bb);
    //    }
    //    void addSubLoop(const shared_ptr<Loop>& loop) {
    //        subLoops.push_back(loop);
    //    }
    //    void addExitingBlocks(const shared_ptr<BasicBlock>& bb) {
    //        exitingBlocks.insert(bb);
    //    }
    //    void addExitBlocks(const shared_ptr<BasicBlock>& bb) {
    //        exitBlocks.insert(bb);
    //    }
    //    void setLoopDepth(int depth) {
    //        loopDepth = depth;
    //    }
    //    void setIndexCondInstr(shared_ptr<Instruction> instr) {
    //        indCondVar = std::move(instr);
    //    }
    //    void setICmpKind(IcmpKind kind) {
    //        icmpKind = kind;
    //    }
    //    void setIndexEnd(ValuePtr ind) {
    //        indEnd = std::move(ind);
    //    }
    //    void setNext(Value* n) {
    //        next = n;
    //    }
    //    void setStep(intmax_t s) {
    //        step = s;
    //    }
    //    void setIndexPhi(PhiInstruction* phi) {
    //        indPhi = phi;
    //    }
    //
    //    SCEV& getSCEV(Instruction* instr);
    //
    //    bool hasSCEV(Instruction* instr);
    //
    // void registerSCEV(Instruction* instr, SCEV scev);
    //
    //    void cleanSCEV() {
    //        SCEVCheck.clear();
    //    }
    //
    //    static bool isADominatorB(shared_ptr<BasicBlock> A, shared_ptr<BasicBlock> B, shared_ptr<BasicBlock> entry);  // NOLINT
    bool isSimpleLoopInvariant(ValuePtr value);
    //    bool isInnerLoop() const {
    //        return parentLoop != nullptr;
    //    }
    //
    //    // SCEV& getSCEV(Instruction* instr);
    //
    //    // bool hasSCEV(Instruction* instr);
    //
    //    // void registerSCEV(Instruction* instr, SCEV scev);
    //
    //    // void cleanSCEV() { SCEVCheck.clear(); }
    //
    //    // using scev_iterator = std::unordered_map<Instruction*, SCEV>::iterator;
    //    // iterator_range<scev_iterator> getSCEV() { return make_range(SCEVCheck.begin(), SCEVCheck.end()); }
    //
    //    // PhiInstruction* getIndexPhi() { return indPhi; }
    //    // void setIndexPhi(PhiInstruction* phi) { indPhi = phi; }
    //
    //    // ValuePtr getIndexEnd() { return indEnd; }
    //    // void setIndexEnd(ValuePtr end) { indEnd = end; }
    //
    //    // InstructionPtr getIndexCondInstr() { return indCondVar; }
    //    // void setIndexCondInstr(InstructionPtr instr) { indCondVar = instr; }
    //
    //    // int getTripCount() { return tripCount; }
    //    // void setTripCount(int c) { tripCount = c; }
};
using LoopPtr = Loop*;
