#include "../../include/ir/inline.h"
#include "../../include/ir/cfg.h"
#include "../../include/ir/llvm_ir.h"
#include <vector>
#include <map>
#include <iostream>
#include <memory>
#include <algorithm>
#include <cassert>
#include <set> // Added for std::set
#include <functional>

namespace llvm_ir {
namespace inliner {

// Global counter for unique inlining identifiers
static int inlineCounter = 0;

// Forward declarations
bool inlineFunction(CallInst* CI, Function* Caller, Module& M);
bool canBeInlined(Function* Callee, Function* Caller, Module& M);
void fixupCFGAndPHIs(Function& F);

// Helper for RPO traversal
void postOrderTraversal(BasicBlock* BB, std::set<BasicBlock*>& visited, std::vector<BasicBlock*>& postOrder) {
    if (!BB || visited.count(BB)) {
        return;
    }
    visited.insert(BB);
    for (BasicBlock* succ : BB->successors) {
        postOrderTraversal(succ, visited, postOrder);
    }
    postOrder.push_back(BB);
}

// Function to check for duplicate labels in a function
bool hasDuplicateLabels(const Function& F) {
    std::set<std::string> labels;
    for (const auto& BB : F.basicBlocks) {
        if (labels.find(BB->label) != labels.end()) {
            // std::cout << "[inline] hasDuplicateLabels: Found duplicate label: " << BB->label << std::endl;
            return true;
        }
        labels.insert(BB->label);
    }
    return false;
}

// Function to check for duplicate variable names in a function
bool hasDuplicateVariableNames(const Function& F) {
    std::set<std::string> varNames;
    for (const auto& BB : F.basicBlocks) {
        // Check PHI instructions
        for (const auto& phi : BB->phi_instructions) {
            if (!phi->name.empty()) {
                if (varNames.find(phi->name) != varNames.end()) {
                    // std::cout << "[inline] hasDuplicateVariableNames: Found duplicate variable name: " << phi->name << std::endl;
                    return true;
                }
                varNames.insert(phi->name);
            }
        }
        
        // Check regular instructions
        for (const auto& instr : BB->instructions) {
            if (!instr->name.empty()) {
                if (varNames.find(instr->name) != varNames.end()) {
                    // std::cout << "[inline] hasDuplicateVariableNames: Found duplicate variable name: " << instr->name << std::endl;
                    return true;
                }
                varNames.insert(instr->name);
            }
        }
    }
    return false;
}

// Function to validate IR after inlining
bool validateInlinedIR(const Function& F) {
    // Check for duplicate labels
    if (hasDuplicateLabels(F)) {
        // std::cout << "[inline] validateInlinedIR: Duplicate labels found!" << std::endl;
        return false;
    }
    
    // Check for duplicate variable names
    if (hasDuplicateVariableNames(F)) {
        // std::cout << "[inline] validateInlinedIR: Duplicate variable names found!" << std::endl;
        return false;
    }
    
    // Check that all basic blocks have valid terminators
    for (const auto& BB : F.basicBlocks) {
        if (!BB->instructions.empty()) {
            Instruction* terminator = BB->getTerminator();
            if (!terminator) {
                // std::cout << "[inline] validateInlinedIR: Block " << BB->label << " has no terminator!" << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

// New function to immediately fixup PHI nodes after block splitting
void fixupPHIsAfterBlockSplit(BasicBlock* originalBlock, BasicBlock* newBlock, 
                              const std::string& newBlockLabel) {
    // std::cout << "[inline] fixupPHIsAfterBlockSplit: Fixing PHIs after splitting " 
    //           << originalBlock->label << " -> " << newBlockLabel << std::endl;
    
    // Collect all blocks that might have PHI nodes pointing to the original block
    std::set<BasicBlock*> blocksToCheck;
    
    // Add all successors of the original block
    for (BasicBlock* succ : originalBlock->successors) {
        blocksToCheck.insert(succ);
    }
    
    // Add all successors of the new block (in case it has any)
    for (BasicBlock* succ : newBlock->successors) {
        blocksToCheck.insert(succ);
    }
    
    // Fixup PHI nodes in all affected blocks
    for (BasicBlock* bb : blocksToCheck) {
        if (!bb || bb->phi_instructions.empty()) continue;
        
        // std::cout << "[inline] fixupPHIsAfterBlockSplit: Checking PHIs in " << bb->label << std::endl;
        
        for (auto& phi_ptr : bb->phi_instructions) {
            PhiInst* phi = phi_ptr.get();
            bool needsUpdate = false;
            
            // Check if this PHI has incoming values from the original block
            for (auto& incoming : phi->incoming_values) {
                if (incoming.second == originalBlock) {
                    // std::cout << "[inline] fixupPHIsAfterBlockSplit: PHI " << phi->name 
                    //           << " has incoming from " << originalBlock->label 
                    //           << ", updating to " << newBlockLabel << std::endl;
                    
                    // Update the incoming block to point to the new block
                    incoming.second = newBlock;
                    needsUpdate = true;
                }
            }
            
            if (needsUpdate) {
                // std::cout << "[inline] fixupPHIsAfterBlockSplit: Updated PHI " << phi->name << std::endl;
            }
        }
    }
}

// // Function to check if a function contains recursive calls (including self-recursion)
// bool hasRecursiveCalls(Function* F, std::set<Function*>& visited, Module& M) {
//     if (visited.find(F) != visited.end()) {
//         return true; // Found recursion
//     }
    
//     visited.insert(F);
    
//     for (const auto& BB : F->basicBlocks) {
//         for (const auto& I : BB->instructions) {
//             if (auto* CI = dynamic_cast<CallInst*>(I.get())) {
//                 // Check if this call is to the same function or a function we've already visited
//                 for (auto& func : M.functions) {
//                     if (func->name == CI->functionName) {
//                         if (visited.find(func.get()) != visited.end()) {
//                             // std::cout << "[inline] hasRecursiveCalls: Found recursive call to " << func->name << std::endl;
//                             return true;
//                         }
//                         // Recursively check the called function
//                         if (hasRecursiveCalls(func.get(), visited, M)) {
//                             return true;
//                         }
//                         break;
//                     }
//                 }
//             }
//         }
//     }
    
//     visited.erase(F);
//     return false;
// }

// Helper: collect direct callees of a function (by pointer) from the module
static void collectDirectCallees(Function* F, Module& M, std::vector<Function*>& out) {
    std::set<std::string> seenNames;
    for (const auto& BB : F->basicBlocks) {
        for (const auto& I : BB->instructions) {
            if (auto* CI = dynamic_cast<CallInst*>(I.get())) {
                // Avoid duplicate lookups for same function name
                if (seenNames.insert(CI->functionName).second) {
                    for (auto& func : M.functions) {
                        if (func->name == CI->functionName) {
                            out.push_back(func.get());
                            break;
                        }
                    }
                }
            }
        }
    }
}

// Check whether `start` participates in a cycle reachable from itself.
// Standard DFS with recursion-stack detection.
static bool isRecursive(Function* start, Module& M) {
    // We only consider `start` to be recursive if there exists a non-empty path
    // from `start` back to `start` (i.e. `start` participates in a cycle).
    if (!start) return false;

    std::set<Function*> visited;
    std::set<Function*> inStack;

    std::function<bool(Function*)> dfs = [&](Function* cur) -> bool {
        if (!cur) return false;
        visited.insert(cur);
        inStack.insert(cur);

        std::vector<Function*> callees;
        collectDirectCallees(cur, M, callees);
        for (Function* c : callees) {
            // If a callee is the original start, we've found a cycle that includes start
            if (c == start) return true;
            if (!visited.count(c)) {
                if (dfs(c)) return true;
            }
        }

        inStack.erase(cur);
        return false;
    };

    return dfs(start);
}

// Check whether `src` can reach `target` via call edges (simple reachability DFS).
static bool reaches(Function* src, Function* target, Module& M) {
    if (!src || !target) return false;
    if (src == target) return true;

    std::set<Function*> visited;
    std::function<bool(Function*)> dfs = [&](Function* cur) -> bool {
        if (!cur) return false;
        if (cur == target) return true;
        if (visited.count(cur)) return false;
        visited.insert(cur);

        std::vector<Function*> callees;
        collectDirectCallees(cur, M, callees);
        for (Function* c : callees) {
            if (dfs(c)) return true;
        }
        return false;
    };

    return dfs(src);
}

// A simple inlining heuristic.
bool canBeInlined(Function* Callee, Function* Caller, Module& M) {
    std::cout << "[inline] canBeInlined: Callee=" << Callee->name << ", Caller=" << Caller->name << std::endl;
    // Rule 1: Don't inline declarations (functions without a body).
    if (Callee->isDeclaration()) {
        // std::cout << "[inline] canBeInlined: Callee is declaration, not inlining." << std::endl;
        return false;
    }

    // 跳过global函数
    if (Callee->name == "global") {
        return false;
    }

    // Rule 2: Prevent direct recursion.
    if (Callee == Caller) {
        std::cout << "[inline] canBeInlined: Direct recursion detected, not inlining." << std::endl;
        return false;
    }

    // Rule 3: Check for recursive calls (including indirect recursion)
    // Use more precise checks:
    // 1) If Callee itself participates in a recursion cycle, don't inline it.
    // 2) If Callee can reach Caller via call edges, inlining would introduce recursion, so disallow it.
    if (isRecursive(Callee, M)) {
        std::cout << "[inline] canBeInlined: Callee participates in recursion, not inlining." << std::endl;
        return false;
    }

    if (reaches(Callee, Caller, M)) {
        std::cout << "[inline] canBeInlined: Inlining would introduce recursion (callee reaches caller), not inlining." << std::endl;
        return false;
    }

    // Rule 4: A simple size-based heuristic.
    unsigned instructionCount = 0;
    for (const auto& BB : Callee->basicBlocks) {
        instructionCount += BB->instructions.size();
    }
    // std::cout << "[inline] canBeInlined: instructionCount=" << instructionCount << std::endl;

    // Heuristic: only inline small functions
    if (instructionCount < 10000) {
        // std::cout << "[inline] canBeInlined: Callee is small, can inline." << std::endl;
        return true;
    }

    // std::cout << "[inline] canBeInlined: Callee too large, not inlining." << std::endl;
    return false;
}

// Main function to perform inlining on a given function.
bool runOnFunction(Function& F, Module& M) {
    // std::cout << "[inline] runOnFunction: Function=" << F.name << std::endl;
    bool overallChange = false;
    bool changedInIteration;
    do {
        changedInIteration = false;
        std::vector<CallInst*> worklist;

        // 1. Collect all call instructions in the current state of the function.
        for (auto& BB : F.basicBlocks) {
            for (auto& I : BB->instructions) {
                if (auto* CI = dynamic_cast<CallInst*>(I.get())) {
                    worklist.push_back(CI);
                }
            }
        }

        if (worklist.empty()) {
            break; // No more calls to process
        }

        // 2. Try to inline the first possible call.
        for (CallInst* CI : worklist) {
            Function* Callee = nullptr;
            for (auto& func : M.functions) {
                if (func->name == CI->functionName) {
                    Callee = func.get();
                    break;
                }
            }

            if (Callee && canBeInlined(Callee, &F, M)) {
                // std::cout << "[inline] runOnFunction: Inlining call to " << Callee->name << std::endl;
                if (inlineFunction(CI, &F, M)) {
                    changedInIteration = true;
                    overallChange = true;
                    // Inlining modified the function, so we must break and restart the scan.
                    // Note: PHI fixup is now done immediately during block splitting
                    break; 
                } else {
                    // std::cout << "[inline] runOnFunction: inlineFunction failed for " << Callee->name << std::endl;
                }
            }
        }
    } while (changedInIteration); // Continue until a full pass over the function makes no changes.
    
    return overallChange;
}

// The core inlining implementation.
bool inlineFunction(CallInst* CI, Function* Caller, Module& M) {
    std::cout << "[inline] inlineFunction: Inlining call to " << CI->functionName << " in " << Caller->name << std::endl;
    BasicBlock* callBlock = CI->parent;
    if (!callBlock) {
        // std::cout << "[inline] inlineFunction: callBlock is nullptr!" << std::endl;
        return false;
    }

    Function* Callee = nullptr;
    for (auto& func : M.functions) {
        if (func->name == CI->functionName) {
            Callee = func.get();
            break;
        }
    }

    if (!Callee || Callee->isDeclaration()) {
        std::cout << "[inline] inlineFunction: Callee not found or is declaration!" << std::endl;
        return false;
    }

    // 1. Split the basic block containing the call instruction.
    auto it = std::find_if(callBlock->instructions.begin(), callBlock->instructions.end(),
                           [CI](const std::unique_ptr<Instruction>& instr) { return instr.get() == CI; });

    if (it == callBlock->instructions.end()) {
        std::cout << "[inline][Warning] inlineFunction: CallInst not found in callBlock!" << std::endl;
        return false; // Should not happen
    }

    // Generate unique identifiers for this inlining operation
    int currentInlineId = ++inlineCounter;
    
    // Create a unique prefix for this inlining operation
    std::string inlinePrefix = std::to_string(currentInlineId);
    
    std::string after_inline_label = callBlock->label + "_" + inlinePrefix;
    auto afterCallBlock = std::make_unique<BasicBlock>(after_inline_label, Caller);

    // Move instructions after the call to the new block
    afterCallBlock->instructions.splice(afterCallBlock->instructions.begin(), callBlock->instructions, std::next(it), callBlock->instructions.end());
    
    // Update the parent pointer for the moved instructions.
    for (auto& instr : afterCallBlock->instructions) {
        instr->parent = afterCallBlock.get();
    }
    
    // IMMEDIATELY fixup PHI nodes that might be affected by this block split
    fixupPHIsAfterBlockSplit(callBlock, afterCallBlock.get(), after_inline_label);
    
    // The call instruction is now the last instruction in callBlock.
    // We will remove it later, after we are done using it.

    // 2. Value mapping for arguments and cloned instructions.
    std::map<Value*, Value*> valueMap;
    for (size_t i = 0; i < Callee->parameters.size(); ++i) {
        valueMap[Callee->parameters[i]] = CI->operands[i];
    }

    // 3. Clone Callee's basic blocks, manage ownership correctly.
    std::map<BasicBlock*, BasicBlock*> blockMap;
    std::vector<std::unique_ptr<BasicBlock>> newBlocks; // Hold new blocks to manage their lifetime
    newBlocks.reserve(Callee->basicBlocks.size());

    for (const auto& calleeBB : Callee->basicBlocks) {
        auto newBB = std::make_unique<BasicBlock>(Callee->name + "_" + calleeBB->label + "_" + inlinePrefix, Caller);
        blockMap[calleeBB.get()] = newBB.get();
        newBlocks.push_back(std::move(newBB));
    }

    // Branch from the call block to the Callee's entry block.
    IRBuilder builder;
    builder.setInsertPoint(callBlock);

    // 4. Clone instructions from Callee's blocks to the new blocks.
    Value* returnValue = nullptr;
    std::vector<std::pair<BasicBlock*, Value*>> incomingReturnValues;

    auto get_mapped_value = [&](Value* v) -> Value* {
        if (!v) return nullptr;
        if (dynamic_cast<Constant*>(v) || dynamic_cast<UndefValue*>(v)) {
            return v; // Constants and Undef values are not mapped, used directly
        }
        // Add an explicit check for GlobalVariable
        if (dynamic_cast<GlobalVariable*>(v)) {
            return v;
        }
        // Global variables (starting with '@') should be used directly
        if (v->name.size() > 0 && v->name[0] == '@') {
            return v;
        }
        if (valueMap.count(v)) {
            return valueMap.at(v);
        }
        
        // If we get here, it's a bug. A value that should have been mapped was not.
        std::cerr << "FATAL INLINER ERROR: Value '" << v->name << "' not found in valueMap." << std::endl;
        assert(false && "Value not found in valueMap during inlining");
        return nullptr;
    };

    // Get blocks in RPO to ensure defs are processed before uses for reachable code.
    cfg::buildCFG(*Callee);
    std::vector<BasicBlock*> rpoBlocks;
    std::set<BasicBlock*> visited;
    {
        std::vector<BasicBlock*> postOrder;
        if (!Callee->basicBlocks.empty()) {
            postOrderTraversal(Callee->getEntryBlock(), visited, postOrder);
        }
        rpoBlocks.assign(postOrder.rbegin(), postOrder.rend());
    }

    // Create a list of all blocks to process: reachable in RPO, then unreachable ones.
    // This ensures that unreachable blocks are still cloned and not left empty, which
    // would result in invalid IR.
    std::vector<BasicBlock*> allBlocksToProcess = rpoBlocks;
    for (const auto& bb : Callee->basicBlocks) {
        if (visited.find(bb.get()) == visited.end()) {
            allBlocksToProcess.push_back(bb.get());
        }
    }

    for (BasicBlock* calleeBB : allBlocksToProcess) {
        BasicBlock* newBB = blockMap[calleeBB];
        builder.setInsertPoint(newBB);

        // First, clone and map all PHI nodes. This is not fully robust for complex CFGs
        // but is necessary to handle cases where instructions depend on PHI results.
        for (const auto& phi_ptr : calleeBB->phi_instructions) {
            std::string newPhiName = phi_ptr->name.empty() ? "" :  phi_ptr->name + "_" + inlinePrefix;
            auto newPhi = std::make_unique<PhiInst>(phi_ptr->type, newPhiName);
            newPhi->parent = newBB;
            // The operands will be filled in a second pass to handle dependencies.
            valueMap[phi_ptr.get()] = newPhi.get();
            newBB->addPhi(std::move(newPhi));
        }

        for (const auto& instr_ptr : calleeBB->instructions) {
            const Instruction& instr = *instr_ptr;
            std::unique_ptr<Instruction> newInst = nullptr;
            std::string new_name = instr.name.empty() ? "" : instr.name + "_" + inlinePrefix;

            switch (instr.opcode) {
                case Opcode::Ret: {
                    auto retInst = static_cast<const ReturnInst*>(&instr);
                    if (!retInst->operands.empty()) {
                        Value* retVal = get_mapped_value(retInst->operands[0]);
                        incomingReturnValues.push_back({newBB, retVal});
                        // std::cout << "[inline] inlineFunction: Return value mapped." << std::endl;
                    }
                    builder.createBr(after_inline_label);
                    break;
                }
                case Opcode::Br: {
                    auto brInst = static_cast<const BranchInst*>(&instr);
                    if (!brInst->falseLabel.empty()) { // Conditional
                        Value* cond = get_mapped_value(brInst->operands[0]);
                        BasicBlock* trueDest = Callee->getBlockByName(brInst->trueLabel);
                        BasicBlock* falseDest = Callee->getBlockByName(brInst->falseLabel);
                        builder.createCondBr(cond, blockMap.at(trueDest)->label, blockMap.at(falseDest)->label);
                    } else { // Unconditional
                        BasicBlock* dest = Callee->getBlockByName(brInst->trueLabel);
                        builder.createBr(blockMap.at(dest)->label);
                    }
                    break;
                }
                default: {
                    // Use the generic cloneInstruction function for all other instructions
                    newInst = cloneInstruction(&instr, valueMap, "_" + inlinePrefix);
                    break;
                }
            }
            
            if (newInst) {
                valueMap[instr_ptr.get()] = newInst.get();
                newBB->addInstruction(std::move(newInst));
            }
        }
    }
    
    // Now, populate the incoming values for the cloned PHI nodes.
    for (BasicBlock* calleeBB : rpoBlocks) {
        BasicBlock* newBB = blockMap[calleeBB];
        for (size_t i = 0; i < calleeBB->phi_instructions.size(); ++i) {
            PhiInst* originalPhi = calleeBB->phi_instructions[i].get();
            PhiInst* clonedPhi = newBB->phi_instructions[i].get();

            for (const auto& incoming : originalPhi->incoming_values) {
                Value* mapped_val = get_mapped_value(incoming.first);
                BasicBlock* mapped_block = blockMap.at(incoming.second);
                clonedPhi->addIncoming(mapped_val, mapped_block);
            }
        }
    }

    // 5. Handle the return value.
    if (!CI->getUses().empty()) { // Only create a PHI node if the return value is used.
        // For non-phi instructions, the builder's insert point is fine.
        builder.setInsertPoint(afterCallBlock.get()); 

        if (incomingReturnValues.size() == 1) {
            returnValue = incomingReturnValues[0].second;
            // std::cout << "[inline] inlineFunction: Single return value." << std::endl;
        } else if (incomingReturnValues.size() > 1) {
            // Manually create and insert the PHI node to ensure it's at the top of the block.
            auto phi = std::make_unique<PhiInst>(CI->type, CI->name);
            phi->parent = afterCallBlock.get();
            for (auto const& [pred, val] : incomingReturnValues) {
                phi->addIncoming(val, pred);
            }
            returnValue = phi.get();

            // Use the dedicated method to add PHI nodes.
            afterCallBlock->addPhi(std::move(phi));
            // std::cout << "[inline] inlineFunction: Multiple return values, PHI created." << std::endl;
        }
    }
    
    // 6. Update uses of the original call instruction.
    if (returnValue) {
        CI->replaceAllUsesWith(returnValue);
        // std::cout << "[inline] inlineFunction: Replaced uses of call with return value." << std::endl;
    }
    
    // 7. Disconnect the call instruction from its operands to prevent dangling pointers.
    for (Value* op : CI->operands) {
        if (op) {
            op->removeUse(CI);
        }
    }
    CI->operands.clear();
    
    // 8. Now that it's disconnected, remove the instruction from its parent block.
    // This leaves callBlock without a terminator.
    auto final_it = std::find_if(callBlock->instructions.begin(), callBlock->instructions.end(),
                               [CI](const auto& instr) { return instr.get() == CI; });
    if (final_it != callBlock->instructions.end()) {
        // Set the parent to nullptr before removal to prevent issues
        CI->parent = nullptr;
        callBlock->instructions.erase(final_it);
    }
    
    // 9. Now, add the branch to the inlined function's entry.
    builder.setInsertPoint(callBlock);
    builder.createBr(blockMap[Callee->getEntryBlock()]->label);
    
    // Find the position of the call block in the function's basic blocks list
    size_t callBlockPosition = 0;
    for (size_t i = 0; i < Caller->basicBlocks.size(); ++i) {
        if (Caller->basicBlocks[i].get() == callBlock) {
            callBlockPosition = i;
            break;
        }
    }
    
    // Insert the inlined blocks at the call site position
    for (auto& bb : newBlocks) {
        Caller->insertBasicBlockAt(std::move(bb), callBlockPosition + 1);
        // Caller->addBasicBlock(std::move(bb));
        callBlockPosition++; // Update position for next insertion
    }
    
    // Insert the after-call block right after the inlined blocks
    auto* afterCallBlockPtr = afterCallBlock.get();
    Caller->insertBasicBlockAt(std::move(afterCallBlock), callBlockPosition + 1);
    // Caller->addBasicBlock(std::move(afterCallBlock));

    // Fixup successors for blocks that were split
    try {
        cfg::buildCFG(*Caller);
        // std::cout << "[inline] inlineFunction: CFG rebuilt successfully" << std::endl;
    } catch (...) {
        std::cerr << "[inline][Warning] inlineFunction: Warning: buildCFG failed, attempting manual fixup" << std::endl;
        // Manual fixup as fallback
        for(auto& pair : blockMap) {
            if(auto term = pair.second->getTerminator()) {
               if(auto br = dynamic_cast<BranchInst*>(term)) {
                   if(br->getTrueLabel() == after_inline_label) {
                       pair.second->successors.push_back(afterCallBlockPtr);
                       afterCallBlockPtr->predecessors.push_back(pair.second);
                   }
                   if(br->isConditional() && br->getFalseLabel() == after_inline_label)
                   {
                       pair.second->successors.push_back(afterCallBlockPtr);
                       afterCallBlockPtr->predecessors.push_back(pair.second);
                   }
               }
            }
        }
    }

    // This part is now redundant as we replaced all uses and removed the instruction.
    // if (returnValue) {
    //     CI->replaceAllUsesWith(returnValue);
    //     std::cout << "[inline] inlineFunction: Replaced uses of call with return value." << std::endl;
    // }
    
    // Final validation: check that all PHI nodes are consistent
    bool phiConsistent = true;
    for (auto& BB : Caller->basicBlocks) {
        if (!BB || BB->phi_instructions.empty()) continue;
        
        std::set<BasicBlock*> actualPreds(BB->predecessors.begin(), BB->predecessors.end());
        
        for (auto& phi_ptr : BB->phi_instructions) {
            PhiInst* phi = phi_ptr.get();
            std::set<BasicBlock*> phiPreds;
            
            for (const auto& incoming : phi->incoming_values) {
                if (incoming.second) {
                    phiPreds.insert(incoming.second);
                }
            }
            
            // Check if PHI has incoming values for all actual predecessors
            for (BasicBlock* pred : actualPreds) {
                if (phiPreds.find(pred) == phiPreds.end()) {
                    std::cout << "[inline][Warning] inlineFunction: PHI " << phi->name 
                              << " missing incoming value for predecessor " << pred->label << std::endl;
                    phiConsistent = false;
                }
            }
        }
    }
    
    if (!phiConsistent) {
        std::cout << "[inline][Warning] inlineFunction: Some PHI nodes are inconsistent" << std::endl;
    }
    
    // Final validation of the inlined IR
    // if (!validateInlinedIR(*Caller)) { //TODO, 没有这个hFunction的28 side effect 会在SCCP段错误，why
    //     std::cout << "[inline][Error] inlineFunction: Inlined IR validation failed" << std::endl;
    //     return false;
    // }
    
    // std::cout << "[inline] inlineFunction: Inlining finished for " << CI->functionName << std::endl;
    return true;
}

// Helper function to fixup CFG and PHI nodes after inlining
void fixupCFGAndPHIs(Function& F) {
    cfg::buildCFG(F);
    
    // Then fixup PHI nodes to match the new CFG
    for (auto& BB : F.basicBlocks) {
        if (!BB || BB->phi_instructions.empty()) continue;
        // std::cout << "[inline] fixupCFGAndPHIs: Fixing up PHI nodes for " << BB->label << std::endl;

        // --- Pass 1: Create new PHIs and a map from old to new ---
        std::map<PhiInst*, PhiInst*> phi_map;
        std::vector<std::unique_ptr<PhiInst>> new_phi_storage;
        new_phi_storage.reserve(BB->phi_instructions.size());

        for (const auto& old_phi_ptr : BB->phi_instructions) {
            auto new_phi = std::make_unique<PhiInst>(old_phi_ptr->type, old_phi_ptr->name);
            new_phi->parent = BB.get();
            phi_map[old_phi_ptr.get()] = new_phi.get();
            new_phi_storage.push_back(std::move(new_phi));
        }

        // --- Pass 2: Populate the new PHIs with correct incoming values ---
        for (const auto& old_phi_ptr : BB->phi_instructions) {
            PhiInst* old_phi = old_phi_ptr.get();
            PhiInst* new_phi = phi_map[old_phi];

            std::set<BasicBlock*> actualPreds(BB->predecessors.begin(), BB->predecessors.end());
            std::map<BasicBlock*, Value*> oldIncomingMap;
            std::set<BasicBlock*> oldPreds;
            for (const auto& incoming : old_phi->incoming_values) {
                if (incoming.second) {
                    oldIncomingMap[incoming.second] = incoming.first;
                    oldPreds.insert(incoming.second);
                }
            }
            
            std::vector<BasicBlock*> lostPreds;
            std::set_difference(oldPreds.begin(), oldPreds.end(),
                                actualPreds.begin(), actualPreds.end(),
                                std::back_inserter(lostPreds));

            std::vector<BasicBlock*> newPreds;
            std::set_difference(actualPreds.begin(), actualPreds.end(),
                                oldPreds.begin(), oldPreds.end(),
                                std::back_inserter(newPreds));

            // a. Add entries for predecessors that still exist.
            for (auto pred : actualPreds) {
                if (oldPreds.count(pred)) { // This predecessor existed before
                    Value* incoming_val = oldIncomingMap[pred];
                    // Check if the value is one of the PHIs being replaced in this block
                    if (auto incoming_phi = dynamic_cast<PhiInst*>(incoming_val)) {
                        if (phi_map.count(incoming_phi)) {
                            incoming_val = phi_map[incoming_phi];
                        }
                    }
                    new_phi->addIncoming(incoming_val, pred);
                }
            }

            // b. Handle new predecessors by mapping from lost ones.
            if (!newPreds.empty()) {
                Value* val_for_new_preds = nullptr;
                if (newPreds.size() == 1 && lostPreds.size() == 1 && oldIncomingMap.count(lostPreds[0])) {
                    val_for_new_preds = oldIncomingMap[lostPreds[0]];
                } else {
                    val_for_new_preds = new UndefValue(old_phi->type);
                }
                // Check if this value itself needs mapping
                if (auto incoming_phi = dynamic_cast<PhiInst*>(val_for_new_preds)) {
                    if (phi_map.count(incoming_phi)) {
                        val_for_new_preds = phi_map[incoming_phi];
                    }
                }
                for (auto pred : newPreds) {
                    new_phi->addIncoming(val_for_new_preds, pred);
                }
            }
        }

        // std::cout << "[inline] fixupCFGAndPHIs: Pass 2 done" << std::endl;

        // --- Pass 3: Replace all uses of old PHIs with new PHIs ---
        for (const auto& old_phi_ptr : BB->phi_instructions) {
             if (phi_map.count(old_phi_ptr.get())) {
                PhiInst* new_phi = phi_map[old_phi_ptr.get()];
                if (new_phi->incoming_values.size() > 0) {
                    try {
                        old_phi_ptr->replaceAllUsesWith(new_phi);
                    } catch (...) {
                        std::cerr << "[inline] fixupCFGAndPHIs: Warning: replaceAllUsesWith failed for PHI " 
                                  << old_phi_ptr->name << std::endl;
                    }
                }
             }
        }

        // std::cout << "[inline] fixupCFGAndPHIs: Pass 3 done" << std::endl;

        // --- Pass 4: Replace the old PHI list with the new one ---
        BB->phi_instructions.clear();
        for (auto& new_phi : new_phi_storage) {
            BB->addPhi(std::move(new_phi));
        }
    }
}

// Entry point for the inlining pass on a module.
bool runOnModule(Module& M) {
    // std::cout << "[inline] runOnModule: Start" << std::endl;
    
    // Reset the inline counter for this module pass
    inlineCounter = 0;
    
    bool madeChange = false;
    std::vector<Function*> functions;
    for (auto& F : M.functions) {
        functions.push_back(F.get());
    }

    for (auto* F : functions) {
        if (!F->isDeclaration()) {
            // std::cout << "[inline] runOnModule: Processing " << F->name << std::endl;
            if (runOnFunction(*F, M)) {
                madeChange = true;
                // std::cout << "[inline] runOnModule: Inlining changed " << F->name << std::endl;
            }
        }
    }

    if (madeChange) {
        // std::cout << "[inline] runOnModule: Rebuilding analysis data." << std::endl;
        // Rebuild analysis data after modifications.
        cfg::rebuildUseDefChainsModule(M);
        for(auto& F : M.functions) {
            if (!F->isDeclaration()) {
                cfg::buildCFG(*F);
            }
        }
    }

    // std::cout << "[inline] runOnModule: Finished" << std::endl;
    return madeChange;
}

} // namespace inliner
} // namespace llvm_ir