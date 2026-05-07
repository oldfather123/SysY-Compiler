#pragma once

// #include "Function.h"
#include "Block.h"
#include "Instruction.h"
#include "Type.h"
#include <cassert>
#include <memory>

struct Function;
using FunctionPtr = Function*;

class IRbuilder {
private:
    BasicBlock* mCurBasicBlock = nullptr;
    Function* mCurFunc = nullptr;
    vector<unique_ptr<Instruction>>::iterator mCurInsertPoint;

public:
    IRbuilder() = default;
    IRbuilder(BasicBlockPtr bb) : mCurBasicBlock(bb), mCurFunc(bb->belongfunc) {
        mCurInsertPoint = bb->instructionsRef().end();
        // insertPointIsBlockEnd = true;
    }
    void setInsertPoint(BasicBlockPtr bb);
    void setInsertPoint(InstructionPtr instruction);
    void setInsertPoint(BasicBlockPtr bb, const vector<unique_ptr<Instruction>>::iterator& insertPoint);
    
    vector<unique_ptr<Instruction>>::iterator getInsertPoint() {
        return mCurInsertPoint;
    }

    bool insertPointIsBlockEnd = true;

    BasicBlockPtr addBasicBlock(string name);
    BasicBlockPtr pushBasicBlock(std::unique_ptr<BasicBlock> bb);
    void setCurFunc(FunctionPtr func) {
        mCurFunc = func;
        mCurBasicBlock = nullptr;
    }

    [[nodiscard]] FunctionPtr getCurFunc() const {
        // assert(mCurFunc);
        return mCurFunc;
    }
    void setCurBasicBlock(BasicBlockPtr bb) {
        if(mCurBasicBlock == bb) {
            mCurInsertPoint = bb->instructionsRef().end();
            insertPointIsBlockEnd = true;
            return;
        }
        mCurFunc = bb ? bb->belongfunc : nullptr;
        mCurBasicBlock = bb;
        if(bb) {
            mCurInsertPoint = bb->instructionsRef().end();
            insertPointIsBlockEnd = true;
        } else {
            mCurInsertPoint = vector<unique_ptr<Instruction>>::iterator();
            insertPointIsBlockEnd = false;
        }
    }
    [[nodiscard]] BasicBlockPtr getCurBasicBlock() const {

        return mCurBasicBlock;
    }
    bool pushInstruction(unique_ptr<Instruction> inst);
    ValuePtr createAlloca(TypePtr type, bool isConst = false, const string& name = "");
    ValuePtr createStore(ValuePtr dest, ValuePtr val);
    ValuePtr createLoad(ValuePtr dest, TypePtr toType, const string& name = "");
    ValuePtr createBinary(BinaryInstructionOps op,  ValuePtr lhs, ValuePtr rhs, const string& name = "");
    ValuePtr createFneg(ValuePtr val, const string& name = "");
    ValuePtr createIcmp(IcmpKind kind, ValuePtr lhs, ValuePtr rhs, const string& name = "");
    ValuePtr createFcmp(IcmpKind kind, ValuePtr lhs, ValuePtr rhs, const string& name = "");
    ValuePtr createBranch(BasicBlockPtr trueBranch, BasicBlockPtr falseBranch, ValuePtr cond);
    ValuePtr createJump(BasicBlockPtr dest);
    ValuePtr createSiToFp(ValuePtr from, const string& name = "");
    ValuePtr createFpToSi(ValuePtr from, const string& name = "");
    ValuePtr createCall(FunctionPtr func, vector<ValuePtr> args, const string& name = "");
    ValuePtr createRet(ValuePtr val);
    ValuePtr createGEP(ValuePtr from, vector<ValuePtr> indexs, const string& name = "");
    ValuePtr createCast(ValuePtr from, TypePtr to, const string& name = "");
    ValuePtr createExt(ValuePtr from, TypePtr to, bool isSigned, const string& name = "");
};