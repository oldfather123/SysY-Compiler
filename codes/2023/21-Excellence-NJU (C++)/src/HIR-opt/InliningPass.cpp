//
// Created by divin on 23-8-20.
//
#include "HIR-opt/InliningPass.h"
#include "IRInstruction.h"

void InliningPass::Build_Simple_Call_Graph() {
    for(const auto& it : this->gu->func_table){
        auto caller = it.second;

        for (auto bb : caller->block_list) {
            for (auto inst : bb->local_instr) {
                if (inst->instType == CALL) {
                    auto callee = dynamic_cast<CallInstruction*>(inst)->function;

                    if (std::find(caller->callee.begin(), caller->callee.end(), callee) == caller->callee.end())
                        caller->callee.insert(callee);
                    if (std::find(callee->caller.begin(), callee->caller.end(), caller) == callee->caller.end())
                        callee->caller.insert(caller);
                }
            }
        }
    }
}




void InliningPass::Build_Call_Graph() {

    unordered_set<Function*> visited;
    stack<Function*> stk;


    for (auto it : this->gu->func_table) {
        auto caller = it.second;
        visited.clear();
        stk.push(caller);


        while (!stk.empty()) {
            auto f = stk.top();
            stk.pop();

            if (visited.find(f) == visited.end()) {
                visited.insert(f);
            }

            for (auto callee : f->callee) {
                caller->callee.insert(callee);
                callee->caller.insert(caller);
                if (visited.find(callee) == visited.end()) {
                    stk.push(callee);
                }
            }
        }
    }
}

bool InliningPass::has_se_inst(Function *func) {
    for (auto bb : func->block_list) {
        for (auto inst : bb->local_instr) {
            if (inst->instType == STORE)
                return true;
        }
    }
    return false;
}

void InliningPass::Set_Side_Effect() {
    unordered_set<Function*> visited;
    stack<Function*> stk;

    for (auto it : this->gu->func_table) {
        auto caller = it.second;
        visited.clear();
        caller->has_side_effect = false;
        stk.push(caller);

        while (!stk.empty()) {
            auto f = stk.top();
            stk.pop();

            f->has_side_effect = has_se_inst(f);
            caller->has_side_effect = caller->has_side_effect || f->has_side_effect;

            if (visited.find(f) == visited.end()) {
                visited.insert(f);
            }

            for (auto callee : f->callee) {
                if (visited.find(callee) == visited.end()) {
                    stk.push(callee);
                }
            }
        }
//        if (caller->has_side_effect)
//            cerr << caller->name << " has side-effect"<<endl;
//        else
//            cerr << caller->name << " has not side-effect"<<endl;
    }

}

void InliningPass::debug(map<string,Function*>& func_table) {
    for(const auto& it:func_table) {
        auto caller = it.second;
        cerr << caller->name<<":"<<endl;
        cerr << "  callee:";
        for (auto calle : caller->callee) {
            cerr<<"  "<<calle->name<<",";
        }
        cerr << endl << "  caller:";
        for (auto caller : caller->caller) {
            cerr<<"  "<<caller->name<<",";
        }
        cerr<<endl;
    }
}

void InliningPass::run() {
    Build_Simple_Call_Graph();
    Build_Call_Graph();
    Set_Side_Effect();
    // debug(gu->func_table);
}