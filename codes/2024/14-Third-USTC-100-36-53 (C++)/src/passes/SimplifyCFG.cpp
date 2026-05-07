#include "SimplifyCFG.hpp"
#include <algorithm>

void SimplifyCFG::run(Function *f, AnalysisPassManager &APM) {
    while (remove_dead_blocks(f) || merge_into_single_predecessor(f)
           /* || remove_consecutive_jump(&f)*/)
        ;
    APM.invalidateAll<Function>(f);
}

bool SimplifyCFG::remove_dead_blocks(Function *f) {
    for (auto &BB : f->get_basic_blocks()) {
        auto bb = &BB;
        if (bb == f->get_entry_block() || !bb->get_pre_basic_blocks().empty())
            continue;

        remove_bb(bb);
        return true;
    }

    return false;
}

bool SimplifyCFG::merge_into_single_predecessor(Function *f) {
    for (auto &BB : f->get_basic_blocks()) {
        auto bb = &BB;

        auto &preds = bb->get_pre_basic_blocks();
        if (!(preds.size() == 1 &&
              preds.front()->get_succ_basic_blocks().size() == 1))
            continue;

        auto pred = preds.front();
        assert(pred != bb);

        pred->erase_instr(pred->get_terminator());

        auto &insts = bb->get_instructions();
        while (!insts.empty()) {
            auto &inst = insts.front();
            if (!inst.is_phi())
                break;

            assert(inst.get_num_operand() == 2);
            inst.replace_all_use_with(inst.get_operand(0));
            insts.pop_front();
        }

        while (!insts.empty()) {
            auto &inst = insts.front();
            insts.remove(&inst); // not erase

            // inst.set_parent(pred);
            pred->add_instruction(&inst);
        }

        pred->remove_succ_basic_block(bb);
        for (auto succ : bb->get_succ_basic_blocks()) {
            succ->remove_pre_basic_block(bb);
            succ->add_pre_basic_block(pred);
            pred->add_succ_basic_block(succ);
        }

        bb->replace_all_use_with(pred);
        bb->erase_from_parent();
        return true;
    }

    return false;
}

void SimplifyCFG::remove_dead_phi(BasicBlock *bb1, BasicBlock *bb2) {
    std::vector<PhiInst *> phi_to_delete;

    for (auto &inst : bb2->get_instructions()) {
        auto phi = dynamic_cast<PhiInst *>(&inst);
        if (!phi)
            break;

        auto &ops = phi->get_operands();
        auto it = std::find(ops.begin(), ops.end(), bb1);
        if (it == ops.end())
            continue;

        auto i = std::distance(ops.begin(), it);
        phi->remove_operand(i);
        phi->remove_operand(i - 1);

        if (phi->get_num_operand() == 2) {
            phi->replace_all_use_with(phi->get_operand(0));
            phi_to_delete.push_back(phi);
        }
    }

    for (auto phi : phi_to_delete) {
        bb2->erase_instr(phi);
    }
}

void SimplifyCFG::remove_edge(BasicBlock *bb1, BasicBlock *bb2) {
    bb1->remove_succ_basic_block(bb2);
    bb2->remove_pre_basic_block(bb1);

    remove_dead_phi(bb1, bb2);
}

void SimplifyCFG::remove_bb(BasicBlock *bb) {
    for (auto succ : bb->get_succ_basic_blocks()) {
        remove_dead_phi(bb, succ);
    }

    bb->erase_from_parent();
}
