#include "Dom.h"
#include "Liveness.h"
#include "Loop.h"
#include "LoopCharacteristics.h"
#include "AliasAnalysis.h"
#include "CallGraphAnalysis.h"
#include "SideEffectAnalysis.h"
#include "SysYIRCFGOpt.h"
#include "SysYIRPrinter.h"
#include "DCE.h"
#include "Mem2Reg.h"
#include "Reg2Mem.h"
#include "GVN.h"
#include "SCCP.h"
#include "BuildCFG.h"
#include "LoopNormalization.h"
#include "LICM.h"
#include "LoopStrengthReduction.h"
#include "InductionVariableElimination.h"
#include "GlobalStrengthReduction.h"
#include "TailCallOpt.h"
#include "Pass.h"
#include <iostream>
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <vector>

extern int DEBUG; // 全局调试标志
namespace sysy {

// ======================================================================
// 封装优化流程的函数：包含Pass注册和迭代运行逻辑
// ======================================================================

void PassManager::runOptimizationPipeline(Module* moduleIR, IRBuilder* builderIR, int optLevel) {
    if (DEBUG) std::cout << "--- Starting Middle-End Optimizations (Level -O" << optLevel << ") ---\n";

    /*
      中端开发框架基本流程：
      1) 分析pass
        1. 实现分析pass并引入Pass.cpp
        2. 注册分析pass
      2) 优化pass
        1. 实现优化pass并引入Pass.cpp
        2. 注册优化pass
        3. 添加优化passid
    */
    // 注册分析遍
    registerAnalysisPass<DominatorTreeAnalysisPass>();
    registerAnalysisPass<LivenessAnalysisPass>();
    registerAnalysisPass<sysy::DominatorTreeAnalysisPass>();
    registerAnalysisPass<sysy::LivenessAnalysisPass>();
    registerAnalysisPass<SysYAliasAnalysisPass>();           // 别名分析 (优先级高)
    registerAnalysisPass<CallGraphAnalysisPass>();           // 调用图分析 (Module级别，独立分析)
    registerAnalysisPass<SysYSideEffectAnalysisPass>();      // 副作用分析 (依赖别名分析和调用图)
    registerAnalysisPass<LoopAnalysisPass>();
    registerAnalysisPass<LoopCharacteristicsPass>();        // 循环特征分析依赖别名分析

    // 注册优化遍
    registerOptimizationPass<BuildCFG>();
    registerOptimizationPass<GVN>();
    
    registerOptimizationPass<SysYDelInstAfterBrPass>();
    registerOptimizationPass<SysYDelNoPreBLockPass>();
    registerOptimizationPass<SysYBlockMergePass>();

    registerOptimizationPass<SysYDelEmptyBlockPass>(builderIR);
    registerOptimizationPass<SysYCondBr2BrPass>(builderIR);
    registerOptimizationPass<SysYAddReturnPass>(builderIR);

    registerOptimizationPass<DCE>();
    registerOptimizationPass<Mem2Reg>(builderIR);
    registerOptimizationPass<LoopNormalizationPass>(builderIR);
    registerOptimizationPass<LICM>(builderIR);
    registerOptimizationPass<LoopStrengthReduction>(builderIR);
    registerOptimizationPass<InductionVariableElimination>();

    registerOptimizationPass<GlobalStrengthReduction>(builderIR);
    registerOptimizationPass<Reg2Mem>(builderIR);
    registerOptimizationPass<TailCallOpt>(builderIR);

    registerOptimizationPass<SCCP>(builderIR);

    if (optLevel >= 1) {
      //经过设计安排优化遍的执行顺序以及执行逻辑
      if (DEBUG) std::cout << "Applying -O1 optimizations.\n";
      if (DEBUG) std::cout << "--- Running custom optimization sequence ---\n";

      if(DEBUG) {
        std::cout << "=== IR Before CFGOpt Optimizations ===\n";
        printPasses();
      }

      this->clearPasses();
      this->addPass(&BuildCFG::ID);
      this->run();

      this->clearPasses(); 
      this->addPass(&SysYDelInstAfterBrPass::ID);
      this->addPass(&SysYDelNoPreBLockPass::ID);
      this->addPass(&SysYBlockMergePass::ID);
      this->addPass(&SysYDelEmptyBlockPass::ID);
      this->addPass(&SysYCondBr2BrPass::ID);
      this->addPass(&SysYAddReturnPass::ID);
      this->run(); 

      this->clearPasses();
      this->addPass(&BuildCFG::ID);
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After CFGOpt Optimizations ===\n";
        printPasses();
      }

      this->clearPasses();
      this->addPass(&DCE::ID); 
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After DCE Optimizations ===\n";
        printPasses();
      }

      this->clearPasses();
      this->addPass(&Mem2Reg::ID);
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After Mem2Reg Optimizations ===\n";
        printPasses();
      }

      this->clearPasses();
      this->addPass(&GVN::ID);
      this->run();

      this->clearPasses();
      this->addPass(&TailCallOpt::ID);
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After TailCallOpt ===\n";
        SysYPrinter printer(moduleIR);
        printer.printIR();
      }

      if(DEBUG) {
        std::cout << "=== IR After GVN Optimizations ===\n";
        printPasses();
      }

      this->clearPasses();
      this->addPass(&SCCP::ID);
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After SCCP Optimizations ===\n";
        printPasses();
      }

      this->clearPasses();
      this->addPass(&LoopNormalizationPass::ID);
      this->addPass(&InductionVariableElimination::ID);
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After Loop Normalization, Induction Variable Elimination ===\n";
        printPasses();
      }
      

      this->clearPasses();
      this->addPass(&LICM::ID);
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After LICM ===\n";
        printPasses();
      }
      
      this->clearPasses();
      this->addPass(&LoopStrengthReduction::ID);
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After Loop Normalization, and Strength Reduction Optimizations ===\n";
        printPasses();
      }

      // 全局强度削弱优化，包括代数优化和魔数除法
      this->clearPasses();
      this->addPass(&GlobalStrengthReduction::ID);
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After Global Strength Reduction Optimizations ===\n";
        printPasses();
      }

      this->clearPasses();
      this->addPass(&Reg2Mem::ID);
      this->run();

      if(DEBUG) {
        std::cout << "=== IR After Reg2Mem Optimizations ===\n";
        printPasses();
      }
      this->clearPasses();
      this->addPass(&BuildCFG::ID);
      this->run();
      if (DEBUG) std::cout << "--- Custom optimization sequence finished ---\n";
    }

    // 2. 创建遍管理器
    // 3. 根据优化级别添加不同的优化遍
    // TODO : 根据 optLevel 添加不同的优化遍
    //        讨论 是不动点迭代进行优化遍还是手动客制化优化遍的顺序？


    if (DEBUG) {
      std::cout << "=== Final IR After Middle-End Optimizations (Level -O" << optLevel << ") ===\n";
      SysYPrinter printer(moduleIR);
      printer.printIR();
    }
    
}

void PassManager::clearPasses() {
  passes.clear();
}

void PassManager::addPass(void *passID) {

  PassRegistry &registry = PassRegistry::getPassRegistry();
  std::unique_ptr<Pass> P = registry.createPass(passID);
  if (!P) {
    // Error: Pass not found or failed to create
    return;
  }

  passes.push_back(std::move(P));
}

// 运行所有注册的遍
bool PassManager::run() {
  bool changed = false;
  for (const auto &p : passes) {
    bool passChanged = false; // 记录当前遍是否修改了 IR

    // 处理优化遍的分析依赖和失效
    if (p->getPassKind() == Pass::PassKind::Optimization) {
      OptimizationPass *optPass = static_cast<OptimizationPass *>(p.get());
      std::set<void *> analysisDependencies;
      std::set<void *> analysisInvalidations;
      optPass->getAnalysisUsage(analysisDependencies, analysisInvalidations);

      // PassManager 不显式运行分析依赖。
      // 而是优化遍在 runOnFunction 内部通过 AnalysisManager.getAnalysisResult 按需请求。
    }

    if (p->getGranularity() == Pass::Granularity::Module) {
      passChanged = p->runOnModule(pmodule, analysisManager);
    } else if (p->getGranularity() == Pass::Granularity::Function) {
      for (auto &funcPair : pmodule->getFunctions()) {
        Function *F = funcPair.second.get();
        passChanged = p->runOnFunction(F, analysisManager) || passChanged;

        if (passChanged && p->getPassKind() == Pass::PassKind::Optimization) {
          OptimizationPass *optPass = static_cast<OptimizationPass *>(p.get());
          std::set<void *> analysisDependencies;
          std::set<void *> analysisInvalidations;
          optPass->getAnalysisUsage(analysisDependencies, analysisInvalidations);
          for (void *invalidationID : analysisInvalidations) {
            analysisManager.invalidateAnalysis(invalidationID, F);
          }
        }
      }
    } else if (p->getGranularity() == Pass::Granularity::BasicBlock) {
      for (auto &funcPair : pmodule->getFunctions()) {
        Function *F = funcPair.second.get();
        for (auto &bbPtr : funcPair.second->getBasicBlocks()) {
          passChanged = p->runOnBasicBlock(bbPtr.get(), analysisManager) || passChanged;

          if (passChanged && p->getPassKind() == Pass::PassKind::Optimization) {
            OptimizationPass *optPass = static_cast<OptimizationPass *>(p.get());
            std::set<void *> analysisDependencies;
            std::set<void *> analysisInvalidations;
            optPass->getAnalysisUsage(analysisDependencies, analysisInvalidations);
            for (void *invalidationID : analysisInvalidations) {
              analysisManager.invalidateAnalysis(invalidationID, F);
            }
          }
        }
      }
    }
    changed = changed || passChanged;
  }
  return changed;

}

void PassManager::printPasses() const {
  std::cout << "Registered Passes:\n";
  for (const auto &p : passes) {
    std::cout << " - " << p->getName() << " (Granularity: " 
              << static_cast<int>(p->getGranularity()) 
              << ", Kind: " << static_cast<int>(p->getPassKind()) << ")\n";
  }
  std::cout << "Total Passes: " << passes.size() << "\n";
  if (pmodule) {
    SysYPrinter printer(pmodule);
    std::cout << "Module IR:\n";
    printer.printIR();
  }
}

template <typename AnalysisPassType> void registerAnalysisPass() {
  PassRegistry::getPassRegistry().registerPass(&AnalysisPassType::ID,
                                               []() { return std::make_unique<AnalysisPassType>(); });
}

template <typename OptimizationPassType, typename std::enable_if<
    std::is_constructible<OptimizationPassType, IRBuilder*>::value, int>::type>
void registerOptimizationPass(IRBuilder* builder) {
    PassRegistry::getPassRegistry().registerPass(&OptimizationPassType::ID,
                                                 [builder]() { return std::make_unique<OptimizationPassType>(builder); });
}

template <typename OptimizationPassType, typename std::enable_if<
    !std::is_constructible<OptimizationPassType, IRBuilder*>::value, int>::type>
void registerOptimizationPass() {
    PassRegistry::getPassRegistry().registerPass(&OptimizationPassType::ID,
                                                 []() { return std::make_unique<OptimizationPassType>(); });
}

} // namespace sysy