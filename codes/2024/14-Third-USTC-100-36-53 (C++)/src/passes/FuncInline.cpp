#include "FuncInline.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "Value.hpp"
#include <algorithm>
#include <vector>

void FuncInline::run(Module *m, AnalysisPassManager &APM) {
    auto func_info = APM.getResult<FuncInfo>(m);
    inlinecnt = 0;
    auto tryinline = [&](Function &func1) {
        for (auto &bb : func1.get_basic_blocks()) {
            // assert(bb.get_instructions().size() > 0);
            if (bb.get_instructions().size() == 0)
                continue;
            for (auto &inst : bb.get_instructions()) {
                if (inst.is_call()) {
                    auto func = &func1;
                    if (dynamic_cast<Function *>(inst.get_operand(0))
                                ->is_declaration() == false and
                        should_inline(func, func_info)) {
                        inline_func(func, &bb, &inst, m);
                        return true;
                    }
                }
            }
        }
        return false;
    };
    int count = 0;
    for (auto &func : m->get_functions()) {
        while (tryinline(func)) {
            count++;
        };
    }
    APM.invalidateAll<Module>(m);
}

void FuncInline::inline_func(Function *caller, BasicBlock *parent_bb,
                             Instruction *inst, Module *m_) {
    auto call = dynamic_cast<CallInst *>(inst);
    auto callee = dynamic_cast<Function *>(call->get_operand(0));
    auto old_entry = callee->get_entry_block();
    std::vector<BasicBlock *> bodies_order; // 被调用函数bb块
    std::map<Value *, Value *> old2new; // 参数映射,虚参到实参,bb映射,指令映射
    std::map<BasicBlock *, Value *> retbb2val;
    auto &callee_argus = callee->get_args();
    int temp = 0;
    for (auto &arg : callee_argus) {
        old2new[&arg] = call->get_operand(++temp);
    }
    for (auto &bb : callee->get_basic_blocks())
        bodies_order.emplace_back(&bb);

    for (auto bb : bodies_order) {
        // old2new[bb] = BasicBlock::create(
        //     m_, "old2new_" + bb->get_name() + "_" + std::to_string(inlinecnt++),
        //     caller);
        old2new[bb] = BasicBlock::create(m_, "", caller);
    }

    for (auto bb : bodies_order) {
        auto new_bb = dynamic_cast<BasicBlock *>(old2new.at(bb));
        for (auto &inst : bb->get_instructions()) {
            auto new_inst = Instruction::clone_inst(&inst, new_bb, false);
            old2new[&inst] = new_inst;
        }
    }
    for (auto bb : bodies_order) {
        for (auto &inst : bb->get_instructions()) {
            auto new_inst = dynamic_cast<Instruction *>(old2new[&inst]);
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
    auto new_entry = dynamic_cast<BasicBlock *>(old2new[old_entry]);
    // auto resbb = BasicBlock::create(
    //     m_, "retfrom" + callee->get_name() + "_" +
    //     std::to_string(inlinecnt++), caller);
    auto resbb = BasicBlock::create(m_, "", caller);
    // std::vector<Instruction *> collect_restinst; //
    // 存储call指令后面的所有指令
    for (auto inst1 = parent_bb->get_instructions().begin();
         inst1 != parent_bb->get_instructions().end(); inst1++) {
        if (&(*inst1) == inst) {
            for (auto inst2 = ++inst1;
                 inst2 != parent_bb->get_instructions().end(); inst2++) {
                old2new[&(*inst2)] =
                    Instruction::clone_inst(&(*inst2), resbb, false);
            }
            int distance =
                std::distance(inst1, parent_bb->get_instructions().end());
            // 删除call后面的所有指令
            while (distance-- > 0) {
                (parent_bb->get_instructions().back())
                    .replace_all_use_with(
                        old2new[&(parent_bb->get_instructions().back())]);
                parent_bb->get_instructions().pop_back();
            }
            parent_bb->get_instructions().remove(inst); // 这个指令可能没有释放
            BranchInst::create_br(new_entry, parent_bb); // 跳转到内联部分
            break;
        }
    }

    // 处理ret指令
    for (auto bb : bodies_order) {
        auto &temp_inst = bb->get_instructions().back();
        if (temp_inst.is_ret()) {
            auto ret = dynamic_cast<ReturnInst *>(&temp_inst);
            auto newbb = dynamic_cast<BasicBlock *>(old2new[bb]);
            if (ret->is_void_ret()) {
                newbb->get_instructions().pop_back();
                BranchInst::create_br(resbb, newbb);
            } else {
                if (old2new.find(temp_inst.get_operand(0)) != old2new.end())
                    retbb2val[newbb] = old2new[temp_inst.get_operand(0)];
                else
                    retbb2val[newbb] = temp_inst.get_operand(0);
                newbb->get_instructions().pop_back();
                BranchInst::create_br(resbb, newbb);
            }
        }
    }
    // 函数有返回值
    if (!retbb2val.empty()) {
        if (retbb2val.size() > 1) {
            std::vector<Value *> rets;
            std::vector<BasicBlock *> ret_bbs;
            for (auto &pair : retbb2val) {
                rets.push_back(pair.second);
                ret_bbs.push_back(pair.first);
            }
            auto retphi = PhiInst::create_phi(callee->get_return_type(), resbb,
                                              rets, ret_bbs);
            resbb->add_instr_begin(retphi);
            inst->replace_all_use_with(retphi);
        } else {
            for (auto &pair : retbb2val) {
                inst->replace_all_use_with(pair.second);
            }
        }
        delete inst;
    }
    // 维护修改后的bb块关系
    // ret
    // for (auto retbb : retbb2val) {
    //     resbb->add_pre_basic_block(retbb.first);
    //     retbb.first->add_succ_basic_block(resbb);
    // }
    // br
    // std::vector<BasicBlock *> subbs;
    // for (auto subb : parent_bb->get_succ_basic_blocks())
    //     subbs.emplace_back(subb);
    // for (auto succ_bb : subbs) {
    //     resbb->add_succ_basic_block(succ_bb);
    //     parent_bb->remove_succ_basic_block(succ_bb);
    // }
    // 将phi指令中指向parentbb的改为指向resbb
    for (auto tar_bb : resbb->get_succ_basic_blocks()) {
        for (auto &inst_r : tar_bb->get_instructions()) {
            if (inst_r.is_phi()) {
                for (unsigned i = 1; i < inst_r.get_num_operand(); i += 2) {
                    if (dynamic_cast<BasicBlock *>(inst_r.get_operand(i)) ==
                        parent_bb) {
                        inst_r.set_operand(i, resbb);
                    }
                }
            } else
                break;
        }
    }

    // parent_bb->add_succ_basic_block(new_entry);
    // new_entry->add_pre_basic_block(parent_bb);
    for (auto it = bodies_order.begin(); it != bodies_order.end();) {

        auto bb = dynamic_cast<BasicBlock *>(old2new.at(*it));
        auto br_inst =
            dynamic_cast<BranchInst *>(&*(bb->get_instructions().rbegin()));

        if (!br_inst)
            continue;
        else {
            if (br_inst->get_num_operand() == 1) {
                auto succ_bb =
                    dynamic_cast<BasicBlock *>(br_inst->get_operand(0));
                if (std::find(bb->get_succ_basic_blocks().begin(),
                              bb->get_succ_basic_blocks().end(),
                              succ_bb) == bb->get_succ_basic_blocks().end())
                    bb->add_succ_basic_block(succ_bb);
                if (std::find(succ_bb->get_pre_basic_blocks().begin(),
                              succ_bb->get_pre_basic_blocks().end(),
                              bb) == succ_bb->get_pre_basic_blocks().end())
                    succ_bb->add_pre_basic_block(bb);
            } else {
                auto succ_bb =
                    dynamic_cast<BasicBlock *>(br_inst->get_operand(1));
                if (std::find(bb->get_succ_basic_blocks().begin(),
                              bb->get_succ_basic_blocks().end(),
                              succ_bb) == bb->get_succ_basic_blocks().end())
                    bb->add_succ_basic_block(succ_bb);
                if (std::find(succ_bb->get_pre_basic_blocks().begin(),
                              succ_bb->get_pre_basic_blocks().end(),
                              bb) == succ_bb->get_pre_basic_blocks().end())
                    succ_bb->add_pre_basic_block(bb);
                succ_bb = dynamic_cast<BasicBlock *>(br_inst->get_operand(2));
                if (std::find(bb->get_succ_basic_blocks().begin(),
                              bb->get_succ_basic_blocks().end(),
                              succ_bb) == bb->get_succ_basic_blocks().end())
                    bb->add_succ_basic_block(succ_bb);
                if (std::find(succ_bb->get_pre_basic_blocks().begin(),
                              succ_bb->get_pre_basic_blocks().end(),
                              bb) == succ_bb->get_pre_basic_blocks().end())
                    succ_bb->add_pre_basic_block(bb);
            }
        }
        // 在clone指令部分会在caller的bb的sucbb中自动加入callee的bb，去除这些sucbb
        auto pre_blocks = (*it)->get_pre_basic_blocks();
        for (auto prebb = pre_blocks.begin(); prebb != pre_blocks.end();
             ++prebb) {
            if (*prebb && std::find(bodies_order.begin(), bodies_order.end(),
                                    *prebb) == bodies_order.end()) {
                (*it)->remove_pre_basic_block(*prebb);
                (*prebb)->remove_succ_basic_block(*it);
            }
        }
        auto succ_blocks = (*it)->get_succ_basic_blocks();
        for (auto sucbb = succ_blocks.begin(); sucbb != succ_blocks.end();
             ++sucbb) {
            if (*sucbb && std::find(bodies_order.begin(), bodies_order.end(),
                                    *sucbb) == bodies_order.end()) {
                (*it)->remove_succ_basic_block(*sucbb);
                (*sucbb)->remove_pre_basic_block(*it);
            }
        }
        ++it;
    }
    callee->remove_use(call, 0);
}
bool FuncInline::should_inline(Function *func, const FuncInfo *func_info_) {
    // 递归函数不能内联
    for (auto &bb : func->get_basic_blocks()) {
        for (auto &inst : bb.get_instructions()) {
            if (inst.is_call()) {
                if (func_info_->is_recursive_function(
                        dynamic_cast<Function *>(inst.get_operand(0)))) {
                    return false;
                }
            }
        }
    }
    return true;
}

// 可以在死代码删除添加检测函数,若是全部被内联,就彻底删除这一函数
