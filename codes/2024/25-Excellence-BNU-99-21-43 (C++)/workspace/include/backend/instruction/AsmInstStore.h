#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstStore : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            SD,  // Store Doubleword(64-bit)
            SW,  // Store Word(32-bit)
            FSD, // Store Doubleword(64-bit) with floating-point
            FSW  // Store Word(32-bit) with floating-point
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::SD:
                return "sd";
            case Opcode::SW:
                return "sw";
            case Opcode::FSD:
                return "fsd";
            case Opcode::FSW:
                return "fsw";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstStore(AsmOperandRegister *src, AsmOperandStackVariable *dst) : AsmInstBasic(OpcodeBasic::AsmInstStore, src, dst, nullptr)
        {
            if (!ImmediateWithin12Bits(dst->getOffset()))
                Error::Error(__PRETTY_FUNCTION__, "Offset out of range");
            if (src->isRegisterInt())
            {
                if (dst->getSize() == 8)
                    opcode = Opcode::SD;
                else if (dst->getSize() == 4)
                    opcode = Opcode::SW;
                else
                    Error::Error(__PRETTY_FUNCTION__, "Invalid stack variable size");
            }
            else if (src->isRegisterFloat())
            {
                if (dst->getSize() == 8)
                    opcode = Opcode::FSD;
                else if (dst->getSize() == 4)
                    opcode = Opcode::FSW;
                else
                    Error::Error(__PRETTY_FUNCTION__, "Invalid stack variable size");
            }
            else
            {
                Error::Error(__PRETTY_FUNCTION__, "Invalid source operand type");
            }
        }

        AsmInstStore(AsmOperandRegister *src, AsmOperandMemoryAddress *dst, int bitLength) : AsmInstBasic(OpcodeBasic::AsmInstStore, src, dst, nullptr)
        {
            if (!ImmediateWithin12Bits(dst->getOffset()))
                Error::Error(__PRETTY_FUNCTION__, "Offset out of range");
            if (src->isRegisterInt())
            {
                if (bitLength == 64)
                    opcode = Opcode::SD;
                else if (bitLength == 32)
                    opcode = Opcode::SW;
                else
                    Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");
            }
            else if (src->isRegisterFloat())
            {
                if (bitLength == 64)
                    opcode = Opcode::FSD;
                else if (bitLength == 32)
                    opcode = Opcode::FSW;
                else
                    Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");
            }
            else
            {
                Error::Error(__PRETTY_FUNCTION__, "Invalid source operand type");
            }
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstStore(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {}; }

        std::set<AsmOperandRegister *> getUse() override
        {
            AsmOperandBasic *src = getOperand(1);
            if (src->isMemoryAddress())
                return {dynamic_cast<AsmOperandRegister *>(getOperand(0)), dynamic_cast<AsmOperandRegister *>(dynamic_cast<AsmOperandMemoryAddress *>(src)->getBaseRegister())};
            else
                return {dynamic_cast<AsmOperandRegister *>(getOperand(0)), dynamic_cast<AsmOperandRegister *>(dynamic_cast<AsmOperandStackVariable *>(src)->getAddress()->getBaseRegister())};
        }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return true; }
    };
}