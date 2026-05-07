#include "LoopRotate.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Dominators.hpp"
#include "Function.hpp"
#include "Instruction.hpp"

#include "LoopDetection.hpp"
#include "User.hpp"
#include "Value.hpp"
#include <algorithm>
#include <cassert>
#include <csetjmp>
#include <map>
#include <vector>

void LoopRotate::clear_phi(Loop *loop) {
    for (auto bbs : loop->get_blocks()) {
        std::vector<Instruction *> worklist;
        if (bbs->get_pre_basic_blocks().size() > 1)
            continue;
        for (auto &inst : bbs->get_instructions()) {
            if (inst.is_phi() and inst.get_num_operand() == 2) {
                worklist.push_back(&inst);
            }
        }
        for (auto &inst : worklist) {
            auto val = inst->get_operand(0);
            inst->replace_all_use_with(val);
            bbs->get_instructions().erase(inst);
            val->remove_use(inst, 0);
        }
    }
}

void LoopRotate::rotate_loop(Loop *loop, AnalysisPassManager &APM) {
    BasicBlock *header = loop->get_header();
    BasicBlock *preheader = loop->get_preheader();

    assert(header->get_terminator()->get_num_operand() == 3);
    auto body_entry =
        dynamic_cast<BasicBlock *>(header->get_terminator()->get_operand(1));
    auto header_exit =
        dynamic_cast<BasicBlock *>(header->get_terminator()->get_operand(2));
    auto dom = APM.getResult<Dominators>(header->get_parent());
    if (dom->is_dominate(header, header_exit) &&
        dom->is_dominate(header_exit, *loop->get_back_edges_nodes().begin())) {
        auto temp = body_entry;
        body_entry = header_exit;
        header_exit = temp;
    }
    assert(body_entry->get_pre_basic_blocks().size() == 1);
    for (auto &pre : header_exit->get_pre_basic_blocks()) {
        if (pre == header)
            continue;
        else {
            if (std::find(loop->get_blocks().begin(), loop->get_blocks().end(),
                          pre) != loop->get_blocks().end())
                return;
        }
    }
    // assume that loop has only one backedge node
    auto backedge = *loop->get_back_edges_nodes().begin();
    // clone header as rotate header
    BasicBlock *rotated_header =
        BasicBlock::create(header->get_module(), "", header->get_parent());
    std::map<Value *, Value *>
        old2new; // header中的指令到rotated_header的指令,对于phi，old为preheader的，new为backedge的
    std::map<Value *, Value *>
        old2phibody; // header指令到phibody，用于新建退出块的情况
    std::vector<Instruction *> move_list;
    for (auto &inst1 : header->get_instructions()) {
        auto inst = &inst1;

        // deal with phi
        if (inst->is_phi()) {
            // for body_entry
            if (inst->get_num_operand() < 4) {
                if (inst->get_operand(1) == backedge) {
                    inst->replace_use_with_if(
                        inst->get_operand(0), [&](const Use &use) -> bool {
                            auto user_inst =
                                dynamic_cast<Instruction *>(use.val_);
                            auto user_bb = user_inst->get_parent();
                            if (dom->is_dominate(header_exit, user_bb)) {
                                return true;
                            }
                            return false;
                        });
                    move_list.push_back(inst);
                    // 为移动指令做准备
                    inst->replace_operand(backedge, rotated_header);
                    continue;
                }
            }

            for (int i = 1; i < inst->get_num_operand(); i += 2) {
                if (inst->get_operand(i) == backedge) {
                    auto body_val = inst->get_operand(i - 1);
                    // new_inst->add_phi_pair_operand(body_val, rotated_header);
                    old2new[inst] = body_val;
                    inst->remove_operand(i - 1);
                    inst->remove_operand(i - 1);
                    i -= 2;
                }
            }
        } else {
            auto new_inst =
                Instruction::clone_inst(inst, rotated_header, false);
            old2new[inst] = new_inst;
            new_inst->set_operand_for_each_if(
                [&](Value *op) -> std::pair<bool, Value *> {
                    if (old2new.find(op) != old2new.end() and
                        (dynamic_cast<BasicBlock *>(op) != nullptr or
                         dynamic_cast<Instruction *>(op) != nullptr or
                         dynamic_cast<Argument *>(op) != nullptr))
                        return {true, old2new[op]};
                    else
                        return {false, nullptr};
                });
        }
    }
    for (auto &inst : move_list) {
        header->remove_instr(inst);
        body_entry->add_instr_begin(inst);
    }
    BasicBlock *oldexit = nullptr;
    // 退出块前驱大于1，在经过循环简化之后说明这些前驱全部来自循环内部
    if (header_exit->get_pre_basic_blocks().size() > 2) {
        // 创建一个新的退出块，前驱为新旧header
        BasicBlock *new_exit = BasicBlock::create(header_exit->get_module(), "",
                                                  header_exit->get_parent());
        header->get_terminator()->replace_operand(header_exit, new_exit);
        rotated_header->get_terminator()->replace_operand(header_exit,
                                                          new_exit);
        BranchInst::create_br(header_exit, new_exit);
        // phi
        for (auto &&inst : header_exit->get_instructions()) {
            if (not inst.is_phi()) {
                break;
            }
            auto phi = dynamic_cast<PhiInst *>(&inst);
            for (size_t i = 0; i < phi->get_num_operand(); i++) {
                if (phi->get_operand(i) == header) {
                    phi->set_operand(i, new_exit);
                }
            }
        }
        header->remove_succ_basic_block(header_exit);
        header_exit->remove_pre_basic_block(header);
        header->add_succ_basic_block(new_exit);
        new_exit->add_pre_basic_block(header);

        rotated_header->remove_succ_basic_block(header_exit);
        header_exit->remove_pre_basic_block(rotated_header);
        rotated_header->add_succ_basic_block(new_exit);
        new_exit->add_pre_basic_block(rotated_header);
        oldexit = header_exit;
        header_exit = new_exit;
    }

    // update predecessors and successors
    backedge->get_terminator()->replace_operand(header, rotated_header);
    backedge->remove_succ_basic_block(header);
    backedge->add_succ_basic_block(rotated_header);
    header->remove_pre_basic_block(backedge);
    rotated_header->remove_pre_basic_block(preheader);
    rotated_header->add_pre_basic_block(backedge);

    //  create phi for each inst of header in exit and body entry because now
    //  they has two prebb
    APM.invalidate<Dominators>(header->get_parent());
    auto dominator = APM.getResult<Dominators>(header->get_parent());
    for (auto &valpair : old2new) {
        auto phi_exit =
            PhiInst::create_phi(valpair.first->get_type(), header_exit);
        phi_exit->add_phi_pair_operand(valpair.first, header);
        phi_exit->add_phi_pair_operand(valpair.second, rotated_header);
        header_exit->add_instr_begin(phi_exit);
        valpair.first->replace_use_with_if(
            phi_exit, [&](const Use &use) -> bool {
                auto user_inst = dynamic_cast<Instruction *>(use.val_);
                auto user_bb = user_inst->get_parent();
                if (dynamic_cast<PhiInst *>(use.val_) != phi_exit) {
                    if (oldexit != nullptr) {
                        if (dominator->is_dominate(oldexit, user_bb))
                            return true;
                        else if (user_inst->is_phi()) {
                            auto srcbb = dynamic_cast<BasicBlock *>(
                                user_inst->get_operand(use.arg_no_ + 1));
                            if (dominator->is_dominate(oldexit, srcbb))
                                return true;
                        }
                        return false;
                    }
                    if (dominator->is_dominate(header_exit, user_bb))
                        return true;
                    else if (user_inst->is_phi()) {
                        auto srcbb = dynamic_cast<BasicBlock *>(
                            user_inst->get_operand(use.arg_no_ + 1));
                        if (dominator->is_dominate(header_exit, srcbb))
                            return true;
                    }
                }
                return false;
            });
        auto phi_body =
            PhiInst::create_phi(valpair.first->get_type(), header_exit);
        old2phibody[valpair.first] = phi_body;
        phi_body->add_phi_pair_operand(valpair.first, header);
        phi_body->add_phi_pair_operand(valpair.second, rotated_header);
        body_entry->add_instr_begin(phi_body);
        valpair.first->replace_use_with_if(
            phi_body, [&](const Use &use) -> bool {
                auto user_inst = dynamic_cast<Instruction *>(use.val_);
                auto user_bb = user_inst->get_parent();
                if (dominator->is_dominate(body_entry, user_bb) and
                    user_bb != header and
                    dynamic_cast<PhiInst *>(use.val_) != phi_body) {
                    return true;
                }
                return false;
            });
    }
    if (oldexit != nullptr) {
        for (auto &inst : header_exit->get_instructions()) {
            auto phi = dynamic_cast<PhiInst *>(&inst);
            if (phi == nullptr)
                break;
            auto newphi = PhiInst::create_phi(phi->get_type(), oldexit);
            oldexit->add_instr_begin(newphi);
            auto inloopval = old2phibody.at(phi->get_operand(
                0)); // 用循环入口处的phi作为除了两个header的其余来源的val,因为phibody一定支配这些来源块
            for (auto &prebb : oldexit->get_pre_basic_blocks()) {
                if (prebb == header_exit) {
                    newphi->add_phi_pair_operand(phi, prebb);
                } else {
                    newphi->add_phi_pair_operand(inloopval, prebb);
                }
            }
            phi->replace_use_with_if(newphi, [&](const Use use) -> bool {
                if (dynamic_cast<PhiInst *>(use.val_) != newphi)
                    return true;
                return false;
            });
        }
    }
}

void LoopRotate::run(Function *func, AnalysisPassManager &APM) {
    // FIXME: 处理循环判断需要多个bb的情况
    if (func->is_declaration()) {
        return;
    }
    for (auto &loop : APM.getResult<LoopDetection>(func)->get_loops()) {
        if (loop->get_blocks().size() == 1)
            continue;
        if (loop->get_exits().find(loop->get_header()) ==
            loop->get_exits().end())
            continue;

        // must execute after loopsimplify
        assert(loop->is_loopsimplifyform());
        // clear_phi(loop.get());
        rotate_loop(loop.get(), APM);
        clear_phi(loop.get());
    }
    APM.invalidateAll<Function>(func);
}