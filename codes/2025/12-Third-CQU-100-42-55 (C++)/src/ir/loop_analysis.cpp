#include "../../include/ir/loop_analysis.h"
#include "../../include/ir/cfg.h"
#include "../../debug.h"
#include <iostream>
#include <queue>
#include <algorithm>
#include <map>
#include <set>
#include <functional>
#include <cassert>

namespace llvm_ir {

void LoopAnalysis::runOnFunction(Function& F, const cfg::DominatorTree& DT) {
    loops.clear();
    std::cout << "[loop_anlys] Start loop analysis on function: " << F.name << std::endl;
    
    std::map<BasicBlock*, Loop*> header_to_loop;

    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* B = bb_ptr.get();
        for (BasicBlock* succ : B->successors) {
            // If successor dominates current block, it's a back edge
            if (DT.dominates(succ, B)) {
                // std::cout << "[loop_anlys]   Found back edge: " << B->label << " -> " << succ->label << std::endl;
                
                Loop* loop = nullptr;
                if (header_to_loop.count(succ)) {
                    loop = header_to_loop[succ];
                } else {
                    auto new_loop = std::make_unique<Loop>(succ); // succ是循环头
                    loop = new_loop.get();
                    header_to_loop[succ] = loop;
                    loops.push_back(std::move(new_loop));
                }
                
                // Use a worklist to find all blocks in the loop
                std::vector<BasicBlock*> worklist;
                if (!loop->contains(succ)) {
                    loop->addBlock(succ);
                }
                
                if (!loop->contains(B)) {
                    loop->addBlock(B);
                    worklist.push_back(B);
                }

                while (!worklist.empty()) {
                    BasicBlock* current = worklist.back();
                    worklist.pop_back();

                    for (BasicBlock* pred : current->predecessors) {
                        if (!loop->contains(pred)) {
                            loop->addBlock(pred);
                            worklist.push_back(pred);
                            // std::cout << "[loop_anlys]     Add block to loop: " << pred->label << std::endl;
                        }
                    }
                }
            }
        }
    }

    // Debug print
    for (const auto& loop_ptr : loops) {
        std::cout << "[loop_anlys]   Final Loop header: " << loop_ptr->getHeader()->label << " blocks: ";
        for (auto* blk : loop_ptr->getBlocks()) std::cout << blk->label << " ";
        std::cout << std::endl;
    }
    
    std::cout << "[loop_anlys] Loop analysis finished, total loops: " << loops.size() << std::endl;

    // ====== 嵌套关系分析 ======
    // 先建立父指针
    for (auto& outer_ptr : loops) {
        llvm_ir::Loop* outer = outer_ptr.get();
        for (auto& inner_ptr : loops) {
            llvm_ir::Loop* inner = inner_ptr.get();
            if (outer == inner) continue;
            const auto& inner_blocks = inner->getBlocks();
            const auto& outer_blocks = outer->getBlocks();
            // outer严格包含inner
            if (std::includes(outer_blocks.begin(), outer_blocks.end(),
                              inner_blocks.begin(), inner_blocks.end())) {
                // 只保留最外层父亲
                if (!inner->getParent() || 
                    outer->getBlocks().size() < inner->getParent()->getBlocks().size()) {
                    inner->setParent(outer);
                }
            }
        }
    }
    // 填充children
    for (auto& loop_ptr : loops) {
        llvm_ir::Loop* loop = loop_ptr.get();
        if (loop->getParent()) {
            loop->getParent()->addChild(loop);
        }
    }
    // 输出嵌套结构
    std::cout << "[loop_anlys] Loop Nesting Structure:" << std::endl;
    // 递归打印循环嵌套结构，避免lambda递归捕获自身
    std::function<void(llvm_ir::Loop*, int)> printLoop;
    printLoop = [&printLoop](llvm_ir::Loop* l, int depth) {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "Loop header: " << l->getHeader()->label << std::endl;
        for (auto* child : l->getChildren()) {
            printLoop(child, depth + 1);
        }
    };
    for (const auto& loop_ptr : loops) {
        llvm_ir::Loop* loop = loop_ptr.get();
        if (!loop->getParent()) {
            printLoop(loop, 0);
        }
    }

    // ====== 找到latches和exit nodes ======
    for (auto& loop_ptr : loops) {
        Loop* loop = loop_ptr.get();
        loop->findLatches();
        loop->findExitNodes(F);
    }

    // 重新计算 preheader（如果需要）
    for (auto& loop_ptr : loops) {
        Loop* loop = loop_ptr.get();
        std::vector<BasicBlock*> preds_outside_loop;
        for (BasicBlock* pred : loop->getHeader()->predecessors) {
            if (!loop->contains(pred)) {
                preds_outside_loop.push_back(pred);
            }
        }
        
        if (preds_outside_loop.size() == 1) {
            BasicBlock* single_pred = preds_outside_loop[0];
            if (single_pred->successors.size() == 1) {
                loop->setPreheader(single_pred);
                std::cout << "[reAnalysisLoop]   Found existing preheader: " << single_pred->label << std::endl;
            }
        }
    }

    #if LOOP_COMMENT
        commentLoops();
    #endif
}

void LoopAnalysis::reAnalysisLoop(Loop* loop, Function& F, const cfg::DominatorTree& DT) {
    if (!loop) {
        std::cerr << "[reAnalysisLoop] Error: loop is nullptr" << std::endl;
        return;
    }
    
    std::cout << "[reAnalysisLoop] Re-analyzing loop with header: " << loop->getHeader()->label << std::endl;
    
    // 清空循环的现有信息
    loop->blocks.clear();
    loop->exitBlocks.clear();
    loop->latches.clear();
    loop->exitingBlocks.clear();
    loop->preheader = nullptr;
    
    // 重新找到循环的所有基本块
    BasicBlock* header = loop->getHeader();
    std::vector<BasicBlock*> worklist;
    
    // 添加循环头
    loop->addBlock(header);
    
    // 找到所有回边并收集循环内的基本块
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* B = bb_ptr.get();
        for (BasicBlock* succ : B->successors) {
            // 如果后继支配当前块，这是一个回边
            if (succ == header && DT.dominates(succ, B)) {
                // std::cout << "[reAnalysisLoop]   Found back edge: " << B->label << " -> " << succ->label << std::endl;
                
                if (!loop->contains(B)) {
                    loop->addBlock(B);
                    worklist.push_back(B);
                }
            }
        }
    }
    
    // 使用工作列表找到循环内的所有基本块
    while (!worklist.empty()) {
        BasicBlock* current = worklist.back();
        worklist.pop_back();

        for (BasicBlock* pred : current->predecessors) {
            if (!loop->contains(pred)) {
                loop->addBlock(pred);
                worklist.push_back(pred);
                // std::cout << "[reAnalysisLoop]     Add block to loop: " << pred->label << std::endl;
            }
        }
    }
    
    // 重新找到 latches 和 exit nodes
    loop->findLatches();
    loop->findExitNodes(F);
    
    // 重新计算 preheader（如果需要）
    std::vector<BasicBlock*> preds_outside_loop;
    for (BasicBlock* pred : header->predecessors) {
        if (!loop->contains(pred)) {
            preds_outside_loop.push_back(pred);
        }
    }
    
    if (preds_outside_loop.size() == 1) {
        BasicBlock* single_pred = preds_outside_loop[0];
        if (single_pred->successors.size() == 1) {
            loop->setPreheader(single_pred);
            std::cout << "[reAnalysisLoop]   Found existing preheader: " << single_pred->label << std::endl;
        }
    }

    #if LOOP_COMMENT
        commentLoops();
    #endif
    
    // 输出重新分析的结果
    // std::cout << "[reAnalysisLoop]   Re-analysis completed for loop: " << header->label << std::endl;
    // loop->printLoopInfo();
}

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

// 确保每个循环头都有唯一的前置块（preheader），用于简化后续优化和分析。
// 对于每个循环：
//   1. 找到循环头的所有前驱中不属于循环的块（即从循环外进入循环头的块）。
//   2. 如果只有一个且它只指向循环头，则直接作为preheader。
//   3. 否则，创建一个新的preheader块，将所有外部前驱的跳转目标改为preheader，再由preheader无条件跳转到循环头。
//   4. 处理循环头中的PHI指令，将来自外部前驱的值合并到preheader（如有多个外部前驱则在preheader新建PHI）。
//   5. 更新CFG结构，保证循环头的所有外部前驱都通过preheader进入。
void LoopAnalysis::ensurePreheaders(Function& F, const cfg::DominatorTree& DT) {
    std::cout << "[loop_anlys] Ensuring preheaders..." << std::endl;
    std::vector<std::unique_ptr<Loop>> final_loops;

    for (auto& loop_ptr : loops) {
        Loop* loop = loop_ptr.get();
        BasicBlock* header = loop->getHeader();
        std::cout << "[loop_anlys]   Loop header: " << header->label << std::endl;
        std::vector<BasicBlock*> preds_outside_loop;
        
        for (BasicBlock* pred : header->predecessors) {
            if (!loop->contains(pred)) {
                preds_outside_loop.push_back(pred);
            }
        }
        std::cout << "[loop_anlys]     Predecessors outside loop: ";
        for (auto* blk : preds_outside_loop) std::cout << blk->label << " ";
        std::cout << std::endl;

        if (preds_outside_loop.size() == 1) { // 只有一个外部前驱
            BasicBlock* single_pred = preds_outside_loop[0];
            if (single_pred->successors.size() == 1) {
                std::cout << "[loop_anlys]     Found existing preheader: " << single_pred->label << std::endl;
                loop->setPreheader(single_pred);
                continue;
            }
        }

        if (!preds_outside_loop.empty()) { // 有外部前驱，而且不止一个，创建新的 preheader
            std::string header_label = header->label.empty() ? "unnamed" : header->label;
            std::string preheader_name = generateUniqueLabel("loop_preheader_" + header_label, &F);
            auto new_preheader_ptr = std::make_unique<BasicBlock>(preheader_name, header->parent);
            BasicBlock* preheader = new_preheader_ptr.get();

            auto it = std::find_if(F.basicBlocks.begin(), F.basicBlocks.end(),
                [header](const std::unique_ptr<BasicBlock>& bb) { return bb.get() == header; });
            F.basicBlocks.insert(it, std::move(new_preheader_ptr));

            for (BasicBlock* pred : preds_outside_loop) {
                if (auto* term = pred->getTerminator()) {
                    if (auto* br = dynamic_cast<BranchInst*>(term)) {
                        if (br->isConditional()) {
                            if (br->trueLabel == header->label) br->trueLabel = preheader->label;
                            if (br->falseLabel == header->label) br->falseLabel = preheader->label;
                        } else {
                            if (br->trueLabel == header->label) br->trueLabel = preheader->label;
                        }
                    }
                }
                std::replace(pred->successors.begin(), pred->successors.end(), header, preheader);
            }
            
            preheader->addInstruction(std::make_unique<BranchInst>(header->label));
            preheader->successors.push_back(header);
            preheader->predecessors = preds_outside_loop;

            header->predecessors.erase(std::remove_if(header->predecessors.begin(), header->predecessors.end(),
                [&](BasicBlock* p) { return std::find(preds_outside_loop.begin(), preds_outside_loop.end(), p) != preds_outside_loop.end(); }),
                header->predecessors.end());
            header->predecessors.push_back(preheader);

            for (auto& phi_ptr : header->phi_instructions) {
                PhiInst* phi = phi_ptr.get();
                std::vector<std::pair<Value*, BasicBlock*>> outside_incomings;
                std::vector<std::pair<Value*, BasicBlock*>> inside_incomings;
                
                for (auto& incoming : phi->incoming_values) {
                    bool is_outside = std::find(preds_outside_loop.begin(), preds_outside_loop.end(), incoming.second) != preds_outside_loop.end();
                    if (is_outside) {
                        outside_incomings.push_back(incoming);
                    } else {
                        inside_incomings.push_back(incoming);
                    }
                }

                if (outside_incomings.empty()) {
                    continue; // Should not happen if preheader was created
                }

                Value* val_from_preheader = nullptr;
                if (outside_incomings.size() > 1) {
                    auto new_phi = std::make_unique<PhiInst>(phi->type, phi->name + "_preheader");
                    for (const auto& incoming : outside_incomings) {
                        new_phi->addIncoming(incoming.first, incoming.second);
                    }
                    val_from_preheader = new_phi.get();
                    preheader->addPhi(std::move(new_phi));
                } else {
                    val_from_preheader = outside_incomings[0].first;
                }
                
                inside_incomings.push_back({val_from_preheader, preheader});
                phi->incoming_values = inside_incomings;
            }

            loop->setPreheader(preheader);
            std::cout << "[loop_anlys]     Created new preheader: " << preheader->label << std::endl;
        } else {
            std::cout << "[loop_anlys][Warning]     No predecessors from outside the loop. No preheader needed." << std::endl;
            loop->setPreheader(nullptr); // Or handle as an error/special case
        }
    }
    // ... optional CFG print
}

// --- LoopAnalysis类方法实现 ---

void LoopAnalysis::normalizeLoops(Function& F) {
    std::cout << "[LoopAnalysis] Starting loop normalization..." << std::endl;
    
    // 首先执行ensurePreheaders
    std::cout << "[LoopAnalysis] Step 1: Ensuring preheaders..." << std::endl;
    cfg::DominatorTree DT;
    DT.runOnFunction(F);
    ensurePreheaders(F, DT);
    
    // 为每个循环找到latches和exit nodes
    std::cout << "[LoopAnalysis] Step 2: Finding latches and exit nodes..." << std::endl;
    for (auto& loop_ptr : loops) {
        Loop* loop = loop_ptr.get();
        loop->findLatches();
        loop->findExitNodes(F);
    }
    
    // 按后序遍历循环（从内层到外层）
    std::vector<Loop*> loopsInPostorder;
    std::function<void(Loop*)> postorder = [&](Loop* L) {
        for (auto child : L->getChildren()) {
            postorder(child);
        }
        loopsInPostorder.push_back(L);
    };
    
    // 收集最外层循环
    for (auto& loop_ptr : loops) {
        if (!loop_ptr->getParent()) {
            postorder(loop_ptr.get());
        }
    }
    
    // 对每个循环执行规范化
    std::cout << "[LoopAnalysis] Step 3: Performing loop normalization..." << std::endl;
    for (Loop* loop : loopsInPostorder) {
        std::cout << "[LoopAnalysis] Normalizing loop with header: " 
                  << loop->getHeader()->label << std::endl;
        
        // 执行SingleLatchInsert
        // loop->SingleLatchInsert(F);
        
        // 执行ExitInsert
        loop->ExitInsert(F);
    }
    
    // 重新分析loop TODO，只有变化了才重新分析
    DT.runOnFunction(F);
    runOnFunction(F, DT);
    std::cout << "[LoopAnalysis] Loop normalization completed" << std::endl;
}

// --- LCSSA实现 ---

// 辅助函数：获取在循环外使用的操作数
static std::tuple<std::set<Value*>, std::map<Value*, Type>> GetUsedOperandOutOfLoop(Function& F, Loop* L) {
    std::set<Value*> varList;
    std::map<Value*, Type> typeMap;
    
    // 收集循环内定义的所有值
    for (auto bb : L->getBlocks()) {
        // 检查PHI指令
        for (auto& phi : bb->phi_instructions) {
            if (phi) {
                varList.insert(phi.get());
                typeMap[phi.get()] = phi->type;
            }
        }
        
        // 检查普通指令
        for (auto& inst : bb->instructions) {
            if (inst) {
                varList.insert(inst.get());
                typeMap[inst.get()] = inst->type;
            }
        }
    }
    
    std::set<Value*> regUsedSet;  // 在循环内定义但在循环外使用的值
    std::map<Value*, Type> regUsedTypeMap;
    
    // 检查循环外的使用
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (L->contains(bb)) {
            continue;  // 在循环内
        }
        
        // 跳过exit nodes中的PHI指令（我们假设这些值在循环内使用）
        // if (L->getExitBlocks().count(bb) > 0) { // TODO
        //     continue;
        // }
        // 不应该跳过exit nodes，因为可能exit node里面也在use循环中的值
        
        // 检查指令的操作数
        for (auto& inst : bb->instructions) {
            for (auto operand : inst->operands) {
                if (varList.count(operand) > 0) {
                    regUsedSet.insert(operand);
                    regUsedTypeMap[operand] = typeMap[operand];
                }
            }
        }

        // 跳过exit nodes中的PHI指令（我们假设这些值在循环内使用）
        if (L->getExitBlocks().count(bb) > 0) { // TODO
            continue;
        }
        
        // 检查PHI指令的incoming values
        for (auto& phi : bb->phi_instructions) {
            for (auto& incoming : phi->incoming_values) {
                if (varList.count(incoming.first) > 0) {
                    regUsedSet.insert(incoming.first);
                    regUsedTypeMap[incoming.first] = typeMap[incoming.first];
                }
            }
        }
    }
    
    return std::make_tuple(regUsedSet, regUsedTypeMap);
}

void Loop::LCSSA(Function& F) {
    std::cout << "[LCSSA] Processing loop with header: " << header->label << std::endl;
    
    // 现在忽略有多个退出的循环
    if (exitBlocks.size() > 1) {
        std::cout << "[LCSSA]   Skipping loop with multiple exits (" << exitBlocks.size() << " exits)" << std::endl;
        return;
    }
    
    auto [vset, typeMap] = GetUsedOperandOutOfLoop(F, this);
    std::map<Value*, Value*> replaceMap;
    
    if (exitBlocks.empty()) {
        std::cout << "[LCSSA]   No exit blocks found" << std::endl;
        return;
    }
    
    auto exitBB = *exitBlocks.begin();
    std::cout << "[LCSSA]   Exit block: " << exitBB->label << std::endl;
    std::cout << "[LCSSA]   Values used outside loop: " << vset.size() << std::endl;
    
    // 为每个在循环外使用的值创建PHI指令
    for (auto v : vset) {
        std::string phiName = v->name + "_lcssa";
        auto phiInst = std::make_unique<PhiInst>(typeMap[v], phiName);
        PhiInst* phiPtr = phiInst.get();
        
        // 为exit block的所有前驱添加incoming
        for (auto pred : exitBB->predecessors) {
            phiPtr->addIncoming(v, pred);
        }
        
        // 将PHI指令插入到exit block的开头
        exitBB->addPhi(std::move(phiInst));
        
        replaceMap[v] = phiPtr;
        std::cout << "[LCSSA]        " << v->name << " -> " << phiName << std::endl;
    }
    
    // 替换循环外对原值的使用
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (this->contains(bb)) {
            continue;  // 在循环内
        }
        
        // 替换指令中的操作数
        for (auto& inst : bb->instructions) {
            for (size_t i = 0; i < inst->operands.size(); ++i) {
                auto it = replaceMap.find(inst->operands[i]);
                if (it != replaceMap.end()) {
                    // 移除旧操作数的use
                    inst->operands[i]->removeUse(inst.get());
                    // 设置新操作数
                    inst->operands[i] = it->second;
                    // 添加新操作数的use
                    it->second->addUse(inst.get());
                }
            }
        }
        
        // 替换PHI指令中的incoming values
        if (bb == exitBB) { // 因为前面保证了exitblock是被循环内dominate的，所以exit里面不会有除了lcssa的phi
            std::cout << "[LCSSA]   Skip phi in exit block: " << bb->label << std::endl;
            continue;
        }
        for (auto& phi : bb->phi_instructions) {
            for (auto& incoming : phi->incoming_values) {
                auto it = replaceMap.find(incoming.first);
                if (it != replaceMap.end()) {
                    // 移除旧值的use
                    incoming.first->removeUse(phi.get());
                    // 设置新值
                    incoming.first = it->second;
                    // 添加新值的use
                    it->second->addUse(phi.get());
                }
            }
        }
    }
    
    std::cout << "[LCSSA]   LCSSA transformation completed for loop" << std::endl;
}

void LoopAnalysis::runLCSSA(Function& F) {
    std::cout << "[LCSSA] Starting LCSSA transformation on function: " << F.name << std::endl;
    
    // 为每个循环执行LCSSA转换
    for (auto& loop_ptr : loops) {
        Loop* loop = loop_ptr.get();
        // 有可能子loop在normalize或者其他地方更新了自己的结构，这个时候就需要重新分析了
        cfg::DominatorTree DT;
        DT.runOnFunction(F);
        reAnalysisLoop(loop, F, DT);
        //End
        loop->LCSSA(F);
    }
    
    std::cout << "[LCSSA] LCSSA transformation completed" << std::endl;
}

// --- Loop类方法实现 ---

void Loop::findLatches() {
    latches.clear();
    for (BasicBlock* block : blocks) {
        for (BasicBlock* succ : block->successors) {
            if (succ == header) {
                latches.insert(block);
                std::cout << "[Loop] Found latch: " << block->label << " -> " << header->label << std::endl;
            }
        }
    }
}

void Loop::findExitNodes(Function& F) {
    exitBlocks.clear();
    exitingBlocks.clear();
    
    for (BasicBlock* block : blocks) {
        for (BasicBlock* succ : block->successors) {
            if (!contains(succ)) {
                exitBlocks.insert(succ);
                exitingBlocks.insert(block);
                std::cout << "[Loop] Found exit: " << block->label << " -> " << succ->label << std::endl;
            }
        }
    }
}

void Loop::SingleLatchInsert(Function& F) {
    assert(latches.size() >= 1);

    // 如果latch中只有br指令和phi指令，则不需要SingleLatchInsert
    bool onlyBrInst = true;
    for (auto* latch : latches) {
        for (auto& inst : latch->instructions) {
            if (auto* br = dynamic_cast<BranchInst*>(inst.get())) {
                
            } else {
                onlyBrInst = false;
                break;
            }
        }
    }
    if (onlyBrInst) {
        return;
    }
    
    std::cout << "[Loop] SingleLatchInsert for loop with header: " << header->label << std::endl;
    
    // 使用InsertTransferBlock创建新的latch
    auto newLatch = llvm_ir::cfg::InsertTransferBlock(latches, header, true);
    if (!newLatch) {
        std::cerr << "[Loop] Error: Failed to create new latch" << std::endl;
        return;
    }
    
    // 清空原有latches并添加新的latch
    latches.clear();
    latches.insert(newLatch);
    blocks.insert(newLatch);
    
    // 更新父循环的loop nodes
    Loop* current = this;
    while (current->parent != nullptr) {
        current = current->parent;
        current->blocks.insert(newLatch);
    }
    
    std::cout << "[Loop]   New latch: " << newLatch->label << std::endl;
}

void Loop::ExitInsert(Function& F) {
    std::cout << "[Loop] ExitInsert for loop with header: " << header->label << std::endl;
    
    // 清空并重新找到exit nodes
    exitBlocks.clear();
    exitingBlocks.clear();
    findExitNodes(F);
    
    std::set<BasicBlock*> inloopPreblocks;
    std::map<BasicBlock*, BasicBlock*> exitMap;
    
    for (auto exit : exitBlocks) {
        inloopPreblocks.clear();
        bool isDomExit = true;
        
        // 检查exit的所有前驱，如果所有前驱都在循环内，则跳过
        for (auto preBB : exit->predecessors) {
            if (blocks.find(preBB) != blocks.end()) {
                inloopPreblocks.insert(preBB);  // 在循环内
            } else {  // 在循环外
                isDomExit = false;
            }
        }
        
        if (isDomExit) {
            std::cout << "[Loop]   Skip dominated exit: " << exit->label << std::endl;
            continue;
        }
        
        if (inloopPreblocks.empty()) {
            std::cerr << "[Loop]   No in-loop predecessors for exit: " << exit->label << std::endl;
            continue;
        }
        
        // 创建新的exit块
        auto newExit = llvm_ir::cfg::InsertTransferBlock(inloopPreblocks, exit, false);
        if (!newExit) {
            std::cerr << "[Loop] Error: Failed to create new exit for " << exit->label << std::endl;
            continue;
        }
        
        // 更新父循环的loop nodes
        Loop* current = this;
        while (current->parent != nullptr) {
            current = current->parent;
            
            if (current->blocks.find(exit) != current->blocks.end()) {
                current->blocks.insert(newExit);
            }
            
            // 重新计算父循环的exit nodes
            current->exitBlocks.clear();
            current->exitingBlocks.clear();
            current->findExitNodes(F);
        }
        
        exitMap[exit] = newExit;
        std::cout << "[Loop]   Created new exit: " << newExit->label << " for " << exit->label << std::endl;
    }
    
    // 更新exit nodes映射
    for (auto [pre, now] : exitMap) {
        exitBlocks.erase(pre);
        exitBlocks.insert(now);
    }
    
    std::cout << "[Loop]   Final exit blocks: ";
    for (auto* exit : exitBlocks) {
        std::cout << exit->label << " ";
    }
    std::cout << std::endl;
}

// ========debug========
void LoopAnalysis::commentLoops() {
    // 按照由顶层到底层的顺序为loop编号
    int loopcount = 0;
    std::function<void(Loop*)> commentLoop;
    commentLoop = [&](Loop* loop) {
        loopcount++;
        int this_loop_id = loopcount;
        for (auto* block : loop->getBlocks()) {
            block->comments.push_back("Loop_" + std::to_string(this_loop_id) + ": ");
            if (block == loop->header) {
                continue;
            }
            if (loop->latches.count(block) > 0) {
                continue;
            }
            if (loop->exitingBlocks.count(block) > 0) {
                continue;
            }
            block->comments.push_back("Body");
        }
        loop->header->comments.push_back("Header");
        for (auto* latch : loop->latches) {
            latch->comments.push_back("Latch");
        }
        for (auto* exiting : loop->exitingBlocks) {
            exiting->comments.push_back("Exiting");
        }
        for (auto* exit : loop->exitBlocks) {
            exit->comments.push_back("Exit");
        }
        // 递归处理子循环
        for (auto* child : loop->getChildren()) {
            commentLoop(child);
        }
    };
    // 只从最外层loop开始递归
    for (auto& loop_ptr : loops) {
        Loop* loop = loop_ptr.get();
        if (!loop->getParent()) {
            commentLoop(loop);
        }
    }
}

void Loop::printLoopInfo() {
   std::cout << "[Loop] Loop with header: " << (header ? header->label : "nullptr") << std::endl;
   std::cout << "   Latches: ";
   for (auto* latch : latches) {
       std::cout << latch->label << " ";
   }
   std::cout << std::endl;
   std::cout << "   Exit blocks: ";
   for (auto* exit : exitBlocks) {
       std::cout << exit->label << " ";
   }
   std::cout << std::endl;
   std::cout << "   Exiting nodes: ";
   for (auto* exiting : exitingBlocks) {
       std::cout << exiting->label << " ";
   }
   std::cout << std::endl;
   std::cout << "   Blocks: ";
   for (auto* block : blocks) {
       std::cout << block->label << " ";
   }
   std::cout << std::endl;
   std::cout << "   Preheader: " << (preheader ? preheader->label : "nullptr") << std::endl;
   std::cout << "   Parent: " << (parent ? parent->header->label : "nullptr") << std::endl;
   std::cout << "   Children: ";
   for (auto* child : children) {
       std::cout << child->header->label << " ";
   }
   std::cout << std::endl;
}

} // namespace llvm_ir