#include "middleend/global_code_motion.hpp"

namespace middleend {

void GlobalCodeMotion::clear_visit() {
    for (auto inst : inst_list) {
        visit[inst] = false;
    }
}

void GlobalCodeMotion::schedule_early(ir::Instruction *inst) {
    if (visit[inst]) {
        return;
    }
    visit[inst] = true;
    TypeCase(loadimm, ir::instruction::LoadImm4 *, inst) {
        if (inst->get_parent() != root) {
            inst->get_parent()->remove_instruction(inst);
            root->add_instruction_before_terminal(inst);
        }
    } else TypeCase(loadaddr, ir::instruction::LoadAddr *, inst) {
        if (inst->get_parent() != root) {
            inst->get_parent()->remove_instruction(inst);
            root->add_instruction_before_terminal(inst);
        }
    } else TypeCase(binary, ir::instruction::Binary *, inst) {
        ir::BasicBlock *candidate = root;
        if (!func->is_param(binary->getlhs())) {
            auto inst1 = *uda_->get_defset(binary->getlhs()).begin();
            schedule_early(inst1);
            if (dt_->get_dom_depth(inst1->get_parent()) > dt_->get_dom_depth(candidate)) {
                candidate = inst1->get_parent();
            }
        }
        if (!func->is_param(binary->getrhs())) {
            auto inst2 = *uda_->get_defset(binary->getrhs()).begin();
            schedule_early(inst2);
            if (dt_->get_dom_depth(inst2->get_parent()) > dt_->get_dom_depth(candidate)) {
                candidate = inst2->get_parent();
            }
        }
        if (inst->get_parent() != candidate) {
            inst->get_parent()->remove_instruction(inst);
            candidate->add_instruction_before_terminal(inst);
        }
    } else TypeCase(binaryimm, ir::instruction::BinaryImm *, inst) {
        ir::BasicBlock *candidate = root;
        if (!func->is_param(binaryimm->getlhs())) {
            auto inst1 = *uda_->get_defset(binaryimm->getlhs()).begin();
            schedule_early(inst1);
            if (dt_->get_dom_depth(inst1->get_parent()) > dt_->get_dom_depth(candidate)) {
                candidate = inst1->get_parent();
            }
        }
        if (inst->get_parent() != candidate) {
            inst->get_parent()->remove_instruction(inst);
            candidate->add_instruction_before_terminal(inst);
        }
    } else TypeCase(unary, ir::instruction::Unary *, inst) {
        ir::BasicBlock *candidate = root;
        if (!func->is_param(unary->getsrc())) {
            auto inst1 = *uda_->get_defset(unary->getsrc()).begin();
            schedule_early(inst1);
            if (dt_->get_dom_depth(inst1->get_parent()) > dt_->get_dom_depth(candidate)) {
                candidate = inst1->get_parent();
            }
        }
        if (inst->get_parent() != candidate) {
            inst->get_parent()->remove_instruction(inst);
            candidate->add_instruction_before_terminal(inst);
        }
    } else TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
        ir::BasicBlock *candidate = root;
        for (auto src : *elementptr->get_src()) {
            if (!func->is_param(src)) {
                auto inst1 = *uda_->get_defset(src).begin();
                schedule_early(inst1);
                if (dt_->get_dom_depth(inst1->get_parent()) > dt_->get_dom_depth(candidate)) {
                    candidate = inst1->get_parent();
                }
            }
        }
        if (inst->get_parent() != candidate) {
            inst->get_parent()->remove_instruction(inst);
            candidate->add_instruction_before_terminal(inst);
        }
    } else TypeCase(call, ir::instruction::Call *, inst) {
        // TODO: 分析函数是否具有副作用再实现
    } else TypeCase(branch, ir::instruction::Branch *, inst) {
        // nothing to do
    } else TypeCase(ret, ir::instruction::Return *, inst) {
        // nothing to do
    } else TypeCase(store, ir::instruction::Store *, inst) {
        // nothing to do
    } else TypeCase(load, ir::instruction::Load *, inst) {
        // nothing to do
    } else TypeCase(arrayload, ir::instruction::ArrayLoad *, inst) {
        ir::BasicBlock *candidate = root;
        for (auto src : *arrayload->get_src()) {
            if (!func->is_param(src)) {
                auto inst1 = *uda_->get_defset(src).begin();
                schedule_early(inst1);
                if (dt_->get_dom_depth(inst1->get_parent()) > dt_->get_dom_depth(candidate)) {
                    candidate = inst1->get_parent();
                }
            }
        }
        if (inst->get_parent() != candidate) {
            inst->get_parent()->remove_instruction(inst);
            candidate->add_instruction_before_terminal(inst);
        }
    } else TypeCase(arraystore, ir::instruction::ArrayStore *, inst) {
        // nothing to do
    } else TypeCase(alloc, ir::instruction::Alloca *, inst) {
        if (inst->get_parent() != root) {
            inst->get_parent()->remove_instruction(inst);
            root->add_instruction_before_terminal(inst);
        }
    } else TypeCase(phi, ir::instruction::Phi *, inst) {
        // nothing to do
    } else {
        // nothing to do
    }
}

bool GlobalCodeMotion::is_pinned(ir::Instruction *inst) { // 判断是否是不可移动的指令
    TypeCase(load, ir::instruction::Load *, inst) {
        return true;
    } else TypeCase(store, ir::instruction::Store *, inst) {
        return true;
    } else TypeCase(arrayload, ir::instruction::ArrayLoad *, inst) {
        return false;
    } else TypeCase(arraystore, ir::instruction::ArrayStore *, inst) {
        return true;
    } else TypeCase(branch, ir::instruction::Branch *, inst) {
        return true;
    } else TypeCase(cond, ir::instruction::CondBranch *, inst) {
        return true;
    } else TypeCase(ret, ir::instruction::Return *, inst) {
        return true;
    } else TypeCase(call, ir::instruction::Call *, inst) {
        return true; // TODO: 分析函数是否具有副作用再实现
    } else TypeCase(phi, ir::instruction::Phi *, inst) {
        return true;
    } else {
        return false;
    }
}

void add_instruction_after_phi(ir::Instruction *inst, ir::BasicBlock *bb) {
    for (int i = 0; i < bb->get_instructions()->size(); ++i) {
        TypeCase(phi, ir::instruction::Phi *, bb->get_instructions()->at(i)) {
            continue;
        } else {
            bb->get_instructions()->insert(bb->get_instructions()->begin() + i, inst);
            inst->set_parent(bb);
            break;
        }
    }
}

void GlobalCodeMotion::schedule_late(ir::Instruction *inst) {
    if (visit[inst]) {
        return;
    }
    visit[inst] = true;
    if (is_pinned(inst)) {
        return;
    }
    if (inst->get_dst()->size() == 0) {
        return;
    }

    auto dst = inst->get_dst()->front();
    if (uda_->get_useset(dst).size() == 0) {
        return;
    }

    ir::BasicBlock *lca = nullptr;
    for (auto use_inst : uda_->get_useset(dst)) {
        schedule_late(use_inst);
        ir::BasicBlock *use = use_inst->get_parent();
        TypeCase(phi, ir::instruction::Phi *, use_inst) { // 如果是phi指令，找到对应的基本块
            use = nullptr;
            for (int i = 0; i < phi->get_src()->size(); i++) {
                if (phi->get_src()->at(i) == dst) {
                    if (use == nullptr) {
                        use = phi->getbbs()->at(i);
                    } else {
                        use = dt_->find_lca(use, phi->getbbs()->at(i));
                    }
                }
            }
            // assert(use != nullptr);
            if (use == nullptr) {
                assert(phi->get_used()->find(dst) != phi->get_used()->end());
                use = phi->get_parent();
            }
        }
        lca = dt_->find_lca(lca, use);
    }
    assert(lca != nullptr);
    ir::BasicBlock *best = lca;
    // std::cout << inst->to_str() << "\n";
    while (lca != inst->get_parent()) {
        if (la_->get_loop_depth(lca) < la_->get_loop_depth(best)){
            best = lca;
        }
        lca = dt_->get_idom(lca);
        assert(lca != nullptr);
    }
    if (la_->get_loop_depth(lca) < la_->get_loop_depth(best)){
        best = lca;
    }
    if (best != inst->get_parent()) {
        inst->get_parent()->remove_instruction(inst);
        add_instruction_after_phi(inst, best);
    }
}

void GlobalCodeMotion::run() {
    clear_visit();
    for (auto inst : inst_list) {
        schedule_early(inst);
    }
    clear_visit();
    for (auto inst : inst_list) {
        schedule_late(inst);
    }
}

} // namespace middleend