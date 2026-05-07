#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstShiftRightArithmetic : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            SRA,  // Arithmetic shift right (64-bit)
            SRAW, // Arithmetic shift right (32-bit)
            SRAI, // Arithmetic shift right immediate (64-bit)
            SRAIW // Arithmetic shift right immediate (32-bit)
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::SRA:
                return "sra";
            case Opcode::SRAW:
                return "sraw";
            case Opcode::SRAI:
                return "srai";
            case Opcode::SRAIW:
                return "sraiw";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstShiftRightArithmetic(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandBasic *src2, int bitLength)
            : AsmInstBasic(OpcodeBasic::AsmInstShiftRightArithmetic, dst, src1, src2)
        {
            if (bitLength != 32 && bitLength != 64)
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");

            if (src2->isRegisterInt())
                opcode = bitLength == 32 ? Opcode::SRAW : Opcode::SRA;
            else if (src2->isImmediate())
                opcode = bitLength == 32 ? Opcode::SRAIW : Opcode::SRAI;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid operand type");
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstShiftRightArithmetic(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override
        {
            switch (opcode)
            {
            case Opcode::SRA:
            case Opcode::SRAW:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))};
            case Opcode::SRAI:
            case Opcode::SRAIW:
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
