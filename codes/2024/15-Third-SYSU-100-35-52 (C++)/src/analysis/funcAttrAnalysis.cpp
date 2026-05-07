#include "Function.h"
#include "funcAttrAnalysis.h"
#include "pointerAddrSpaceAnalysis.hpp"


// reference: https://github.com/dtcxzyw/cmmc/blob/main/cmmc/Transforms/Analysis/FunctionAttrInfer.cpp
void runFuncAttrAnalysis(FunctionPtr func) {
    auto addressSpace = runPointerAddrSpaceAnalysis(func);

    // TODO: !!! 对于memory load  store 需要分析

    bool modified = false;
    if(!func->noReturn) {
        bool noReturn = true;
        for(auto& block : func->basicBlocks)
            if(block->getTerminator()->insId == InsID::Return) {
                noReturn = false;
                break;
            }
        if(noReturn) {
            func->noReturn = true;
            modified = true;
        }
    }

    if(!func->noMemoryRead) {
        auto hasLoad = [&] {
            for(auto& block : func->basicBlocks) {
                for(auto& inst : block->instructionsRef()) {
                    switch(inst->insId) {
                        case InsID::Load:
                            // TODO: add address space analysis

                            return true;
                        case InsID::AtomicAdd: {
                            const auto addr = inst->getOperand(0);
                            if(addressSpace.mustBe(addr, AddressSpaceType::InternalStack))
                                continue;
                            return true;
                        }
                        case InsID::Call: {
                            // const auto callee = inst.lastOperand();
                            auto callee = inst->getOperand(inst->getNumOperands() - 1);
                            if(callee == func)
                                continue;
                            if(callee->is<Function>()) {
                                if(!callee->as<Function>()->noMemoryRead) {
                                    return true;
                                }
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
            return false;
        };

        if(!hasLoad()) {
            func->noMemoryRead = true;
        }
    }

    if(!func->noMemoryWrite) {
        auto hasWrite = [&] {
            for(auto& block : func->basicBlocks) {
                for(auto& inst : block->instructionsRef()) {
                    switch(inst->insId) {
                        case InsID::Store:
                        case InsID::AtomicAdd: {
                            const auto addr = inst->getOperand(0);
                            if(addressSpace.mustBe(addr, AddressSpaceType::InternalStack))
                                continue;
                            return true;
                        }
                        case InsID::Call: {
                            // const auto callee = inst.lastOperand();
                            auto callee = inst->getOperand(inst->getNumOperands() - 1);
                            if(callee == func)
                                continue;
                            if(callee->is<Function>()) {
                                if(!callee->as<Function>()->noMemoryWrite) {
                                    return true;
                                }
                            } else
                                return true;
                        }
                        default:
                            break;
                    }
                }
            }

            return false;
        };

        if(!hasWrite()) {
            func->noMemoryWrite = true;
        }
    }

    if(func->noMemoryWrite && !func->noSideEffect) {
        auto hasSideEffect = [&] {
            for(auto& block : func->basicBlocks) {
                for(auto& inst : block->instructionsRef()) {
                    switch(inst->insId) {
                        case InsID::Call: {
                            // const auto callee = inst.lastOperand();
                            auto callee = inst->getOperand(inst->getNumOperands() - 1);
                            if(callee == func)
                                continue;
                            if(callee->is<Function>()) {
                                if(!callee->as<Function>()->noSideEffect)
                                    return true;
                            } else
                                return true;
                        }
                        default:
                            break;
                    }
                }
            }

            return false;
        };

        if(!hasSideEffect()) {
            func->noSideEffect = true;
        }
    }

    if(!func->stateless) {
        if(func->noMemoryRead && func->noSideEffect) {
            func->stateless = true;
        }
    }

    if(!func->noRecurse) {
        auto hasSelfCall = [&] {
            for(auto& block : func->basicBlocks) {
                for(auto& inst : block->instructionsRef()) {
                    switch(inst->insId) {
                        case InsID::Call: {
                            // const auto callee = inst.lastOperand();
                            auto callee = inst->getOperand(inst->getNumOperands() - 1);
                            if(callee == func)
                                return true;
                        }
                        default:
                            break;
                    }
                }
            }

            return false;
        };

        if(!hasSelfCall()) {
            func->noRecurse = true;
        }
    }
}
