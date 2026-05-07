#pragma once
#include "AsmOperandRegisterFloat.h"
#include "AsmOperandRegisterInt.h"
#include "instruction.h"
#include <map>

namespace Backend
{
    class AsmRegisterAllocator
    {
    private:
        std::map<IR::Instruction *, AsmOperandRegisterFloat *> floatRegisterMap;
        std::map<IR::Instruction *, AsmOperandRegisterInt *> intRegisterMap;
        std::map<int, AsmOperandRegister *> indexRegisterMap;
        int counter = 0;

    public:
        AsmOperandRegisterFloat *allocateFloatRegister(IR::Instruction *inst)
        {
            counter++;
            AsmOperandRegisterFloat *reg = new AsmOperandRegisterFloat(-counter);
            floatRegisterMap[inst] = reg;
            indexRegisterMap[reg->getAbsoluteIndex()] = reg;
            return reg;
        }

        AsmOperandRegisterInt *allocateIntRegister(IR::Instruction *inst)
        {
            counter++;
            AsmOperandRegisterInt *reg = new AsmOperandRegisterInt(-counter);
            intRegisterMap[inst] = reg;
            indexRegisterMap[reg->getAbsoluteIndex()] = reg;
            return reg;
        }

        AsmOperandRegisterInt *allocateIntRegister()
        {
            counter++;
            AsmOperandRegisterInt *reg = new AsmOperandRegisterInt(-counter);
            indexRegisterMap[reg->getAbsoluteIndex()] = reg;
            return reg;
        }

        AsmOperandRegisterFloat *allocateFloatRegister()
        {
            counter++;
            AsmOperandRegisterFloat *reg = new AsmOperandRegisterFloat(-counter);
            indexRegisterMap[reg->getAbsoluteIndex()] = reg;
            return reg;
        }

        AsmOperandRegister *allocateRegister(IR::Instruction *inst)
        {
            if (inst->type->isFloat())
                return allocateFloatRegister(inst);
            else
                return allocateIntRegister(inst);
        }

        AsmOperandRegister *allocateRegister(IR::BasicType *type)
        {
            if (type->isFloat())
                return allocateFloatRegister();
            else
                return allocateIntRegister();
        }

        AsmOperandRegister *allocateRegister(AsmOperandRegister *srcRegister)
        {
            if (srcRegister->isRegisterFloat())
                return allocateFloatRegister();
            else
                return allocateIntRegister();
        }

        bool contains(IR::Instruction *inst)
        {
            return floatRegisterMap.find(inst) != floatRegisterMap.end() || intRegisterMap.find(inst) != intRegisterMap.end();
        }

        bool contain(int index)
        {
            return indexRegisterMap.find(index) != indexRegisterMap.end();
        }

        AsmOperandRegister *get(IR::Instruction *inst)
        {
            if (intRegisterMap.find(inst) != intRegisterMap.end())
                return intRegisterMap[inst];
            else if (floatRegisterMap.find(inst) != floatRegisterMap.end())
                return floatRegisterMap[inst];
            else
            {
                Error::Error(__PRETTY_FUNCTION__, "Register not found");
                return nullptr;
            }
        }

        AsmOperandRegister *get(int index)
        {
            if (indexRegisterMap.find(index) != indexRegisterMap.end())
                return indexRegisterMap[index];
            else
            {
                Error::Error(__PRETTY_FUNCTION__, "Register not found");
                return nullptr;
            }
        }
    };
}