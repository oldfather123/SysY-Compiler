//
// Created by 牛奕博 on 2023/4/17.
//

#ifndef ANUC_IRBUILDER_H
#define ANUC_IRBUILDER_H

#include "core.h"
#include "type.h"
#include "rtti.h"
#include <map>
#include <set>
#include <assert.h>
#include <memory>
#include <math.h>

using namespace std;
namespace anuc {
    class IRBuilder {
        Module &modu;
        //把所有创建过的type都扔进去
        //处理普通变量类型
        map<Type::TypeKind, Type *> typeLookUp;
        //处理数组变量类型
        map<pair<Type *, unsigned>, Type *> arrayTypeLookUp;
        //处理指针
        map<Type *, PointerType *> pointerTypeLookUp;
        //处理constant
        map<pair<Type *, int>, ConstantInt *> constantIntLookUp;
        map<pair<Type *, float>, ConstantFloat *> constantFloatLookUp;
        //处理变量名 变量名-版本号
        map<string, unsigned> pointerVarName;
        //中间变量名称
        unsigned registerVarNameNum{0};
        map<string, unsigned> bbName;

        //指向当前函数
        Function *currentFunc{nullptr};
        //指向当前基本块
        BasicBlock *currentBlock{nullptr};
        //指向当前插入指令
        Instruction *insertPoint{nullptr};
        //建立指针变量-allocate的映射
        map<Value *, AllocateInst *> pointToAllocate;
        Type *blockType{nullptr};
    public:
        IRBuilder &operator=(const IRBuilder &) = delete;

        IRBuilder(Module &modu) : modu(modu) {}

        Module *getMoudle() { return &modu; }

        Instruction *getCurrentInst() { return insertPoint; }
        BasicBlock *getCurrentBB() { return currentBlock; }


        void SetCurrentFunc(Function *func) {
            currentFunc = func;
            return;
        }
        //创建type的API
        Type *GetVoidTy() {
            auto i = typeLookUp.find(Type::TK_Void);
            if (i == typeLookUp.end()) {
                Type *ty = new VoidType();
                typeLookUp.insert(pair<Type::TypeKind, Type *>
                                          (Type::TK_Void, ty));
                modu.insertIntoPool(ty);
                return ty;
            }
            return i->second;
        }

        Type *GetInt32Ty() {
            auto i = typeLookUp.find(Type::TK_Int32);
            if (i == typeLookUp.end()) {
                Type *ty = new Int32Type();
                typeLookUp.insert(pair<Type::TypeKind, Type *>
                                          (Type::TK_Int32, ty));
                modu.insertIntoPool(ty);
                return ty;
            }
            return i->second;
        }

        Type *GetInt1Ty() {
            auto i = typeLookUp.find(Type::TK_Int1);
            if (i == typeLookUp.end()) {
                Type *ty = new Int1Type();
                typeLookUp.insert(pair<Type::TypeKind, Type *>
                                          (Type::TK_Int1, ty));
                modu.insertIntoPool(ty);
                return ty;
            }
            return i->second;
        }

        Type *GetFloatTy() {
            auto i = typeLookUp.find(Type::TK_Float);
            if (i == typeLookUp.end()) {
                Type *ty = new FloatType();
                typeLookUp.insert(pair<Type::TypeKind, Type *>
                                          (Type::TK_Float, ty));
                modu.insertIntoPool(ty);
                return ty;
            }
            return i->second;
        }

        Type *GetArrayTy(Type *ty, unsigned size) {
            auto i = arrayTypeLookUp.find({ty, size});
            if (i == arrayTypeLookUp.end()) {
                Type *arrayTy = new ArrayType(ty, size);
                arrayTypeLookUp.insert(pair<pair<Type *, unsigned>, Type *>(
                        pair<Type *, unsigned>(ty, size), arrayTy));
                modu.insertIntoPool(arrayTy);
                return arrayTy;
            }
            return i->second;
        }

        PointerType *GetPointerType(Type *ty) {
            auto i = pointerTypeLookUp.find(ty);
            if (i == pointerTypeLookUp.end()) {
                PointerType *pointerType = new PointerType(ty);
                pointerTypeLookUp.insert({ty, pointerType});
                modu.insertIntoPool(pointerType);
                return pointerType;
            }
            return i->second;
        }

        FunctionType *GetFunctionType(Type *resultType, std::vector<Type *> params) {
            FunctionType *ty = new FunctionType(resultType, params);
            modu.insertIntoPool(ty);
            return ty;
        }

        //创建Constant的API
        ConstantInt *GetConstantInt(Type *ty, int value) {
            auto i = constantIntLookUp.find({ty, value});
            if (i == constantIntLookUp.end()) {
                ConstantInt *ci = new ConstantInt(ty, value);
                constantIntLookUp.insert(pair<pair<Type *, int>, ConstantInt *>(
                        pair<Type *, int>(ty, value), ci));
                modu.insertIntoPool(ci);
                return ci;
            }
            return i->second;
        }

        ConstantInt *GetConstantInt32(int value) {
            Type *ty = this->GetInt32Ty();
            auto i = constantIntLookUp.find({ty, value});
            if (i == constantIntLookUp.end()) {
                ConstantInt *ci = new ConstantInt(ty, value);
                constantIntLookUp.insert(pair<pair<Type *, int>, ConstantInt *>(
                        pair<Type *, int>(ty, value), ci));
                modu.insertIntoPool(ci);
                return ci;
            }
            return i->second;
        }

        ConstantFloat *GetConstantFloat(Type *ty, float value) {
            auto i = constantFloatLookUp.find({ty, value});
            if (i == constantFloatLookUp.end()) {
                ConstantFloat *cf = new ConstantFloat(ty, value);
                constantFloatLookUp.insert(pair<pair<Type *, float>, ConstantFloat *>(
                        pair<Type *, float>(ty, value), cf));
                modu.insertIntoPool(cf);
                return cf;
            }
            return i->second;
        }

        ConstantFloat *GetConstantFloat(float value) {
            Type *ty = this->GetFloatTy();
            auto i = constantFloatLookUp.find({ty, value});
            if (i == constantFloatLookUp.end()) {
                ConstantFloat *cf = new ConstantFloat(ty, value);
                constantFloatLookUp.insert(pair<pair<Type *, float>, ConstantFloat *>(
                        pair<Type *, float>(ty, value), cf));
                modu.insertIntoPool(cf);
                return cf;
            }
            return i->second;
        }

        //创建全局变量
        GlobalVar *CreateGlobalVar(Type *ty, string name, Constant *init) {
            if (!isa<ArrayType>(ty) && init->getType() != ty) cerr << "警告：全局变量存在类型不匹配！" << endl;
            auto ptrTy = this->GetPointerType(ty);
            GlobalVar *global = new GlobalVar(ptrTy, name, init);
            modu.insertBackToChild(global);
            if (!modu.insertGlobal(name, global)) cerr << "全局变量创建错误！" << endl;
            return global;
        }

        GlobalVar *CreateGlobalVar(Type *ty, string name) {
            auto ptrTy = this->GetPointerType(ty);
            GlobalVar *global = new GlobalVar(ptrTy, name);
            modu.insertBackToChild(global);
            if (!modu.insertGlobal(name, global)) cerr << "全局变量创建错误！" << endl;
            return global;
        }

        GlobalVar *CreateGlobalVar(Type *ty, string name, InitList *list) {
            auto ptrTy = this->GetPointerType(ty);
            GlobalVar *global = new GlobalVar(ptrTy, name, list);
            modu.insertBackToChild(global);
            if (!modu.insertGlobal(name, global)) cerr << "全局变量创建错误！" << endl;
            return global;
        }

        //查找全局变量
        GlobalVar *LookUpGlobalVar(string name) {
            return modu.lookUpGlobalVar(name);
        }

        //创建函数
        Function *CreateFunction(FunctionType *type, string name, vector<string> argvNames) {
            Function *func = new Function(&modu, type, name);
            func->setArgvName(argvNames);
            modu.insertBackToChild(func);
            if (!modu.insertFunc(name, func)) cerr << "func create error" << endl;
            currentFunc = func;
            return func;
        }

        //创建外部函数
        Function *CreateExternFunc(FunctionType *type, string name, vector<string> argvNames) {
            Function *func = new Function(nullptr, type, name);
            func->setArgvName(argvNames);
            if (!modu.insertFunc(name, func)) cerr << "func create error" << endl;
            modu.getExternFuncs().push_back(func);
            return func;
        }

        //获取BasicBlock
        BasicBlock *GetBasicBlock(string name) {
            if(!blockType)blockType = new BlockType();
            if (!bbName.insert({ name, 0}).second) {
                name = name + to_string(++bbName[name]);
            }
            if (!currentFunc) {
                cerr << "must have create func";
                exit(1);
            }
            BasicBlock *bb = new BasicBlock(name, blockType);
            modu.insertIntoPool(bb);
            currentFunc->insertBackToChild(bb);
            return bb;
        }

        //设置插入指令的块
        void SetBlockInsert(BasicBlock *bb) {
            currentBlock = bb;
            bb->setParent(currentFunc);
            insertPoint = &*bb->getBack();
            return;
        }

        //设置插入指令的节点
        void SetInstInsert(Instruction *inst) {
            currentBlock = inst->getParent();
            insertPoint = inst;
            return;
        }


        //数组操作
        //get element ptr
        Value *CreateGEP(Type *ty, Value *ptr, vector<Value *> idx) {
            Type *ptrTy;
            if (ty->isArrayType()) {
                ptrTy = this->GetPointerType(cast<ArrayType>(ty)->getArrayType() );
            } else {
                ptrTy = this->GetPointerType(ty);
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *elementPtr = new RegisterVar(ptrTy, name);
            GEPInst *inst = new GEPInst(currentBlock, ty, ptr, idx, elementPtr);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(elementPtr, inst);
            return elementPtr;
        }

        //alloca store load
        Value *CreateAllocate(Type *ty, string name, bool insertIntoEntry = false) {
            if (!pointerVarName.insert({name, 0}).second)
                name = name + (to_string(++pointerVarName[name]));
            auto ptrTy = this->GetPointerType(ty);
            auto ptr = new RegisterVar(ptrTy, name);
            auto ai = new AllocateInst(currentBlock, ty, ptr);
            if (!insertIntoEntry) {
                currentBlock->insertIntoBackChild(insertPoint, ai);
                insertPoint = ai;
            } else {
                BasicBlock *entry = currentFunc->getEnrty();
                Instruction *inst = &*entry->getBegin();
                entry->insertFrontToChild(ai);
                if (insertPoint == &*entry->getFront()) insertPoint = &*currentBlock->getBack();
            }
            modu.insertIntoPool(ptr, ai);
            return ptr;
        }

        void CreateStore(Value *v, Value *ptr) {
            StoreInst *inst = new StoreInst(currentBlock, v, ptr);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(inst);
        }

        RegisterVar *CreateLoad(Type *ty, Value *ptr) {
            PointerType *pty = cast<PointerType>(ptr->getType());
            assert(pty->getElementType() == ty && "Load操作符类型不一致！");
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(ty, name);
            LoadInst *inst = new LoadInst(currentBlock, ty, ptr, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //二元运算
        Value *CreateAdd(Value *L, Value *R) {
            assert(isSameType(L, R) && "加法指令两个操作数类型不同！");
            if (isa<ConstantInt>(L) && isa<ConstantInt>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = this->GetConstantInt(this->GetInt32Ty(), lv + rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            assert(L->getType() != nullptr && "the value have null type");
            RegisterVar *rv = new RegisterVar(L->getType(), name);
            AddInst *inst = new AddInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        Value *CreateSub(Value *L, Value *R) {
            assert(isSameType(L, R) && "减法指令两个操作数类型不同！");
            if (isa<ConstantInt>(L) && isa<ConstantInt>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = this->GetConstantInt(this->GetInt32Ty(), lv - rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(L->getType(), name);
            SubInst *inst = new SubInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //mul
        Value *CreateMul(Value *L, Value *R) {
            assert(isSameType(L, R) && "乘法指令两个操作数类型不同！");
            if (isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = this->GetConstantInt(this->GetInt32Ty(), lv * rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(L->getType(), name);
            MulInst *inst = new MulInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //div
        Value *CreateDiv(Value *L, Value *R) {
            assert(isSameType(L, R) && "除法指令两个操作数类型不同！");
            if (isa<Constant>(R)) {
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                assert(rv != 0 && "被除数为0！");
            }
            if (isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = this->GetConstantInt(this->GetInt32Ty(), lv / rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(L->getType(), name);
            DivInst *inst = new DivInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        Value *CreateRem(Value *L, Value *R) {
            assert(isSameType(L, R) && "膜除指令两个操作数类型不同！");
            if (isa<Constant>(R)) {
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                assert(rv != 0 && "被除数为0！");
            }

            if (isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = this->GetConstantInt(this->GetInt32Ty(), lv % rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(L->getType(), name);
            RemInst *inst = new RemInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        Value *CreateFAdd(Value *L, Value *R) {
            assert(isSameType(L, R) && "浮点加法指令两个操作数类型不同！");
            assert(isa<FloatType>(L->getType()) && "浮点加法指令不是浮点数！");
            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantFloat *result = this->GetConstantFloat(this->GetFloatTy(), lv + rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(L->getType(), name);
            FAddInst *inst = new FAddInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //fsub
        Value *CreateFSub(Value *L, Value *R) {
            assert(isSameType(L, R) && "浮点减法指令两个操作数类型不同！");
            assert(isa<FloatType>(L->getType()) && "浮点减法指令不是浮点数！");
            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantFloat *result = this->GetConstantFloat(this->GetFloatTy(), lv - rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(L->getType(), name);
            FSubInst *inst = new FSubInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }


        //fmul
        Value *CreateFMul(Value *L, Value *R) {
            assert(isSameType(L, R) && "浮点减法指令两个操作数类型不同！");
            assert(isa<FloatType>(L->getType()) && "浮点减法指令不是浮点数！");
            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantFloat *result = this->GetConstantFloat(this->GetFloatTy(), lv * rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(L->getType(), name);
            FMulInst *inst = new FMulInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //fdiv
        Value *CreateFDiv(Value *L, Value *R) {
            assert(isSameType(L, R) && "浮点减法指令两个操作数类型不同！");
            assert(isa<FloatType>(L->getType()) && "浮点减法指令不是浮点数！");
            if (isa<ConstantFloat>(R)) {
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                assert(fabs(rv) > 1e-6 && "被除数为0！");
            }

            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantFloat *result = this->GetConstantFloat(this->GetFloatTy(), lv / rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(L->getType(), name);
            FDivInst *inst = new FDivInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        Value *CreateICmpEQ(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "指令两个操作数类型不同！");

            if (isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), lv == rv);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            ICmpEQ *inst = new ICmpEQ(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //IcmpNE
        Value *CreateICmpNE(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "指令两个操作数类型不同！");

            if (isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), lv != rv);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            ICmpNE *inst = new ICmpNE(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //IcmpLT
        Value *CreateICmpLT(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "指令两个操作数类型不同！");

            if (isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), lv < rv);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            ICmpLT *inst = new ICmpLT(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //IcmpLE
        Value *CreateICmpLE(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "指令两个操作数类型不同！");

            if (isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), lv <= rv);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            ICmpLE *inst = new ICmpLE(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //IcmpGT
        Value *CreateICmpGT(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "指令两个操作数类型不同！");

            if (isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), lv > rv);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            ICmpGT *inst = new ICmpGT(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //IcmpGE
        Value *CreateICmpGE(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "指令两个操作数类型不同！");

            if (isa<Constant>(L) && isa<Constant>(R)) {
                int lv = dyn_cast<ConstantInt>(L)->getValue();
                int rv = dyn_cast<ConstantInt>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), lv >= rv);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            ICmpGE *inst = new ICmpGE(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //FcmpEQ

        Value *CreateFCmpEQ(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "指令两个操作数类型不同！");

            if (L->getType() != GetFloatTy()) {
                assert(false && "L should be float types!");
            }
            if (R->getType() != GetFloatTy()) {
                assert(false && "R should be float types!");
            }

            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), fabs(lv - rv) < 1e-6);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            FCmpEQ *inst = new FCmpEQ(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }


        Value *CreateFCmpNE(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "指令两个操作数类型不同！");

            if (L->getType() != GetFloatTy()) {
                assert(false && "L should be float types!");
            }
            if (R->getType() != GetFloatTy()) {
                assert(false && "R should be float types!");
            }

            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), fabs(lv - rv) > 1e-6);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            FCmpNE *inst = new FCmpNE(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            currentBlock->insertBackToChild(inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //FcmpLT
        Value *CreateFCmpLT(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "L and R have different types!");

            if (L->getType() != GetFloatTy()) {
                assert(false && "L should be float types!");
            }
            if (R->getType() != GetFloatTy()) {
                assert(false && "R should be float types!");
            }

            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), lv < rv);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            FCmpLT *inst = new FCmpLT(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            currentBlock->insertBackToChild(inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //FcmpLE
        Value *CreateFCmpLE(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "L and R have different types!");

            if (L->getType() != GetFloatTy()) {
                assert(false && "L should be float types!");
            }
            if (R->getType() != GetFloatTy()) {
                assert(false && "R should be float types!");
            }

            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), (lv < rv || fabs(lv - rv) < 1e-6));
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            FCmpLE *inst = new FCmpLE(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            currentBlock->insertBackToChild(inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //FcmpGT
        Value *CreateFCmpGT(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "L and R have different types!");

            if (L->getType() != GetFloatTy()) {
                assert(false && "L should be float types!");
            }
            if (R->getType() != GetFloatTy()) {
                assert(false && "R should be float types!");
            }

            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), lv > rv);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            FCmpGT *inst = new FCmpGT(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            currentBlock->insertBackToChild(inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //FcmpGE
        Value *CreateFCmpGE(Value *L, Value *R) {
            assert(L->getType() == R->getType() && "L and R have different types!");
            if (L->getType() != GetFloatTy()) {
                assert(false && "L should be float types!");
            }
            if (R->getType() != GetFloatTy()) {
                assert(false && "R should be float types!");
            }

            if (isa<Constant>(L) && isa<Constant>(R)) {
                float lv = dyn_cast<ConstantFloat>(L)->getValue();
                float rv = dyn_cast<ConstantFloat>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), (lv > rv || fabs(lv - rv) < 1e-6));
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            FCmpGE *inst = new FCmpGE(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            currentBlock->insertBackToChild(inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        Constant *ConstantIToF(Constant *V, Type *DestTy) {
            assert(V->getType() == GetInt32Ty() && "整数转浮点数操作数不为整数！");
            int iv = dyn_cast<ConstantInt>(V)->getValue();
            float fv = (float) iv;
            ConstantFloat *result = GetConstantFloat(GetFloatTy(), fv);
            return result;
        }

        Constant *ConstantFToI(Constant *V, Type *DestTy) {
            assert(V->getType() == GetFloatTy() && "浮点转换为整型的操作数不是浮点数！");
            float fv = dyn_cast<ConstantFloat>(V)->getValue();
            int iv = static_cast<int>(fv);
            ConstantInt *result = GetConstantInt(GetInt32Ty(), iv);
            return result;
        }

        Value *CreateIToF(Value *V, Type *DestTy) {
            assert(V->getType() == GetInt32Ty() && "整数转浮点数操作数不为整数！");
            if (isa<ConstantInt>(V)) {
                int iv = dyn_cast<ConstantInt>(V)->getValue();
                float fv = (float) iv;
                ConstantFloat *result = GetConstantFloat(GetFloatTy(), fv);
                return result;
            }

            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(DestTy, name);
            IToFInst *inst = new IToFInst(currentBlock, V, DestTy, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        //FToI
        Value *CreateFToI(Value *V, Type *DestTy) {
            assert(V->getType() == GetFloatTy() && "浮点转换为整型的操作数不是浮点数！");
            if (isa<ConstantFloat>(V)) {
                float fv = dyn_cast<ConstantFloat>(V)->getValue();
                int iv = static_cast<int>(fv);
                ConstantInt *result = GetConstantInt(GetInt32Ty(), iv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(DestTy, name);
            FToIInst *inst = new FToIInst(currentBlock, V, DestTy, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        Value *CreateFNeg(Value *V) {
            if (isa<ConstantFloat>(V)) {
                float fv = dyn_cast<ConstantFloat>(V)->getValue();
                ConstantFloat *result = GetConstantFloat(-fv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetFloatTy(), name);
            FNegInst *inst = new FNegInst(currentBlock, V, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        Value *CreateXor(Value *L, Value *R) {
            if (isa<ConstantInt>(L) && isa<ConstantInt>(R)) {
                int lv = cast<ConstantInt>(L)->getValue();
                int rv = cast<ConstantInt>(R)->getValue();
                ConstantInt *result = GetConstantInt(GetInt1Ty(), lv ^ rv);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(GetInt1Ty(), name);
            XorInst *inst = new XorInst(currentBlock, L, R, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        };

        Value *CreateZExt(Value *V, Type *DestTy) {
            if (isa<ConstantInt>(V)) {
                int v = cast<ConstantInt>(V)->getValue();
                ConstantInt *result = GetConstantInt(GetInt32Ty(), v);
                return result;
            }
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv = new RegisterVar(DestTy, name);
            ZExtInst *inst = new ZExtInst(currentBlock, V, DestTy, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;

        }

        void CreateBr(BasicBlock *dest) {
            currentBlock->pushBackToSucc(dest);
            dest->pushBackToPred(currentBlock);
            BranchInst *inst = new BranchInst(currentBlock, dest);
            currentBlock->insertBackToChild(inst);
            modu.insertIntoPool(inst);
            insertPoint = inst;
            return;
        }

        void CreateCondBr(Value *Cond, BasicBlock *True, BasicBlock *False) {
            currentBlock->pushBackToSucc(True, False);
            True->pushBackToPred(currentBlock);
            False->pushBackToPred(currentBlock);
            CondBranchInst *inst = new CondBranchInst(currentBlock, Cond, True, False);
            currentBlock->insertBackToChild(inst);
            modu.insertIntoPool(inst);
            insertPoint = inst;
            return;
        }

        void CreateRet(Value *ret) {
            RetInst *ri = new RetInst(currentBlock, ret);
            currentBlock->insertIntoBackChild(insertPoint, ri);
            insertPoint = ri;
            modu.insertIntoPool(ri);
            return;
        }

        Value *CreateCall(Function *fn, vector<Value *> args) {
            string name = "x" + to_string(registerVarNameNum++);
            RegisterVar *rv;
            if (isa<VoidType>(fn->getFuncType()->getRetType())) rv = nullptr;
            else rv = new RegisterVar(fn->getFuncType()->getRetType(), name);
            //cout << fn->getFuncType()->getRetType()->toString() << endl;
            CallInst *inst = new CallInst(currentBlock, fn, args, rv);
            currentBlock->insertIntoBackChild(insertPoint, inst);
            insertPoint = inst;
            modu.insertIntoPool(rv, inst);
            return rv;
        }

        string GetNewVarName() {
            return "x" + to_string(registerVarNameNum++);
        }
        void InsertIntoPool(Value *v) {
            modu.insertIntoPool(v);
        }

        void InsertIntoPool(Value *v, Value *rest ...) {
            InsertIntoPool(v);
            InsertIntoPool(rest);
        }
    private:
        bool isSameType(Value *a, Value *b) {
            return a->getType() == b->getType();
        }

    };

}

#endif //ANUC_IRBUILDER_H
