#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstMax : public AsmInstBasic
    {
    public:
        AsmInstMax(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandRegisterInt *src2)
            : AsmInstBasic(OpcodeBasic::AsmInstMax, dst, src1, src2) {}

        std::string emit() const override
        {
            return "\tmax " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstMax(" + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))}; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
} // namespace Backend
