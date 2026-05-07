#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstReturn : public AsmInstBasic
    {
    public:
        AsmInstReturn() : AsmInstBasic(OpcodeBasic::AsmInstReturn, nullptr, nullptr, nullptr) {}

        std::string emit() const override
        {
            return "\tret\n";
        }

        std::string toString() const override
        {
            return "AsmInstReturn()";
        }

        std::set<AsmOperandRegister *> getDef() override { return {AsmOperandRegisterInt::getPhysical(10), AsmOperandRegisterFloat::getPhysical(10)}; }

        std::set<AsmOperandRegister *> getUse() override { return {}; }

        bool mayNotReturn() override { return true; }

        bool willNeverReturn() override { return true; }

        bool mayHaveSideEffect() override { return false; }
    };
}