#include "middleend/copy_propagation.hpp"

namespace middleend {

void copy_propagation(UseDefAnalysis *ud, Temp *dst, Temp *src) {
    while(ud->get_usesets()[dst].size() > 0) {
        auto inst = *(ud->get_usesets()[dst].begin());
        ud->change_use(dst, src, inst);
        inst->change_use(dst, src);
    }
}

void CopyPropagation::address_resolve() {
    ud_ = new UseDefAnalysis(cfg_);
    for (auto bb: *func_->get_basic_blocks()) {
        for (auto &inst: *bb->get_instructions()) {
            TypeCase (load, ir::instruction::Load*, inst) {
                auto def = *ud_->get_defset(load->getaddr()).begin();
                TypeCase (loadimm, ir::instruction::BinaryImm *, def) {
                    if (loadimm->get_type() == BinaryOp::Add) {
                        if (loadimm->getimm() < 2048 && loadimm->getimm() >= -2048) {
                            load->setimm(loadimm->getimm());
                            load->setaddr(*loadimm->get_src()->begin());
                        }
                    }
                }
            }
            TypeCase (store, ir::instruction::Store*, inst) {
                auto def = *ud_->get_defset(store->getaddr()).begin();
                TypeCase (loadimm, ir::instruction::BinaryImm *, def) {
                    if (loadimm->get_type() == BinaryOp::Add) {
                        if (loadimm->getimm() < 2048 && loadimm->getimm() >= -2048) {
                            store->setimm(loadimm->getimm());
                            store->setaddr(*loadimm->get_src()->begin());
                        }
                    }
                }
            }
        }
    }
}

void CopyPropagation::run() {
    cfg_ = new CFG(func_);
    std::unordered_set<int> visit;
    std::vector<int> q;
    q.push_back(func_->get_entry()->get_index());
    visit.insert(func_->get_entry()->get_index());
    while (!q.empty()) {
        auto bb_idx = q.back();
        q.pop_back();
        auto bb = cfg_->get_bb(bb_idx);
        for (auto & inst : *bb->get_instructions()) {
            if (auto assign = dynamic_cast<ir::instruction::Assign *>(inst)) {
                auto dst = assign->getdst();
                auto src = assign->getsrc();
                if (!copy_map.count(src->get_index())) {
                    copy_map[dst->get_index()] = src;
                }
                else{
                    copy_map[dst->get_index()] = copy_map[src->get_index()];
                }
            }
            if (auto phi = dynamic_cast<ir::instruction::Phi *>(inst)) {
                auto vals = *phi->getvalues();
                if (std::adjacent_find(vals.begin(), vals.end(), std::not_equal_to<>()) != vals.end()) continue;
                auto new_inst = new ir::instruction::Assign(phi->getdst(), (*phi->getvalues()).front());
                if (!copy_map.count(new_inst->getsrc()->get_index())) {
                    copy_map[new_inst->getdst()->get_index()] = new_inst->getsrc();
                }
                else{
                    copy_map[new_inst->getdst()->get_index()] = copy_map[new_inst->getsrc()->get_index()];
                }
                inst = new_inst;
            }
        }
        for (auto succ : *cfg_->get_bb_succ(bb_idx)) {
            if (!visit.count(succ)) {
                visit.insert(succ);
                q.push_back(succ);
            }
        }
    }
    for (auto bb : *func_->get_basic_blocks()) {
        for (auto & inst : *bb->get_instructions()) {
            for(auto & use : *inst->get_src()){
                if(use == nullptr)continue;
                if(copy_map.count(use->get_index())){
                    use = copy_map[use->get_index()];
                }
            }
        }
    }
}

}