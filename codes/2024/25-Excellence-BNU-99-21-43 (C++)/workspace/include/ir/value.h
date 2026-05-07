#pragma once
#include "basictype.h"
#include "errortype.h"
#include "use.h"
#include "list.h"
#include <assert.h>

#define HANDLE_TYPECHEK_CREATE(value)               \
    bool is##value() const                          \
    {                                               \
        return getValueID() == ValueTy::value##Val; \
    }

namespace IR
{
    struct Constant;
    struct Value
    {
        enum ValueTy
        {
            // Constant
            TemporaryVal,
            FunctionVal,
            GlobalVariableVal,
            ConstantInt32Val,
            ConstantFloatVal,
            ConstantArrayVal,
            ConstantPointerVal,

            // Value
            ArgumentVal,
            BasicBlockVal,

            // Instructions must be the last, in order to have a unique ID for each instruction
            InstructionVal,
            // 不准在InstructionVal后加东西
        };

        virtual ~Value() = default;

        BasicType *type = BasicType::getErrorType();

        List<Use *> useList;

        std::string name = "";

        int number = 0;

        const unsigned int subClassID = 0;

        std::vector<Use *> getVectorUses();

        Value(BasicType *type, const unsigned int subID) : type(type), subClassID(subID)
        {
            number = 0;
        };

        Value(std::string n, const unsigned int subID) : name(n), subClassID(subID)
        {
            number = 0;
        };

        Value(BasicType *ty, std::string n, const unsigned int subID) : type(ty), name(n), subClassID(subID)
        {
            number = 0;
        };

        void addUsage(Use *use);

        std::vector<User *> getUsers();

        virtual void emitUse(std::ostream &os);

        virtual void setInitializer(Constant *init) { Error::Error("Value", "setInitializer"); }

        void setType(BasicType *type) { this->type = type; }

        BasicType *getType() { return type; }

        std::string getTypeName() { return type->getTypeName(); }

        std::string getOperandName() { return "@" + name; }

        virtual std::string getIRName() const { return "@" + name; }

        virtual void emitIR(std::ostream &os) { os << getIRName() << std::endl; }

        virtual void waste();

        static int totalNumber;

        static void resetTotalNumber() { totalNumber = 0; }

        static Value* createTemp(BasicType* type);

        // 将自己改为另一个User
        // 需要将所有使用自己的User的use的val改为newUser？
        void replaceAllUsageTo(Value *newUser);

        unsigned int getValueID() const { return subClassID; }

        HANDLE_TYPECHEK_CREATE(Temporary)
        HANDLE_TYPECHEK_CREATE(Function)
        HANDLE_TYPECHEK_CREATE(GlobalVariable)
        HANDLE_TYPECHEK_CREATE(ConstantFloat)
        HANDLE_TYPECHEK_CREATE(ConstantInt32)
        HANDLE_TYPECHEK_CREATE(ConstantArray)
        HANDLE_TYPECHEK_CREATE(ConstantPointer)
        HANDLE_TYPECHEK_CREATE(Argument)
        HANDLE_TYPECHEK_CREATE(BasicBlock)
        bool isInstruction() const
        {
            return getValueID() >= ValueTy::InstructionVal;
        }
    };
}

#undef HANDLE_TYPECHEK_CREATE