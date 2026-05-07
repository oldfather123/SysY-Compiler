#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstShiftLeft : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            SLL,  // Shift left logical (64-bit)
            SLLI, // Shift left logical immediate (64-bit)
            SLLW, // Shift left logical (32-bit)
            SLLIW // Shift left logical immediate (32-bit)
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::SLL:
                return "sll";
            case Opcode::SLLI:
                return "slli";
            case Opcode::SLLW:
                return "sllw";
            case Opcode::SLLIW:
                return "slliw";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstShiftLeft(AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source1, AsmOperandBasic *source2, int bitLength)
            : AsmInstBasic(OpcodeBasic::AsmInstShiftLeft, dest, source1, source2)
        {
            if (bitLength != 32 && bitLength != 64)
                throw std::invalid_argument("Invalid bit length");

            if (!source2->isRegisterInt() && !source2->isImmediate())
                throw std::invalid_argument("Invalid operand type");

            if (bitLength == 64)
            {
                opcode = source2->isImmediate() ? Opcode::SLLI : Opcode::SLL;
            }
            else
            {
                opcode = source2->isImmediate() ? Opcode::SLLIW : Opcode::SLLW;
            }
        }

        std::string toString() const override
        {
            return "AsmInstShiftLeft(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override
        {
            switch (opcode)
            {
            case Opcode::SLL:
            case Opcode::SLLW:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))};
            case Opcode::SLLI:
            case Opcode::SLLIW:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1))};
            default:
                throw std::invalid_argument("Invalid opcode");
            }
        }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
} // namespace Backend
