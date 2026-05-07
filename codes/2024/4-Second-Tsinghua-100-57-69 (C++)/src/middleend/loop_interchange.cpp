#include "middleend/loop_interchange.hpp"

namespace middleend {

InterchangeLoopInfo LoopInterchange::get_interchange_info(Loop *loop) {
    // std::cout << "get loop info\n";
    auto in_loop = [loop](ir::BasicBlock *b, LoopAnalysis *la_) {
        Loop *l = la_->get_loop(b);
        while (l != nullptr && l != loop) {
            l = l->parent_;
        }
        return l == loop;
    };
    InterchangeLoopInfo info;
    info.valid = false;
    info.entry = loop->header_;
    std::vector<ir::BasicBlock *> stack;
    std::unordered_set<ir::BasicBlock *> loop_bbs;
    stack.push_back(loop->header_);
    while (!stack.empty()) {
        auto bb = stack.back();
        stack.pop_back();
        if (in_loop(bb, la_)) {
            loop_bbs.insert(bb);
            for (auto each : dt_->get_dominate(bb)) {
                stack.push_back(each);
            }
        }
    }
    info.exit = nullptr;
    for (auto bb : loop_bbs) {
        // std::cout << "【loop bb】"; bb->print(std::cout);
        for (auto succ_bb : cfg_->succ(bb)) {
            if (!in_loop(succ_bb, la_)) {
                if (info.exit != nullptr && succ_bb != info.exit) {
                    return info; // multiple exit
                }
                info.exit = succ_bb;
            }
        }
    }
    if (info.exit == nullptr) return info; // dead loop
    info.exit_prev = nullptr;
    for (auto bb : cfg_->prev(info.exit)) {
        if (in_loop(bb, la_)) {
            if (info.exit_prev == nullptr) info.exit_prev = bb;
            else return info; // multiple exit_prev
        }
    }
    TypeCase (br_cond, ir::instruction::CondBranch *, (*info.exit_prev->get_instructions()).back()) {
        info.ind_br = br_cond;
        // std::cout << "【ind_br】"; br_cond->print(std::cout);
        TypeCase (binary_cond, ir::instruction::Binary *, *uda_->get_defset(br_cond->getcond()).begin()) {
            info.ind_cond = binary_cond;
            // std::cout << "【ind_cond】"; binary_cond->print(std::cout);
            // if (!binary_cond->get_parent()->get_parent()->is_param(binary_cond->getrhs()) && in_loop((*uda_->get_defset(binary_cond->getrhs()).begin())->get_parent(), la_)) return info; // end_reg should be region constant
            if (binary_cond->get_parent()->get_parent()->is_param(binary_cond->getlhs())) return info;
            TypeCase (binary_update, ir::instruction::Binary *, *uda_->get_defset(binary_cond->getlhs()).begin()) {
                info.ind_upd = binary_update;
                // std::cout << "【ind_upd】"; binary_update->print(std::cout);
                if (binary_update->getdst()->get_type().base_type != Int) return info;
                if (uda_->get_useset(binary_update->getdst()).size() > 2) return info; // updated i only used in cond_cmp & phi
                Temp *reg_i = binary_update->getlhs(), *reg_c = binary_update->getrhs();
                if (binary_update->get_parent()->get_parent()->is_param(reg_i) || binary_update->get_parent()->get_parent()->is_param(reg_c)) return info;
                TypeCase (dummy, ir::instruction::LoadImm4 *, *uda_->get_defset(reg_i).begin()) {
                    std::swap(reg_i, reg_c);
                }
                TypeCase (inst_phi, ir::instruction::Phi *, *uda_->get_defset(reg_i).begin()) {
                    if ((*inst_phi->getbbs()).size() != 2) return info;
                    if (inst_phi->get_parent() != info.entry) return info;
                    // std::cout << "【ind_phi】"; inst_phi->print(std::cout);
                    info.ind_phi = inst_phi;
                } else return info; // no phi inst
            } else return info; // update not binary
        } else return info; // cond not binary
    } else assert(false);
    // std::cout << "info is valid\n";
    info.valid = true;
    return info;
}

void LoopInterchange::loop_interchange(TightlyNestedLoopInfo &nested_info) {
    std::cout << "loop interchange\n";
    auto find_insn = [](ir::Instruction *inst) -> std::vector<ir::Instruction *>::iterator {
        auto insns = inst->get_parent()->get_instructions();
        for (auto insn = insns->begin(); insn != insns->end(); insn++) {
            if (*insn == inst) return insn;
        }
        assert(false);
    };
    Temp *reg_i = nested_info.outer.ind_phi->getdst(), *reg_j = nested_info.inner.ind_phi->getdst();
    // swap two phi
    auto outer_phi = find_insn(nested_info.outer.ind_phi);
    auto inner_phi = find_insn(nested_info.inner.ind_phi);
    uda_->erase_use_def(*outer_phi);
    uda_->erase_use_def(*inner_phi);
    auto i_bb = (*inner_phi)->get_parent();
    auto o_bb = (*outer_phi)->get_parent();
    // (*inner_phi)->set_parent(o_bb);
    // (*outer_phi)->set_parent(i_bb);
    auto outer_phi_inst = dynamic_cast<ir::instruction::Phi*>(*outer_phi);
    auto inner_phi_inst = dynamic_cast<ir::instruction::Phi*>(*inner_phi);
    auto new_outer = new ir::instruction::Phi(inner_phi_inst->getdst(), *inner_phi_inst->getvalues(), *inner_phi_inst->getbbs());
    auto new_inner = new ir::instruction::Phi(outer_phi_inst->getdst(), *outer_phi_inst->getvalues(), *outer_phi_inst->getbbs());
    // std::cout << "【i_bb】"; i_bb->print(std::cout);
    // std::cout << "【o_bb】"; o_bb->print(std::cout);
    *outer_phi = new_outer;
    new_outer->set_parent(i_bb);
    *inner_phi = new_inner;
    new_inner->set_parent(o_bb);
    // std::cout << "【i_bb】"; i_bb->print(std::cout);
    // std::cout << "【o_bb】"; o_bb->print(std::cout);
    // func->print(std::cout);
    uda_->compute_use_def(*outer_phi);
    uda_->compute_use_def(*inner_phi);
    // modify phi's incoming bb
    ir::BasicBlock *into_bb = nullptr;
    TypeCase (new_inner_phi, ir::instruction::Phi *, *inner_phi) {
        std::unordered_map<ir::BasicBlock *, Temp*> new_incoming;
        for (auto &pair: new_inner_phi->getpairs()) {
            if (pair.second == nested_info.outer.exit_prev) {
                new_incoming[nested_info.inner.exit_prev] = pair.first;
            } else {
                into_bb = pair.second;
                new_incoming[nested_info.outer.entry] = pair.first;
            }
        }
        new_inner_phi->setpairs(new_incoming);
    } else assert(false);
    assert(into_bb != nullptr);
    TypeCase (new_outer_phi, ir::instruction::Phi *, *outer_phi) {
        std::unordered_map<ir::BasicBlock *, Temp*> new_incoming;
        for (auto &pair : new_outer_phi->getpairs()) {
            if (pair.second == nested_info.inner.exit_prev) {
                new_incoming[nested_info.outer.exit_prev] = pair.first;
            } else if (pair.second == nested_info.outer.entry) {
                new_incoming[into_bb] = pair.first;
            } else assert(false);
        }
        new_outer_phi->setpairs(new_incoming);
    } else assert(false);
    // std::cout << "【i_bb】"; i_bb->print(std::cout);
    // std::cout << "【o_bb】"; o_bb->print(std::cout);
    // new br val, upd dst
    auto func = nested_info.outer.entry->get_parent();
    auto regid = func->get_temp_used();
    nested_info.outer.ind_br->change_use(nested_info.outer.ind_br->getcond(), new Temp(regid++, Int));
    nested_info.inner.ind_br->change_use(nested_info.inner.ind_br->getcond(), new Temp(regid++, Int));
    auto new_outer_ind_upd = new ir::instruction::Binary(nested_info.inner.ind_upd->get_type(), new Temp(regid++, Int), nested_info.inner.ind_upd->getlhs(), nested_info.inner.ind_upd->getrhs());
    auto new_inner_ind_upd = new ir::instruction::Binary(nested_info.outer.ind_upd->get_type(), new Temp(regid++, Int), nested_info.outer.ind_upd->getlhs(), nested_info.outer.ind_upd->getrhs());
    auto new_outer_ind_cond = new ir::instruction::Binary(nested_info.inner.ind_cond->get_type(), nested_info.outer.ind_br->getcond(), new_outer_ind_upd->getdst(), nested_info.inner.ind_cond->getrhs());
    auto new_inner_ind_cond = new ir::instruction::Binary(nested_info.outer.ind_cond->get_type(), nested_info.inner.ind_br->getcond(), new_inner_ind_upd->getdst(), nested_info.outer.ind_cond->getrhs());
    func->set_temp_used(regid);
    // remove old insts
    // std::cout << "【nested_info.outer.exit_prev】"; nested_info.outer.exit_prev->print(std::cout);
    // std::cout << "【nested_info.inner.exit_prev】"; nested_info.inner.exit_prev->print(std::cout);
    auto outer_upd = find_insn(nested_info.outer.ind_upd);
    nested_info.outer.exit_prev->get_instructions()->erase(outer_upd);
    auto inner_upd = find_insn(nested_info.inner.ind_upd);
    nested_info.inner.exit_prev->get_instructions()->erase(inner_upd);
    auto outer_cond = find_insn(nested_info.outer.ind_cond);
    nested_info.outer.exit_prev->get_instructions()->erase(outer_cond);
    auto inner_cond = find_insn(nested_info.inner.ind_cond);
    nested_info.inner.exit_prev->get_instructions()->erase(inner_cond);
    // insert new insts
    nested_info.outer.exit_prev->add_instruction_before_terminal(new_outer_ind_upd);
    new_outer_ind_upd->set_parent(nested_info.outer.exit_prev);
    nested_info.outer.exit_prev->add_instruction_before_terminal(new_outer_ind_cond);
    new_outer_ind_cond->set_parent(nested_info.outer.exit_prev);
    uda_->compute_use_def(new_outer_ind_upd);
    uda_->compute_use_def(new_outer_ind_cond);
    nested_info.inner.exit_prev->add_instruction_before_terminal(new_inner_ind_upd);
    new_inner_ind_upd->set_parent(nested_info.inner.exit_prev);
    nested_info.inner.exit_prev->add_instruction_before_terminal(new_inner_ind_cond);
    new_inner_ind_cond->set_parent(nested_info.inner.exit_prev);
    uda_->compute_use_def(new_inner_ind_upd);
    uda_->compute_use_def(new_inner_ind_cond);
    // change phi incoming reg
    (*outer_phi)->change_use(nested_info.inner.ind_upd->getdst(), new_outer_ind_upd->getdst());
    (*inner_phi)->change_use(nested_info.outer.ind_upd->getdst(), new_inner_ind_upd->getdst());
    // std::cout << "【nested_info.outer.exit_prev】"; nested_info.outer.exit_prev->print(std::cout);
    // std::cout << "【nested_info.inner.exit_prev】"; nested_info.inner.exit_prev->print(std::cout);
}

void LoopInterchange::run() {
    for (auto &bb : *func->get_basic_blocks()) {
        for (auto &insn : *bb->get_instructions()) {
            insn->set_parent(bb);
        }
        bb->set_parent(func);
    }
    for (auto &loop : la_->loops) {
        if (loop->parent_ == nullptr) continue;
        if (loop->parent_->parent_ != nullptr) continue;
        // if (loop->child_ == nullptr) continue;
        // only consider interchange outermost and second outermost loops
        auto outer_info = get_interchange_info(loop->parent_);
        auto inner_info = get_interchange_info(loop);
        auto nested_info = TightlyNestedLoopInfo(inner_info, outer_info, la_, dt_, cfg_);
        if (!nested_info.valid) continue;
        if (nested_info.profitable(loop)) {
            loop_interchange(nested_info);
        }
    }
}

}