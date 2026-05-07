#include "constantArrPropagation.h"

void constantArrPropagation(Module& mod) {
    struct ArrInfo {
        std::vector<Value*> baseAndIdx;
        bool operator==(const ArrInfo& other) const {
            return baseAndIdx == other.baseAndIdx;
        }
        struct HashFunc {
            std::size_t operator()(const ArrInfo& instInfo) const {
                std::hash<Value*> hasher;
                std::size_t v = 0;
                for(auto& val : instInfo.baseAndIdx) {
                    v <<= 2;
                    v ^= hasher(val);
                }
                return v;
            }
        };
    };
    for(auto& func : mod.getGlobalFunctions()) {
        if(func->isLib)
            continue;
        std::set<AllocaInstruction*> nonConstArr;
        for(auto& bb : func->getBasicBlocks()) {
            for(auto& inst : bb->instructions()) {
                if(auto storeInst = inst->as<StoreInstruction>()) {
                    auto basePtr = storeInst->getPtr();
                    if(auto gepInst = basePtr->as<GetElementPtrInstruction>()) {
                        basePtr = gepInst->getBase();
                        while(auto baseBasePtr = basePtr->as<GetElementPtrInstruction>()) {
                            basePtr = baseBasePtr->getBase();
                        }
                    }
                    if(auto allocInst = basePtr->as<AllocaInstruction>()) {
                        if(storeInst->getValue() != 0) {
                            nonConstArr.insert(allocInst);
                        }
                    }
                }
                if(auto callInst = inst->as<CallInstruction>()) {
                    for(auto arg : callInst->getArgs()) {
                        auto basePtr = arg;
                        if(auto gepInst = arg->as<GetElementPtrInstruction>()) {
                            basePtr = gepInst->getBase();
                            while(auto baseBasePtr = basePtr->as<GetElementPtrInstruction>()) {
                                basePtr = baseBasePtr->getBase();
                            }
                        }
                        if(auto allocInst = basePtr->as<AllocaInstruction>()) {
                            nonConstArr.insert(allocInst);
                        }
                    }
                }
            }
        }
        for(auto& bb : func->getBasicBlocks()) {
            for(auto& inst : bb->instructions()) {
                if(auto loadInst = inst->as<LoadInstruction>()) {
                    auto basePtr = loadInst->getPtr();
                    if(auto gepInst = basePtr->as<GetElementPtrInstruction>()) {
                        basePtr = gepInst->getBase();
                        while(auto baseBasePtr = basePtr->as<GetElementPtrInstruction>()) {
                            basePtr = baseBasePtr->getBase();
                        }
                        if(auto allocInst = basePtr->as<AllocaInstruction>()) {
                            if(nonConstArr.find(allocInst) == nonConstArr.end()) {
                                loadInst->replaceWith(Const::getConst(Type::getInt(), 0));
                            }
                            //  else {
                            //     loadInst->replaceWith(Const::getConst(Type::getInt(), 0));
                            // }
                        }
                    }
                }
            }
        }
    }
}