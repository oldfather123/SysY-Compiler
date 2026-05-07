#pragma once
#include <map>

#include "AsmInstHeaders.h"
#include "AsmOperandStackVariable.h"
#include "instruction.h"

namespace Backend
{
    class AsmStackAllocator
    {
    private:
        int pointer = 16; // stack pointer (16B for preserving ra and s0)
        int pointerMax = 16;
        int argSize = 0; // extra size for arguments when calling functions
        int argSizeMax = 0;
        int regSize = 0;         // extra size for preserving registers when allocating registers
        bool needSaveRa = false; // TODO : maybe need to be false alternatively for better performance

        std::map<IR::Instruction *, AsmOperandStackVariable *> stackVariableMap;

    public:
        AsmStackAllocator() {}
        AsmOperandStackVariable *allocate(IR::Instruction *instruction, int size)
        {
            // align to 8 bytes if size >= 8
            if (size >= 8)
            {
                pointer = (pointer + 7) / 8 * 8;
            }

            // allocate stack variable
            pointer += size;
            pointerMax = std::max(pointerMax, pointer);
            AsmOperandStackVariable *stackVariable = new AsmOperandStackVariable(true, -pointer, size);
            stackVariableMap[instruction] = stackVariable;

            return stackVariable;
        }

        void pushBack(AsmOperandStackVariable *stackVariable)
        {
            argSize += stackVariable->getSize();
            argSizeMax = std::max(argSizeMax, argSize);
        }

        bool contains(IR::Instruction *instruction)
        {
            return stackVariableMap.find(instruction) != stackVariableMap.end();
        }

        AsmOperandStackVariable *get(IR::Instruction *instruction)
        {
            if (contains(instruction))
            {
                return stackVariableMap[instruction];
            }
            else
            {
                Error::Error(__PRETTY_FUNCTION__, "Instruction does not have a stack variable");
                return nullptr;
            }
        }

        void prepareForFunctionCall()
        {
            argSize = 0;
            needSaveRa = true;
        }

        int getArgSizeMax() { return argSizeMax; }

        int getRegSizeMax() { return regSize; }

        int getPointerMax() { return pointerMax; }

        AsmOperandStackVariable *allocateForRegPreserve()
        {
            pointerMax += 8;
            regSize += 8;
            // ra s0 reg_preserve temporary_stack_space arg_space
            return new AsmOperandStackVariable(true, -16 - regSize, 8, true);
        }

        AsmInstBasicList emitPrologue()
        {
            // align stack pointer to 16 bytes
            pointerMax += argSizeMax;
            pointerMax = (pointerMax + 15) / 16 * 16;

            AsmInstBasicList instList;
            AsmOperandRegisterInt *sp = AsmOperandRegisterInt::SP;
            AsmOperandRegisterInt *ra = AsmOperandRegisterInt::RA;
            AsmOperandRegisterInt *s0 = AsmOperandRegisterInt::S0;

            // preserve %ra and %s0, using %sp as base register
            if (needSaveRa)
                instList.addInst(new AsmInstStore(ra, new AsmOperandStackVariable(false, -8, 8)));
            instList.addInst(new AsmInstStore(s0, new AsmOperandStackVariable(false, -16, 8)));

            // update %sp and $s0 (modify %sp to allocate stack, backup %sp to %s0)
            if (ImmediateWithin12Bits(pointerMax))
            {
                // if pointerMax is within 12 bits, use immediate value
                instList.addInst(new AsmInstAdd(sp, sp, new AsmOperandImmediate(-pointerMax), 64));
                instList.addInst(new AsmInstAdd(s0, sp, new AsmOperandImmediate(pointerMax), 64));
            }
            else
            {
                // if pointerMax is not within 12 bits, use register
                AsmOperandRegisterInt *t0 = AsmOperandRegisterInt::T0;
                instList.addInst(new AsmInstLoad(t0, new AsmOperandImmediate(pointerMax)));
                instList.addInst(new AsmInstSub(sp, sp, t0, 64));
                instList.addInst(new AsmInstAdd(s0, sp, t0, 64));
            }

            return instList;
        }

        AsmInstBasicList emitEpilogue()
        {
            AsmInstBasicList instList;
            AsmOperandRegisterInt *sp = AsmOperandRegisterInt::SP;
            AsmOperandRegisterInt *ra = AsmOperandRegisterInt::RA;
            AsmOperandRegisterInt *s0 = AsmOperandRegisterInt::S0;

            // restore %sp
            if (ImmediateWithin12Bits(pointerMax))
            {
                // if pointerMax is within 12 bits, use immediate value
                instList.addInst(new AsmInstAdd(sp, sp, new AsmOperandImmediate(pointerMax), 64));
            }
            else
            {
                // if pointerMax is not within 12 bits, use register
                AsmOperandRegisterInt *t0 = AsmOperandRegisterInt::T0;
                instList.addInst(new AsmInstLoad(t0, new AsmOperandImmediate(pointerMax)));
                instList.addInst(new AsmInstAdd(sp, sp, t0, 64));
            }

            // restore %ra and %s0 (note: restpre %sp before restoring %ra and %s0)
            if (needSaveRa)
                instList.addInst(new AsmInstLoad(ra, new AsmOperandStackVariable(false, -8, 8)));
            instList.addInst(new AsmInstLoad(s0, new AsmOperandStackVariable(false, -16, 8)));

            // add reture instruction (ret)
            instList.addInst(new AsmInstReturn());

            return instList;
        }
    };
} // namespace Backend