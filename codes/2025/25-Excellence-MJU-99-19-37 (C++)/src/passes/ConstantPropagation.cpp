#include "ConstantPropagation.hpp"
#include "logging.hpp"
#include <vector>
#include <algorithm>

// ConstFolder 实现（失败，没看到效果）
ConstantInt *ConstFolder::compute(Instruction::OpID op, ConstantInt *value1, ConstantInt *value2) {
    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();

    switch (op) {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
    case Instruction::sdiv:
        if (c_value2 != 0) {
            return ConstantInt::get(c_value1 / c_value2, module_);
        }
        return nullptr;
    case Instruction::eq:
        return ConstantInt::get(c_value1 == c_value2, module_);
    case Instruction::ne:
        return ConstantInt::get(c_value1 != c_value2, module_);
    case Instruction::gt:
        return ConstantInt::get(c_value1 > c_value2, module_);
    case Instruction::ge:
        return ConstantInt::get(c_value1 >= c_value2, module_);
    case Instruction::lt:
        return ConstantInt::get(c_value1 < c_value2, module_);
    case Instruction::le:
        return ConstantInt::get(c_value1 <= c_value2, module_);
    default:
        return nullptr;
    }
}

ConstantFP *ConstFolder::compute(Instruction::OpID op, ConstantFP *value1, ConstantFP *value2) {
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    
    switch (op) {
    case Instruction::fadd:
        return ConstantFP::get(c_value1 + c_value2, module_);
    case Instruction::fsub:
        return ConstantFP::get(c_value1 - c_value2, module_);
    case Instruction::fmul:
        return ConstantFP::get(c_value1 * c_value2, module_);
    case Instruction::fdiv:
        if (c_value2 != 0.0f) {
            return ConstantFP::get(c_value1 / c_value2, module_);
        }
        return nullptr;
    case Instruction::feq:
        return ConstantFP::get(c_value1 == c_value2, module_);
    case Instruction::fne:
        return ConstantFP::get(c_value1 != c_value2, module_);
    case Instruction::fgt:
        return ConstantFP::get(c_value1 > c_value2, module_);
    case Instruction::fge:
        return ConstantFP::get(c_value1 >= c_value2, module_);
    case Instruction::flt:
        return ConstantFP::get(c_value1 < c_value2, module_);
    case Instruction::fle:
        return ConstantFP::get(c_value1 <= c_value2, module_);
    default:
        return nullptr;
    }
}

ConstantFP *ConstFolder::compute(Instruction::OpID op, ConstantInt *value1) {
    int c_value1 = value1->get_value();

    switch (op) {
    case Instruction::sitofp:
        return ConstantFP::get((float)c_value1, module_);
    default:
        return nullptr;
    }
}

ConstantInt *ConstFolder::compute(Instruction::OpID op, ConstantFP *value1) {
    float c_value1 = value1->get_value();
    
    switch (op) {
    case Instruction::fptosi:
        return ConstantInt::get(static_cast<int>(c_value1), module_);
    default:
        return nullptr;
    }
}

// 辅助函数
ConstantFP *cast_constantfp(Value *value) {
    return dynamic_cast<ConstantFP *>(value);
}

ConstantInt *cast_constantint(Value *value) {
    return dynamic_cast<ConstantInt *>(value);
}

// ConstantPropagation 实现（失败，深层函数未提取）
void ConstantPropagation::run() {
    LOG(INFO) << "Running constant propagation optimization";
    
    // 收集全局常量
    std::unordered_map<GlobalVariable*, Constant*> global_const_map;
    for (auto &gvar : m_->get_global_variable()) {
        if (gvar.is_const() && gvar.get_init()) {
            global_const_map[&gvar] = gvar.get_init();
        }
    }
    
    for (auto &func : m_->get_functions()) {
        for (auto &bb : func.get_basic_blocks()) {
            wait_delete_.clear();

            for (auto &instr : bb.get_instructions()) {
                // 处理从全局常量的load
                if (instr.is_load()) {
                    auto *load_inst = static_cast<LoadInst*>(&instr);
                    auto *ptr = load_inst->get_lval();
                    
                    // 检查是否直接从全局常量加载
                    if (auto *gvar = dynamic_cast<GlobalVariable*>(ptr)) {
                        auto it = global_const_map.find(gvar);
                        if (it != global_const_map.end()) {
                            instr.replace_all_use_with(it->second);
                            wait_delete_.push_back(&instr);
                            propagated_count_++;
                            continue;
                        }
                    }
                    
                    
                    if (auto *gep = dynamic_cast<GetElementPtrInst*>(ptr)) {
                        if (auto *gvar = dynamic_cast<GlobalVariable*>(gep->get_operand(0))) {
                            auto it = global_const_map.find(gvar);
                            if (it != global_const_map.end()) {
                                
                                bool all_const_indices = true;
                                std::vector<int> indices;
                                
                                for (size_t i = 1; i < gep->get_num_operand(); ++i) {
                                    if (auto *const_idx = cast_constantint(gep->get_operand(i))) {
                                        indices.push_back(const_idx->get_value());
                                    } else {
                                        all_const_indices = false;
                                        break;
                                    }
                                }
                                
                                if (all_const_indices && !indices.empty()) {
                                    
                                    Constant *element = it->second;
                                    
                                    for (size_t i = 0; i < indices.size(); ++i) {
                                        if (auto *const_array = dynamic_cast<ConstantArray*>(element)) {
                                            if (indices[i] >= 0 && 
                                                indices[i] < static_cast<int>(const_array->get_size_of_array())) {
                                                element = const_array->get_element_value(indices[i]);
                                            } else {
                                                element = nullptr;
                                                break;
                                            }
                                        } else if (i == 0 && indices[i] == 0) {
                                            // 第一个索引为0时，可能是指针解引用
                                            continue;
                                        } else {
                                            element = nullptr;
                                            break;
                                        }
                                    }
                                    
                                    if (element) {
                                        instr.replace_all_use_with(element);
                                        wait_delete_.push_back(&instr);
                                        propagated_count_++;
                                        continue;
                                    }
                                }
                            }
                        }
                    }
                }
                // 处理整数二元运算
                else if (instr.is_add() || instr.is_sub() || instr.is_mul() || instr.is_div()) {
                    auto value1 = cast_constantint(instr.get_operand(0));
                    auto value2 = cast_constantint(instr.get_operand(1));
                    if (value1 && value2) {
                        auto fold_const = folder_->compute(instr.get_instr_type(), value1, value2);
                        if (fold_const) {
                            instr.replace_all_use_with(fold_const);
                            wait_delete_.push_back(&instr);
                            propagated_count_++;
                        }
                    }
                }
                // 处理浮点二元运算
                else if (instr.is_fadd() || instr.is_fsub() || instr.is_fmul() || instr.is_fdiv()) {
                    auto value1 = cast_constantfp(instr.get_operand(0));
                    auto value2 = cast_constantfp(instr.get_operand(1));
                    if (value1 && value2) {
                        auto fold_const = folder_->compute(instr.get_instr_type(), value1, value2);
                        if (fold_const) {
                            instr.replace_all_use_with(fold_const);
                            wait_delete_.push_back(&instr);
                            propagated_count_++;
                        }
                    }
                }
                else if (instr.is_si2fp()) {
                    auto value1 = cast_constantint(instr.get_operand(0));
                    if (value1) {
                        auto fold_const = folder_->compute(instr.get_instr_type(), value1);
                        if (fold_const) {
                            instr.replace_all_use_with(fold_const);
                            wait_delete_.push_back(&instr);
                            propagated_count_++;
                        }
                    }
                } 
                else if (instr.is_fp2si()) {
                    auto value1 = cast_constantfp(instr.get_operand(0));
                    if (value1) {
                        auto fold_const = folder_->compute(instr.get_instr_type(), value1);
                        if (fold_const) {
                            instr.replace_all_use_with(fold_const);
                            wait_delete_.push_back(&instr);
                            propagated_count_++;
                        }
                    }
                }
                // 处理整数比较
                else if (instr.is_cmp()) {
                    auto value1 = cast_constantint(instr.get_operand(0));
                    auto value2 = cast_constantint(instr.get_operand(1));
                    if (value1 && value2) {
                        auto fold_const = folder_->compute(instr.get_instr_type(), value1, value2);
                        if (fold_const) {
                            instr.replace_all_use_with(fold_const);
                            wait_delete_.push_back(&instr);
                            propagated_count_++;
                        }
                    }
                }
                // 处理浮点比较
                else if (instr.is_fcmp()) {
                    auto value1 = cast_constantfp(instr.get_operand(0));
                    auto value2 = cast_constantfp(instr.get_operand(1));
                    if (value1 && value2) {
                        auto fold_const = folder_->compute(instr.get_instr_type(), value1, value2);
                        if (fold_const) {
                            ConstantInt *result;
                            if (fold_const == ConstantFP::get(0, m_)) {
                                result = ConstantInt::get(0, m_);
                            } else {
                                result = ConstantInt::get(1, m_);
                            }
                            instr.replace_all_use_with(result);
                            wait_delete_.push_back(&instr);
                            propagated_count_++;
                        }
                    }
                }
                // 处理phi节点
                else if (instr.is_phi() && instr.get_operands().size() == 4) {
                    //整数
                    auto int_value1 = cast_constantint(instr.get_operand(0));
                    auto int_value2 = cast_constantint(instr.get_operand(2));
                    if (int_value1 && int_value2) {
                        if (int_value1->get_value() == int_value2->get_value()) {
                            instr.replace_all_use_with(instr.get_operand(0));
                            wait_delete_.push_back(&instr);
                            propagated_count_++;
                        }
                    } else {
                        // 浮点
                        auto fp_value1 = cast_constantfp(instr.get_operand(0));
                        auto fp_value2 = cast_constantfp(instr.get_operand(2));
                        if (fp_value1 && fp_value2) {
                            if (fp_value1->get_value() == fp_value2->get_value()) {
                                instr.replace_all_use_with(instr.get_operand(0));
                                wait_delete_.push_back(&instr);
                                propagated_count_++;
                            }
                        }
                    }
                }
            }
            
            // 批量删除指令
            for (auto instr : wait_delete_) {
                bb.erase_instr(instr);
            }
        }
    }

    //条件分支优化
    for (auto &func : m_->get_functions()) {
        // 记录需要清理的不可达基本块
        std::set<BasicBlock*> unreachable_blocks;
        
        for (auto &bb : func.get_basic_blocks()) {
            builder_->set_insert_point(&bb);
            auto terminator = bb.get_terminator();
            
            if (terminator && terminator->is_br() && terminator->get_operands().size() == 3) {
                auto cond_br = static_cast<BranchInst*>(terminator);
                auto cond = cond_br->get_operand(0);
                auto cond_const = cast_constantint(cond);
                
                if (cond_const) {
                    auto ops = cond_br->get_operands();
                    BasicBlock* target_bb;
                    BasicBlock* removed_bb;
                    
                    if (cond_const->get_value() != 0) {
                        target_bb = static_cast<BasicBlock*>(ops[1]);
                        removed_bb = static_cast<BasicBlock*>(ops[2]);
                    } else {
                        target_bb = static_cast<BasicBlock*>(ops[2]);
                        removed_bb = static_cast<BasicBlock*>(ops[1]);
                    }
                    
                    // 从被移除分支的前驱列表中移除当前基本块
                    removed_bb->remove_pre_basic_block(&bb);
                    
                    std::vector<Instruction*> phi_to_update;
                    for (auto &inst : removed_bb->get_instructions()) {
                        if (inst.is_phi()) {
                            phi_to_update.push_back(&inst);
                        }
                    }
                    
                    for (auto phi : phi_to_update) {
                        for (int i = phi->get_num_operand() - 2; i >= 0; i -= 2) {
                            if (phi->get_operand(i + 1) == &bb) {
                                phi->remove_operand(i);
                                phi->remove_operand(i);
                                break;
                            }
                        }
                    }
                    
                    // 删除条件分支，创建无条件分支
                    bb.erase_instr(cond_br);
                    builder_->create_br(target_bb);
                    
                    // 如果被移除的分支没有其他前驱，标记为不可达
                    if (removed_bb->get_pre_basic_blocks().empty() && !is_entry(removed_bb)) {
                        unreachable_blocks.insert(removed_bb);
                    }
                    
                    propagated_count_++;
                }
            }
        }
        
        // 清理不可达基本块
        while (!unreachable_blocks.empty()) {
            std::set<BasicBlock*> new_unreachable;
            
            for (auto bb : unreachable_blocks) {
                // 处理后继基本块
                auto succ_blocks = bb->get_succ_basic_blocks();
                for (auto succ : succ_blocks) {
                    // 从后继块的前驱列表中移除
                    succ->remove_pre_basic_block(bb);
                    
                    // 更新后继块的phi节点
                    std::vector<Instruction*> phi_to_update;
                    for (auto &inst : succ->get_instructions()) {
                        if (inst.is_phi()) {
                            phi_to_update.push_back(&inst);
                        }
                    }
                    
                    for (auto phi : phi_to_update) {
                        for (int i = phi->get_num_operand() - 2; i >= 0; i -= 2) {
                            if (phi->get_operand(i + 1) == bb) {
                                phi->remove_operand(i);
                                phi->remove_operand(i);
                                break;
                            }
                        }
                        
                        // 如果phi只剩一个操作数，用该操作数替换phi
                        if (phi->get_num_operand() == 2) {
                            auto value = phi->get_operand(0);
                            phi->replace_all_use_with(value);
                            succ->erase_instr(phi);
                        }
                    }
                    
                    // 如果后继块也变成不可达，加入待处理集合
                    if (succ->get_pre_basic_blocks().empty() && !is_entry(succ)) {
                        new_unreachable.insert(succ);
                    }
                }
                
                // 从函数中移除不可达基本块
                func.remove(bb);
            }
            
            unreachable_blocks = new_unreachable;
        }
    }
    
    //LOG(INFO) << "Constant propagation: propagated " << propagated_count_ << " constants";
}

bool ConstantPropagation::is_entry(BasicBlock *bb) {
    return bb == bb->get_parent()->get_entry_block();
}

void ConstantPropagation::clear_blocks_recs(BasicBlock *start_bb) {
    auto func = start_bb->get_parent();
    if (!func) return;
    
    auto prev_bb = start_bb->get_pre_basic_blocks();
    
    // 如果基本块没有前驱且不是入口块，则可以删除
    if (prev_bb.empty() && !is_entry(start_bb)) {
        func->remove(start_bb);
        auto succ_bb = start_bb->get_succ_basic_blocks();
        
        for (auto each_succ_bb : succ_bb) {
            std::vector<Instruction*> del_inst;
            
            // 更新后继块的phi节点
            for (auto &instr : each_succ_bb->get_instructions()) {
                if (instr.is_phi()) {
                    for (unsigned int i = 1; i < instr.get_num_operand(); i += 2) {
                        if (instr.get_operand(i) == start_bb && 
                            start_bb->get_pre_basic_blocks().size() <= 0) {
                            instr.remove_operand(i - 1);
                            instr.remove_operand(i - 1);
                        }
                    }
                    
                    // 如果phi只剩一个操作数，用该操作数替换phi
                    if (instr.get_num_operand() == 2) {
                        auto value = instr.get_operand(0);
                        instr.replace_all_use_with(value);
                        del_inst.push_back(&instr);
                    }
                }
            }
            
            // 删除无用的phi节点
            for (auto instr : del_inst) {
                each_succ_bb->erase_instr(instr);
            }
            
            // 递归处理后继块
            clear_blocks_recs(each_succ_bb);
        }
    }
}