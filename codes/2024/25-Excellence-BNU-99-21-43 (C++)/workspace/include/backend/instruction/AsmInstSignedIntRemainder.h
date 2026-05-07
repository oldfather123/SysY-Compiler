#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstSignedIntRemainder : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            REM, // Signed integer remainder (64-bit)
            REMW // Signed integer remainder (32-bit)
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::REM:
                return "rem";
            case Opcode::REMW:
                return "remw";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstSignedIntRemainder(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandRegisterInt *src2, int bitLength)
            : AsmInstBasic(OpcodeBasic::AsmInstSignedIntRemainder, dst, src1, src2)
        {
            if (bitLength != 32 && bitLength != 64)
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");

            opcode = bitLength == 32 ? Opcode::REMW : Opcode::REM;
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstSignedIntRemainder(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))}; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
}