#include "gcm.h"

#include <algorithm>
#include <iostream>
#include <queue>

namespace llvm_ir {
namespace gcm {

// Module-level entry point for GCM pass
bool runOnModule(Module& module) {
    bool hasChanged = false;

    for (auto& func : module.functions) {
        if (!func->isDeclaration()) {
            GCMPass gcmPass;
            bool funcChanged = gcmPass.runOnFunction(*func);
            hasChanged = hasChanged || funcChanged;
        }
    }

    return hasChanged;
}

bool GCMPass::runOnFunction(Function& F) {
    std::cout << "[GCM] Running Global Code Motion on function: " << F.name << std::endl;

    // Record initial state to check for changes
    size_t initialInstructionCount = 0;
    std::map<BasicBlock*, size_t> initialBlockSizes;

    for (auto& bb : F.basicBlocks) {
        size_t blockSize = bb->phi_instructions.size() + bb->instructions.size();
        initialBlockSizes[bb.get()] = blockSize;
        initialInstructionCount += blockSize;
    }

    // Phase 1: Preprocessing - compute dominator depths and loop nesting
    computeDominatorDepths(F);
    computeLoopNestingDepths(F);

    // Phase 2: Schedule Early - move instructions as early as possible
    scheduleEarly(F);

    // Phase 3: Schedule Late - find latest legal positions
    scheduleLate(F);

    // Phase 4: Select final positions using heuristics
    selectFinalPositions(F);

    // Phase 5: Reschedule code
    rescheduleCode(F);

    // Check if any changes were made
    bool hasChanged = false;

    // Check if any instructions were moved
    for (auto& [targetBlock, instructions] : scheduled_instructions) {
        if (!instructions.empty()) {
            hasChanged = true;
            break;
        }
    }

    // Additional check: compare block sizes
    if (!hasChanged) {
        for (auto& bb : F.basicBlocks) {
            size_t currentBlockSize = bb->phi_instructions.size() + bb->instructions.size();
            if (currentBlockSize != initialBlockSizes[bb.get()]) {
                hasChanged = true;
                break;
            }
        }
    }

    // 如果有变化，进行局部的Use-Def链维护
    if (hasChanged) {
        std::cout << "[GCM] Maintaining Use-Def chains after code motion..." << std::endl;

        // 验证和修复Use-Def链
        for (auto& bb : F.basicBlocks) {
            // 处理PHI指令 - PHI指令从不移动，但需要维护其Use-Def关系
            for (auto& phi : bb->phi_instructions) {
                for (auto& incoming : phi->incoming_values) {
                    if (auto* defInst = dynamic_cast<Instruction*>(incoming.first)) {
                        // 确保定义指令的使用列表包含这个PHI
                        auto& uses = defInst->getUses();
                        if (std::find(uses.begin(), uses.end(), phi.get()) == uses.end()) {
                            uses.push_back(phi.get());
                        }
                    }
                }
            }

            // 处理普通指令
            for (auto& inst : bb->instructions) {
                for (Value* operand : inst->operands) {
                    if (auto* defInst = dynamic_cast<Instruction*>(operand)) {
                        // 确保定义指令的使用列表包含当前指令
                        auto& uses = defInst->getUses();
                        if (std::find(uses.begin(), uses.end(), inst.get()) == uses.end()) {
                            uses.push_back(inst.get());
                        }
                    }
                }
            }
        }

        // 清理无效的使用关系
        for (auto& bb : F.basicBlocks) {
            // PHI指令的使用关系清理
            for (auto& phi : bb->phi_instructions) {
                auto& uses = phi->getUses();
                uses.erase(std::remove_if(uses.begin(), uses.end(),
                                          [&](Instruction* user) {
                                              // 检查使用者是否还在操作数中引用此指令
                                              for (Value* operand : user->operands) {
                                                  if (operand == phi.get()) {
                                                      return false;  // 仍然被使用，不删除
                                                  }
                                              }
                                              return true;  // 不再被使用，删除
                                          }),
                           uses.end());
            }

            // 普通指令的使用关系清理
            for (auto& inst : bb->instructions) {
                auto& uses = inst->getUses();
                uses.erase(std::remove_if(uses.begin(), uses.end(),
                                          [&](Instruction* user) {
                                              // 检查使用者是否还在操作数中引用此指令
                                              for (Value* operand : user->operands) {
                                                  if (operand == inst.get()) {
                                                      return false;  // 仍然被使用，不删除
                                                  }
                                              }
                                              return true;  // 不再被使用，删除
                                          }),
                           uses.end());
            }
        }
    }

    std::cout << "[GCM] Global Code Motion completed" << (hasChanged ? " with changes" : " without changes") << std::endl;

    return hasChanged;
}

// === Phase 1: Preprocessing ===

void GCMPass::computeDominatorDepths(Function& F) {
    // Build dominator tree if not already built
    if (!DT) {
        DT = std::make_unique<cfg::DominatorTree>();
        DT->runOnFunction(F);
    }

    // Compute depth for each block using BFS from entry
    std::queue<std::pair<BasicBlock*, int>> queue;
    std::set<BasicBlock*> visited;

    BasicBlock* entry = F.getEntryBlock();
    if (!entry)
        return;

    queue.push({entry, 0});
    visited.insert(entry);
    dom_depth[entry] = 0;

    while (!queue.empty()) {
        auto [current, depth] = queue.front();
        queue.pop();

        // Visit all children in dominator tree
        const auto& children = DT->getChildren(current);
        for (BasicBlock* child : children) {
            if (visited.find(child) == visited.end()) {
                visited.insert(child);
                dom_depth[child] = depth + 1;
                queue.push({child, depth + 1});
            }
        }
    }

    std::cout << "[GCM] Computed dominator depths for " << dom_depth.size() << " blocks" << std::endl;
}

void GCMPass::computeLoopNestingDepths(Function& F) {
    // Build loop analysis if not already built
    if (!LA) {
        LA = std::make_unique<LoopAnalysis>();
        LA->runOnFunction(F, *DT);
    }

    // Initialize all blocks to nesting depth 0
    for (auto& bb : F.basicBlocks) {
        loop_nest[bb.get()] = 0;
    }

    // For each loop, increment nesting depth of all contained blocks
    for (auto& loop : LA->loops) {
        int loopDepth = 1;

        // Calculate the nesting depth of this loop
        Loop* parent = loop->getParent();
        while (parent) {
            loopDepth++;
            parent = parent->getParent();
        }

        // Set nesting depth for all blocks in this loop
        for (BasicBlock* bb : loop->getBlocks()) {
            loop_nest[bb] = std::max(loop_nest[bb], loopDepth);
        }
    }

    std::cout << "[GCM] Computed loop nesting depths" << std::endl;
}

// === Phase 2: Schedule Early ===

void GCMPass::scheduleEarly(Function& F) {
    std::cout << "[GCM] Phase 2: Schedule Early" << std::endl;

    // 获取支配树根节点（入口块）
    BasicBlock* rootBlock = F.getEntryBlock();
    if (!rootBlock) {
        std::cerr << "[GCM Error] Function has no entry block" << std::endl;
        return;
    }

    // 初始化：标记所有指令未访问
    early_visited.clear();

    // 从固定指令（pinned）开始调度
    for (auto& bb : F.basicBlocks) {
        // PHI指令是固定的
        for (auto& phi : bb->phi_instructions) {
            if (!early_visited[phi.get()]) {
                early_visited[phi.get()] = true;
                early_block[phi.get()] = phi->parent;  // 保留在原始块

                // 递归调度其输入
                for (const auto& incoming : phi->incoming_values) {
                    if (auto* inputInst = dynamic_cast<Instruction*>(incoming.first)) {
                        scheduleEarlyForInstruction(inputInst, rootBlock);
                    }
                }
            }
        }

        // 其他固定指令
        for (auto& inst : bb->instructions) {
            if (isPinned(inst.get()) && !early_visited[inst.get()]) {
                early_visited[inst.get()] = true;
                early_block[inst.get()] = inst->parent;  // 保留在原始块

                // 递归调度其输入
                for (Value* operand : inst->operands) {
                    if (auto* inputInst = dynamic_cast<Instruction*>(operand)) {
                        scheduleEarlyForInstruction(inputInst, rootBlock);
                    }
                }
            }
        }
    }

    // 处理所有剩余的非固定指令 (这一步通常由递归调度自动完成，但为了确保完整性)
    for (auto& bb : F.basicBlocks) {
        for (auto& inst : bb->instructions) {
            if (!early_visited[inst.get()]) {
                scheduleEarlyForInstruction(inst.get(), rootBlock);
            }
        }
    }
}

void GCMPass::scheduleEarlyForInstruction(Instruction* inst, BasicBlock* rootBlock) {
    // 如果已访问，直接返回
    if (early_visited[inst]) {
        return;
    }
    early_visited[inst] = true;

    // 如果是固定指令，保留在原始块
    if (isPinned(inst)) {
        early_block[inst] = inst->parent;
        return;
    }

    // 初始化为支配树根节点（深度最浅）
    early_block[inst] = rootBlock;

    // 递归调度所有输入指令
    for (Value* operand : inst->operands) {
        if (auto* inputInst = dynamic_cast<Instruction*>(operand)) {
            scheduleEarlyForInstruction(inputInst, rootBlock);

            // 选择最深支配深度的输入所在块
            BasicBlock* inputBlock = early_block[inputInst];
            if (inputBlock && dom_depth[inputBlock] > dom_depth[early_block[inst]]) {
                early_block[inst] = inputBlock;  // 更新为最深输入块
            }
        }
    }
}

// === Phase 3: Schedule Late ===

void GCMPass::scheduleLate(Function& F) {
    std::cout << "[GCM] Phase 3: Schedule Late" << std::endl;

    // 初始化：标记所有指令未访问
    late_visited.clear();

    // 从固定指令开始调度
    for (auto& bb : F.basicBlocks) {
        // PHI指令是固定的
        for (auto& phi : bb->phi_instructions) {
            if (!late_visited[phi.get()]) {
                late_visited[phi.get()] = true;
                late_block[phi.get()] = phi->parent;  // 保留在原始块

                // 递归调度其使用者 - 先复制使用者列表以避免迭代器失效
                auto users = phi->getUses(); // 复制一份
                for (Instruction* user : users) {
                    scheduleLateForInstruction(user);
                }
            }
        }

        // 其他固定指令
        for (auto& inst : bb->instructions) {
            if (isPinned(inst.get()) && !late_visited[inst.get()]) {
                late_visited[inst.get()] = true;
                late_block[inst.get()] = inst->parent;  // 保留在原始块

                // 递归调度其使用者 - 先复制使用者列表以避免迭代器失效
                auto users = inst->getUses(); // 复制一份
                for (Instruction* user : users) {
                    scheduleLateForInstruction(user);
                }
            }
        }
    }

    // 处理所有剩余的非固定指令 (这一步通常由递归调度自动完成，但为了确保完整性)
    for (auto& bb : F.basicBlocks) {
        for (auto& inst : bb->instructions) {
            if (!late_visited[inst.get()]) {
                scheduleLateForInstruction(inst.get());
            }
        }
    }
}

void GCMPass::scheduleLateForInstruction(Instruction* inst) {
    // 如果已访问，直接返回
    if (late_visited[inst]) {
        return;
    }
    late_visited[inst] = true;

    // 如果是固定指令，保留在原始块
    if (isPinned(inst)) {
        late_block[inst] = inst->parent;
        return;
    }

    BasicBlock* lca = nullptr;  // 初始化最近公共祖先（LCA）

    // 递归调度所有使用指令 - 先复制使用者列表以避免迭代器失效
    auto users = inst->getUses(); // 复制一份
    for (Instruction* user : users) {
        scheduleLateForInstruction(user);

        // 确定使用点所在块（处理Phi特殊逻辑）
        BasicBlock* useBlock = user->parent;
        if (auto* phi = dynamic_cast<PhiInst*>(user)) {
            // 为PHI指令找到对应的前驱块
            for (const auto& incoming : phi->incoming_values) {
                if (incoming.first == inst) {
                    useBlock = incoming.second;  // 使用对应前驱块
                    break;
                }
            }
        }

        // 计算当前使用块与LCA的最近公共祖先
        lca = findLCA(lca, useBlock);
    }

    // 如果没有使用者，使用early_block作为后推位置
    if (!lca) {
        lca = early_block[inst];
    }

    late_block[inst] = lca;
}

std::vector<BasicBlock*> GCMPass::getUseBlocks(Instruction* inst) {
    std::vector<BasicBlock*> useBlocks;

    for (Instruction* user : inst->getUses()) {
        BasicBlock* useBlock = user->parent;

        // Special handling for PHI instructions
        if (auto* phi = dynamic_cast<PhiInst*>(user)) {
            // For PHI, the use is considered to be in the predecessor block
            // corresponding to this operand
            for (const auto& incoming : phi->incoming_values) {
                if (incoming.first == inst) {
                    useBlocks.push_back(incoming.second);
                }
            }
        } else {
            useBlocks.push_back(useBlock);
        }
    }

    return useBlocks;
}

BasicBlock* GCMPass::computeLCA(const std::vector<BasicBlock*>& blocks) {
    if (blocks.empty())
        return nullptr;
    if (blocks.size() == 1)
        return blocks[0];

    BasicBlock* lca = blocks[0];
    for (size_t i = 1; i < blocks.size(); ++i) {
        lca = findLCA(lca, blocks[i]);
    }

    return lca;
}

BasicBlock* GCMPass::findLCA(BasicBlock* a, BasicBlock* b) {
    if (!a)
        return b;
    if (!b)
        return a;

    // Align depths first
    alignDepths(a, b);

    // Move both up until they meet
    while (a != b) {
        a = DT->getIdom(a);
        b = DT->getIdom(b);
        if (!a || !b)
            break;
    }

    return a;
}

void GCMPass::alignDepths(BasicBlock*& a, BasicBlock*& b) {
    int depthA = dom_depth[a];
    int depthB = dom_depth[b];

    // Move the deeper block up to match the shallower one
    while (depthA > depthB) {
        a = DT->getIdom(a);
        depthA--;
    }

    while (depthB > depthA) {
        b = DT->getIdom(b);
        depthB--;
    }
}

// === Phase 4: Select Final Position ===

void GCMPass::selectFinalPositions(Function& F) {
    std::cout << "[GCM] Phase 4: Select Final Positions" << std::endl;

    for (auto& bb : F.basicBlocks) {
        // PHI instructions are always pinned to their current block
        for (auto& phi : bb->phi_instructions) {
            final_block[phi.get()] = phi->parent;
        }

        // Only process non-PHI instructions for position selection
        for (auto& inst : bb->instructions) {
            if (!isPinned(inst.get())) {
                BasicBlock* early = early_block[inst.get()];
                BasicBlock* late = late_block[inst.get()];
                BasicBlock* best = selectBestBlock(inst.get(), early, late);
                final_block[inst.get()] = best;
            } else {
                final_block[inst.get()] = inst->parent;
            }
        }
    }
}

BasicBlock* GCMPass::selectBestBlock(Instruction* inst, BasicBlock* early, BasicBlock* late) {
    if (!early || !late)
        return inst->parent;

    // 如果 early 和 late 是同一个块，直接返回
    if (early == late) {
        return early;
    }

    BasicBlock* best = late;  // 从最晚位置开始
    BasicBlock* current = late;

    // 从 late 向 early 遍历支配树路径
    while (current && current != early) {
        if (loop_nest[current] < loop_nest[best]) {  // 优先选择loop_nest最浅的块（移出循环）
            best = current;
        } else if (loop_nest[current] == loop_nest[best] && dom_depth[current] > dom_depth[best]) {  // 同循环深度时选最深的块（减少高频路径执行）
            best = current;
        }

        current = DT->getIdom(current);
    }

    // 也考虑 early 块
    if (early) {
        if (loop_nest[early] < loop_nest[best]) {
            best = early;
        } else if (loop_nest[early] == loop_nest[best] && dom_depth[early] > dom_depth[best]) {
            best = early;
        }
    }

    return best;
}

// === Phase 5: Code Reschedule ===

void GCMPass::rescheduleCode(Function& F) {
    std::cout << "[GCM] Phase 5: Reschedule Code" << std::endl;

    // Collect all instructions that need to be moved
    // PHI instructions are never moved since they are pinned
    std::vector<std::pair<BasicBlock*, Instruction*>> instructionsToMove;
    
    for (auto& bb : F.basicBlocks) {
        // Skip PHI instructions - they are pinned and never move

        // Only consider non-PHI instructions for movement
        for (auto& inst : bb->instructions) {
            BasicBlock* targetBlock = final_block[inst.get()];
            if (targetBlock && targetBlock != inst->parent) {
                instructionsToMove.push_back({targetBlock, inst.get()});
            }
        }
    }

    // Move instructions to their target blocks
    std::cout << "[GCM] Moving " << instructionsToMove.size() << " instructions to target blocks..." << std::endl;
    for (auto& [targetBlock, inst] : instructionsToMove) {
        // 在移动前检查指令是否仍然有效（没有被之前的移动操作删除）
        if (inst->parent) {  // 如果parent非空，说明指令仍然在某个块中
            moveInstructionToBlock(inst, targetBlock);
        }
    }
}

void GCMPass::moveInstructionToBlock(Instruction* inst, BasicBlock* targetBlock) {
    BasicBlock* oldBlock = inst->parent;
    if (!oldBlock || !targetBlock || oldBlock == targetBlock) {
        return;
    }

    // PHI instructions should never be moved - they are pinned
    if (auto* phi = dynamic_cast<PhiInst*>(inst)) {
        std::cerr << "[GCM Error] Attempted to move PHI instruction - this should never happen!" << std::endl;
        return;
    }

    std::cout << "[GCM] [@@Moving !!!!] Moving instruction " << inst->toString() << " from " << oldBlock->label << " to " << targetBlock->label << std::endl;

    // 在移动前先检查指令是否仍然存在于父块中
    bool found = false;
    for (const auto& instPtr : oldBlock->instructions) {
        if (instPtr.get() == inst) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        std::cerr << "[GCM Warning] Instruction not found in parent block, skipping move" << std::endl;
        return;
    }

    // 使用新的API进行指令移动
    // 1. cloneInstruction - 克隆指令
    std::map<Value*, Value*> valueMap;  // 空的值映射，因为我们移动的是同一个函数内的指令
    auto clonedInst = cloneInstruction(inst, valueMap, "");

    if (!clonedInst) {
        std::cerr << "[GCM Error] Failed to clone instruction during move" << std::endl;
        return;
    }

    // 找到合适的插入位置，确保所有操作数的定义都在此指令之前
    size_t insertPos = targetBlock->instructions.size(); // 默认插入到末尾
    for (size_t i = 0; i < targetBlock->instructions.size(); ++i) {
        bool canInsertHere = true;

        // 检查当前指令的所有操作数
        for (Value* operand : clonedInst->operands) {
            if (auto* operandInst = dynamic_cast<Instruction*>(operand)) {
                // 如果操作数在同一个块中，确保它在当前位置之前
                if (operandInst->parent == targetBlock) {
                    // 检查operandInst是否在当前位置i之后
                    auto it = targetBlock->instructions.begin();
                    std::advance(it, i);
                    for (auto checkIt = it; checkIt != targetBlock->instructions.end(); ++checkIt) {
                        if (checkIt->get() == operandInst) {
                            canInsertHere = false;
                            break;
                        }
                    }
                }
            }
            if (!canInsertHere)
                break;
        }

        if (canInsertHere) {
            insertPos = i;
            break;
        }
    }

    // 2. replaceAllUsesWith - 用新指令替换所有对旧指令的使用
    inst->replaceAllUsesWith(clonedInst.get());

    // 3. removeFromParent - 从原始块中移除旧指令
    // 注意：这会销毁inst指向的对象，之后不能再访问inst
    inst->removeFromParent();

    // 4. 使用insertInstructionAt在指定位置插入指令
    targetBlock->insertInstructionAt(std::move(clonedInst), insertPos);
}

// === Helper Functions ===

bool GCMPass::isPinned(Instruction* inst) {
    if (!inst)
        return true;

    // PHI instructions are pinned to their blocks
    if (dynamic_cast<PhiInst*>(inst)) {
        return true;
    }

    // Control flow instructions are pinned
    switch (inst->opcode) {
    case Opcode::Store:
    case Opcode::Call:
    case Opcode::Load:
    case Opcode::Br:
    case Opcode::Ret:
    case Opcode::Switch: return true;
    default: return false;
    }
}

bool GCMPass::isDominatedBy(BasicBlock* bb, BasicBlock* dom) { return DT && DT->dominates(dom, bb); }

// === Utility Functions ===

bool isControlDependent(Instruction* inst) {
    if (!inst)
        return false;

    switch (inst->opcode) {
    case Opcode::Br:
    case Opcode::Ret:
    case Opcode::Switch:
    case Opcode::Call:
    case Opcode::Store:
    case Opcode::Load:  // Can trapreturn true;
    default: return false;
    }
}

bool canMoveInstruction(Instruction* inst) { return !isControlDependent(inst) && !dynamic_cast<PhiInst*>(inst); }

std::vector<Instruction*> getInstructionInputs(Instruction* inst) {
    std::vector<Instruction*> inputs;

    if (!inst)
        return inputs;

    for (Value* operand : inst->operands) {
        if (auto* inputInst = dynamic_cast<Instruction*>(operand)) {
            inputs.push_back(inputInst);
        }
    }

    return inputs;
}

}  // namespace gcm
}  // namespace llvm_ir
