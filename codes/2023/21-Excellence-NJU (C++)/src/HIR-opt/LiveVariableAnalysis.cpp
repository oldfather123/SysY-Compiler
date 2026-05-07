//
// Created by divin on 23-7-27.
//

#include "HIR-opt/LiveVariableAnalysis.h"

LiveVariableAnalysis::LiveVariableAnalysis(GlobalUnit *gu) {
    this->globalUnit = gu;
}

void LiveVariableAnalysis::calcUseDef(Function* func) {
    for (auto BB : func->block_list) {
        auto &use = BB->live_use;
        auto &def = BB->live_def;

        for (auto inst : BB->local_instr) {
            for (auto use_var : inst->use_list) {
                if (use.find(use_var) == use.end()) {
                    use.insert(use_var);
                }
            }

            for (auto def_var : inst->def_list) {
                def.insert(def_var);
            }
        }
    }
}

void LiveVariableAnalysis::calcIn(BasicBlock* BB) {
    BB->live_in.clear();

    std::set<ValueRef* > diff;
    std::set_difference(BB->live_out.begin(), BB->live_out.end(),
                        BB->live_def.begin(), BB->live_def.end(),
                        std::inserter(diff, diff.begin()));

    BB->live_in.insert(BB->live_use.begin(), BB->live_use.end());
    BB->live_in.insert(diff.begin(), diff.end());
}

void LiveVariableAnalysis::calcOut(BasicBlock* BB) {
    BB->live_out.clear();
    for (auto succBB : BB->succ) {
        BB->live_out.insert(succBB->live_in.begin(), succBB->live_in.end());
    }
}

void LiveVariableAnalysis::analysis() {
    for (const auto& it1 : this->globalUnit->func_table) {
        auto func = it1.second;
        calcUseDef(func);
        bool changed;
        do {
            changed = false;
            for (auto it2 = func->block_list.rbegin(); it2 != func->block_list.rend(); ++it2) {
                BasicBlock* BB = *it2;
                auto old_live_in(BB->live_in);

                calcOut(BB);
                calcIn(BB);

                changed |= old_live_in != BB->live_in;
            }
        } while (changed);
    }
}

void LiveVariableAnalysis::printBBIn(BasicBlock *BB) {
        cerr << BB->label <<":"<<endl;
        cerr << "  IN = [ ";
        for (auto varname : BB->live_in)
            cerr << varname <<" ";
        cerr << "]"<<endl;
}

void LiveVariableAnalysis::printBBOut(BasicBlock *BB) {
    cerr << "  OUT = [ ";
    for (auto varname : BB->live_out)
        cerr << varname <<" ";
    cerr << "]"<<endl;
}

void LiveVariableAnalysis::listDefUse() {
    for (const auto& it1 : this->globalUnit->func_table) {
        auto func = it1.second;
        cerr << func->name << ":"<<endl;
        for (auto BB : func->block_list) {
            cerr << BB->label <<":"<<endl;
            cerr << "  DEF = [ ";
            for (auto var : BB->live_def)
                cerr << var->get_Ref() <<" ";
            cerr << "]"<<endl;

            cerr << "  USE = [ ";
            for (auto var : BB->live_use)
                cerr << var->get_Ref() <<" ";
            cerr << "]"<<endl;
        }
    }
}

void LiveVariableAnalysis::listInOut() {
    for (const auto& it1 : this->globalUnit->func_table) {
        auto func = it1.second;
        cerr << func->name << ":"<<endl;
        for (auto BB : func->block_list) {
               cerr << BB->label <<":"<<endl;
               cerr << "  IN = [ ";
               for (auto var : BB->live_in)
                   cerr << var->get_Ref() <<" ";
               cerr << "]"<<endl;

            cerr << "  OUT = [ ";
            for (auto var : BB->live_out)
                cerr << var->get_Ref() <<" ";
            cerr << "]"<<endl;
        }
    }
}

