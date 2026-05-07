#include "middleend/arrayssa.hpp"

namespace middleend {

ArraySSA::ArraySSA(ir::Module *module) : module_(module) {
    // for (auto func : *module->get_functions()) { // 将load指令尽可能移动到store指令之前
    //     for (auto bb : *func->get_basic_blocks()) {
    //         std::unordered_map<Temp *, ir::instruction::Store *> addr2store;
    //         std::unordered_map<ir::instruction::Store *, std::unordered_set<ir::instruction::Load *>> storeload;
    //         for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end();) {
    //             auto inst = *iter;
    //             TypeCase(store, ir::instruction::Store *, inst) {
    //                 if (store->getoffset() == 0) {
    //                     addr2store[store->getaddr()] = store;
    //                 }
    //             } else TypeCase(load, ir::instruction::Load *, inst) {
    //                 if (load->getoffset() == 0) {
    //                     if (addr2store.find(load->getaddr()) != addr2store.end()) {
    //                         storeload[addr2store[load->getaddr()]].insert(load);
    //                     } else {
    //                         storeload[nullptr].insert(load);
    //                     }
    //                     iter = bb->get_instructions()->erase(iter);
    //                     continue;
    //                 }
    //             }
    //             iter ++;
    //         }
    //         for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
    //             auto inst = *iter;
    //             TypeCase(phi, ir::instruction::Phi *, inst) {
    //                 continue;
    //             } else {
    //                 for (auto load : storeload[nullptr]) {
    //                     iter = bb->get_instructions()->insert(iter, load);
    //                     iter ++;
    //                 }
    //                 break;
    //             }
    //         }
    //         for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
    //             auto inst = *iter;
    //             TypeCase(store, ir::instruction::Store *, inst) {
    //                 if (storeload.find(store) != storeload.end()) {
    //                     for (auto load : storeload[store]) {
    //                         iter ++;
    //                         iter = bb->get_instructions()->insert(iter, load);
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }
}

void ArraySSA::find_func_use(ir::Function *func) {
    std::unordered_map<Temp *, Temp *> param_base;
    for (int i = 0; i < func->get_arg_temp()->size(); i++) {
        auto param = (*func->get_arg_temp())[i];
        if (param->get_type().is_array()) {
            param_base[param] = param;
        }
    }

    func_global_use[func] = std::unordered_set<std::string>();
    func_param_use[func] = std::unordered_set<Temp *>();
    for (auto &bb : *func->get_basic_blocks()) {
        for (auto &inst : *bb->get_instructions()) {
            TypeCase(store, ir::instruction::Store *, inst) {
                if (param_base.find(store->getaddr()) != param_base.end()) {
                    func_param_use[func].insert(param_base[store->getaddr()]);
                }
            } else TypeCase(load, ir::instruction::Load *, inst) {
                
            } else TypeCase(loadaddr, ir::instruction::LoadAddr *, inst) {
                func_global_use[func].insert(loadaddr->get_name());
            } else TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
                if (param_base.find(elementptr->get_base()) != param_base.end()) {
                    param_base[elementptr->get_dst()] = param_base[elementptr->get_base()];
                }
            } else TypeCase(call, ir::instruction::Call *, inst) {
                if (module_->func_map.find(call->get_func_name()) != module_->func_map.end()) {
                    if (func_global_use.find(module_->func_map[call->get_func_name()]) == func_global_use.end()) {
                        find_func_use(module_->func_map[call->get_func_name()]);
                    }
                    func_global_use[func].insert(func_global_use[module_->func_map[call->get_func_name()]].begin(), func_global_use[module_->func_map[call->get_func_name()]].end());
                    func_param_use[func].insert(func_param_use[module_->func_map[call->get_func_name()]].begin(), func_param_use[module_->func_map[call->get_func_name()]].end());
                }
            }
        }
    }
}

void ArraySSA::build_analysis(DominatorTree_ *dom_tree, int &next_temp_id_) {
    UseDefAnalysis uda(dom_tree->get_cfg());
    alloc_bb.clear();
    alloc_map.clear();
    def_bbs.clear();
    reg2base.clear();
    name2base.clear();
    phi2base.clear();
    auto func = dom_tree->get_func();
    auto entry = func->get_entry();
    for (auto global : *module_->get_global_variables()) {
        auto global_type = global->get_data_type();
        auto new_temp = new Temp(next_temp_id_ ++, global_type.base_type);
        if (global_type.is_array()) {
            new_temp->set_type(global_type.array2pointer());
        } else {
            new_temp->set_type(global_type.get_pointer_type());
        }
        entry->add_instruction_front(new ir::instruction::LoadAddr(new_temp, global->get_name(), global->get_data_type()));
    }

    for (auto param : *dom_tree->get_func()->get_arg_temp()) {
        if (param->get_type().is_array()) {
            alloc_bb[param] = entry;
            alloc_map[entry][param] = param;
            reg2base[param] = param;
        }
    }
    auto preo = dom_tree->get_preo(entry);
    for (auto &bb : preo) {
        for (auto &inst : *bb->get_instructions()) {
            TypeCase(alloc, ir::instruction::Alloca *, inst) {
                alloc_bb[alloc->getaddr()] = bb;
                def_bbs[alloc->getaddr()].insert(bb);
                reg2base[alloc->getaddr()] = alloc->getaddr();
            } else TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
                reg2base[elementptr->get_dst()] = reg2base[elementptr->get_base()];
            } else TypeCase(store, ir::instruction::Store *, inst) {
                if (reg2base.find(store->getaddr()) != reg2base.end()) {
                    def_bbs[reg2base[store->getaddr()]].insert(bb);
                }
            } else TypeCase(load, ir::instruction::Load *, inst) {
                
            } else TypeCase(loadaddr, ir::instruction::LoadAddr *, inst) {
                reg2base[loadaddr->get_addr()] = loadaddr->get_addr();
                if (name2base.find(loadaddr->get_name()) == name2base.end()) {
                    alloc_bb[loadaddr->get_addr()] = bb;
                    def_bbs[loadaddr->get_addr()].insert(bb);
                    name2base[loadaddr->get_name()] = loadaddr->get_addr();
                } else {
                    reg2base[loadaddr->get_addr()] = name2base[loadaddr->get_name()];
                    def_bbs[loadaddr->get_addr()] = def_bbs[name2base[loadaddr->get_name()]];
                    alloc_bb[loadaddr->get_addr()] = alloc_bb[name2base[loadaddr->get_name()]];
                    inst = new ir::instruction::Assign(loadaddr->get_addr(), name2base[loadaddr->get_name()]);
                    delete loadaddr;
                }
            } else TypeCase(call, ir::instruction::Call *, inst) {
                for (auto &param : call->get_srcs()) {
                    if (reg2base.find(param) != reg2base.end()) {
                        def_bbs[reg2base[param]].insert(bb);
                    }
                }
                ir::Function *dst_func = nullptr;
                if (module_->func_map.find(call->get_func_name()) != module_->func_map.end()) {
                    dst_func = module_->func_map[call->get_func_name()];
                }
                for (auto &global : func_global_use[dst_func]) {
                    assert(name2base.find(global) != name2base.end());
                    def_bbs[name2base[global]].insert(bb);
                }
            } else TypeCase(assign, ir::instruction::Assign *, inst) {
                if (reg2base.find(assign->getsrc()) != reg2base.end()) {
                    reg2base[assign->getdst()] = reg2base[assign->getsrc()];
                    def_bbs[assign->getdst()] = def_bbs[assign->getsrc()];
                    alloc_bb[assign->getdst()] = alloc_bb[assign->getsrc()];
                }
            }
        }
    }
}

void ArraySSA::insert_phi(DominatorTree_ *dom_tree, int &next_temp_id_) {
    for (auto item : alloc_bb) {
        auto temp = item.first;
        auto bb = item.second;
        auto W = def_bbs[temp];
        std::unordered_set<ir::BasicBlock *> visited;
        while (!W.empty()) {
            auto X = *W.begin();
            W.erase(W.begin());
            for (auto Y : dom_tree->get_dom_frontier(X)) {
                if (visited.find(Y) == visited.end()) {
                    visited.insert(Y);
                    auto new_temp = new Temp(next_temp_id_ ++, temp->get_type());
                    auto phi = new ir::instruction::Phi(new_temp, std::vector<Temp *>(), std::vector<ir::BasicBlock *>(), false, true);
                    Y->add_instruction_front(phi);
                    phi2base[phi] = reg2base[temp];
                    if (def_bbs[temp].find(Y) == def_bbs[temp].end()) {
                        W.insert(Y);
                    }
                }
            }
        }
    }
}

void ArraySSA::rename(DominatorTree_ *dom_tree, int &next_temp_id_) {
    std::unordered_map<ir::BasicBlock *, std::unordered_map<Temp *, std::unordered_set<Temp *>>> load_before_store;
    auto preo = dom_tree->get_preo(dom_tree->get_func()->get_entry());
    for (auto &bb : preo) {
        load_before_store[bb] = load_before_store[dom_tree->get_idom(bb)];
        for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); ) {
            auto &inst = *iter;
            TypeCase(phi, ir::instruction::Phi *, inst) {
                if (phi2base.find(phi) != phi2base.end()) { // 只对新生成的phi处理
                    alloc_map[bb][phi2base[phi]] = phi->getdst();
                    phi->set_used(load_before_store[bb][phi2base[phi]]);
                    load_before_store[bb][phi2base[phi]].clear();
                }
            } else TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
                
            } else TypeCase(store, ir::instruction::Store *, inst) {
                assert(reg2base.find(store->getaddr()) != reg2base.end());
                auto base = reg2base[store->getaddr()];
                auto base_def = bb;
                while (alloc_map[base_def].find(base) == alloc_map[base_def].end()) {
                    base_def = dom_tree->get_idom(base_def);
                    assert(base_def != nullptr);
                }
                auto dep = alloc_map[base_def][base];
                auto new_temp = new Temp(next_temp_id_ ++, store->getsrc()->get_type().base_type);
                inst = new ir::instruction::ArrayStore(new_temp, dep, store->getaddr(), store->getsrc(), store->getoffset(), load_before_store[bb][base]);
                inst->set_parent(bb);
                alloc_map[bb][base] = new_temp;
                load_before_store[bb][base].clear();
                delete store;
            } else TypeCase(load, ir::instruction::Load *, inst) {
                assert(reg2base.find(load->getaddr()) != reg2base.end());
                auto base = reg2base[load->getaddr()];
                auto base_def = bb;
                while (alloc_map[base_def].find(base) == alloc_map[base_def].end()) {
                    base_def = dom_tree->get_idom(base_def);
                    assert(base_def != nullptr);
                }
                auto dep = alloc_map[base_def][base];
                inst = new ir::instruction::ArrayLoad(load->getdst(), dep, load->getaddr(), load->getoffset());
                inst->set_parent(bb);
                load_before_store[bb][base].insert(load->getdst());
                delete load;
            } else TypeCase(loadaddr, ir::instruction::LoadAddr *, inst) {
                alloc_map[bb][loadaddr->get_addr()] = loadaddr->get_addr();
            } else TypeCase(call, ir::instruction::Call *, inst) {
                std::unordered_map<Temp *, Temp *> param_load;
                std::unordered_map<std::string, Temp *> global_load;
                ir::Function *dst_func = nullptr;
                if (module_->func_map.find(call->get_func_name()) != module_->func_map.end()) {
                    dst_func = module_->func_map[call->get_func_name()];
                }
                for (auto &param : call->get_srcs()) {
                    if (reg2base.find(param) != reg2base.end()) {
                        auto base = reg2base[param];
                        auto base_def = bb;
                        while (alloc_map[base_def].find(base) == alloc_map[base_def].end()) {
                            base_def = dom_tree->get_idom(base_def);
                            assert(base_def != nullptr);
                        }
                        auto dep = alloc_map[base_def][base];
                        auto new_temp = new Temp(next_temp_id_ ++, param->get_type().base_type);
                        auto new_inst = new ir::instruction::ArrayLoad(new_temp, dep, new_temp, 0, true); // 这里不知道调用内部的offset
                        iter = bb->get_instructions()->insert(iter, new_inst);
                        iter ++;
                        new_inst->set_parent(bb);
                        load_before_store[bb][base].insert(new_temp);
                        param_load[param] = new_temp;
                    }
                }
                for (auto &global : func_global_use[dst_func]) {
                    if (name2base.find(global) != name2base.end()) {
                        auto base = name2base[global];
                        auto base_def = bb;
                        while (alloc_map[base_def].find(base) == alloc_map[base_def].end()) {
                            base_def = dom_tree->get_idom(base_def);
                            assert(base_def != nullptr);
                        }
                        auto dep = alloc_map[base_def][base];
                        auto new_temp = new Temp(next_temp_id_ ++, base->get_type().base_type);
                        auto new_inst = new ir::instruction::ArrayLoad(new_temp, dep, new_temp, 0, true); // 这里不知道调用内部的offset
                        iter = bb->get_instructions()->insert(iter, new_inst);
                        iter ++;
                        new_inst->set_parent(bb);
                        load_before_store[bb][base].insert(new_temp);
                        global_load[global] = new_temp;
                    }
                }
                for (auto item : global_load) {
                    call->add_global_load(item.second);
                }
                for (auto &global : func_global_use[dst_func]) {
                    if (name2base.find(global) != name2base.end()) {
                        auto base = name2base[global];
                        auto base_def = bb;
                        while (alloc_map[base_def].find(base) == alloc_map[base_def].end()) {
                            base_def = dom_tree->get_idom(base_def);
                            assert(base_def != nullptr);
                        }
                        auto dep = alloc_map[base_def][base];
                        auto new_temp = new Temp(next_temp_id_ ++, base->get_type().base_type);
                        auto new_inst = new ir::instruction::ArrayStore(new_temp, dep, global_load[global], new_temp, 0, load_before_store[bb][base], true);
                        iter = bb->get_instructions()->insert(iter + 1, new_inst);
                        new_inst->set_parent(bb);
                        load_before_store[bb][base].clear();
                        alloc_map[bb][base] = new_temp;
                    }
                }
                for (int i = 0; i < call->get_srcs().size(); i++) {
                    auto param = call->get_srcs()[i];
                    if (reg2base.find(param) != reg2base.end()) {
                        // if (call->get_func_name() != "__memset_zero__" && module_->get_func_map().find(call->get_func_name()) != module_->get_func_map().end()) {
                        //     auto dst_func = module_->func_map[call->get_func_name()];
                        //     if (func_param_use[dst_func].find(dst_func->get_arg_temp()->at(i)) == func_param_use[dst_func].end()) {
                        //         continue;
                        //     }
                        // }
                        auto base = reg2base[param];
                        auto base_def = bb;
                        while (alloc_map[base_def].find(base) == alloc_map[base_def].end()) {
                            base_def = dom_tree->get_idom(base_def);
                            assert(base_def != nullptr);
                        }
                        auto dep = alloc_map[base_def][base];
                        auto new_temp = new Temp(next_temp_id_ ++, param->get_type().base_type);
                        auto new_inst = new ir::instruction::ArrayStore(new_temp, dep, param_load[param], new_temp, 0, load_before_store[bb][base], true);
                        iter = bb->get_instructions()->insert(iter + 1, new_inst);
                        new_inst->set_parent(bb);
                        load_before_store[bb][base].clear();
                        alloc_map[bb][base] = new_temp;
                    }
                }
            } else TypeCase(alloc, ir::instruction::Alloca *, inst) {
                alloc_map[bb][alloc->getaddr()] = alloc->getaddr();
            }
            iter ++;
        }
        for (auto &succ_idx : *dom_tree->get_cfg()->get_bb_succ(bb->get_index())) {
            auto succ = dom_tree->get_cfg()->get_bb(succ_idx);
            for (auto &inst : *succ->get_instructions()) {
                TypeCase(phi, ir::instruction::Phi *, inst) {
                    if (phi2base.find(phi) != phi2base.end()) {
                        auto base_def = bb;
                        auto temp = phi2base[phi];
                        while (base_def != nullptr && alloc_map[base_def].find(temp) == alloc_map[base_def].end()) {
                            base_def = dom_tree->get_idom(base_def);
                            // assert(base_def != nullptr);
                        }
                        if (base_def != nullptr) {
                            phi->add_src_and_bb(bb, alloc_map[base_def][temp]);
                        }
                    }
                } else {
                    break;
                }
            }
        }
    }
}

void ArraySSA::remove_useless_phi(DominatorTree_ *dom_tree) {
    UseDefAnalysis uda(dom_tree->get_cfg());
    for (auto &bb : *dom_tree->get_func()->get_basic_blocks()) {
        for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); ) {
            auto inst = *iter;
            TypeCase(phi, ir::instruction::Phi *, inst) {
                if (uda.get_useset(phi->getdst()).empty()) {
                    iter = bb->get_instructions()->erase(iter);
                } else {
                    ++iter;
                }
            } else {
                break;
            }
        }
    }

}

void ArraySSA::array2reg() {
    func_global_use.clear();
    func_param_use.clear();
    find_func_use(module_->func_map["main"]);
    for (auto &func : *module_->get_functions()) {
        CFG cfg(func);
        int next_temp_id_ = func->get_temp_used();
        DominatorTree_ dom_tree(&cfg);
        build_analysis(&dom_tree, next_temp_id_);
        insert_phi(&dom_tree, next_temp_id_);
        rename(&dom_tree, next_temp_id_);
        remove_useless_phi(&dom_tree);
        func->set_temp_used(next_temp_id_);
    }
}

void ArraySSA::reg2array() {
    for (auto &func : *module_->get_functions()) {
        for (auto &bb : *func->get_basic_blocks()) {
            for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); ) {
                auto& inst = *iter;
                TypeCase(arrayload, ir::instruction::ArrayLoad *, inst) {
                    if (arrayload->before_call()){
                        iter = bb->get_instructions()->erase(iter);
                    } else {
                        inst = new ir::instruction::Load(arrayload->getdst(), arrayload->getaddr(), false, arrayload->getoffset());
                        inst->set_parent(bb);
                    }
                    delete arrayload;
                } else TypeCase(arraystore, ir::instruction::ArrayStore *, inst) {
                    if (arraystore->after_call()) {
                        iter = bb->get_instructions()->erase(iter);
                    } else {
                        inst = new ir::instruction::Store(arraystore->getaddr(), arraystore->getsrc(), false, arraystore->getoffset());
                        inst->set_parent(bb);
                    }
                    delete arraystore;
                } else TypeCase(phi, ir::instruction::Phi *, inst) {
                    if (phi2base.find(phi) != phi2base.end()) {
                        iter = bb->get_instructions()->erase(iter);
                    } else {
                        ++iter;
                    }
                } else TypeCase(call, ir::instruction::Call *, inst) {
                    call->clear_global_load();
                    ++ iter;
                } else {
                    ++iter;
                }
            }
        }
    }
}

} // namespace middleend