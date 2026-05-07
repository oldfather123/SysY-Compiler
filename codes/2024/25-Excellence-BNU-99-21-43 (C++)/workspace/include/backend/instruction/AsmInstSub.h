#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstSub : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            SUBW,   // integer subtraction (32-bit)
            SUB,    // integer subtraction (64-bit)
            FSUB_S, // single-precision floating-point subtraction
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::SUBW:
                return "subw";
            case Opcode::SUB:
                return "sub";
            case Opcode::FSUB_S:
                return "fsub.s";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstSub(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandRegisterInt *src2, int bitLength)
            : AsmInstBasic(OpcodeBasic::AsmInstSub, dst, src1, src2)
        {
            if (bitLength == 32)
                opcode = Opcode::SUBW;
            else if (bitLength == 64)
                opcode = Opcode::SUB;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");
        }

        AsmInstSub(AsmOperandRegisterFloat *dst, AsmOperandRegisterFloat *src1, AsmOperandRegisterFloat *src2)
            : AsmInstBasic(OpcodeBasic::AsmInstSub, dst, src1, src2), opcode(Opcode::FSUB_S) {}

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstSub(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))}; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
}