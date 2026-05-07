#include "middleend/loop_parallel.hpp"
#include <iostream>
#include <unordered_set>
#include <unordered_map>

namespace middleend {

using ir::BasicBlock;
static const int THREAD_NUM = 4;

ParallelLoopInfo LoopParallel::get_parallel_info(Loop *loop, const std::unordered_set<BasicBlock *> &loop_bbs, BasicBlock *exit) {
    auto in_loop = [=](BasicBlock *b) {
        Loop *l = la_->get_loop(b);
        while (l != nullptr && l != loop) {
            l = l->parent_;
        }
        return l == loop;
    };
    ParallelLoopInfo info;
    info.valid = false;
    info.entry = loop->header_;
    info.exit = exit;
    info.exit_prev = nullptr;
    for (auto bb : cfg_->prev(exit)) {
        if (in_loop(bb)) {
            if (info.exit_prev == nullptr) info.exit_prev = bb;
            else return info; // multiple exit_prev
        }
    }
    info.into_entry = nullptr;
    for (auto bb : cfg_->prev(info.entry)) {
        if (in_loop(bb)) {
            if (bb != info.exit_prev) return info; // multiple back edge
        } else {
            if (info.into_entry == nullptr) info.into_entry = bb;
            else return info; // multiple into edge
        }
    }
    int cnt_phi = 0;
    // std::cout << "【info.entry】"; info.entry->print(std::cout);
    for (auto &insn : *info.entry->get_instructions()) {
        TypeCase (dummy, ir::instruction::Phi *, insn) {
            cnt_phi++;
        } else break;
    }
    if (cnt_phi > 1) { return info; } // multiple phi at entry
    for (auto &bb : loop_bbs) {
        for (auto &insn : *bb->get_instructions()) {
            TypeCase (call, ir::instruction::Call *, insn) {
                return info;
                // auto funcs = mod->get_functions();
                // auto lib_func = mod->lib_funcs;
                // if (lib_func.count(call->getfunc()->get_name())) {}
                // else {
                //     return info;
                //     if (std::find(funcs->begin(), funcs->end(), call->getfunc()) == funcs->end()) { std::cout << "cannot find callee\n"; return info; }
                //     if (!call->getfunc()->is_pure()) { std::cout << "function not pure\n"; return info; }
                //     if (!call->getfunc()->only_load_arg) { std::cout << "not only load param\n"; return info; }
                // }
                // std::cerr << "could pass call check\n";
                // return info;
            } else if (insn->is_output_inst()) {
                bool flag = true; // all use in loop
                if (uda_->get_usesets().count(insn->get_dst()->front())) {
                    for (auto use : uda_->get_useset(insn->get_dst()->front())) {
                        flag &= in_loop(use->get_parent());
                    }
                }
                if (!flag) { return info; }
            }
        }
    }
    // std::cout << "【exit_prev】"; info.exit_prev->print(std::cout);
    TypeCase (br_cond, ir::instruction::CondBranch *, info.exit_prev->get_instructions()->back()) { // br entry, exit
        TypeCase (binary_cond, ir::instruction::Binary *, *uda_->get_defset(br_cond->getcond()).begin()) { // i < n
            if (binary_cond->getdst()->get_type().base_type != ScalarType::Int) return info;
            // if (binary_cond->get_type() == BinaryOp::Geq) { // try change op
            //     if (uda_->get_useset(binary_cond->getdst()).size() == 1) {
            //         binary_cond->settype(BinaryOp::Lt);
            //         std::swap(br_cond->get_true_bb(), br_cond->get_false_bb());
            //     }
            // }
            if (binary_cond->get_type() != BinaryOp::Lt && binary_cond->get_type() != BinaryOp::Leq) return info;
            if (binary_cond->get_parent()->get_parent()->is_param(binary_cond->getrhs())) return info; // end_reg should be region constant
            TypeCase (loadimm, ir::instruction::LoadImm4*, *uda_->get_defset(binary_cond->getrhs()).begin()) {
                if (loadimm->getimm().iv <= 200) return info;
            }
            info.end_reg = binary_cond->getrhs();
            // auto def_end_reg = *uda_->get_defset(info.end_reg).begin();
            // TypeCase (li, ir::instruction::LoadImm4*, def_end_reg) {} else return info;
            info.cond_cmp = binary_cond;
            info.bop = binary_cond->get_type();
            if (binary_cond->get_parent()->get_parent()->is_param(binary_cond->getlhs())) return info;
            TypeCase (binary_update, ir::instruction::Binary *, *uda_->get_defset(binary_cond->getlhs()).begin()) { // i = i + c
                // std::cout << "【binary_update】"; binary_update->print(std::cout);
                if (binary_update->getdst()->get_type().base_type != Int) return info;
                if (binary_update->get_type() != BinaryOp::Add) return info; // TODO: only consider i + c now
                if (uda_->get_useset(binary_update->getdst()).size() > 2) return info; // updated i only used in cond_cmp & phi
                Temp* reg_i = binary_update->getlhs(), *reg_c = binary_update->getrhs();
                if (binary_update->get_parent()->get_parent()->is_param(reg_i) || binary_update->get_parent()->get_parent()->is_param(reg_c)) return info;
                TypeCase (dummy, ir::instruction::LoadImm4 *, *uda_->get_defset(reg_i).begin()) {
                    std::swap(reg_i, reg_c);
                }
                TypeCase(update_imm, ir::instruction::LoadImm4 *, *uda_->get_defset(reg_c).begin()) { // c
                    assert(update_imm->getimm().type == Int);
                    if (update_imm->getimm().iv != 1) return info; // TODO: only consider c = 1 now
                    info.step = update_imm->getimm().iv;
                } else return info; // step not const
                TypeCase (inst_phi, ir::instruction::Phi *, *uda_->get_defset(reg_i).begin()) {
                    if (inst_phi->getpairs().size() != 2) return info;
                    if (inst_phi->get_parent() != info.entry) return info;
                    Temp* reg_i_init;
                    for (auto pair : inst_phi->getpairs()) {
                        if (pair.first != binary_update->getdst()) { // find init
                            reg_i_init = pair.first;
                            if (la_->get_loop(pair.second) != nullptr) return info;
                        }
                    }
                    info.start_reg = reg_i_init;
                    info.entry_ind_phi = inst_phi;
                } else return info; // no phi inst
                info.binary_upd = binary_update;
            } else return info; // update not binary
        } else return info; // cond not binary
    } else assert(false);
    info.valid = true;
    return info;
}

using ir::Instruction;

Instruction* LoopParallel::copy_inst(ParallelLoopInfo info, Instruction *inst, const std::unordered_map<BasicBlock *, BasicBlock *> &bb_map, const std::unordered_map<Temp*, Temp*> &reg_map) {
    auto reg_map_if = [=](Temp* reg) {
        if (reg_map.count(reg)) return reg_map.at(reg);
        else return reg;
    };
    TypeCase (binary, ir::instruction::Binary *, inst) {
        auto new_inst = new ir::instruction::Binary(binary->get_type(), reg_map_if(binary->getdst()), reg_map_if(binary->getlhs()), reg_map_if(binary->getrhs()));
        return new_inst;
    } else TypeCase (unary, ir::instruction::Unary *, inst) {
        auto new_inst = new ir::instruction::Unary(unary->get_type(), reg_map_if(unary->getdst()), reg_map_if(unary->getsrc()));
        return new_inst;
    } else TypeCase(load, ir::instruction::Load *, inst) {
        auto new_inst = new ir::instruction::Load(reg_map_if(load->getdst()), reg_map_if(load->getaddr()), load->not_delete(), load->getoffset());
        return new_inst;
    } else if (auto convert = dynamic_cast<ir::instruction::Assign *>(inst)) {
        auto new_inst = new ir::instruction::Assign(reg_map_if(convert->getdst()), reg_map_if(convert->getsrc()));
        return new_inst;
    } else TypeCase (loadaddr, ir::instruction::LoadAddr *, inst) {
        auto new_inst = new ir::instruction::LoadAddr(reg_map_if(loadaddr->get_addr()), loadaddr->get_name(), loadaddr->type_);
        return new_inst;
    } else TypeCase (loadimm, ir::instruction::LoadImm4 *, inst) {
        auto new_inst = new ir::instruction::LoadImm4(reg_map_if(loadimm->getdst()), loadimm->getimm());
        return new_inst;
    } else TypeCase (gep, ir::instruction::ElementPtr *, inst) {
        auto indices = gep->get_indices();
        for(auto &index : indices){
            index = reg_map_if(index);
        }
        auto new_inst = new ir::instruction::ElementPtr(reg_map_if(gep->get_dst()), reg_map_if(gep->get_base()), indices);
        return new_inst;
    } else TypeCase (call, ir::instruction::Call *, inst) {
        auto args = call->get_srcs();
        for(auto &arg : args){
            arg = reg_map_if(arg);
        }
        auto new_inst = new ir::instruction::Call(reg_map_if(call->getdst()), args, call->getfunc());
        return new_inst;
    } else TypeCase (store, ir::instruction::Store *, inst) {
        auto new_inst = new ir::instruction::Store(reg_map_if(store->getaddr()), reg_map_if(store->getsrc()), store->getoffset());
        return new_inst;
    } else TypeCase (br, ir::instruction::CondBranch *, inst) {
        auto new_inst = new ir::instruction::CondBranch(br->get_type(), bb_map.at(br->get_true_bb()), bb_map.at(br->get_false_bb()), reg_map_if(br->getcond()));
        return new_inst;
    } else TypeCase (jmp, ir::instruction::Branch *, inst) {
        auto new_inst = new ir::instruction::Branch(bb_map.at(jmp->get_bb()));
        return new_inst;
    } else TypeCase (ret, ir::instruction::Return *, inst) {
        if (ret->has_return_value()) {
            auto ret_val = reg_map_if(ret->getvalue());
            auto new_inst = new ir::instruction::Return(ret_val);
            return new_inst;
        }
        else {
            return new ir::instruction::Return();
        }
    } else TypeCase (phi, ir::instruction::Phi *, inst) {
        auto new_inst = new ir::instruction::Phi(reg_map_if(phi->getdst()), {}, {});
        for (auto pair : phi->getpairs()) {
            auto mapped_bb = bb_map.at(pair.second);
            auto mapped_reg = reg_map_if(pair.first);
            new_inst->add_pair(mapped_bb, mapped_reg);
        }
        return new_inst;
    } else if (auto alloca = dynamic_cast<ir::instruction::Alloca *>(inst)) {
        auto new_inst = new ir::instruction::Alloca(reg_map_if(alloca->getaddr()), alloca->gettype());
        return new_inst;
    } else if (auto cast = dynamic_cast<ir::instruction::Cast *>(inst)) {
        auto new_inst = new ir::instruction::Cast(reg_map_if(cast->getdst()), reg_map_if(cast->getsrc()), cast->gettype());
        return new_inst;
    } else assert(false);
}

void LoopParallel::copy_bb(ParallelLoopInfo info, BasicBlock *bb, BasicBlock *new_bb,
                    const std::unordered_map<BasicBlock *, BasicBlock *> &bb_map, const std::unordered_map<Temp*, Temp*> &reg_map) {
    // map prev & succ
    for (auto prev_bb : cfg_->prev(bb)) {
        cfg_->insert_prev(new_bb, bb_map.at(prev_bb));
    }
    for (auto succ_bb : cfg_->succ(bb)) {
        cfg_->insert_succ(new_bb, bb_map.at(succ_bb));
    }
    // copy insts
    for (auto &insn : *bb->get_instructions()) {
        auto new_inst = copy_inst(info, insn, bb_map, reg_map);
        new_bb->add_instruction(new_inst);
        new_inst->set_parent(new_bb);
    }
    // std::cout << "【original bb】"; bb->print(std::cout);
    // std::cout << "【new_bb】"; new_bb->print(std::cout);
}

bool is_first = true;

void LoopParallel::loop_parallel(ParallelLoopInfo &info, Loop *loop, const std::unordered_set<BasicBlock *> &loop_bbs) {
    // if (!is_first) return;
    // is_first = false;
    std::cout << "loop parallel\n";
    success = true; cnt += 1;
    // TypeCase (li, ir::instruction::LoadImm4*, *uda_->get_defset(info.end_reg).begin()) {
    //     if (li->get_parent() != info.into_entry) {
    //         info.into_entry->add_instruction_before_terminal(li);
    //     }
    // }
    BasicBlock *entry = info.entry;
    BasicBlock *exit = info.exit;
    BasicBlock *into_entry = info.into_entry;
    BasicBlock *exit_prev = info.exit_prev;
    auto bbid = func->get_bb_used();
    // std::cout << "【into_entry】 " << into_entry->get_name() << "\n";
    // std::cout << "【exit_prev】 " << exit_prev->get_name() << "\n";
    // std::cout << "【entry】 " << entry->get_name() << "\n";
    // std::cout << "【exit】 " << exit->get_name() << "\n";
    BasicBlock *new_into_entry = new BasicBlock(func, {}, bbid++); // determine whether to go parallel
    // std::cout << "【new_into_entry】 " << new_into_entry->get_name() << "\n";
    func->add_basic_block(new_into_entry);
    BasicBlock *parallel_entry = new BasicBlock(func, {}, bbid++); // into_entry for all parallel loop, for distributing control flow
    BasicBlock *parallel_entries[THREAD_NUM]; // into_entry for parallel loop
    BasicBlock *parallel_exits[THREAD_NUM]; // exit for parallel loop
    // std::cout << "【parallel_entry】 " << parallel_entry->get_name() << "\n";
    func->add_basic_block(parallel_entry);
    for (int i = 0; i < THREAD_NUM; i++) {
        parallel_entries[i] = new BasicBlock(func, {}, bbid++);
        func->add_basic_block(parallel_entries[i]);
        parallel_exits[i] = new BasicBlock(func, {}, bbid++);
        func->add_basic_block(parallel_exits[i]);
    }
    auto tmp_id = func->get_temp_used();
    // setup new_into_entry
    cfg_->insert_prev(new_into_entry, into_entry);
    cfg_->insert_succ(new_into_entry, parallel_entry);
    cfg_->insert_succ(new_into_entry, entry);
    Temp* parallel_cond = new Temp(tmp_id++, Int), *imm_reg = new Temp(tmp_id++, Int);
    auto new1 = new ir::instruction::LoadImm4(imm_reg, ConstValue(200));
    new_into_entry->add_instruction(new1);
    new1->set_parent(new_into_entry);
    auto new2 = new ir::instruction::Binary(BinaryOp::Geq, parallel_cond, info.end_reg, imm_reg);
    new_into_entry->add_instruction(new2); // check condition for parallel
    new2->set_parent(new_into_entry);
    auto br = new ir::instruction::CondBranch(ir::instruction::CondBranch::IR_Instr_BEQ, parallel_entry, entry, parallel_cond);
    new_into_entry->add_instruction(br);
    br->set_parent(new_into_entry);
    // setup parallel_entry
    cfg_->insert_prev(parallel_entry, new_into_entry);
    for (int i = 0; i < THREAD_NUM; i++) {
        cfg_->insert_succ(parallel_entry, parallel_entries[i]);
    }
    Temp* tid = new Temp(tmp_id++, Int);
    // Temp* trash = new Temp(tmp_id++, Int);
    // Temp* trash0 = new Temp(tmp_id++, Int);
    // auto li = new ir::instruction::LoadImm4(trash, 4);
    // auto syscall0 = new ir::instruction::Call(trash0, {trash}, mod->lib_funcs["putint"]);
    // parallel_entry->add_instruction(li);
    // parallel_entry->add_instruction(syscall0);
    auto syscall = new ir::instruction::Call(tid, {}, create_threads);
    parallel_entry->add_instruction(syscall);
    syscall->set_parent(parallel_entry);
    std::map<int, BasicBlock *> targets;
    for (int i = 0; i < THREAD_NUM - 1; i++) {
        targets[i] = parallel_entries[i];
    }
    BasicBlock* bb = parallel_entry;
    for (auto t: targets) {
        auto new_reg = new Temp(tmp_id++, Int);
        auto new_reg2 = new Temp(tmp_id++, Int);
        BasicBlock* new_bb = new BasicBlock(func, {}, bbid++);
        func->add_basic_block(new_bb);
        auto li = new ir::instruction::LoadImm4(new_reg, t.first);
        auto binary = new ir::instruction::Binary(BinaryOp::Sub, new_reg2, new_reg, tid);
        auto br = new ir::instruction::CondBranch(ir::instruction::CondBranch::IR_Instr_BEQ, new_bb, t.second, new_reg2);
        bb->add_instruction(li);
        bb->add_instruction(binary);
        bb->add_instruction(br);
        li->set_parent(bb);
        binary->set_parent(bb);
        br->set_parent(bb);
        bb = new_bb;
    }
    auto new_j = new ir::instruction::Branch(parallel_entries[THREAD_NUM - 1]);
    bb->add_instruction(new_j);
    new_j->set_parent(bb);

    Temp* new_start, *new_end; // store the last
    std::unordered_map<Temp*, Temp*> reg_map[THREAD_NUM];
    for (int tid = 0; tid < THREAD_NUM; tid++) {
        std::unordered_map<BasicBlock *, BasicBlock *> bb_map;
        for (auto bb : loop_bbs) {
            BasicBlock *new_bb = new BasicBlock(func, {}, bbid++);
            // new_bb->label = "parallel_" + std::to_string(tid) + "_" + bb->label;
            func->add_basic_block(new_bb);
            bb_map[bb] = new_bb;
            for (auto &insn : *bb->get_instructions()) {
                if (insn->is_output_inst()) {
                    Temp* new_reg = new Temp(tmp_id++, insn->get_dst()->front()->get_type());
                    reg_map[tid][insn->get_dst()->front()] = new_reg;
                }
            }
        }
        // setup parallel_entry_tid
        Temp* imm_tid = new Temp(tmp_id++, Int);
        cfg_->insert_prev(parallel_entries[tid], parallel_entry);
        cfg_->insert_succ(parallel_entries[tid], bb_map.at(entry));
        Temp* range_len = new Temp(tmp_id++, Int);
        auto range_sub = new ir::instruction::Binary(BinaryOp::Sub, range_len, info.end_reg, info.start_reg);
        parallel_entries[tid]->add_instruction(range_sub);
        range_sub->set_parent(parallel_entries[tid]);
        auto loadimm_tid = new ir::instruction::LoadImm4(imm_tid, ConstValue(tid));
        parallel_entries[tid]->add_instruction(loadimm_tid);
        loadimm_tid->set_parent(parallel_entries[tid]);
        Temp* tid_plus_one = new Temp(tmp_id++, Int);
        auto tid_inc = new ir::instruction::LoadImm4(tid_plus_one, ConstValue(tid + 1));
        parallel_entries[tid]->add_instruction(tid_inc);
        tid_inc->set_parent(parallel_entries[tid]);
        Temp* imm_TN = new Temp(tmp_id++, Int);
        auto loadimm_TN = new ir::instruction::LoadImm4(imm_TN, ConstValue(THREAD_NUM));
        parallel_entries[tid]->add_instruction(loadimm_TN);
        loadimm_TN->set_parent(parallel_entries[tid]);
        Temp* mul_l = new Temp(tmp_id++, Int), * mul_r = new Temp(tmp_id++, Int);
        auto l_mul = new ir::instruction::Binary(BinaryOp::Mul, mul_l, range_len, imm_tid);
        auto r_mul = new ir::instruction::Binary(BinaryOp::Mul, mul_r, range_len, tid_plus_one);
        parallel_entries[tid]->add_instruction(l_mul);
        l_mul->set_parent(parallel_entries[tid]);
        parallel_entries[tid]->add_instruction(r_mul);
        r_mul->set_parent(parallel_entries[tid]);
        Temp* l = new Temp(tmp_id++, Int), *r = new Temp(tmp_id++, Int);
        auto l_div = new ir::instruction::Binary(BinaryOp::Div, l, mul_l, imm_TN); // l = [(end - start) * tid] / THREAD_NUM
        auto r_div = new ir::instruction::Binary(BinaryOp::Div, r, mul_r, imm_TN); // r = [(end - start) * (tid + 1)] / THREAD_NUM
        parallel_entries[tid]->add_instruction(l_div);
        parallel_entries[tid]->add_instruction(r_div);
        l_div->set_parent(parallel_entries[tid]);
        r_div->set_parent(parallel_entries[tid]);
        Temp* new_start = new Temp(tmp_id++, Int), *new_end = new Temp(tmp_id++, Int);
        auto new_start_add = new ir::instruction::Binary(BinaryOp::Add, new_start, info.start_reg, l);
        auto new_end_add = new ir::instruction::Binary(BinaryOp::Add, new_end, info.start_reg, r);
        parallel_entries[tid]->add_instruction(new_start_add);
        new_start_add->set_parent(parallel_entries[tid]);
        parallel_entries[tid]->add_instruction(new_end_add);
        new_end_add->set_parent(parallel_entries[tid]);
        info.entry_ind_phi->change_use(info.start_reg, new_start);
        info.cond_cmp->change_use(info.end_reg, new_end);

        auto jmp = new ir::instruction::Branch(bb_map.at(entry));
        parallel_entries[tid]->add_instruction(jmp);
        jmp->set_parent(parallel_entries[tid]);
        // std::cout << "【parallel_entries[" << tid << "]】"; parallel_entries[tid]->print(std::cout);
        
        // setup parallel_exit_tid
        cfg_->insert_prev(parallel_exits[tid], exit_prev);
        cfg_->insert_succ(parallel_exits[tid], exit);
        auto dst = new Temp(tmp_id++, Int);
        auto syscall = new ir::instruction::Call(dst, {imm_tid}, join_threads);
        auto jmp0 = new ir::instruction::Branch(exit);
        parallel_exits[tid]->add_instruction(syscall);
        syscall->set_parent(parallel_exits[tid]);
        parallel_exits[tid]->add_instruction(jmp0);
        jmp0->set_parent(parallel_exits[tid]);
        // std::cout << "【parallel_exits[" << tid << "]】"; parallel_exits[tid]->print(std::cout);

        bb_map[into_entry] = parallel_entries[tid];
        bb_map[exit] = parallel_exits[tid];
        if (info.bop == BinaryOp::Leq) {
            info.cond_cmp->settype((tid == THREAD_NUM - 1) ? BinaryOp::Leq : BinaryOp::Lt);
        }
        for (auto bb : loop_bbs) {
            copy_bb(info, bb, bb_map.at(bb), bb_map, reg_map[tid]);
        }
        info.cond_cmp->settype(info.bop);
        // change back original loop's start & end
        // std::cout << info.start_reg->to_str() << ", " << new_start->to_str() << "\n";
        info.entry_ind_phi->change_use(new_start, info.start_reg);
        info.cond_cmp->change_use(new_end, info.end_reg);
    }
    // into_entry -> entry  ==>  into_entry -> new_into_entry -> entry
    // modify into_entry's succ & terinst from entry to new_into_entry
    cfg_->erase_from_succ(into_entry, entry);
    cfg_->insert_succ(into_entry, new_into_entry);
    TypeCase (into_entry_br, ir::instruction::CondBranch *, into_entry->get_instructions()->back()) {
        if (into_entry_br->get_true_bb() == entry) {
            into_entry_br->set_true_bb(new_into_entry);
        } else if (into_entry_br->get_false_bb() == entry) {
            into_entry_br->set_false_bb(new_into_entry);
        } else assert(false);
    } else TypeCase (into_entry_jmp, ir::instruction::Branch *, into_entry->get_instructions()->back()) {
        assert(into_entry_jmp->get_bb() == entry);
        into_entry_jmp->set_target(new_into_entry);
    } else assert(false);
    // modify entry's prev & phi from into_entry to new_into_entry
    cfg_->erase_from_prev(entry, into_entry);
    cfg_->insert_prev(entry, new_into_entry);
    for (auto &insn : *entry->get_instructions()) {
        TypeCase (phi, ir::instruction::Phi *, insn) {
            uda_->erase_use_def(phi);
            phi->add_pair(new_into_entry, phi->get_src_temp(into_entry));
            phi->erase_src_temp(into_entry);
            uda_->compute_use_def(phi);
        } else break;
    }
    // modify exit
    // add new exit from parallel_exit
    for (int i = 0; i < THREAD_NUM; i++) {
        cfg_->insert_prev(exit, parallel_exits[i]);
    }
    for (auto &insn : *exit->get_instructions()) {
        TypeCase (phi, ir::instruction::Phi *, insn) {
            uda_->erase_use_def(phi);
            Temp* new_reg = phi->get_src_temp(exit_prev);
            for (int i = 0; i < THREAD_NUM; i++) {
                if (reg_map[i].count(new_reg)) 
                    new_reg = reg_map[i].at(new_reg);
                phi->add_pair(parallel_exits[i], new_reg);
            }
            uda_->compute_use_def(phi);
        } else break;
    }
    func->set_temp_used(tmp_id);
    func->set_bb_used(bbid);
}

void LoopParallel::run() {
    for (auto &bb : *func->get_basic_blocks()) {
        for (auto &insn : *bb->get_instructions()) {
            insn->set_parent(bb);
        }
        bb->set_parent(func);
    }
    for (auto& loop: la_->loops) {
        if (loop->parent_ != nullptr) continue;
        if (loop->child_ == nullptr) continue;
        // if (loop->child_->child_ == nullptr) continue;
        std::unordered_set<BasicBlock*> loop_bbs;
        auto in_loop = [=](BasicBlock *b) {
            Loop *l = la_->get_loop(b);
            while (l != nullptr && l != loop) {
                l = l->parent_;
            }
            return l == loop;
        };
        std::vector<BasicBlock *> stack;
        stack.push_back(loop->header_);
        while (stack.size()) {
            auto bb = stack.back();
            stack.pop_back();
            if (in_loop(bb)) {
                loop_bbs.insert(bb);
                for (auto each : dt_->get_dominate(bb)) {
                    stack.push_back(each);
                }
            }
        }

        BasicBlock *exit_bb = nullptr;
        bool check = true; // check loop has only one exit
        for (auto bb : loop_bbs) {
            if (!check) break;
            for (auto succ_bb : cfg_->succ(bb)) {
                if (!in_loop(succ_bb)) {
                    if (exit_bb != nullptr && succ_bb != exit_bb) {
                        check = false;
                        break;
                    }
                    exit_bb = succ_bb;
                }
            }
        }

        if (exit_bb == nullptr || !check) { // dead loop or multiple exit
            continue;
        }
            // Parallel this loop
        auto loop_info = get_parallel_info(loop, loop_bbs, exit_bb);
        if (!loop_info.valid) continue;
        if (loop_info.profitable(loop, loop_bbs, uda_)) {
            loop_parallel(loop_info, loop, loop_bbs);
        }
    }
}

}