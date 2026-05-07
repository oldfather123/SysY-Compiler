#pragma once
#include "AsmOperandBasic.h"
#include "AsmOperandRegisterInt.h"
#include <stdexcept>

namespace Backend
{
    class AsmOperandMemoryAddress : public AsmOperandBasic, public AsmRegisterInterface
    {
    private:
        AsmOperandRegisterInt *baseRegister;
        long long offset;

    public:
        AsmOperandMemoryAddress(AsmOperandRegisterInt *baseRegister, long long offset)
            : AsmOperandBasic(OperandType::MEMORY_ADDRESS), baseRegister(baseRegister), offset(offset) {}

        std::string emit() const override { return std::to_string(offset) + "(" + baseRegister->emit() + ")"; }
        std::string toString() const override { return std::to_string(offset) + "(" + baseRegister->toString() + ")"; }
        AsmOperandRegisterInt *getBaseRegister() const { return baseRegister; }
        long long getOffset() const { return offset; }
        bool operator==(const AsmOperandMemoryAddress &other) const { return baseRegister == other.baseRegister && offset == other.offset; }
        AsmOperandRegisterInt *getRegister() const override { return baseRegister; }
        AsmOperandMemoryAddress *withBaseRegister(AsmOperandRegisterInt *baseRegister_) const
        {
            return new AsmOperandMemoryAddress(baseRegister_, getOffset());
        }

        AsmOperandBasic *withRegister(AsmOperandRegister *register_) const override
        {
            if (register_ == nullptr || !register_->isRegisterInt())
            {
                throw std::invalid_argument("Invalid register");
            }
            return new AsmOperandMemoryAddress(static_cast<AsmOperandRegisterInt *>(register_), getOffset());
        }
        
        AsmOperandMemoryAddress *addOffset(long long offset_) const
        {
            return new AsmOperandMemoryAddress(getBaseRegister(), getOffset() + offset_);
        }

        AsmOperandMemoryAddress *getAddress()
        {
            return new AsmOperandMemoryAddress(baseRegister, offset);
        }

        AsmOperandMemoryAddress *setOffset(long long offset_)
        {
            return new AsmOperandMemoryAddress(baseRegister, offset_);
        }
    };
}