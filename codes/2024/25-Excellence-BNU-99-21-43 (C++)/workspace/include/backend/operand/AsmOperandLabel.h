#pragma once

#include "AsmOperandBasic.h"
namespace Backend
{
    class AsmOperandLabel : public AsmOperandBasic
    {
    private:
        std::string labelName;
        bool isGlobal;

    public:
        AsmOperandLabel(std::string labelName, bool isGlobal) : AsmOperandBasic(OperandType::LABEL), labelName(labelName), isGlobal(isGlobal) {}
        std::string emit() const override { return isGlobal ? labelName : "." + labelName; }
        std::string toString() const override { return isGlobal ? labelName : "." + labelName; }
        std::string getLabelName() const { return labelName; }
        bool isGlobalLabel() const { return isGlobal; }
        bool operator==(const AsmOperandLabel &other) const { return labelName == other.labelName && isGlobal == other.isGlobal; }
        bool operator!=(const AsmOperandLabel &other) const { return !(*this == other); }
        bool operator<(const AsmOperandLabel &other) const { if (labelName != other.labelName) return labelName < other.labelName; return isGlobal < other.isGlobal; }

        struct AsmOperandLabelCmp
        {
            bool operator()(const AsmOperandLabel *a, const AsmOperandLabel *b) const
            {
                return *a < *b;
            }
        };
    };
}