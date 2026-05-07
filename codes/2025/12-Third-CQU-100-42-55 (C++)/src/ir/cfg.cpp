#include "../../include/ir/cfg.h"
#include "../../debug.h"
#include <iostream>
#include <set>
#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace llvm_ir {
namespace cfg {

// Function级别use-def链重建函数
void rebuildUseDefChainsOnFunction(Function& F, bool debug = false) {
    if (debug) std::cout << "[UseDef] 在函数 " << F.name << " 中重新建立use-def关系..." << std::endl;
    
    // 第一步：清理所有Value的uses列表
    for (auto& bb : F.basicBlocks) {
        if (!bb) continue;
        // 普通指令
        for (auto& inst : bb->instructions) {
            if (inst) {
                inst->getUses().clear();
            }
        }
        // PHI
        for (auto& phi : bb->phi_instructions) {
            if (phi) {
                phi->getUses().clear();
            }
        }
    }
    // 函数参数
    for (auto& param : F.parameters) {
        if (param) {
            param->getUses().clear();
        }
    }
    
    // 第二步：重新建立use-def关系
    int addedCount = 0;
    for (auto& bb : F.basicBlocks) {
        if (!bb) continue;
        
        // 重建指令的use-def关系
        for (auto& inst : bb->instructions) {
            if (!inst) continue;
            
            for (auto operand : inst->operands) {
                if (operand) {
                    operand->addUse(inst.get());
                    addedCount++;
                    // 安全检查：如果某个Value的uses列表过大，可能存在问题
                    if (operand->getUses().size() > 10000) {
                        std::cerr << "[UseDef][WARNING] Value " << operand->name << " 有异常多的uses: " << operand->getUses().size() << std::endl;
                    }
                } else {
                    std::cerr << "[UseDef][WARNING] Instruction operand is nullptr" << std::endl;
                    std::cerr << "[UseDef][WARNING] Instruction: " << inst->name << std::endl;
                }
            }
        }
        
        // 重建PHI指令的use-def关系
        for (auto& phi : bb->phi_instructions) {
            if (!phi) continue;
            
            for (auto& incoming : phi->incoming_values) {
                if (incoming.first) {
                        incoming.first->addUse(phi.get());
                        addedCount++;
                    if (incoming.first->getUses().size() > 10000) {
                        std::cerr << "[UseDef][WARNING] PHI: " << phi->name << std::endl;
                        std::cerr << "[UseDef][WARNING] incoming value " << (incoming.first ? "unknown" : incoming.first->name) << " has too many uses: " << incoming.first->getUses().size() << std::endl;
                    }
                } else {
                    std::cerr << "[UseDef][WARNING] PHI incoming value is nullptr" << std::endl;
                    std::cerr << "[UseDef][WARNING] PHI: " << phi->name << std::endl;
                }
            }
        }
    }
    if (debug) {
        std::cout << "[UseDef] 总共添加了 " << addedCount << " 个use关系" << std::endl;
        std::cout << "[UseDef] use-def关系重建完成" << std::endl;
    }
}

// Module级别的use-def链重建函数
void rebuildUseDefChainsModule(Module& M) {
    for (auto& func : M.functions) {
        rebuildUseDefChainsOnFunction(*func, false);
    }
}

// Function级别的CFG重建函数
void buildCFG(Function& F) {
    // 优化：预先构建一个 map 来加速按名称查找，避免在循环中进行 O(N) 的线性搜索。
    std::unordered_map<std::string, BasicBlock*> blockMap;
    for (auto& bb : F.basicBlocks) {
        blockMap[bb->getName()] = bb.get();
    }
    
    auto find_block = [&](const std::string& name) -> BasicBlock* {
        auto it = blockMap.find(name);
        return (it != blockMap.end()) ? it->second : nullptr;
    };

    for (auto& bb : F.basicBlocks) {
        bb->predecessors.clear();
        bb->successors.clear();
    }
    for (auto& bb : F.basicBlocks) {
        Instruction* terminator = bb->getTerminator();
        if (!terminator) continue;
        
        if (auto br = dynamic_cast<BranchInst*>(terminator)) {
            auto trueDest = find_block(br->getTrueLabel());
            auto falseDest = find_block(br->getFalseLabel());
            if (trueDest) {
                bb->successors.push_back(trueDest);
                trueDest->predecessors.push_back(bb.get());
            }
            if (falseDest) {
                bb->successors.push_back(falseDest);
                falseDest->predecessors.push_back(bb.get());
            }
        }
    }
}

// ================= DominatorTree Implementation =================
void DominatorTree::runOnFunction(Function& F, bool reverse) {
    if (F.basicBlocks.empty()) return;
    
    // 清理旧数据
    idomMap.clear();
    dominanceFrontiers.clear();
    childrenMap.clear();

    // 计算前驱/后继信息
    buildCFG(F);

    // 如果需要构建后向支配树，交换CFG的前驱和后继关系
    if (reverse) {
        swapCFG(F);
    }

    computeIdom(F, reverse);
    buildTree(F);                 // 构建 childrenMap
    computeDominanceFrontiers(F); // 计算完整的DF
    
    // ====== DEBUG CFG & 支配树结构 ======
#if DEBUG_DOM
    std::cout << "[CFG/DOM] BasicBlock结构：" << std::endl;
    for (auto& bb : F.basicBlocks) {
        std::cout << "  Block %" << bb->label << ":\n    preds:";
        for (auto p : bb->predecessors) std::cout << " %" << p->label;
        std::cout << "\n    succs:";
        for (auto s : bb->successors) std::cout << " %" << s->label;
        std::cout << std::endl;
    }
    std::cout << "[DOM] idomMap：" << std::endl;
    for (auto& bb : F.basicBlocks) {
        std::cout << "  Block %" << bb->label << " idom: ";
        if (idomMap.count(bb.get()) && idomMap[bb.get()])
            std::cout << "%" << idomMap[bb.get()]->label;
        else
            std::cout << "null";
        std::cout << std::endl;
    }
    std::cout << "[DOM] childrenMap：" << std::endl;
    for (auto& bb : F.basicBlocks) {
        std::cout << "  Block %" << bb->label << " children:";
        for (auto c : childrenMap[bb.get()]) std::cout << " %" << c->label;
        std::cout << std::endl;
    }
#endif

    // 3. 如果是后向支配树，恢复原始的CFG关系
    if (reverse) {
        swapCFG(F);
    }
}

// 辅助函数，执行后序遍历
void postOrderTraversal(BasicBlock* b, std::set<BasicBlock*>& visited, std::vector<BasicBlock*>& postOrder) {
    visited.insert(b);
    // 在正向CFG上遍历后继
    for (BasicBlock* succ : b->successors) {
        if (visited.find(succ) == visited.end()) {
            postOrderTraversal(succ, visited, postOrder);
        }
    }
    postOrder.push_back(b);
}

// 获取RPO序列的函数
std::vector<BasicBlock*> getRPO(Function& F, BasicBlock* entry) {
    std::vector<BasicBlock*> postOrder;
    std::set<BasicBlock*> visited;
    if (entry) {
        postOrderTraversal(entry, visited, postOrder);
    }
    // 处理图中可能不可达的部分（可选，但健全）
    for (auto& bb_ptr : F.basicBlocks) {
        if(visited.find(bb_ptr.get()) == visited.end()) {
            postOrderTraversal(bb_ptr.get(), visited, postOrder);
        }
    }
    std::reverse(postOrder.begin(), postOrder.end());
    return postOrder;
}

void DominatorTree::computeIdom(Function& F, bool reverse) {
    BasicBlock* entry = getEntryBlock(F, reverse);
    if (!entry) return;

    idomMap.clear();
    for (auto& bb : F.basicBlocks) {
        idomMap[bb.get()] = nullptr;
    }

    std::vector<BasicBlock*> rpo_order = getRPO(F, entry);
    bool changed = true;
    int iteration = 0;
    
    while (changed) {
        changed = false;
        iteration++;
        
        for (BasicBlock* b : rpo_order) {
            if (b == entry) continue;
            if (b->predecessors.empty()) continue; // 跳过不可达的基本块

            BasicBlock* new_idom = nullptr;
            
            // 找到第一个已处理的前驱，作为交集的起点
            for (auto p : b->predecessors) {
                if (idomMap[p] != nullptr || p == entry) {
                    new_idom = p;
                    break;
                }
            }

            // 如果没有找到任何已处理的前驱，则暂时无法计算idom
            if (new_idom == nullptr) {
                continue;
            }

            // 与其他已处理的前驱做交集
            for (auto p : b->predecessors) {
                if (p != new_idom && (idomMap[p] != nullptr || p == entry)) {
                    new_idom = findCommonDominator(p, new_idom);
                }
            }
            
            if (idomMap[b] != new_idom) {
                idomMap[b] = new_idom;
                changed = true;
            }
        }
    }
}

BasicBlock* DominatorTree::findCommonDominator(BasicBlock* b1, BasicBlock* b2) {
    // b1 和 b2 在这个函数调用时一定不为 null
    // 使用两个指针在树上移动，而不是用 set
    BasicBlock* finger1 = b1;
    BasicBlock* finger2 = b2;

    // 通过交替标记路径来查找，或者更简单地，让一个finger走完路径，另一个finger去匹配
    // 这里我们用一个更高效的双指针方法
    // 首先需要每个节点的RPO序号来快速比较深度（RPO序号越大，离入口越近）
    // 如果没有RPO序号，可以先将一个路径存入一个临时数据结构
    std::unordered_set<BasicBlock*> path1;
    while (finger1 && idomMap.count(finger1)) {
        path1.insert(finger1);
        if (finger1 == idomMap[finger1]) break; // 到达根节点
        finger1 = idomMap[finger1];
    }

    while (finger2 && idomMap.count(finger2)) {
        if (path1.count(finger2)) {
            return finger2;
        }
        if (finger2 == idomMap[finger2]) break; // 到达根节点
        finger2 = idomMap[finger2];
    }

    return nullptr; // 理论上对于连接图总能找到（至少是入口块）
}

void DominatorTree::computeDominanceFrontiers(Function& F) {
    dominanceFrontiers.clear();
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* b = bb_ptr.get();

        if (b->predecessors.size() >= 2) {
            for (auto p : b->predecessors) {
                BasicBlock* runner = p;
                // 修正循环条件：确保 runner 不为空
                while (runner && runner != idomMap[b]) {
                    dominanceFrontiers[runner].insert(b);
                    
                    // 在赋值前最好也检查一下map中是否存在key，避免意外创建映射
                    auto it = idomMap.find(runner);
                    if (it != idomMap.end()) {
                        runner = it->second;
                    } else {
                        // 如果 runner 不在 idomMap 中（不应该发生，除非是入口块）
                        // 直接跳出循环
                        runner = nullptr;
                    }
                }
            }
        }
    }
    // debug输出
    // for (auto& bb : F.basicBlocks) {
    //     std::cout << "[DF] Block %" << bb->label << " DF: ";
    //     for (auto df : dominanceFrontiers[bb.get()]) std::cout << df->label << " ";
    //     std::cout << std::endl;
    // }
}

void DominatorTree::buildTree(Function& F) {
    // Clear any previous state
    childrenMap.clear();
    // 从 idomMap (子 -> 父) 构建 childrenMap (父 -> 子列表)。
    for (auto const& [child, parent] : idomMap) {
        if (parent) { // 根节点(入口块)的 parent 是 nullptr，不处理
            childrenMap[parent].push_back(child);
        }
    }
}

// 处理反向CFG的辅助方法
void DominatorTree::swapCFG(Function& F) {
    // 交换所有基本块的前驱和后继关系
    for (auto& bb : F.basicBlocks) {
        std::swap(bb->predecessors, bb->successors);
    }
}

BasicBlock* DominatorTree::getEntryBlock(Function& F, bool reverse) {
    if (!reverse) {
        // 正向支配树：返回函数的入口块
        return F.getEntryBlock();
    } else {
        // 后向支配树：返回函数的出口块
        // 注意：在swapCFG之后，原始CFG中的出口块（没有后继的块）
        // 变成了没有前驱的块
        // 我们在前置pass中保证了function只有一个出口块
        for (auto& bb : F.basicBlocks) {
            if (bb->predecessors.empty()) {
                return bb.get();
            }
        }
        // 如果没有找到出口块，返回最后一个基本块
        return F.basicBlocks.back().get();
    }
}

bool DominatorTree::dominates(BasicBlock* dom, BasicBlock* b) const {
    if (dom == b) return true;
    
    // 从b开始，沿着idomMap向上走到根，检查是否遇到dom
    BasicBlock* current = b;
    while (current) {
        auto it = idomMap.find(current);
        if (it == idomMap.end()) break;
        current = it->second;
        if (current == dom) return true;
    }
    return false;
}

// ================= InsertTransferBlock Implementation =================
// 生成唯一的标签名称
static std::string generateUniqueLabel(const std::string& base, Function* F) {
    std::set<std::string> existingLabels;
    for (const auto& bb : F->basicBlocks) {
        existingLabels.insert(bb->label);
    }
    
    std::string candidate = base;
    int idx = 1;
    while (existingLabels.count(candidate)) {
        candidate = base + "_" + std::to_string(idx++);
    }
    return candidate;
}

// 生成唯一的phi名称
static std::string getUniquePhiName(const std::string& base, const Function& F) {
    std::set<std::string> phi_names;
    for (const auto& bb_ptr : F.basicBlocks) {
        for (const auto& phi_ptr : bb_ptr->phi_instructions) {
            phi_names.insert(phi_ptr->name);
        }
    }
    std::string candidate = base;
    int idx = 1;
    while (phi_names.count(candidate)) {
        candidate = base + "_" + std::to_string(idx++);
    }
    return candidate;
}

BasicBlock* InsertTransferBlock(std::set<BasicBlock*>& froms, BasicBlock* to, bool insertAfterFrom) {
    assert(froms.size() >= 1);
    
    // 获取父函数
    Function* F = to->parent;
    if (!F) {
        std::cerr << "[CFG] Error: BasicBlock has no parent function" << std::endl;
        return nullptr;
    }
    
    // 创建中间基本块
    std::string midBlockLabel = generateUniqueLabel("transfer_block", F);
    auto midBB = std::make_unique<BasicBlock>(midBlockLabel, F);
    BasicBlock* midBBPtr = midBB.get();
    
    // 处理PHI指令
    for (auto& phi_ptr : to->phi_instructions) {
        if (phi_ptr->opcode != Opcode::PHI) {
            break;
        }
        
        // 创建新的PHI指令用于中间块
        auto midPhi = std::make_unique<PhiInst>(phi_ptr->type, getUniquePhiName(phi_ptr->name, *F));
        
        // 为每个from块添加incoming值到中间PHI
        for (auto from : froms) {
            // 找到对应from块的incoming值
            Value* incomingVal = nullptr;
            for (const auto& incoming : phi_ptr->incoming_values) {
                if (incoming.second == from) {
                    incomingVal = incoming.first;
                    break;
                }
            }
            
            if (incomingVal) {
                midPhi->addIncoming(incomingVal, from);
                // 从原始PHI中移除这个incoming
                // 注意：这里不能直接调用removeUse，因为我们需要从incoming_values中移除
                // 而不是从uses列表中移除
            }
        }
        
        // 从原始PHI中移除所有来自froms块的incoming
        auto& incoming_values = phi_ptr->incoming_values;
        incoming_values.erase(
            std::remove_if(incoming_values.begin(), incoming_values.end(),
                [&froms](const std::pair<Value*, BasicBlock*>& incoming) {
                    return froms.count(incoming.second) > 0;
                }),
            incoming_values.end()
        );
        
        // 将中间PHI添加到中间块
        midBB->addPhi(std::move(midPhi));
        
        // 为原始PHI添加来自中间块的新incoming
        // 注意：这里需要获取刚刚添加的PHI指令的指针
        if (!midBBPtr->phi_instructions.empty()) {
            phi_ptr->addIncoming(midBBPtr->phi_instructions.back().get(), midBBPtr);
        }
    }
    
    // 添加无条件分支指令到目标块
    auto brInst = std::make_unique<BranchInst>(to->label);
    midBB->addInstruction(std::move(brInst));
    
    // 更新from块的分支指令
    for (auto from : froms) {
        assert(from->instructions.size() >= 1);
        
        // 获取终止指令
        Instruction* terminator = from->getTerminator();
        if (!terminator) {
            std::cerr << "[CFG] Warning: Block " << from->label << " has no terminator" << std::endl;
            continue;
        }
        
        if (auto* br = dynamic_cast<BranchInst*>(terminator)) {
            if (!br->isConditional()) {
                // 无条件分支，直接修改目标
                if (br->getTrueLabel() == to->label) {
                    br->trueLabel = midBlockLabel;
                }
            } else {
                // 条件分支，需要修改对应的目标
                if (br->getTrueLabel() == to->label) {
                    br->trueLabel = midBlockLabel;
                }
                if (br->getFalseLabel() == to->label) {
                    br->falseLabel = midBlockLabel;
                }
            }
        }
    }
    
    // 将中间块插入到最后一个from块正后方
    size_t insertPos = 0;
    if (insertAfterFrom) {
        for (size_t i = 0; i < F->basicBlocks.size(); ++i) {
            for (auto from : froms) {
                if (F->basicBlocks[i].get() == from) {
                    if (i + 1 > insertPos) insertPos = i + 1;
                }
            }
        }
    } else {//在to之前添加
        for (size_t i = 0; i < F->basicBlocks.size(); ++i) {
            if (F->basicBlocks[i].get() == to) {
                insertPos = i;
                break;
            }
        }
    }
    F->insertBasicBlockAt(std::move(midBB), insertPos);
    
    // 重建CFG关系
    buildCFG(*F);
    
    return midBBPtr;
}

} // namespace cfg
} // namespace llvm_ir
