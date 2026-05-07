#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstShiftLeftAdd : public AsmInstBasic
    {
    private:
        int n; // Shift length

    public:
        AsmInstShiftLeftAdd(int n, AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
            : AsmInstBasic(OpcodeBasic::AsmInstShiftLeftAdd, dest, source1, source2)
        {
            if (n < 1 || n > 3)
                throw std::invalid_argument("Invalid shift length");
            this->n = n;
        }

        int getShiftLength() const
        {
            return n;
        }

        std::string toString() const override
        {
            return "AsmShiftLeftAdd(" + std::to_string(n) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        std::string emit() const override
        {
            return "\tsh" + std::to_string(n) + "add " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))}; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
} // namespace Backend
