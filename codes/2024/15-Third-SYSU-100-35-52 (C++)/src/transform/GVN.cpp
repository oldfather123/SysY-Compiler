#include "GVN.h"
#include "DCE.h"
#include "domTreeAnalysis.h"
vector<GetElementPtrInstruction*> geps;

bool GVN(FunctionPtr func) {
    geps.clear();
    cout << "Enter GVN" << endl;
    bool modified = false;
    auto cfg = runCFGAnalysis(func);
    auto domTree = runDominateTreeAnalysis(func,cfg);
    for(auto& bb : func->getBasicBlocks()) {
        // unordered_set<InstructionPtr> toDelete;
        // toDelete.clear();
        
        for(auto& instr : bb->instructionsRef()) {
            // cout << "Here!!!" << endl;
            if(instr->insId == InsID::GEP) {
                auto target = dynamic_cast<GetElementPtrInstruction*>(instr.get());
                bool flag = false;

                for(auto pre : geps) {
                    if(pre->getBase() == target->getBase() && pre->getIdx().size() == target->getIdx().size()) {
                        bool flag2 = false;
                        auto preIdx = pre->getIdx();
                        auto targetIdx = target->getIdx();
                        for(int i = 0; i < preIdx.size(); i++) {
                            if(preIdx[i] != targetIdx[i] || !preIdx[i]->isConst) {
                                flag2 = true;
                                break;
                            }
                        }

                        if(!flag2) {
                            auto prebb = pre->basicblock;
                            auto curbb = target->basicblock;
                            if(domTree.dominate(prebb, curbb)) {
                                flag = true;
                                target->replaceWith(pre);

                                modified = true;
                                break;
                            }
                        }
                    }
                }
                if(!flag)
                    geps.push_back(target);
            }
        }
        // if(modified) {
        //     // cout<< "Modified!!!" <<endl;
        //     // for(auto myins: toDelete)
        //     // {
        //     //     myins->print();
        //     // }
        //     vector<InstructionPtr> newIns;
        //     for(auto instr : bb->instructions) {
        //         if(!toDelete.count(instr))
        //             newIns.push_back(instr);
        //     }
        //     bb->instructions = newIns;
        // }
        // toDelete.clear();
    }
    return modified;

}

void runGVN(Module& module){
    for(auto& func : module.getGlobalFunctions()) {
        if(func->isLib)
            continue;
        GVN(func);
    }
    runDCE(module);
}