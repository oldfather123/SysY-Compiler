#include "middleend/remove_useless_inst.hpp"
#include "middleend/use_def_analysis.hpp"
#include "middleend/loop_analysis.hpp"
#include <iostream>

namespace middleend {

void search4pure(ir::Module *module, ir::Function *func) {
    /*
    看看是否包含如下情况：
    1. 使用到了全局变量
    2. store param(array): store的addr是elementptr的dst，其中elementptr的base是param
    */
    std::unordered_set<Temp*> side_effect_temp;
    std::unordered_set<Temp*> arg_temp;
    for (auto arg : *func->get_arg_temp()) {
        if (arg->get_type().is_array()) {
            side_effect_temp.insert(arg);
            arg_temp.insert(arg);
        }
    }
    for (auto bb : *func->get_basic_blocks()) {
        for (auto inst : *bb->get_instructions()) {
            // global var
            TypeCase(loadaddr, ir::instruction::LoadAddr*, inst) {
                side_effect_temp.insert(loadaddr->get_addr());
                func->pure = 0;
            }
            // store param
            else TypeCase(store, ir::instruction::Store*, inst) {
                if (arg_temp.count(store->getaddr())) {
                    func->only_load_arg = 0;
                    func->pure = 0;
                }
                if (side_effect_temp.count(store->getaddr())) {
                    func->has_side_effect = 1;
                }
            }
            else TypeCase(elementptr, ir::instruction::ElementPtr*, inst) {
                if (side_effect_temp.count(elementptr->get_base())) {
                    side_effect_temp.insert(elementptr->get_dst());
                }
                if (arg_temp.count(elementptr->get_base())) {
                    arg_temp.insert(elementptr->get_dst());
                }
            }
            else TypeCase(binary, ir::instruction::Binary*, inst) {
                if (side_effect_temp.count(binary->getlhs()) && binary->getlhs()->get_type().is_array()) {
                    side_effect_temp.insert(binary->getdst());
                }
                if (side_effect_temp.count(binary->getrhs()) && binary->getrhs()->get_type().is_array()) {
                    side_effect_temp.insert(binary->getdst());
                }
                if (arg_temp.count(binary->getlhs()) && binary->getlhs()->get_type().is_array()) {
                    arg_temp.insert(binary->getdst());
                }
                if (arg_temp.count(binary->getrhs()) && binary->getrhs()->get_type().is_array()) {
                    arg_temp.insert(binary->getdst());
                }
            }
            else TypeCase(binaryimm, ir::instruction::BinaryImm*, inst) {
                if (side_effect_temp.count(binaryimm->getlhs()) && binaryimm->getlhs()->get_type().is_array()) {
                    side_effect_temp.insert(binaryimm->getdst());
                }
                if (arg_temp.count(binaryimm->getlhs()) && binaryimm->getlhs()->get_type().is_array()) {
                    arg_temp.insert(binaryimm->getdst());
                }
            }
            
            TypeCase(call, ir::instruction::Call*, inst) {
                if (call->getfunc()->get_name() == func->get_name()) {
                    continue;
                }
                auto call_func = call->getfunc();
                if (std::find(module->get_functions()->begin(), module->get_functions()->end(), call_func) != module->get_functions()->end()) {
                    if (call_func->pure == -1 || call_func->has_side_effect == -1) {
                        search4pure(module, call_func);
                    }
                    if (!call_func->is_pure()) {
                        func->pure = 0;
                    }
                    if (call_func->has_side_effect == 1) {
                        func->has_side_effect = 1;
                    }
                }
                else { // 库函数
                    func->pure = 0;
                    func->has_side_effect = 1;
                }
            }
        }
    }
    if (func->pure == -1) {
        func->pure = 1;
    }
    if (func->only_load_arg == -1) {
        func->only_load_arg = 1;
    }
    if (func->has_side_effect == -1) {
        func->has_side_effect = 0;
    }
}

void mark_pure_functions(ir::Module *module) {
    for (auto func : *module->get_functions()) {
        func->pure = -1;
        func->has_side_effect = -1;
    }
    search4pure(module, module->func_map["main"]);
    module->func_map["main"]->pure = 0;
}

void remove_useless_function(ir::Module *module) {
    std::unordered_set<ir::Function *> func_used;
    std::queue<ir::Function *> q;
    q.push(module->func_map["main"]);
    while(!q.empty()) {
        auto cur_func = q.front();
        q.pop();
        if (std::find(module->get_functions()->begin(), module->get_functions()->end(), cur_func)
                == module->get_functions()->end())
            continue;
        func_used.insert(cur_func);
        for (auto bb : *cur_func->get_basic_blocks()) {
            for (auto inst : *bb->get_instructions()) {
                if (auto call = dynamic_cast<ir::instruction::Call *>(inst)) {
                    if (!func_used.count(call->getfunc())) {
                        q.push(call->getfunc());
                    }
                }
            }
        }
    }
    auto functions = module->get_functions();
    for (auto iter = functions->begin(); iter != functions->end();) {
        if (!func_used.count(*iter)){
            iter = functions->erase(iter);
        } else {
            iter++;
        }
    }
}

bool RemoveUselessInst::remove_useless_definition() {
    bool flag_ret = false;
    std::unordered_set<int> used_temp;
    std::unordered_set<Temp*> arg_temp;
    std::unordered_map<int, ir::Instruction*> temp2inst_; // temp -> inst(temp = ...)
    std::queue<int> q;

    // init used_temp of return, param, global_var, condbranch
    for (auto arg : *func_->get_arg_temp()) {
        if (arg->get_type().is_array()) {
            q.push(arg->get_index());
            arg_temp.insert(arg);
        }
    }
    for (auto bb : *func_->get_basic_blocks()) {
        for (auto inst : *bb->get_instructions()) {
            if (auto ret = dynamic_cast<ir::instruction::Return*>(inst)) {
                if (ret->getvalue()) {
                    q.push(ret->getvalue()->get_index());
                }
            } else if (auto condbranch = dynamic_cast<ir::instruction::CondBranch*>(inst)) { // TODO
                q.push(condbranch->getcond()->get_index());
            } else if (auto call = dynamic_cast<ir::instruction::Call*>(inst)) { // TODO
                for (auto arg : *call->get_src()) {
                    q.push(arg->get_index());
                }
            } else if (auto store = dynamic_cast<ir::instruction::Store*>(inst)) {
                q.push(store->getsrc()->get_index());
                q.push(store->getaddr()->get_index());
            }

            if (inst->get_dst()->size() > 0 && inst->get_dst()->at(0)) {
                temp2inst_[inst->get_dst()->at(0)->get_index()] = inst;
            }
        }
    }

    while(!q.empty()) {
        int index = q.front();
        q.pop();
        if (used_temp.count(index)) continue;
        used_temp.insert(index);
        if (temp2inst_.count(index)) {
            auto inst = temp2inst_[index];
            for (auto src : *inst->get_src()) {
                q.push(src->get_index());
            }
        }
    }

    // for (auto bb : *func_->get_basic_blocks()) {
    //     for (auto inst : *bb->get_instructions()) {
    //         if (auto store = dynamic_cast<ir::instruction::Store*>(inst)) {
    //             // std::cout << "store: " << store->to_str() << std::endl;
    //             used_temp.insert(store->getaddr()->get_index());
    //             for (auto src : *inst->get_src()) {
    //                 used_temp.insert(src->get_index());
    //             }
    //         } else {
    //             // std::cout << "others: " << inst->to_str() << std::endl;
    //             for (auto src : *inst->get_src()) {
    //                 if(src == nullptr) continue;
    //                 used_temp.insert(src->get_index());
    //             }
    //         }
    //     }
    // }
    for (auto bb : *func_->get_basic_blocks()) {
        for (std::vector<ir::Instruction *>::iterator ins = bb->get_instructions()->begin(); ins != bb->get_instructions()->end(); ins++) {
            if (auto call = dynamic_cast<ir::instruction::Call*>(*ins)) {
                if (call->getdst() == nullptr) continue;
                if (!used_temp.count(call->getdst()->get_index())) {
                    (*call->get_dst())[0] = nullptr;
                    if (call->getfunc()->has_side_effect == 0) {
                        bb->get_instructions()->erase(ins);
                        ins--;
                        flag_ret = true;
                    }
                }
                continue;
            }
            if ((*ins)->get_instr_kind() != ir::IRInstrKind::IRInstr_SEQ) continue;
            if (auto store = dynamic_cast<ir::instruction::Store*>(*ins)) continue; // TODO 处理多余的store
            bool flag = false;
            for (auto dst : *(*ins)->get_dst()) {
                if (used_temp.count(dst->get_index())) {
                    flag = true;
                    break;
                }
            }
            if (!flag) {
                // std::cout << "remove unuseless inst: " << (*ins)->to_str() << std::endl;
                bb->get_instructions()->erase(ins);
                ins--;
                flag_ret = true;
            }
        }
    }
    return flag_ret;
}

bool RemoveUselessInst::remove_useless_loop() {
    bool flag_ret = false;

    CFG *cfg = new CFG(func_);
    DominatorTree_ *dom_tree_ = new DominatorTree_(cfg);
    LoopAnalysis *la = new LoopAnalysis(cfg);
    UseDefAnalysis *ud = new UseDefAnalysis(cfg);
    auto use_list = ud->get_usesets();

    // std::cout << "function: " << func_->get_name() << std::endl;
    // for (auto bb : *func_->get_basic_blocks()) {
    //     std::cout << "bb: " << bb->get_name() << " is in loop " << (la->get_loop(bb) ? la->get_loop(bb)->header_->get_name() : "null") << std::endl;
    // }

    for (auto loop : la->loops) {
        // TODO: 处理循环嵌套
        if (loop->child_) {
            continue;
        }

        for (auto bb: *func_->get_basic_blocks()) {
            for (auto inst : *bb->get_instructions()) {
                inst->set_parent(bb);
            }
        }

        delete dom_tree_;
        dom_tree_ = new DominatorTree_(cfg);
        auto idom = dom_tree_->get_idom(loop->header_);
        bool can_delete = true;
        std::unordered_set<ir::BasicBlock *> bbs_in_loop;
        std::queue<ir::BasicBlock *> q;
        q.push(loop->header_);
        while(!q.empty()) {
            auto bb = q.front();
            q.pop();
            bbs_in_loop.insert(bb);
            for (auto &bb_prev_idx : *cfg->get_bb_prev(bb->get_index())) {
                auto bb_prev = cfg->get_bb(bb_prev_idx);
                if (la->get_loop(bb_prev) == loop && !bbs_in_loop.count(bb_prev))
                    q.push(bb_prev);
            }
        }

        // std::cout << "loop: " << loop->header_->get_name() << std::endl;
        // for (auto bb: bbs_in_loop) {
        //     std::cout << "bb: " << bb->get_name() << std::endl;
        // }

        // only one entry: idom
        for (auto &bb_prev_idx : *cfg->get_bb_prev(loop->header_->get_index())) {
            auto bb_prev = cfg->get_bb(bb_prev_idx);
            if (bb_prev != idom) {
                if (!la->get_loop(bb_prev) || la->get_loop(bb_prev) != loop) {
                    can_delete = false;
                    break;
                }
            }
        }

        ir::BasicBlock * out = nullptr;
        ir::BasicBlock* out_bb = nullptr;
        for (auto bb: bbs_in_loop) {
            if (!can_delete) break;
            for (auto inst : *bb->get_instructions()) {
                if (inst->get_dst()->size() > 0 && inst->get_dst()->at(0)) {
                    auto uses = use_list[inst->get_dst()->at(0)];
                    for (auto use : uses) {
                        if (!bbs_in_loop.count(use->get_parent())) {
                            can_delete = false;
                            // if (use->get_dst()->size() > 0 && use->get_dst()->at(0))
                            //     std::cout << use->get_dst()->at(0)->to_str() << std::endl;
                            break;
                        }
                    }
                }
                TypeCase(call, ir::instruction::Call*, inst) {
                    auto module = func_->get_parent();
                    auto call_func = call->getfunc();
                    if (std::find(module->get_functions()->begin(), module->get_functions()->end(), call_func) == module->get_functions()->end()
                        || call_func->has_side_effect == 1) {
                        can_delete = false;
                        break;
                    }
                }
                TypeCase(store, ir::instruction::Store*, inst) {
                    // TODO: check if store addr is in loop
                    can_delete = false;
                    break;
                }
            }
            // only one exit
            for (auto bb_succ_idx : *cfg->get_bb_succ(bb->get_index())) {
                auto bb_succ = cfg->get_bb(bb_succ_idx);
                if (!la->get_loop(bb_succ) || la->get_loop(bb_succ) != loop) {
                    if (out && (bb_succ != out)) {
                        can_delete = false;
                        break;
                    }
                    else {
                        out = bb_succ;
                        out_bb = bb;
                    }
                }
            }
        }
        if (!out || !can_delete) continue;
        flag_ret = true;
        // std::cout << "remove useless loop: " << loop->header_->get_name() << std::endl;
        // remove loop
        for (auto iter = func_->get_basic_blocks()->begin(); iter != func_->get_basic_blocks()->end();) {
            auto bb = *iter;
            if (bbs_in_loop.count(bb)) {
                func_->get_basic_blocks()->erase(iter);
            }
            else {
                iter++;
            }
        }
        cfg->change_succ(idom, loop->header_, out);
        cfg->change_prev(out, out_bb, idom);
    }
    return flag_ret;
}

} // namespace middleend