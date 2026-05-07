#include "Mem2Reg.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <algorithm>
#include <queue>
#include <stack>
#include <iostream>

void Mem2Reg::run() {
    // std::cerr << "[DEBUG Mem2Reg] Starting Mem2Reg pass" << std::endl;
    
    // 以函数为单元遍历实现 Mem2Reg 算法
    for (auto &f : m_->get_functions()) {
        if (f.is_declaration())
            continue;
            
        func_ = &f;
        // std::cerr << "[DEBUG Mem2Reg] Processing function: " << f.get_name() << std::endl;
        
        // 为每个函数创建新的支配树分析
        dominators_ = std::make_unique<Dominators>(m_);
        dominators_->run_on_func(func_);
        // std::cerr << "[DEBUG Mem2Reg] Dominators analysis completed for function: " << f.get_name() << std::endl;
        
        if (func_->get_basic_blocks().size() >= 1) {
            // 清空数据结构
            var_val_stack_.clear();
            phi_map.clear();
            
            // 将所有 alloca 指令移动到入口块的开始
            loop_alloc_inv_hoist();
            
            // 对应伪代码中 phi 指令插入的阶段
            generate_phi();
            
            // 对应伪代码中重命名阶段
            rename(func_->get_entry_block());
            
            // 确保所有 PHI 节点有完整的参数
            complete_phi_nodes();
            
            // 移除 trivial phi
            remove_trivial_phi();
            
            // 清理未使用的 alloca
            remove_dead_allocas();
        }
        // 后续 DeadCode 将移除冗余的局部变量的分配空间
    }
}

void Mem2Reg::complete_phi_nodes() {
    // 确保每个 PHI 节点都有来自所有前驱块的参数
    for (auto &bb : func_->get_basic_blocks()) {
        std::vector<PhiInst*> phi_nodes;
        
        // 收集所有 PHI 节点
        for (auto &inst : bb.get_instructions()) {
            if (auto phi = dynamic_cast<PhiInst*>(&inst)) {
                phi_nodes.push_back(phi);
            }
        }
        
        // 对每个 PHI 节点，确保有来自所有前驱的参数
        for (auto phi : phi_nodes) {
            auto var_it = phi_map.find(phi);
            if (var_it == phi_map.end()) continue;
            
            auto var = var_it->second;
            Type *var_type = var->get_type()->get_pointer_element_type();
            
            // 获取所有前驱块
            auto preds = bb.get_pre_basic_blocks();
            
            // 检查每个前驱是否都有对应的参数
            for (auto pred : preds) {
                bool has_param = false;
                
                // 检查是否已经有来自该前驱的参数
                for (size_t i = 1; i < phi->get_num_operand(); i += 2) {
                    if (phi->get_operand(i) == pred) {
                        has_param = true;
                        break;
                    }
                }
                
                // 如果没有，添加默认值
                if (!has_param) {
                    Value *default_val = ConstantZero::get(var_type, m_);
                    phi->add_operand(default_val);
                    phi->add_operand(pred);
                }
            }
            
            // 验证 PHI 节点的完整性
            if (phi->get_num_operand() != preds.size() * 2) {
                // std::cerr << "[WARNING] PHI node has " << phi->get_num_operand() 
                //           << " operands but " << preds.size() << " predecessors" << std::endl;
            }
        }
    }
}

void Mem2Reg::loop_alloc_inv_hoist() {
    // std::cerr << "[DEBUG] Moving all allocas to entry block" << std::endl;
    std::vector<Instruction*> alloc_vec;
    
    // 收集所有 alloca 指令
    for(auto &bb : func_->get_basic_blocks()) {
        auto b = &bb;
        std::vector<Instruction*> replace_inst;
        for(auto &inst : b->get_instructions()) {
            auto instr = &inst;
            if(instr->is_alloca()) {
                alloc_vec.push_back(instr);
                replace_inst.push_back(instr);
            }
        }
        // 从原基本块中移除
        for(auto instr : replace_inst) {
            instr->get_parent()->remove_instr(instr);
        }
    }
    
    // 将所有 alloca 添加到入口块的开始
    auto entry_bb = func_->get_entry_block();
    for(auto &instr : alloc_vec) {
        entry_bb->add_instr_begin(instr);
    }
}

void Mem2Reg::generate_phi() {
    //std::cerr << "[DEBUG] Generating PHI nodes" << std::endl;
    
   
    std::set<Value *> alloca_var;
    std::map<Value *, std::set<BasicBlock *>> alloca_var_map;
    
   
    for (auto &bb : func_->get_basic_blocks()) {
        auto b = &bb;
        for (auto &inst : b->get_instructions()) {
            auto instr = &inst;
            if (inst.is_store()) {
                auto l_val = dynamic_cast<StoreInst *>(instr)->get_lval();
                if (is_valid_ptr(l_val)) {
                    alloca_var.insert(l_val);
                    alloca_var_map[l_val].insert(b);
                }
            }
        }
    }
    
    
    for (auto var : alloca_var) {
        // 初始化W集合
        std::vector<BasicBlock *> w;
        w.assign(alloca_var_map[var].begin(), alloca_var_map[var].end());
        std::set<BasicBlock *> F;
        
        // 迭代算法
        for (size_t i = 0; i < w.size(); i++) {
            auto bb = w[i];
            for (auto y : dominators_->get_dominance_frontier(bb)) {
                if (F.find(y) == F.end()) {
                    // 创建 phi 节点，使用正确的构造函数
                    std::vector<Value*> vals;
                    std::vector<BasicBlock*> val_bbs;
                    auto phi_inst = PhiInst::create_phi(
                        var->get_type()->get_pointer_element_type(), 
                        y, 
                        vals, 
                        val_bbs
                    );
                    
                    phi_map[phi_inst] = var;
                    y->add_instr_begin(phi_inst);
                    F.insert(y);
                    
                    // 继续迭代
                    w.push_back(y);
                }
            }
        }
    }
}

void Mem2Reg::rename(BasicBlock *bb) {
    std::vector<Instruction *> deleted_instructions;
    
    //std::cerr << "[DEBUG rename] Processing basic block: " << bb->get_name() << std::endl;
    
    
    for (auto &inst : bb->get_instructions()) {
        auto instr = &inst;
        if (instr->is_phi()) {
            auto var = phi_map.find(instr);
            if(var == phi_map.end())
                continue;
            var_val_stack_[var->second].push_back(instr);
        }
    }
    
   
    for (auto &inst : bb->get_instructions()) {
        auto instr = &inst;
        if (instr->is_load()) {
            auto l_val = dynamic_cast<LoadInst *>(instr)->get_lval();
            if (is_valid_ptr(l_val)) {
                if (var_val_stack_.find(l_val) != var_val_stack_.end()) {
                    if (!var_val_stack_[l_val].empty()) {
                        instr->replace_all_use_with(var_val_stack_[l_val].back());
                        deleted_instructions.push_back(instr);
                    } else {
                        // 如果栈为空，使用默认值
                        Type *alloca_type = l_val->get_type()->get_pointer_element_type();
                        Constant *default_val = ConstantZero::get(alloca_type, m_);
                        instr->replace_all_use_with(default_val);
                        deleted_instructions.push_back(instr);
                    }
                }
            }
        }
        
       
        if (instr->is_store()) {
            auto l_val = dynamic_cast<StoreInst *>(instr)->get_lval();
            auto r_val = dynamic_cast<StoreInst *>(instr)->get_rval();
            if (is_valid_ptr(l_val)) {
                var_val_stack_[l_val].push_back(r_val);
                deleted_instructions.push_back(instr);
            }
        }
    }
    
   
    for (auto succ_b : bb->get_succ_basic_blocks()) {
        for (auto &inst : succ_b->get_instructions()) {
            auto instr = &inst;
            if (instr->is_phi()) {
                auto var = phi_map.find(instr);
                if(var == phi_map.end())
                    continue;
                
                // 添加来自当前基本块的参数
                if (var_val_stack_.find(var->second) != var_val_stack_.end()) {
                    Value *val_to_add = nullptr;
                    
                    if (!var_val_stack_[var->second].empty()) {
                        val_to_add = var_val_stack_[var->second].back();
                    } else {
                        // 如果栈为空，使用默认值
                        Type *alloca_type = var->second->get_type()->get_pointer_element_type();
                        val_to_add = ConstantZero::get(alloca_type, m_);
                    }
                    
                    // PhiInst 的操作数是成对的 (value, block)
                    dynamic_cast<PhiInst *>(instr)->add_operand(val_to_add);
                    dynamic_cast<PhiInst *>(instr)->add_operand(bb);
                }
            }
        }
    }
    
   
    const auto &dom_tree_succs = dominators_->get_dom_tree_succ_blocks(bb);
    for (auto b : dom_tree_succs) {
        rename(b);
    }
    
    
    for (auto &inst : bb->get_instructions()) {
        auto instr = &inst;
        if (instr->is_phi()) {
            auto var = phi_map.find(instr);
            if(var == phi_map.end())
                continue;
            if (is_valid_ptr(var->second)) {
                if (!var_val_stack_[var->second].empty()) {
                    var_val_stack_[var->second].pop_back();
                }
            }
        }
        else if (instr->is_store()) {
            auto l_val = dynamic_cast<StoreInst *>(instr)->get_lval();
            if (is_valid_ptr(l_val)) {
                if (!var_val_stack_[l_val].empty()) {
                    var_val_stack_[l_val].pop_back();
                }
            }
        }
    }
    
   
    for (auto inst : deleted_instructions) {
        bb->erase_instr(inst);
    }
}

void Mem2Reg::remove_trivial_phi() {
    //std::cerr << "[DEBUG] Removing trivial PHI nodes" << std::endl;
    bool changed = true;
    
    while (changed) {
        changed = false;
        
        for (auto &bb : func_->get_basic_blocks()) {
            std::vector<Instruction*> to_remove;
            
            for (auto &instr : bb.get_instructions()) {
                if (auto *phi = dynamic_cast<PhiInst*>(&instr)) {
                    // 如果 PHI 没有操作数，直接删除
                    if (phi->get_num_operand() == 0) {
                        Type *phi_type = phi->get_type();
                        Value *default_val = ConstantZero::get(phi_type, func_->get_parent());
                        if (default_val) {
                            phi->replace_all_use_with(default_val);
                        }
                        to_remove.push_back(phi);
                        changed = true;
                        continue;
                    }
                    
                    // 检查是否是 trivial PHI
                    Value *same_value = nullptr;
                    bool is_trivial = true;
                    
                    // PHI 的操作数是成对出现的 (value, block)
                    for (size_t i = 0; i < phi->get_num_operand(); i += 2) {
                        if (i + 1 >= phi->get_num_operand()) {
                            // 操作数不成对，PHI 节点格式错误
                            is_trivial = false;
                            break;
                        }
                        
                        Value *val = phi->get_operand(i);
                        
                        if (val == phi) continue; // 跳过自引用
                        
                        if (same_value == nullptr) {
                            same_value = val;
                        } else if (same_value != val) {
                            is_trivial = false;
                            break;
                        }
                    }
                    
                    // 如果所有非自引用的值都相同，则是 trivial PHI
                    if (is_trivial && same_value) {
                        phi->replace_all_use_with(same_value);
                        to_remove.push_back(phi);
                        changed = true;
                    }
                }
            }
            
            // 删除 trivial PHI 节点
            for (auto *instr : to_remove) {
                // 从 phi_map 中移除
                phi_map.erase(instr);
                bb.erase_instr(instr);
            }
        }
    }
}

void Mem2Reg::remove_dead_allocas() {
    auto entry_bb = func_->get_entry_block();
    std::vector<Instruction*> to_remove;
    
    for (auto &inst : entry_bb->get_instructions()) {
        if (inst.is_alloca()) {
            // 检查这个 alloca 是否还有使用
            if (inst.get_use_list().empty()) {
                to_remove.push_back(&inst);
            }
        }
    }
    
    for (auto *inst : to_remove) {
        entry_bb->erase_instr(inst);
    }
}

// 判断是否为可提升的指针
bool Mem2Reg::is_valid_ptr(Value *v) {
    // 检查是否是 alloca 指令的结果
    if (!v || !v->get_type()->is_pointer_type()) {
        return false;
    }
    
    // 检查是否是 alloca 指令
    auto inst = dynamic_cast<Instruction*>(v);
    if (!inst || !inst->is_alloca()) {
        return false;
    }
    
    // 不处理全局变量和 GEP
    return !is_gep_instr(v) && !is_global_variable(v);
}

bool Mem2Reg::is_global_variable(Value *v) {
    return dynamic_cast<GlobalVariable *>(v) != nullptr;
}

bool Mem2Reg::is_gep_instr(Value *v) {
    return dynamic_cast<GetElementPtrInst *>(v) != nullptr;
}