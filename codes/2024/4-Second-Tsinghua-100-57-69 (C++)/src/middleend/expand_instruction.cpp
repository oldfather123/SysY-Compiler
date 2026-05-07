#include "middleend/expand_instruction.hpp"

namespace middleend {

void ExpandInstruction::max_min() {
    std::unordered_map<ir::BasicBlock *, ir::BasicBlock *> empty_bb_to;
    for (auto bb : *func_->get_basic_blocks()) {
        if (bb->get_instructions()->size() == 1) {
            TypeCase(branch, ir::instruction::Branch *, bb->get_instructions()->front()) {
                empty_bb_to[bb] = branch->get_bb();
            }
        }
    }

    for (auto bb : *func_->get_basic_blocks()) {
        auto &inst = bb->get_instructions()->back();
        TypeCase(condbranch, ir::instruction::CondBranch *, inst) {
            bool is_min = true;
            if (condbranch->get_type() == ir::instruction::CondBranch::IRInstrCondBranch::IR_Instr_BNE) {
                is_min = !is_min;
            }
            ir::BasicBlock *branch_bb;
            ir::BasicBlock *target_bb;
            bool bb_pattern = false;
            if (empty_bb_to.count(condbranch->get_true_bb())) {
                target_bb = condbranch->get_false_bb();
                branch_bb = condbranch->get_true_bb();
                if (empty_bb_to[branch_bb] == target_bb) {
                    bb_pattern = true;
                }
            } else if (empty_bb_to.count(condbranch->get_false_bb())) {
                target_bb = condbranch->get_true_bb();
                branch_bb = condbranch->get_false_bb();
                if (empty_bb_to[branch_bb] == target_bb) {
                    bb_pattern = true;
                }
                is_min = !is_min;
            }
            if (!bb_pattern) {
                continue;
            }
            if (cfg_->get_bb_prev_map()->at(branch_bb->get_index()).size() != 1) {
                continue;
            }
            auto cond_inst = *uda_->get_defset(condbranch->getcond()).begin();
            Temp *lhs, *rhs, *dst;
            TypeCase(binary, ir::instruction::Binary *, cond_inst) {
                if (binary->get_type() == BinaryOp::Lt || binary->get_type() == BinaryOp::Leq) { // TODO: add more binary op
                    lhs = binary->getlhs();
                    rhs = binary->getrhs();
                } else if (binary->get_type() == BinaryOp::Gt || binary->get_type() == BinaryOp::Geq) {
                    lhs = binary->getlhs();
                    rhs = binary->getrhs();
                    is_min = !is_min;
                } else {
                    continue;
                }
            } else {
                continue;
            }
            auto &target_phi = target_bb->get_instructions()->front();
            TypeCase(phi, ir::instruction::Phi *, target_phi) {
                dst = phi->getdst();
                if (phi->getbbs()->size() != 2) {
                    continue;
                }
                if (std::find(phi->get_src()->begin(), phi->get_src()->end(), lhs) == phi->get_src()->end()) {
                    continue;
                }
                if (std::find(phi->get_src()->begin(), phi->get_src()->end(), rhs) == phi->get_src()->end()) {
                    continue;
                }
                if (std::find(phi->getbbs()->begin(), phi->getbbs()->end(), branch_bb) == phi->getbbs()->end()) {
                    continue;
                }
                if (std::find(phi->getbbs()->begin(), phi->getbbs()->end(), bb) == phi->getbbs()->end()) {
                    continue;
                }
                if (phi->get_src_temp(bb) == lhs) {
                    assert(phi->get_src_temp(branch_bb) == rhs);
                    is_min = !is_min;
                } else {
                    assert(phi->get_src_temp(bb) == rhs);
                    assert(phi->get_src_temp(branch_bb) == lhs);
                }
            } else {
                continue;
            }
            TypeCase(phi, ir::instruction::Phi *, *(target_bb->get_instructions()->begin() + 1)) {
                continue;
            }
            inst = new ir::instruction::Branch(target_bb);
            delete condbranch;
            delete target_phi;
            if (is_min) {
                target_phi = new ir::instruction::Minimum(dst, lhs, rhs);
            } else {
                target_phi = new ir::instruction::Maximum(dst, lhs, rhs);
            }
        }
    }
}

void ExpandInstruction::run() {
    max_min();
}

} // namespace middleend