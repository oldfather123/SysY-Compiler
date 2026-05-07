#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstFloatCompare : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            FEQ_S, // set if equal
            FLT_S, // set if less than
            FLE_S  // set if less than or equal
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::FEQ_S:
                return "feq.s";
            case Opcode::FLT_S:
                return "flt.s";
            case Opcode::FLE_S:
                return "fle.s";
            default:
                return "";
            }
        }

        enum class Condition
        {
            EQ,
            LT,
            LE
        };

    private:
        Opcode opcode;
        Condition condition;

    public:
        AsmInstFloatCompare(Opcode opcode, Condition condition, AsmOperandRegisterInt *dst, AsmOperandRegisterFloat *src1, AsmOperandRegisterFloat *src2)
            : AsmInstBasic(OpcodeBasic::AsmInstFloatCompare, dst, src1, src2), opcode(opcode), condition(condition) {}

        Opcode getOpcode() const
        {
            return opcode;
        }

        Condition getCondition() const
        {
            return condition;
        }

        std::string toString() const override
        {
            return "AsmInstFloatCompare(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
        }

        std::string emit() const override
        {
            return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
        }

        static AsmInstFloatCompare *createEQ(AsmOperandRegisterInt *dest, AsmOperandRegisterFloat *source1, AsmOperandRegisterFloat *source2)
        {
            return new AsmInstFloatCompare(
                Opcode::FEQ_S,
                Condition::EQ,
                dest,
                source1,
                source2);
        }

        static AsmInstFloatCompare *createLT(AsmOperandRegisterInt *dest, AsmOperandRegisterFloat *source1, AsmOperandRegisterFloat *source2)
        {
            return new AsmInstFloatCompare(
                Opcode::FLT_S,
                Condition::LT,
                dest,
                source1,
                source2);
        }

        static AsmInstFloatCompare *createLE(AsmOperandRegisterInt *dest, AsmOperandRegisterFloat *source1, AsmOperandRegisterFloat *source2)
        {
            return new AsmInstFloatCompare(
                Opcode::FLE_S,
                Condition::LE,
                dest,
                source1,
                source2);
        }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))}; }

        bool mayHaveSideEffect() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayNotReturn() override { return false; }
    };
} // namespace Backend
