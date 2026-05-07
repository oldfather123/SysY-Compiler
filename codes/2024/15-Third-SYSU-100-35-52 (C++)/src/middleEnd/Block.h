#pragma once

// #include "Function.h"
#include "Instruction.h"
#include "Label.h"
#include "Value.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;
// struct BasicBlock;
struct Instruction;
struct PhiInstruction;
struct IcmpInstruction;
struct Function;
struct Loop;
struct IRbuilder;

struct BasicBlock final {
    Label label;
    Function* belongfunc = nullptr;
    // vector<shared_ptr<Instruction>> removedInst;
    vector<std::unique_ptr<Instruction>> mInstructions;
    map<Instruction*, int> rank;
    uint32_t bbIdx;
    Instruction* endInstruction = nullptr;
    Instruction* beforeEndIns = nullptr;


    int rotateCount = 0;

    explicit BasicBlock(const string& name = "entry") : label{ name }, belongfunc{ nullptr } {};

    void eraseFromParent();

    void setLabel(const string& name) {
        label.setName(name);
    }

    string getName() const {
        return label.name();
    }

    void setBelongFunc(Function* func) {
        belongfunc = func;
    }

    Function* getParent() const {
        return belongfunc;
    }
    //! DO NOT USE IN FRONTEND
    InstructionPtr getTerminator() const {
        if(mInstructions.back()->isTerminator()) {
            return mInstructions.back().get();
        }
        return nullptr;
    }

    vector<Instruction*> instructions() const {
        vector<InstructionPtr> res;
        res.reserve(mInstructions.size());
        for(auto& inst : mInstructions) {
            res.push_back(inst.get());
        }
        return res;
    }

    vector<std::unique_ptr<Instruction>>& instructionsRef() {
        return mInstructions;
    }

    void setInstructions(vector<Instruction*> instructions) {
        for(auto it = mInstructions.begin(); it != mInstructions.end();) {
            if(find(instructions.begin(), instructions.end(), it->get()) == instructions.end()) {
                it = mInstructions.erase(it);
            } else {
                InstructionPtr tmp = it->release();
                it++;
            }
        }
        mInstructions.clear();
        for(auto inst : instructions) {
            mInstructions.push_back(std::unique_ptr<Instruction>(inst));
        }
    }

    size_t getBlockSize() {
        return mInstructions.size();
    }

    void setLabel(string label, uint32_t idx = 0) {
        this->label = Label(label);
        bbIdx = idx;
    }

    void dump(std::ostream& out = std::cout);

    unique_ptr<BasicBlock> clone(std::unordered_map<Value*, Value*>& vmap,std::string prefix="clone.") const;
    unique_ptr<BasicBlock> mClone(std::unordered_map<Value*, Value*>& vmap,std::string prefix="clone.") const;

    void forceSetEndInst(std::unique_ptr<Instruction> instruction);
    // just set the end instruction, not push it into mInstructions
    bool setEndInstruction(std::unique_ptr<Instruction> instruction);

    // mark for opt
    bool isDeadBB = false;
    bool isWhileCond = false;
    bool isIfCond = false;

    int loopDepth = 0;
    shared_ptr<Loop> loop = nullptr;

    static void basicBlockDFSPost(BasicBlock* bb, function<bool(BasicBlock*)> cond,
                                  function<void(BasicBlock*)> action);
    // static void domTreeDFSPost(shared_ptr<BasicBlock> bb, function<bool(shared_ptr<BasicBlock>)> cond,
    //                            function<void(shared_ptr<BasicBlock>)> action);

    // cfg member & utility
    vector<BasicBlock*> predBasicBlocks;
    vector<BasicBlock*> succBasicBlocks;
    // ! 没有处理Phi指令
    void addPredBasicBlock(BasicBlock* bb) {
        predBasicBlocks.push_back(bb);
    }
     // ! 没有处理Phi指令
    void addSuccBasicBlock(BasicBlock* bb) {
        succBasicBlocks.push_back(bb);
    }
    void deletePredBasicBlock(BasicBlock* bb) {
        for(auto& inst : instructionsRef()) {
            if(auto phi = dynamic_cast<PhiInstruction*>(inst.get())) {
                phi->removeSource(bb);
            }
        }
        predBasicBlocks.erase(std::remove(predBasicBlocks.begin(), predBasicBlocks.end(), bb), predBasicBlocks.end());
    }
    void deleteSuccBasicBlock(BasicBlock* bb) {
        // succBasicBlocks.erase(bb);
        for(auto& inst : bb->instructionsRef()) {
            if(auto phi = dynamic_cast<PhiInstruction*>(inst.get())) {
                phi->removeSource(this);
            }
        }
        succBasicBlocks.erase(std::remove(succBasicBlocks.begin(), succBasicBlocks.end(), bb), succBasicBlocks.end());
    }

    vector<BasicBlock*> getPredecessor() const {
        return predBasicBlocks;
    }
    vector<BasicBlock*> getSuccessor() const {
        return succBasicBlocks;
    }

    // NOLINTBEGIN
    int getIndegree() const {
        return predBasicBlocks.size();
    }
    int getOutdegree() const {
        return succBasicBlocks.size();
    }
    // NOLINTEND
    void rankReorder();

    string getStr() const {
        return label.name();
    }

    vector<std::unique_ptr<Instruction>>::iterator begin() {
        return mInstructions.begin();
    }

    vector<std::unique_ptr<Instruction>>::iterator end() {
        return mInstructions.end();
    }

    void replacePhiSource(BasicBlock* oldPred, BasicBlock* newPred);

    void replacePredecessor(BasicBlock* oldPred, BasicBlock* newPred);
    void replaceSuccessor(BasicBlock* oldSucc, BasicBlock* newSucc);


    BasicBlock* releaseFromParent();
    // ~BasicBlock() {
    //     for (auto& inst : mInstructions) {
    //         inst.release();
    //     }
    // }

};

// shared_ptr<BasicBlock> lcaCompute(shared_ptr<BasicBlock> bb1, shared_ptr<BasicBlock> bb2);
using BasicBlockPtr = BasicBlock*;

// TODO: utils functinon
//  remove inst, not erase; accroding llvm
//void removeInst(InstructionPtr inst);
//using ReplaceMap = std::unordered_map<Value*, Value*>;
// using BlockReducer = std::function<ValuePtr(InstructionPtr inst)>;
// bool reduceBlock(IRbuilder& builder, BasicBlockPtr block, const BlockReducer& reducer);
//  NOTICE: no terminator/operand fix
// void fixPhiNode(BasicBlockPtr oldPred, BasicBlockPtr newPred);
//bool applyReplace(Instruction* inst, const ReplaceMap& replace);
// BasicBlockPtr createIndirectBlock(const Module& module, Function& func, BasicBlockPtr sourceBlock, BasicBlockPtr targetBlock);
// bool isNoSideEffectExpr(const Instruction& inst);
// bool isMovableExpr(const Instruction& inst, bool relaxedCtx);

// void copyTarget(BasicBlockPtr target, BasicBlockPtr oldSource, BasicBlockPtr newSource);
void fixPhiNode(BasicBlock* oldPred, BasicBlock* newPred);
// bool hasSamePhiValue(BasicBlockPtr target, BasicBlockPtr sourceLhs, BasicBlockPtr sourceRhs);
// bool removePhi(BasicBlockPtr source, BasicBlockPtr target);
void applyForSuccessors(InstructionPtr branchOrSwitch, const std::function<void(BasicBlockPtr)>& functor);
//uint32_t estimateBlockSize(BasicBlockPtr block, bool dynamic);

void mergeBasicBlock(BasicBlock* a, BasicBlock* b);
void resetTarget(Instruction* brInst, BasicBlockPtr oldTarget, BasicBlockPtr newTarget);
void retargetBlock(BasicBlockPtr target, BasicBlockPtr oldSource, BasicBlockPtr newSource);
BasicBlockPtr splitBasicBlock(Instruction* after);
