#include "PeepholeOptimization.hpp"
#include "MachineFunction.hpp"
#include "MachineInstr.hpp"
#include "Operand.hpp"
#include <memory>

bool PeepholeOptimization::check_const(std::shared_ptr<MachineInstr> inst,
                                       int &imm) {
    if ((inst->get_tag() == MachineInstr::Tag::ADDI ||
         inst->get_tag() == MachineInstr::Tag::ORI) &&
        inst->get_operand(1) == PhysicalRegister::zero()) {
        imm = std::dynamic_pointer_cast<Immediate>(inst->get_operand(2))
                  ->get_value();
        return true;
    }
    if (inst->get_tag() == MachineInstr::Tag::LI) {
        imm = std::dynamic_pointer_cast<Immediate>(inst->get_operand(1))
                  ->get_value();
        return true;
    }
    return false;
}

template <typename T>
inst_it PeepholeOptimization::find_pre_inst(inst_it inst, T pred) {
    if (inst == mbb->get_instrs_begin())
        return mbb->get_instrs_end();

    do {
        --inst;
        if (pred(*inst))
            return inst;
    } while (inst != mbb->get_instrs_begin());
    return mbb->get_instrs_end();
}

inst_it PeepholeOptimization::find_def(inst_it it,
                                       std::shared_ptr<Operand> reg) {
    inst_it def_it = find_pre_inst(it, [reg](auto inst) {
        return inst->has_dst() && inst->get_dst() == reg;
    });

    while (def_it != mbb->get_instrs_end()) {
        if ((*def_it)->get_tag() != MachineInstr::Tag::MOV)
            break;
        auto dst = (*def_it)->get_operand(1);
        def_it = find_pre_inst(def_it, [dst](auto inst) {
            return inst->has_dst() && inst->get_dst() == dst;
        });
    }
    return def_it;
}

void PeepholeOptimization::opt_mul(inst_it &inst_) {
    auto inst = *inst_;
    if (inst->get_tag() != MachineInstr::Tag::MUL)
        return;
    auto inst2_ =
        find_pre_inst(inst_, [inst](std::shared_ptr<MachineInstr> inst_x) {
            return inst_x->has_dst() &&
                   inst_x->get_dst() == inst->get_operand(1);
        });
    auto inst3_ =
        find_pre_inst(inst_, [inst](std::shared_ptr<MachineInstr> inst_x) {
            return inst_x->has_dst() &&
                   inst_x->get_dst() == inst->get_operand(2);
        });

    int imm = 0;
    if (inst2_ == inst3_) {
        if (inst2_ == mbb->get_instrs_end())
            return;
        auto inst2 = *inst2_;
        if (check_const(inst2, imm)) {
            inst_ = mbb->erase_instr(inst_);
            builder->set_insert_point(mbb, inst_);
            builder->load_int32(imm * imm, inst->get_dst());
        }
    } else if (inst2_ != mbb->get_instrs_end() && check_const(*inst2_, imm) &&
               (imm & (imm - 1)) == 0) {
        if (imm == 0) {
            inst_ = mbb->erase_instr(inst_);
            builder->set_insert_point(mbb, inst_);
            builder->load_int32(0, inst->get_dst());
            return;
        }
        auto inst2 = *inst2_;
        int bits = __builtin_ffs(imm) - 1;
        inst_ = mbb->erase_instr(inst_);
        builder->set_insert_point(mbb, inst_);
        builder->insert_instr(
            MachineInstr::Tag::SLLI,
            {inst->get_dst(), inst->get_operand(2), Immediate::create(bits)},
            inst->get_suffix());
    } else if (inst3_ != mbb->get_instrs_end() && check_const(*inst3_, imm) &&
               (imm & (imm - 1)) == 0) {
        if (imm == 0) {
            inst_ = mbb->erase_instr(inst_);
            builder->set_insert_point(mbb, inst_);
            builder->load_int32(0, inst->get_dst());
            return;
        }
        auto inst3 = *inst3_;
        int bits = __builtin_ffs(imm) - 1;
        inst_ = mbb->erase_instr(inst_);
        builder->set_insert_point(mbb, inst_);
        builder->insert_instr(
            MachineInstr::Tag::SLLI,
            {inst->get_dst(), inst->get_operand(1), Immediate::create(bits)},
            inst->get_suffix());
    }
}

void PeepholeOptimization::opt_div(inst_it &inst_) {
    auto inst = *inst_;
    if (inst->get_tag() != MachineInstr::Tag::DIV)
        return;
    auto inst2_ =
        find_pre_inst(inst_, [inst](std::shared_ptr<MachineInstr> inst_x) {
            return inst_x->has_dst() &&
                   inst_x->get_dst() == inst->get_operand(2);
        });
    if (inst2_ == mbb->get_instrs_end())
        return;
    auto inst2 = *inst2_;

    int imm = 0;
    if (check_const(inst2, imm) && (imm & (imm - 1)) == 0) {
        int bits = __builtin_ffs(imm) - 1;
        inst_ = mbb->erase_instr(inst_);
        builder->set_insert_point(mbb, inst_);
        builder->insert_instr(
            MachineInstr::Tag::SRAI,
            {inst->get_dst(), inst->get_operand(1), Immediate::create(bits)},
            inst->get_suffix());
    }
}

void PeepholeOptimization::opt_rem(inst_it &inst_) {
    auto inst = *inst_;
    if (inst->get_tag() != MachineInstr::Tag::REM)
        return;
    auto inst2_ =
        find_pre_inst(inst_, [inst](std::shared_ptr<MachineInstr> inst_x) {
            return inst_x->has_dst() &&
                   inst_x->get_dst() == inst->get_operand(2);
        });

    if (inst2_ == mbb->get_instrs_end())
        return;
    auto inst2 = *inst2_;

    int imm = 0;
    if (check_const(inst2, imm) && (imm & (imm - 1)) == 0 && imm > 0) {
        inst_ = mbb->erase_instr(inst_);
        builder->set_insert_point(mbb, inst_);

        if (imm < 2047) {
            builder->insert_instr(MachineInstr::Tag::ANDI,
                                  {inst->get_dst(), inst->get_operand(1),
                                   Immediate::create(imm - 1)});
        } else {
            builder->insert_instr(
                MachineInstr::Tag::AND,
                {inst->get_dst(), inst->get_operand(1), inst2->get_dst()});
        }
    }
}

void PeepholeOptimization::opt_ld(inst_it &inst_) {
    auto inst = *inst_;
    if (inst->get_tag() != MachineInstr::Tag::LD &&
        inst->get_tag() != MachineInstr::Tag::FLW)
        return;

    auto imm = std::static_pointer_cast<Immediate>(inst->get_operand(2));
    if (imm->get_value() != 0)
        return;

    auto inst2_ = find_def(inst_, inst->get_operand(1));
    if (inst2_ == mbb->get_instrs_end())
        return;
    auto inst2 = *inst2_;

    if (inst2->get_tag() == MachineInstr::Tag::ADDI) {
        inst_ = mbb->erase_instr(inst_);
        builder->set_insert_point(mbb, inst_);
        builder->insert_instr(
            inst->get_tag(),
            {inst->get_dst(), inst2->get_operand(1), inst2->get_operand(2)},
            inst->get_suffix());
    }
}

void PeepholeOptimization::opt_st(inst_it &inst_) {
    auto inst = *inst_;
    if (inst->get_tag() != MachineInstr::Tag::ST)
        return;

    auto imm = std::static_pointer_cast<Immediate>(inst->get_operand(2));
    if (imm->get_value() != 0)
        return;

    auto inst2_ = find_def(inst_, inst->get_operand(1));
    if (inst2_ == mbb->get_instrs_end())
        return;
    auto inst2 = *inst2_;

    if (inst2->get_tag() == MachineInstr::Tag::ADDI) {
        inst_ = mbb->erase_instr(inst_);
        builder->set_insert_point(mbb, inst_);
        builder->insert_instr(inst->get_tag(),
                              {inst->get_operand(0), inst2->get_operand(1),
                               inst2->get_operand(2)},
                              inst->get_suffix());
    }
}

void PeepholeOptimization::opt_add(inst_it &inst_) {
    auto inst = *inst_;
    if (inst->get_tag() != MachineInstr::Tag::ADD)
        return;
    auto inst2_ =
        find_pre_inst(inst_, [inst](std::shared_ptr<MachineInstr> inst_x) {
            return inst_x->has_dst() &&
                   inst_x->get_dst() == inst->get_operand(1);
        });
    auto inst3_ =
        find_pre_inst(inst_, [inst](std::shared_ptr<MachineInstr> inst_x) {
            return inst_x->has_dst() &&
                   inst_x->get_dst() == inst->get_operand(2);
        });

    int imm = 0;
    if (inst2_ == inst3_) {
        if (inst2_ == mbb->get_instrs_end())
            return;
        auto inst2 = *inst2_;
        if (check_const(inst2, imm)) {
            inst_ = mbb->erase_instr(inst_);
            builder->set_insert_point(mbb, inst_);
            builder->load_int32(2 * imm, inst->get_dst());
        }
    } else if (inst2_ != mbb->get_instrs_end() && check_const(*inst2_, imm)) {
        if (inst->get_suffix() == MachineInstr::Suffix::WORD and
            (imm > 2047 or imm < -2048))
            return;
        auto inst2 = *inst2_;
        inst_ = mbb->erase_instr(inst_);
        builder->set_insert_point(mbb, inst_);
        builder->insert_instr(
            MachineInstr::Tag::ADDI,
            {inst->get_dst(), inst->get_operand(2), Immediate::create(imm)},
            inst->get_suffix());
    } else if (inst3_ != mbb->get_instrs_end() && check_const(*inst3_, imm)) {
        if (inst->get_suffix() == MachineInstr::Suffix::WORD and
            (imm > 2047 or imm < -2048))
            return;
        auto inst3 = *inst3_;
        inst_ = mbb->erase_instr(inst_);
        builder->set_insert_point(mbb, inst_);
        builder->add_int_to_reg(
            std::dynamic_pointer_cast<Register>(inst->get_dst()),
            std::dynamic_pointer_cast<Register>(inst->get_operand(1)), imm);
    }
}

void PeepholeOptimization::remove_redundant_jump(
    const std::shared_ptr<MachineFunction> &mfunc) {
    const auto &blocks = mfunc->get_basic_blocks();
    if (blocks.empty())
        return;

    using T = MachineInstr::Tag;
    using S = MachineInstr::Suffix;

    auto get_rev_op = [](MachineInstr::Tag tag) {
        switch (tag) {
        default:
            assert(false && "not a cond br");
        case T::BEQZ:
            return T::BNEZ;
        case T::BNEZ:
            return T::BEQZ;
        case T::BEQ:
            return T::BNE;
        case T::BNE:
            return T::BEQ;
        case T::BGE:
            return T::BLT;
        case T::BLT:
            return T::BGE;
        case T::BGT:
            return T::BLE;
        case T::BLE:
            return T::BGT;
        }
    };

    for (size_t i = 0; i < blocks.size() - 1; ++i) {
        auto bb = blocks[i];
        auto &instrs = bb->get_instrs();

        auto term = instrs.back();
        if (term->get_tag() != T::J)
            continue;

        auto dst = std::static_pointer_cast<Label>(term->get_operand(0));
        if (dst->get_name() == blocks[i + 1]->get_name()) {
            instrs.pop_back();
            continue;
        }

        if (instrs.size() == 1)
            continue;

        auto term2 = *std::next(instrs.rbegin());
        switch (auto tag2 = term2->get_tag()) {
        default:
            break;
        case T::BNEZ:
        case T::BEQZ: {
            auto dst2 = std::static_pointer_cast<Label>(term2->get_operand(1));
            if (dst2->get_name() == blocks[i + 1]->get_name()) {
                instrs.pop_back();
                instrs.pop_back();
                auto rev_tag = get_rev_op(tag2);
                auto val = term2->get_operand(0);
                auto new_br = std::shared_ptr<MachineInstr>(
                    new MachineInstr(bb, rev_tag, {val, dst}, S::NONE, 0U));
                instrs.push_back(new_br);
            }
            break;
        }
        case T::BEQ:
        case T::BNE:
        case T::BGE:
        case T::BLT:
        case T::BGT:
        case T::BLE: {
            auto dst2 = std::static_pointer_cast<Label>(term2->get_operand(2));
            if (dst2->get_name() == blocks[i + 1]->get_name()) {
                instrs.pop_back();
                instrs.pop_back();
                auto rev_tag = get_rev_op(tag2);
                auto lhs = term2->get_operand(0);
                auto rhs = term2->get_operand(1);
                auto new_br = std::shared_ptr<MachineInstr>(new MachineInstr(
                    bb, rev_tag, {lhs, rhs, dst}, S::NONE, 0U));
                instrs.push_back(new_br);
            }
            break;
        }
        }
    }
}

void PeepholeOptimization::run_on_func(std::shared_ptr<MachineFunction> func) {
#ifndef MAX_ITER
#define MAX_ITER 100000
#endif
    for (auto &mbb_ : func->get_basic_blocks()) {
        mbb = mbb_;
        int cnt = 0;
        while (true && ++cnt < MAX_ITER) {
            bool changed = false;
            for (auto inst = mbb->get_instrs_begin();
                 inst != mbb->get_instrs_end(); inst++) {
                opt_add(inst);
                opt_mul(inst);
                // opt_div(inst);
                opt_rem(inst);
                opt_ld(inst);
                // opt_st(inst);
            }
            if (!changed) {
                break;
            }
        }
    }
#undef MAX_ITER

    remove_redundant_jump(func);
}

void PeepholeOptimization::run() {
    for (auto &func : module->get_functions()) {
        run_on_func(func);
    }
}
