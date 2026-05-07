#pragma once
#include "AsmInstBasic.h"

namespace Backend
{
    /**
     * @brief This directive is a placeholder at the end of the base block and is only used to mark the end of the base block.
     */
    class AsmInstBlockEnd : public AsmInstBasic
    {
    public:
        AsmInstBlockEnd() : AsmInstBasic(OpcodeBasic::AsmInstBlockEnd, nullptr, nullptr, nullptr) {}

        std::string emit() const override
        {
            return "";
        }

        std::string toString() const override
        {
            return "AsmInstBlockEnd()";
        }

        std::set<AsmOperandRegister *> getDef() override { throw std::invalid_argument("Invalid operation"); }

        std::set<AsmOperandRegister *> getUse() override { throw std::invalid_argument("Invalid operation"); }

        bool mayNotReturn() override { throw std::invalid_argument("Invalid operation"); }

        bool willNeverReturn() override { throw std::invalid_argument("Invalid operation"); }

        bool mayHaveSideEffect() override { throw std::invalid_argument("Invalid operation"); }
    };
}