#include "middleend/split_critical_edges.hpp"

namespace middleend {

void SplitCriticalEdges::split_critical_edges(ir::Function *func) {
    auto &br_frequency = func->branch_frequency;
    phi_info.clear();
    incoming_bbs.clear();
    new_bbs.clear();
    CFG* cfg = new CFG(func);

    // construct phi_info and incoming bbs
    for(auto bb: *func->get_basic_blocks()){
        for(auto inst: *bb->get_instructions()) {
            TypeCase(phi, ir::instruction::Phi *, inst) {
                phi_info[bb].emplace_back(phi);
                for(int i = 0; i < phi->getsize(); i++){
                    auto bb = (*phi->getbbs())[i];
                    auto r = (*phi->getvalues())[i];
                    assert(bb); assert(r);
                    if(cfg->get_bb_succ(bb->get_index())->size() > 1) {
                        // successor size > 1
                        incoming_bbs.insert(bb);
                        

                    }

                }
            }
        }
    }

    for(auto bb: *func->get_basic_blocks()) {
        auto preds = *cfg->get_bb_prev(bb->get_index());
        for(auto pred_idx: preds) {
            auto bb_pred = cfg->get_bb(pred_idx);
            if(!incoming_bbs.count(bb_pred)) {
                continue;
            }

            // bb_mid will be added to func eventually
            auto bb_mid = new ir::BasicBlock(func, {}, func->get_bb_used());
            func->set_bb_used(func->get_bb_used() + 1);

            new_bbs.push_back({bb_pred, bb_mid});
            
            // change (bb_pred -> bb) to (bb_pred -> bb_mid) and (bb_mid -> bb)
            cfg->erase_from_succ(bb_pred, bb);
            cfg->erase_from_prev(bb, bb_pred);
            cfg->insert_succ(bb_pred, bb_mid);
            cfg->insert_prev(bb_mid, bb_pred);
            cfg->insert_succ(bb_mid, bb);
            cfg->insert_prev(bb, bb_mid);

            auto it = br_frequency.find({bb, bb_pred});
            if(it != br_frequency.end()) {
                auto fre = it->second;
                br_frequency[{bb_pred, bb_mid}] = fre;
                br_frequency[{bb_mid, bb}] = fre;
                br_frequency.erase(it);
            }

            auto last = bb_pred->get_instructions()->back();
            TypeCase(br, ir::instruction::CondBranch *, last) {
                if(br->get_true_bb() == bb) {
                    br->set_true_bb(bb_mid);
                } else {
                    br->set_false_bb(bb_mid);
                }
            } else {
                last->print(std::cout);
                assert(false), "in split critical edges";
            }

            bb_mid->add_instruction(new ir::instruction::Branch(bb));

            if(!phi_info.count(bb)) {
                continue;
            }
            for(auto phi: phi_info.at(bb)) {
                auto it = phi->get_src_temp(bb_pred);
                if(it){
                    phi->erase_src_temp(bb_pred);
                    phi->add_src_and_bb(bb_mid, it);
                }
            }
        }
    }

    for(auto bb_iter = func->get_basic_blocks()->begin(); bb_iter != func->get_basic_blocks()->end();) {
        auto bb = *bb_iter;
        ++bb_iter;
        for(auto it = new_bbs.begin(); it != new_bbs.end();) {
            if(it->first == bb) {
                bb_iter = func->get_basic_blocks()->insert(bb_iter, it->second);
                assert(*bb_iter == it->second);
                bb_iter++;
                it = new_bbs.erase(it);
            } else {
                ++it;
            }
        }
    }
}


void SplitCriticalEdges::run() {
    for(auto func: *module_->get_functions()){
        split_critical_edges(func);
    }
}

}