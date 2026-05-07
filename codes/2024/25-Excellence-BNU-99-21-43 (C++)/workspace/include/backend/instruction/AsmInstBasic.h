#pragma once
#include "AsmOperandHeaders.h"
#include "Constraint.h"
#include <stdexcept>
#include <vector>
#include <set>

namespace Backend
{
    class AsmInstBasic
    {
    public:
        enum class OpcodeBasic
        {
            AsmInstAdd,
            AsmInstAnd,
            AsmInstBlockEnd,
            AsmInstCall,
            AsmInstConvertFloatInt,
            AsmInstFloatCompare,
            AsmInstFloatDivide,
            AsmInstFloatNegate,
            AsmInstIntCompare,
            AsmInstJump,
            AsmInstLabel,
            AsmInstLoad,
            AsmInstMax,
            AsmInstMin,
            AsmInstMove,
            AsmInstMul,
            AsmInstNop,
            AsmInstReturn,
            AsmInstShiftLeft,
            AsmInstShiftLeftAdd,
            AsmInstShiftRightArithmetic,
            AsmInstShiftRightLogical,
            AsmInstSignedIntDivide,
            AsmInstSignedIntRemainder,
            AsmInstStore,
            AsmInstSub,

            AsmInstOr,
            AsmInstXor,
        };

    private:
        OpcodeBasic opcodeBasic;
        AsmOperandBasic *operands[3];

    public:
        AsmInstBasic(OpcodeBasic opcodeBasic, AsmOperandBasic *op1, AsmOperandBasic *op2, AsmOperandBasic *op3) : opcodeBasic(opcodeBasic), operands{op1, op2, op3} {}
        virtual ~AsmInstBasic() = default;
        virtual std::string emit() const = 0;
        virtual std::string toString() const = 0;
        virtual std::set<AsmOperandRegister *> getDef() = 0; // return the set of registers that are defined by this instruction
        virtual std::set<AsmOperandRegister *> getUse() = 0; // return the set of registers that are used by this instruction
        virtual bool mayNotReturn() = 0;
        virtual bool willNeverReturn() = 0;
        virtual bool mayHaveSideEffect() = 0;

        AsmOperandBasic *getOperand(int index) const
        {
            if (index < 0 || index >= 3)
                Error::Error(__PRETTY_FUNCTION__, "Invalid index");
            // if (operands[index] == nullptr)
            //     Error::Error(__PRETTY_FUNCTION__, "Operand is null");
            return operands[index];
        }

        void setOperand(int index, AsmOperandBasic *operand)
        {
            if (operand == nullptr)
                Error::Error(__PRETTY_FUNCTION__, "Operand is null");
            if (index < 0 || index >= 3 || operands[index] == nullptr)
                Error::Error(__PRETTY_FUNCTION__, "Invalid index");
            if (operand->getType() != operands[index]->getType())
                Error::Error(__PRETTY_FUNCTION__, "Operand type mismatch with the original operand, original = " + operands[index]->toString() + ", new = " + operand->toString());
            operands[index] = operand;
        }

        OpcodeBasic getOpcodeBasic() const { return opcodeBasic; }
    };

    class AsmInstBasicList
    {
    private:
        std::vector<AsmInstBasic *> instructions;

    public:
        AsmInstBasicList() {}

        AsmInstBasicList(std::vector<AsmInstBasic *> instructions) : instructions(instructions) {}

        void addInst(AsmInstBasic *inst)
        {
            instructions.push_back(inst);
        }

        void addInst(AsmInstBasicList instList)
        {
            instructions.insert(instructions.end(), instList.getList().begin(), instList.getList().end());
        }

        void addInst(int index, AsmInstBasic *inst)
        {
            if (index < 0 || index >= (int)instructions.size())
                Error::Error(__PRETTY_FUNCTION__, "Invalid index, size = " + std::to_string(instructions.size()) + ", index = " + std::to_string(index));
            instructions.insert(instructions.begin() + index, inst);
        }

        void addInst(int index, AsmInstBasicList instList)
        {
            if (index < 0 || index >= (int)instructions.size())
                Error::Error(__PRETTY_FUNCTION__, "Invalid index, size = " + std::to_string(instructions.size()) + ", index = " + std::to_string(index));
            instructions.insert(instructions.begin() + index, instList.getList().begin(), instList.getList().end());
        }

        void addInst(std::vector<AsmInstBasic *> instList)
        {
            instructions.insert(instructions.end(), instList.begin(), instList.end());
        }

        std::vector<AsmInstBasic *> &getList() { return instructions; }

        void setList(std::vector<AsmInstBasic *> list) { instructions = list; }

        void clear() { instructions.clear(); }
    };
}