#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstSignedIntDivide : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            DIVW, // integer division (32-bit)
            DIV,  // integer division (64-bit)
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::DIVW:
                return "divw";
            case Opcode::DIV:
                return "div";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstSignedIntDivide(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandRegisterInt *src2, int bitLength)
            : AsmInstBasic(OpcodeBasic::AsmInstSignedIntDivide, dst, src1, src2)
        {
            if (bitLength == 32)
                opcode = Opcode::DIVW;
            else if (bitLength == 64)
                opcode = Opcode::DIV;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstSignedIntDivide(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))}; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
} // namespace Backend
