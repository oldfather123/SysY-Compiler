#include "dead_code_elimination.hpp"
#include "../ir/ir.hpp"
#include "function_info.hpp"
#include "pass_type.hpp"
#include <memory>
#include <unordered_set>

void Passes::DeadCodeElimination::run() {
    if(!compunit->check_passes_completed(FUNCTION_INFO)) {
        auto func_info_pass = std::make_shared<Passes::FunctionInfo>(compunit);
        func_info_pass->run();
    }
    bool changed = true;
    while(changed) {
        changed = false;
        for(auto [_name, fun] : compunit->usrfuncs) {
            mark_by_fun(fun);
            changed |= sweep(fun);
        }
    }
}

bool Passes::DeadCodeElimination::sweep(ptr<ir::ir_userfunc> fun) {
    std::unordered_set<ptr<ir::ir_instr>> del_lst;
    for(auto block : fun->get_bbs()) {
        for(auto ins : block->get_instructions()) {
            if(!marked[ins]) {
                del_lst.insert(ins);
            }
        }
    }
    for(auto ins : del_lst) {
        ins->get_block()->del_ins(ins);
    }
    return !del_lst.empty();
}

void Passes::DeadCodeElimination::mark_by_ins(ptr<ir::ir_instr> ins) {
    auto use_val = ins->use_reg();
    for(auto use : use_val) {
        auto def = use->get_def_loc();
        if(def) {
            if(marked[def]) {
                continue;
            }
            if(def->get_fun() != ins->get_fun())
                continue;
            marked[def] = true;
            work_lst.push_back(def);
        }
    }
}

void Passes::DeadCodeElimination::mark_by_fun(ptr<ir::ir_userfunc> fun) {
    work_lst.clear();
    marked.clear();
    for(auto block : fun->get_bbs()) {
        for(auto ins : block->get_instructions()) {
            if(check_critical(ins)) {
                marked[ins] = true;
                work_lst.push_back(ins);
            }
        }
    }
    while(!work_lst.empty()) {
        auto cur = work_lst.front();
        work_lst.pop_front();
        mark_by_ins(cur);
    }
}

bool Passes::DeadCodeElimination::check_critical(ptr<ir::ir_instr> tar) {
    auto is_memset = std::dynamic_pointer_cast<ir::memset>(tar);
    auto is_call = std::dynamic_pointer_cast<ir::func_call>(tar);
    auto is_br = std::dynamic_pointer_cast<ir::br>(tar);
    auto is_jmp = std::dynamic_pointer_cast<ir::jump>(tar);
    auto is_bc = std::dynamic_pointer_cast<ir::break_or_continue>(tar);
    auto is_loop_back = std::dynamic_pointer_cast<ir::while_loop>(tar);
    auto is_ret = std::dynamic_pointer_cast<ir::ret>(tar);
    auto is_store = std::dynamic_pointer_cast<ir::store>(tar);
    auto is_tail_call = std::dynamic_pointer_cast<ir::tail_call>(tar);
    if(is_memset) {
        return true;
    }
    if(is_call) {
        auto callee = is_call->get_callee();
        return !callee->check_pure();
    }
    if(is_br || is_jmp || is_bc || is_loop_back || is_ret) {
        return true;
    }
    if(is_store) {
        return true;
    }
    if(is_tail_call) {
        return true;
    }
    return false;
}