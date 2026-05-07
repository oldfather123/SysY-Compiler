#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstNop : public AsmInstBasic
    {
    public:
        AsmInstNop()
            : AsmInstBasic(OpcodeBasic::AsmInstNop, nullptr, nullptr, nullptr) {}

        std::string emit() const override
        {
            return "\tnop\n";
        }

        std::string toString() const override
        {
            return "AsmInstNop()";
        }

        std::set<AsmOperandRegister *> getDef() override { return {}; }

        std::set<AsmOperandRegister *> getUse() override { return {}; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
} // namespace Backend
