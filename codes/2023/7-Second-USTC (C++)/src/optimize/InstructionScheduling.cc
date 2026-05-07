#include "InstructionScheduling.hh"
#include "RedundantInstsEli.hh"

#define DEBUG_INSTRUCTION_SCHEDULING

/**
 * & 指令的延迟 中间代码层次上，采用启发式方法
*/
int DependencyTreeNode::get_self_delay() {
    int delay;
    switch(inst_->get_instr_type()) {
        case Instruction::call:
            delay = 50;
        case Instruction::sdiv:
            delay = 20;
            break;
        case Instruction::fdiv:
            delay = 12;
            break;
        case Instruction::load:
        case Instruction::loadoffset:
        case Instruction::store:
        case Instruction::storeoffset:
        case Instruction::fmul:
            delay = 5;
            break;
        case Instruction::mul:
        case Instruction::mul64:
        case Instruction::fadd:
        case Instruction::fsub:
            delay = 3;
            break;
        default:
            delay = 1;
            break;
    }

    return delay;
}   

void InstructionScheduling::execute() {
    auto redundant_insts_eli = RedundantInstsEli(m_);
    redundant_insts_eli.execute();

    analyse_ptrphis();
    for(auto func: m_->get_functions()) {
        if(func->is_declaration())
            continue;
        for(auto bb: func->get_basic_blocks()) {
            schedule(bb);
        }
    }
}

void InstructionScheduling::analyse_ptrphis() {
    for(auto func: m_->get_functions()) {
        if(func->is_declaration())
            continue;
        //& 构建phi指令usedef图
        for(auto bb: func->get_basic_blocks()) {
            for(auto instr: bb->get_instructions()) {
                if(instr->is_phi() && instr->get_type()->is_pointer_type()) {
                    auto phi_instr = dynamic_cast<PhiInst*>(instr);
                    if(phi2phinode.find(phi_instr) == phi2phinode.end()) {
                        phi2phinode.insert({phi_instr, new PhiAddrNode(phi_instr, nullptr)});
                    }
                    auto phinode = phi2phinode[phi_instr];
                    for(auto opr: phi_instr->get_operands()) {
                        if(opr == phi_instr) 
                            continue;
                        auto gep_opr = dynamic_cast<GetElementPtrInst*>(opr);
                        if(gep_opr) {
                            auto lval = gep_opr->get_operand(0);
                            while(dynamic_cast<GetElementPtrInst*>(lval))
                                lval = dynamic_cast<GetElementPtrInst*>(lval)->get_operand(0);
                            auto phi_lval = dynamic_cast<PhiInst*>(lval);
                            if(phi_lval) {
                                if(phi2phinode.find(phi_lval) == phi2phinode.end()) {
                                    phi2phinode.insert({phi_lval, new PhiAddrNode(phi_lval, nullptr)});
                                }
                                auto opr_phinode = phi2phinode[phi_lval];
                                opr_phinode->parents_.push_back(phinode);
                                phinode->childs_.push_back(opr_phinode);
                            } else {
                                if(leaf2phinode.find(lval) == leaf2phinode.end()) {
                                    leaf2phinode.insert({lval, new PhiAddrNode(nullptr, lval)});
                                }
                                auto leafnode = leaf2phinode[lval];
                                leafnode->parents_.push_back(phinode);
                                phinode->childs_.push_back(leafnode);
                            }
                        } else {
                             auto phi_opr = dynamic_cast<PhiInst*>(opr);
                             if(phi_opr) {
                                if(phi2phinode.find(phi_opr) == phi2phinode.end()) {
                                    phi2phinode.insert({phi_opr, new PhiAddrNode(phi_opr, nullptr)});
                                }
                                auto opr_phinode = phi2phinode[phi_opr];
                                opr_phinode->parents_.push_back(phinode);
                                phinode->childs_.push_back(opr_phinode);
                            } else {
                                if(leaf2phinode.find(opr) == leaf2phinode.end()) {
                                    leaf2phinode.insert({opr, new PhiAddrNode(nullptr, opr)});
                                }
                                auto leafnode = leaf2phinode[opr];
                                leafnode->parents_.push_back(phinode);
                                phinode->childs_.push_back(leafnode);
                            }
                        }
                    }
                }
            }
        }
        //& 从leaf向上传播,得到phi指令(指针类型)所有可能的实际参数(alloc或global的变量)
        for(auto &[val, leaf]: leaf2phinode) {
            for(auto &par: leaf->parents_) {
                start = par;
                cur_loc = val;
                par->real_addrs.insert(cur_loc);
                bottom_up_propagatioin(start);
            }
        }
    }
}

void InstructionScheduling::bottom_up_propagatioin(PhiAddrNode *node) {
    for(auto &par: node->parents_) {
        par->real_addrs.insert(cur_loc);
        //& 如果存在环并回到起始位置则终止
        if(par != start) {
            bottom_up_propagatioin(par);
        }
    }
}

/**
 * & 表调度算法
 * & 基于基本块的调度,为简单起见,由于call指令调用的函数可能存在复杂的调用链从而不好统计其可能造成的副作用,因此默认call与所有的load和store构成全依赖
 * TODO 完善的指针分析,改进call指令的调度
*/

void InstructionScheduling::schedule(BasicBlock *bb) {
    #ifdef DEBUG_INSTRUCTION_SCHEDULING
        std::cout << "Before Scheduling:" << std::endl; 
        std::cout << bb->print() << std::endl;
    #endif

    init(bb);
    numbering_insts();

    handle_alloc_insts();
    handle_phi_insts();
    handle_memset_insts();

    build_dependency_tree();

    compute_delays();

    init_ready_queue();

    cycle = 1;
    while(!ready.empty() || !active.empty()) {
        for(auto active_iter = active.begin(); active_iter != active.end(); ++active_iter) {
            if(inst2node[*active_iter]->get_end_time() < cycle) {
                for(auto par: inst2node[*active_iter]->get_parents()) {
                    if(par == root)
                        continue;
                    par->delete_dependency(inst2node[*active_iter]);
                    if(par->get_num_of_dependencys() == 0) {
                        ready.push(par);
                    }
                }
                inst2node[*active_iter]->clear_parents();
                active.erase(active_iter);
                --active_iter;
            }
        }

        if(! ready.empty()) {
            auto node = ready.top();
            ready.pop();
            node->set_start_time(cycle);
            scheduled_insts_of_curbb.push_back(node->get_inst());
            active.push_back(node->get_inst());
        }

        cycle += 1;
    }

    handle_terminator_inst();

    bb->set_instructions_list(scheduled_insts_of_curbb);

    #ifdef DEBUG_INSTRUCTION_SCHEDULING
        std::cout << "After Scheduling:" << std::endl; 
        std::cout << bb->print() << std::endl;
    #endif

    return ;
}

void InstructionScheduling::numbering_insts() {
    int idx = 0;
    for(auto inst: cur_bb->get_instructions()) {
        inst2idx[inst] = idx;
        idx++;
    }
    return ;
}

void InstructionScheduling::init(BasicBlock *bb) {
    cur_bb = bb;
    active.clear();
    scheduled_insts_of_curbb.clear();
    root->clear_dependency();
    inst2idx.clear();
    cur_var2locuse.clear();
    cur_array2locuse.clear();
    cur_arg2locuse.clear();
    cur_callnode.clear();
    inst2node.clear();
    scheduled_insts_of_curbb.clear();
    return ;
}

void InstructionScheduling::handle_alloc_insts() {
    for(auto inst: cur_bb->get_instructions()) {
        if(inst->is_alloca()) {
            scheduled_insts_of_curbb.push_back(inst);
        }
    }
}

void InstructionScheduling::handle_phi_insts() {
    for(auto inst: cur_bb->get_instructions()) {
        if(inst->is_phi()) {
            scheduled_insts_of_curbb.push_back(inst);
        }
    }
}

void InstructionScheduling::handle_memset_insts() {
    for(auto inst: cur_bb->get_instructions()) {
        if(inst->is_memset()) {
            scheduled_insts_of_curbb.push_back(inst);
        }
    }
}   

void InstructionScheduling::handle_terminator_inst() {
    scheduled_insts_of_curbb.push_back(cur_bb->get_terminator());
}

void InstructionScheduling::build_usedef_dependency_tree() {
    for(auto riter = cur_bb->get_instructions().rbegin(); riter != cur_bb->get_instructions().rend(); riter++) {
        if(*riter == cur_bb->get_terminator() || (*riter)->is_alloca() || (*riter)->is_memset() || (*riter)->is_phi())
            continue;
        if(inst2node.find(*riter) == inst2node.end()) {
            inst2node.insert({*riter, new DependencyTreeNode(*riter)});
        }
        auto inst = *riter;
        if(dynamic_cast<LoadInst*>(inst)) {
            auto lval = dynamic_cast<GlobalVariable*>(inst->get_operand(0));
            if(lval == nullptr)
                LOG(ERROR) << "出现未预期情况";
            if(cur_var2locuse.find(lval) == cur_var2locuse.end()) {
                cur_var2locuse.insert({lval, new LocUse(false, lval)});
            }
            cur_var2locuse[lval]->loadinsts.insert(inst);
        } else if(dynamic_cast<LoadOffsetInst*>(inst)) {
            auto loadoffset_inst = dynamic_cast<LoadOffsetInst*>(inst);
            auto lval = dynamic_cast<GetElementPtrInst*>(loadoffset_inst->get_lval())->get_operand(0);
            if(dynamic_cast<PhiInst*>(lval)) {
                auto phi_node = phi2phinode[dynamic_cast<PhiInst*>(lval)];
                for(auto real_addr: phi_node->real_addrs) {
                    auto arg_addr = dynamic_cast<Argument*>(real_addr);
                    if(arg_addr) {
                        if(cur_arg2locuse.find(arg_addr) == cur_arg2locuse.end()) {
                            cur_arg2locuse.insert({arg_addr, new LocUse(true, arg_addr)});
                        }
                        cur_arg2locuse[arg_addr]->loadinsts.insert(inst);
                    } else {
                        if(cur_array2locuse.find(real_addr) == cur_array2locuse.end()) {
                            cur_array2locuse.insert({real_addr, new LocUse(false, real_addr)});
                        }
                        cur_array2locuse[real_addr]->loadinsts.insert(inst);
                    }
                }
            } else if(dynamic_cast<Argument*>(lval)) {
                auto arg_addr = dynamic_cast<Argument*>(lval);
                if(cur_arg2locuse.find(arg_addr) == cur_arg2locuse.end()) {
                    cur_arg2locuse.insert({arg_addr, new LocUse(true, arg_addr)});
                }
                cur_arg2locuse[arg_addr]->loadinsts.insert(inst);
            } else {
                if(cur_array2locuse.find(lval) == cur_array2locuse.end()) {
                    cur_array2locuse.insert({lval, new LocUse(false, lval)});
                }
                cur_array2locuse[lval]->loadinsts.insert(inst);
            }
        } else if(dynamic_cast<StoreInst*>(inst)) {
            auto lval = dynamic_cast<GlobalVariable*>(inst->get_operand(1));
            if(lval == nullptr)
                LOG(ERROR) << "出现未预期情况";
            if(cur_var2locuse.find(lval) == cur_var2locuse.end()) {
                cur_var2locuse.insert({lval, new LocUse(false, lval)});
            }
            cur_var2locuse[lval]->storeinsts.insert(inst); 
        } else if(dynamic_cast<StoreOffsetInst*>(inst)) {
            auto storeoffset_inst = dynamic_cast<StoreOffsetInst*>(inst);
            auto lval = dynamic_cast<GetElementPtrInst*>(storeoffset_inst->get_lval())->get_operand(0);
            if(dynamic_cast<PhiInst*>(lval)) {
                auto phi_node = phi2phinode[dynamic_cast<PhiInst*>(lval)];
                for(auto real_addr: phi_node->real_addrs) {
                    auto arg_addr = dynamic_cast<Argument*>(real_addr);
                    if(arg_addr) {
                        if(cur_arg2locuse.find(arg_addr) == cur_arg2locuse.end()) {
                            cur_arg2locuse.insert({arg_addr, new LocUse(true, arg_addr)});
                        }
                        cur_arg2locuse[arg_addr]->storeinsts.insert(inst);
                    } else {
                        if(cur_array2locuse.find(real_addr) == cur_array2locuse.end()) {
                            cur_array2locuse.insert({real_addr, new LocUse(false, real_addr)});
                        }
                        cur_array2locuse[real_addr]->storeinsts.insert(inst);
                    }
                }
            } else if(dynamic_cast<Argument*>(lval)) {
                auto arg_addr = dynamic_cast<Argument*>(lval);
                if(cur_arg2locuse.find(arg_addr) == cur_arg2locuse.end()) {
                    cur_arg2locuse.insert({arg_addr, new LocUse(true, arg_addr)});
                }
                cur_arg2locuse[arg_addr]->storeinsts.insert(inst);
            } else {
                if(cur_array2locuse.find(lval) == cur_array2locuse.end()) {
                    cur_array2locuse.insert({lval, new LocUse(false, lval)});
                }
                cur_array2locuse[lval]->storeinsts.insert(inst);
            }
        } else if(dynamic_cast<CallInst*>(inst)) {
            cur_callnode.push_back(inst2node[inst]);
        }

        auto inst_node = inst2node[*riter];

        for(auto opr: (*riter)->get_operands()) {
            auto inst_opr = dynamic_cast<Instruction*>(opr);
            if(inst_opr == nullptr || 
            inst_opr->get_parent() != cur_bb ||
            dynamic_cast<AllocaInst*>(opr) || 
            dynamic_cast<PhiInst*>(opr) || 
            dynamic_cast<MemsetInst*>(opr))
                continue;
            if(inst2node.find(inst_opr) == inst2node.end()) {
                inst2node.insert({inst_opr, new DependencyTreeNode(inst_opr)});
            }
            inst_node->add_dependency(inst2node[inst_opr]);
        }
    }
}

void InstructionScheduling::build_loadstore_dependency_tree() {
    //& 参数地址 保守起见,参数地址构成全依赖
    for(auto &[arg1, loc1]: cur_arg2locuse) {
        for(auto &[arg2, loc2]: cur_arg2locuse) {
            for(auto storeinst1: loc1->storeinsts) {
                for(auto storeinst2: loc2->storeinsts) {
                    if(inst2idx[storeinst2] > inst2idx[storeinst1]) {
                        inst2node[storeinst2]->add_dependency(inst2node[storeinst1]);
                    }
                }
                for(auto loadinst2: loc2->loadinsts) {
                    if(inst2idx[loadinst2] > inst2idx[storeinst1]) {
                        inst2node[loadinst2]->add_dependency(inst2node[storeinst1]);
                    }
                }
            }
            for(auto loadinst1: loc1->loadinsts) {
                for(auto storeinst2: loc2->storeinsts) {
                    if(inst2idx[storeinst2] > inst2idx[loadinst1]) {
                        inst2node[storeinst2]->add_dependency(inst2node[loadinst1]);
                    }
                }
            }
        }
        for(auto &[arr, loc2]: cur_array2locuse) {
            for(auto storeinst1: loc1->storeinsts) {
                for(auto storeinst2: loc2->storeinsts) {
                    if(inst2idx[storeinst2] > inst2idx[storeinst1]) {
                        inst2node[storeinst2]->add_dependency(inst2node[storeinst1]);
                    }
                }
                for(auto loadinst2: loc2->loadinsts) {
                    if(inst2idx[loadinst2] > inst2idx[storeinst1]) {
                        inst2node[loadinst2]->add_dependency(inst2node[storeinst1]);
                    }
                }
            }
            for(auto loadinst1: loc1->loadinsts) {
                for(auto storeinst2: loc2->storeinsts) {
                    if(inst2idx[storeinst2] > inst2idx[loadinst1]) {
                        inst2node[storeinst2]->add_dependency(inst2node[loadinst1]);
                    }
                }
            }
        }

        for(auto storeinst1: loc1->storeinsts) {
            for(auto call_node: cur_callnode) {
                if(inst2idx[call_node->get_inst()] > inst2idx[storeinst1]) {
                    call_node->add_dependency(inst2node[storeinst1]);
                }
            }
        }

        for(auto loadinst1: loc1->loadinsts) {
            for(auto call_node: cur_callnode) {
                if(inst2idx[call_node->get_inst()] > inst2idx[loadinst1]) {
                    call_node->add_dependency(inst2node[loadinst1]);
                }
            }
        }
    }

    //& 数组地址
    for(auto &[arr, loc1]: cur_array2locuse) {
        for(auto &[arg, loc2]: cur_arg2locuse) {
            for(auto storeinst1: loc1->storeinsts) {
                for(auto storeinst2: loc2->storeinsts) {
                    if(inst2idx[storeinst2] > inst2idx[storeinst1]) {
                        inst2node[storeinst2]->add_dependency(inst2node[storeinst1]);
                    }
                }
                for(auto loadinst2: loc2->loadinsts) {
                    if(inst2idx[loadinst2] > inst2idx[storeinst1]) {
                        inst2node[loadinst2]->add_dependency(inst2node[storeinst1]);
                    }
                }
            }
            for(auto loadinst1: loc1->loadinsts) {
                for(auto storeinst2: loc2->storeinsts) {
                    if(inst2idx[storeinst2] > inst2idx[loadinst1]) {
                        inst2node[storeinst2]->add_dependency(inst2node[loadinst1]);
                    }
                }
            }
        }
        for(auto storeinst1: loc1->storeinsts) {
            for(auto storeinst2: loc1->storeinsts) {
                if(inst2idx[storeinst2] > inst2idx[storeinst1]) {
                    inst2node[storeinst2]->add_dependency(inst2node[storeinst1]);
                }
            }
            for(auto loadinst2: loc1->loadinsts) {
                if(inst2idx[loadinst2] > inst2idx[storeinst1]) {
                    inst2node[loadinst2]->add_dependency(inst2node[storeinst1]);
                }
            }
        }
        for(auto loadinst1: loc1->loadinsts) {
            for(auto storeinst2: loc1->storeinsts) {
                if(inst2idx[storeinst2] > inst2idx[loadinst1]) {
                    inst2node[storeinst2]->add_dependency(inst2node[loadinst1]);
                }
            }
        }

        for(auto storeinst1: loc1->storeinsts) {
            for(auto call_node: cur_callnode) {
                if(inst2idx[call_node->get_inst()] > inst2idx[storeinst1]) {
                    call_node->add_dependency(inst2node[storeinst1]);
                }
            }
        }

        for(auto loadinst1: loc1->loadinsts) {
            for(auto call_node: cur_callnode) {
                if(inst2idx[call_node->get_inst()] > inst2idx[loadinst1]) {
                    call_node->add_dependency(inst2node[loadinst1]);
                }
            }
        }
    }

    //& 单独变量地址(经过mem2reg后,只剩下全局的单独变量)
    for(auto &[var, loc]: cur_var2locuse) {
        for(auto storeinst1: loc->storeinsts) {
            for(auto storeinst2: loc->storeinsts) {
                if(inst2idx[storeinst2] > inst2idx[storeinst1]) {
                    inst2node[storeinst2]->add_dependency(inst2node[storeinst1]);
                }
            }
            for(auto loadinst2: loc->loadinsts) {
                if(inst2idx[loadinst2] > inst2idx[storeinst1]) {
                    inst2node[loadinst2]->add_dependency(inst2node[storeinst1]);
                }
            }
        }
        for(auto loadinst1: loc->loadinsts) {
            for(auto storeinst2: loc->storeinsts) {
                if(inst2idx[storeinst2] > inst2idx[loadinst1]) {
                    inst2node[storeinst2]->add_dependency(inst2node[loadinst1]);
                }
            }
        }

        for(auto storeinst1: loc->storeinsts) {
            for(auto call_node: cur_callnode) {
                if(inst2idx[call_node->get_inst()] > inst2idx[storeinst1]) {
                    call_node->add_dependency(inst2node[storeinst1]);
                }
            }
        }

        for(auto loadinst1: loc->loadinsts) {
            for(auto call_node: cur_callnode) {
                if(inst2idx[call_node->get_inst()] > inst2idx[loadinst1]) {
                    call_node->add_dependency(inst2node[loadinst1]);
                }
            }
        }
    }

    //& call inst(简单的认为call指令和所有其他的访存指令形成依赖)
    for(auto call_node1: cur_callnode) {
        for(auto &[arg, loc]: cur_arg2locuse) {
            for(auto storeinst2: loc->storeinsts) {
                if(inst2idx[storeinst2] > inst2idx[call_node1->get_inst()]) {
                    inst2node[storeinst2]->add_dependency(call_node1);
                }
            }
            for(auto loadinst2: loc->loadinsts) {
                if(inst2idx[loadinst2] > inst2idx[call_node1->get_inst()]) {
                    inst2node[loadinst2]->add_dependency(call_node1);
                }
            }
        }

        for(auto &[arr, loc]: cur_array2locuse) {
            for(auto storeinst2: loc->storeinsts) {
                if(inst2idx[storeinst2] > inst2idx[call_node1->get_inst()]) {
                    inst2node[storeinst2]->add_dependency(call_node1);
                }
            }
            for(auto loadinst2: loc->loadinsts) {
                if(inst2idx[loadinst2] > inst2idx[call_node1->get_inst()]) {
                    inst2node[loadinst2]->add_dependency(call_node1);
                }
            }
        }

        for(auto &[var, loc]: cur_var2locuse) {
            for(auto storeinst2: loc->storeinsts) {
                if(inst2idx[storeinst2] > inst2idx[call_node1->get_inst()]) {
                    inst2node[storeinst2]->add_dependency(call_node1);
                }
            }
            for(auto loadinst2: loc->loadinsts) {
                if(inst2idx[loadinst2] > inst2idx[call_node1->get_inst()]) {
                    inst2node[loadinst2]->add_dependency(call_node1);
                }
            }
        }

        for(auto call_node2: cur_callnode) {
            if(inst2idx[call_node2->get_inst()] > inst2idx[call_node1->get_inst()]) {
                call_node2->add_dependency(call_node1);
            }
        }
    }
}

void InstructionScheduling::aggregate_roots() {
    for(auto &[inst, node]: inst2node) {
        if(node->is_root()) {
            root->add_dependency(node);
        }
    }
    return ;
}

void InstructionScheduling::init_ready_queue() {
    for(auto inst: cur_bb->get_instructions()) {
        if(inst->is_phi() || inst->is_alloca() || inst->is_memset() || inst == cur_bb->get_terminator())
            continue;
        if(inst2node[inst]->get_num_of_dependencys() == 0 && inst2node[inst] && inst2node[inst] != root)
            ready.push(inst2node[inst]);
    }
    return ;
}

//& dfs计算依赖图上的延迟(存在多条路径时取最大延迟)
void InstructionScheduling::compute_delays() {
    std::stack<std::pair<DependencyTreeNode*, int>> dfs_stack;
    std::set<DependencyTreeNode*> visited;
    for(auto root_child: root->get_dependencys()) {
        dfs_stack.push({root_child, 0});
        visited.insert(root_child);
        while(!dfs_stack.empty()) {
            auto cur_node = dfs_stack.top();
            DependencyTreeNode* next_child = nullptr;
            for(auto child_node: cur_node.first->get_dependencys()) {
                if(visited.find(child_node) == visited.end()) {
                    next_child = child_node;
                    break;
                }
            }
            if(next_child == nullptr) {
                cur_node.first->set_delay(cur_node.second + cur_node.first->get_self_delay());
                // for(auto child_node: cur_node.first->get_dependencys()) {
                //     visited.erase(child_node);
                // }
                dfs_stack.pop();
            } else {
                dfs_stack.push({next_child, cur_node.second + cur_node.first->get_self_delay()});
                visited.insert(next_child);
            }
        }
    }
}