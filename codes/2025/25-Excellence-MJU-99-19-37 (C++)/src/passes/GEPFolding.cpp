#include "GEPFolding.hpp"
#include "logging.hpp"
#include "IRBuilder.hpp"

#include <algorithm>
#include <unordered_set>


void GEPFolding::run() {
    bool changed = false;
    
    do {
        changed = false;
        for (auto &func : m_->get_functions()) {
            if (!func.is_declaration()) {
                changed |= runOnFunction(&func);
            }
        }
    } while (changed);
    
   // LOG_INFO << "GEP folding pass folded " << folded_count << " GEP instructions";
}

bool GEPFolding::runOnFunction(Function *func) {
    bool changed = false;
    
    for (auto &bb : func->get_basic_blocks()) {
        changed |= foldGEPChains(&bb);
        changed |= runOnBasicBlock(&bb);
        changed |= eliminateRedundantGEPs(&bb);
    }
    
    return changed;
}

bool GEPFolding::foldGEPChains(BasicBlock *bb) {
    bool changed = false;
    struct Replacement {
        GetElementPtrInst* old_gep;
        GetElementPtrInst* new_gep;
        GetElementPtrInst* base_gep;
    };
    std::vector<Replacement> replacements;
    
    // 查找 GEP 链
    for (auto &inst : bb->get_instructions()) {
        if (auto *gep = dynamic_cast<GetElementPtrInst*>(&inst)) {
            // 检查基址是否也是GEP指令
            if (auto *base_gep = dynamic_cast<GetElementPtrInst*>(gep->get_operand(0))) {
                // 检查是否可以合并
                bool can_merge = false;
                std::vector<Value*> combined_indices;
                
               
                for (size_t i = 1; i < base_gep->get_num_operand(); ++i) {
                    combined_indices.push_back(base_gep->get_operand(i));
                }
                
                // 检查当前GEP的第一个索引
                if (gep->get_num_operand() > 1) {
                    auto *first_idx = gep->get_operand(1);
                    if (auto *const_idx = dynamic_cast<ConstantInt*>(first_idx)) {
                        if (const_idx->get_value() == 0) {
                            // 可以合并，跳过第一个索引
                            can_merge = true;
                            for (size_t i = 2; i < gep->get_num_operand(); ++i) {
                                combined_indices.push_back(gep->get_operand(i));
                            }
                        }
                    }
                }
                
                if (can_merge && !combined_indices.empty()) {
                    // 创建临时基本块来创建新GEP
                    auto temp_bb = BasicBlock::create(m_, "", bb->get_parent());
                    
                    auto *new_gep = GetElementPtrInst::create_gep(
                        base_gep->get_operand(0), 
                        combined_indices,
                        temp_bb
                    );
                    
                    // 从临时基本块移除
                    temp_bb->get_instructions().remove(new_gep);
                    
                    // 删除临时基本块
                    bb->get_parent()->remove(temp_bb);
                    
                    replacements.push_back({gep, new_gep, base_gep});
                    changed = true;
                    folded_count++;
                }
            }
        }
    }
    
    // 执行替换
    for (auto &rep : replacements) {
        // 手动将新GEP插入到旧GEP之前
        auto &instrs = bb->get_instructions();
        
        // 手动查找旧GEP的位置
        auto it = instrs.end();
        for (auto iter = instrs.begin(); iter != instrs.end(); ++iter) {
            if (&*iter == rep.old_gep) {
                it = iter;
                break;
            }
        }
        
        if (it != instrs.end()) {
            // 在旧GEP之前插入新GEP
            instrs.insert(it, rep.new_gep);
            rep.new_gep->set_parent(bb);
            
            // 替换所有使用
            rep.old_gep->replace_all_use_with(rep.new_gep);
        }
    }
    
    // 删除旧的GEP指令
    for (auto &rep : replacements) {
        bb->erase_instr(rep.old_gep);
    }
    
    return changed;
}

bool GEPFolding::runOnBasicBlock(BasicBlock *bb) {
    bool changed = false;
    std::vector<Instruction*> to_remove;
    
    for (auto &inst : bb->get_instructions()) {
        if (auto *gep = dynamic_cast<GetElementPtrInst*>(&inst)) {
            // 如果所有索引都是0，GEP可以被消除
            if (allIndicesAreZero(gep)) {
                gep->replace_all_use_with(gep->get_operand(0));
                to_remove.push_back(gep);
                changed = true;
                folded_count++;
                continue;
            }
            
            // 合并相邻的常量索引
            if (canSimplifyGEP(gep)) {
                if (auto *simplified = simplifyGEP(gep, bb)) {
                    gep->replace_all_use_with(simplified);
                    to_remove.push_back(gep);
                    changed = true;
                    folded_count++;
                }
            }
        }
    }
    
    // 移除已被优化的GEP指令
    for (auto *inst : to_remove) {
        bb->erase_instr(inst);
    }
    
    return changed;
}

bool GEPFolding::allIndicesAreZero(GetElementPtrInst *gep) {
    for (size_t i = 1; i < gep->get_num_operand(); ++i) {
        if (auto *const_idx = dynamic_cast<ConstantInt*>(gep->get_operand(i))) {
            if (const_idx->get_value() != 0) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

bool GEPFolding::canSimplifyGEP(GetElementPtrInst *gep) {
    // 检查是否有连续的常量索引可以合并
    bool has_consecutive_constants = false;
    
    for (size_t i = 1; i < gep->get_num_operand() - 1; ++i) {
        if (dynamic_cast<ConstantInt*>(gep->get_operand(i)) &&
            dynamic_cast<ConstantInt*>(gep->get_operand(i + 1))) {
            has_consecutive_constants = true;
            break;
        }
    }
    
    return has_consecutive_constants;
}

Instruction *GEPFolding::simplifyGEP(GetElementPtrInst *gep, BasicBlock *bb) {
    
    return nullptr;
}

bool GEPFolding::canFoldGEP(GetElementPtrInst *gep) {
    if (!gep) return false;
    
    // 检查所有索引是否都是常量
    return allIndicesAreConstant(gep);
}

Constant *GEPFolding::foldGEP(GetElementPtrInst *gep) {
    // 实现主要在foldGEPChains中处理
    return nullptr;
}

bool GEPFolding::allIndicesAreConstant(GetElementPtrInst *gep) {
    // 跳过第一个操作数，检查所有索引（基指针）
    for (size_t i = 1; i < gep->get_num_operand(); ++i) {
        if (!dynamic_cast<Constant*>(gep->get_operand(i))) {
            return false;
        }
    }
    return true;
}

size_t GEPFolding::getTypeSize(Type *type) {
    if (type->is_int32_type()) {
        return 4;
    } else if (type->is_float_type()) {
        return 4;
    } else if (type->is_int1_type()) {
        return 1;
    } else if (type->is_array_type()) {
        auto *array_type = static_cast<ArrayType*>(type);
        return getArrayElementSize(array_type) * array_type->get_num_of_elements();
    } else if (type->is_pointer_type()) {
        return 8; // 64位指针
    }
    
    return 4; // 默认大小
}

size_t GEPFolding::getArrayElementSize(ArrayType *array_type) {
    return getTypeSize(array_type->get_element_type());
}

Constant *GEPFolding::createConstantGEP(Value *base, const std::vector<Constant*> &indices) {
    //没啥用不需要
    return nullptr;
}

bool GEPFolding::eliminateRedundantGEPs(BasicBlock *bb) {
    bool changed = false;
    std::vector<GetElementPtrInst*> geps;
    
    // 收集所有GEP指令
    for (auto &inst : bb->get_instructions()) {
        if (auto *gep = dynamic_cast<GetElementPtrInst*>(&inst)) {
            geps.push_back(gep);
        }
    }
    
    // 查找并消除冗余的GEP
    std::unordered_set<GetElementPtrInst*> to_remove;
    for (size_t i = 0; i < geps.size(); ++i) {
        if (to_remove.count(geps[i])) continue;
        
        for (size_t j = i + 1; j < geps.size(); ++j) {
            if (to_remove.count(geps[j])) continue;
            
            if (areGEPsEquivalent(geps[i], geps[j])) {
                // 用第一个GEP替换第二个
                geps[j]->replace_all_use_with(geps[i]);
                to_remove.insert(geps[j]);
                changed = true;
                folded_count++;
            }
        }
    }
    
    // 移除冗余的GEP指令
    for (auto *gep : to_remove) {
        bb->erase_instr(gep);
    }
    
    return changed;
}

bool GEPFolding::areGEPsEquivalent(GetElementPtrInst *gep1, GetElementPtrInst *gep2) {
    if (gep1->get_num_operand() != gep2->get_num_operand()) {
        return false;
    }
    
    // 检查基址是否相同
    if (gep1->get_operand(0) != gep2->get_operand(0)) {
        return false;
    }
    
    // 检查所有索引是否相同
    for (size_t i = 1; i < gep1->get_num_operand(); ++i) {
        if (gep1->get_operand(i) != gep2->get_operand(i)) {
            return false;
        }
    }
    
    return true;
}

Constant *GEPFolding::calculateGEPOffset(Type *base_type, const std::vector<Constant*> &indices) {
    size_t total_offset = 0;
    Type *current_type = base_type;
    
    for (size_t i = 0; i < indices.size(); ++i) {
        auto *index_const = dynamic_cast<ConstantInt*>(indices[i]);
        if (!index_const) return nullptr;
        
        int index_val = index_const->get_value();
        
        if (i == 0) {
            // 跳过指针解引用
            if (current_type->is_pointer_type()) {
                current_type = current_type->get_pointer_element_type();
                total_offset += index_val * getTypeSize(current_type);
            }
        } else {
            // 处理数组或结构体成员
            if (current_type->is_array_type()) {
                auto *array_type = static_cast<ArrayType*>(current_type);
                size_t element_size = getArrayElementSize(array_type);
                total_offset += index_val * element_size;
                current_type = array_type->get_element_type();
            } else {
                // 对于非数组类型，暂不处理
                return nullptr;
            }
        }
    }
    
    return ConstantInt::get(static_cast<int>(total_offset), m_);
}