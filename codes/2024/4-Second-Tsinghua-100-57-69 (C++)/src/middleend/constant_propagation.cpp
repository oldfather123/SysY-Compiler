#include "middleend/constant_propagation.hpp"
#include <iostream>

namespace middleend {

void update_phi(ir::BasicBlock* bb, int now_bb){
    for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
        if (auto phi = dynamic_cast<ir::instruction::Phi *>(*iter)) {
            std::vector<ir::BasicBlock *> bbs;
            std::vector<Temp *> values;
            for (int i = 0; i < phi->getbbs()->size(); i++) {
                if (now_bb != phi->getbbs()->at(i)->get_index()) {
                    bbs.push_back(phi->getbbs()->at(i));
                    values.push_back(phi->getvalues()->at(i));
                }
            }
            bb->get_instructions()->erase(iter);
            if (bbs.size() == 0) {
                assert(false);
            } else if (bbs.size() == 1) {
                bb->get_instructions()->insert(iter, new ir::instruction::Assign(phi->getdst(), values[0]));
            } else {
                bb->get_instructions()->insert(iter, new ir::instruction::Phi(phi->getdst(), values, bbs));
            }
            delete phi;
        }
        else break;
    }
}


void ConstantPropagation::run() {
    bool flag = true;
    while(flag){
        flag = false;
        for (auto bb : *func_->get_basic_blocks()) {
            for (auto & inst : *bb->get_instructions()) {
                if (auto loadimm = dynamic_cast<ir::instruction::LoadImm4 *>(inst)) {
                    auto dst = loadimm->getdst();
                    if(!constant_map.count(dst->get_index())) flag = true;
                    constant_map[dst->get_index()] = loadimm->getimm();
                } else if (auto assign = dynamic_cast<ir::instruction::Assign *>(inst)) {
                    auto dst = assign->getdst();
                    auto src = assign->getsrc();
                    if (constant_map.count(src->get_index())) {
                        constant_map[dst->get_index()] = constant_map[src->get_index()];
                        inst = new ir::instruction::LoadImm4(dst, constant_map[dst->get_index()]);
                        flag = true;
                        delete assign;
                    }
                } else if (auto unary = dynamic_cast<ir::instruction::Unary *>(inst)) {
                    auto dst = unary->getdst();
                    auto src = unary->getsrc();
                    if (constant_map.count(src->get_index())) {
                        constant_map[dst->get_index()] = unary->to_const(constant_map[src->get_index()]);
                        inst = new ir::instruction::LoadImm4(dst, constant_map[dst->get_index()]);
                        flag = true;
                        delete unary;
                    }
                } else if (auto binary = dynamic_cast<ir::instruction::Binary *>(inst)) {
                    auto dst = binary->getdst();
                    auto lhs = binary->getlhs();
                    auto rhs = binary->getrhs();
                    if (constant_map.count(lhs->get_index()) && constant_map.count(rhs->get_index())) {
                        constant_map[dst->get_index()] = binary->to_const(constant_map[lhs->get_index()], constant_map[rhs->get_index()]);
                        inst = new ir::instruction::LoadImm4(dst, constant_map[dst->get_index()]);
                        flag = true;
                        delete binary;
                    }
                } 
                else if (auto condbranch = dynamic_cast<ir::instruction::CondBranch *>(inst)) {
                    auto cond = condbranch->getcond();
                    if (constant_map.count(cond->get_index())) {
                        if (constant_map[cond->get_index()] == 0 || constant_map[cond->get_index()] == 0.0f) {
                            update_phi(condbranch->get_true_bb(), bb->get_index());
                            inst = new ir::instruction::Branch(condbranch->get_false_bb());
                        } else {
                            update_phi(condbranch->get_false_bb(), bb->get_index());
                            inst = new ir::instruction::Branch(condbranch->get_true_bb());
                        }
                        delete condbranch;
                        flag = true;
                    }
                }
            }
        }
    }
}

}