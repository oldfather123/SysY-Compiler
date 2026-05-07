#pragma once

#include "constant.h"
#include "functiontype.h"
#include "argument.h"
#include "basicblock.h"
#include <set>

namespace IR
{
    struct GlobalValue : public Constant
    {
        GlobalValue(BasicType *type, const unsigned int ID) : Constant(type, ID) {}
        GlobalValue(BasicType *type, std::string name, const unsigned int ID) : Constant(type, name, ID) {}
    };

    // czr: 看名字应该都能看懂吧，懒得写了
    struct Function : public GlobalValue
    {
        BasicBlock *entryBlock = nullptr;
        bool isBuiltin = false;
        std::vector<Argument *> funArgs;
        List<BasicBlock *> funBlocks;
        std::map<BasicBlock *, std::set<BasicBlock *>> cfg();

        Function(BasicType *retType, std::string name);

        Function(BasicType *retType, std::string name, std::vector<BasicType *> argtypes);

        void waste() override;

        BasicType *getReturnType() { return type->getBaseType(); }

        List<BasicBlock *> &blocks() { return funBlocks; }
        std::vector<BasicBlock *> getVectorBlocks();

        std::vector<Argument *> &args() { return funArgs; }

        Argument *getArg(unsigned int i) { return funArgs[i]; }

        BasicType *getArgType(unsigned int i) { return static_cast<FunctionType *>(type)->getParamType(i); }

        void addArgument(BasicType *arg)
        {
            int id = funArgs.size();
            funArgs.push_back(new Argument(arg, id));
        }

        void addBlock(BasicBlock *block, bool entry = false);

        void insertBlock(BasicBlock *block, BasicBlock *prev);

        void removeBlock(BasicBlock *block);

        static Function *create(BasicType *retType, std::vector<BasicType *> argtypes, std::string name)
        {
            return new Function(retType, name, argtypes);
        }

        static Function *create(BasicType *retType, std::string name)
        {
            return new Function(retType, name);
        }

        void emitIR(std::ostream &os) override;

        void emitUse(std::ostream &os) override;

        bool isBuiltinFunction() { return isBuiltin; }

        BasicBlock *getEntryBlock() { return entryBlock; }

        void mergeBlock(BasicBlock *prev, BasicBlock *nxt);

        BasicBlock *splitBlock(BasicBlock *block, Instruction *pos);
    };

    struct ExternalFunction : public Function
    {
        ExternalFunction(BasicType *retType, std::string name, std::vector<BasicType *> argtypes);

        void emitIR(std::ostream &os) override;

        static ExternalFunction *create(BasicType *retType, std::vector<BasicType *> argtypes, std::string name)
        {
            return new ExternalFunction(retType, name, argtypes);
        }
    };

    // TODO: 写一个方法用来获取全局数组的哪些位置被初始化了
    struct GlobalVariable final : public GlobalValue
    {
        bool isInit = false;
        bool isConstant = false;
        Constant *initializer;
        GlobalVariable(BasicType *type, std::string name, bool isConstant = false);

        GlobalVariable(BasicType *type, std::string name, Constant *initializer, bool isConstant = false);

        bool isInitialized() { return isInit; }
        bool isPointingConst() { return isConstant; }

        void emitIR(std::ostream &os) override;

        void setInitializer(Constant *init) override
        {
            initializer = init;
            isInit = true;
            operands[0] = init;
        }

        static GlobalVariable *create(BasicType *type, std::string name, Constant *initializer, bool isConstant = false)
        {
            auto pointTy = PointerType::get(type);
            return new GlobalVariable(pointTy, name, initializer, isConstant);
        }

        static GlobalVariable *create(BasicType *type, std::string name, bool isConstant = false)
        {
            auto pointTy = PointerType::get(type);
            return new GlobalVariable(pointTy, name, isConstant);
        }
    };
}