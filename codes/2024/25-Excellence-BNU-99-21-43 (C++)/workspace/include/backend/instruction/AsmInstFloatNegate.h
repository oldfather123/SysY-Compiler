#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstFloatNegate : public AsmInstBasic
    {
    public:
        AsmInstFloatNegate(AsmOperandRegisterFloat *dst, AsmOperandRegisterFloat *src) : AsmInstBasic(OpcodeBasic::AsmInstFloatNegate, dst, src, nullptr) {}

        std::string emit() const override
        {
            return "\tfneg.s " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstFloatNegate(" + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ")";
        }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1))}; }

        bool mayHaveSideEffect() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayNotReturn() override { return false; }
    };
} // namespace Backend
