#pragma once

namespace Backend
{
    class AsmOperandRegister;
    class AsmOperandBasic;

    class AsmRegisterInterface
    {
    public:
        virtual AsmOperandRegister *getRegister() const = 0;
        virtual AsmOperandBasic *withRegister(AsmOperandRegister *register_) const = 0;
    };
}