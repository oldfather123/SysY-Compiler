#pragma once
#include "AsmOperandBasic.h"
#include "AsmRegisterInterface.h"
namespace Backend
{
    class AsmOperandRegister : public AsmOperandBasic, public AsmRegisterInterface
    {
    private:
        int regIndex; // < 0 for virtual registers
        bool isFloat;

    public:
        AsmOperandRegister(int regIndex, bool isFloat) : AsmOperandBasic(isFloat ? OperandType::REGISTER_FLOAT : OperandType::REGISTER_INT), regIndex(regIndex), isFloat(isFloat) {}
        virtual std::string emit() const override = 0;
        virtual std::string toString() const override = 0;
        int getRegIndex() const { return regIndex; }
        bool isVirtualRegister() const { return regIndex < 0; }
        bool isVirtual() const { return regIndex < 0; }
        int getAbsRegIndex() const { return regIndex < 0 ? -regIndex : regIndex; }
        int getAbsoluteIndex() const { return regIndex < 0 ? -regIndex : regIndex; }
        bool operator==(const AsmOperandRegister &other) const { return regIndex == other.regIndex && isFloat == other.isFloat; }
        bool operator!=(const AsmOperandRegister &other) const { return !(*this == other); }
        bool operator<(const AsmOperandRegister &other) const { return regIndex < other.regIndex || (regIndex == other.regIndex && isFloat < other.isFloat); }
        AsmOperandRegister *getRegister() const override { return const_cast<AsmOperandRegister *>(this); }
        AsmOperandBasic *withRegister(AsmOperandRegister *register_) const override { return register_; }
    };

    struct AsmOperandRegisterCompare
    {
        bool operator()(const AsmOperandRegister *lhs, const AsmOperandRegister *rhs) const
        {
            if (lhs->isRegisterInt() != rhs->isRegisterInt())
                return lhs->isRegisterInt();
            return lhs->getRegIndex() < rhs->getRegIndex();
        }
    };
}