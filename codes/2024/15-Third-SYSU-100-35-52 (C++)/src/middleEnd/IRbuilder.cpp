#include "Block.h"
#include "IRbuilder.h"
#include "Instruction.h"
#include "Type.h"
#include <Function.h>
#include <memory>
#include <utility>

// NOLINTBEGIN

using std::make_unique;

BasicBlockPtr IRbuilder::addBasicBlock(string name) {
    auto bb = std::make_unique<BasicBlock>(name);
    bb->setBelongFunc(mCurFunc);
    mCurFunc->addBasicBlock(std::move(bb));
    return mCurFunc->getBasicBlocks().back().get();
}

BasicBlockPtr IRbuilder::pushBasicBlock(std::unique_ptr<BasicBlock> bb) {
    bb->setBelongFunc(mCurFunc);
    mCurFunc->addBasicBlock(std::move(bb));
    return mCurFunc->getBasicBlocks().back().get();
}

void IRbuilder::setInsertPoint(BasicBlock* bb) {
    setCurBasicBlock(bb);
}

void IRbuilder::setInsertPoint(InstructionPtr instruction) {
    mCurBasicBlock = instruction->basicblock;
    mCurFunc = mCurBasicBlock->belongfunc;
    mCurInsertPoint =
        std::find_if(mCurBasicBlock->instructionsRef().begin(), mCurBasicBlock->instructionsRef().end(),
                     [instruction](const std::unique_ptr<Instruction>& ptr) -> bool { return ptr.get() == instruction; });
    insertPointIsBlockEnd = false;
}

void IRbuilder::setInsertPoint(BasicBlockPtr bb, const vector<unique_ptr<Instruction>>::iterator& insertPoint) {
    setCurBasicBlock(bb);
    mCurInsertPoint = insertPoint;
    insertPointIsBlockEnd = false;
}

bool IRbuilder::pushInstruction(unique_ptr<Instruction> inst) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    if(!mCurBasicBlock) {
        return false;
    }
    inst->basicblock = mCurBasicBlock;
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));
    return true;
}

ValuePtr IRbuilder::createAlloca(TypePtr type, bool isConst, const string& name) {
    auto inst = std::make_unique<AllocaInstruction>(type, isConst);
    inst->setBasicBlock(mCurFunc->getEntryBlock());
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("alloca");
    }
    mCurFunc->getEntryBlock()->instructionsRef().insert(mCurFunc->getEntryBlock()->instructionsRef().begin(), std::move(inst));
    // if(insertPointIsBlockEnd) {
    //     mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    // }

    return mCurFunc->getEntryBlock()->instructionsRef().front().get();
}

ValuePtr IRbuilder::createStore(ValuePtr dest, ValuePtr val) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<StoreInstruction>(dest, val);
    inst->setBasicBlock(mCurBasicBlock);
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createLoad(ValuePtr dest, TypePtr toType, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<LoadInstruction>(dest, toType);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("load");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));
    // TODO: vector.end()
    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createBinary(BinaryInstructionOps op, ValuePtr lhs, ValuePtr rhs, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<BinaryInstruction>(op, lhs, rhs);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("binary");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createFneg(ValuePtr val, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<FnegInstruction>(val);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("fneg");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createIcmp(IcmpKind kind, ValuePtr lhs, ValuePtr rhs, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<IcmpInstruction>(lhs, rhs, kind);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("icmp");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createFcmp(IcmpKind kind, ValuePtr lhs, ValuePtr rhs, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<FcmpInstruction>(lhs, rhs, kind);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("fcmp");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createBranch(BasicBlockPtr trueBranch, BasicBlockPtr falseBranch, ValuePtr cond) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<BrInstruction>(cond, trueBranch, falseBranch);
    inst->setBasicBlock(mCurBasicBlock);
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}
ValuePtr IRbuilder::createJump(BasicBlockPtr dest) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<BrInstruction>(dest);
    inst->setBasicBlock(mCurBasicBlock);
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createSiToFp(ValuePtr from, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<SitofpInstruction>(from);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("sitofp");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}
ValuePtr IRbuilder::createFpToSi(ValuePtr from, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<FptosiInstruction>(from);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("fptosi");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createCall(FunctionPtr func, vector<ValuePtr> args, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<CallInstruction>(func, args);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("call");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createRet(ValuePtr val) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<ReturnInstruction>(val);
    inst->setBasicBlock(mCurBasicBlock);
    assert(mCurFunc == mCurBasicBlock->belongfunc);
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}
ValuePtr IRbuilder::createGEP(ValuePtr from, vector<ValuePtr> indexs, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<GetElementPtrInstruction>(from, indexs);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("gep");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createCast(ValuePtr from, TypePtr to, const string& name) {
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<BitCastInstruction>(from, to);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("bitcast");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}

ValuePtr IRbuilder::createExt(ValuePtr from, TypePtr to, bool isSigned, const string& name) {  // NOLINT
    if(insertPointIsBlockEnd) {
        mCurInsertPoint = mCurBasicBlock->instructionsRef().end();
    }
    auto inst = std::make_unique<ExtInstruction>(from, to, isSigned);
    inst->setBasicBlock(mCurBasicBlock);
    if(!name.empty()) {
        inst->setLabel(name);
    } else {
        inst->setLabel("ext");
    }
    mCurBasicBlock->instructionsRef().insert(mCurInsertPoint, std::move(inst));

    return mCurBasicBlock->instructionsRef().back().get();
}
// NOLINTEND