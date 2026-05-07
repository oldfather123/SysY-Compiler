
#include "inline.h"
#include "Block.h"
#include "CFGAnalysis.h"
#include "callGraphAnalysis.h"
#include "Function.h"
#include "Instruction.h"
#include "utils.h"
#include <cassert>
#include <chrono>

static void inlineFunction(CallInstruction* callInst) {
    auto beforeBB = callInst->getBasicBlock();
    auto callee = callInst->getFunc();
    auto afterBB = splitBasicBlock(callInst);
    auto& calleeBBs = callee->getBasicBlocks();
    auto curFunc = beforeBB->belongfunc;
    auto beforeBBiterator = curFunc->getIterator(beforeBB);

    std::unordered_map<Value*, Value*> vmap;
    for(int i = 0; i < callee->formArguments.size(); i++) {
        vmap[callee->formArguments[i]] = callInst->getArg(i);
    }
    std::unordered_map<BasicBlock*, BasicBlock*> bbmap;
    std::vector<unique_ptr<BasicBlock>> newBBs;
    for(auto& bb : calleeBBs) {
        auto newBB = bb->mClone(vmap, "inline." + callee->getName() + ".");
        newBB->setBelongFunc(curFunc);
        if(bb.get() == callee->getEntryBlock()) {
            auto beforeTerminator = dynamic_cast<BrInstruction*>(beforeBB->getTerminator());
            assert(beforeTerminator);
            beforeTerminator->trueTarget = newBB.get();
            auto callerEntry = curFunc->getEntryBlock();
            for(auto it = newBB->begin();it!=newBB->end();it++){
                if((*it)->insId==InsID::Alloca){
                    (*it)->setBasicBlock(callerEntry);
                    callerEntry->instructionsRef().insert(callerEntry->begin(),std::move(*it));
                    it = newBB->instructionsRef().erase(it);
                    it--;
                }
            }
        }
        bbmap[bb.get()] = newBB.get();
        newBBs.push_back(std::move(newBB));
    }
    for(auto& bb : newBBs) {
        auto terminator = bb->getTerminator();
        if(auto brInst = dynamic_cast<BrInstruction*>(terminator)) {
            brInst->trueTarget = bbmap[brInst->getTrueTarget()];
            if(brInst->getFalseTarget()) {
                brInst->falseTarget = bbmap[brInst->getFalseTarget()];
            }
        } else if(auto retInst = dynamic_cast<ReturnInstruction*>(terminator)) {
            auto retval = retInst->getRetVal();
            if(!retval->isVoid()) {
                callInst->replaceWith(retval);
            }
            callInst->eraseFromParent();
            bb->instructionsRef().pop_back();
            auto newBrInst = std::make_unique<BrInstruction>(afterBB);
            newBrInst->setBasicBlock(bb.get());
            bb->forceSetEndInst(std::move(newBrInst));
        } else {
            assert(false);
        }
        for(auto& inst : bb->instructionsRef()) {
            if(auto phiInst = dynamic_cast<PhiInstruction*>(inst.get())) {
                for(auto& oldBB : calleeBBs) {
                    phiInst->replaceSource(oldBB.get(), bbmap[oldBB.get()]);
                }
            }
            for(auto& operand : inst->getOperandsRef()) {
                if(auto iter = vmap.find(operand->val); iter != vmap.end()) {
                    operand->replaceValue(iter->second);
                }
            }
        }

        beforeBBiterator = curFunc->insertAfter(beforeBBiterator, std::move(bb));
    }
}

static bool shouldInline(FunctionPtr callee, callGraphAnalysisResult& cg) {
    if(callee->isLib)
        return false;
    if(cg.callGraph()[callee].isRecursive)
        return false;
    if(callee->getBasicBlocks().size() > 8)
        return false;
    return true;
}

static bool tryInline(FunctionPtr func, callGraphAnalysisResult& cg) {
    bool modified = false;
    for(auto& callInst : cg.callInstList(func)) {
        FunctionPtr callee = callInst->getFunc();
        if(!shouldInline(callee, cg))
            continue;
        cout << "Inline " << callee->getName() << " to " << func->getName() << endl;
        inlineFunction(callInst);
        modified = true;
    }
    return modified;
}

bool runInline(Module& mod) {
    auto cg = runCallGraphAnalysis(mod);
    cout << YELLOW << "Begin Inline" << RESET << endl;
    auto start = chrono::high_resolution_clock::now();
    bool modified = false;
    if(cg.callGraph().empty()) {
        return false;
    }
    for(auto& func : mod.getGlobalFunctions()) {
        if(func->isLib)
            continue;
        // for(int i = 0; i < 16; i++) {
        //     if(tryInline(func, cg)){
        //         modified = true;
        //         cg = runCallGraphAnalysis(func,cg);
        //     }
        //     else
        //         break;
        // }
        if(tryInline(func, cg)) {
            modified = true;
            runCFGAnalysis(func);
            // func->dump();
        }
    }

    auto end = chrono::high_resolution_clock::now();
    cout << GREEN <<"Inline finished in " << RESET << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << endl;
    return modified;
}