#ifndef LLVM_IR_GCM_H
#define LLVM_IR_GCM_H

#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>

#include "cfg.h"
#include "llvm_ir.h"
#include "loop_analysis.h"

namespace llvm_ir {
namespace gcm {

// Module-level entry point for GCM pass
bool runOnModule(Module& module);

/**
 * Global Code Motion (GCM) Implementation
 * Based on "Global Code Motion / Global Value Numbering" paper
 *
 * This implementation follows the 5-phase algorithm:
 * 1. Dominator Tree & Loop Analysis
 * 2. Schedule Early (forward pass)
 * 3. Schedule Late (backward pass)
 * 4. Select final position (heuristics)
 * 5. Code reschedule
 */

class GCMPass {
public:
    // Main entry point - returns true if any changes were made
    bool runOnFunction(Function& F);

private:
    // === Data structures for GCM ===

    std::map<BasicBlock*, int> dom_depth;                                     // Dominator tree depth for each basic block
    std::map<BasicBlock*, int> loop_nest;                                     // Loop nesting depth for each basic block
    std::map<Instruction*, BasicBlock*> early_block;                          // Early schedule position for each instruction
    std::map<Instruction*, BasicBlock*> late_block;                           // Late schedule position for each instruction
    std::map<Instruction*, BasicBlock*> final_block;                          // Final selected position for each instruction
    std::map<BasicBlock*, std::vector<Instruction*>> scheduled_instructions;  // Instructions scheduled to move to each basic block

    // Visited flags for early and late scheduling
    std::map<Instruction*, bool> early_visited;
    std::map<Instruction*, bool> late_visited;

    // Cache for dominator tree and loop analysis
    std::unique_ptr<cfg::DominatorTree> DT;
    std::unique_ptr<LoopAnalysis> LA;

    // === Phase 1: Preprocessing ===
    void computeDominatorDepths(Function& F);
    void computeLoopNestingDepths(Function& F);

    // === Phase 2: Schedule Early ===
    void scheduleEarly(Function& F);
    void scheduleEarlyForInstruction(Instruction* inst, BasicBlock* rootBlock);

    // === Phase 3: Schedule Late ===
    void scheduleLate(Function& F);
    void scheduleLateForInstruction(Instruction* inst);
    BasicBlock* computeLCA(const std::vector<BasicBlock*>& blocks);
    std::vector<BasicBlock*> getUseBlocks(Instruction* inst);

    // === Phase 4: Select Final Position ===
    void selectFinalPositions(Function& F);
    BasicBlock* selectBestBlock(Instruction* inst, BasicBlock* early, BasicBlock* late);

    // === Phase 5: Code Reschedule ===
    void rescheduleCode(Function& F);
    void moveInstructionToBlock(Instruction* inst, BasicBlock* targetBlock);

    // === Helper functions ===
    bool isPinned(Instruction* inst);
    bool isDominatedBy(BasicBlock* bb, BasicBlock* dom);

    // LCA computation helpers
    BasicBlock* findLCA(BasicBlock* a, BasicBlock* b);
    void alignDepths(BasicBlock*& a, BasicBlock*& b);
};

// Utility functions
bool isControlDependent(Instruction* inst);
bool canMoveInstruction(Instruction* inst);
std::vector<Instruction*> getInstructionInputs(Instruction* inst);

}  // namespace gcm
}  // namespace llvm_ir

#endif  // LLVM_IR_GCM_H
