#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstShiftRightLogical : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            SRL,  // Logical shift right (64-bit)
            SRLW, // Logical shift right (32-bit)
            SRLI, // Logical shift right immediate (64-bit)
            SRLIW // Logical shift right immediate (32-bit)
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::SRL:
                return "srl";
            case Opcode::SRLW:
                return "srlw";
            case Opcode::SRLI:
                return "srli";
            case Opcode::SRLIW:
                return "srliw";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstShiftRightLogical(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandBasic *src2, int bitLength)
            : AsmInstBasic(OpcodeBasic::AsmInstShiftRightLogical, dst, src1, src2)
        {
            if (bitLength != 32 && bitLength != 64)
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");

            if (src2->isRegisterInt())
                opcode = bitLength == 32 ? Opcode::SRLW : Opcode::SRL;
            else if (src2->isImmediate())
                opcode = bitLength == 32 ? Opcode::SRLIW : Opcode::SRLI;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid operand type");
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstShiftRightLogic(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override
        {
            switch (opcode)
            {
            case Opcode::SRL:
            case Opcode::SRLW:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))};
            case Opcode::SRLI:
            case Opcode::SRLIW:
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
