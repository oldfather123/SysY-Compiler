#include "Registers.h"

namespace Backend
{
    // 初始化可用寄存器集合
    const std::set<AsmOperandRegister *, AsmOperandRegisterCompare> Registers::USABLE_REGISTERS = []
    {
        std::set<AsmOperandRegister *, AsmOperandRegisterCompare> regs;
        for (int i = 0; i <= 31; ++i)
        {
            if ((5 <= i && i <= 7) || (18 <= i))
                regs.insert(AsmOperandRegisterInt::getPhysical(i));
            if (i <= 9 || 18 <= i)
                regs.insert(AsmOperandRegisterFloat::getPhysical(i));
        }
        return regs;
    }();

    // 初始化调用者保存寄存器集合
    const std::set<AsmOperandRegister *, AsmOperandRegisterCompare> Registers::CALLER_SAVED_REGISTERS = []
    {
        std::set<AsmOperandRegister *, AsmOperandRegisterCompare> regs;
        regs.insert(AsmOperandRegisterInt::getPhysical(1));
        for (int i = 5; i <= 7; ++i)
            regs.insert(AsmOperandRegisterInt::getPhysical(i));
        for (int i = 10; i <= 17; ++i)
            regs.insert(AsmOperandRegisterInt::getPhysical(i));
        for (int i = 28; i <= 31; ++i)
            regs.insert(AsmOperandRegisterInt::getPhysical(i));
        for (int i = 0; i <= 7; ++i)
            regs.insert(AsmOperandRegisterFloat::getPhysical(i));
        for (int i = 10; i <= 17; ++i)
            regs.insert(AsmOperandRegisterFloat::getPhysical(i));
        for (int i = 28; i <= 31; ++i)
            regs.insert(AsmOperandRegisterFloat::getPhysical(i));
        return regs;
    }();

    // 初始化常量寄存器集合
    const std::set<AsmOperandRegister *, AsmOperandRegisterCompare> Registers::CONSTANT_REGISTERS = []
    {
        std::set<AsmOperandRegister *, AsmOperandRegisterCompare> regs;
        regs.insert(AsmOperandRegisterInt::S0);
        for (int i = 0; i < 5; ++i)
            regs.insert(AsmOperandRegisterInt::getPhysical(i));
        return regs;
    }();
}
