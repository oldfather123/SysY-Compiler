#include "LICM.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "GlobalVariable.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include <cstddef>
#include <memory>

void LoopInvariantCodeMotion::run(Function *func, AnalysisPassManager &APM) {

    // loop_detection_ = std::make_unique<LoopDetection>(m_);
    // loop_detection_->run();
    // func_info_ = std::make_unique<FuncInfo>(m_);
    // func_info_->run();
    auto loop_detection_ = APM.getResult<LoopDetection>(func);
    auto func_info_ = APM.getResult<FuncInfo>(func->get_parent());
    for (auto &loop : loop_detection_->get_loops()) {
        is_loop_done_[loop] = false;
    }

    for (auto &loop : loop_detection_->get_loops()) {
        traverse_loop(loop, func_info_, func);
    }
    APM.invalidateAll<Function>(func);
}

void LoopInvariantCodeMotion::traverse_loop(std::shared_ptr<Loop> loop,
                                            const FuncInfo *func_info_,
                                            Function *func) {
    if (is_loop_done_[loop]) {
        return;
    }
    is_loop_done_[loop] = true;
    for (auto &sub_loop : loop->get_sub_loops()) {
        traverse_loop(sub_loop, func_info_, func);
    }
    run_on_loop(loop, func_info_, func);
}

void LoopInvariantCodeMotion::run_on_loop(std::shared_ptr<Loop> loop,
                                          const FuncInfo *func_info_,
                                          Function *func) {
    std::set<Value *> loop_exists;
    std::set<Value *> updated_global;
    bool has_not_pure_func = false;
    std::function<void(std::shared_ptr<Loop>)> traverse =
        [&](std::shared_ptr<Loop> loop) {
            for (auto &sub_loop : loop->get_sub_loops()) {
                traverse(sub_loop);
            }
            for (auto &bb : loop->get_blocks()) {
                for (auto &inst : bb->get_instructions()) {
                    loop_exists.insert(&inst);
                    if (inst.get_instr_type() == Instruction::store) {
                        auto *store_inst = dynamic_cast<StoreInst *>(&inst);
                        if (auto *global = dynamic_cast<GlobalVariable *>(
                                store_inst->get_lval())) {
                            updated_global.insert(global);
                        }
                    }
                    if (inst.get_instr_type() == Instruction::call &&
                        !func_info_->is_pure_function(dynamic_cast<Function *>(
                            dynamic_cast<CallInst *>(&inst)->get_operand(0)))) {
                        has_not_pure_func = true;
                    }
                }
            }
        };

    traverse(loop);

    std::set<Value *> loop_invariant;
    std::vector<Value *> loop_invariant_ordered;
    bool changed;
    do {
        changed = false;
        for (auto &bb : loop->get_blocks()) {
            for (auto &inst : bb->get_instructions()) {
                if (loop_invariant.find(&inst) != loop_invariant.end())
                    continue;
                auto inst_type = inst.get_instr_type();
                if (inst_type == Instruction::alloca ||
                    inst_type == Instruction::store ||
                    inst_type == Instruction::ret ||
                    inst_type == Instruction::br ||
                    inst_type == Instruction::phi)
                    continue;

                if (inst_type == Instruction::call &&
                    !func_info_->is_pure_function(dynamic_cast<Function *>(
                        dynamic_cast<CallInst *>(&inst)->get_operand(0))))
                    continue;

                if (inst_type == Instruction::load) {
                    if (dynamic_cast<GlobalVariable *>(
                            dynamic_cast<LoadInst *>(&inst)->get_lval()) ==
                            nullptr ||
                        updated_global.find(
                            dynamic_cast<LoadInst *>(&inst)->get_lval()) !=
                            updated_global.end() ||
                        has_not_pure_func)
                        continue;
                }

                bool is_invariant = true;
                for (auto &op : inst.get_operands()) {
                    if ((loop_exists.find(op) != loop_exists.end() &&
                         loop_invariant.find(op) == loop_invariant.end())) {
                        is_invariant = false;
                        break;
                    }
                }
                if (is_invariant) {
                    loop_invariant.insert(&inst);
                    loop_exists.erase(&inst);
                    loop_invariant_ordered.push_back(&inst);
                    changed = true;
                }
            }
        }
    } while (changed);

    if (loop->get_preheader() == nullptr) {
        // loop->set_preheader(BasicBlock::create(
        //     func->get_parent(), "", loop->get_header()->get_parent()));
        throw std::runtime_error("Need loop simplify first");
    }

    if (loop_invariant.empty())
        return;

    // insert preheader
    auto preheader = loop->get_preheader();

    // update phi
    // for (auto &phi_inst_ : loop->get_header()->get_instructions()) {
    //     if (phi_inst_.get_instr_type() != Instruction::phi)
    //         break;
    //     auto *phi_inst = dynamic_cast<PhiInst *>(&phi_inst_);

    //     std::vector<std::pair<Value *, Value *>> to_move;
    //     for (unsigned i = 0; i < phi_inst->get_num_operand(); i += 2) {
    //         auto *val = phi_inst->get_operand(i);
    //         auto *bb = dynamic_cast<BasicBlock *>(phi_inst->get_operand(i +
    //         1)); if (loop->get_back_edges_nodes().find(bb) !=
    //             loop->get_back_edges_nodes().end())
    //             continue;
    //         to_move.push_back({val, bb});
    //     }

    //     if (to_move.size() == 0)
    //         continue;
    //     auto *new_phi_inst =
    //         PhiInst::create_phi(phi_inst->get_type(), preheader);
    //     preheader->add_instr_before_terminator(new_phi_inst);

    //     for (auto &pair : to_move) {
    //         phi_inst->remove_phi_operand(pair.second);
    //         new_phi_inst->add_phi_pair_operand(pair.first, pair.second);
    //     }

    //     phi_inst->add_phi_pair_operand(new_phi_inst, preheader);
    // }

    // rebuild cfg and br instruction
    // std::vector<BasicBlock *> pred_to_remove;
    // for (auto &pred : loop->get_header()->get_pre_basic_blocks()) {
    //     if (loop->get_back_edges_nodes().find(pred) !=
    //         loop->get_back_edges_nodes().end())
    //         continue;
    //     auto *term = pred->get_terminator();
    //     for (unsigned i = 0; i < term->get_num_operand(); i++) {
    //         if (term->get_operand(i) == loop->get_header()) {
    //             term->set_operand(i, preheader);
    //         }
    //     }
    //     pred->remove_succ_basic_block(loop->get_header());
    //     pred->add_succ_basic_block(preheader);
    //     preheader->add_pre_basic_block(pred);
    //     pred_to_remove.push_back(pred);
    // }

    // for (auto &pred : pred_to_remove) {
    //     loop->get_header()->remove_pre_basic_block(pred);
    // }

    // insert loop invariant
    for (auto &inst_ : loop_invariant_ordered) {
        auto *inst = dynamic_cast<Instruction *>(inst_);
        inst->get_parent()->remove_instr(inst);
        preheader->add_instr_before_terminator(inst);
    }
    // clear phi
    for (auto &inst : preheader->get_instructions()) {
        if (inst.is_phi() and inst.get_operand(1) == preheader and
            inst.get_num_operand() == 2) {
            inst.replace_all_use_with(inst.get_operand(0));
        }
    }

    // insert preheader br to header
    // BranchInst::create_br(loop->get_header(), preheader);

    // insert preheader to parent loop
    // if (loop->get_parent() != nullptr) {
    //     loop->get_parent()->add_block(preheader);
    // }
}