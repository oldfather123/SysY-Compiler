#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstOr : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            OR,  // Bitwise OR (64-bit)
            ORW, // Bitwise OR (32-bit)
            ORI, // Bitwise OR with immediate (64-bit)
            ORIW // Bitwise OR with immediate (32-bit)
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::OR:
                return "or";
            case Opcode::ORW:
                return "orw";
            case Opcode::ORI:
                return "ori";
            case Opcode::ORIW:
                return "oriw";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstOr(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandBasic *src2, int bitLength)
            : AsmInstBasic(OpcodeBasic::AsmInstOr, dst, src1, src2)
        {
            if (bitLength != 32 && bitLength != 64)
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");

            if (src2->isRegisterInt())
                opcode = bitLength == 32 ? Opcode::ORW : Opcode::OR;
            else if (src2->isImmediate())
                opcode = bitLength == 32 ? Opcode::ORIW : Opcode::ORI;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid operand type");
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstOr(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))}; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
} // namespace Backend
