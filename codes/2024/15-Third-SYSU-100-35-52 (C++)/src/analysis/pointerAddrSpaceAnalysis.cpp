#include "CFGAnalysis.h"
#include "Function.h"
#include "domTreeAnalysis.h"
#include "pointerAddrSpaceAnalysis.hpp"



void PointerAddressSpaceAnalysisResult::addTag(Value* ptr, AddressSpaceType space) {
    assert(!isTagged(ptr));
    mMappings.emplace(ptr, space);
}

bool PointerAddressSpaceAnalysisResult::isTagged(Value* ptr) const {
    return mMappings.count(ptr);
}

AddressSpaceType PointerAddressSpaceAnalysisResult::getAddressSpace(Value* ptr) const {
    return mMappings.at(ptr);
}
bool PointerAddressSpaceAnalysisResult::mayBe(Value* ptr, AddressSpaceType space) const {
    const auto iter = mMappings.find(ptr);
    if(iter != mMappings.cend()) {
        return static_cast<uint32_t>(iter->second) & static_cast<uint32_t>(space);
    }
    return true;
}

bool PointerAddressSpaceAnalysisResult::mustBe(Value* ptr, AddressSpaceType space) const {
    const auto iter = mMappings.find(ptr);
    if(iter != mMappings.cend())
        return iter->second == space;
    return false;
}

PointerAddressSpaceAnalysisResult runPointerAddrSpaceAnalysis(FunctionPtr func) {
    auto cfg = runCFGAnalysis(func);
    auto dom = runDominateTreeAnalysis(func, cfg);
    PointerAddressSpaceAnalysisResult result;

    while(true) {
        bool modified = false;

        auto inheritFrom = [&](Value* src, Value* dst) {
            if(result.isTagged(src)) {
                result.addTag(dst, result.getAddressSpace(src));
                modified = true;
            }
        };
        for(auto block : dom.blocks()) {
            // FIXME
            /*
            for(auto arg : block->args()) {
                if(!arg->getType()->isPointer() || result.isTagged(arg))
                    continue;
                if(const auto target = blockArgRef.query(arg); target)
                    inheritFrom(target, arg);
                else {
                    // TODO: mixed tag
                }
            }*/

            for(auto& inst : block->instructionsRef()) {
                if(!inst->type->isPtr() || result.isTagged(inst.get()))
                    continue;
                switch(inst->insId) {
                    // TODO: value analysis
                    case InsID::Load: {
                        break;
                    }
                    case InsID::Call: {
                        break;
                    }
                    case InsID::Alloca: {
                        result.addTag(inst.get(), AddressSpaceType::InternalStack);
                        break;
                    }
                    case InsID::GEP: {
                        // const auto base = inst.lastOperand();
                        auto base = inst->getOperand(inst->getNumOperands() - 1);
                        inheritFrom(base, inst.get());
                        break;
                    }
                    // case InstructionID::PtrCast: {
                    //     inheritFrom(inst.getOperand(0), &inst);
                    //     break;
                    // }
                    // case InstructionID::IntToPtr: {
                    //     break;
                    // }
                    case InsID::Phi: {
                        bool allTagged = true;
                        AddressSpaceType space = AddressSpaceType::InternalStack;
                        for(auto& ptr : inst->getOperandsRef()) {
                            if(!result.isTagged(ptr->val) || result.getAddressSpace(ptr->val) != space) {
                                allTagged = false;
                                break;
                            }
                        }
                        if(allTagged)
                            result.addTag(inst.get(), space);
                        break;
                    }
                    // case InstructionID::PtrAdd: {
                    //     inheritFrom(inst.getOperand(0), &inst);
                    //     break;
                    // }
                    // case InstructionID::FunctionPtr: {
                    //     break;
                    // }
                    default: {
                        break;
                        // block->dump(reportError() << "unimplemented inst "sv, HighlightInst{ &inst });
                        // reportNotImplemented(CMMC_LOCATION());
                    }
                }
            }
        }

        if(!modified)
            break;
    }
    return result;
}