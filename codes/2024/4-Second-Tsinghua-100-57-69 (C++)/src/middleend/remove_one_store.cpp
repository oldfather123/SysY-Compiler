#include "middleend/remove_one_store.hpp"

namespace middleend {

void RemoveOneStore::run() {
    for (auto &bb : *module_->get_func_map()["main"]->get_basic_blocks()) {
        for (auto inst : *bb->get_instructions()) {
            TypeCase(call, ir::instruction::Call *, inst) {
                if (call->get_func_name() == "getint"){
                    continue;
                }
                if (call->get_func_name() == "putint"){
                    continue;
                }
                if (call->get_func_name() == "putch"){
                    continue;
                }
                if (call->get_func_name() == "getch"){
                    continue;
                }
                if (call->get_func_name() == "starttime"){
                    continue;
                }
                if (call->get_func_name() == "stoptime"){
                    continue;
                }
                if (call->get_func_name() == "__memset_zero__"){
                    return ;
                }
                if (call->get_func_name() == "getarray"){
                    return ;
                }
                if (call->get_func_name() == "putarray"){
                    return ;
                }
                if (call->get_func_name() == "putfarray"){
                    return ;
                }
                if (call->get_func_name() == "getfarray"){
                    return ;
                }
                if (module_->get_func_map().find(call->get_func_name()) == module_->get_func_map().end()){
                    return ;
                }
                if (module_->get_func_map()[call->get_func_name()]->is_pure()){
                    continue;
                }
                return ;
            }
        }
    }
    for (auto &bb : *module_->get_func_map()["main"]->get_basic_blocks()) {
        if (bb->get_index() == 0){
            continue;
        }
        for (auto inst : *bb->get_instructions()) {
            TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
                return;
            }
        }
    }
    std::unordered_map<Temp *, std::unordered_set<ir::instruction::Store *>> store_map;
    std::unordered_set<ir::instruction::Store *> first_bb_store;
    std::unordered_set<Temp *> first_bb_addr;
    for (auto &bb : *module_->get_func_map()["main"]->get_basic_blocks()) {
        assert(bb->get_index() == 0);
        for (auto inst : *bb->get_instructions()) {
            TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
                first_bb_addr.insert(elementptr->get_dst());
            }
        }
        break;
    }
    for (auto &bb : *module_->get_func_map()["main"]->get_basic_blocks()) {
        for (auto inst : *bb->get_instructions()) {
            TypeCase(store, ir::instruction::Store *, inst) {
                if (!first_bb_addr.count(store->getaddr())){
                    continue;
                }
                store_map[store->getaddr()].insert(store);
                if (bb->get_index() == 0){
                    first_bb_store.insert(store);
                }
            }
        }
    }
    std::unordered_map<Temp *, Temp *> addr2src;
    std::unordered_set<ir::instruction::Store *> remove_store;
    for (auto item : store_map){
        if (item.second.size() == 1) {
            auto store = *item.second.begin();
            if (first_bb_store.count(store)){
                addr2src[store->getaddr()] = store->getsrc();
                remove_store.insert(store);
            }
        }
    }
    for (auto &bb : *module_->get_func_map()["main"]->get_basic_blocks()) {
        for (auto &inst : *bb->get_instructions()) {
            TypeCase(load, ir::instruction::Load *, inst) {
                if (addr2src.count(load->getaddr())){
                    inst = new ir::instruction::Assign(load->getdst(), addr2src[load->getaddr()]);
                    delete load;
                }
            }
        }
    }
    // for (auto &bb : *module_->get_func_map()["main"]->get_basic_blocks()) {
    //     for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end();) {
    //         auto inst = *iter;
    //         TypeCase(store, ir::instruction::Store *, inst) {
    //             if (remove_store.count(store)) {
    //                 iter = bb->get_instructions()->erase(iter);
    //             } else {
    //                 iter++;
    //             }
    //         } else {
    //             iter++;
    //         }
    //     }
    // }
    for (auto &bb : *module_->get_func_map()["main"]->get_basic_blocks()) {
        for (auto &inst : *bb->get_instructions()) {
            TypeCase(load, ir::instruction::Load *, inst) {
                return;
            }
        }
    }
    for (auto &bb : *module_->get_func_map()["main"]->get_basic_blocks()) {
        for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end();) {
            auto inst = *iter;
            TypeCase(store, ir::instruction::Store *, inst) {
                iter = bb->get_instructions()->erase(iter);
            } else {
                iter++;
            }
        }
    }
}

} // namespace middleend