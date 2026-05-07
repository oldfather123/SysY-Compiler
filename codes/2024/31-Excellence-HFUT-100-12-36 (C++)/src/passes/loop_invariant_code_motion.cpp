#include "loop_invariant_code_motion.hpp"
#include "../ir/ir.hpp"
#include "../parser/SyntaxTree.hpp"
#include <algorithm>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

void Passes::LoopInvariantCodeMotion::run() {
    for(auto [_name, fun] : compunit->usrfuncs) {
        auto loops = fun->get_loops();
        // while(!loops.empty()) {
        for(auto [key, loop_vec] : loops) {
            // auto loop_ins = loops.top();
            // loops.pop();
            for(auto loop_ins : loop_vec) {
                auto invariants = analyse_invariant(loop_ins);
                if(!invariants.empty() && invariants.size() < 90) {
                    motion(loop_ins, invariants);
                }
            }
        }
    }
}

void Passes::LoopInvariantCodeMotion::motion(ptr<ir::while_loop> while_loop, ptr_list<ir::ir_instr> invariants) {
    auto blocks = get_body(while_loop);
    auto fun = while_loop->get_fun();
    auto entry = while_loop->get_cond_from();
    auto new_block = fun->new_block();
    ptr_list<ir::ir_basicblock> predecessors;
    for(auto pre : fun->check_predecessor(while_loop->get_cond_from())) {
        if(blocks.find(pre) == blocks.end()) {       // 去除while_body中跳回cond_from的块
            predecessors.push_back(pre);
        }
    }


    for(auto ins : entry->get_instructions()) {
        auto is_phi = std::dynamic_pointer_cast<ir::phi>(ins);
        if(!is_phi) break;
        std::vector<std::pair<ptr<ir::ir_value>, std::weak_ptr<ir::ir_basicblock>>> loop_back;
        std::vector<std::pair<ptr<ir::ir_value>, std::weak_ptr<ir::ir_basicblock>>> loop_in;
        for(auto [reg, weak_block] : is_phi->uses) {
            auto block = weak_block.lock();
            if(blocks.find(block) != blocks.end()) {
                loop_back.emplace_back(reg, block);
            }
            else {
                loop_in.emplace_back(reg, block);
            }
        }
        if(loop_in.size() > 1) {
            auto type = loop_in.begin()->first->get_type();
            auto reg = loop_in.begin()->second.lock()->get_fun()->new_reg(type);
            auto phi = std::make_shared<ir::phi>(reg);
            new_block->push_back(phi);
            for(auto [reg, weak_block] : loop_in) {
                phi->uses.emplace_back(reg, weak_block);
            }
            is_phi->uses.clear();
            for(auto [reg, weak_block] : loop_back) {
                is_phi->uses.emplace_back(reg, weak_block);
            }
            is_phi->uses.emplace_back(phi->dst, new_block);
        }
        else {
            is_phi->uses.clear();
            for(auto [reg, weak_block] : loop_in) {
                is_phi->uses.emplace_back(reg, new_block);
            }
            for(auto [reg, weak_block] : loop_back) {
                is_phi->uses.emplace_back(reg, weak_block);
            }
        }
    }

    for(auto ins : invariants) {
        ins->get_block()->erase(ins->get_block()->search(ins));
        new_block->push_back(ins);
        ins->mark_block(new_block);
    }

    new_block->push_back(std::make_shared<ir::jump>(entry));
    auto &pre_ref = fun->get_predecessor_ref();
    auto &suc_ref = fun->get_nxt_ref();
    pre_ref[entry].push_back(new_block);
    suc_ref[new_block].push_back(entry);

    for(auto pre : predecessors) {
        auto jump_ins = pre->get_instructions().back();
        auto is_jump = std::dynamic_pointer_cast<ir::jump>(jump_ins);
        auto is_br = std::dynamic_pointer_cast<ir::br>(jump_ins);
        if(is_jump) {
            is_jump->replace_target(new_block);
        }
        else if(is_br) {
            if(is_br->get_target_true() == entry) {
                is_br->set_target_true(new_block);
            }
            else {
                is_br->set_target_false(new_block);
            }
        }
        pre_ref[entry].erase(std::find(pre_ref[entry].begin(), pre_ref[entry].end(), pre));
        pre_ref[new_block].push_back(pre);
        suc_ref[pre].erase(std::find(suc_ref[pre].begin(), suc_ref[pre].end(), entry));
        suc_ref[pre].push_back(new_block);
    }
}

std::unordered_set<ptr<ir::ir_basicblock>> Passes::LoopInvariantCodeMotion::get_body(ptr<ir::while_loop> while_loop) {
    auto cond = while_loop->get_cond_from();
    auto out_block = while_loop->get_out_block();
    auto func = while_loop->get_fun();
    std::unordered_set<ptr<ir::ir_basicblock>> loop_bodys;
    std::unordered_set<ptr<ir::ir_basicblock>> work_lst = {cond};
    while(!work_lst.empty()) {
        std::unordered_set<ptr<ir::ir_basicblock>> nxt;
        for(auto current : work_lst) {
            auto what = func->check_nxt(current);
            for(auto block : func->check_nxt(current)) {
                if(block != out_block && loop_bodys.find(block) == loop_bodys.end()) {
                    loop_bodys.insert(block);
                    nxt.insert(block);
                }
            }
        }
        work_lst = nxt;
    }
    return loop_bodys;
}

ptr_list<ir::ir_instr> Passes::LoopInvariantCodeMotion::analyse_invariant(ptr<ir::while_loop> while_loop) {
    ptr_list<ir::ir_instr> res;
    std::unordered_set<ptr<ir::ir_reg>> def_var;
    auto blocks = get_body(while_loop);
    // blocks.erase(blocks.find(while_loop->get_cond_from()));
    std::map<ptr<ir::ir_instr>, ptr_list<ir::ir_reg>> store_map;
    // std::map<ptr<ir::ir_reg>, ptr_list<ir::ir_instr>> store_rev;
    std::unordered_set<ptr<ir::ir_reg>> store_set;
    std::map<ptr<ir::ir_instr>, ptr<ir::ir_reg>> load_map;
    bool have_impure_call = false;

    for(auto block : blocks) {
        for(auto ins : block->get_instructions()) {
            for(auto def : ins->def_reg()) {
                def_var.insert(def);
            }
            auto is_store = std::dynamic_pointer_cast<ir::store>(ins);
            auto is_load = std::dynamic_pointer_cast<ir::load>(ins);
            auto is_call = std::dynamic_pointer_cast<ir::func_call>(ins);
            if(is_store) {
                auto addr = is_store->get_addr();
                // store_rev[addr].push_back(ins);
                store_set.insert(addr);
                store_map[ins].push_back(addr);
                auto is_get_element = std::dynamic_pointer_cast<ir::get_element_ptr>(addr->get_def_loc());
                if(is_get_element) {
                    auto base_addr = is_get_element->get_base();
                    store_set.insert(base_addr);
                    store_map[ins].push_back(base_addr);
                }
            }
            if(is_load) {
                load_map[ins] = is_load->get_addr();
            }
            if(is_call && !is_call->get_callee()->check_pure()) {
                have_impure_call = true;
            }
        }
    }
    auto while_br = while_loop->get_cond_from()->get_instructions().back();
    auto br = std::dynamic_pointer_cast<ir::br>(while_br);
    if(br && br->use_reg().size()) {
        auto cmp = std::dynamic_pointer_cast<ir::cmp_ins>(br->use_reg().front()->get_def_loc());
        assert(cmp);
        for(auto reg : cmp->use_reg()) {
            def_var.insert(reg);
        }
    }
    bool changed = true;
    while(changed) {
        changed = false;
        for(auto block : blocks) {
            for(auto ins : block->get_instructions()) {
                bool cur_is_invariant = true;
                auto is_call = std::dynamic_pointer_cast<ir::func_call>(ins);
                auto is_alloca = std::dynamic_pointer_cast<ir::alloc>(ins);
                auto is_ret = std::dynamic_pointer_cast<ir::ret>(ins);
                auto is_br = std::dynamic_pointer_cast<ir::br>(ins);
                auto is_cmp = std::dynamic_pointer_cast<ir::cmp_ins>(ins);
                auto is_phi = std::dynamic_pointer_cast<ir::phi>(ins);
                auto is_store = std::dynamic_pointer_cast<ir::store>(ins);
                auto is_load = std::dynamic_pointer_cast<ir::load>(ins);
                auto is_jump = std::dynamic_pointer_cast<ir::jump>(ins);
                auto is_while = std::dynamic_pointer_cast<ir::while_loop>(ins);
                auto is_bc = std::dynamic_pointer_cast<ir::break_or_continue>(ins);
                // auto is_control = std::dynamic_pointer_cast<ir::control_ins>(ins);

                if(is_alloca || is_ret || is_br || is_cmp || is_phi || is_jump || is_while || is_store || is_bc)
                    continue;
                bool def_in_loop = false;
                for(auto def : ins->def_reg()) {
                    if(def_var.find(def) == def_var.end()) {
                        def_in_loop = true;
                        break;
                    }
                }
                if(def_in_loop) continue;
                for(auto use : ins->use_reg()) {
                    if(def_var.find(use) != def_var.end()){
                        cur_is_invariant = false;
                    }
                }
                // if(is_load) {
                //     auto def = is_load->use_reg().front();
                //     if(!def->check_global()) continue;
                // }
                if(is_store) {
                    continue;
                }
                if(cur_is_invariant && is_load) {
                    if(have_impure_call)
                        continue;
                    auto addr = load_map[ins];
                    ptr<ir::ir_reg> elem_tar = nullptr;
                    auto is_get_element = std::dynamic_pointer_cast<ir::get_element_ptr>(addr->get_def_loc());
                    if(is_get_element) {
                        elem_tar = is_get_element->get_base();
                    }
                    // for(auto pair : store_map){
                    //     if (pair.second == addr || elem_tar == addr) {
                    //         cur_is_invariant = false;
                    //         break;
                    //     }
                    // }
                    if(store_set.find(addr) != store_set.end() || store_set.find(elem_tar) != store_set.end()) continue;
                }

                if(is_call) {
                    if(!is_call->get_callee()->check_pure()) {
                        cur_is_invariant = false;
                    }
                }
                if(cur_is_invariant) {
                    for(ptr<ir::ir_reg> value : ins->def_reg()) {
                        def_var.erase(value);
                    }
                    res.push_back(ins);
                    changed = true;
                }
            }
        }
    }
    return res;
}