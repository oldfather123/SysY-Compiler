//
// Created by 牛奕博 on 2023/4/18.
//

#ifndef ANUC_VALUE_H
#define ANUC_VALUE_H

#include "alist.h"
#include <vector>
#include <string>

namespace anuc {
    class Value;

    struct Use;

    class User;

    class Type;

    class Instruction;

    class Value {
    public:
        enum ValueKind {
            VK_User,
            VK_BasicBlock,

            VK_Instruction,
            VK_GEPInst,
            VK_AllocaInst,
            VK_StoreInst,
            VK_LoadInst,
            VK_PhiInst,
            VK_AddInst,
            VK_SubInst,
            VK_MulInst,
            VK_DivInst,
            VK_RemInst,

            VK_FAddInst,
            VK_FSubInst,
            VK_FMulInst,
            VK_FDivInst,
            VK_IToFInst,
            VK_FToIInst,

            VK_ICmpEQ,
            VK_ICmpNE,
            VK_ICmpLT,
            VK_ICmpLE,
            VK_ICmpGT,
            VK_ICmpGE,

            VK_FCmpEQ,
            VK_FCmpNE,
            VK_FCmpLT,
            VK_FCmpLE,
            VK_FCmpGT,
            VK_FCmpGE,
            VK_CallInst,
            VK_FNegInst,
            VK_XorInst,
            VK_ZExtInst,

            VK_RetInst,
            VK_BrInst,
            VK_CondBrInst,

            VK_LowInst,
            VK_LowLoad,
            VK_LowStore,
            VK_RVLowGEP,
            VK_LowRet,
            VK_GlobalLoad,
            VK_FloatLoad,
            VK_RVLoad,
            VK_RVStore,
            VK_RVli,
            VK_RVaddi,
            VK_RVfmvsx,

            VK_RVasmd,
            VK_RVfasmd,
            VK_RVCondBranch,
            VK_RVBranch,
            VK_RVfcmp,
            VK_RVicmp,
            VK_RVzicmp,
            VK_RVZeroCondBranch,
            VK_RVFToI,
            VK_RVIToF,
            VK_RVMv,
            VK_RVCall,
            VK_LastLowInst,
            VK_LastInstruction,
            VK_UserLast,

            VK_Constant,
            VK_ConstantFloat,
            VK_ConstantInt,
            VK_LastConstant,
            VK_BaseReg,
            VK_RegisterVar,
            VK_RvRegister,
            VK_LastBaseReg,
            VK_GlobalVar,
            VK_StackFrame,
        };
    private:
        const ValueKind kind;
        Type *type;
    protected:
        alist<Use> uses;

    public:
        Value(ValueKind kind, Type *type) : kind(kind), type(type) {}

        ValueKind getKind() { return kind; }

        void insertBackToUses(Use *u);

        void printAllUsers();

        //请搭配内存管理使用
        void replaceAllUseWith(Value *v);

        bool usesEmpty() { return uses.empty(); }

        int getUsesLength() {return uses.getLength(); }

        alist<Use>::iterator getUsesBegin() { return uses.begin(); }

        alist<Use>::iterator getUsesEnd() { return uses.end(); }

        alist<Use>::iterator getUsesFront() { return uses.front(); }

        alist<Use>::iterator getUsesBack() { return uses.back(); }

        void eraseFromList(Use *node);

        Type *getType() { return type; }
        void setType(Type *ty) {type = ty;}

        virtual string toString() { return "undef"; }

        virtual void print() {}

        virtual Instruction *getDef() { return nullptr; }
        virtual void setInst(Instruction *inst) {}

    };

    struct Use : public alist_node<Use> {
        Value *value{nullptr};
        User *user{nullptr};
        Value *parent{nullptr};
        Use() = default;
        //更换value，将use从原value上的use链条中移除
        void replaceValueWith(Value* v);
        void eraseFromParent();
        Use(Value *v, User *u) : value(v), user(u) {}
    };

    class User : public Value {
    protected:
        std::vector<Use *> operands;
    public:
        User(Value::ValueKind kind) : Value(kind, nullptr) {}

        virtual void print() {}

        bool static classof(Value *v) { return v->getKind() >= VK_User && v->getKind() < VK_UserLast; }

        vector<Use *>::iterator getBegin() { return operands.begin(); }
        vector<Use *>::iterator getEnd() { return operands.end(); }
        vector<Use *> *getOperands() {return &operands;}
        Use *getOperands(int n) { return operands[n]; }
    };


}

#endif //ANUC_VALUE_H
