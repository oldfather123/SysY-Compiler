#include "middleend/remove_useless_memory.hpp"

namespace middleend {

bool RemoveUselessMemory::run() {
    std::unordered_set<ir::instruction::ArrayStore *> useless_store;
    std::unordered_set<ir::instruction::ArrayLoad *> useless_load;
    for (auto &bb : *func_->get_basic_blocks()) {
        for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
            auto inst = *iter;
            TypeCase(arrayload, ir::instruction::ArrayLoad *, inst) {
                if (!uda_->get_defset(arrayload->getdep()).empty()) {
                    auto def_inst = *uda_->get_defset(arrayload->getdep()).begin();
                    TypeCase(arraystore, ir::instruction::ArrayStore *, def_inst) { // store后load
                        if (arrayload->getaddr() == arraystore->getaddr() && arrayload->getoffset() == arraystore->getoffset()) {
                            auto useset = uda_->get_useset(arrayload->getdst());
                            for (auto use_inst : useset) {
                                for (auto &src : *use_inst->get_src()) {
                                    if (src == arrayload->getdst()) {
                                        src = arraystore->getsrc();
                                    }
                                }
                            }
                            useless_load.insert(arrayload);
                        }
                    }
                }
            }
        }
        for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
            auto inst = *iter;
            TypeCase(arraystore2, ir::instruction::ArrayStore *, inst) {
                if (!uda_->get_defset(arraystore2->getdep()).empty()) {
                    auto def_inst = *uda_->get_defset(arraystore2->getdep()).begin();
                    TypeCase(arraystore1, ir::instruction::ArrayStore *, def_inst) { // store后store
                        if (arraystore1->get_parent() == arraystore2->get_parent()) {
                            if (arraystore1->getaddr() == arraystore2->getaddr() && arraystore1->getoffset() == arraystore2->getoffset()) {
                                if (arraystore1->getsrc() == arraystore2->getsrc()) {
                                    useless_store.insert(arraystore2);
                                }
                                if (arraystore2->get_used()->empty()) {
                                    useless_store.insert(arraystore1);
                                }
                            }
                        }
                    }
                }
            }
        }
        for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
            auto inst = *iter;
            TypeCase(arraystore, ir::instruction::ArrayStore *, inst) {
                if (!arraystore->get_used()->empty()) {
                    for (auto use_temp : *arraystore->get_used()) {
                        if (!uda_->get_defset(use_temp).empty()) {
                            auto def_inst = *uda_->get_defset(use_temp).begin();
                            TypeCase(arrayload, ir::instruction::ArrayLoad *, def_inst) { // load后store
                                if (arrayload->get_parent() == arraystore->get_parent()) {
                                    if (arrayload->getaddr() == arraystore->getaddr() && arrayload->getoffset() == arraystore->getoffset()) {
                                        if (arrayload->getdst() == arraystore->getsrc()) {
                                            useless_store.insert(arraystore);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (func_->get_name() == "main") { // TODO: 通过判断所使用的地址是否具有副作用来判断是否可以删除
            for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
                auto inst = *iter;
                TypeCase(arraystore, ir::instruction::ArrayStore *, inst) {
                    if (uda_->get_useset(arraystore->getdst()).empty()) {
                        useless_store.insert(arraystore);
                    }
                }
            }
        }
        for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
            auto inst = *iter;
            TypeCase(arrayload, ir::instruction::ArrayLoad *, inst) {
                if (uda_->get_useset(arrayload->getdst()).empty()) {
                    useless_load.insert(arrayload);
                }
            }
        }
    }
    bool changed = false;
    for (auto &bb : *func_->get_basic_blocks()) {
        for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end();) {
            auto inst = *iter;
            TypeCase(arraystore, ir::instruction::ArrayStore *, inst) {
                if (useless_store.count(arraystore)) {
                    iter = bb->get_instructions()->erase(iter);
                    changed = true;
                } else {
                    iter++;
                }
            } else TypeCase(arrayload, ir::instruction::ArrayLoad *, inst) {
                if (useless_load.count(arrayload)) {
                    iter = bb->get_instructions()->erase(iter);
                    changed = true;
                } else {
                    iter++;
                }
            } else {
                iter++;
            }
        }
    }
    return changed;
}

} // namespace middleend