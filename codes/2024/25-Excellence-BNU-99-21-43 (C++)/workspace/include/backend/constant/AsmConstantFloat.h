#pragma once

#include "AsmConstantData.h"
#include "AsmOperandLabel.h"
#include "convert.h"

namespace Backend
{
    class AsmConstantFloat
    {
    private:
        std::string labelName;
        AsmConstantData constantData;

    public:
        AsmConstantFloat(float value) : labelName("acm_float_" + std::to_string(floatToUnsigned(value))), constantData(value) {}

        std::string emit() const
        {
            return "\t.align 2\n." + labelName + ":\n" + constantData.emit();
        }

        AsmOperandLabel *getAsmOperandLabel() { return new AsmOperandLabel(labelName, false); }
    };
}