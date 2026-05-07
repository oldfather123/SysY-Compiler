#include "coloring_allocator.hpp"
#include "../ir/ir.hpp"
#include "arch.hpp"
#include "register_allocator.hpp"
#include "../parser/SyntaxTree.hpp"
#include <cassert>
// #include <cstdlib>
#include <iterator>
#include <list>
#include <memory>
// #include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

LoongArch::ColoringAllocator::ColoringAllocator(ptr<ir::ir_userfunc> fun, int base_reg, ptr_list<ir::global_def> global_var) : fun(fun) {
    if(!fun->check_analysed()) {        // spill时改变代码不会改变控制流的结构
        analyse_cfg_flow();
    }
}

LoongArch::alloc_res LoongArch::ColoringAllocator::run(Rtype target) {
        switch (target) {
        case Rtype::INT: {
            is_target = [](ptr<ir::ir_reg> ireg) {
                return ireg->get_type() != vartype::FLOAT/* && ireg->get_type() != vartype::FBOOL*/;
            };
            this->using_color = i_color;
            break;
        }
        case Rtype::FLOAT: {
            is_target = [](ptr<ir::ir_reg> ireg) {
                return ireg->get_type() == vartype::FLOAT;
            };
            this->using_color = f_color;
            break;
        }
        // case Rtype::FBOOL: {
        //     is_target = [](ptr<ir::ir_reg> ireg) {
        //         return ireg->get_type() == vartype::FBOOL;
        //     };
        //     this->using_color = fb_color;
        //     break;
        // }
    }
    this->dealing = target;

    bool success = false;
    int rewrite_cnt = 0;
    while(!success) {
        clear();
        // analyse_live();
        build_ig();
        if(kempe()) {
            rewrite();
            rewrite_cnt++;
            continue;
        }

        success = true;
    }


    std::vector<std::shared_ptr<ir::ir_reg>> empty_spill;
    std::vector<std::shared_ptr<ir::ir_memobj>> empty_alloc;
    alloc_res res(mapping_to_reg, empty_spill, empty_alloc);
    return res;
}

bool LoongArch::ColoringAllocator::rewrite() {
    if(spill_set.empty()) {
        return false;
    }
    // std::unordered_map<vartype, vartype> base_type = {
    // //    {vartype::FLOATADDR, "float"},
    //     {vartype::FLOATADDR, vartype::FLOAT},
    //     {vartype::INTADDR, vartype::INT},
    //     {vartype::BOOLADDR, vartype::BOOL},
    //     {vartype::FBOOLADDR, vartype::FBOOL}
    // };
    // std::clog << "rewrite!" << std::endl;
    // std::clog << "rewrite!" << std::endl;
    // std::clog << "rewrite!" << std::endl;
    // std::clog << "rewrite!" << std::endl;
    // std::clog << "rewrite!" << std::endl;
    // std::clog << "rewrite!" << std::endl;
    // std::clog << "rewrite!" << std::endl;
    // std::clog << "rewrite!" << std::endl;
    // std::clog << "rewrite!" << std::endl;
    // std::clog << "rewrite!" << std::endl;
    std::unordered_map<ptr<ir::ir_reg>, ptr<ir::ir_memobj>> spill_map;
    std::set<ptr<ir::ir_reg>> phi_side_effect;
    std::set<ptr<ir::ir_reg>> all_spilled = spill_set;
    std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map(1);
    while(!spill_set.empty()) {
        phi_side_effect.clear();
        for(auto reg : spill_set) {
            reg->mark_unspillable();
            ptr<ir::ir_basicblock> def_block;
            ptr<ir::ir_instr> def_ins;
            if(reg->check_is_param()) {
                def_block = fun->get_entry();
            }
            else {
                def_ins = reg->get_def_loc();
                def_block = def_ins->get_block();
            }
            ptr<ir::ir_memobj> spill_obj = nullptr;
            // auto spill_obj = fun->new_spill_obj(std::to_string(reg->get_id()) + "spilled_virtual_reg", reg->get_type());
            ptr_list<ir::ir_basicblock> work_lst = {def_block};
            ptr_list<ir::ir_basicblock> nxt_iter;
            std::unordered_map<ptr<ir::ir_basicblock>, bool> visit;
            visit[def_block] = true;
            while(!work_lst.empty()) {
                nxt_iter.clear();
                for(auto block : work_lst) {
                    std::vector<std::pair<std::list<ptr<ir::ir_instr>>::iterator, ptr<ir::ir_reg>>> load_vec;
                    std::vector<std::pair<std::list<ptr<ir::ir_instr>>::iterator, ptr<ir::ir_reg>>> change_vec;
                    // auto ins = block->get_instructions();
                    std::list<ptr<ir::ir_instr>>::iterator it;
                    if(block == def_block) {
                        if(reg->check_is_param()) {
                            spill_obj = fun->new_spill_obj(std::to_string(reg->get_id()) + "spilled_virtual_reg", reg->get_type());
                            fun->insert_spilled_args(reg, spill_obj);
                            // block->insert_spill(block->get_ins_begin(), std::make_shared<ir::store>(spill_obj->get_addr(), reg));
                            it = block->get_ins_begin();
                            // it++;
                            spill_map[reg] = spill_obj;
                        }
                        else {
                            it = block->search(def_ins);
                            auto is_phi = std::dynamic_pointer_cast<ir::phi>(def_ins);
                            // assert(it != block->get_ins_end());
                            if(is_phi) {
                                int rank = def_block->get_phi_rank(is_phi);
                                if(it != block->get_ins_end()) {            // 判断phi指令是否已经在上一轮循环中处理，处理后仍须执行rename操作
                                    auto spill_map_it = spill_map.find(reg);
                                    if(spill_map_it != spill_map.end()) {
                                        spill_obj = spill_map_it->second;
                                    }
                                    else {
                                        spill_obj = fun->new_spill_obj(std::to_string(reg->get_id()) + "spilled_virtual_reg", reg->get_type());
                                        spill_map[reg] = spill_obj;
                                    }
                                    for(auto [value, block_from] : is_phi->uses) {
                                        assert(!block_from.expired());
                                        auto reg_from = std::dynamic_pointer_cast<ir::ir_reg>(value);
                                        std::list<ptr<ir::ir_instr>>::iterator def_it;
                                        if(reg_from) {
                                            if(reg_from->check_is_param()) {
                                                auto entry = fun->get_entry();
                                                if(entry == block_from.lock()) {
                                                    auto par_it = spill_map.find(reg_from);
                                                    if(par_it == spill_map.end()) {             // 这个param还没有被处理spill或者不用被处理spill
                                                        entry->insert_after_phi(std::make_shared<ir::store>(spill_obj->get_addr(), value));
                                                    }
                                                    else {                                      // 这个param已经被spill处理过了，应该有加载
                                                        auto par_obj = par_it->second;
                                                        assert(par_obj);
                                                        auto middle = fun->new_spill_reg(reg_from, par_obj);
                                                        entry->insert_after_phi(std::make_shared<ir::store>(spill_obj->get_addr(), middle));
                                                        entry->insert_after_phi(std::make_shared<ir::load>(middle, par_obj->get_addr()));
                                                    }
                                                    // entry->insert_after_phi(std::make_shared<ir::store>(spill_obj->get_addr(), value));
                                                    auto addr = spill_obj->get_addr();
                                                    int id = addr->get_id();
                                                    int what;
                                                }
                                                else {
                                                    auto reg_in_spill = spill_map.find(reg_from);
                                                    if(reg_in_spill == spill_map.end()) {
                                                        block_from.lock()->insert_phi_spill(std::make_shared<ir::store>(spill_obj->get_addr(), value), rank);
                                                    }
                                                    else {
                                                        auto par_obj = reg_in_spill->second;
                                                        auto load_reg = fun->new_spill_reg(reg_from, par_obj);
                                                        block_from.lock()->insert_phi_spill(std::make_shared<ir::load>(load_reg, reg_in_spill->second->get_addr()), rank);
                                                        block_from.lock()->insert_phi_spill(std::make_shared<ir::store>(spill_obj->get_addr(), load_reg), rank);
                                                    }
                                                }
                                            }
                                            else {
                                                auto def_block = reg_from->get_def_loc()->get_block();
                                                if(def_block == block_from.lock()) {
                                                    def_it = def_block->search(reg_from->get_def_loc());
                                                    // if(def_it != def_block->get_ins_end()) {            // 表示的是如果def指令是除phi之外的指令或者是目前没被消解的phi指令
                                                    //     def_it = std::next(def_it);
                                                    //     auto block_end = def_block->get_ins_end();
                                                    //     while(def_it != block_end) {
                                                    //         auto is_phi = std::dynamic_pointer_cast<ir::phi>(*def_it);
                                                    //         if(is_phi) {
                                                    //             def_it = std::next(def_it);
                                                    //         }
                                                    //         else {
                                                    //             break;
                                                    //         }
                                                    //     }
                                                    //     def_block->insert_spill(def_it, std::make_shared<ir::store>(spill_obj->get_addr(), reg_from));
                                                    // }
                                                    // else {
                                                        auto reg_in_spill = spill_map.find(reg_from);
                                                        // assert(reg_in_spill != spill_map.end());
                                                        ptr<ir::ir_memobj> par_obj;
                                                        if(reg_in_spill != spill_map.end()) {
                                                            par_obj = reg_in_spill->second;
                                                            auto load_reg = fun->new_spill_reg(reg_from, par_obj);
                                                            def_block->insert_phi_spill(std::make_shared<ir::load>(load_reg, reg_in_spill->second->get_addr()), rank);
                                                            def_block->insert_phi_spill(std::make_shared<ir::store>(spill_obj->get_addr(), load_reg), rank);
                                                        }
                                                        else {
                                                            par_obj = spill_obj;
                                                            def_block->insert_phi_spill(std::make_shared<ir::store>(spill_obj->get_addr(), value), rank);
                                                        }

                                                    // }
                                                }
                                                else {
                                                    auto reg_in_spill = spill_map.find(reg_from);
                                                    if(reg_in_spill == spill_map.end()) {
                                                        block_from.lock()->insert_phi_spill(std::make_shared<ir::store>(spill_obj->get_addr(), value), rank);
                                                    }
                                                    else {
                                                        auto par_obj = reg_in_spill->second;
                                                        auto load_reg = fun->new_spill_reg(reg_from, par_obj);
                                                        block_from.lock()->insert_phi_spill(std::make_shared<ir::load>(load_reg, reg_in_spill->second->get_addr()), rank);
                                                        block_from.lock()->insert_phi_spill(std::make_shared<ir::store>(spill_obj->get_addr(), load_reg), rank);
                                                    }
                                                }
                                            }



                                            // // if(reg_from == reg) continue;
                                            // if(reg_from->check_is_param()) {
                                            //     def_it = fun->get_entry()->get_ins_begin();
                                            //     // fun->get_entry()->insert_spill(def_it, std::make_shared<ir::store>(spill_obj->get_addr(), value));   // 已经添加至最初始的加载名单
                                            // }
                                            // else {
                                            //     auto reg_from_def_block = reg_from->get_def_loc()->get_block();
                                            //     def_it = reg_from_def_block->search(reg_from->get_def_loc());
                                            //     // auto is_deleted = reg_from_def_block->search(reg_from->get_def_loc());
                                            //     if(def_it == reg_from_def_block->get_ins_end()) {
                                            //         auto spill_map_it = spill_map.find(reg_from);
                                            //         assert(spill_map_it != spill_map.end());
                                            //         auto phi_obj = spill_map_it->second;
                                            //         auto load_reg = fun->new_spill_reg(reg_from);
                                            //         auto load_ins = std::make_shared<ir::load>(load_reg, phi_obj->get_addr());
                                            //         reg_from_def_block->insert_after_phi(load_ins);
                                            //         auto load_it = reg_from_def_block->search(load_ins);
                                            //         auto store_it = std::next(load_it);
                                            //         reg_from_def_block->insert_spill(store_it, std::make_shared<ir::store>(spill_obj->get_addr(), load_reg));
                                            //     }
                                            //     else {
                                            //         def_it = std::next(def_it);             // 避免在def_ins的前面进行插入
                                            //         auto block_end = reg_from_def_block->get_ins_end();
                                            //         while(def_it != block_end) {                // 由于既要保证在def_it之后又要保证在phi之后，所以没法直接使用insdert_after_phi
                                            //             auto is_phi = std::dynamic_pointer_cast<ir::phi>(*def_it);
                                            //             if(is_phi) {
                                            //                 def_it = std::next(def_it);
                                            //             }
                                            //             else {
                                            //                 break;
                                            //             }
                                            //         }

                                            //         if(all_spilled.find(reg_from) != all_spilled.end()) {
                                            //             phi_side_effect.insert(reg_from);
                                            //         }
                                            //         reg_from_def_block->insert_spill(def_it, std::make_shared<ir::store>(spill_obj->get_addr(), value));
                                            //     }
                                            // }
                                        }
                                        else {
                                            block_from.lock()->insert_phi_spill(std::make_shared<ir::store>(spill_obj->get_addr(), value), rank);
                                            // def_it = block_from->get_ins_end();
                                            // auto begin = block_from->get_ins_begin();
                                            // while(def_it != begin) {
                                            //     auto prev_it = std::prev(def_it);
                                            //     auto is_jump = std::dynamic_pointer_cast<ir::jump>(*prev_it);
                                            //     auto is_br = std::dynamic_pointer_cast<ir::br>(*prev_it);
                                            //     auto is_bc = std::dynamic_pointer_cast<ir::break_or_continue>(*prev_it);
                                            //     auto is_while_loop = std::dynamic_pointer_cast<ir::while_loop>(*prev_it);
                                            //     if(is_jump == nullptr && is_br == nullptr && is_bc == nullptr && is_while_loop == nullptr) {
                                            //         break;
                                            //     }
                                            //     else {
                                            //         def_it = prev_it;
                                            //     }
                                            // }
                                            // block_from->insert_spill(def_it, std::make_shared<ir::store>(spill_obj->get_addr(), value));
                                        }
                                    }
                                    auto del_it = it;
                                    it = std::next(it);         // 先指向下一个语句，以免删除时it受影响而失效
                                    block->erase(del_it);
                                    // phi_side_effect.erase(reg);     // 如果mark_spill但是def本来也被spill的话，在第一遍phi就被处理了，所以需要删掉
                                    phi_side_effect.insert(reg);
                                }
                                else {
                                    auto spill_map_it = spill_map.find(reg);
                                    assert(spill_map_it != spill_map.end());
                                    spill_obj = spill_map_it->second;
                                }
                            }
                            else {
                                spill_obj = fun->new_spill_obj(std::to_string(reg->get_id()) + "spilled_virtual_reg", reg->get_type());
                                spill_map[reg] = spill_obj;
                                // it = std::find(ins.begin(), ins.end(), def_ins);
                                auto store_it = std::next(it);
                                block->insert_spill(store_it, std::make_shared<ir::store>(spill_obj->get_addr(), reg), false);
                                it++;   // 此时指向插入的store语句
                                it++;   // 此时指向原文的下一条语句
                            }
                        }
                    }
                    else {
                        it = block->get_ins_begin();
                    }

                    assert(spill_map.find(reg) != spill_map.end());
                    assert(spill_obj);
                    bool cur_block_is_finished = false;
                    auto end = block->get_ins_end();
                    for(; it != end; it++) {
                        auto cur_ins = *it;
                        auto is_phi = std::dynamic_pointer_cast<ir::phi>(cur_ins);
                        auto is_call = std::dynamic_pointer_cast<ir::func_call>(cur_ins);
                        auto is_tail = std::dynamic_pointer_cast<ir::tail_call>(cur_ins);
                        auto is_get_element = std::dynamic_pointer_cast<ir::get_element_ptr>(cur_ins);
                        
                        // if(live_in[cur_ins].find(reg) != live_in[cur_ins].end()) {    // 这个reg在当前指令处仍然活跃
                            auto use = cur_ins->use_reg();
                            if(std::find(use.begin(), use.end(), reg) != use.end()) {   // 这个reg活跃且被使用
                                if(is_phi) {
                                    auto def = is_phi->def_reg();
                                    auto uses = is_phi->uses;
                                    for(auto d : def) {
                                        auto spill_map_it = spill_map.find(d);
                                        if(spill_map_it == spill_map.end()) {
                                            bool def_at_from = false;
                                            // for(auto [use_value, from] : uses) {
                                            //     auto use_reg = std::dynamic_pointer_cast<ir::ir_reg>(use_value);
                                            //     if(use_reg == reg) {
                                            //         if(use_reg->check_is_param()) {
                                            //             def_at_from = false;
                                            //         }
                                            //         else {
                                            //             def_at_from = (use_reg->get_def_loc()->get_block() == from);
                                            //         }
                                            //         break;
                                            //     }
                                            // }
                                            if(def_at_from) {
                                                spill_map[d] = spill_obj;
                                            }
                                            else {
                                                spill_map[d] = fun->new_spill_obj(std::to_string(d->get_id()) + "spilled_virtual_reg", d->get_type());
                                            }
                                            phi_side_effect.insert(d);
                                            auto def_ins = d->get_def_loc();
                                            auto def_block = def_ins->get_block();
                                            if(def_ins) {
                                                assert(def_block->search(def_ins) != def_block->get_ins_end());
                                            }
                                        }
                                    }
                                    // change_vec.push_back({it, is_phi->dst});
                                    // for(auto [value, block_from] : is_phi->uses) {
                                    //     auto reg_from = std::dynamic_pointer_cast<ir::ir_reg>(value);
                                    //     std::list<ptr<ir::ir_instr>>::iterator def_it;
                                    //     if(reg_from) {
                                    //         if(reg_from == reg) continue;
                                    //         def_it = block_from->search(reg_from->get_def_loc());
                                    //         def_it = std::next(def_it);
                                    //     }
                                    //     else {
                                    //         def_it = block_from->get_ins_end();
                                    //         auto begin = block_from->get_ins_begin();
                                    //         while(def_it != begin) {
                                    //             auto prev_it = std::prev(def_it);
                                    //             auto is_jump = std::dynamic_pointer_cast<ir::jump>(*prev_it);
                                    //             auto is_br = std::dynamic_pointer_cast<ir::br>(*prev_it);
                                    //             auto is_bc = std::dynamic_pointer_cast<ir::break_or_continue>(*prev_it);
                                    //             auto is_while_loop = std::dynamic_pointer_cast<ir::while_loop>(*prev_it);
                                    //             if(is_jump == nullptr && is_br == nullptr && is_bc == nullptr && is_while_loop == nullptr) {
                                    //                 break;
                                    //             }
                                    //             else {
                                    //                 def_it = prev_it;
                                    //             }
                                    //         }
                                    //     }
                                    //     block_from->insert_spill(def_it, std::make_shared<ir::store>(spill_obj->get_addr(), value));
                                    // }
                                }
                                else if(is_call) {
                                    auto load_reg = fun->new_spill_reg(reg, spill_obj);    // 换成load_reg的作用是能够将rewrite的中间代码通过llvm的检查编译，如果直接转汇编可以不换
                                    load_reg->mark_local();                                  // 仅仅是用来绕过下一次的live分析，可能会因为冲突换成另一个标志
                                    is_call->insert_spilled_obj(load_reg, spill_obj);
                                    replace_map[reg] = load_reg;
                                    cur_ins->replace_reg(replace_map);
                                }
                                else if(is_tail) {
                                    auto load_reg = fun->new_spill_reg(reg, spill_obj);    // 换成load_reg的作用是能够将rewrite的中间代码通过llvm的检查编译，如果直接转汇编可以不换
                                    load_reg->mark_local();                                  // 仅仅是用来绕过下一次的live分析，可能会因为冲突换成另一个标志
                                    is_tail->insert_spilled_obj(load_reg, spill_obj);
                                    replace_map[reg] = load_reg;
                                    cur_ins->replace_reg(replace_map);
                                }
                                else if(is_get_element && reg != is_get_element->get_base()) {
                                    auto load_reg = fun->new_spill_reg(reg, spill_obj);    // 换成load_reg的作用是能够将rewrite的中间代码通过llvm的检查编译，如果直接转汇编可以不换
                                    load_reg->mark_local();                                  // 仅仅是用来绕过下一次的live分析，可能会因为冲突换成另一个标志
                                    is_get_element->insert_spilled_obj(load_reg, spill_obj);
                                    replace_map[reg] = load_reg;
                                    cur_ins->replace_reg(replace_map);
                                }
                                else {
                                    auto load_reg = fun->new_spill_reg(reg, spill_obj);
                                    load_vec.push_back({it, load_reg});
                                    replace_map.clear();
                                    replace_map[reg] = load_reg;
                                    cur_ins->replace_reg(replace_map);
                                }
                            }
                            auto def = cur_ins->def_reg();
                            if(std::find(def.begin(), def.end(), reg) != def.end()) {   // 这个reg活跃且被定值
                                abort();                                                                // 不应该出现这种情况
                            }
                        // }
                        // 由于中途可能插入指令，所以不可以直接判定不再活跃
                        // else {      // 这个reg已经在当前指令已经不再活跃了
                        //     cur_block_is_finished = true;
                        //     auto a = live_out[cur_ins];
                        //     break;  // 没必要继续分析了
                        // }
                    }

                    // 插入load
                    // auto &ins_ref = block->get_instructions_ref();
                    for(auto [load_it, load_reg] : load_vec) {
                        auto id = spill_obj->get_addr()->id;
                        block->insert_spill(load_it, std::make_shared<ir::load>(load_reg, spill_obj->get_addr()), true);
                    }

                    // for(auto [phi_it, _load_reg] : change_vec) {
                    //     block->erase(phi_it);
                    // }
                    // for(auto [_phi_it, load_reg] : change_vec) {
                    //     auto id = spill_obj->get_addr()->id;
                    //     block->insert_after_phi(std::make_shared<ir::load>(load_reg, spill_obj->get_addr()));
                    // }

                    if(cur_block_is_finished) continue;
                    auto successor = fun->check_nxt(block);     // 如果已经抵达了use的终点，则不用寻找这个block的nxt
                    for(auto s : successor) {
                        if(!visit[s]) {
                            visit[s] = true;
                            nxt_iter.push_back(s);
                        }
                    }
                }
                work_lst = nxt_iter;
            }
        }
        spill_set = phi_side_effect;
        for(auto reg : phi_side_effect) {
            all_spilled.insert(reg);
        }
    }
    return true;
}

bool LoongArch::ColoringAllocator::kempe() {
    // auto remove_ig = ig;
    std::unordered_map<ptr<ir::ir_reg>, int> remove_ig;
    for(auto [reg, vec] : ig) {
        remove_ig.insert({reg, vec.size()});
    }
    bool need_spill = false;
    ptr_list<ir::ir_reg> stk;
    int param_num = 0;
    if(dealing == INT) {
        for(auto reg : this->using_color) {
            if(reg.id >= 10 && reg.id <= 17) {
                param_num++;
            }
        }
    }
    int num = 0;
    while(remove_ig.size()) {
        // std::clog << remove_ig.size() << std::endl;
        ptr<ir::ir_reg> del_item = nullptr;
        for(auto [reg, vec] : remove_ig) {
            if(vec < (this->using_color.size() - (reg->check_is_param() ? param_num : 0))) {
                del_item = reg;
                break;
            }
        }
        if(del_item) {
            remove(remove_ig, del_item);
            // assert(assign_color(del_item));
            stk.push_back(del_item);
        }
        else {
            bool spilled = false;
            for(auto [reg, vec] : remove_ig) {
                if(!reg->check_is_unspillable()) {
                    spilled = true;
                    spill_set.insert(reg);
                    remove(remove_ig, reg);
                    // remove(ig, reg);
                    break;
                }
            }
            // assert(spilled);
            if(!spilled) {
                for(auto [reg, vec] : remove_ig) {
                    int cnt = vec;
                    bool unspillable = reg->check_is_unspillable();
                    bool what = true;
                }
            }
            assert(spilled);
            need_spill = true;
        }
    }
    while(!stk.empty()) {
        auto reg = stk.back();
        stk.pop_back();
        assert(assign_color(reg));
    }
    // if(!spill_set.empty())
    //     std::clog << spill_set.size() << std::endl;
    return need_spill;
}

// // 返回值：true代表需要spill，false代表不需要
// bool LoongArch::ColoringAllocator::kempe_opt() {
//     auto remove_ig = ig;
//     ptr_list<ir::ir_reg> stk;
//     bool triggered = false;
//     bool need_spill = false;
//     ptr_list<ir::ir_reg> unsp;
//     while(remove_ig.size()) {
//         auto n = remove(remove_ig, triggered);
//         stk.push_back(n);
//     }
//     while(stk.size()) {
//         auto n = stk.back();
//         stk.pop_back();
//         if(!assign_color(n)) {
//             need_spill = true;
//             if(n->check_is_unspillable()) {
//                 unsp.push_back(n);
//                 continue;
//             }
//             spill_set.insert(n);
//         }
//     }
//     assert(!(need_spill && spill_set.empty()));    // 剩下的全是不可spill的reg了
//     return need_spill;
// }

bool LoongArch::ColoringAllocator::assign_color(ptr<ir::ir_reg> node) {
    vector<Reg> available_color;
    if(node->check_is_param() && dealing == INT) {
        for(auto color : this->using_color) {
            if(!(color.id >= 10 && color.id <= 17)) {
                available_color.push_back(color);
            }
        }
    }
    else {
        available_color = using_color;
    }
    assert(!available_color.empty());
    for(auto reg : ig[node]) {
        auto colored_it = mapping_to_reg.find(reg);
        if(colored_it != mapping_to_reg.end()) {
            auto color = colored_it->second;
            auto available_it = std::find(available_color.begin(), available_color.end(), color);
            if(available_it != available_color.end()) {
                available_color.erase(available_it);
            }
        }
    }
    if(!available_color.empty()) {
        mapping_to_reg[node] = available_color.front();
        return true;
    }
    else {
        return false;
    }
}

ptr<ir::ir_reg> LoongArch::ColoringAllocator::remove(std::unordered_map<ptr<ir::ir_reg>, int> &g, ptr<ir::ir_reg> del_item) {
    assert(!g.empty());
    assert(!this->using_color.empty());
    // ptr<ir::ir_reg> del_item = nullptr;
    // for(auto [reg, vec] : g) {
    //     if(vec.size() < this->using_color.size()) {
    //         del_item = reg;
    //         if(triggered) {
    //             if(reg->check_is_unspillable()) break; // 在已经存在可能溢出的节点时，不可溢出的寄存器应该放到最后
    //         }
    //         else {
    //             if(reg->check_is_unspillable()) break;  // 不可溢出的寄存器优先
    //         }
    //     }
    // }
    // if(del_item == nullptr) {
    //     triggered = true;
    //     for(auto [reg, vec] : g) {
    //         del_item = reg;
    //         if(reg->check_is_unspillable()) break;  // 在可能溢出时，不可溢出的寄存器应该放到最后
    //     }
    // }
    // if(del_item == nullptr) {
    //     del_item = g.begin()->first;
    // }

    assert(del_item);
    // auto iter = g[del_item];
    // for(auto conf : iter) {
    //     auto it = g[conf].find(del_item);
    //     if(it != g[conf].end()) {
    //         g[conf].erase(it);
    //     }
    //     // else {
    //     //     g[conf].erase(del_item);
    //     // }
    // }
    // auto it = g.find(del_item);
    // if(it != g.end()) {
    //     g.erase(it);
    // }
    // g.erase(del_item);
    for(auto conf : ig[del_item]) {
        if(g.find(conf) != g.end()) g[conf]--;
    }
    g.erase(del_item);
    return del_item;
}

void LoongArch::ColoringAllocator::build_ig() {
    ig.clear();
    analyse_live();
    for(auto arg : fun->get_params()) {
        if(is_target(arg->get_addr()))
            ig[arg->get_addr()] = {};
    }
    ptr_list<ir::ir_basicblock> work_lst = {fun->get_entry()};
    ptr_list<ir::ir_basicblock> nxt_iter;
    std::unordered_map<ptr<ir::ir_basicblock>, bool> visit;
    visit[fun->get_entry()] = true;
    while(!work_lst.empty()) {
        nxt_iter.clear();
        for(auto block : work_lst) {
            auto successor = fun->check_nxt(block);
            for(auto s : successor) {
                if(!visit[s]) {
                    visit[s] = true;
                    nxt_iter.push_back(s);
                }
            }

            for(auto ins : block->get_instructions()) {
                // llvm-ir不存在直接的赋值语句（x = y）
                for(auto u : live_out[ins]) {
                    for(auto v : ins->def_reg()) {
                        if(is_target(u) && is_target(v) && u != v) {
                            ig[u].insert(v);
                            ig[v].insert(u);
                        }
                    }
                    auto use_reg = ins->use_reg();
                    for(auto v : use_reg) {
                        if(is_target(u) && is_target(v) && u != v && !v->check_global() && !v->check_local()) {
                            ig[u].insert(v);
                            ig[v].insert(u);
                        }
                    }

                    // for(auto i_it = use_reg.begin(); i_it != use_reg.end(); i_it++) {
                    //     auto u = *i_it;
                    //     if(is_target(u) && !u->check_global() && !u->check_local()) {
                    //         for(auto j_it = std::next(i_it); j_it != use_reg.end(); j_it++) {
                    //             auto v = *j_it;
                    //             if(is_target(v) && !v->check_global() && !v->check_local()) {
                    //                 ig[u].insert(v);
                    //                 ig[v].insert(u);
                    //             }
                    //         }
                    //     }
                    // }
                }
                std::unordered_set<ptr<ir::ir_reg>> conf_set;
                auto use_reg = ins->use_reg();
                for(auto u : use_reg) {
                    if(is_target(u) && !u->check_global() && !u->check_local()) {
                        conf_set.insert(u);
                    }
                }
                for(auto u : use_reg) {
                    if(is_target(u) && !u->check_global() && !u->check_local()) {
                        ig[u].insert(conf_set.begin(), conf_set.end());
                        ig[u].erase(u);
                    }
                }
            }
        }
        work_lst = nxt_iter;
    }
}

void LoongArch::ColoringAllocator::analyse_live() {
    bool changed = true;
    while(changed) {
        changed = false;
        ptr_list<ir::ir_basicblock> work_lst;
        ptr_list<ir::ir_basicblock> nxt_iter;
        std::unordered_map<ptr<ir::ir_basicblock>, bool> visit;
        for(auto block : fun->get_bbs()) {
            if(block->is_ret()) {
                work_lst.push_back(block);
                visit[block] = true;
                // break;
            }
        }
        assert(!work_lst.empty());
        while(!work_lst.empty()) {                        // BFS从出口遍历基本块
            nxt_iter.clear();
            for(auto block : work_lst) {
                // 确认下一层的基本块
                auto predecessor = fun->check_predecessor(block);
                for(auto p : predecessor) {
                    if(!visit[p]) {
                        visit[p] = true;
                        nxt_iter.push_back(p);
                    }
                }

                auto instructions = block->get_instructions();
                for(auto it = instructions.rbegin(); it != instructions.rend(); it++) {
                    auto ins = *it;
                    // auto is_phi = std::dynamic_pointer_cast<ir::phi>(ins);
                    auto &out = live_out[ins];
                    auto bak = live_out[ins];
                    // out.clear();            // 每次迭代都是新增的话clear理应能去除掉
                    if(it == instructions.rbegin()) {
                        auto successor = fun->check_nxt(block);
                        for(auto s : successor) {
                            for(auto in : live_in[s->get_instructions().front()]) {
                                if(is_target(in)) {
                                    out.insert(in);
                                }
                            }
                        }
                    }
                    else {
                        for(auto in : live_in[*std::prev(it)]) {
                            if(is_target(in)) {
                                out.insert(in);
                            }
                        }
                    }
                    changed |= (out != bak);

                    auto &in = live_in[ins];
                    bak = live_in[ins];
                    in = live_out[ins];
                    for(auto def : ins->def_reg()) {
                        // auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(def);
                        // assert(is_reg);
                        // if(is_reg) in.erase(is_reg);
                        if(is_target(def) && !def->check_global() && !def->check_local()) {
                            in.erase(def);
                            ig[def] = {};           // addVertix
                        }
                    }
                    for(auto use : ins->use_reg()) {
                        // auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(use);
                        // assert(is_reg);
                        // if(is_reg) in.insert(is_reg);
                        if(is_target(use) && !use->check_global() && !use->check_local()) {
                            in.insert(use);
                            ig[use] = {};           // addVertix
                        }
                        // if(is_phi) {
                        //     use->mark_unspillable();    // 书上没有，但是phi应该放在首位
                        // }
                    }
                    bool cur_changed = (in != bak);
                    if(!cur_changed) {
                        auto is_call = std::dynamic_pointer_cast<ir::func_call>(ins);
                        if(is_call) {
                            is_call->set_live_in(in, dealing);
                        }
                        auto is_tail_call = std::dynamic_pointer_cast<ir::tail_call>(ins);
                        if(is_tail_call) {
                            is_tail_call->get_call_ins()->set_live_in(in, dealing);
                        }
                    }
                    // changed |= (in != bak);
                    changed |= cur_changed;
                }
            }
            work_lst = nxt_iter;
        }
    }
}

void LoongArch::ColoringAllocator::clear() {
    this->ig.clear();
    this->mapping_to_reg.clear();
    this->spill_set.clear();
    this->live_in.clear();
    this->live_out.clear();
}

void LoongArch::ColoringAllocator::analyse_cfg_flow() {
    auto blocks = fun->get_bbs();
    std::unordered_map<ptr<ir::ir_basicblock>, ptr_list<ir::ir_basicblock>> predecessor;
    std::unordered_map<ptr<ir::ir_basicblock>, ptr_list<ir::ir_basicblock>> nxt;                    // cfg中的逆映射
    for(auto block : blocks) {
        for(auto instruction : block->get_instructions()) {
            auto jump_ins = std::dynamic_pointer_cast<ir::jump>(instruction);
            auto br_ins = std::dynamic_pointer_cast<ir::br>(instruction);
            auto bc_ins = std::dynamic_pointer_cast<ir::break_or_continue>(instruction);
            auto while_ins = std::dynamic_pointer_cast<ir::while_loop>(instruction);
            auto store_ins = std::dynamic_pointer_cast<ir::store>(instruction);
            if(jump_ins) {
                auto tar = jump_ins->get_target();
                predecessor[tar].push_back(block);
                nxt[block].push_back(tar);
            }
            if(bc_ins) {
                auto tar = bc_ins->get_target();
                predecessor[tar].push_back(block);
                nxt[block].push_back(tar);
            }
            if(br_ins) {
                auto tar = br_ins->get_target_true();
                predecessor[tar].push_back(block);
                nxt[block].push_back(tar);
                tar = br_ins->get_target_false();
                predecessor[tar].push_back(block);
                nxt[block].push_back(tar);
            }
            if(while_ins) {
                auto cond_from = while_ins->get_cond_from();
                auto tar = while_ins->get_out_block();
                predecessor[tar].push_back(cond_from);
                nxt[cond_from].push_back(tar);
                // s_back_trace[cond_from].push_back(block);
                predecessor[cond_from].push_back(block);
                // n_back_trace[block].push_back(cond_from);
                nxt[block].push_back(cond_from);
            }
        }
    }
    fun->set_predecessor(predecessor);
    fun->set_nxt(nxt);
    fun->mark_analysed();
}