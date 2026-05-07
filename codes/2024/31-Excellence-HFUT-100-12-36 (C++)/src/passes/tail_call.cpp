#include "tail_call.hpp"
#include "../ir/ir.hpp"
#include "../parser/SyntaxTree.hpp"
#include "pass_type.hpp"
#include <algorithm>
#include <cassert>
#include <memory>

void Passes::TailCall::run() {
    for(auto [name, fun] : compunit->usrfuncs) {
        this->work_lst.clear();
        this->dealing_fun = fun;
        get_work_lst();
        if(!work_lst.empty()) fun->clear_analysed();
        create_tail_call();
    }
    compunit->mark_passes_completed(TAIL_CALL);
}

void Passes::TailCall::create_tail_call() {
    for(auto [reg, block] : this->work_lst) {
        auto block_from = block.lock();
        assert(block_from);
        auto tail_call_block = this->dealing_fun->new_block();
        tail_call_block->mark_ret();
        auto jump = std::dynamic_pointer_cast<ir::jump>(block_from->get_instructions().back());
        assert(jump);
        jump->replace_target(tail_call_block);
        auto ins = reg->get_def_loc();
        auto func_call = std::dynamic_pointer_cast<ir::func_call>(ins);
        assert(func_call);
        auto def_block = ins->get_block();
        auto del_it = def_block->search(ins);
        assert(del_it != def_block->get_ins_end());
        def_block->erase(del_it);

        auto tail_ins = std::make_shared<ir::tail_call>(func_call);
        tail_call_block->push_back(tail_ins);
    }
}

void Passes::TailCall::get_work_lst() {
    std::vector<std::pair<ptr<ir::ir_value>, std::weak_ptr<ir::ir_basicblock>>> del_lst;
    ptr_list<ir::ir_basicblock> del_blocks;
    for(auto block : dealing_fun->get_bbs()) {
        if(block->is_ret()) {
            for(auto tar : block->get_tail_call()) {
                auto ret_reg = std::dynamic_pointer_cast<ir::ir_reg>(tar.first);
                assert(ret_reg);
                auto call_ins = std::dynamic_pointer_cast<ir::func_call>(ret_reg->get_def_loc());
                assert(call_ins);
                bool skip_this_call = false;
                for(auto par : call_ins->get_params()) {
                    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(par);
                    if(is_reg) {
                        if(is_reg->check_is_arr()) {
                            auto source_reg = is_reg;
                            auto is_get_element = std::dynamic_pointer_cast<ir::get_element_ptr>(source_reg->get_def_loc());
                            while(is_get_element) {
                                source_reg = is_get_element->get_base();
                                is_get_element = std::dynamic_pointer_cast<ir::get_element_ptr>(source_reg->get_def_loc());
                            }
                            if(!source_reg->check_global() && !source_reg->check_is_param()) {
                                skip_this_call = true;
                                break;
                            }
                        }
                    }
                }
                if(skip_this_call) continue;
                this->work_lst.push_back(tar);
            }
            del_lst = work_lst;
            for(auto ins : block->get_instructions()) {
                auto is_phi = std::dynamic_pointer_cast<ir::phi>(ins);
                if(is_phi) {
                    while(!del_lst.empty()) {
                        auto other = del_lst.back();
                        auto it = std::find_if(is_phi->uses.begin(), is_phi->uses.end(), [other] (std::pair<ptr<ir::ir_value>, std::weak_ptr<ir::ir_basicblock>> &target){
                            if(target.first == other.first) {
                                auto tar_lck = target.second.lock();
                                auto oth_lck = other.second.lock();
                                return tar_lck == oth_lck;
                            }
                            return false;
                        });
                        if(it != is_phi->uses.end()) {
                            is_phi->uses.erase(it);
                        }
                        del_lst.pop_back();
                    }
                    if(is_phi->uses.empty()) {
                        del_blocks.push_back(block);
                    }
                }
            }
        }
    }
    while(!del_blocks.empty()) {
        dealing_fun->del_ret_block(del_blocks.back());
        del_blocks.pop_back();
    }
}
