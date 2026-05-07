#include "middleend/dead_code_eliminate.hpp"
#define TypeCase(res, type, expr) if (auto res = dynamic_cast<type>(expr))

namespace middleend {

// CLEAN Algo
bool DeadCodeEliminate::eliminate_useless_cf_one_pass(ir::Function *func){
    func->clear_visit();
    CFG *cfg = new CFG(func);
    std::vector<ir::BasicBlock*> bb_stack;
    if(func->get_basic_blocks()->empty()){
        // no bbs in this function
        return false;
    }
    bb_stack.push_back(func->get_basic_blocks()->front());
    std::vector<ir::BasicBlock*> order;
    func->get_basic_blocks()->front()->visit = true;
    ir::BasicBlock* entry = func->get_basic_blocks()->front();
    // traverse in a bfs order using bb_stack, and get the order 
    while(!bb_stack.empty()){
        auto bb = bb_stack.back();
        bb_stack.pop_back();
        order.push_back(bb);
        auto succ_bb = cfg->get_bb_succ(bb->get_index());
        for(auto next: *succ_bb){
            if(!cfg->get_bb(next)->visit){
                bb_stack.push_back(cfg->get_bb(next));
                cfg->get_bb(next)->visit = true;
            }
        }
    }
    bool ret = false;
    func->clear_visit();
    for(int i = order.size() - 1; i >= 0; i--){
        auto bb = order[i];
        // std::cout << bb->get_name() << " in " << std::endl;
        auto inst = bb->get_instructions()->back();
        TypeCase(branch, ir::instruction::CondBranch *, inst) {
            if(branch->get_true_bb() == branch->get_false_bb()){
                // std::cout << "1" << std::endl;
                bb->get_instructions()->pop_back();
                inst = new ir::instruction::Branch(branch->get_true_bb());
                bb->get_instructions()->push_back(inst);
                inst->set_parent(bb);
                ret = true;
            }
        }
        TypeCase(jmp, ir::instruction::Branch *, inst) {
            auto target = jmp->get_bb();
            if(bb->get_instructions()->size() == 1){
                // std::cout << "2" << std::endl;
                if(auto phi = dynamic_cast<ir::instruction::Phi*>(target->get_instructions()->front())){
                    bool eliminate = true;
                    for(auto each: *cfg->get_bb_prev(bb->get_index())){
                        if((*cfg->get_bb_prev(target->get_index())).count(each)){
                            eliminate = false;
                            break;
                        }
                    }
                    if(!eliminate){
                        continue;
                    }
                }
                if(entry == bb){
                    continue;
                }
                for(auto pre: *cfg->get_bb_prev(bb->get_index())){
                    auto pre_bb = cfg->get_bb(pre);
                    if(cfg->get_bb_succ(pre)->count(bb->get_index())){
                        auto inst = pre_bb->get_instructions()->back();
                        cfg->erase_from_succ(pre_bb, bb);
                        cfg->insert_succ(pre_bb, target);
                        TypeCase(br, ir::instruction::CondBranch *, inst) {
                            if(br->get_true_bb() == bb) {
                                br->set_true_bb(target);
                            }
                            if(br->get_false_bb() == bb) {
                                br->set_false_bb(target);
                            }
                        }
                        TypeCase(jmp, ir::instruction::Branch *, inst) {
                            if(jmp->get_bb() == bb){
                                jmp->set_target(target);
                            }
                        }
                    }
                    cfg->insert_prev(target, pre_bb);
                    for(auto ins: *target->get_instructions()){
                        TypeCase(phi, ir::instruction::Phi *, ins){
                            if(auto b = phi->get_src_temp(bb)){
                                phi->add_src_and_bb(pre_bb, b);
                            }
                        } else {
                            break;
                        }
                    }
                }
                cfg->erase_from_prev(target, bb);
                for(auto ins : *target->get_instructions()){
                    TypeCase(phi, ir::instruction::Phi *, ins){
                        phi->erase_src_temp(bb);
                    } else {
                        break;
                    }
                }
                bb->visit = true;
                ret = true;
                continue;
            }
            if(cfg->get_bb_prev(target->get_index())->size() == 1){
                // std::cout << "3" << std::endl;
                // only this block will jump to target
                cfg->erase_from_succ(bb, target);
                auto sc = *cfg->get_bb_succ(target->get_index());
                for(auto s: sc){
                    cfg->insert_succ(bb, cfg->get_bb(s));
                    // std::cout << "hahah:" << cfg->get_bb(s)->get_name() << std::endl;
                }
                bb->get_instructions()->pop_back();
                for(auto suc: sc){
                    auto sub = cfg->get_bb(suc);
                    cfg->erase_from_prev(sub, target);
                    cfg->insert_prev(sub, bb);
                    for(auto inst: *sub->get_instructions()){
                        TypeCase(phi, ir::instruction::Phi *, inst) {
                            if(auto b = phi->get_src_temp(target)){
                                // phi->print(std::cout);
                                phi->add_src_and_bb(bb, b);
                                // phi->print(std::cout);
                                phi->erase_src_temp(target);
                                // phi->print(std::cout);
                            }
                        } else {
                            break;
                        }
                    }
                }
                for(auto iter = target->get_instructions()->begin(); iter != target->get_instructions()->end();){
                    TypeCase(phi, ir::instruction::Phi *, *iter){
                        assert(phi->get_src()->size() == 1);
                        // TODO: there is something there
                        iter = target->get_instructions()->erase(iter);
                    } else {
                        (*iter)->set_parent(bb);
                        iter++;
                    }
                }
                for(auto inst: *target->get_instructions()){
                    bb->get_instructions()->push_back(inst);
                }
                target->get_instructions()->clear();
                target->visit = true;
                sc = *cfg->get_bb_succ(target->get_index());
                for(auto s: sc){
                    cfg->erase_from_succ(target, cfg->get_bb(s));
                }
                ret = true;
                continue;
            }
            if(target->get_instructions()->size() == 1){
                // std::cout << "4" << std::endl;
                TypeCase(br, ir::instruction::CondBranch *, target->get_instructions()->back()){
                    cfg->erase_from_succ(bb, target);
                    cfg->erase_from_prev(target, bb);
                    cfg->insert_succ(bb, br->get_true_bb());
                    cfg->insert_succ(bb, br->get_false_bb());
                    cfg->insert_prev(br->get_true_bb(), bb);
                    cfg->insert_prev(br->get_false_bb(), bb);
                    for(auto ins: *br->get_true_bb()->get_instructions()){
                        TypeCase(phi, ir::instruction::Phi *, ins){
                            if(auto b = phi->get_src_temp(target)){
                                phi->add_src_and_bb(bb, b);
                            }
                        } else {
                            break;
                        }
                    }
                    for(auto ins: *br->get_false_bb()->get_instructions()){
                        TypeCase(phi, ir::instruction::Phi *, ins){
                            if(auto b = phi->get_src_temp(target)){
                                phi->add_src_and_bb(bb, b);
                            }
                        } else {
                            break;
                        }
                    }
                    auto new_inst = new ir::instruction::CondBranch(br->get_type(), br->get_true_bb(), br->get_false_bb(), br->getcond());
                    bb->get_instructions()->pop_back();
                    bb->get_instructions()->push_back(new_inst);
                    new_inst->set_parent(bb);
                    ret = true;
                    continue;
                }
            }
        }
        // for(auto bb: *func->get_basic_blocks()){
        //     std::cout << bb->get_name() << std::endl;
        //     for(auto inst: *bb->get_instructions()){
        //         inst->print(std::cout);
        //     }
        // }
    }
    for(auto iter = func->get_basic_blocks()->begin(); iter != func->get_basic_blocks()->end();){
        if((*iter)->visit){
            // std::cout << (*iter)->get_name() << std::endl;
            iter = func->get_basic_blocks()->erase(iter);
        } else {
            iter++;
        }
    }
    return ret;
}

void DeadCodeEliminate::run() {
    for(auto func: *module_->get_functions()){
        bool change = true;
        while(change){
            // std::cout << "begin" << std::endl;
            change = eliminate_useless_cf_one_pass(func);
            // std::cout << "end" << std::endl;
        }
        // for(auto bb: *func->get_basic_blocks()){
        //     std::cout << bb->get_name() << std::endl;
        //     for(auto inst: *bb->get_instructions()){
        //         inst->print(std::cout);
        //     }
        // }
    }
}

}