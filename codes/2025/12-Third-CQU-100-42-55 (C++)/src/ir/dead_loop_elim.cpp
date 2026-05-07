#include "../../include/ir/dead_loop_elim.h"
#include "../../include/ir/loop_analysis.h"
#include "../../include/ir/function_analysis.h"
#include "../../include/ir/cfg.h"

#include <iostream>
#include <vector>
#include <set>
#include <unordered_set>
#include <queue>
#include <functional>

namespace llvm_ir {

// Helper to find all exit blocks of a loop
static std::vector<BasicBlock*> findLoopExitBlocks(const Loop* L) {
    std::vector<BasicBlock*> exitBlocks;
    std::set<BasicBlock*> seen;
    
    for (BasicBlock* B : L->getBlocks()) {
        for (BasicBlock* succ : B->successors) {
            if (!L->contains(succ) && seen.find(succ) == seen.end()) {
                exitBlocks.push_back(succ);
                seen.insert(succ);
            }
        }
    }
    return exitBlocks;
}

// Generic instruction safety check based on side effects analysis
static bool isInstructionSafeToRemove(Instruction* inst, const Loop* L, const function_analysis::FunctionAttributeAnalyzer* analyzer = nullptr) {
    switch (inst->opcode) {
        case Opcode::Store:
            return false; // Memory writes have side effects
            
        case Opcode::Call: {
            auto call = dynamic_cast<CallInst*>(inst);
            if (!call) return false;
            
            // Check if the call has a return value that might be used
            if (call->type != Type::Void) {
                return false; // Functions with return values should not be removed
            }
            
            // Use function_analysis to check if the call is safe
            if (analyzer) {
                return analyzer->isCallSafe(call);
            }
            // Fallback: be conservative if no analyzer available
            return false;
        }
        
        case Opcode::Load: {
            // Loads from loop-invariant locations might be safe
            // Will be checked by use analysis
            return true;
        }
        
        case Opcode::Br:
        case Opcode::Ret:
            // Control flow instructions are safe if loop can be bypassed
            return true;
            
        default:
            // Arithmetic, logic, and conversion operations are generally safe
            return true;
    }
}

// Helper: 判断是否递归或间接递归调用
static bool isRecursiveOrIndirectRecursive(const CallInst* call, const Function* currentFunc, const function_analysis::FunctionAttributeAnalyzer* analyzer) {
    if (!call || !currentFunc) return false;
    // 直接递归
    if (call->functionName == currentFunc->name) return true;
    // 间接递归：利用调用图
    // 这里只做简单的直接递归和同名检测，间接递归可扩展
    return false;
}

// Helper: 检查指令是否有关键操作（return、递归调用、数组访问等）
static bool hasCriticalOperations(Instruction* inst, const Function* currentFunc, const function_analysis::FunctionAttributeAnalyzer* analyzer) {
    if (!inst) return false;
    
    switch (inst->opcode) {
        case Opcode::Ret:
            return true; // Return语句绝对不能消除
            
        case Opcode::Call: {
            auto call = dynamic_cast<CallInst*>(inst);
            if (!call) return false;
            
            // 递归调用绝对不能消除
            if (isRecursiveOrIndirectRecursive(call, currentFunc, analyzer)) {
                return true;
            }
            
            // 有返回值的函数调用不能消除
            if (call->type != Type::Void) {
                return true;
            }
            
            // 有副作用的函数调用不能消除
            if (analyzer && !analyzer->isCallSafe(call)) {
                return true;
            }
            
            return false;
        }
        
        case Opcode::Store:
            return true; // 内存写入不能消除
            
        case Opcode::Load: {
            // 数组访问需要特别检查
            auto load = dynamic_cast<LoadInst*>(inst);
            if (load) {
                // 检查是否访问数组（通过GEP指令）
                Value* ptr = load->getPointerOperand();
                if (ptr && ptr->type == Type::Ptr) {
                    // 数组访问可能影响程序语义，保守处理
                    return true;
                }
            }
            return false;
        }
        
        case Opcode::GetElementPtr:
            return true; // 数组访问不能消除
            
        default:
            return false;
    }
}

// Check if a loop contains any instructions that affect program semantics
static bool hasSemanticEffects(const Loop* L, const function_analysis::FunctionAttributeAnalyzer* analyzer = nullptr) {
    for (BasicBlock* B : L->getBlocks()) {
        for (auto& inst_ptr : B->instructions) {
            Instruction* inst = inst_ptr.get();
            
            // Check for memory writes
            if (inst->opcode == Opcode::Store) {
                return true;
            }
            
            // Check for function calls with side effects
            if (inst->opcode == Opcode::Call) {
                auto call = dynamic_cast<CallInst*>(inst);
                if (call) {
                    // Functions with return values affect semantics
                    if (call->type != Type::Void) {
                        return true;
                    }
                    
                    // Check if function has side effects
                    if (analyzer && !analyzer->isCallSafe(call)) {
                        return true;
                    }
                }
            }
            
            // Check for control flow that affects program behavior
            if (inst->opcode == Opcode::Ret) {
                return true;
            }
            
            // Check for array access
            if (inst->opcode == Opcode::GetElementPtr) {
                return true;
            }
        }
    }
    return false;
}

// Check if removing a loop would leave any PHI nodes in an invalid state
static bool wouldBreakPhiNodes(const Loop* L, const std::vector<BasicBlock*>& exitBlocks) {
    for (BasicBlock* exitBlock : exitBlocks) {
        for (auto& phi : exitBlock->phi_instructions) {
            // Check if any incoming values come from loop blocks
            bool hasLoopIncoming = false;
            bool hasNonLoopIncoming = false;
            
            for (const auto& incoming : phi->incoming_values) {
                if (L->contains(incoming.second)) {
                    hasLoopIncoming = true;
                } else {
                    hasNonLoopIncoming = true;
                }
            }
            
            // If PHI has incoming from both loop and non-loop blocks,
            // we need to handle it carefully
            if (hasLoopIncoming && hasNonLoopIncoming) {
                continue; // We'll handle this in the elimination phase
            }
        }
    }
    return false;
}

// Fix PHI nodes after loop elimination
static void fixPhiNodesAfterElimination(const Loop* L, const std::vector<BasicBlock*>& exitBlocks, BasicBlock* preheader) {
    for (BasicBlock* exitBlock : exitBlocks) {
        for (auto& phi : exitBlock->phi_instructions) {
            // Remove incoming values from loop blocks and update with preheader
            std::vector<std::pair<Value*, BasicBlock*>> newIncoming;
            Value* loopValue = nullptr;
            
            for (const auto& incoming : phi->incoming_values) {
                if (L->contains(incoming.second)) {
                    // This incoming value is from the loop
                    if (!loopValue) {
                        loopValue = incoming.first;
                    }
                } else {
                    // Keep non-loop incoming values
                    newIncoming.push_back(incoming);
                }
            }
            
            // If there was a value coming from the loop, add it as coming from preheader
            if (loopValue && preheader) {
                newIncoming.push_back({loopValue, preheader});
            }
            
            // Update the PHI node
            phi->incoming_values = newIncoming;
        }
    }
}

// Enhanced loop elimination that handles multiple exit blocks
static bool eliminateLoop(Loop* L) {
    BasicBlock* preheader = L->getPreheader();
    if (!preheader) {
        return false; // Need a preheader to modify control flow
    }
    
    std::vector<BasicBlock*> exitBlocks = findLoopExitBlocks(L);
    if (exitBlocks.empty()) {
        return false; // Infinite loop, cannot eliminate safely
    }
    
    // For multiple exit blocks, we need to be more careful
    if (exitBlocks.size() == 1) {
        // Simple case: single exit block
        BasicBlock* exitBlock = exitBlocks[0];
        
        // Fix PHI nodes before changing control flow
        fixPhiNodesAfterElimination(L, exitBlocks, preheader);
        
        // Change preheader's terminator to jump directly to the exit block
        Instruction* terminator = preheader->getTerminator();
        if (auto br = dynamic_cast<BranchInst*>(terminator)) {
            br->removeFromParent(); // Remove the old branch
            
            IRBuilder builder;
            builder.setInsertPoint(preheader);
            builder.createBr(exitBlock->getName());
            
            return true;
        }
    } else {
        // Multiple exit blocks - more complex case
        // For now, we'll handle simple cases where the preheader has a conditional
        // branch that can be simplified to jump to one of the exits
        
        Instruction* terminator = preheader->getTerminator();
        if (auto br = dynamic_cast<BranchInst*>(terminator)) {
            if (br->isConditional()) {
                // For conditional branches leading into loops with multiple exits,
                // we could potentially optimize by analyzing which exit is more likely
                // For now, skip these complex cases
                return false;
            } else {
                // Unconditional branch - we can try to find the most common exit
                if (!exitBlocks.empty()) {
                    // Use the first exit block as a simple heuristic
                    BasicBlock* targetExit = exitBlocks[0];
                    
                    fixPhiNodesAfterElimination(L, exitBlocks, preheader);
                    
                    br->removeFromParent();
                    IRBuilder builder;
                    builder.setInsertPoint(preheader);
                    builder.createBr(targetExit->getName());
                    
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool DeadLoopElimination::runOnFunction(Function& F) {
    bool changed = false;
    bool iterationChanged = true;
    int iterations = 0;
    const int MAX_ITERATIONS = 5; // Prevent infinite loops
    
    std::cout << "[DeadLoopElim] 分析函数: " << F.name << std::endl;
    
    function_analysis::FunctionAttributeAnalyzer analyzer(&_module);
    while (iterationChanged && iterations < MAX_ITERATIONS) {
        iterationChanged = false;
        iterations++;
        std::cout << "[DeadLoopElim] 迭代 " << iterations << std::endl;
        cfg::rebuildUseDefChainsOnFunction(F, false);
        cfg::buildCFG(F);
        cfg::DominatorTree DT;
        DT.runOnFunction(F);
        LoopAnalysis LA;
        LA.runOnFunction(F, DT);
        LA.ensurePreheaders(F, DT);
        std::vector<Loop*> worklist;
        std::function<void(Loop*)> collectLoops = [&](Loop* L) -> void {
            for (Loop* child : L->getChildren()) collectLoops(child);
            worklist.push_back(L);
        };
        for (const auto& loop : LA.loops) {
            if (loop->getParent() == nullptr) collectLoops(loop.get());
        }
        for (Loop* L : worklist) {
            std::cout << "[DeadLoopElim] 检查循环，头部: " << (L->getHeader() ? L->getHeader()->getName() : "null") << std::endl;
            BasicBlock* preheader = L->getPreheader();
            if (!preheader) { std::cout << "[DeadLoopElim] 循环没有前置头部，跳过" << std::endl; continue; }
            std::vector<BasicBlock*> exitBlocks = findLoopExitBlocks(L);
            if (exitBlocks.empty()) { std::cout << "[DeadLoopElim] 循环没有退出块（可能是无限循环），跳过" << std::endl; continue; }
            if (wouldBreakPhiNodes(L, exitBlocks)) { std::cout << "[DeadLoopElim] 消除循环会破坏PHI节点，跳过" << std::endl; continue; }
            
            // 检查循环是否包含关键操作
            bool hasCriticalOps = false;
            for (BasicBlock* B : L->getBlocks()) {
                for (auto& inst_ptr : B->instructions) {
                    if (hasCriticalOperations(inst_ptr.get(), &F, &analyzer)) {
                        hasCriticalOps = true;
                        std::cout << "[DeadLoopElim] 循环包含关键操作: " << inst_ptr->toString() << std::endl;
                        break;
                    }
                }
                if (hasCriticalOps) break;
            }
            if (hasCriticalOps) {
                std::cout << "[DeadLoopElim] 循环包含关键操作，跳过" << std::endl;
                continue;
            }
            
            bool canDelete = true;
            std::vector<Instruction*> potentiallyDeadInstructions;
            for (BasicBlock* B : L->getBlocks()) {
                for (auto& inst_ptr : B->instructions) {
                    Instruction* inst = inst_ptr.get();
                    
                    // 3. use-def链分析
                    for (Instruction* user : inst->getUses()) {
                        if (!L->contains(user)) {
                            canDelete = false;
                            break;
                        }
                    }
                    if (!canDelete) break;
                    
                    potentiallyDeadInstructions.push_back(inst);
                }
                
                for (auto& phi_ptr : B->phi_instructions) {
                    PhiInst* phi = phi_ptr.get();
                    
                    for (Instruction* user : phi->getUses()) {
                        if (!L->contains(user)) {
                            canDelete = false;
                            break;
                        }
                    }
                    if (!canDelete) break;
                    
                    potentiallyDeadInstructions.push_back(phi);
                }
                
                if (!canDelete) break;
            }
            if (canDelete) {
                std::cout << "[DeadLoopElim] 可以安全删除循环，包含 " << potentiallyDeadInstructions.size() << " 条指令" << std::endl;
                if (eliminateLoop(L)) {
                    std::cout << "[DeadLoopElim] 成功消除循环" << std::endl;
                    iterationChanged = true;
                    changed = true;
                    break; // Process one loop at a time to maintain CFG consistency
                } else {
                    std::cout << "[DeadLoopElim] 循环消除失败" << std::endl;
                }
            } else {
                std::cout << "[DeadLoopElim] 循环不能安全删除" << std::endl;
            }
        }
    }
    if (changed) {
        std::cout << "[DeadLoopElim] 重建use-def链和CFG" << std::endl;
        cfg::rebuildUseDefChainsOnFunction(F, false);
        cfg::buildCFG(F);
    }
    std::cout << "[DeadLoopElim] 函数 " << F.name << " 处理完成，改变: " << (changed ? "是" : "否") << std::endl;
    if (changed) {
        std::cerr << "[DeadLoopElim] 消除了循环，这是一个未经大量测试的pass，可能有误。" << std::endl;
    }
    return changed;
}

bool DeadLoopElimination::run() {
    bool changed = false;
    std::cout << "[DeadLoopElim] 开始死循环消除优化" << std::endl;
    
    // First, analyze function attributes for the entire module
    function_analysis::analyzeFunctionAttributes(_module);
    
    // Create a function attribute analyzer for use in loop elimination
    function_analysis::FunctionAttributeAnalyzer analyzer(&_module);
    
    for (auto& F : _module.functions) {
        if (!F->isDeclaration()) {
            changed |= runOnFunction(*F);
        }
    }
    
    if (changed) {
        // Final module-level cleanup
        cfg::rebuildUseDefChainsModule(_module);
        for (auto& F : _module.functions) {
            if (!F->isDeclaration()) {
                cfg::buildCFG(*F);
            }
        }
    }
    
    std::cout << "[DeadLoopElim] 死循环消除优化完成，改变: " << (changed ? "是" : "否") << std::endl;
    return changed;
}

} // namespace llvm_ir