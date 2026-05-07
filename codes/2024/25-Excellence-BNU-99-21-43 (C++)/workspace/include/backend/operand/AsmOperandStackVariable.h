#pragma once
#include "AsmOperandBasic.h"
#include "AsmOperandMemoryAddress.h"
#include "AsmOperandRegisterInt.h"
namespace Backend
{
    class AsmOperandStackVariable : public AsmOperandBasic, public AsmRegisterInterface
    {
    private:
        AsmOperandMemoryAddress *memoryAddress;
        int size;
        bool isRegPreserved = false;

    public:
        AsmOperandStackVariable(AsmOperandMemoryAddress *memoryAddress, int size, bool isRegPreserved = false)
            : AsmOperandBasic(OperandType::STACK_VARIABLE), memoryAddress(memoryAddress), size(size), isRegPreserved(isRegPreserved) {}
        AsmOperandStackVariable(bool baseRegIsS0, long long offset, int size, bool isRegPreserved = false)
            : AsmOperandBasic(OperandType::STACK_VARIABLE), memoryAddress(new AsmOperandMemoryAddress((baseRegIsS0 ? AsmOperandRegisterInt::S0 : AsmOperandRegisterInt::SP), offset)), size(size), isRegPreserved(isRegPreserved) {}
        virtual ~AsmOperandStackVariable() = default; // 使得可通过基类指针删除派生类对象

        std::string emit() const override { return memoryAddress->emit(); }
        std::string toString() const override { return "stack[" + memoryAddress->toString() + ", " + std::to_string(size) + "]"; }
        int getSize() const { return size; }
        bool operator==(const AsmOperandStackVariable &other) const { return memoryAddress == other.memoryAddress && size == other.size; }
        AsmOperandMemoryAddress *getAddress() const { return memoryAddress; }
        AsmOperandRegisterInt *getRegister() const override { return getAddress()->getBaseRegister(); }
        AsmOperandBasic *withRegister(AsmOperandRegister *register_) const override
        {
            return new AsmOperandStackVariable(memoryAddress->withBaseRegister(static_cast<AsmOperandRegisterInt *>(register_)), size, isRegPreserved);
        }
        bool isForRegPreserve() const { return isRegPreserved; }
        AsmOperandStackVariable* addOffset(int offset) { return new AsmOperandStackVariable(memoryAddress->addOffset(offset), size, isRegPreserved); }
        long long getOffset() const { return memoryAddress->getOffset(); }
        bool isS0Based() const { return memoryAddress->getBaseRegister() == AsmOperandRegisterInt::S0; }
        AsmOperandStackVariable *flip() { return new AsmOperandStackVariable(!isS0Based(), getOffset(), size, isRegPreserved); }

    };
}