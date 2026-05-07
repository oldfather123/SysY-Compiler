#include "../../include/ir/llvm_ir.h"
#include "../../include/ir/cfg.h"
#include "../../include/ir/cfg_simplify.h"
#include <set>
#include <queue>
#include <iostream>
#include <algorithm>
#include <functional>

namespace llvm_ir {
namespace cfg {

// ====== 空跳转块消除优化实现 ======
void RemoveTrampolineBlocks(Function& F) {
    std::vector<BasicBlock*> toRemove;
    std::map<BasicBlock*, BasicBlock*> redirectMap; // 被移除块 -> 跳转目标
    BasicBlock* currentEntryBlock = F.getEntryBlock(); // 追踪当前有效的入口块

    // std::cout << "[CFG-RemoveTrampoline] 开始空跳转块消除优化" << std::endl;

    // 0. 先建立CFG关系
    cfg::buildCFG(F);

    // 1. 找到所有空跳转块
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        // 只包含一条无条件br，且无phi，且无其他指令
        if (!bb->phi_instructions.empty()) continue;
        if (bb->instructions.size() != 1) continue;
        Instruction* inst = bb->instructions.front().get();
        auto br = dynamic_cast<BranchInst*>(inst);
        if (!br) continue;
        if (!br->operands.empty()) continue; // 有条件跳转不处理
        // 记录可移除
        BasicBlock* target = nullptr;
        // 查找目标块
        for (auto& b2_ptr : F.basicBlocks) {
            if (b2_ptr->label == br->trueLabel) {
                target = b2_ptr.get();
                break;
            }
        }
        if (!target) continue;
        // 不能自跳
        if (bb == target) continue;
        // 入口块特殊处理：如果入口块是空跳转，可以移除但需要特殊处理
        bool isEntryBlock = (bb == currentEntryBlock);
        // std::cout << "[CFG-RemoveTrampoline] current entry block: %" << currentEntryBlock->label << std::endl;
        
        // 检查删除这个块是否会影响PHI指令的值确定
        bool canRemove = true;
        
        // entry块特殊判定：只有目标块没有其他前驱时才允许消除
        if (isEntryBlock) {
            if (!(target->predecessors.size() == 1 && target->predecessors[0] == bb)) {
                canRemove = false;
                continue;
            }
        }
        
        // 检查目标块是否有PHI指令，且这些PHI指令的incoming_values中有来自当前块的
        for (const auto& phi : target->phi_instructions) {
            // 收集所有来自当前块的incoming values
            std::vector<Value*> incomingFromCurrent;
            for (const auto& incoming : phi->incoming_values) {
                if (incoming.second == bb) {
                    incomingFromCurrent.push_back(incoming.first);
                }
            }
            
            if (!incomingFromCurrent.empty()) {
                // 检查删除当前块后，是否还有其他不同的incoming values
                std::set<Value*> otherIncomingValues;
                for (const auto& incoming : phi->incoming_values) {
                    if (incoming.second != bb) {
                        otherIncomingValues.insert(incoming.first);
                    }
                }
                
                // 如果删除当前块后，所有incoming values都相同，则可以删除
                // 否则不能删除，因为会导致PHI指令无法确定正确的值
                if (otherIncomingValues.size() > 1 || 
                    (otherIncomingValues.size() == 1 && incomingFromCurrent.size() > 0 && 
                     otherIncomingValues.find(incomingFromCurrent[0]) == otherIncomingValues.end())) {
                    canRemove = false;
                    // std::cout << "[CFG-RemoveTrampoline]   跳过块 %" << bb->label 
                    //           << "，因为删除后PHI指令 %" << phi->getName() 
                    //           << " 将无法确定正确的值（有多个不同的incoming values）" << std::endl;
                    break;
                }
            }
        }
        
        if (canRemove) {
            toRemove.push_back(bb);
            redirectMap[bb] = target;
            // 如果是入口块，标记需要特殊处理
            if (isEntryBlock) {
                // 将目标块标记为新的入口块
                // std::cout << "[CFG-RemoveTrampoline] Entry block will redirect to final target: %" << target->label << std::endl;
                target->label = "entry";
                currentEntryBlock = target; // 更新当前入口块引用
            }
            // std::cout << "[CFG-RemoveTrampoline] 标记可移除空跳转块: %" << bb->label << " -> %" << target->label << std::endl;
        }
    }

    // 新增：解决链式跳转问题，确保所有重定向都指向最终目标
    bool changedInLoop = true;
    while(changedInLoop) {
        changedInLoop = false;
        for (auto& pair : redirectMap) {
            auto it = redirectMap.find(pair.second);
            if (it != redirectMap.end()) {
                pair.second = it->second;
                changedInLoop = true;
            }
        }
    }
 
    // 预构建：被删除块集合与“递归展开的具体前驱”映射
    std::set<BasicBlock*> removedSet(toRemove.begin(), toRemove.end());
    std::map<BasicBlock*, std::vector<BasicBlock*>> removedToConcretePreds;

    // 递归收集某个（可能被删的）块的所有未被删除的“叶子前驱”
    std::function<const std::vector<BasicBlock*>&(BasicBlock*)> collectConcretePreds =
        [&](BasicBlock* blk) -> const std::vector<BasicBlock*>& {
            auto it = removedToConcretePreds.find(blk);
            if (it != removedToConcretePreds.end()) return it->second;

            std::vector<BasicBlock*> result;
            // 如果该块本身未被删除，则它自身就是一个具体前驱
            if (removedSet.find(blk) == removedSet.end()) {
                result.push_back(blk);
                auto inserted = removedToConcretePreds.emplace(blk, std::move(result));
                return inserted.first->second;
            }

            // 否则，递归展开它的前驱
            for (BasicBlock* p : blk->predecessors) {
                const std::vector<BasicBlock*>& sub = collectConcretePreds(p);
                result.insert(result.end(), sub.begin(), sub.end());
            }
            auto inserted = removedToConcretePreds.emplace(blk, std::move(result));
            return inserted.first->second;
        };

    // 为每个将被删除的块预先计算展开前驱
    for (BasicBlock* rb : toRemove) {
        (void)collectConcretePreds(rb);
    }
    
    if (toRemove.empty()) {
        // std::cout << "[CFG-RemoveTrampoline] 没有发现可移除的空跳转块" << std::endl;
        return;
    }
    // std::cout << "[CFG-RemoveTrampoline] Blocks to remove: ";
    // for (auto* bb : toRemove) std::cout << (bb ? ("%" + bb->label) : "nullptr") << " ";
    // std::cout << std::endl;
    std::cout << "[CFG-RemoveTrampoline] Removed " << toRemove.size() << " trampoline blocks" << std::endl;

    // 2. 建立CFG关系（如果还没有建立）
    cfg::buildCFG(F);

    // 3. 替换所有前驱的跳转目标
    for (BasicBlock* bb : toRemove) {
        BasicBlock* target = redirectMap[bb];
        // std::cout << "[CFG-RemoveTrampoline] Remove block %" << bb->label << ", redirect to %" << target->label << std::endl;
        
        // 遍历所有前驱
        std::vector<BasicBlock*> preds = bb->predecessors;
        for (BasicBlock* pred : preds) {
            // 找到pred的最后一条指令
            if (pred->instructions.empty()) continue;
            Instruction* last = pred->instructions.back().get();
            // 只处理br指令
            if (auto br = dynamic_cast<BranchInst*>(last)) {
                // 无条件跳转
                if (br->operands.empty()) {
                    if (br->trueLabel == bb->label) {
                        // std::cout << "[CFG-RemoveTrampoline] Predecessor %" << pred->label << " br target from %" << bb->label << " to %" << target->label << std::endl;
                        br->trueLabel = target->label;
                    }
                } else {
                    // 条件跳转
                    if (br->trueLabel == bb->label) {
                        // std::cout << "[CFG-RemoveTrampoline] Predecessor %" << pred->label << " br true branch from %" << bb->label << " to %" << target->label << std::endl;
                        br->trueLabel = target->label;
                    }
                    if (br->falseLabel == bb->label) {
                        // std::cout << "[CFG-RemoveTrampoline] Predecessor %" << pred->label << " br false branch from %" << bb->label << " to %" << target->label << std::endl;
                        br->falseLabel = target->label;
                    }
                }
            }
        }
        
        // 更新target的前驱
        for (auto it = target->predecessors.begin(); it != target->predecessors.end(); ) {
            if (*it == bb) it = target->predecessors.erase(it);
            else ++it;
        }
        for (BasicBlock* pred : preds) {
            if (std::find(target->predecessors.begin(), target->predecessors.end(), pred) == target->predecessors.end()) {
                target->predecessors.push_back(pred);
            }
        }
    }

    // 3.5. 额外检查：确保所有跳转到被删除块的指令都被更新
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        for (auto& inst_ptr : bb->instructions) {
            if (auto br = dynamic_cast<BranchInst*>(inst_ptr.get())) {
                // 检查无条件跳转
                if (br->operands.empty()) {
                    // 检查目标是否在toRemove中
                    for (BasicBlock* removed : toRemove) {
                        if (br->trueLabel == removed->label) {
                            auto it = redirectMap.find(removed);
                            if (it != redirectMap.end()) {
                                std::cout << "[CFG-RemoveTrampoline] Additional fix: block %" << bb->label 
                                          << " br target from %" << removed->label << " to %" << it->second->label << std::endl;
                                br->trueLabel = it->second->label;
                            }
                        }
                    }
                } else {
                    // 检查条件跳转
                    for (BasicBlock* removed : toRemove) {
                        auto it = redirectMap.find(removed);
                        if (it != redirectMap.end()) {
                            if (br->trueLabel == removed->label) {
                                std::cout << "[CFG-RemoveTrampoline] Additional fix: block %" << bb->label 
                                          << " br true branch from %" << removed->label << " to %" << it->second->label << std::endl;
                                br->trueLabel = it->second->label;
                            }
                            if (br->falseLabel == removed->label) {
                                std::cout << "[CFG-RemoveTrampoline] Additional fix: block %" << bb->label 
                                          << " br false branch from %" << removed->label << " to %" << it->second->label << std::endl;
                                br->falseLabel = it->second->label;
                            }
                        }
                    }
                }
            }
        }
    }

    // 4. 从函数中移除这些块
    F.basicBlocks.erase(
        std::remove_if(F.basicBlocks.begin(), F.basicBlocks.end(),
            [&](const std::unique_ptr<BasicBlock>& bb_ptr) {
                return std::find(toRemove.begin(), toRemove.end(), bb_ptr.get()) != toRemove.end();
            }),
        F.basicBlocks.end());
    
    // 特殊处理：如果入口块被移除或重定向，确保新的入口块在列表首位
    bool entryRemoved = false;
    bool entryRedirected = false;
    for (BasicBlock* removed : toRemove) {
        if (removed == currentEntryBlock) {
            entryRemoved = true;
            break;
        }
    }
    
    // 检查是否有块被重定向为新的入口块
    for (const auto& pair : redirectMap) {
        if (pair.second->label == "entry") {
            entryRedirected = true;
            break;
        }
    }
    
    if (entryRemoved || entryRedirected) {
        // 找到新的入口块（label为"entry"的块）
        auto entryIt = std::find_if(F.basicBlocks.begin(), F.basicBlocks.end(),
            [](const std::unique_ptr<BasicBlock>& bb) {
                return bb->label == "entry";
            });
        
        if (entryIt != F.basicBlocks.end()) {
            // 将新的入口块移到列表首位
            auto entryBlock = std::move(*entryIt);
            F.basicBlocks.erase(entryIt);
            F.basicBlocks.insert(F.basicBlocks.begin(), std::move(entryBlock));
            std::cout << "[CFG-RemoveTrampoline] Moved entry block to front of function" << std::endl;
        }
    }

    // 5. 清理CFG关系和PHI指令
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        
        // 清理后继中已被移除的块
        bb->successors.erase(
            std::remove_if(bb->successors.begin(), bb->successors.end(),
                [&](BasicBlock* succ) {
                    return std::find(toRemove.begin(), toRemove.end(), succ) != toRemove.end();
                }),
            bb->successors.end());
        
        // 清理前驱
        bb->predecessors.erase(
            std::remove_if(bb->predecessors.begin(), bb->predecessors.end(),
                [&](BasicBlock* pred) {
                    return std::find(toRemove.begin(), toRemove.end(), pred) != toRemove.end();
                }),
            bb->predecessors.end());
        
        // 清理phi指令的incoming_values
        for (auto& phi : bb->phi_instructions) {
            size_t before = phi->incoming_values.size();

            // 当前块真实前驱集合（去重）
            std::set<BasicBlock*> currentPreds(bb->predecessors.begin(), bb->predecessors.end());

            // 先展开：把来自被删块的incoming根据“具体前驱”映射展开
            std::vector<std::pair<Value*, BasicBlock*>> expanded;
            expanded.reserve(phi->incoming_values.size());
            for (const auto& incoming : phi->incoming_values) {
                Value* inVal = incoming.first;
                BasicBlock* inFrom = incoming.second;

                if (removedSet.find(inFrom) != removedSet.end()) {
                    // 展开为多个来自具体前驱的incoming
                    auto itMap = removedToConcretePreds.find(inFrom);
                    if (itMap != removedToConcretePreds.end()) {
                        for (BasicBlock* concPred : itMap->second) {
                            // 仅保留仍为当前块真实前驱的条目
                            if (currentPreds.count(concPred)) {
                                expanded.emplace_back(inVal, concPred);
                            }
                        }
                    }
                } else {
                    // 非被删块直接保留（若它不再是当前前驱，后续唯一化会自然过滤）
                    if (currentPreds.count(inFrom)) {
                        expanded.emplace_back(inVal, inFrom);
                    }
                }
            }

            // 唯一化：保证一个前驱对应一条incoming，遇到冲突沿用首次值（与原行为一致）
            std::map<BasicBlock*, Value*> uniqueIncoming;
            for (const auto& inc : expanded) {
                if (uniqueIncoming.find(inc.second) == uniqueIncoming.end()) {
                    uniqueIncoming.emplace(inc.second, inc.first);
                } else {
                    // 如果同一前驱出现多次且值不同，保留首次，忽略后续
                    // 可选：这里可增加一致性检查日志
                }
            }

            // 按照当前前驱集合重建incoming，保持与CFG一致
            phi->incoming_values.clear();
            for (BasicBlock* pred : currentPreds) {
                auto it = uniqueIncoming.find(pred);
                if (it != uniqueIncoming.end()) {
                    phi->incoming_values.push_back({it->second, pred});
                }
            }

            if (before != phi->incoming_values.size()) {
                std::cout << "[CFG-RemoveTrampoline] Clean phi " << phi->getName()
                          << " incoming, rebuilt for merged predecessors" << std::endl;
            }
        }
    }

    // 合并true false label一致的分支
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        auto* br = bb->getTerminator();
        if (auto* brInst = dynamic_cast<BranchInst*>(br)) {
            if (brInst->isConditional()) {
                if (brInst->trueLabel == brInst->falseLabel) {
                    auto newBrInst = std::make_unique<BranchInst>(brInst->trueLabel);
                    brInst->removeFromParent();
                    std::cout << "[CFG-RemoveTrampoline] Merged conditional branch to " << newBrInst->toString() << std::endl;
                    bb->addInstruction(std::move(newBrInst));
                }
            }
        }
    }
    // 6. 重建CFG关系
    cfg::buildCFG(F);
    
    // 7. 验证PHI指令的正确性
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        
        // 收集当前基本块的所有前驱
        std::set<BasicBlock*> currentPreds(bb->predecessors.begin(), bb->predecessors.end());
        
        for (auto& phi : bb->phi_instructions) {
            // 收集PHI指令当前的所有incoming blocks
            std::set<BasicBlock*> phiIncomingBlocks;
            for (const auto& incoming : phi->incoming_values) {
                phiIncomingBlocks.insert(incoming.second);
            }
            
            // 验证PHI指令的incoming_values数量与predecessors数量匹配
            if (phiIncomingBlocks.size() != currentPreds.size()) {
                std::cout << "[CFG-RemoveTrampoline][Warning] phi " << phi->getName() 
                          << " incoming_values number(" << phiIncomingBlocks.size() 
                          << ") not match predecessors number(" << currentPreds.size() << ")" << std::endl;
            }
        }
    }
    
    // std::cout << "[CFG-RemoveTrampoline] 共移除了 " << toRemove.size() << " 个空跳转块" << std::endl;
}

// ================= MakeFunctionOneExit Implementation =================
void MakeFunctionOneExit(Function& F) {
    if (F.isDeclaration()) return;

    std::vector<BasicBlock*> returnBlocks;
    Type returnType = F.returnType;
    
    // 第一步：收集所有包含返回指令的基本块
    for (auto& bb : F.basicBlocks) {
        Instruction* terminator = bb->getTerminator();
        if (terminator && terminator->opcode == Opcode::Ret) {
            returnBlocks.push_back(bb.get());
        }
    }
    
    // 如果没有返回块，创建一个默认的返回块
    if (returnBlocks.empty()) {
        std::cout << "[MakeFunctionOneExit] no return block" << std::endl;
        // 清空函数的所有基本块
        F.basicBlocks.clear();
        
        // 创建一个新的基本块作为唯一的返回块
        auto exitBlock = std::make_unique<BasicBlock>("exit", &F);
        
        // 根据返回类型创建相应的返回指令
        if (F.returnType == Type::Void) {
            auto retInst = std::make_unique<ReturnInst>();
            exitBlock->addInstruction(std::move(retInst));
        } else if (F.returnType == Type::I32) {
            auto constZero = std::make_unique<ConstantInt>(0);
            auto retInst = std::make_unique<ReturnInst>(constZero.get());
            exitBlock->addInstruction(std::move(retInst));
        } else if (F.returnType == Type::Float) {
            auto constZero = std::make_unique<ConstantFloat>(0.0f);
            auto retInst = std::make_unique<ReturnInst>(constZero.get());
            exitBlock->addInstruction(std::move(retInst));
        } else {
            // 对于其他类型，使用默认值
            auto constZero = std::make_unique<ConstantInt>(0);
            auto retInst = std::make_unique<ReturnInst>(constZero.get());
            exitBlock->addInstruction(std::move(retInst));
        }
        
        F.addBasicBlock(std::move(exitBlock));
        cfg::buildCFG(F);
        return;
    }
    
    // 如果只有一个返回块，不需要处理
    if (returnBlocks.size() == 1) {
        return;
    }
    
    // 创建新的统一返回块
    std::string exitLabel = "unified_exit";
    auto exitBlock = std::make_unique<BasicBlock>(exitLabel, &F);
    
    // 如果返回类型不是void，需要处理返回值
    if (F.returnType != Type::Void) {
        // 在第一个基本块中创建alloca指令来存储返回值
        if (!F.basicBlocks.empty()) {
            BasicBlock* firstBlock = F.getEntryBlock();
            std::string retVarName = "%ret_val";
            auto allocaInst = std::make_unique<AllocaInst>(F.returnType, retVarName);
            firstBlock->insertInstructionAt(std::move(allocaInst), 0);
            
            // 处理每个返回块
            for (auto retBlock : returnBlocks) {
                // 找到返回指令
                for (auto it = retBlock->instructions.begin(); it != retBlock->instructions.end(); ++it) {
                    if ((*it)->opcode == Opcode::Ret) {
                        auto retInst = dynamic_cast<ReturnInst*>(it->get());
                        if (retInst && !retInst->operands.empty()) {
                            // 创建store指令存储返回值
                            auto storeInst = std::make_unique<StoreInst>(
                                retInst->operands[0], 
                                firstBlock->instructions.front().get()
                            );
                            
                            // 替换返回指令为store指令
                            retInst->removeFromParent();
                            retBlock->instructions.push_back(std::move(storeInst));

                            // std::cout << "[MakeFunctionOneExit] replaced return instruction with store instruction: " << retBlock->instructions.back()->toString() << std::endl;
                            
                            // 添加无条件分支到统一返回块
                            auto brInst = std::make_unique<BranchInst>(exitLabel);
                            retBlock->addInstruction(std::move(brInst));
                        }
                        break;
                    }
                }
            }
            
            // 在统一返回块中加载返回值并返回
            auto loadInst = std::make_unique<LoadInst>(
                firstBlock->instructions.front().get(), 
                "%loaded_ret", 
                F.returnType
            );
            exitBlock->addInstruction(std::move(loadInst));
            
            auto retInst = std::make_unique<ReturnInst>(
                exitBlock->instructions.back().get()
            );
            exitBlock->addInstruction(std::move(retInst));
        }
    } else {
        // 对于void类型，只需要替换返回指令为分支指令
        for (auto retBlock : returnBlocks) {
            for (auto it = retBlock->instructions.begin(); it != retBlock->instructions.end(); ++it) {
                if ((*it)->opcode == Opcode::Ret) {
                    // 删除返回指令
                    retBlock->instructions.erase(it);
                    
                    // 添加无条件分支到统一返回块
                    auto brInst = std::make_unique<BranchInst>(exitLabel);
                    retBlock->addInstruction(std::move(brInst));
                    break;
                }
            }
        }
        
        // 在统一返回块中添加void返回指令
        auto retInst = std::make_unique<ReturnInst>();
        exitBlock->addInstruction(std::move(retInst));
    }
    
    // 将统一返回块添加到函数中
    F.addBasicBlock(std::move(exitBlock));
    
    // 重建CFG
    buildCFG(F);
}

} // namespace cfg
} // namespace llvm_ir