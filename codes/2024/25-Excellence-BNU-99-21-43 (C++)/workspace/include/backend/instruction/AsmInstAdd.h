#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstAdd : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            ADD,    // integer addition (64-bit)
            ADDW,   // integer addition (32-bit)
            ADDI,   // integer addition (immediate, 64-bit)
            ADDIW,  // integer addition (immediate, 32-bit)
            FADD_S, // single-precision floating-point addition
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::ADD:
                return "add";
            case Opcode::ADDW:
                return "addw";
            case Opcode::ADDI:
                return "addi";
            case Opcode::ADDIW:
                return "addiw";
            case Opcode::FADD_S:
                return "fadd.s";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstAdd(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1,
                   AsmOperandBasic *src2, int bitLength) : AsmInstBasic(OpcodeBasic::AsmInstAdd, dst, src1, src2)
        {
            if (bitLength != 32 && bitLength != 64)
                Error::Error(__PRETTY_FUNCTION__, "Invalid bit length");

            if (src2->isRegisterInt())
            {
                opcode = bitLength == 32 ? Opcode::ADDW : Opcode::ADD;
            }
            else if (src2->isImmediate() || src2->isMemoryAddress() || src2->isLabel())
            {
                if (src2->isImmediate() && !ImmediateWithin12Bits(dynamic_cast<AsmOperandImmediate *>(src2)->getValue()))
                    Error::Error(__PRETTY_FUNCTION__, "Immediate value out of range");
                opcode = bitLength == 32 ? Opcode::ADDIW : Opcode::ADDI;
            }
            else
            {
                Error::Error(__PRETTY_FUNCTION__, "Invalid operand type");
            }
        }

        AsmInstAdd(AsmOperandRegisterFloat *dst, AsmOperandRegisterFloat *src1, AsmOperandRegisterFloat *src2)
            : AsmInstBasic(OpcodeBasic::AsmInstAdd, dst, src1, src2), opcode(Opcode::FADD_S) {}

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        std::string toString() const override
        {
            return "AsmInstAdd(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        Opcode getOpcode() const { return opcode; }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override
        {
            switch (opcode)
            {
            case Opcode::ADD:
            case Opcode::ADDW:
            case Opcode::FADD_S:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))};
            case Opcode::ADDI:
            case Opcode::ADDIW:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1))};
            default:
                return {};
            }
        }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return false; }
    };
} // namespace Backend