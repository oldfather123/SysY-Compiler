#pragma once

#include <string>
#include "convert.h"

namespace Backend
{
    class AsmConstantData
    {
    public:
        enum DataType
        {
            WORD,
            DWORD,
            ZERO
        };

    private:
        DataType type;
        long long value;
        int length; // in bytes, only used for DataType::ZERO

    public:
        AsmConstantData(DataType type, long long value, int length) : type(type), value(value), length(length) {}
        AsmConstantData(int value) : type(DataType::WORD), value(value), length(0) {}
        AsmConstantData(long long value) : type(DataType::DWORD), value(value), length(0) {}
        AsmConstantData(float value) : type(DataType::WORD), value(floatToUnsigned(value)), length(0) {}
        DataType getType() const { return type; }
        std::string emit() const
        {
            switch (type)
            {
            case DataType::WORD:
                return "\t.word " + std::to_string(value) + "\n";
            case DataType::DWORD:
                return "\t.dword " + std::to_string(value) + "\n";
            case DataType::ZERO:
                return "\t.zero " + std::to_string(length) + "\n";
            default:
                return "";
            }
        }

    public:
        static AsmConstantData zero(int length) { return AsmConstantData(DataType::ZERO, 0, length); }
    };
}