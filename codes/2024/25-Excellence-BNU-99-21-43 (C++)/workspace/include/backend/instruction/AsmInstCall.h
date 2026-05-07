#pragma once
#include "AsmInstBasic.h"
#include "Registers.h"

namespace Backend
{
    class AsmInstCall : public AsmInstBasic
    {
    private:
        std::vector<AsmOperandRegister *> paramRegList; // List of parameter registers
        AsmOperandRegister *returnReg;                  // Register for the return value

    public:
        AsmInstCall(AsmOperandLabel *label, std::vector<AsmOperandRegister *> paramRegList, AsmOperandRegister *returnReg)
            : AsmInstBasic(OpcodeBasic::AsmInstCall, label, nullptr, nullptr), paramRegList(std::move(paramRegList)), returnReg(returnReg) {}

        std::vector<AsmOperandRegister *> getParamRegList() const
        {
            return paramRegList;
        }

        AsmOperandRegister *getReturnRegister() const
        {
            return returnReg;
        }

        void setParamRegList(const std::vector<AsmOperandRegister *> &paramRegList)
        {
            this->paramRegList = paramRegList;
        }

        void setReturnRegister(AsmOperandRegister *returnReg)
        {
            this->returnReg = returnReg;
        }

        std::string toString() const override
        {
            std::string result = "AsmInstCall(" + getOperand(0)->toString() + ", [";
            for (size_t i = 0; i < paramRegList.size(); ++i)
            {
                result += paramRegList[i]->toString();
                if (i < paramRegList.size() - 1)
                    result += ", ";
            }
            result += "]";
            if (returnReg)
                result += "->" + returnReg->toString();
            result += ")";
            return result;
        }

        std::string emit() const override
        {
            return "\tcall " + getOperand(0)->emit() + "\n";
        }

        std::set<AsmOperandRegister *> getDef() override
        {
            // note: Registers::CALLER_SAVED_REGISTERS has special compare function,
            //   so, we can't convert it to std::set directly
            std::set<AsmOperandRegister *> def;
            def.insert(Registers::CALLER_SAVED_REGISTERS.begin(), Registers::CALLER_SAVED_REGISTERS.end());
            return def;
        }

        std::set<AsmOperandRegister *> getUse() override
        {
            std::set<AsmOperandRegister *> use;
            for (auto &reg : paramRegList)
            {
                use.insert(reg);
            }
            return use;
        }

        bool mayNotReturn() override { return false; }

        bool willNeverReturn() override { return false; }

        bool mayHaveSideEffect() override { return true; }  // Function call may have side effect, such as modifying memory
    };
} // namespace Backend
