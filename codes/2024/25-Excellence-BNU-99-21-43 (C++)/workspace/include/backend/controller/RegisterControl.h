#pragma once

#include "AsmRegisterAllocator.h"
#include "AsmOperandHeaders.h"
#include "AsmFunction.h"
#include "AsmInstBasic.h"
#include "StackPool.h"
#include "Registers.h"
#include <unordered_map>
#include <utility>
#include <vector>

namespace Backend
{

    class RegisterControl
    {
    public:
        AsmFunction *function;
        std::unordered_map<AsmOperandRegister *, AsmOperandStackVariable *> preservedRegisterSaved;
        StackPool stackPool;

    public:
        std::vector<AsmInstBasic *> emitPrologue()
        {
            std::vector<AsmInstBasic *> instrList;
            for (const auto &entry : preservedRegisterSaved)
            {
                AsmOperandRegister *register_ = entry.first;
                AsmOperandStackVariable *stackVar = entry.second;
                auto tmpl = saveToStackVar(register_, stackVar, AsmOperandRegisterInt::getPhysical(5));
                instrList.insert(instrList.end(), tmpl.begin(), tmpl.end());
            }
            return instrList;
        }

        std::vector<AsmInstBasic *> emitEpilogue()
        {
            std::vector<AsmInstBasic *> instrList;
            for (const auto &entry : preservedRegisterSaved)
            {
                AsmOperandRegister *register_ = entry.first;
                AsmOperandStackVariable *stackVar = entry.second;
                auto tmpl = loadFromStackVar(register_, stackVar, AsmOperandRegisterInt::getPhysical(5));
                instrList.insert(instrList.end(), tmpl.begin(), tmpl.end());
            }
            return instrList;
        }

        void updateRegisterPreserve(AsmOperandRegister *register_)
        {
            if (Registers::isPreservedAcrossCalls(register_) &&
                preservedRegisterSaved.find(register_) == preservedRegisterSaved.end())
            {
                preservedRegisterSaved[register_] = stackPool.pop();
            }
        }

        void updateRegisterPreserve(AsmOperandRegister *register_, AsmOperandStackVariable *saved)
        {
            if (Registers::isPreservedAcrossCalls(register_) &&
                preservedRegisterSaved.find(register_) == preservedRegisterSaved.end())
            {
                preservedRegisterSaved[register_] = saved;
            }
        }

        std::vector<AsmInstBasic *> loadFromStackVar(AsmOperandRegister *register_,
                                                     AsmOperandStackVariable *stackVar,
                                                     AsmOperandRegisterInt *tmpReg)
        {
            std::vector<AsmInstBasic *> instList;
            if (!ImmediateWithin12Bits(stackVar->getAddress()->getOffset()))
            {
                instList.push_back(new AsmInstLoad(tmpReg, new AsmOperandImmediate(static_cast<int>(stackVar->getAddress()->getOffset()))));
                instList.push_back(new AsmInstAdd(tmpReg, tmpReg, stackVar->getAddress()->getRegister(), 64));
                stackVar = new AsmOperandStackVariable(true, 0, stackVar->getSize(), false);
                stackVar = static_cast<AsmOperandStackVariable *>(stackVar->withRegister(tmpReg));
            }
            instList.push_back(new AsmInstLoad(register_, stackVar));
            return instList;
        }

        std::vector<AsmInstBasic *> saveToStackVar(AsmOperandRegister *register_,
                                                   AsmOperandStackVariable *stackVar,
                                                   AsmOperandRegisterInt *tmpReg)
        {
            std::vector<AsmInstBasic *> instList;
            if (!ImmediateWithin12Bits(stackVar->getAddress()->getOffset()))
            {
                instList.push_back(new AsmInstLoad(tmpReg, new AsmOperandImmediate(static_cast<int>(stackVar->getAddress()->getOffset()))));
                instList.push_back(new AsmInstAdd(tmpReg, tmpReg, stackVar->getAddress()->getRegister(), 64));
                stackVar = new AsmOperandStackVariable(true, 0, stackVar->getSize(), false);
                stackVar = static_cast<AsmOperandStackVariable *>(stackVar->withRegister(tmpReg));
            }
            instList.push_back(new AsmInstStore(register_, stackVar));
            return instList;
        }

    public:
        RegisterControl(AsmFunction *function, AsmStackAllocator *allocator)
            : function(function), stackPool(allocator) {}

        virtual ~RegisterControl()
        {
            // Clean up preservedRegisterSaved
            for (auto &entry : preservedRegisterSaved)
            {
                delete entry.second; // Assuming StackVar needs to be deleted
            }
        }

        virtual AsmInstBasicList work(AsmInstBasicList instructionList) = 0;
    };

}
