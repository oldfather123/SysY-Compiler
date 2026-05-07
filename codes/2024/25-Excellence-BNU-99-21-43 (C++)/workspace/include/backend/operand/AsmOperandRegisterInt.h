#pragma once
#include "AsmOperandRegister.h"
#include <unordered_map>

namespace Backend
{
    class AsmOperandRegisterInt : public AsmOperandRegister
    {
    private:
        static std::unordered_map<int, AsmOperandRegisterInt *> cache;

    public:
        AsmOperandRegisterInt(int regIndex) : AsmOperandRegister(regIndex, false) {}
        std::string emit() const override { return (isVirtualRegister() ? "vx" : "x") + std::to_string(getAbsRegIndex()); }
        std::string toString() const override { return (isVirtualRegister() ? "vx" : "x") + std::to_string(getAbsRegIndex()); }
        bool operator==(const AsmOperandRegisterInt &other) const { return AsmOperandRegister::operator==(other); }
        static AsmOperandRegisterInt *get(int regIndex)
        {
            if (cache.find(regIndex) == cache.end())
                cache[regIndex] = new AsmOperandRegisterInt(regIndex);
            return cache[regIndex];
        }

        static AsmOperandRegisterInt *getVirtual(int absRegIndex)
        {
            return get(-absRegIndex);
        }

        static AsmOperandRegisterInt *getPhysical(int absRegIndex)
        {
            return get(absRegIndex);
        }

        static AsmOperandRegisterInt *getParamReg(int paramIndex)
        {
            if (paramIndex < 0 || paramIndex >= 8)
            {
                Error::Error(__PRETTY_FUNCTION__, "Invalid param index: " + std::to_string(paramIndex));
            }

            return getPhysical(10 + paramIndex);
        }

    public:
        static AsmOperandRegisterInt *S0;
        static AsmOperandRegisterInt *S1;
        static AsmOperandRegisterInt *RA;
        static AsmOperandRegisterInt *ZERO;
        static AsmOperandRegisterInt *SP;
        static AsmOperandRegisterInt *T0;
    };
}