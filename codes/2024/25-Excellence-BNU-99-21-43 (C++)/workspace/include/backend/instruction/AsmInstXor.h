#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstXor : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            XOR,  // Bitwise XOR (64-bit)
            XORW, // Bitwise XOR (32-bit)
            XORI, // Bitwise XOR with immediate (64-bit)
            XORIW // Bitwise XOR with immediate (32-bit)
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::XOR:
                return "xor";
            case Opcode::XORW:
                return "xorw";
            case Opcode::XORI:
                return "xori";
            case Opcode::XORIW:
                return "xoriw";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstXor(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandBasic *src2, int bitLength)
            : AsmInstBasic(OpcodeBasic::AsmInstXor, dst, src1, src2)
        {
            if (bitLength != 32 && bitLength != 64)
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");

            if (src2->isRegisterInt())
                opcode = bitLength == 32 ? Opcode::XORW : Opcode::XOR;
            else if (src2->isImmediate())
                opcode = bitLength == 32 ? Opcode::XORIW : Opcode::XORI;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid operand type");
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstXor(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override
        {
            switch (opcode)
            {
            case Opcode::XOR:
            case Opcode::XORW:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))};
            case Opcode::XORI:
            case Opcode::XORIW:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1))};
            default:
                Error::Error(__PRETTY_FUNCTION__, "Invalid opcode");
                return {};
            }
        }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
} // namespace Backend
