#include "global2Local.h"
#include "CFGAnalysis.h"
#include "IRbuilder.h"
#include "callGraphAnalysis.h"
#include "mem2reg.h"

void global2Local(Module& module) {
    auto useFunc = std::map<Variable*, FunctionPtr>{};
    for(auto glob : module.getGlobalVariables()) {
        if(glob->isConst)
            continue;
        if(glob->type->isArr())
            continue;
        auto it = glob->mUsers.begin();
        while(it != glob->mUsers.end()) {
            auto user = *it;
            if(auto storeInst = dynamic_cast<StoreInstruction*>(user)) {
                if(storeInst->getPtr() == glob) {
                    auto func = storeInst->getBasicBlock()->getParent();
                    if(useFunc.find(glob) == useFunc.end()) {
                        useFunc[glob] = func;
                    } else if(useFunc[glob] != func) {
                        useFunc[glob] = nullptr;
                    }
                }
            }
            if(auto loadInst = dynamic_cast<LoadInstruction*>(user)) {
                if(loadInst->getPtr() == glob) {
                    auto func = loadInst->getBasicBlock()->getParent();
                    if(useFunc.find(glob) == useFunc.end()) {
                        useFunc[glob] = func;
                    } else if(useFunc[glob] != func) {
                        useFunc[glob] = nullptr;
                    }
                }
            }
            it++;
        }
    }
    for(auto iter : useFunc) {
        if(auto func = iter.second) {
            auto glob = iter.first;
            if(func->isLib)
                continue;
            IRbuilder builder(func->getEntryBlock());
            auto allocaInst = builder.createAlloca(glob->getType());
            auto allocaIter = dynamic_cast<InstructionPtr>(allocaInst)->getIteratorRef();
            glob->replaceWithInFunc(func, allocaInst);
            if (auto fGlob = dynamic_cast<Float*>(glob)) {
                auto storeInst = make_unique<StoreInstruction>(allocaInst, Const::getConst(Type::getFloat(), fGlob->floatVal));
                storeInst->setBasicBlock(func->getEntryBlock());
                func->getEntryBlock()->instructionsRef().insert(++allocaIter, std::move(storeInst));
                // auto cfg = runCFGAnalysis(func);
                // auto domTree = runDominateTreeAnalysis(func, cfg);
                // mem2reg(func,domTree);
            }
            if (auto iGlob = dynamic_cast<Int*>(glob)) {
                auto storeInst = make_unique<StoreInstruction>(allocaInst, Const::getConst(Type::getInt(), iGlob->intVal));
                storeInst->setBasicBlock(func->getEntryBlock());
                func->getEntryBlock()->instructionsRef().insert(++allocaIter, std::move(storeInst));
                // auto cfg = runCFGAnalysis(func);
                // auto domTree = runDominateTreeAnalysis(func, cfg);
                // mem2reg(func,domTree);
            }
        }

    }
}