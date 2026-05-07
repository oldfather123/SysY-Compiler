#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstConvertFloatInt : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            FCVT_W_S, // Convert float to int
            FCVT_S_W  // Convert int to float
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::FCVT_W_S:
                return "fcvt.w.s";
            case Opcode::FCVT_S_W:
                return "fcvt.s.w";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstConvertFloatInt(AsmOperandRegisterInt *dest, AsmOperandRegisterFloat *source)
            : AsmInstBasic(OpcodeBasic::AsmInstConvertFloatInt, dest, source, nullptr), opcode(Opcode::FCVT_W_S) {}

        AsmInstConvertFloatInt(AsmOperandRegisterFloat *dest, AsmOperandRegisterInt *source)
            : AsmInstBasic(OpcodeBasic::AsmInstConvertFloatInt, dest, source, nullptr), opcode(Opcode::FCVT_S_W) {}

        std::string toString() const override
        {
            return "AsmInstConvertFloatInt(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ")";
        }

        std::string emit() const override
        {
            if (opcode == Opcode::FCVT_W_S)
            {
                return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", rtz\n";
            }
            else // Opcode::FCVT_S_W
            {
                return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + "\n";
            }
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1))}; }

        bool mayHaveSideEffect() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayNotReturn() override { return false; }
    };
} // namespace Backend
