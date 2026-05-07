#include "function_info.hpp"
#include "../ir/ir.hpp"
#include "pass_type.hpp"
#include <cassert>
#include <memory>

void Passes::FunctionInfo::run() {
    for(auto usr : compunit->usrfuncs) {
        this->func_lst.push_back(usr);
    }
    for(auto lib : compunit->libfuncs) {
        this->func_lst.push_back(lib);
    }
    for(auto [name, fun] : func_lst) {
        auto is_usr_fun = std::dynamic_pointer_cast<ir::ir_userfunc>(fun);
        if(is_usr_fun)
            judge_pure(name, is_usr_fun);
        else {
            fun->clear_pure();      // 库函数直接认为非纯
        }
        if(!fun->check_pure()) {
            work_lst.push(fun);
        }
    }
    while(!work_lst.empty()) {
        auto cur_fun = work_lst.front();
        work_lst.pop();
        judge_caller(cur_fun);
    }
    compunit->mark_passes_completed(FUNCTION_INFO);
}

void Passes::FunctionInfo::judge_caller(ptr<ir::ir_func> fun) {
    auto caller = fun->get_caller();
    for(auto tar : caller) {
        auto lock = tar.lock();
        assert(lock);
        if(lock->check_pure()) {
            lock->clear_pure();
            work_lst.push(lock);
        }
    }
}

void Passes::FunctionInfo::judge_pure(string name, ptr<ir::ir_userfunc> fun) {
    if(name == "main") {
        return;
    }
    auto param = fun->get_params();
    for(auto arg : param) {
        if(arg->is_arr()) {
            return;
        }
    }
    for(auto block : fun->get_bbs()) {
        for(auto ins : block->get_instructions()) {
            auto is_store = std::dynamic_pointer_cast<ir::store>(ins);
            auto is_load = std::dynamic_pointer_cast<ir::load>(ins);
            auto is_call = std::dynamic_pointer_cast<ir::func_call>(ins);
            if(is_store) {
                if(!is_store->get_addr()->check_local()) {
                    return;
                }
            }
            if(is_load) {
                if(!is_load->get_addr()->check_local()) {
                    return;
                }
            }
        }
    }
    fun->mark_pure();
}