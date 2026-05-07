#include "LoopDependenceAnalysis.h"
#include <iostream>

// Debug开关 - 可以通过编译时定义来控制
#ifndef LOOP_DEPENDENCE_DEBUG
//#define LOOP_DEPENDENCE_DEBUG 1
#endif

#if LOOP_DEPENDENCE_DEBUG
#define DEPENDENCE_DEBUG(x) do { x; } while(0)
#else
#define DEPENDENCE_DEBUG(x) do {} while(0)
#endif

void LoopDependenceAnalysisPass::Execute() {
    loop_dependence_map.clear();
    
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        auto loop_info = cfg->getLoopInfo();
        auto SE = cfg->getSCEVInfo();
        
        if (!loop_info || !SE) continue;
        
        for (auto loop : loop_info->getTopLevelLoops()) {
            analyzeLoopDependencies(loop, cfg, SE);
        }
    }
    
    // Display results
    displayResults();
}

void LoopDependenceAnalysisPass::analyzeLoopDependencies(Loop* loop, CFG* cfg, ScalarEvolution* SE) {
    // Process nested loops first
    for (auto subloop : loop->getSubLoops()) {
        analyzeLoopDependencies(subloop, cfg, SE);
    }
    
    // Analyze current loop
    loop_dependence_map[loop] = detectLoopCarriedDependencies(loop, cfg, SE);
}

bool LoopDependenceAnalysisPass::detectLoopCarriedDependencies(Loop* loop, CFG* cfg, ScalarEvolution* SE) {
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Starting loop carried dependence detection for loop with header " << loop->getHeader()->block_id << std::endl);
    
    if (!alias_analyser) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: No alias analyser available - assuming dependence" << std::endl);
        return true;
    }
    
    // Phase 1: Build instruction registry and collect memory operations
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Building instruction registry..." << std::endl);
    auto result = buildInstructionRegistry(loop, cfg);
    auto& inst_registry = result.first;
    auto& mem_ops = result.second;
    
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Found " << mem_ops.first.size() << " loads and " << mem_ops.second.size() << " stores" << std::endl);
    
    // If function calls were found, return conservative result
    if (mem_ops.first.empty() && mem_ops.second.empty()) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Function calls detected - assuming dependence" << std::endl);
        return true;
    }
    
    // Phase 2: Analyze memory access patterns
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Analyzing memory access patterns..." << std::endl);
    return analyzeMemoryAccessPatterns(mem_ops, inst_registry, SE, loop, cfg);
}

// Helper function to build instruction registry and collect memory operations
std::pair<std::map<int, Instruction>, std::pair<std::vector<LoadInstruction*>, std::vector<StoreInstruction*>>> 
LoopDependenceAnalysisPass::buildInstructionRegistry(Loop* loop, CFG* cfg) {
    std::map<int, Instruction> inst_registry;
    std::vector<LoadInstruction*> loads;
    std::vector<StoreInstruction*> stores;
    
    // Build instruction registry
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Building instruction registry..." << std::endl);
    for (auto [id, bb] : *cfg->block_map) {
        for (auto I : bb->Instruction_list) {
            int def_reg = I->GetDefRegno();
            if (def_reg != -1) {
                inst_registry[def_reg] = I;
                DEPENDENCE_DEBUG(std::cout << "DEBUG: Mapped reg " << def_reg << " to instruction with opcode " << I->GetOpcode() << std::endl);
            }
        }
    }
    
    // Collect memory operations
    for (auto block : loop->getBlocks()) {
        for (auto I : block->Instruction_list) {
            switch (I->GetOpcode()) {
                case BasicInstruction::LOAD:
                    loads.push_back((LoadInstruction*)I);
                    break;
                case BasicInstruction::STORE:
                    stores.push_back((StoreInstruction*)I);
                    break;
                case BasicInstruction::CALL:
                    // Function calls indicate potential dependencies
                    return {{}, {{}, {}}}; // Return empty to trigger conservative handling
                default:
                    break;
            }
        }
    }
    
    return {inst_registry, {loads, stores}};
}

// Helper function to analyze memory access patterns
bool LoopDependenceAnalysisPass::analyzeMemoryAccessPatterns(
    const std::pair<std::vector<LoadInstruction*>, std::vector<StoreInstruction*>>& mem_ops,
    const std::map<int, Instruction>& inst_registry, ScalarEvolution* SE, Loop* loop, CFG* cfg) {
    
    auto [loads, stores] = mem_ops;
    
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Checking " << loads.size() << "x" << stores.size() << " load-store pairs" << std::endl);
    
    // Check load-store dependencies
    for (auto load : loads) {
        for (auto store : stores) {
            DEPENDENCE_DEBUG(std::cout << "DEBUG: Checking load-store dependence..." << std::endl);
            if (checkMemoryDependence((BasicInstruction*)load, (BasicInstruction*)store, inst_registry, SE, loop, cfg)) {
                DEPENDENCE_DEBUG(std::cout << "DEBUG: Load-store dependence found" << std::endl);
                return true;
            }
        }
    }
    
    // Check store-store dependencies
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Checking " << stores.size() << "x" << stores.size() << " store-store pairs" << std::endl);
    for (size_t i = 0; i < stores.size(); ++i) {
        for (size_t j = i + 1; j < stores.size(); ++j) {
            DEPENDENCE_DEBUG(std::cout << "DEBUG: Checking store-store dependence..." << std::endl);
            if (checkMemoryDependence((BasicInstruction*)stores[i], (BasicInstruction*)stores[j], inst_registry, SE, loop, cfg)) {
                DEPENDENCE_DEBUG(std::cout << "DEBUG: Store-store dependence found" << std::endl);
                return true;
            }
        }
    }
    
    DEPENDENCE_DEBUG(std::cout << "DEBUG: No dependencies found - loop can be parallelized" << std::endl);
    return false;
}

// Unified memory dependence checker
bool LoopDependenceAnalysisPass::checkMemoryDependence(BasicInstruction* inst1, BasicInstruction* inst2, 
                                                      const std::map<int, Instruction>& inst_registry, 
                                                      ScalarEvolution* SE, Loop* loop, CFG* cfg) {
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Checking memory dependence between instructions" << std::endl);
    
    auto ptr1 = inst1->GetOpcode() == BasicInstruction::LOAD ? 
                ((LoadInstruction*)inst1)->GetPointer() : 
                ((StoreInstruction*)inst1)->GetPointer();
    auto ptr2 = inst2->GetOpcode() == BasicInstruction::LOAD ? 
                ((LoadInstruction*)inst2)->GetPointer() : 
                ((StoreInstruction*)inst2)->GetPointer();
    
    // Check for alias
    auto alias_result = alias_analyser->QueryAlias(ptr1, ptr2, cfg);
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Alias analysis result: " << (int)alias_result << std::endl);
    
    if (alias_result == AliasStatus::NoAlias) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: No alias - no dependence" << std::endl);
        return false;
    }
    
    // Even if there's an alias, we need to analyze GEP instructions to determine
    // if there's actually a loop-carried dependence
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Alias detected, analyzing GEP instructions..." << std::endl);
    
    // Global variables always cause dependencies
    if (ptr1->GetOperandType() == BasicOperand::GLOBAL || ptr2->GetOperandType() == BasicOperand::GLOBAL) {
        return true;
    }
    
    // Try to analyze GEP instructions
    auto reg1 = ((RegOperand*)ptr1)->GetRegNo();
    auto reg2 = ((RegOperand*)ptr2)->GetRegNo();
    
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Analyzing GEP instructions for registers " << reg1 << " and " << reg2 << std::endl);
    
    auto it1 = inst_registry.find(reg1);
    auto it2 = inst_registry.find(reg2);
    
    if (it1 == inst_registry.end() || it2 == inst_registry.end()) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: GEP instructions not found in registry - assuming dependence" << std::endl);
        return true; // Conservative: assume dependence
    }
    
    auto found_inst1 = it1->second;
    auto found_inst2 = it2->second;
    
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Found instructions with opcodes " << found_inst1->GetOpcode() << " and " << found_inst2->GetOpcode() << std::endl);
    
    // Handle PHI instructions - trace back to find the actual GEP instructions
    if (found_inst1->GetOpcode() == BasicInstruction::PHI) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Found PHI instruction for reg " << reg1 << ", tracing back..." << std::endl);
        found_inst1 = tracePhiToGEP(found_inst1, inst_registry);
    }
    
    if (found_inst2->GetOpcode() == BasicInstruction::PHI) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Found PHI instruction for reg " << reg2 << ", tracing back..." << std::endl);
        found_inst2 = tracePhiToGEP(found_inst2, inst_registry);
    }
    
    if (found_inst1->GetOpcode() != BasicInstruction::GETELEMENTPTR || 
        found_inst2->GetOpcode() != BasicInstruction::GETELEMENTPTR) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Instructions are not GEP after tracing - assuming dependence" << std::endl);
        return true; // Conservative: assume dependence
    }
    
    return analyzeGEPDependence(found_inst1, found_inst2, SE, loop);
}

isLoopDep LoopDependenceAnalysisPass::analyzeGEPDependence(Instruction I1, Instruction I2, ScalarEvolution* SE, Loop* L) {
    assert(I1->GetOpcode() == BasicInstruction::GETELEMENTPTR);
    assert(I2->GetOpcode() == BasicInstruction::GETELEMENTPTR);

    auto GEP1 = (GetElementptrInstruction*)I1;
    auto GEP2 = (GetElementptrInstruction*)I2;

    DEPENDENCE_DEBUG(std::cout << "DEBUG: Analyzing GEP dependence" << std::endl);

    // Quick checks for independence
    if (GEP1->GetPtrVal() != GEP2->GetPtrVal()) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Different base pointers - no dependence" << std::endl);
        return false;
    }
    if (GEP1->GetDims().size() != GEP2->GetDims().size()) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Different dimension counts - assuming dependence" << std::endl);
        return true;
    }
    
    // Check dimension sizes
    auto dims1 = GEP1->GetDims();
    auto dims2 = GEP2->GetDims();
    for (size_t i = 0; i < dims1.size(); ++i) {
        if (dims1[i] != dims2[i]) {
            DEPENDENCE_DEBUG(std::cout << "DEBUG: Different dimension sizes - assuming dependence" << std::endl);
            return true;
        }
    }
    
    // Check indices
    auto idx1 = GEP1->GetIndexes();
    auto idx2 = GEP2->GetIndexes();
    if (idx1.size() != idx2.size()) {
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Different index counts - assuming dependence" << std::endl);
        return true;
    }
    
    // Analyze each index dimension
    for (size_t i = 0; i < idx1.size(); ++i) {
        if (idx1[i]->GetOperandType() != BasicOperand::REG || 
            idx2[i]->GetOperandType() != BasicOperand::REG) {
            continue;
        }
        
        auto reg1 = ((RegOperand*)idx1[i])->GetRegNo();
        auto reg2 = ((RegOperand*)idx2[i])->GetRegNo();
        
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Analyzing index " << i << " (regs " << reg1 << " vs " << reg2 << ")" << std::endl);
        
        // Independence conditions
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Checking independence conditions" << std::endl);
        if (SE->isLoopInvariant(idx1[i], L) && SE->isLoopInvariant(idx2[i], L)) {
            DEPENDENCE_DEBUG(std::cout << "DEBUG: Both indices are loop invariant - no dependence" << std::endl);
            return false;
        }
        if (reg1 == reg2) {
            DEPENDENCE_DEBUG(std::cout << "DEBUG: Same register - no dependence" << std::endl);
            return false;
        }
        
        // Check if both indices are the same loop variable (induction variable)
        // This is a more sophisticated check for loop-carried dependencies
        if (SE->isLoopInvariant(idx1[i], L) == false && SE->isLoopInvariant(idx2[i], L) == false) {
            // Both are loop variables, check if they represent the same induction pattern
            // For simple cases like a[i] = a[i] + 1, both indices should be the same loop variable
            // We can check if they have the same SCEV pattern
            auto scev1 = SE->getSCEV(idx1[i], L);
            auto scev2 = SE->getSCEV(idx2[i], L);
            
            DEPENDENCE_DEBUG(std::cout << "DEBUG: Comparing SCEVs for indices:" << std::endl);
            DEPENDENCE_DEBUG(std::cout << "  Index1 (reg " << reg1 << "): " << (scev1 ? "SCEV found" : "No SCEV") << std::endl);
            DEPENDENCE_DEBUG(std::cout << "  Index2 (reg " << reg2 << "): " << (scev2 ? "SCEV found" : "No SCEV") << std::endl);
            
            if (scev1 && scev2 && SE->SCEVStructurallyEqual(scev1, scev2)) {
                DEPENDENCE_DEBUG(std::cout << "DEBUG: SCEVs are equal - no dependence" << std::endl);
                return false; // Same induction variable pattern, no dependence
            } else {
                DEPENDENCE_DEBUG(std::cout << "DEBUG: SCEVs are different - assuming dependence" << std::endl);
            }
        }
        
        DEPENDENCE_DEBUG(std::cout << "DEBUG: Conservative assumption - dependence" << std::endl);
        return true; // Conservative: assume dependence
    }
    
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Default conservative behavior - dependence" << std::endl);
    return true; // Default conservative behavior
}

// Helper function to trace PHI instructions back to GEP instructions
Instruction LoopDependenceAnalysisPass::tracePhiToGEP(Instruction phi_inst, const std::map<int, Instruction>& inst_registry) {
    if (phi_inst->GetOpcode() != BasicInstruction::PHI) {
        return phi_inst; // Not a PHI instruction
    }
    
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Tracing PHI instruction..." << std::endl);
    
    // Get PHI instruction operands
    auto phi_list = ((PhiInstruction*)phi_inst)->GetPhiList();
    for (auto [label, operand] : phi_list) {
        if (operand->GetOperandType() == BasicOperand::REG) {
            auto reg_no = ((RegOperand*)operand)->GetRegNo();
            auto it = inst_registry.find(reg_no);
            if (it != inst_registry.end()) {
                auto def_inst = it->second;
                DEPENDENCE_DEBUG(std::cout << "DEBUG: Found definition for reg " << reg_no << " with opcode " << def_inst->GetOpcode() << std::endl);
                
                if (def_inst->GetOpcode() == BasicInstruction::GETELEMENTPTR) {
                    DEPENDENCE_DEBUG(std::cout << "DEBUG: Found GEP instruction for reg " << reg_no << std::endl);
                    return def_inst; // Found a GEP instruction
                } else if (def_inst->GetOpcode() == BasicInstruction::PHI) {
                    // Recursively trace this PHI instruction
                    DEPENDENCE_DEBUG(std::cout << "DEBUG: Recursively tracing PHI for reg " << reg_no << std::endl);
                    auto traced_inst = tracePhiToGEP(def_inst, inst_registry);
                    if (traced_inst->GetOpcode() == BasicInstruction::GETELEMENTPTR) {
                        return traced_inst;
                    }
                }
            }
        }
    }
    
    DEPENDENCE_DEBUG(std::cout << "DEBUG: Could not trace PHI to GEP instruction" << std::endl);
    return phi_inst; // Return original if no GEP found
}

isLoopDep LoopDependenceAnalysisPass::getLoopDependenceResult(Loop* loop) const {
    auto it = loop_dependence_map.find(loop);
    return (it != loop_dependence_map.end()) ? it->second : true;
}

void LoopDependenceAnalysisPass::displayResults() const {
    if (loop_dependence_map.empty()) {
        std::cout << "Loop Dependence: No loops found" << std::endl;
        return;
    }
    
    std::vector<int> parallelizable_headers;
    std::vector<int> dependent_headers;
    
    for (const auto& [loop, has_dependence] : loop_dependence_map) {
        if (loop && loop->getHeader()) {
            int header_id = loop->getHeader()->block_id;
            if (has_dependence) {
                dependent_headers.push_back(header_id);
            } else {
                parallelizable_headers.push_back(header_id);
            }
        }
    }
    
    std::cout << "Loop Dependence: " << parallelizable_headers.size() << "/" << loop_dependence_map.size() 
              << " loops can be parallelized" << std::endl;
    
    if (!parallelizable_headers.empty()) {
        std::cout << "  Parallelizable headers: ";
        for (size_t i = 0; i < parallelizable_headers.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << parallelizable_headers[i];
        }
        std::cout << std::endl;
    }
    
    if (!dependent_headers.empty()) {
        std::cout << "  Dependent headers: ";
        for (size_t i = 0; i < dependent_headers.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << dependent_headers[i];
        }
        std::cout << std::endl;
    }
}
