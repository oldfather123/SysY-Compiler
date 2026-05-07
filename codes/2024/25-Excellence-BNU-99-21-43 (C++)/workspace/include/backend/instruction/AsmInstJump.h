#pragma once
#include "AsmInstBasic.h"
#include <vector>
#include <string>
#include <set>

namespace Backend
{
    class AsmInstJump : public AsmInstBasic
    {
    public:
        enum class Opcode
        {
            J,    // unconditional jump (j label)
            BLTZ, // branch if less than zero (bltz rs, label)
            BGTZ, // branch if greater than zero (bgtz rs, label)
            BLEZ, // branch if less than or equal to zero (blez rs, label)
            BGEZ, // branch if greater than or equal to zero (bgez rs, label)
            BEQZ, // branch if equal to zero (beqz rs, label)
            BNEZ, // branch if not equal to zero (bnez rs, label)
            BLT,  // branch if less than (blt rs, rt, label)
            BGT,  // branch if greater than (bgt rs, rt, label)
            BLE,  // branch if less than or equal to (ble rs, rt, label)
            BGE,  // branch if greater than or equal to (bge rs, rt, label)
            BEQ,  // branch if equal (beq rs, rt, label)
            BNE,  // branch if not equal (bne rs, rt, label)
        };

        static std::string opcodeToString(Opcode opcode)
        {
            switch (opcode)
            {
            case Opcode::J:
                return "j";
            case Opcode::BLTZ:
                return "bltz";
            case Opcode::BGTZ:
                return "bgtz";
            case Opcode::BLEZ:
                return "blez";
            case Opcode::BGEZ:
                return "bgez";
            case Opcode::BEQZ:
                return "beqz";
            case Opcode::BNEZ:
                return "bnez";
            case Opcode::BLT:
                return "blt";
            case Opcode::BGT:
                return "bgt";
            case Opcode::BLE:
                return "ble";
            case Opcode::BGE:
                return "bge";
            case Opcode::BEQ:
                return "beq";
            case Opcode::BNE:
                return "bne";
            default:
                return "";
            }
        }

        enum class Condition
        {
            UNCONDITIONAL,
            LTZ,
            GTZ,
            LEZ,
            GEZ,
            EQZ,
            NEZ,
            LT,
            GT,
            LE,
            GE,
            EQ,
            NE,
            DEFAULT,
        };

    private:
        Opcode opcode;
        Condition condition;

    public:
        AsmInstJump(Opcode opcode, Condition condition, AsmOperandBasic *operand0, AsmOperandBasic *operand1 = nullptr, AsmOperandBasic *operand2 = nullptr)
            : AsmInstBasic(OpcodeBasic::AsmInstJump, operand0, operand1, operand2), opcode(opcode), condition(condition) {}

        std::string emit() const override
        {
            switch (opcode)
            {
            case Opcode::J:
                return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + "\n";
            case Opcode::BLTZ:
            case Opcode::BGTZ:
            case Opcode::BLEZ:
            case Opcode::BGEZ:
            case Opcode::BEQZ:
            case Opcode::BNEZ:
                return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + "\n";
            case Opcode::BLT:
            case Opcode::BGT:
            case Opcode::BLE:
            case Opcode::BGE:
            case Opcode::BEQ:
            case Opcode::BNE:
                return "\t" + opcodeToString(opcode) + " " + getOperand(0)->emit() + ", " + getOperand(1)->emit() + ", " + getOperand(2)->emit() + "\n";
            default:
                return "";
            }
        }

        std::string toString() const override
        {
            std::string s = "AsmInstJump(" + opcodeToString(opcode) + ", ";
            switch (opcode)
            {
            case Opcode::J:
                s += getOperand(0)->toString();
                break;
            case Opcode::BLTZ:
            case Opcode::BGTZ:
            case Opcode::BLEZ:
            case Opcode::BGEZ:
            case Opcode::BEQZ:
            case Opcode::BNEZ:
                s += getOperand(0)->toString() + ", " + getOperand(1)->toString();
                break;
            case Opcode::BLT:
            case Opcode::BGT:
            case Opcode::BLE:
            case Opcode::BGE:
            case Opcode::BEQ:
            case Opcode::BNE:
                s += getOperand(0)->toString() + ", " + getOperand(1)->toString() + ", " + getOperand(2)->toString();
                break;
            default:
                return "";
            }
            s += ")";

            return s;
        }

        Opcode getOpcode() const
        {
            return opcode;
        }

        AsmOperandLabel *getTargetLabel() const
        {
            AsmOperandBasic *labelOperand = nullptr;
            switch (opcode)
            {
            case Opcode::J:
                labelOperand = getOperand(0);
                break;
            case Opcode::BLTZ:
            case Opcode::BGTZ:
            case Opcode::BLEZ:
            case Opcode::BGEZ:
            case Opcode::BEQZ:
            case Opcode::BNEZ:
                labelOperand = getOperand(1);
                break;
            case Opcode::BLT:
            case Opcode::BGT:
            case Opcode::BLE:
            case Opcode::BGE:
            case Opcode::BEQ:
            case Opcode::BNE:
                labelOperand = getOperand(2);
                break;
            default:
                break;
            }
            return dynamic_cast<AsmOperandLabel *>(labelOperand);
        }

        AsmOperandLabel *getAsmOperandLabel() const
        {
            return getTargetLabel();
        }

        static AsmInstJump *createUnconditional(AsmOperandLabel *targetLabel)
        {
            return new AsmInstJump(
                Opcode::J,
                Condition::UNCONDITIONAL,
                targetLabel);
        }

        static AsmInstJump *createLTZ(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source)
        {
            return new AsmInstJump(
                Opcode::BLTZ,
                Condition::LTZ,
                source,
                targetLabel);
        }

        static AsmInstJump *createGTZ(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source)
        {
            return new AsmInstJump(
                Opcode::BGTZ,
                Condition::GTZ,
                source,
                targetLabel);
        }

        static AsmInstJump *createLEZ(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source)
        {
            return new AsmInstJump(
                Opcode::BLEZ,
                Condition::LEZ,
                source,
                targetLabel);
        }

        static AsmInstJump *createGEZ(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source)
        {
            return new AsmInstJump(
                Opcode::BGEZ,
                Condition::GEZ,
                source,
                targetLabel);
        }

        static AsmInstJump *createEQZ(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source)
        {
            return new AsmInstJump(
                Opcode::BEQZ,
                Condition::EQZ,
                source,
                targetLabel);
        }

        static AsmInstJump *createNEZ(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source)
        {
            return new AsmInstJump(
                Opcode::BNEZ,
                Condition::NEZ,
                source,
                targetLabel);
        }

        static AsmInstJump *createLT(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            return new AsmInstJump(
                Opcode::BLT,
                Condition::LT,
                source1,
                source2,
                targetLabel);
        }

        static AsmInstJump *createGT(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            return new AsmInstJump(
                Opcode::BGT,
                Condition::GT,
                source1,
                source2,
                targetLabel);
        }

        static AsmInstJump *createLE(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            return new AsmInstJump(
                Opcode::BLE,
                Condition::LE,
                source1,
                source2,
                targetLabel);
        }

        static AsmInstJump *createGE(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            return new AsmInstJump(
                Opcode::BGE,
                Condition::GE,
                source1,
                source2,
                targetLabel);
        }

        static AsmInstJump *createEQ(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            return new AsmInstJump(
                Opcode::BEQ,
                Condition::EQ,
                source1,
                source2,
                targetLabel);
        }

        static AsmInstJump *createNE(AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            return new AsmInstJump(
                Opcode::BNE,
                Condition::NE,
                source1,
                source2,
                targetLabel);
        }

        static AsmInstJump *createUnary(Condition condition, AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source)
        {
            switch (condition)
            {
            case Condition::LTZ:
                return createLTZ(targetLabel, source);
            case Condition::GTZ:
                return createGTZ(targetLabel, source);
            case Condition::LEZ:
                return createLEZ(targetLabel, source);
            case Condition::GEZ:
                return createGEZ(targetLabel, source);
            case Condition::EQZ:
                return createEQZ(targetLabel, source);
            case Condition::NEZ:
                return createNEZ(targetLabel, source);
            default:
                throw std::invalid_argument("Invalid unary condition");
            }
        }

        static AsmInstJump *createBinary(Condition condition, AsmOperandLabel *targetLabel, AsmOperandRegisterInt *source1, AsmOperandRegisterInt *source2)
        {
            switch (condition)
            {  
            case Condition::LT:
                return createLT(targetLabel, source1, source2);
            case Condition::GT:
                return createGT(targetLabel, source1, source2);
            case Condition::LE:
                return createLE(targetLabel, source1, source2);
            case Condition::GE:
                return createGE(targetLabel, source1, source2);
            case Condition::EQ:
                return createEQ(targetLabel, source1, source2);
            case Condition::NE:
                return createNE(targetLabel, source1, source2);
            default:
                throw std::invalid_argument("Invalid binary condition");
            }
        }

        Condition getCondition() const
        {
            return condition;
        }

        std::set<AsmOperandRegister *> getDef() override { return {}; }

        std::set<AsmOperandRegister *> getUse() override
        {
            std::set<AsmOperandRegister *> useSet;
            switch (opcode)
            {
            case Opcode::J:
                break;
            case Opcode::BLTZ:
            case Opcode::BGTZ:
            case Opcode::BLEZ:
            case Opcode::BGEZ:
            case Opcode::BEQZ:
            case Opcode::BNEZ:
                useSet.insert(dynamic_cast<AsmOperandRegister *>(getOperand(0)));
                break;
            case Opcode::BLT:
            case Opcode::BGT:
            case Opcode::BLE:
            case Opcode::BGE:
            case Opcode::BEQ:
            case Opcode::BNE:
                useSet.insert(dynamic_cast<AsmOperandRegister *>(getOperand(0)));
                useSet.insert(dynamic_cast<AsmOperandRegister *>(getOperand(1)));
                break;
            default:
                break;
            }
            return useSet;
        }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return condition == Condition::UNCONDITIONAL; }

        bool mayHaveSideEffect() override { return false; }
    };
}
