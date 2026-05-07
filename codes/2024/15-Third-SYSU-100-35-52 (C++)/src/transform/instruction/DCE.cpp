#include "DCE.h"
#include "Block.h"
#include "Instruction.h"
#include "Value.h"
#include "callGraphAnalysis.h"
int count = 0;
bool DCE(FunctionPtr func) {
    bool changed = false;
    if(func->isLib) return false;
    for(auto& bb : func->getBasicBlocks()) {
        for(auto it = bb->instructionsRef().rbegin(); it != bb->instructionsRef().rend(); it++) {
            auto& inst = *it;
            if(inst->getInsID() == InsID::Call || inst->getInsID() == InsID::Br || inst->getInsID() == InsID::Return) {
                continue;
            }
            if(inst->getInsID() == InsID::Store) {
                continue;
            }
            if(inst->users().useCount == 0) {
                inst->eraseFromParent();
                changed = true;
                it--;
                ::count++;
            }
        }
    }
    return changed;
}

bool findArrUseDfs(Instruction* inst, std::set<Instruction*>& visited) {
    if(visited.find(inst) != visited.end()) {
        return false;
    }
    visited.insert(inst);
    if(inst->getInsID() == InsID::Load || inst->getInsID() == InsID::Call) {
        return true;
    }
    for(auto user : inst->users()) {
        if(findArrUseDfs(user, visited)) {
            return true;
        }
    }
    return false;
}

void eraseArrDFS(Instruction* inst, std::set<Instruction*>& visited) {
    if(visited.find(inst) != visited.end()) {
        return;
    }
    visited.insert(inst);
    for(auto user : inst->users()) {
        eraseArrDFS(user, visited);
    }
    inst->eraseFromParent();
}

void deadArrayEliminate(FunctionPtr func) {
    auto entry = func->getEntryBlock();
    std::vector<AllocaInstruction*>  localArrList;
    for(auto& inst : entry->instructionsRef()) {
        if(auto allocaInst = dynamic_cast<AllocaInstruction*>(inst.get())) {
            if(allocaInst->type->isArr()) {
                localArrList.push_back(allocaInst);
            }
        }
    }
    for(auto arr : localArrList) {
        bool isUsed = false;
        std::set<Instruction*> visited = {};
        isUsed = findArrUseDfs(arr, visited);
        if(!isUsed) {
            visited.clear();
            eraseArrDFS(arr, visited);
        }
    }
}

void runDCE(Module& mod) {
    cout << YELLOW <<"Begin DCE" << RESET <<endl;
    bool changed = false;
    do {
        changed = false;
        for(auto& func : mod.globalFunctions) {
            if(func->isLib)
                continue;
            changed |= DCE(func.get());
        }
    } while(changed);
    cout << "DCE remove " << ::count << " instructions" << endl;
    auto cg = runCallGraphAnalysis(mod);
    for(auto it = mod.globalFunctions.begin(); it != mod.globalFunctions.end(); it++) {
        auto func = it->get();
        if (func->isLib) continue;
        if(func->getName() == "main")
            continue;
        if(cg.caller(func).empty()) {
            if(!func->isLib)
                cout << "DCE remove function " << func->getName() << endl;
            it = mod.globalFunctions.erase(it);
            it--;
            
        }
    }
}

void runDeadArrElemim(Module& mod) {
    // cout << YELLOW <<"Begin DeadArrElemim" << RESET <<endl;
    for(auto& func : mod.globalFunctions) {
        if(func->isLib)
            continue;
        deadArrayEliminate(func.get());
    }
}