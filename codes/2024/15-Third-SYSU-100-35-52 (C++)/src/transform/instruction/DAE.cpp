#include "DAE.h"
#include "callGraphAnalysis.h"
#include "domTreeAnalysis.h"
#include "DCE.h"

set<AllocaInstruction*> dirtyArr;

bool DAE(FunctionPtr func) {
    auto cfg = runCFGAnalysis(func);
    // auto domTree = runDominateTreeAnalysis(func, cfg);
    bool changed = false;
    for(auto& bb : func->getBasicBlocks()) {
        // 已定义的变量
        std::map<Value*, Instruction*> storedVal;
        // 已取出的变量
        std::map<Value*, Instruction*> loadedVal;
        vector<BasicBlock*> workList = { bb.get() };
        // for(auto& succ : cfg.successors(bb.get())) {
        //     if(domTree.dominate(bb.get(), succ) && bb.get() != succ) {
        //         succBB.push_back(succ);
        //     }
        // }
        // if(succBB.size() > 2)
        //     continue;
        for(auto succ : cfg.successors(bb.get())) {
            if(cfg.predecessors(succ).size() == 1 && succ != bb.get()) {
                workList.push_back(succ);
            }
        }
        if(workList.size() > 2)
            continue;
        for(auto& workBB : workList) {
            for(auto inst : workBB->instructions()) {
                if(auto storeInst = inst->as<StoreInstruction>()) {
                    auto ptrVal = storeInst->getPtr();
                    // 重复定义, 之前的 store 指令可删除
                    if(storedVal.find(ptrVal) != storedVal.end()) {
                        storedVal[ptrVal]->eraseFromParent();
                        changed = true;
                    }

                    // 记录新定义或覆盖之前的定义
                    storedVal[ptrVal] = storeInst;
                    auto des = storeInst->getPtr();
                    bool flag = false;
                    while(auto gep = dynamic_cast<GetElementPtrInstruction*>(des)) {
                        for(auto idx : gep->getIdx()) {
                            if(!idx->isConst) {
                                flag = true;
                                break;
                            }
                        }
                        des = gep->getBase();
                    }
                    if(auto alloca = dynamic_cast<AllocaInstruction*>(des)) {
                        if(flag) {
                            dirtyArr.insert(alloca);
                        }
                    }

                }
                // 该 load 指令前后的 store 不是一个值
                else if(auto loadInst = inst->as<LoadInstruction>()) {
                    auto ptrVal = loadInst->getPtr();
                    auto ptr = ptrVal;
                    if(storedVal.find(ptrVal) != storedVal.end()) {
                        // TODO 全局数组
                        auto storeInst = dynamic_cast<StoreInstruction*>(storedVal[ptrVal]);
                        assert(storeInst != nullptr);
                        while(auto gep = dynamic_cast<GetElementPtrInstruction*>(ptrVal)) {
                            ptrVal = gep->getBase();
                        }
                        if(auto alloca = dynamic_cast<AllocaInstruction*>(ptrVal)) {
                            if(dirtyArr.find(alloca) == dirtyArr.end()) {
                                loadInst->replaceWith(storeInst->getValue());
                                changed = true;
                            }
                        }

                        // loadInst->eraseFromParent();
                        // continue;
                    }
                    storedVal.erase(ptr);
                } else if(auto callInst = inst->as<CallInstruction>()) {
                    storedVal.clear();
                }
            }
            for(auto inst : workBB->instructions()) {
                if(auto loadInst = inst->as<LoadInstruction>()) {
                    auto ptrVal = loadInst->getPtr();
                    auto ptr = ptrVal;
                    // 重复 load, 该 load 指令可删除
                    if(loadedVal.find(ptrVal) != loadedVal.end()) {
                        while(auto gep = dynamic_cast<GetElementPtrInstruction*>(ptrVal)) {
                            ptrVal = gep->getBase();
                        }
                        if(auto alloca = dynamic_cast<AllocaInstruction*>(ptrVal)) {
                            if(dirtyArr.find(alloca) == dirtyArr.end()) {
                                loadInst->replaceWith(loadedVal[ptr]);
                                changed = true;
                            }
                        }
                    }
                    loadedVal[ptr] = loadInst;
                } else if(auto storeInst = inst->as<StoreInstruction>()) {
                    // 该 store 指令前后的 load 不是一个值
                    auto ptrVal = storeInst->getPtr();
                    loadedVal.erase(ptrVal);
                } else if(auto callInst = inst->as<CallInstruction>()) {
                    loadedVal.clear();
                }
            }
        }
    }
    return changed;
}

void runDAE(Module& mod) {

    for(auto& func : mod.getGlobalFunctions()) {
        if(func->isLib)
            continue;
        while(DAE(func)) {DCE(func);}
    }
}
