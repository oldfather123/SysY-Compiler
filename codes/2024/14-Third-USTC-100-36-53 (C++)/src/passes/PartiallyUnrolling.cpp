#include "../../include/passes/PartiallyUnrolling.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "syntax_analyzer.h"
#include <algorithm>
#include <codecvt>
#include <list>
#include <optional>
#include <type_traits>

using namespace std;

void PartiallyUnrolling::run(Function *f, AnalysisPassManager &APM) {
    // loopdetection_ = std::make_unique<LoopDetection>(m_);
    // loopdetection_->run();
    auto loopdetection_ = APM.getResult<LoopDetection>(f);
    for (auto &loop : loopdetection_->get_loops())
        handle_loop(loop, f->get_parent());
    APM.invalidateAll<Function>(f);
}

std::optional<PartiallyUnrolling::SimpleLoopInfo>
PartiallyUnrolling::parse_simple_loop(const std::shared_ptr<Loop> loop) {
    SimpleLoopInfo ret;

    // parse header
    ret.header = loop->get_header();

    // parse bbs
    ret.bbs = loop->get_blocks();

    // parse preheader
    for (auto &pred : ret.header->get_pre_basic_blocks())
        if (std::find(ret.bbs.begin(), ret.bbs.end(), pred) == ret.bbs.end())
            ret.preheader = pred;

    // parse body
    ret.bodies.assign(ret.bbs.begin() + 1, ret.bbs.end());

    if (loop->get_sub_loops().size() > 0) {
        return nullopt;
    }

    if (loop->get_back_edges_nodes().size() > 1) {
        return nullopt;
    }

    // the loop has too many exiting edges
    if (loop->get_exits().size() > 1) {
        return nullopt;
    }

    // the exiting bb is not header
    if (loop->get_exits().begin()->first != ret.header) {
        return nullopt;
    }

    // parse exit
    ret.exit = loop->get_exits().begin()->second;

    // induction var
    auto br_inst =
        dynamic_cast<BranchInst *>(&ret.header->get_instructions().back());
    assert(br_inst->get_num_operand() == 3);
    auto cond = br_inst->get_operand(0);
    // the induction variable should be of int type
    if (not dynamic_cast<Instruction *>(cond)->is_cmp()) {
        return nullopt;
    }

    auto icmp_inst = dynamic_cast<ICmpInst *>(cond);
    ret.cond = icmp_inst;
    auto lhs = icmp_inst->get_operand(0);
    auto rhs = icmp_inst->get_operand(1);

    // at least one op should be mutable after constant folding
    assert(dynamic_cast<ConstantInt *>(lhs) == nullptr or
           dynamic_cast<ConstantInt *>(rhs) == nullptr);
    if (dynamic_cast<ConstantInt *>(lhs) != nullptr) {
        ret.ind_var = rhs;
        ret.bound = lhs;
    } else if (dynamic_cast<ConstantInt *>(rhs) != nullptr) {
        ret.ind_var = lhs;
        ret.bound = rhs;
    } else {
        return nullopt;
    }

    // find initial and step
    for (auto &&inst : ret.header->get_instructions()) {
        if (dynamic_cast<PhiInst *>(&inst) == nullptr)
            break;
        if (dynamic_cast<Value *>(&inst) != ret.ind_var)
            continue;
        // TODO
        // the phi for induction variable
        auto phi_inst = dynamic_cast<PhiInst *>(&inst);
        // the phi should has two source, one from preheader and the other from
        // loop body
        assert(phi_inst->get_pairs().size() == 2);
        for (auto [value, source] : phi_inst->get_pairs()) {
            if (std::find(loop->get_blocks().begin(), loop->get_blocks().end(),
                          source) != loop->get_blocks().end()) {
                // step
                if (dynamic_cast<IBinaryInst *>(value) == nullptr) {
                    continue;
                }
                auto ibinary_inst = dynamic_cast<IBinaryInst *>(value);
                auto ibin_op = ibinary_inst->get_instr_op_name();
                if (ibin_op != "add" && ibin_op != "sub") {
                    continue;
                }
                if (dynamic_cast<ConstantInt *>(
                        ibinary_inst->get_operand(0)) and
                    ibinary_inst->get_operand(1) == ret.ind_var) {
                    ret.step = ibinary_inst->get_operand(0);
                } else if (dynamic_cast<ConstantInt *>(
                               ibinary_inst->get_operand(1)) and
                           ibinary_inst->get_operand(0) == ret.ind_var) {
                    ret.step = ibinary_inst->get_operand(1);
                }
            } else {
                // initial
                if (dynamic_cast<ConstantInt *>(value) == nullptr) {
                    continue;
                }
                ret.initial = value;
            }
        }
    }
    if (ret.initial == nullptr or ret.step == nullptr) {
        return nullopt;
    }

    return ret;
}

bool PartiallyUnrolling::should_unroll(const SimpleLoopInfo &simple_loop) {
    int initial = dynamic_cast<ConstantInt *>(simple_loop.initial)->get_value();
    int step = dynamic_cast<ConstantInt *>(simple_loop.step)->get_value();
    int bound = dynamic_cast<ConstantInt *>(simple_loop.bound)->get_value();

    int estimate = (bound - initial) / step;

    return estimate >= UNROLL_TIMES;
}

Value *PartiallyUnrolling::find_map(
    Instruction *inst, int i, const SimpleLoopInfo &simple_loop,
    std::map<Instruction *, std::vector<Instruction *>> oldnew_inst) {
    assert(inst->is_phi() && "Illegal instruction type");
    PhiInst *phi_inst = dynamic_cast<PhiInst *>(inst);
    for (auto [value, source] : phi_inst->get_pairs())
        if (std::find(simple_loop.bbs.begin(), simple_loop.bbs.end(), source) !=
            simple_loop.bbs.end()) {
            if (auto inst_ = dynamic_cast<Instruction *>(value)) {
                auto parent_bb = inst_->get_parent();
                if (parent_bb == simple_loop.header && i > 1)
                    return find_map(inst_, i - 1, simple_loop, oldnew_inst);
                else
                    return oldnew_inst[inst_].at(i - 1);
            } else
                return value;
        }
    return nullptr;
}

void PartiallyUnrolling::unroll_simple_loop(const SimpleLoopInfo &simple_loop,
                                       Module *m_) {
    // 1.generate new header and bodies and copy instructions from old loop
    auto func = simple_loop.header->get_parent();
    std::map<BasicBlock *, std::vector<BasicBlock *>> oldnew_bb;
    std::map<Instruction *, std::vector<Instruction *>> oldnew_inst;

    // 1.1 generate new header and bodies, copy instructions
    BasicBlock *new_header = BasicBlock::create(m_, "", func);
    BasicBlock *bodies_entry;
    oldnew_bb[simple_loop.header].push_back(new_header);
    for (auto &inst : simple_loop.header->get_instructions()) {
        Instruction *new_inst =
            Instruction::clone_inst(&inst, new_header, false);
        oldnew_inst[&inst].push_back(new_inst);
    }
    std::vector<BasicBlock *> new_bodies;
    for (int i = 0; i < UNROLL_TIMES; i++) {
        for (auto bb = simple_loop.bodies.begin();
             bb != simple_loop.bodies.end(); bb++) {
            auto new_bb = BasicBlock::create(m_, "", func);
            new_bodies.push_back(new_bb);
            oldnew_bb[*bb].push_back(new_bb);
            for (auto &inst : (*bb)->get_instructions()) {
                Instruction *new_inst =
                    Instruction::clone_inst(&inst, new_bb, false);
                oldnew_inst[&inst].push_back(new_inst);
            }
        }
    }

    // 2.modify the operands of the new loop
    // 2.1 modify the operands of the new header
    for (auto &new_inst : new_header->get_instructions()) {
        for (unsigned i = 0; i < (&new_inst)->get_num_operand(); i++) {
            Value *op = (&new_inst)->get_operand(i);
            if (auto inst = dynamic_cast<Instruction *>(op)) {
                auto parent_bb = inst->get_parent();
                // the instrucion is in the header
                if (parent_bb == simple_loop.header && (&new_inst)->is_phi())
                    (&new_inst)->set_operand(i, find_map(inst, UNROLL_TIMES - 1,
                                                         simple_loop,
                                                         oldnew_inst));
                else if (std::find(simple_loop.bbs.begin(),
                                   simple_loop.bbs.end(),
                                   parent_bb) != simple_loop.bbs.end())
                    (&new_inst)->set_operand(i, oldnew_inst[inst].back());
            } else if (auto bb = dynamic_cast<BasicBlock *>(op)) {
                assert(((&new_inst)->is_phi() || (&new_inst)->is_br()) &&
                       "Illegal instruction type");
                if ((&new_inst)->is_phi()) {
                    if (std::find(simple_loop.bbs.begin(),
                                  simple_loop.bbs.end(),
                                  bb) != simple_loop.bbs.end())
                        (&new_inst)->set_operand(i, oldnew_bb[bb].back());
                } else {
                    if (std::find(simple_loop.bbs.begin(),
                                  simple_loop.bbs.end(),
                                  bb) != simple_loop.bbs.end()) {
                        bodies_entry = bb;
                        (&new_inst)->set_operand(i, oldnew_bb[bb].front());
                    } else if (bb == simple_loop.exit)
                        (&new_inst)->set_operand(i, simple_loop.header);
                }
            }
            if (op == simple_loop.bound &&
                (&new_inst) == oldnew_inst[simple_loop.cond].front()) {
                int initial = dynamic_cast<ConstantInt *>(simple_loop.initial)
                                  ->get_value();
                int step =
                    dynamic_cast<ConstantInt *>(simple_loop.step)->get_value();
                int bound =
                    dynamic_cast<ConstantInt *>(simple_loop.bound)->get_value();
                int new_bound = bound - (bound - initial) % (step * UNROLL_TIMES);
                switch((&new_inst)->get_instr_type())
                {
                    case Instruction::OpID::le: new_bound-=1; break;
                    case Instruction::OpID::ge: new_bound+=1; break;
                    default: break;
                }
                auto new_bound1 = ConstantInt::get(new_bound, m_);
                (&new_inst)->set_operand(i, new_bound1);
            }
        }
    }

    // 2.2 modify the operands of bodies
    for (int i = 0; i < UNROLL_TIMES; i++) {
        for (auto bb = simple_loop.bodies.begin();
             bb != simple_loop.bodies.end(); bb++) {
            auto new_bb = oldnew_bb[*bb].at(i);
            for (auto &new_inst : new_bb->get_instructions()) {
                for (unsigned j = 0; j < (&new_inst)->get_num_operand(); j++) {
                    auto op = (&new_inst)->get_operand(j);
                    if (auto inst = dynamic_cast<Instruction *>(op)) {
                        auto parent_bb = inst->get_parent();
                        // the instrucion is in the header
                        if (parent_bb == simple_loop.header) {
                            if (i == 0)
                                (&new_inst)->set_operand(
                                    j, oldnew_inst[inst].back());
                            else
                                (&new_inst)->set_operand(
                                    j, find_map(inst, i, simple_loop,
                                                oldnew_inst));
                        } else if (std::find(simple_loop.bodies.begin(),
                                             simple_loop.bodies.end(),
                                             parent_bb) !=
                                   simple_loop.bodies.end())
                            (&new_inst)->set_operand(j,
                                                     oldnew_inst[inst].at(i));
                    } else if (auto bb_ = dynamic_cast<BasicBlock *>(op)) {
                        assert(
                            ((&new_inst)->is_phi() || (&new_inst)->is_br()) &&
                            "Illegal instruction type");
                        if ((&new_inst)->is_phi())
                            (&new_inst)->set_operand(j, oldnew_bb[bb_].at(i));
                        else {
                            if (bb_ != simple_loop.header)
                                (&new_inst)->set_operand(j,
                                                         oldnew_bb[bb_].at(i));
                            else {
                                if (i < UNROLL_TIMES - 1)
                                    (&new_inst)->set_operand(
                                        j, oldnew_bb[bodies_entry].at(i + 1));
                                else
                                    (&new_inst)->set_operand(j, new_header);
                            }
                        }
                    }
                }
            }
        }
    }
    // 3.modify the operands of the old loop and the preheader
    // 3.1 modify the operands of the preheader
    for (auto &inst : simple_loop.preheader->get_instructions())
        if ((&inst)->is_br()) {
            for (unsigned i = 0; i < (&inst)->get_num_operand(); i++) {
                auto op = (&inst)->get_operand(i);
                if (dynamic_cast<BasicBlock *>(op) == simple_loop.header)
                    (&inst)->set_operand(i, new_header);
            }
        }
    // 3.2 modify the operands of the old loop
    for (auto &inst : simple_loop.header->get_instructions())
        if ((&inst)->is_phi()) {
            auto new_inst = oldnew_inst[&inst].back();
            auto phi_inst = dynamic_cast<PhiInst *>(&inst);
            phi_inst->remove_phi_operand(simple_loop.preheader);
            phi_inst->add_phi_pair_operand(new_inst, new_header);
        }

    // 4.maintain the relation of blocks
    // 4.1 modify the succ_bb of preheader
    simple_loop.preheader->remove_succ_basic_block(simple_loop.header);
    if (std::find(simple_loop.preheader->get_succ_basic_blocks().begin(),
                  simple_loop.preheader->get_succ_basic_blocks().end(),
                  new_header) ==
        simple_loop.preheader->get_succ_basic_blocks().end())
        simple_loop.preheader->add_succ_basic_block(new_header);
    // 4.2 modify the pre_bb and succ_bb of new_header
    for (auto &new_inst : new_header->get_instructions()) {
        if ((&new_inst)->is_phi())
            for (unsigned i = 1; i < (&new_inst)->get_num_operand(); i += 2) {
                auto bb =
                    dynamic_cast<BasicBlock *>((&new_inst)->get_operand(i));
                if (std::find(new_header->get_pre_basic_blocks().begin(),
                              new_header->get_pre_basic_blocks().end(),
                              bb) == new_header->get_pre_basic_blocks().end())
                    new_header->add_pre_basic_block(bb);
            }
        else
            break;
    }
    BranchInst *br_inst =
        dynamic_cast<BranchInst *>(&new_header->get_instructions().back());
    if (br_inst) {
        auto succ_bbs = new_header->get_succ_basic_blocks();
        for (auto succ_bb : succ_bbs)
            if (std::find(br_inst->get_operands().begin(),
                          br_inst->get_operands().end(),
                          dynamic_cast<Value *>(succ_bb)) ==
                br_inst->get_operands().end()) {
                new_header->remove_succ_basic_block(succ_bb);
                succ_bb->remove_pre_basic_block(new_header);
            }
        for (unsigned i = 0; i < br_inst->get_num_operand(); i++) {
            auto bb = dynamic_cast<BasicBlock *>(br_inst->get_operand(i));
            if (bb) {
                if (std::find(new_header->get_succ_basic_blocks().begin(),
                              new_header->get_succ_basic_blocks().end(),
                              bb) == new_header->get_succ_basic_blocks().end())
                    new_header->add_succ_basic_block(bb);
                if (std::find(bb->get_pre_basic_blocks().begin(),
                              bb->get_pre_basic_blocks().end(),
                              new_header) == bb->get_pre_basic_blocks().end())
                    bb->add_pre_basic_block(new_header);
            }
        }
    }
    // 4.3 modify the pre_bb and succ_bb of new_bodies
    for (int i = 0; i < UNROLL_TIMES; i++) {
        for (auto bb = simple_loop.bodies.begin();
             bb != simple_loop.bodies.end(); bb++) {
            auto new_bb = oldnew_bb[*bb].at(i);
            BranchInst *br_inst =
                dynamic_cast<BranchInst *>(&new_bb->get_instructions().back());
            if (br_inst) {
                auto succ_bbs = new_bb->get_succ_basic_blocks();
                for (auto succ_bb : succ_bbs)
                    if (std::find(br_inst->get_operands().begin(),
                                  br_inst->get_operands().end(),
                                  succ_bb) == br_inst->get_operands().end()) {
                        new_bb->remove_succ_basic_block(succ_bb);
                        succ_bb->remove_pre_basic_block(new_bb);
                    }
            }
            for (unsigned i = 0; i < br_inst->get_num_operand(); i++) {
                auto bb_ = dynamic_cast<BasicBlock *>(br_inst->get_operand(i));
                if (bb_) {
                    if (std::find(new_bb->get_succ_basic_blocks().begin(),
                                  new_bb->get_succ_basic_blocks().end(),
                                  bb_) == new_bb->get_succ_basic_blocks().end())
                        new_bb->add_succ_basic_block(bb_);
                    if (std::find(bb_->get_pre_basic_blocks().begin(),
                                  bb_->get_pre_basic_blocks().end(),
                                  new_bb) == bb_->get_pre_basic_blocks().end())
                        bb_->add_pre_basic_block(new_bb);
                }
            }
        }
    }
    // 4.4 modify the pre_bb of header
    simple_loop.header->remove_pre_basic_block(simple_loop.preheader);
    // simple_loop.header->add_pre_basic_block(new_header);
    //  ? 5.add the new loop information into loop_detection
}

void PartiallyUnrolling::handle_loop(std::shared_ptr<Loop> loop, Module *m_) {
    auto simple_loop = parse_simple_loop(loop);
    if (not simple_loop.has_value())
        return;
    if (not should_unroll(simple_loop.value()))
        return;
    unroll_simple_loop(simple_loop.value(), m_);
}