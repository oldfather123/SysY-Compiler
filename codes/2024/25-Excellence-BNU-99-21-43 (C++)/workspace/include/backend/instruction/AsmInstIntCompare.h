#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    class AsmInstIntCompare : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            SEQZ, // Set if Equal to Zero
            SNEZ, // Set if Not Equal to Zero
            SLT,  // Set if Less Than
            SLTI, // Set if Less Than Immediate
            SGT,  // Set if Greater Than
            SGTI  // Set if Greater Than Immediate
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::SEQZ:
                return "seqz";
            case Opcode::SNEZ:
                return "snez";
            case Opcode::SLT:
                return "slt";
            case Opcode::SLTI:
                return "slti";
            case Opcode::SGT:
                return "sgt";
            case Opcode::SGTI:
                return "sgti";
            default:
                return "";
            }
        }

        enum class Condition
        {
            EQZ,
            NEZ,
            LT,
            GT
        };

    private:
        Opcode opcode;
        Condition condition;

    public:
        AsmInstIntCompare(Opcode opcode, Condition condition, AsmOperandRegisterInt *dst, AsmOperandBasic *src1, AsmOperandBasic *src2)
            : AsmInstBasic(OpcodeBasic::AsmInstIntCompare, dst, src1, src2), opcode(opcode), condition(condition) {}

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
            switch (opcode)
            {
            case Opcode::SEQZ:
            case Opcode::SNEZ:
                return "AsmInstIntCompare(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ")";
            case Opcode::SLT:
            case Opcode::SLTI:
                return "AsmInstIntCompare(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
            case Opcode::SGT:
            case Opcode::SGTI:
                return "AsmInstIntCompare(" + opcodeToString(opcode) + ", " + getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString() + ")";
            default:
                return "";
            }
        }

        std::string emit() const override
        {
            switch (opcode)
            {
            case Opcode::SEQZ:
            case Opcode::SNEZ:
                return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + "\n";
            case Opcode::SLT:
            case Opcode::SLTI:
                return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
            case Opcode::SGT:
            case Opcode::SGTI:
                return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
            default:
                return "";
            }
        }

        static AsmInstIntCompare *createEQZ(AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source)
        {
            return new AsmInstIntCompare(
                Opcode::SEQZ,
                Condition::EQZ,
                dest,
                source,
                nullptr);
        }

        static AsmInstIntCompare *createNEZ(AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source)
        {
            return new AsmInstIntCompare(
                Opcode::SNEZ,
                Condition::NEZ,
                dest,
                source,
                nullptr);
        }

        static AsmInstIntCompare *createLT(AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            return new AsmInstIntCompare(
                Opcode::SLT,
                Condition::LT,
                dest,
                source1,
                source2);
        }

        static AsmInstIntCompare *createLT(AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source1, AsmOperandImmediate *source2)
        {
            return new AsmInstIntCompare(
                Opcode::SLTI,
                Condition::LT,
                dest,
                source1,
                source2);
        }

        static AsmInstIntCompare *createGT(AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            return new AsmInstIntCompare(
                Opcode::SGT,
                Condition::GT,
                dest,
                source1,
                source2);
        }

        static AsmInstIntCompare *createGT(AsmOperandRegisterInt *dest, AsmOperandRegisterInt *source1, AsmOperandImmediate *source2)
        {
            return new AsmInstIntCompare(
                Opcode::SGTI,
                Condition::GT,
                dest,
                source1,
                source2);
        }

        std::set<AsmOperandRegister *> getDef() override { return {dynamic_cast<AsmOperandRegister *>(getOperand(0))}; }

        std::set<AsmOperandRegister *> getUse() override
        {
            switch (opcode)
            {
            case Opcode::SEQZ:
            case Opcode::SNEZ:
            case Opcode::SLTI:
            case Opcode::SGTI:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1))};
            case Opcode::SLT:
            case Opcode::SGT:
                return {dynamic_cast<AsmOperandRegister *>(getOperand(1)), dynamic_cast<AsmOperandRegister *>(getOperand(2))};
            default:
                return {};
            }
        }

        bool mayHaveSideEffect() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayNotReturn() override { return false; }
    };
} // namespace Backend
