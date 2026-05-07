#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstLabel : public AsmInstBasic
    {
    private:
        AsmOperandLabel *label;

    public:
        AsmInstLabel(AsmOperandLabel *label) : AsmInstBasic(OpcodeBasic::AsmInstLabel, nullptr, nullptr, nullptr), label(label) {}

        AsmOperandLabel *getAsmOperandLabel() const
        {
            return label;
        }

        std::string emit() const override
        {
            return label->emit() + ":\n";
        }

        std::string toString() const override
        {
            return "AsmInstLabel( " + label->toString() + " )";
        }

        std::string getLabelName() const
        {
            return label->toString();
        }

        std::set<AsmOperandRegister *> getDef() override { throw std::invalid_argument("Invalid operation"); }

        std::set<AsmOperandRegister *> getUse() override { throw std::invalid_argument("Invalid operation"); }

        bool mayNotReturn() override { throw std::invalid_argument("Invalid operation"); }

        bool willNeverReturn() override { throw std::invalid_argument("Invalid operation"); }

        bool mayHaveSideEffect() override { throw std::invalid_argument("Invalid operation"); }
    };
}