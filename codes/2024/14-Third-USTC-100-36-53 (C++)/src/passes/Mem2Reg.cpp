#include "Mem2Reg.hpp"
#include "AnalysisPass.hpp"
#include "BasicBlock.hpp"
#include "IRBuilder.hpp"
#include "Instruction.hpp"
#include "Value.hpp"

#include <cassert>
#include <memory>
#include <queue>
#include <vector>

void Mem2Reg::run(Function *f, AnalysisPassManager &APM) {
    // 创建支配树分析 Pass 的实例
    // 建立支配树
    // 以函数为单元遍历实现 Mem2Reg 算法
    auto dominators_ = APM.getResult<Dominators>(f);
    if (f->get_basic_blocks().size() >= 1) {
        // 对应伪代码中 phi 指令插入的阶段
        generate_phi(f, dominators_);
        // 对应伪代码中重命名阶段
        rename(f->get_entry_block(), dominators_);
    }
    // 后续 DeadCode 将移除冗余的局部变量的分配空间
    APM.invalidateAll<Function>(f);
}

void Mem2Reg::generate_phi(Function *func_, const Dominators *dominators_) {
    // 步骤一：找到活跃在多个 block 的全局名字集合，以及它们所属的 bb 块
    std::set<Instruction *> globals_names;
    for (auto &bb1 : func_->get_basic_blocks()) {
        auto bb = &bb1;
        for (auto &inst1 : bb->get_instructions()) {
            auto inst = &inst1;
            if (inst->is_alloca()) {
                std::set<BasicBlock *> used_bb;
                used_bb.insert(bb);
                for (auto &use : inst->get_use_list()) {
                    if (dynamic_cast<StoreInst *>(use.val_) != nullptr) {
                        auto store = dynamic_cast<StoreInst *>(use.val_);
                        used_bb.insert(store->get_parent());
                    }
                }
                if (used_bb.size() > 1) {
                    globals_names.insert(inst);
                }
            }
        }
    }

    // 步骤二：从支配树获取支配边界信息，并在对应位置插入 phi 指令
    for (auto &inst : globals_names) {
        std::set<BasicBlock *> insert_pos;
        std::queue<BasicBlock *> define_pos;
        for (auto &use : inst->get_use_list()) {
            if (dynamic_cast<StoreInst *>(use.val_) != nullptr) {
                auto store = dynamic_cast<StoreInst *>(use.val_);
                auto bb = store->get_parent();
                define_pos.push(bb);
            }
        }
        define_pos.push(inst->get_parent());
        while (!define_pos.empty()) {
            auto bb = define_pos.front();
            define_pos.pop();
            for (auto &frontier : dominators_->get_dominance_frontier(bb)) {
                if (insert_pos.find(frontier) == insert_pos.end()) {
                    auto alloca = dynamic_cast<AllocaInst *>(inst);
                    auto phi = PhiInst::create_phi(alloca->get_alloca_type(),
                                                   frontier);
                    frontier->add_instr_begin(phi);
                    phi_map_[phi] = alloca;
                    insert_pos.insert(frontier);
                    if (frontier != inst->get_parent())
                        define_pos.push(frontier);
                }
            }
        }
    }
}

void Mem2Reg::rename(BasicBlock *bb, const Dominators *dominators_) {
    for (auto &inst1 : bb->get_instructions()) {
        auto inst = &inst1;

        // 步骤三：将 phi 指令作为 lval 的最新定值，lval 即是为局部变量 alloca
        // 出的地址空间
        if (inst->is_phi()) {
            auto phi = dynamic_cast<PhiInst *>(inst);
            auto lval = phi_map_[phi];
            var_stack_[lval].push(phi);
        }

        // 步骤四：用 lval 最新的定值替代对应的load指令
        if (inst->is_load()) {
            auto load = dynamic_cast<LoadInst *>(inst);
            auto lval = load->get_lval();
            if (!is_valid_ptr(lval))
                continue;
            assert(!var_stack_[lval].empty());
            auto new_lval = var_stack_[lval].top();
            load->replace_all_use_with(new_lval);
        }

        // 步骤五：将 store 指令的 rval，也即被存入内存的值，作为 lval
        // 的最新定值
        if (inst->is_store()) {
            auto store = dynamic_cast<StoreInst *>(inst);
            auto lval = store->get_lval();
            auto rval = store->get_rval();
            if (!is_valid_ptr(lval))
                continue;
            var_stack_[lval].push(rval);
        }
    }
    // 步骤六：为 lval 对应的 phi 指令参数补充完整
    for (auto &bb_succ : bb->get_succ_basic_blocks()) {
        for (auto &inst1 : bb_succ->get_instructions()) {
            auto inst = &inst1;
            if (inst->is_phi()) {
                auto phi = dynamic_cast<PhiInst *>(inst);
                auto lval = phi_map_[phi];
                if(not lval) continue;
                if (var_stack_.find(lval) == var_stack_.end())
                    continue;
                // assert(!var_stack_[lval].empty());
                if (var_stack_[lval].empty())
                    continue;
                auto rval = var_stack_[lval].top();
                phi->add_phi_pair_operand(rval, bb);
            }
        }
    }

    // 步骤七：对 bb 在支配树上的所有后继节点，递归执行 re_name 操作
    for (auto &bb_succ : dominators_->get_dom_tree_succ_blocks(bb)) {
        rename(bb_succ, dominators_);
    }

    // 步骤八：pop出 lval 的最新定值
    for (auto &inst1 : bb->get_instructions()) {
        auto inst = &inst1;
        if (inst->is_phi()) {
            auto phi = dynamic_cast<PhiInst *>(inst);
            auto lval = phi_map_[phi];
            var_stack_[lval].pop();
        }
        if (inst->is_store()) {
            auto store = dynamic_cast<StoreInst *>(inst);
            auto lval = store->get_lval();
            if (!is_valid_ptr(lval))
                continue;
            var_stack_[lval].pop();
        }
    }
    // 步骤九：清除冗余的指令
    std::set<Instruction *> useless_inst;
    for (auto &inst1 : bb->get_instructions()) {
        auto inst = &inst1;
        if ((inst->is_store())) {
            auto store = dynamic_cast<StoreInst *>(inst);
            auto lval = store->get_lval();
            if (!is_valid_ptr(lval))
                continue;
            useless_inst.insert(inst);
        }
        if (inst->is_load()) {
            auto load = dynamic_cast<LoadInst *>(inst);
            auto lval = load->get_lval();
            if (!is_valid_ptr(lval))
                continue;
            useless_inst.insert(inst);
        }
    }
    for (auto &inst : useless_inst) {
        bb->erase_instr(inst);
    }
}
