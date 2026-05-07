#include <memory>
#include <unordered_map>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

static bool remove_unreachable(std::shared_ptr<ir::Function> func) {
    bool modified = false;
    std::unordered_map<std::shared_ptr<ir::BasicBlock>, bool> reachable;
    func->for_each_block([&](auto &b) { reachable.emplace(b, false); });
    std::vector<std::shared_ptr<ir::BasicBlock>> stack;
    if (auto entry = func->entry_block()) stack.push_back(entry);
    while (!stack.empty()) {
        auto b = stack.back();
        stack.pop_back();
        if (!b || reachable[b]) continue;
        reachable[b] = true;
        for (auto &sw : b->opt_info.successors) {
            if (auto s = sw.lock()) stack.push_back(s);
        }
    }
    for (const auto &p : reachable) {
        if (!p.second) {
            opt::util::remove_basic_block(p.first);
            modified = true;
        }
    }
    return modified;
}

// 条件 br：then 与 else 相同 -> 变为无条件；常量条件 -> 变为无条件
static bool br2jump(std::shared_ptr<ir::Function> func) {
    bool modified = false;
    auto blocks = func->get_basic_blocks();
    for (const auto &block : blocks) {
        auto br = std::dynamic_pointer_cast<ir::Br>(block->tail_instruction());
        if (!br) continue;
        if (br->is_cond_branch()) {
            auto cond = br->get_operands_ref()[0];
            auto tbb = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[1]);
            auto fbb = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[2]);
            if (!tbb || !fbb) continue;
            if (tbb == fbb) {
                // Rewrite to unconditional jump to tbb
                opt::util::remove_all_operands(br);
                br->add_operand(tbb);
                tbb->add_user(br);
                // reset CFG edges: clear old edges then add the single edge
                auto succs = block->opt_info.successors;
                for (auto &sw : succs) {
                    if (auto s = sw.lock()) s->opt_info.remove_predecessor(block);
                }
                block->opt_info.successors.clear();
                block->opt_info.successors.push_back(tbb);
                tbb->opt_info.predecessors.push_back(block);
                modified = true;
                continue;
            }
            // 不在本 Pass 中处理常量条件；交给 SCCP/ConstFold
        }
    }
    return modified;
}

// 支配层级合并：若 block 只有一个后继，沿着后继链合并连续的“单前驱且无 phi”的块到 block
static bool merge_blocks(std::shared_ptr<ir::Function> func) {
    bool modified = false;
    // 使用一份稳定的顺序副本以避免遍历中破坏结构
    auto order = func->get_basic_blocks();
    for (const auto &first : order) {
        if (first->opt_info.successors.size() != 1) continue;
        auto br = std::dynamic_pointer_cast<ir::Br>(first->tail_instruction());
        if (!br || br->is_cond_branch()) continue;
        auto cur = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[0]);
        if (!cur) continue;
        std::vector<std::shared_ptr<ir::BasicBlock>> chain;
        while (cur->opt_info.predecessors.size() == 1) {
            // 避免合并含 phi 的块
            auto insts = cur->get_instructions();
            bool has_phi = false;
            for (const auto &inst : insts) {
                if (std::dynamic_pointer_cast<ir::Phi>(inst)) {
                    has_phi = true;
                    break;
                } else {
                    break;
                }
            }
            if (has_phi) break;
            chain.push_back(cur);
            auto tail = std::dynamic_pointer_cast<ir::Br>(cur->tail_instruction());
            if (!tail || tail->is_cond_branch() || cur->opt_info.successors.size() != 1) break;
            cur = std::dynamic_pointer_cast<ir::BasicBlock>(tail->get_operands_ref()[0]);
            if (!cur) break;
        }
        if (chain.empty()) continue;
        // 记录并移除 first 原有的唯一后继（链首）CFG 边
        auto first_old_succ = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[0]);
        if (first_old_succ) {
            first->opt_info.remove_successor(first_old_succ);
            first_old_succ->opt_info.remove_predecessor(first);
        }
        // 将 first 的尾 br 移除，然后把链中指令（除末尾 terminator 外）搬到 first
        opt::util::remove_instruction(std::dynamic_pointer_cast<ir::Instruction>(first->tail_instruction()));
        for (size_t i = 0; i < chain.size(); i++) {
            auto block = chain[i];
            auto insts = block->get_instructions();
            for (const auto &inst : insts) {
                if (std::dynamic_pointer_cast<ir::Phi>(inst)) {
                    // 保守：链构造时已排除 phi
                    continue;
                }
                if (inst == block->tail_instruction()) break;
                opt::util::remove_instruction_from_parent_basic_block(inst);
                inst->set_parent_block(first);
                first->add_instructions({inst});
            }
        }
        // 处理最后一个块：移动其 terminator 到 first，并修正后继上的 phi 来边
        auto last = chain.back();
        auto term = last->tail_instruction();
        opt::util::remove_instruction_from_parent_basic_block(term);
        term->set_parent_block(first);
        first->add_instructions({term});
        // 迁移 CFG 边：first 继承 last 的后继
        first->opt_info.successors.clear();
        for (const auto &succ_w : last->opt_info.successors) {
            if (auto succ = succ_w.lock()) {
                // 更新 succ 的前驱列表：用 first 替换 last
                succ->opt_info.remove_predecessor(last);
                succ->opt_info.predecessors.push_back(first);
                first->opt_info.successors.push_back(succ);
                auto insts = succ->get_instructions();
                for (const auto &inst : insts) {
                    if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
                        // 安全获取PHI值，避免空指针
                        auto val = opt::util::get_phi_value_safe(phi, last);
                        if (val) {  // 只有当PHI确实有来自last的边时才处理
                            opt::util::remove_phi_block(phi, last);
                            opt::util::add_incoming(phi, first, val);
                        }
                    } else {
                        break;
                    }
                }
            }
        }
        // 更新 CFG：删除链中所有块
        for (const auto &block : chain) {
            opt::util::remove_basic_block(block);
        }
        modified = true;
    }
    return modified;
}

// 把仅含无条件跳转的中继块 B 的前驱改直接指向后继，并删除 B（要求后继首指令不是 PHI）
static bool change_target(std::shared_ptr<ir::Function> func) {
    bool modified = false;
    auto blocks = func->get_basic_blocks();
    for (const auto &block : blocks) {
        // 仅含无条件 br：块中只有一条指令且为无条件 br
        if (block->get_instructions_ref().size() != 1) continue;
        auto only_br = std::dynamic_pointer_cast<ir::Br>(block->tail_instruction());
        if (!only_br || only_br->is_cond_branch()) continue;
        if (block == func->entry_block()) continue;
        if (block->opt_info.successors.size() != 1) continue;
        auto succ = std::dynamic_pointer_cast<ir::BasicBlock>(only_br->get_operands_ref()[0]);
        if (!succ || succ == block) continue;
        // 后继首指令不能是 PHI
        auto insts = succ->get_instructions();
        if (!insts.empty() && std::dynamic_pointer_cast<ir::Phi>(insts.front())) continue;
        if (block->opt_info.predecessors.empty()) continue;

        // 重定向所有前驱的尾 br 目标 block -> succ
        auto preds = block->opt_info.predecessors;
        for (const auto &pw : preds) {
            auto pre = pw.lock();
            if (!pre) continue;
            if (auto pbr = std::dynamic_pointer_cast<ir::Br>(pre->tail_instruction())) {
                if (pbr->is_cond_branch()) {
                    opt::util::substitute_operand(pbr, block, succ);
                } else {
                    opt::util::substitute_operand(pbr, 0, succ);
                }
            }
            pre->opt_info.successors.push_back(succ);
            succ->opt_info.predecessors.push_back(pre);
            pre->opt_info.remove_successor(block);
            succ->opt_info.remove_predecessor(block);
        }
        opt::util::remove_basic_block(block);
        modified = true;
    }
    return modified;
}

// 对“只有一个前驱”的基本块，删除其 PHI：直接用该前驱对应值替换 PHI
static bool merge_phi_for_single_pred(std::shared_ptr<ir::Function> func) {
    bool modified = false;
    auto blocks = func->get_basic_blocks();
    for (const auto &block : blocks) {
        if (block->opt_info.predecessors.size() != 1) continue;
        auto phis = opt::util::get_phis(block);
        if (phis.empty()) continue;
        auto pred = block->opt_info.predecessors[0].lock();
        if (!pred) continue;
        for (const auto &phi : phis) {
            auto val = opt::util::get_phi_value_safe(phi, pred);
            if (val) {  // 安全检查：确保PHI有来自pred的边
                opt::util::substitute(phi, val);
                modified = true;
            }
        }
    }
    return modified;
}

bool SimplifyCFG::run(ir::Module *module) {
    bool modified = false;
    module->for_each_func([&](auto &func) {
        bool changed = false;
        do {
            changed = false;
            changed |= br2jump(func);
            changed |= remove_unreachable(func);
            changed |= merge_blocks(func);
            changed |= remove_unreachable(func);
            changed |= change_target(func);
            changed |= remove_unreachable(func);
            changed |= merge_phi_for_single_pred(func);
            modified |= changed;
        } while (changed);
    });
    return modified;
}

}  // namespace opt::pass
