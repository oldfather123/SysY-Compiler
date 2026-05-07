#pragma once

#include "Common.h"
#include "IR.h"
#include "DFA.h"
#include "register.h"
#include "instruction.h"
#include "instruction_selection.h"
#include "machine_code.h"
#include "register_allocation.h"
#include "peephole.h"

namespace backend {

    void initBackend(const Ptr<ir::Module> &irModule, Ptr<RiscModule> riscModule) {
        for (auto &irFunction : irModule->funcSet()) {
            if (irFunction->isPrototype() && (irFunction->name() == "starttime" || irFunction->name() == "stoptime")) {
                irFunction->name() = "_sysy_" + irFunction->name();
            }
            Ptr<RiscFunction> riscFunction = riscModule->createFunction(irFunction, riscModule, !irFunction->isPrototype());

            if (irFunction->isPrototype()) {
                continue;
            }

            auto irBBList = irFunction->dfsBBList();
            for (auto &irBasicBlock : irBBList) {
                if (irBasicBlock == irFunction->exitBB()) {
                    continue;
                }
                riscFunction->createBasicBlock(irBasicBlock, riscFunction);
            }
            // Promise that exit block is the last block in the function
            riscFunction->createBasicBlock(irFunction->exitBB(), riscFunction);
        }
        for (auto &irGlobalInitInst : irModule->getGlobalInitInsts()) {
            Ptr<ir::GlobalInst> globalInst = castPtr<ir::GlobalInst>(irGlobalInitInst);
            if (!globalInst) {
                continue;
            }
            riscModule->createGlobal(globalInst, riscModule);
        }
    }

    void runBackend(const Ptr<ir::Module> &irModule, Ptr<RiscModule> riscModule) {
        dbgout << std::endl
               << "ASM generation started." << std::endl;

        initBackend(irModule, riscModule);
        for (auto func : riscModule->funcs()) {
            auto irFunction = func->sourceFunc();
            ir::DFAContext dfaCtx{irFunction, ir::DFAContext::LV};

            mapArguments(func);
            generateInst(func);
            allocateRegisters(func, dfaCtx);

            // dbgout << std::endl
            //        << func->tag() << " function ASM before saveAndRestoreCallSites:" << std::endl
            //        << func->toString() << std::endl;
            saveAndRestoreCallSites(func);
            findReadWrittenRegs(func);

            // dbgout << std::endl
            //        << func->tag() << " function ASM before fillVirtualRegs:" << std::endl
            //        << func->toString() << std::endl;
            fillVirtualRegs(func);
            fillOffsets(func);

            FixedPoint{
                [&](bool &changed) {
                    peephole(func, changed);
                },
                true,
                "Peephole optimization",
                func->tag(),
            };
        }
    }
}
