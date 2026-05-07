#ifndef LOOP_DEPENDENCE_ANALYSIS_H
#define LOOP_DEPENDENCE_ANALYSIS_H

#include "../pass.h"
#include "AliasAnalysis.h"
#include "ScalarEvolution.h"
#include "loop.h"
#include "../../include/cfg.h"
#include <map>
#include <vector>
#include <set>

typedef bool isLoopDep;

// 循环依赖分析Pass
class LoopDependenceAnalysisPass : public IRPass {
private:
    AliasAnalysisPass* alias_analyser;
    std::map<Loop*, isLoopDep> loop_dependence_map;

    isLoopDep analyzeGEPDependence(Instruction I1, Instruction I2, ScalarEvolution* SE, Loop* L);
    bool detectLoopCarriedDependencies(Loop* loop, CFG* cfg, ScalarEvolution* SE);
    void analyzeLoopDependencies(Loop* loop, CFG* cfg, ScalarEvolution* SE);
    Instruction tracePhiToGEP(Instruction phi_inst, const std::map<int, Instruction>& inst_registry);
    
    // Helper functions for dependence analysis
    std::pair<std::map<int, Instruction>, std::pair<std::vector<LoadInstruction*>, std::vector<StoreInstruction*>>> 
    buildInstructionRegistry(Loop* loop, CFG* cfg);
    bool analyzeMemoryAccessPatterns(const std::pair<std::vector<LoadInstruction*>, std::vector<StoreInstruction*>>& mem_ops,
                                   const std::map<int, Instruction>& inst_registry, ScalarEvolution* SE, Loop* loop, CFG* cfg);
    bool checkMemoryDependence(BasicInstruction* inst1, BasicInstruction* inst2, 
                              const std::map<int, Instruction>& inst_registry, ScalarEvolution* SE, Loop* loop, CFG* cfg);
    
public:
    LoopDependenceAnalysisPass(LLVMIR* IR, AliasAnalysisPass* AA) : IRPass(IR), alias_analyser(AA) {}
    void Execute() override;
    isLoopDep getLoopDependenceResult(Loop* loop) const;
    const std::map<Loop*, isLoopDep>& getAllLoopDependenceResults() const { return loop_dependence_map; }
    
    // Display function for results
    void displayResults() const;
};

#endif // LOOP_DEPENDENCE_ANALYSIS_H
