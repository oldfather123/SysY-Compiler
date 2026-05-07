#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstLoad : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            LD, // load (64-bit)
            LW, // load (32-bit)

            FLD, // load (floating-point, 64-bit)
            FLW, // load (floating-point, 32-bit)

            LI,  // load immediate (64-bit)
            LUI, // load upper immediate (64-bit, 20-bit immediate)
            LA,  // load address (64-bit)
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::LD:
                return "ld";
            case Opcode::LW:
                return "lw";
            case Opcode::FLD:
                return "fld";
            case Opcode::FLW:
                return "flw";
            case Opcode::LI:
                return "li";
            case Opcode::LUI: // not implemented
                return "lui";
            case Opcode::LA:
                return "la";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstLoad(AsmOperandRegister *dst, AsmOperandBasic *src) : AsmInstBasic(OpcodeBasic::AsmInstLoad, dst, src, nullptr)
        {
            if (dst->isRegisterInt())
            {
                if (src->isStackVariable())
                {
                    int size = dynamic_cast<AsmOperandStackVariable *>(src)->getSize();
                    if (size == 4)
                        opcode = Opcode::LW;
                    else if (size == 8)
                        opcode = Opcode::LD;
                    else
                    {
                        // opcode = Opcode::LD;
                        Error::Error(__PRETTY_FUNCTION__, "Invalid stack variable size in [src->isStackVariable()] branch");
                    }
                }
                else if (src->isLabel())
                    opcode = Opcode::LA;
                else if (src->isImmediate())
                    opcode = Opcode::LI;
                else
                    Error::Error(__PRETTY_FUNCTION__, "Invalid source operand type in [dst->isRegisterInt()] branch");
            }
            else if (dst->isRegisterFloat())
            {
                if (src->isStackVariable())
                {
                    int size = dynamic_cast<AsmOperandStackVariable *>(src)->getSize();
                    if (size == 8)
                        opcode = Opcode::FLD;
                    else if (size == 4)
                        opcode = Opcode::FLW;
                    else
                        Error::Error(__PRETTY_FUNCTION__, "Invalid stack variable size in [dst->isRegisterFloat()] branch");
                }
                else
                    Error::Error(__PRETTY_FUNCTION__, "Invalid source operand type in [dst->isRegisterFloat()] branch, src->getType() = "  + AsmOperandBasic::operandTypeToString(src->getType()));
            }
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid destination operand type");
        }

        AsmInstLoad(AsmOperandRegister *dst, AsmOperandMemoryAddress *src, int bitLength) : AsmInstBasic(OpcodeBasic::AsmInstLoad, dst, src, nullptr)
        {
            if (bitLength != 32 && bitLength != 64)
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");

            if (dst->isRegisterInt())
                opcode = bitLength == 32 ? Opcode::LW : Opcode::LD;
            else if (dst->isRegisterFloat())
                opcode = bitLength == 32 ? Opcode::FLW : Opcode::FLD;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid destination operand type");
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstLoad(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ")";
        }

        Opcode getOpcode() const
        {
            return opcode;
        }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override
        {
            AsmOperandBasic *src = getOperand(1);

            if (src->isMemoryAddress())
                return {dynamic_cast<AsmOperandMemoryAddress *>(src)->getBaseRegister()};
            else if (src->isStackVariable())
                return {dynamic_cast<AsmOperandStackVariable *>(src)->getAddress()->getBaseRegister()};
            else
                return {};
        }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
}