#pragma once

#include <map>
#include "AsmOperandMemoryAddress.h"
#include "AsmInstBasic.h"
#include "instruction.h"

namespace Backend
{
    class AsmAddressAllocator
    {
    private:
        std::map<IR::Instruction *, AsmOperandMemoryAddress *> addressMap;

    public:
        AsmAddressAllocator() {}
        AsmOperandMemoryAddress *allocate(IR::Instruction *instruction, AsmOperandMemoryAddress *address)
        {
            addressMap[instruction] = address;
            return address;
        }

        bool contains(IR::Instruction *instruction)
        {
            return addressMap.find(instruction) != addressMap.end();
        }

        AsmOperandMemoryAddress *get(IR::Instruction *instruction)
        {
            if (contains(instruction))
            {
                return addressMap[instruction];
            }
            else
            {
                Error::Error(__PRETTY_FUNCTION__, "Instruction does not have a memory address");
                return nullptr;
            }
        }
    };
}