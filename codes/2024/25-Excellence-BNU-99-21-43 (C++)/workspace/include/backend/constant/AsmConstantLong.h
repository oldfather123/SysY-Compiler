#pragma once

#include "AsmConstantData.h"
#include "AsmOperandLabel.h"
#include "convert.h"

namespace Backend
{
    class AsmConstantLong
    {
    private:
        std::string labelName;
        AsmConstantData constantData;

    public:
        AsmConstantLong(long long value) : labelName("acm_long_" + std::to_string((unsigned long long)(value))), constantData(value) {}

        std::string emit() const
        {
            return "\t.align 4\n." + labelName + ":\n" + constantData.emit();
        }

        AsmOperandLabel *getAsmOperandLabel() { return new AsmOperandLabel(labelName, false); }
    };
}