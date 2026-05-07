#include "../../include/ir/PHIElimination.h"
#include "../../include/ir/mem2reg.h"
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <iostream>
#include <algorithm>

namespace llvm_ir {
namespace phi_elimination {

using namespace std;

bool DemoteAllPHIs(Module& M) {
    std::cout << "[PHIElimination] DemoteAllPHIs" << std::endl;
    bool changed = false;
    for (auto& func_ptr : M.functions) {
        Function& F = *func_ptr;
        if (F.isDeclaration()) continue;

        // 1. 收集所有PHI节点
        std::vector<PhiInst*> phis_to_demote;
        for (auto& bb_ptr : F.basicBlocks) {
            BasicBlock* BB = bb_ptr.get();
            for (auto& phi_uptr : BB->phi_instructions) {
                PhiInst* phi = phi_uptr.get();
                if (!phi->getUses().empty() || !phi->incoming_values.empty()) {
                    phis_to_demote.push_back(phi);
                }
            }
        }
        if (phis_to_demote.empty()) continue;
        changed = true;
        std::cout << "[PHIElimination] phis_to_demote.size() = " << phis_to_demote.size() << std::endl;

        // 2. 在入口块插入alloca
        BasicBlock* entry = F.getEntryBlock();
        auto& entry_insts = entry->instructions;
        auto insert_pos = entry_insts.begin();
        while (insert_pos != entry_insts.end()) {
            if ((*insert_pos)->opcode != Opcode::Alloca) break;
            ++insert_pos;
        }
        std::map<PhiInst*, AllocaInst*> phi_to_alloca;
        for (auto* phi : phis_to_demote) {
            std::string alloca_name = phi->name + "_mem";
            auto alloca = std::make_unique<AllocaInst>(phi->type, alloca_name);
            AllocaInst* alloca_ptr = alloca.get();
            alloca.get()->parent = entry;
            entry_insts.insert(insert_pos, std::move(alloca));
            phi_to_alloca[phi] = alloca_ptr;
        }
        std::cout << "[PHIElimination] insert alloca done" << std::endl;

        // 3. 处理每个PHI节点
        for (auto* phi : phis_to_demote) {
            // 检查 PHI 节点是否仍然存在（可能已被其他优化删除）
            bool phi_still_exists = false;
            for (auto& bb_ptr : F.basicBlocks) {
                for (auto& phi_ptr : bb_ptr->phi_instructions) {
                    if (phi_ptr.get() == phi) {
                        phi_still_exists = true;
                        break;
                    }
                }
                if (phi_still_exists) break;
            }
            
            if (!phi_still_exists) {
                std::cout << "[PHIElimination] PHI node " << phi << " already deleted by other optimization, skipping" << std::endl;
                continue;
            }
            
            std::cout << "[PHIElimination] processing phi = " << phi->toString() << std::endl;
            AllocaInst* alloca_for_phi = phi_to_alloca[phi];
            std::cout << "[PHIElimination] alloca_for_phi = " << alloca_for_phi->toString() << std::endl;
            // 找到phi块第一个非PHI指令
            BasicBlock* phi_bb = phi->parent;
            // auto& phi_bb_phis = phi_bb->phi_instructions;
            auto& phi_bb_insts = phi_bb->instructions;
            std::cout << "[PHIElimination] phi_bb_insts.size() = " << phi_bb_insts.size() << std::endl;
            auto load_insert = phi_bb_insts.begin();
            // 跳过所有PHI
            while (load_insert != phi_bb_insts.end() && (*load_insert)->opcode == Opcode::PHI) ++load_insert;
            std::cout << "[PHIElimination] load_insert = " << (*load_insert)->toString() << std::endl;
            // 插入load
            std::string load_name = phi->name + "_rel";
            auto load = std::make_unique<LoadInst>(alloca_for_phi, load_name, phi->type);
            load.get()->parent = phi_bb;
            LoadInst* load_ptr = load.get();
            phi_bb_insts.insert(load_insert, std::move(load));
            std::cout << "[PHIElimination] insert load" << std::endl;
            
            // 清理 PHI 节点的 uses 列表，移除已经被删除的指令
            auto& phi_uses = phi->getUses();
            std::vector<Instruction*> valid_uses;
            for (auto* user : phi_uses) {
                bool user_still_exists = false;
                // 检查这个使用者是否还存在于某个基本块中
                for (auto& bb_ptr : F.basicBlocks) {
                    // 检查普通指令
                    for (auto& inst_ptr : bb_ptr->instructions) {
                        if (inst_ptr.get() == user) {
                            user_still_exists = true;
                            break;
                        }
                    }
                    if (user_still_exists) break;
                    
                    // 检查 PHI 指令
                    for (auto& phi_ptr : bb_ptr->phi_instructions) {
                        if (phi_ptr.get() == user) {
                            user_still_exists = true;
                            break;
                        }
                    }
                    if (user_still_exists) break;
                }
                
                if (user_still_exists) {
                    valid_uses.push_back(user);
                } else {
                    std::cout << "[PHIElimination] Removing dangling use from PHI uses list" << std::endl;
                }
            }
            phi_uses = valid_uses;
            
            // 替换所有对phi的使用
            phi->replaceAllUsesWith(load_ptr);
            std::cout << "[PHIElimination] replace all uses with load" << std::endl;
            // 3c. 在每个前驱插入store
            for (const auto& incoming : phi->incoming_values) {
                Value* incoming_val = incoming.first;
                BasicBlock* pred = incoming.second;
                
                // 检查是否为undef，如果是则替换为0
                if (dynamic_cast<UndefValue*>(incoming_val)) {
                    std::cout << "[PHIElimination] Replacing undef value with 0 in phi incoming" << std::endl;
                    // 根据PHI节点的类型创建对应的0值
                    if (phi->type == Type::I1 || phi->type == Type::I8 || phi->type == Type::I32 || phi->type == Type::I64) {
                        incoming_val = new ConstantInt(0, phi->type);
                    } else if (phi->type == Type::Float || phi->type == Type::Double) {
                        incoming_val = new ConstantFloat(0.0, phi->type);
                    } else {
                        std::cout << "[PHIElimination] Warning: Cannot create zero value for unsupported type, skipping" << std::endl;
                        continue;
                    }
                } 
                
                // 插入在terminator之前
                auto& pred_insts = pred->instructions;
                auto term_pos = pred_insts.end();
                if (!pred_insts.empty()) {
                    auto last = pred_insts.end(); --last;
                    if ((*last)->opcode == Opcode::Br || (*last)->opcode == Opcode::Ret || (*last)->opcode == Opcode::Switch) {
                        term_pos = last;
                    }
                }
                auto store = std::make_unique<StoreInst>(incoming_val, alloca_for_phi);
                store.get()->parent = pred;
                pred_insts.insert(term_pos, std::move(store));
            }
        }
        std::cout << "[PHIElimination] insert store done" << std::endl;
        // 4. 删除所有原始PHI节点
        for (auto& bb_ptr : F.basicBlocks) {
            auto& phis = bb_ptr->phi_instructions;
            phis.erase(std::remove_if(phis.begin(), phis.end(), [](const std::unique_ptr<PhiInst>& phi) {
                if (phi->getUses().empty()) return true;
                if (!phi->getUses().empty()) {
                    std::cerr << "[PHIElimination] PHI node still has uses after demotion!" << std::endl;
                    assert(false);
                }
                return false;
            }), phis.end());
        }
        std::cout << "[PHIElimination] delete phi done" << std::endl;
    }
    return changed;
}

} // namespace phi_elimination
} // namespace llvm_ir 