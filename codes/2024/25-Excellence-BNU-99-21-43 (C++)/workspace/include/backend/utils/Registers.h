#pragma once

#include <set>
#include <algorithm>
#include "AsmOperandRegisterInt.h"
#include "AsmOperandRegisterFloat.h"

namespace Backend
{
    class Registers
    {
    private:
        Registers() = default; // 私有构造函数，防止实例化

    public:
        // 可用寄存器集合
        static const std::set<AsmOperandRegister *, AsmOperandRegisterCompare> USABLE_REGISTERS;
        // 调用者保存寄存器集合
        static const std::set<AsmOperandRegister *, AsmOperandRegisterCompare> CALLER_SAVED_REGISTERS;
        // 常量寄存器集合
        static const std::set<AsmOperandRegister *, AsmOperandRegisterCompare> CONSTANT_REGISTERS;

        // 判断寄存器是否在函数调用间保留
        static bool isPreservedAcrossCalls(const AsmOperandRegister *reg)
        {
            return CALLER_SAVED_REGISTERS.find(const_cast<AsmOperandRegister *>(reg)) == CALLER_SAVED_REGISTERS.end();
        }
    };
}
