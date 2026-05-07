#include "middleend/remove_useless_call.hpp"

namespace middleend {

void RemoveUselessCall::run() {
    for (auto func_ : *module_->get_functions()) {
        CFG cfg(func_);
        std::unordered_map<Temp *, Temp *> reg2base;
        for (auto param : *func_->get_arg_temp()) {
            if (param->get_type().is_array()) {
                reg2base[param] = param;
            }
        }
        for (auto bb : *func_->get_basic_blocks()) {
            for (auto inst : *bb->get_instructions()) {
                TypeCase(alloc, ir::instruction::Alloca *, inst) {
                    reg2base[alloc->getaddr()] = alloc->getaddr();
                } else TypeCase(loadaddr, ir::instruction::LoadAddr *, inst) {
                    reg2base[loadaddr->get_addr()] = loadaddr->get_addr();
                }
            }
        }
        for (auto bb : *func_->get_basic_blocks()) {
            for (auto inst : *bb->get_instructions()) {
                TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
                    assert(reg2base.find(elementptr->get_base()) != reg2base.end());
                    reg2base[elementptr->get_dst()] = reg2base[elementptr->get_base()];
                }
            }
        }
        for (auto body_bb : *func_->get_basic_blocks()) {
            if (cfg.get_bb_prev(body_bb->get_index())->size() != 2) {
                continue;
            }
            auto body_term = body_bb->get_instructions()->back();
            TypeCase(condbranch, ir::instruction::CondBranch *, body_term) {
                if (condbranch->get_true_bb() != body_bb) {
                    continue;
                }
            }
            ir::BasicBlock *iprev_bb = nullptr;
            for (auto prev_idx : *cfg.get_bb_prev(body_bb->get_index())) {
                auto prev_bb = cfg.get_bb_map()->at(prev_idx);
                if (prev_bb == body_bb) {
                    continue;
                } else {
                    iprev_bb = prev_bb;
                }
            }
            assert(iprev_bb != nullptr);
            std::unordered_set<ir::instruction::Call *> body_call;
            for (auto iter = body_bb->get_instructions()->begin(); iter != body_bb->get_instructions()->end(); iter++) {
                auto inst = *iter;
                TypeCase(call, ir::instruction::Call *, inst) {
                    if (module_->get_func_map().find(call->get_func_name()) == module_->get_func_map().end()) {
                        continue;
                    }
                    if (module_->get_func_map()[call->get_func_name()]->is_pure()) {
                        body_call.insert(call);
                    }
                }
            }
            for (auto call : body_call) {
                auto prev_bb = iprev_bb;
                for (auto prev_iter = prev_bb->get_instructions()->begin(); prev_iter != prev_bb->get_instructions()->end(); prev_iter++) {
                    auto prev_inst = *prev_iter;
                    TypeCase(prev_call, ir::instruction::Call *, prev_inst) {
                        if (prev_call->get_name() == call->get_name()) {
                            if (prev_call->get_srcs().size() == call->get_srcs().size()) {
                                bool can_remove = true;
                                for (auto i = 0; i < prev_call->get_srcs().size(); i++) {
                                    if (prev_call->get_srcs()[i] != call->get_srcs()[i]) {
                                        can_remove = false;
                                        break;
                                    }
                                    auto param = prev_call->get_srcs()[i];
                                    if (param->get_type().is_array()) {
                                        for (auto prev_later_iter = prev_iter + 1; prev_later_iter != prev_bb->get_instructions()->end(); prev_later_iter++) {
                                            TypeCase(store, ir::instruction::Store *, *prev_later_iter) {
                                                if (reg2base[store->getaddr()] == reg2base[param]) {
                                                    can_remove = false;
                                                    break;
                                                }
                                            } else TypeCase(call_change, ir::instruction::Call *, *prev_later_iter) {
                                                for (auto para : call_change->get_srcs()) {
                                                    if (para->get_type().is_array()) {
                                                        if (reg2base[para] == reg2base[param]) {
                                                            if (module_->get_func_map().find(call_change->get_func_name()) != module_->get_func_map().end() && module_->get_func_map()[call_change->get_func_name()]->is_pure()) {

                                                            } else {
                                                                can_remove = false;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        for (auto body_inst : *body_bb->get_instructions()) {
                                            TypeCase(store, ir::instruction::Store *, body_inst) {
                                                if (reg2base[store->getaddr()] == reg2base[param]) {
                                                    can_remove = false;
                                                    break;
                                                }
                                            } else TypeCase(call_change, ir::instruction::Call *, body_inst) {
                                                for (auto para : call_change->get_srcs()) {
                                                    if (para->get_type().is_array()) {
                                                        if (reg2base[para] == reg2base[param]) {
                                                            if (module_->get_func_map().find(call_change->get_func_name()) != module_->get_func_map().end() && module_->get_func_map()[call_change->get_func_name()]->is_pure()) {

                                                            } else {
                                                                can_remove = false;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                if (can_remove) {
                                    auto call_iter = std::find(body_bb->get_instructions()->begin(), body_bb->get_instructions()->end(), call);
                                    *call_iter = new ir::instruction::Assign(call->getdst(), prev_call->getdst());
                                    delete call;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

} // namespace middleend