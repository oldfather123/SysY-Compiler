//
// Created by 牛奕博 on 2023/6/1.
//

#include "lowerToLIR.h"

using namespace anuc;

bool LIRVisitor0::visit(anuc::GEPInst *inst) {
    lowGep(inst);
    return false;
}

//计算偏移，生成lowgep
void LIRVisitor0::lowGep(anuc::GEPInst *gep) {
    Instruction *insertPoint = gep;
    Builder->SetInstInsert(insertPoint);
    BasicBlock *bb = gep->getParent();
    Value *ptr = gep->getResult();
    Value *result = gep->getResult();
    vector<Value *> offsets;
    while (ptr->getDef() && isa<GEPInst>(ptr->getDef())) {
        gep = dyn_cast<GEPInst>(ptr->getDef());
        Value *ptr2 = gep->getPtr();
        Type *ty = gep->getType();
        int biteSize = 1;
        if (ArrayType *aty = dyn_cast<ArrayType>(ty)) ty = aty->getArrayType();
        else if (PointerType *pty = dyn_cast<PointerType>(ty)) ty = pty->getElementType();
        biteSize *= ty->getByteSize();
        Value *mulRes = Builder->CreateMul(Builder->GetConstantInt32(biteSize),
                                           gep->getOperands()->back()->value);
        if (mulRes->getDef()) insertPoint = mulRes->getDef();
        offsets.push_back(mulRes);
        ptr = ptr2;
    }
    if (isa<GlobalVar>(ptr)) {
        RegisterVar *dest = new RegisterVar(
                ptr->getType(),
                Builder->GetNewVarName());
        GlobalLoad *lowInst = new GlobalLoad(bb, dest, ptr);
        bb->insertIntoBackChild(insertPoint, lowInst);
        Builder->InsertIntoPool(lowInst, dest);
        insertPoint = lowInst;
        ptr = dest;
    }
    Value *addRes = offsets[0];
    for (int i = 1; i < offsets.size(); ++i) {
        addRes = Builder->CreateAdd(addRes, offsets[i]);
        if (addRes->getDef()) insertPoint = addRes->getDef();
    }
    //创建低等级gep
    BaseReg *dest = new RegisterVar(gep->getResult()->getType(), Builder->GetNewVarName());
    auto *lowInst = new LowGEP(bb, dest, addRes, cast<BaseReg>(ptr));
    bb->insertIntoBackChild(insertPoint, lowInst);
    Builder->InsertIntoPool(lowInst);
    //将非gep的user进行替换
    for (auto i = result->getUsesBegin(); i != result->getUsesEnd();) {
        auto u = &*i;
        ++i;
        auto *s = cast<Instruction>(u->user);
        if (isa<GEPInst>(s)) continue;
        u->replaceValueWith(dest);
    }

}

//把变量分配到栈上
bool LIRVisitor0::visit(anuc::AllocateInst *inst) {
    RegisterVar *ptr = cast<RegisterVar>(inst->getResult());
    Function *func = inst->getParent()->getParent();
    map<Value *, int> &frame = func->getFrame();
    int size = cast<ArrayType>(cast<PointerType>(ptr->getType())->getElementType())->getByteSize();
    frame.insert({ptr, size});
    return true;
}


//删除phi，现在没用了
bool LIRVisitor1::visit(anuc::GEPInst *inst) {
    inst->eraseFromParent();
    return 0;
}

bool LIRVisitor1::visit(anuc::AllocateInst *inst) {
    inst->eraseFromParent();
    return 0;
}

//处理phi指令，移除所有常量
bool LIRVisitor1::visit(anuc::PhiInst *inst) {
    for (int i = 0; i < inst->getOperands()->size(); i += 2) {
        auto op = inst->getOperands(i);
        Value *x = op->value;
        Constant *c = dyn_cast<Constant>(x);
        if (!c) continue;
        BasicBlock *b = cast<BasicBlock>(inst->getOperands(i + 1)->value);
        auto back = &*b->getBack();
        LowInst *l{nullptr};
        RegisterVar *dest{nullptr};
        if (ConstantInt *ci = dyn_cast<ConstantInt>(c)) {
            dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            l = new RVli(b, ci, dest);
        } else {
            ConstantFloat *fi = cast<ConstantFloat>(c);
            dest = new RegisterVar(Builder->GetFloatTy(), Builder->GetNewVarName());
            l = new FloatLoad(b, dest, fi);
        }
        b->insertIntoChild(l, back);
        Builder->InsertIntoPool(dest, l);
        op->replaceValueWith(dest);
    }
    return 0;
}

//降级load/store
bool LIRVisitor1::visit(anuc::LoadInst *inst) {
    BasicBlock *bb = inst->getParent();
    Instruction *insertPoint = inst;
    Builder->SetInstInsert(insertPoint);
    Value *ptr = inst->getPtr();
    Value *result = inst->getResult();
    RegisterVar *dest = cast<RegisterVar>(result);
    //处理全局变量
    if (isa<GlobalVar>(ptr)) {
        RegisterVar *x = new RegisterVar(
                ptr->getType(),
                Builder->GetNewVarName());
        GlobalLoad *s1 = new GlobalLoad(bb, x, ptr);
        LowLoad *s2 = new LowLoad(bb, dest, Builder->GetConstantInt32(0),x);
        bb->insertIntoBackChild(insertPoint, {s1, s2});
        Builder->InsertIntoPool(x, s1, s2);
        inst->eraseFromParent();
        return 0;
    }
    LowGEP *gep = cast<LowGEP>(ptr->getDef());
    //创建低等级load
    LowLoad *lowInst = new LowLoad(bb, dest, gep->getOffset(), gep->getPtr());
    bb->insertIntoBackChild(insertPoint, lowInst);
    inst->eraseFromParent();
    Builder->InsertIntoPool(lowInst);
    return 0;
}

bool LIRVisitor1::visit(anuc::StoreInst *inst) {
    BasicBlock *bb = inst->getParent();
    Instruction *insertPoint = inst;
    Builder->SetInstInsert(insertPoint);
    Value *ptr = inst->getPtr();
    Value *val = inst->getVal();
    //如果val为常量，引入一个li
    if (isa<Constant>(val)) {
        if (ConstantInt *ci = dyn_cast<ConstantInt>(val)) {
            RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *li = new RVli(bb, ci, dest);
            bb->insertIntoBackChild(insertPoint, li);
            Builder->InsertIntoPool(dest, li);
            insertPoint = li;
            val = dest;
        } else {
            ConstantFloat *fi = cast<ConstantFloat>(val);
            RegisterVar *dest = new RegisterVar(Builder->GetFloatTy(), Builder->GetNewVarName());
            FloatLoad *fl = new FloatLoad(bb, dest, fi);
            bb->insertIntoBackChild(insertPoint, fl);
            Builder->InsertIntoPool(dest, fl);
            insertPoint = fl;
            val = dest;
        }
    }
    //处理全局变量
    if (isa<GlobalVar>(ptr)) {
        RegisterVar *x = new RegisterVar(
                ptr->getType(),
                Builder->GetNewVarName());
        GlobalLoad *s1 = new GlobalLoad(bb, x, ptr);
        LowStore *s2 = new LowStore(bb, Builder->GetConstantInt32(0), val, x);
        bb->insertIntoBackChild(insertPoint, {s1, s2});
        Builder->InsertIntoPool(x, s1, s2);
        inst->eraseFromParent();
        return 0;
    }
    LowGEP *gep = cast<LowGEP>(ptr->getDef());
    LowStore *lowInst = new LowStore(bb, gep->getOffset(), val, gep->getPtr());
    bb->insertIntoBackChild(insertPoint, lowInst);
    inst->eraseFromParent();
    Builder->InsertIntoPool(lowInst);
    return 0;

}


bool LIRVisitor1::visit(anuc::RetInst *inst) {
    BasicBlock *bb = inst->getParent();
    Value *ret = inst->getRet();
    LowRet *lowInst = new LowRet(bb, ret);
    bb->insertIntoBackChild(inst, lowInst);
    inst->eraseFromParent();
    return 0;
}

bool LIRVisitor1::visit(anuc::FAddInst *inst) {
    transformFASMD(inst, Builder, RVfasmd::fadd);
    return 0;
}

bool LIRVisitor1::visit(anuc::FSubInst *inst) {
    transformFASMD(inst, Builder, RVfasmd::fsub);
    return 0;
}

bool LIRVisitor1::visit(anuc::FMulInst *inst) {
    transformFASMD(inst, Builder, RVfasmd::fmul);
    return 0;
}

bool LIRVisitor1::visit(anuc::FDivInst *inst) {
    transformFASMD(inst, Builder, RVfasmd::fdiv);
    return 0;
}

//条件跳转
bool LIRVisitor1::visit(anuc::CondBranchInst *inst) {
    transformCondBranch(inst, Builder);
    return 0;
}

//普通跳转，直接替换即可
bool LIRVisitor1::visit(anuc::BranchInst *inst) {
    BasicBlock *dest = cast<BasicBlock>(inst->getDest());
    BasicBlock *bb = inst->getParent();
    RVbranch *rvInst = new RVbranch(bb, dest);
    bb->insertIntoBackChild(inst, rvInst);
    inst->eraseFromParent();
    return true;
}

bool LIRVisitor1::visit(anuc::FToIInst *inst) {
    transformFToI(inst, Builder);
    return false;
}

bool LIRVisitor1::visit(anuc::IToFInst *inst) {
    transformIToF(inst, Builder);
    return true;
}


bool LIRVisitor2::visit(anuc::LowGEP *inst) {
    if (inst->getResult()->usesEmpty()) inst->eraseFromParent();
    return 0;
}

bool LIRVisitor2::visit(anuc::AddInst *inst) {
    transformASMD(inst, Builder, RVasmd::add);
    return 0;
}

bool LIRVisitor2::visit(anuc::SubInst *inst) {
    transformASMD(inst, Builder, RVasmd::sub);
    return 0;
}

bool LIRVisitor2::visit(anuc::MulInst *inst) {
    transformASMD(inst, Builder, RVasmd::mul);
    return 0;
}

bool LIRVisitor2::visit(anuc::DivInst *inst) {
    transformASMD(inst, Builder, RVasmd::div);
    return 0;
}

bool LIRVisitor2::visit(anuc::RemInst *inst) {
    transformASMD(inst, Builder, RVasmd::rem);
    return 0;
}

bool LIRVisitor2::visit(ICmpEQ *inst) {
    transformICmp(inst, Builder, RVcondbranch::beq);
    return true;
}

bool LIRVisitor2::visit(ICmpNE *inst) {
    transformICmp(inst, Builder, RVcondbranch::bne);
    return true;
}

bool LIRVisitor2::visit(ICmpGE *inst) {
    transformICmp(inst, Builder, RVcondbranch::bge);
    return true;
}

bool LIRVisitor2::visit(ICmpGT *inst) {
    transformICmp(inst, Builder, RVcondbranch::bgt);
    return true;
}

bool LIRVisitor2::visit(ICmpLE *inst) {
    transformICmp(inst, Builder, RVcondbranch::ble);
    return true;
}

bool LIRVisitor2::visit(ICmpLT *inst) {
    transformICmp(inst, Builder, RVcondbranch::blt);
    return true;
}

bool LIRVisitor2::visit(FCmpEQ *inst) {
    transformFCmp(inst, Builder, regTable, RVcondbranch::beq);
    return true;
}

bool LIRVisitor2::visit(FCmpNE *inst) { return false; }

bool LIRVisitor2::visit(FCmpGE *inst) { return false; }

bool LIRVisitor2::visit(FCmpGT *inst) { return false; }

bool LIRVisitor2::visit(FCmpLE *inst) { return false; }

bool LIRVisitor2::visit(FCmpLT *inst) { return false; }

//后端只有int，直接把int1类型换成int32
//其实只有处理!时需要，先直接删掉试试
bool LIRVisitor2::visit(ZExtInst *inst) {
    if (inst->getResult()->usesEmpty()) {
        inst->eraseFromParent();
        return true;
    } else {
        inst->getSrc()->setType(Builder->GetInt32Ty());
        inst->getResult()->replaceAllUseWith(inst->getSrc());
        inst->eraseFromParent();
    }
    return true;
}

//处理lowGEP
//计算地址
bool LIRVisitor3::visit(anuc::LowGEP *inst) {
    Instruction *insertPoint = inst;
    BasicBlock *bb = inst->getParent();
    Value *offset = inst->getOffset();
    BaseReg *dest = cast<BaseReg>(inst->getResult());
    BaseReg *ptr = cast<BaseReg>(inst->getPtr());
    int frameOffset;
    auto &frame = bb->getParent()->getFrame();
    auto it = frame.find(ptr);

    RVLoad::OpKind kind = RVLoad::lw;
    if (isa<PointerType>(cast<PointerType>(ptr->getType())->getElementType()))
        kind = RVLoad::ld;
    //基地址
    BaseReg *basePtr;
    if (it != frame.end()) {
        frameOffset = it->second;
        basePtr = regTable->getReg(RvRegister::sp);
    } else {
        frameOffset = 0;
        basePtr = cast<BaseReg>(ptr);
    }

    //第一中情况，offset为常数
    if (ConstantInt *ci = dyn_cast<ConstantInt>(offset)) {
        int imm = ci->getValue() + frameOffset;
        //小于2048直接lw，否则相加一下
        if (imm < 2048) {
            RVaddi *s = new RVaddi(bb, basePtr, Builder->GetConstantInt32(imm), dest);
            bb->insertIntoBackChild(inst, s);
            inst->eraseFromParent();
            Builder->InsertIntoPool(s);
            return true;
        } else {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());

            RVli *s1 = new RVli(bb, Builder->GetConstantInt32(imm), x1);
            RVasmd *s2 = new RVasmd(bb, basePtr, x1, dest, RVasmd::add);

            bb->insertIntoBackChild(inst, {s1, s2});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, x2, s1, s2);
            return true;
        }
    } else {
        BaseReg *o = cast<BaseReg>(offset);
        //用addi add相加
        if (frameOffset == 0) {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVasmd *s1 = new RVasmd(bb, basePtr, o, dest, RVasmd::add);
            bb->insertIntoBackChild(inst, s1);
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, s1);
            return true;
        } else if (0 < frameOffset && frameOffset < 2048) {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVaddi *s1 = new RVaddi(bb, o, Builder->GetConstantInt32(frameOffset), x1);
            RVasmd *s2 = new RVasmd(bb, x1, o, dest, RVasmd::add);

            bb->insertIntoBackChild(inst, {s1, s2});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, x2, s1, s2);
            return true;
        }
            //需要li一下
        else {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *s1 = new RVli(bb, Builder->GetConstantInt32(frameOffset), x1);
            RVasmd *s2 = new RVasmd(bb, x1, o, x2, RVasmd::add);
            RVasmd *s3 = new RVasmd(bb, basePtr, x2, dest, RVasmd::add);
            bb->insertIntoBackChild(inst, {s1, s2, s3});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, x2, s1, s2, s3);
            return true;
        }
    }
    return 0;
}

//处理lowLoad和lowStore
//如果偏移量小于2047，可以直接用sp作为基地址进行分配
//否则需要另外计算地址
//整数使用 sw， 指针变量使用sd
bool LIRVisitor3::visit(anuc::LowLoad *inst) {
    Instruction *insertPoint = inst;
    BasicBlock *bb = inst->getParent();
    Value *offset = inst->getOffset();
    BaseReg *dest = cast<BaseReg>(inst->getResult());
    BaseReg *ptr = cast<BaseReg>(inst->getPtr());
    int frameOffset;
    auto &frame = bb->getParent()->getFrame();
    auto it = frame.find(ptr);
    RVLoad::OpKind kind = RVLoad::lw;
    //看看用lw还是ld，变量获取为dest类型
    //如果dest为寄存器，则从ptr获取
    if (!isa<RvRegister>(dest) && isa<PointerType>(dest->getType())) {
        kind = RVLoad::ld;
    } else if (isa<RvRegister>(dest) &&
               isa<PointerType>(cast<PointerType>(ptr->getType())->getElementType())) {
        kind = RVLoad::ld;
    }
    //基地址
    BaseReg *basePtr;
    if (it != frame.end()) {
        frameOffset = it->second;
        basePtr = regTable->getReg(RvRegister::sp);
    } else {
        frameOffset = 0;
        basePtr = cast<BaseReg>(ptr);
    }
    //offset为常数
    if (ConstantInt *ci = dyn_cast<ConstantInt>(offset)) {
        int imm = ci->getValue() + frameOffset;
        //小于2048直接lw，否则相加一下
        if (imm < 2048) {
            RVLoad *s = new RVLoad(bb, basePtr, dest, imm, kind);
            bb->insertIntoBackChild(inst, s);
            inst->eraseFromParent();
            Builder->InsertIntoPool(s);
            return true;
        }
            //大于2048特殊处理
        else {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());

            RVli *s1 = new RVli(bb, Builder->GetConstantInt32(imm), x1);
            RVasmd *s2 = new RVasmd(bb, basePtr, x1, x2, RVasmd::add);
            RVLoad *s3 = new RVLoad(bb, x2, dest, 0, kind);

            bb->insertIntoBackChild(inst, {s1, s2, s3});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, x2, s1, s2, s3);
            return true;
        }
    }
        //offset为变量
    else {
        BaseReg *o = cast<BaseReg>(offset);
        //把frameOffset和sp相加为x1
        //再把x1和offset变量相加
        if (frameOffset == 0) {
            //栈上偏移为0
            //offset + o (sp)
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVasmd *s1 = new RVasmd(bb, basePtr, o, x1, RVasmd::add);
            RVLoad *s2 = new RVLoad(bb, x1, dest, 0, kind);

            bb->insertIntoBackChild(inst, {s1, s2});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, s1, s2);
            return true;
        }
            //栈上偏移小于2048大于0
        else if (0 < frameOffset && frameOffset < 2048) {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVaddi *s1 = new RVaddi(bb, basePtr, Builder->GetConstantInt32(frameOffset), x1);
            RVasmd *s2 = new RVasmd(bb, x1, o, x2, RVasmd::add);
            RVLoad *s3 = new RVLoad(bb, x2, dest, 0, kind);

            bb->insertIntoBackChild(inst, {s1, s2, s3});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, x2, s1, s2, s3);
            return true;
        }
            //需要li一下
        else {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x3 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *s1 = new RVli(bb, Builder->GetConstantInt32(frameOffset), x1);
            RVasmd *s2 = new RVasmd(bb, x1, o, x2, RVasmd::add);
            RVasmd *s3 = new RVasmd(bb, basePtr, x2, x3, RVasmd::add);
            RVLoad *s4 = new RVLoad(bb, x3, dest, 0, kind);

            bb->insertIntoBackChild(inst, {s1, s2, s3, s4});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, x2, x3, s1, s2, s3, s4);
            return true;
        }
    }


}


bool LIRVisitor3::visit(anuc::LowStore *inst) {

    Instruction *insertPoint = inst;
    BasicBlock *bb = inst->getParent();
    Value *offset = inst->getOffset();
    BaseReg *val = cast<BaseReg>(inst->getVal());
    BaseReg *ptr = cast<BaseReg>(inst->getPtr());
    int frameOffset;
    auto &frame = bb->getParent()->getFrame();
    auto it = frame.find(ptr);
    //基地址
    BaseReg *basePtr;
    if (it != frame.end()) {
        frameOffset = it->second;
        basePtr = regTable->getReg(RvRegister::sp);
    } else {
        frameOffset = 0;
        basePtr = cast<BaseReg>(ptr);
    }

    RVStore::OpKind kind = RVStore::sw;
    if (isa<PointerType>(val->getType())) {
        kind = RVStore::sd;
    }
    //offset为常数
    if (ConstantInt *ci = dyn_cast<ConstantInt>(offset)) {
        int imm = ci->getValue() + frameOffset;
        //小于2048直接lw，否则相加一下
        if (imm < 2048) {
            RVStore *s = new RVStore(bb, basePtr, val, imm, kind);
            bb->insertIntoBackChild(inst, s);
            inst->eraseFromParent();
            Builder->InsertIntoPool(s);
            return true;
        }
            //进行拼接
        else {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());

            RVli *s1 = new RVli(bb, Builder->GetConstantInt32(imm), x1);
            RVasmd *s2 = new RVasmd(bb, basePtr, x1, x2, RVasmd::add);
            RVStore *s3 = new RVStore(bb, x2, val, 0, kind);

            bb->insertIntoBackChild(inst, {s1, s2, s3});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, x2, s1, s2, s3);
            return true;
        }
    } else {
        BaseReg *o = cast<BaseReg>(offset);
        //把frameOffset和sp相加为x1
        //再把x1和offset变量相加
        if (frameOffset == 0) {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVasmd *s1 = new RVasmd(bb, basePtr, o, x1, RVasmd::add);
            RVStore *s2 = new RVStore(bb, x1, val, 0, kind);

            bb->insertIntoBackChild(inst, {s1, s2});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, s1, s2);
            return true;
        } else if (0 < frameOffset && frameOffset < 2048) {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVaddi *s1 = new RVaddi(bb, basePtr, Builder->GetConstantInt32(frameOffset), x1);
            RVasmd *s2 = new RVasmd(bb, x1, o, x2, RVasmd::add);
            RVStore *s3 = new RVStore(bb, x2, val, 0, kind);

            bb->insertIntoBackChild(inst, {s1, s2, s3});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, x2, s1, s2, s3);
            return true;
        }
            //需要li一下
        else {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RegisterVar *x3 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *s1 = new RVli(bb, Builder->GetConstantInt32(frameOffset), x1);
            RVasmd *s2 = new RVasmd(bb, x1, o, x2, RVasmd::add);
            RVasmd *s3 = new RVasmd(bb, basePtr, x2, x3, RVasmd::add);
            RVStore *s4 = new RVStore(bb, x3, val, 0, kind);

            bb->insertIntoBackChild(inst, {s1, s2, s3, s4});
            inst->eraseFromParent();
            Builder->InsertIntoPool(x1, x2, x3, s1, s2, s3, s4);
            return true;
        }
    }
}

bool LIRVisitor2::visit(anuc::RVcondbranch *inst) {
    BasicBlock *bb = inst->getParent();
    Value *l = inst->getOperands(0)->value;
    Value *r = inst->getOperands(1)->value;
    if(isImm(l, r)) {
        ConstantInt *limm = dyn_cast<ConstantInt>(l);
        ConstantInt *rimm = dyn_cast<ConstantInt>(r);
        if(limm) {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *s1 = new RVli(bb, Builder->GetConstantInt32(limm->getValue()), x1);
            inst->getOperands(0)->replaceValueWith(x1);
            bb->insertIntoChild(s1, inst);
            Builder->InsertIntoPool(s1, x1);
        }
        if(rimm) {
            RegisterVar *x1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
            RVli *s1 = new RVli(bb, Builder->GetConstantInt32(rimm->getValue()), x1);
            inst->getOperands(0)->replaceValueWith(x1);
            bb->insertIntoChild(s1, inst);
            Builder->InsertIntoPool(s1, x1);
        }
    }
    return false;
}



