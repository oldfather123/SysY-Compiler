#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstAnd : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            AND, // Bitwise AND
            ANDI // Bitwise AND with immediate
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::AND:
                return "and";
            case Opcode::ANDI:
                return "andi";
            default:
                return "";
            }
        }

    private:
        Opcode opcode;

    public:
        AsmInstAnd(AsmOperandRegisterInt *dst, AsmOperandRegisterInt *src1, AsmOperandBasic *src2)
            : AsmInstBasic(OpcodeBasic::AsmInstAnd, dst, src1, src2)
        {
            if (dynamic_cast<AsmOperandRegisterInt *>(src2))
                opcode = Opcode::AND;
            else if (dynamic_cast<AsmOperandImmediate *>(src2))
                opcode = Opcode::ANDI;
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid operand type");
        }

        Opcode getOpcode() const
        {
            return opcode;
        }

        std::string toString() const override
        {
            return "AsmInstAnd(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        static AsmInstAnd *createAnd(AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            return new AsmInstAnd(dest, source1, source2);
        }

        static AsmInstAnd *createAndi(AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source1, AsmOperandImmediate *source2)
        {
            return new AsmInstAnd(dest, source1, source2);
        }

        std::set<AsmOperandRegister *> getDef() override
        {
            return {dynamic_cast<AsmOperandRegister *>(getOperand(0))};
        }

        std::set<AsmOperandRegister *> getUse() override
        {
            switch (opcode)
            {
            case Opcode::AND:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))};
            case Opcode::ANDI:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1))};
            default:
                return {};
            }
        }

        bool mayHaveSideEffect() override { return false; }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }
    };
} // namespace Backend
