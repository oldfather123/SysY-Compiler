#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstMove : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            MV,      // move integer register
            FMV_S,   // move single-precision floating-point register
            FMV_S_X, // move from integer register to single-precision floating-point register
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::MV:
                return "mv";
            case Opcode::FMV_S:
                return "fmv.s";
            case Opcode::FMV_S_X:
                return "fmv.s.x";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstMove(AsmOperandRegister *dst, AsmOperandBasic *src) : AsmInstBasic(OpcodeBasic::AsmInstMove, dst, src, nullptr)
        {
            if (dst->isRegisterInt() && src->isRegisterInt())
                opcode = Opcode::MV;
            else if (dst->isRegisterFloat() && src->isRegisterFloat())
                opcode = Opcode::FMV_S;
            else if (dst->isRegisterFloat() && src->isRegisterInt())
                opcode = Opcode::FMV_S_X;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid operand type");
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstMove(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1))}; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
}