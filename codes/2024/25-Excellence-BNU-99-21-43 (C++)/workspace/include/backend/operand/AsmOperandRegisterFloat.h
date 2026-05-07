#pragma once
#include "AsmOperandRegister.h"
#include <unordered_map>

namespace Backend
{
    class AsmOperandRegisterFloat : public AsmOperandRegister
    {
    private:
        static std::unordered_map<int, AsmOperandRegisterFloat *> cache;

    public:
        AsmOperandRegisterFloat(int regIndex) : AsmOperandRegister(regIndex, true) {}
        std::string emit() const override { return (isVirtualRegister() ? "vf" : "f") + std::to_string(getAbsRegIndex()); }
        std::string toString() const override { return (isVirtualRegister() ? "vf" : "f") + std::to_string(getAbsRegIndex()); }
        bool operator==(const AsmOperandRegisterFloat &other) const { return AsmOperandRegister::operator==(other); }
        static AsmOperandRegisterFloat *get(int regIndex)
        {
            if (cache.find(regIndex) == cache.end())
                cache[regIndex] = new AsmOperandRegisterFloat(regIndex);
            return cache[regIndex];
        }

        static AsmOperandRegisterFloat *getVirtual(int absRegIndex)
        {
            return get(-absRegIndex);
        }

        static AsmOperandRegisterFloat *getPhysical(int absRegIndex)
        {
            return get(absRegIndex);
        }

        static AsmOperandRegisterFloat *getParamReg(int paramIndex)
        {
            if (paramIndex < 0 || paramIndex >= 8)
            {
                Error::Error(__PRETTY_FUNCTION__, "Invalid param index: " + std::to_string(paramIndex));
            }

            return getPhysical(10 + paramIndex);
        }
    };
}