#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstMul : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            MULW,   // integer multiplication (32-bit)
            MUL,    // integer multiplication (64-bit)
            FMUL_S, // single-precision floating-point multiplication
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::MULW:
                return "mulw";
            case Opcode::MUL:
                return "mul";
            case Opcode::FMUL_S:
                return "fmul.s";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstMul(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandRegisterInt *src2, int bitLength)
            : AsmInstBasic(OpcodeBasic::AsmInstMul, dst, src1, src2)
        {
            if (bitLength == 32)
                opcode = Opcode::MULW;
            else if (bitLength == 64)
                opcode = Opcode::MUL;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");
        }

        AsmInstMul(AsmOperandRegisterFloat *dst, AsmOperandRegisterFloat *src1, AsmOperandRegisterFloat *src2)
            : AsmInstBasic(OpcodeBasic::AsmInstMul, dst, src1, src2), opcode(Opcode::FMUL_S) {}

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstMul(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))}; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
} // namespace Backend
