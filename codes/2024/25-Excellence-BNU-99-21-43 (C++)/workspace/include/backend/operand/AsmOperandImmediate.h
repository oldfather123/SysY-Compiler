#pragma once
#include "AsmOperandBasic.h"
namespace Backend
{
    class AsmOperandImmediate : public AsmOperandBasic
    {
    private:
        int value;

    public:
        AsmOperandImmediate(int value) : AsmOperandBasic(OperandType::IMMEDIATE), value(value) {}
        std::string emit() const override { return std::to_string(value); }
        std::string toString() const override { return std::to_string(value); }
        int getValue() const { return value; }
        bool operator==(const AsmOperandImmediate &other) const { return value == other.value; }
    };
}