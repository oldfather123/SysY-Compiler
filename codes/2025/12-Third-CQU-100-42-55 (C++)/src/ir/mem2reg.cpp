#include "../../include/ir/mem2reg.h"
#include "../../include/ir/cfg.h"
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <memory>
#include <algorithm>
#include <iterator>
#include <unordered_map>

namespace llvm_ir {
namespace mem2reg {

// ================= Mem2Reg Implementation =================

bool Mem2Reg::runOnModule(Module& M) {
    bool changed = false;
    for (auto& F : M.functions) {
        if (!F->isDeclaration()) {
            changed |= runOnFunction(*F);
        }
    }
    
    return changed;
}

bool Mem2Reg::runOnFunction(Function& F) {
    promotableAllocas.clear();
    deadInstructions.clear();
    valueStacks.clear();
    phiToAlloca.clear();
    phiNameCounter.clear();
    
    std::cout << "[mem2reg] 处理函数: " << F.name << std::endl;
    
    // 重新建立use-def关系，解决悬空指针问题
    cfg::rebuildUseDefChainsOnFunction(F, true);
    // 1. Find promotable alloca instructions
    findPromotableAllocas(F);
    if (promotableAllocas.empty()) {
        std::cout << "[mem2reg] 无可提升的alloca，跳过。" << std::endl;
        return false;
    }
    // std::cout << "[mem2reg] 可提升的alloca: ";
    for (auto a : promotableAllocas) std::cout << a->getName() << " ";
    std::cout << std::endl;
    // 2. Compute Dominator Tree and Dominance Frontiers
    cfg::DominatorTree DT;
    DT.runOnFunction(F);

    // 3. Place PHI nodes
    placePhiNodes(F, DT);

    // 4. Rename variables using a walk of the dominator tree
    renameVariables(F, DT);
    // 5. Cleanup dead instructions

    fixupIncompletePhis(F); // 修复不完整的PHI节点

    cleanup(F);
    // std::cout << "[mem2reg] 完成函数: " << F.name << std::endl;

    cfg::rebuildUseDefChainsOnFunction(F, false);

    return true;
}

void Mem2Reg::findPromotableAllocas(Function& F) {
    BasicBlock* entry = F.getEntryBlock();
    if (!entry) return;
    for (auto& inst : entry->instructions) {
        if (auto alloca = dynamic_cast<AllocaInst*>(inst.get())) {
            // 只要不是数组分配都尝试提升
            // std::cout << "[mem2reg] 检查alloca: " << alloca->getName() << ", arraySize=" << alloca->arraySize << std::endl;
            if (alloca->arraySize <= 1) {
                promotableAllocas.push_back(alloca);
            }
        }
    }
}

void Mem2Reg::renameVariables(Function& F, cfg::DominatorTree& DT) {
    // std::cout << "[mem2reg] renameVariables 开始" << std::endl;
    for (auto alloca : promotableAllocas) {
        valueStacks[alloca].push(new UndefValue(alloca->allocatedType));
        // std::cout << "[mem2reg] 初始化栈: " << alloca->getName() << std::endl;
    }
    renameRecursive(F.getEntryBlock(), DT);
    // std::cout << "[mem2reg] renameVariables 结束" << std::endl;
}

void Mem2Reg::renameRecursive(BasicBlock* B, cfg::DominatorTree& DT) {
    if (!B) return;
    // std::cout << "[mem2reg] 进入BasicBlock: %" << B->label << std::endl;
    std::unordered_map<AllocaInst*, int> pushesCount;
    
    // 1. 处理此块中的PHI节点，立即入栈
    for(auto& phi : B->phi_instructions){
        // std::cout << "  [mem2reg] 处理phi: " << phi->getName() << std::endl;
        auto it = phiToAlloca.find(phi.get());
        if(it != phiToAlloca.end()){
            AllocaInst* alloca = it->second;
            valueStacks[alloca].push(phi.get());
            pushesCount[alloca]++;
            // std::cout << "    [mem2reg] phi入栈: " << alloca->getName() << std::endl;
        }
    }
    
    // 2. Process regular instructions in this block
    for (auto it = B->instructions.begin(); it != B->instructions.end(); ++it) {
        Instruction* inst = it->get();
        // std::cout << "  [mem2reg] 处理指令: " << inst->toString() << std::endl;

        if (auto load = dynamic_cast<LoadInst*>(inst)) {
            if (auto alloca = dynamic_cast<AllocaInst*>(load->getPointerOperand())) {
                auto stack_it = valueStacks.find(alloca);
                if (stack_it != valueStacks.end() && !stack_it->second.empty()) {
                    // std::cout << "    [mem2reg] load 替换: " << load->toString() << " -> " << stack_it->second.top()->toString() << std::endl;
                    Value* topVal = stack_it->second.top();

                    // 彻底清理use-def关系：从所有操作数的uses列表中移除该指令
                    for (auto operand : load->operands) {
                        if (operand) {
                            operand->removeUse(load);
                        }
                    }

                    // 现在可以安全地替换和标记
                    load->replaceAllUsesWith(topVal);
                    deadInstructions.insert(load);
                }
            }
        } else if (auto store = dynamic_cast<StoreInst*>(inst)) {
            if (auto alloca = dynamic_cast<AllocaInst*>(store->getPointerOperand())) {
                auto stack_it = valueStacks.find(alloca);
                if (stack_it != valueStacks.end()) {
                    Value* valToPush = store->getValueOperand();
                    if (!valToPush) {
                        std::cout << "[WARNING] store 指令 " << store->toString() << " 的值为空，用undef替代。" << std::endl;
                        valToPush = new UndefValue(alloca->allocatedType); 
                    }
                    // std::cout << "    [mem2reg] store 入栈: " << valToPush->toString() << " for " << alloca->getName() << std::endl;
                    stack_it->second.push(valToPush);
                    pushesCount[alloca]++;

                    // 彻底清理use-def关系：从所有操作数的uses列表中移除该指令
                    for (auto operand : store->operands) {
                        if (operand) {
                            operand->removeUse(store);
                        }
                    }
                    
                    // 现在可以安全地标记
                    deadInstructions.insert(store);
                }
            }
        }
    }

    // 3. Fill PHI nodes in successors
    for (auto succ : B->successors) {
        // std::cout << "  [mem2reg] 处理后继: %" << succ->label << std::endl;
        for(auto& phi_ptr : succ->phi_instructions){
            auto it = phiToAlloca.find(phi_ptr.get());
            if (it != phiToAlloca.end()) {
                AllocaInst* alloca = it->second;
                auto valueIt = valueStacks.find(alloca);
                if (valueIt != valueStacks.end() && !valueIt->second.empty()) {
                    // std::cout << "    [mem2reg] phi addIncoming: " << valueIt->second.top()->toString() << " from %" << B->label << std::endl;
                    phi_ptr->addIncoming(valueIt->second.top(), B);
                }
            }
        }
    }

    // 4. Recurse on children in the dominator tree
    const auto& children = DT.getChildren(B);
    std::set<BasicBlock*> processed;
    for (auto child : children) {
        if (processed.insert(child).second) {
            renameRecursive(child, DT);
        }
    }
    // 处理循环回边：如果后继不是孩子且支配B
    for (auto succ : B->successors) {
        if (std::find(children.begin(), children.end(), succ) == children.end() && DT.dominates(B, succ)) {
            if (processed.insert(succ).second) {
                renameRecursive(succ, DT);
            }
        }
    }

    // 5. Pop values from stacks to backtrack
    for (auto const& [alloca, count] : pushesCount) {
        for (int i = 0; i < count; ++i) {
            valueStacks[alloca].pop();
            // std::cout << "    [mem2reg] 回溯出栈: " << alloca->getName() << std::endl;
        }
    }
}

void Mem2Reg::placePhiNodes(Function& F, cfg::DominatorTree& DT) {
    std::cout << "[mem2reg] placePhiNodes 开始" << std::endl;
    std::unordered_map<AllocaInst*, std::set<BasicBlock*>> defBlocks;
    for (auto alloca : promotableAllocas) {
        defBlocks[alloca] = std::set<BasicBlock*>();
    }
    // Find all blocks that define (store to) each alloca
    for (auto& bb : F.basicBlocks) {
        // std::cout << "[mem2reg] 检查 basicblock: %" << bb->label << std::endl;
        for (auto& inst : bb->instructions) {
            // std::cout << "  指令: " << inst->toString() << std::endl;
            if (auto store = dynamic_cast<StoreInst*>(inst.get())) {
                if (auto alloca = dynamic_cast<AllocaInst*>(store->getPointerOperand())) {
                    if (std::find(promotableAllocas.begin(), promotableAllocas.end(), alloca) != promotableAllocas.end()) {
                        defBlocks[alloca].insert(bb.get());
                        // std::cout << "    [mem2reg] store 到可提升 alloca: " << alloca->getName() << std::endl;
                    }
                }
            }
        }
    }

    for (auto alloca : promotableAllocas) {
        std::set<BasicBlock*>& defs = defBlocks[alloca];
        std::set<BasicBlock*> phiPlaced;
        std::vector<BasicBlock*> worklist(defs.begin(), defs.end());
        while (!worklist.empty()) {
            BasicBlock* B = worklist.back();
            worklist.pop_back();
            const auto& DF = DT.getDominanceFrontier(B);
            // std::cout << "[mem2reg] block %" << B->label << " 的DF: ";
            // for (auto d : DF) std::cout << d->label << " ";
            // std::cout << std::endl;
            for (auto dfNode : DF) {
                if (phiPlaced.find(dfNode) == phiPlaced.end()) {
                    // 为每个PHI节点生成唯一名称
                    int phiId = phiNameCounter[alloca]++;
                    std::string phiName = "%" + alloca->getName().substr(1) + "_phi_" + std::to_string(phiId);
                    auto phi = std::make_unique<PhiInst>(alloca->allocatedType, phiName);
                    
                    // 建立PHI节点与alloca的映射关系
                    PhiInst* phiPtr = phi.get();
                    phiToAlloca[phiPtr] = alloca;
                    
                    // std::cout << "[mem2reg] 插入phi: " << phi->getName() << " in block %" << dfNode->label << std::endl;
                    dfNode->addPhi(std::move(phi));
                    phiPlaced.insert(dfNode);
                    bool inWorklist = false;
                    for(auto w : worklist) if (w == dfNode) inWorklist = true;
                    if(!inWorklist) worklist.push_back(dfNode);
                }
            }
        }
    }
    // std::cout << "[mem2reg] placePhiNodes 结束" << std::endl;
}

void Mem2Reg::cleanup(Function& F) {
    std::cout << "[mem2reg] cleanup 开始" << std::endl;
    
    // 清理dead instructions的use-def关系
    for (auto deadInst : deadInstructions) {
        // std::cout << "[mem2reg] 清理死指令的use-def关系: " << deadInst->toString() << std::endl;
        
        // 从所有操作数的uses列表中移除这个指令
        for (auto operand : deadInst->operands) {
            if (operand) {
                operand->removeUse(deadInst);
            }
        }
        
        // 如果是PHI指令，还需要清理incoming_values
        if (auto phi = dynamic_cast<PhiInst*>(deadInst)) {
            for (auto& incoming : phi->incoming_values) {
                if (incoming.first) {
                    incoming.first->removeUse(phi);
                }
            }
        }
        
        // 清空该指令的uses列表
        deadInst->getUses().clear();
    }
    
    // 删除dead instructions
    for (auto& bb : F.basicBlocks) {
        // std::cout << "[mem2reg] 清理 basicblock: %" << bb->label << std::endl;
        auto& insts = bb->instructions;
        insts.erase(std::remove_if(insts.begin(), insts.end(),
            [&](const std::unique_ptr<Instruction>& inst) {
                if (deadInstructions.count(inst.get())) {
                    // std::cout << "[mem2reg] 删除指令: " << inst->toString() << std::endl;
                    return true;
                }
                return false;
            }), insts.end());
    }
    
    // 清理promotable allocas
    BasicBlock* entry = F.getEntryBlock();
    if (entry) {
        auto& insts = entry->instructions;
        insts.erase(std::remove_if(insts.begin(), insts.end(),
            [&](const std::unique_ptr<Instruction>& inst) {
                if (auto alloca = dynamic_cast<AllocaInst*>(inst.get())) {
                    if (std::find(promotableAllocas.begin(), promotableAllocas.end(), alloca) != promotableAllocas.end()) {

                    // 安全检查 
                    if (!alloca->getUses().empty()) {
                        std::cerr << "[ERROR] Mem2Reg Bug Detected: Promotable alloca '" 
                                << alloca->toString()
                                << "' is being deleted but still has users." << std::endl;
                        for (auto use : alloca->getUses()) {
                            std::cerr << "  - Unhandled User: " << use->toString() << std::endl;
                        }
                        return false;
                    }

                        // std::cout << "[mem2reg] 清理alloca的use-def关系: " << alloca->toString() << std::endl;
                        
                        // 从所有操作数的uses列表中移除这个alloca（如果有的话）
                        for (auto operand : alloca->operands) {
                            if (operand) {
                                operand->removeUse(alloca);
                            }
                        }
                        
                        // 清空该alloca的uses列表
                        alloca->getUses().clear();
                        
                        // std::cout << "[mem2reg] 删除alloca: " << alloca->toString() << std::endl;
                        return true;
                    }
                }
                return false;
            }), insts.end());
    }
    for (auto& bb : F.basicBlocks) {
        auto& phis = bb->phi_instructions;
        phis.erase(std::remove_if(phis.begin(), phis.end(),
            [](const std::unique_ptr<PhiInst>& phi) {
                if (phi->getUses().empty()) {
                    // std::cout << "[mem2reg] 删除无用PHI: " << phi->getName() << std::endl;
                    // 清理incoming_values的use-def关系
                    for (auto& incoming : phi->incoming_values) {
                        if (incoming.first) {
                            incoming.first->removeUse(phi.get());
                        }
                    }
                    return true;
                }
                return false;
            }), phis.end());
    }
    // std::cout << "[mem2reg] cleanup 结束" << std::endl;
}

// 临时解决方案，有用别删
void fixupIncompletePhis(Function& F) {
    for (auto& bb : F.basicBlocks) {
        // Collect a map of predecessors for fast lookup
        std::set<BasicBlock*> preds(bb->predecessors.begin(), bb->predecessors.end());

        for (auto& phi : bb->phi_instructions) {
            if (phi->incoming_values.size() == preds.size()) {
                continue; // This PHI is already correct
            }

            std::set<BasicBlock*> phiPreds;
            for (const auto& incoming : phi->incoming_values) {
                phiPreds.insert(incoming.second);
            }

            // Find which predecessors are missing from the PHI
            for (BasicBlock* pred : preds) {
                if (phiPreds.find(pred) == phiPreds.end()) {
                    // This predecessor is in the CFG but not in the PHI.
                    // This happens if 'pred' is unreachable.
                    // Add an 'undef' value to make the IR valid.
                    std::cout << "[mem2reg-fixup] Patching PHI " << phi->getName() 
                              << " in block %" << bb->label 
                              << " with undef for dead predecessor %" << pred->label << std::endl;
                    phi->addIncoming(new UndefValue(phi->type), pred);
                }
            }
        }
    }
}


} // namespace mem2reg
} // namespace llvm_ir