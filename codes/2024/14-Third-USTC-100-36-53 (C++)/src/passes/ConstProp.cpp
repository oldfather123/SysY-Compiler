#include "ConstProp.hpp"
#include "AnalysisPass.hpp"
#include "SimplifyCFG.hpp"

#define CONST_FP(num) ConstantFP::get((float)num, m_)
#define CONST_INT(num) ConstantInt::get(num, m_)

// TODO: this implementation is not efficient
void ConstProp::run(Function *func, AnalysisPassManager &APM) {
    flow_worklist_.emplace(nullptr, func->get_entry_block());

    for (auto &arg : func->get_args()) {
        lattice_map_[&arg].mark_unknown();
    }

    // 1. find all constant
    for (;;) {
        if (!flow_worklist_.empty()) {
            auto [pred, bb] = flow_worklist_.front();
            flow_worklist_.pop();

            if (is_executable(pred, bb))
                continue;

            is_executable_.emplace(pred, bb);
            for (auto &inst : bb->get_instructions()) {
                visit_inst(&inst);
            }
            continue;
        }

        if (!ssa_worklist_.empty()) {
            auto inst = ssa_worklist_.front();
            ssa_worklist_.pop();

            if (is_executable(inst->get_parent())) {
                visit_inst(inst);
            }
            continue;
        }

        break;
    }

    // 2. replace constant, trim dead code
    std::vector<BasicBlock *> bb_to_delete;

    for (auto &B : func->get_basic_blocks()) {
        auto bb = &B;

        if (!is_executable(bb)) {
            bb_to_delete.push_back(bb);
            continue;
        }

        std::vector<Instruction *> inst_to_delete;

        for (auto &inst : bb->get_instructions()) {
            auto cell = get_lattice_cell(&inst);
            if (cell.is_const()) {
                inst.replace_all_use_with(cell.get_const());
                inst_to_delete.push_back(&inst);
            }
        }

        for (auto inst : inst_to_delete) {
            bb->erase_instr(inst);
        }

        auto term = bb->get_terminator();
        if (auto br = dynamic_cast<BranchInst *>(term)) {
            if (!br->is_cond_br())
                continue;

            auto ops = br->get_operands();

            auto l = get_lattice_cell(ops[0]);
            if (!l.is_const())
                continue;

            auto cond = static_cast<ConstantInt *>(l.get_const());
            auto true_bb = static_cast<BasicBlock *>(ops[1]);
            auto false_bb = static_cast<BasicBlock *>(ops[2]);

            bb->erase_instr(br);
            if (cond->get_value()) {
                SimplifyCFG::remove_edge(bb, false_bb);
                bb->remove_succ_basic_block(true_bb);
                BranchInst::create_br(true_bb, bb);
            } else {
                SimplifyCFG::remove_edge(bb, true_bb);
                bb->remove_succ_basic_block(false_bb);
                BranchInst::create_br(false_bb, bb);
            }
        }
    }

    for (auto bb : bb_to_delete) {
        SimplifyCFG::remove_bb(bb);
    }

    APM.invalidateAll<Function>(func);
}

bool ConstProp::is_executable(BasicBlock *from, BasicBlock *to) {
    return is_executable_.find({from, to}) != is_executable_.end();
}

bool ConstProp::is_executable(BasicBlock *bb) {
    for (auto pred : bb->get_pre_basic_blocks()) {
        if (is_executable(pred, bb)) {
            return true;
        }
    }
    return bb == bb->get_parent()->get_entry_block();
}

LatticeCell ConstProp::get_lattice_cell(Value *v) {
    if (auto c = dynamic_cast<Constant *>(v))
        return LatticeCell(c);

    return lattice_map_[v];
}

void ConstProp::update(Value *val, LatticeCell c) {
    auto m = lattice_map_[val];
    m.merge(c);

    if (m == lattice_map_[val])
        return;

    lattice_map_[val] = m;
    for (auto &use : val->get_use_list()) {
        auto inst = dynamic_cast<Instruction *>(use.val_);
        assert(inst);
        ssa_worklist_.push(inst);
    }
}

void ConstProp::visit_inst(Instruction *inst) {
    switch (inst->get_instr_type()) {
    case Instruction::alloca:
    case Instruction::store:
    case Instruction::getelementptr:
    case Instruction::call:
        lattice_map_[inst].mark_unknown();
        break;

    // load const int/float
    case Instruction::load: {
        auto ptr = static_cast<Value *>(inst->get_operand(0));
        if (auto gbl = dynamic_cast<GlobalVariable *>(ptr)) {
            if (gbl->is_const() && !gbl->get_init()->get_type()->is_pointer_type()) {
                lattice_map_[inst].mark_const(gbl->get_init());
                break;
            }
        }

        lattice_map_[inst].mark_unknown();
        break;
    }

    case Instruction::ret:
        lattice_map_[inst].mark_unknown();
        break;

    case Instruction::br:
        lattice_map_[inst].mark_unknown();
        visit_br(static_cast<BranchInst *>(inst));
        break;

    case Instruction::phi:
        visit_phi(static_cast<PhiInst *>(inst));
        break;

    default: {
        LatticeCell lhs = get_lattice_cell(inst->get_operand(0));
        LatticeCell rhs;
        if (inst->get_num_operand() > 1) {
            rhs = get_lattice_cell(inst->get_operand(1));
        } else {
            rhs = LatticeCell(nullptr);
        }

        LatticeCell v;
        if (lhs.is_undef() || rhs.is_undef()) {
            v.mark_undef();
        } else if (lhs.is_unknown() || rhs.is_unknown()) {
            v.mark_unknown();
        } else {
            auto c = evaluate(inst, lhs.get_const(), rhs.get_const());
            v.mark_const(c);
        }
        update(inst, v);
    }
    }
}

void ConstProp::visit_phi(PhiInst *phi) {
    auto bb = phi->get_parent();
    LatticeCell m;

    for (size_t i = 0; i < phi->get_num_operand(); i += 2) {
        auto val = phi->get_operand(i);
        auto pred = static_cast<BasicBlock *>(phi->get_operand(i + 1));

        if (is_executable(pred, bb)) {
            m.merge(get_lattice_cell(val));
        }
    }

    update(phi, m);
}

void ConstProp::visit_br(BranchInst *br) {
    auto bb = br->get_parent();
    auto &ops = br->get_operands();

    if (!br->is_cond_br()) {
        auto succ = static_cast<BasicBlock *>(ops[0]);
        flow_worklist_.push({bb, succ});
        return;
    }

    auto cond = get_lattice_cell(ops[0]);
    if (cond.is_undef())
        return;

    auto true_bb = static_cast<BasicBlock *>(ops[1]);
    auto false_bb = static_cast<BasicBlock *>(ops[2]);

    if (cond.is_unknown()) {
        flow_worklist_.push({bb, true_bb});
        flow_worklist_.push({bb, false_bb});
        return;
    }

    auto cond_const = static_cast<ConstantInt *>(cond.get_const());
    if (cond_const->get_value()) {
        flow_worklist_.push({bb, true_bb});
    } else {
        flow_worklist_.push({bb, false_bb});
    }
}

Constant *ConstProp::evaluate(Instruction *inst, Constant *lhs, Constant *rhs) {
    auto m_ = inst->get_module();
    if (inst->is_zext()) {
        return CONST_INT(static_cast<ConstantInt *>(lhs)->get_value());
    }

    if (inst->is_si2fp()) {
        auto si = static_cast<ConstantInt *>(lhs)->get_value();
        return CONST_FP(static_cast<float>(si));
    }

    if (inst->is_fp2si()) {
        auto fp = static_cast<ConstantFP *>(lhs)->get_value();
        return CONST_INT(static_cast<int>(fp));
    }

    if (lhs->get_type()->is_int32_type()) {
        int l = static_cast<ConstantInt *>(lhs)->get_value();
        int r = static_cast<ConstantInt *>(rhs)->get_value();

        switch (inst->get_instr_type()) {
        case Instruction::add:
            return CONST_INT(l + r);
        case Instruction::sub:
            return CONST_INT(l - r);
        case Instruction::mul:
            return CONST_INT(l * r);
        case Instruction::sdiv:
            return CONST_INT(l / r);
        case Instruction::srem:
            return CONST_INT(l % r);

        case Instruction::ge:
            return CONST_INT(l >= r);
        case Instruction::gt:
            return CONST_INT(l > r);
        case Instruction::le:
            return CONST_INT(l <= r);
        case Instruction::lt:
            return CONST_INT(l < r);
        case Instruction::eq:
            return CONST_INT(l == r);
        case Instruction::ne:
            return CONST_INT(l != r);
        default:
            assert("unreachable" && false);
        }
    }

    if (lhs->get_type()->is_float_type()) {
        float l = static_cast<ConstantFP *>(lhs)->get_value();
        float r = static_cast<ConstantFP *>(rhs)->get_value();

        switch (inst->get_instr_type()) {
        case Instruction::fadd:
            return CONST_FP(l + r);
        case Instruction::fsub:
            return CONST_FP(l - r);
        case Instruction::fmul:
            return CONST_FP(l * r);
        case Instruction::fdiv:
            return CONST_FP(l / r);

        case Instruction::fge:
            return CONST_INT(l >= r);
        case Instruction::fgt:
            return CONST_INT(l > r);
        case Instruction::fle:
            return CONST_INT(l <= r);
        case Instruction::flt:
            return CONST_INT(l < r);
        case Instruction::feq:
            return CONST_INT(l == r);
        case Instruction::fne:
            return CONST_INT(l != r);
        default:
            assert("unreachable" && false);
        }
    }

    assert("unreachable" && false);
}
