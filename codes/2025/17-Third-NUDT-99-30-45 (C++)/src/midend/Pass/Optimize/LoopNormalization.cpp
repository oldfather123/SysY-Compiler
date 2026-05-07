#include "LoopNormalization.h"
#include "Dom.h"
#include "Loop.h"
#include "SysYIROptUtils.h"
#include <iostream>
#include <algorithm>
#include <sstream>

// 使用全局调试开关
extern int DEBUG;

namespace sysy {

// 定义 Pass 的唯一 ID
void *LoopNormalizationPass::ID = (void *)&LoopNormalizationPass::ID;

bool LoopNormalizationPass::runOnFunction(Function *F, AnalysisManager &AM) {
  if (F->getBasicBlocks().empty()) {
    return false; // 空函数
  }

  if (DEBUG)
    std::cout << "Running LoopNormalizationPass on function: " << F->getName() << std::endl;

  // 获取并缓存所有需要的分析结果
  loopAnalysis = AM.getAnalysisResult<LoopAnalysisResult, LoopAnalysisPass>(F);
  if (!loopAnalysis || !loopAnalysis->hasLoops()) {
    if (DEBUG)
      std::cout << "No loops found in function " << F->getName() << ", skipping normalization" << std::endl;
    return false; // 没有循环需要规范化
  }

  domTree = AM.getAnalysisResult<DominatorTree, DominatorTreeAnalysisPass>(F);
  
  if (!domTree) {
    std::cerr << "Error: DominatorTree not available for function " << F->getName() << std::endl;
    return false;
  }

  // 重置统计信息
  stats = NormalizationStats();
  
  bool modified = false;
  const auto& allLoops = loopAnalysis->getAllLoops();
  stats.totalLoops = allLoops.size();

  if (DEBUG) {
    std::cout << "Found " << stats.totalLoops << " loops to analyze for normalization" << std::endl;
  }

  // 按循环深度从外到内处理，确保外层循环先规范化
  std::vector<Loop*> sortedLoops;
  for (const auto& loop_ptr : allLoops) {
    sortedLoops.push_back(loop_ptr.get());
  }
  
  std::sort(sortedLoops.begin(), sortedLoops.end(), [](Loop* a, Loop* b) {
    return a->getLoopDepth() < b->getLoopDepth(); // 按深度升序排列
  });

  // 逐个规范化循环
  for (Loop* loop : sortedLoops) {
    if (needsPreheader(loop)) {
      stats.loopsNeedingPreheader++;
      
      if (DEBUG) {
        std::cout << "  Loop " << loop->getName() << " needs preheader (depth=" 
                  << loop->getLoopDepth() << ")" << std::endl;
      }
      
      if (normalizeLoop(loop)) {
        modified = true;
        stats.loopsNormalized++;
        
        // 验证规范化结果
        if (!validateNormalization(loop)) {
          std::cerr << "Warning: Loop normalization validation failed for loop " 
                    << loop->getName() << std::endl;
        }
      }
    } else {
      if (DEBUG) {
        auto* preheader = getExistingPreheader(loop);
        if (preheader) {
          std::cout << "  Loop " << loop->getName() << " already has preheader: " 
                    << preheader->getName() << std::endl;
        }
      }
    }
  }

  if (DEBUG && modified) {
    printStats(F);
  }

  return modified;
}

bool LoopNormalizationPass::normalizeLoop(Loop* loop) {
  if (DEBUG)
    std::cout << "    Normalizing loop: " << loop->getName() << std::endl;

  // 创建前置块
  BasicBlock* preheader = createPreheaderForLoop(loop);
  if (!preheader) {
    if (DEBUG)
      std::cout << "    Failed to create preheader for loop " << loop->getName() << std::endl;
    return false;
  }

  stats.preheadersCreated++;
  
  if (DEBUG) {
    std::cout << "    Successfully created preheader " << preheader->getName() 
              << " for loop " << loop->getName() << std::endl;
  }

  return true;
}

BasicBlock* LoopNormalizationPass::createPreheaderForLoop(Loop* loop) {
  BasicBlock* header = loop->getHeader();
  if (!header) {
    if (DEBUG)
      std::cerr << "    Error: Loop has no header block" << std::endl;
    return nullptr;
  }

  // 获取循环外的前驱块
  std::vector<BasicBlock*> externalPreds = getExternalPredecessors(loop);
  if (externalPreds.empty()) {
    if (DEBUG)
      std::cout << "    Loop " << loop->getName() << " has no external predecessors" << std::endl;
    return nullptr;
  }

  if (DEBUG) {
    std::cout << "    Found " << externalPreds.size() << " external predecessors for loop " 
              << loop->getName() << std::endl;
    for (auto* pred : externalPreds) {
      std::cout << "      External pred: " << pred->getName() << std::endl;
    }
  }

  // 生成前置块名称
  std::string preheaderName = generatePreheaderName(loop);
  
  // 创建新的前置块
  Function* parentFunction = header->getParent();
  BasicBlock* preheader = parentFunction->addBasicBlock(preheaderName, header);
  
  if (!preheader) {
    if (DEBUG)
      std::cerr << "    Error: Failed to create basic block " << preheaderName << std::endl;
    return nullptr;
  }

  // 在前置块中创建跳转指令到循环头部
  builder->setPosition(preheader, preheader->end());
  UncondBrInst* br = builder->createUncondBrInst(header);

  // 更新preheader的CFG关系
  preheader->addSuccessor(header);
  header->addPredecessor(preheader);

  if(DEBUG) {
    std::cout << "    Created preheader " << preheader->getName() 
              << " with unconditional branch to " << header->getName() << std::endl;
  }
  // 重定向外部前驱到新的前置块
  redirectExternalPredecessors(loop, preheader, header, externalPreds);

  // 更新PHI节点
  updatePhiNodesForPreheader(header, preheader, externalPreds);

  // 更新支配树关系
  updateDominatorRelations(preheader, loop);

  // 重要：更新循环对象的前置块信息
  // 这样后续的优化遍可以通过 loop->getPreHeader() 获取到新创建的前置块
  loop->setPreHeader(preheader);

  if (DEBUG) {
    std::cout << "    Updated loop object: preheader set to " << preheader->getName() << std::endl;
  }

  return preheader;
}

bool LoopNormalizationPass::needsPreheader(Loop* loop) {
  // 检查是否已有合适的前置块
  if (getExistingPreheader(loop) != nullptr) {
    return false;
  }

  // 检查是否有外部前驱（如果没有外部前驱，不需要前置块）
  std::vector<BasicBlock*> externalPreds = getExternalPredecessors(loop);
  if (externalPreds.empty()) {
    return false;
  }

  // 基于结构性需求判断：
  // 1. 如果有多个外部前驱，必须创建前置块来合并它们
  // 2. 如果单个外部前驱不适合作为前置块，需要创建新的前置块
  return (externalPreds.size() > 1) || !isSuitableAsPreheader(externalPreds[0], loop);
}

BasicBlock* LoopNormalizationPass::getExistingPreheader(Loop* loop) {
  BasicBlock* header = loop->getHeader();
  if (!header) return nullptr;

  std::vector<BasicBlock*> externalPreds = getExternalPredecessors(loop);
  
  // 如果只有一个外部前驱，且适合作为前置块，则返回它
  if (externalPreds.size() == 1 && isSuitableAsPreheader(externalPreds[0], loop)) {
    return externalPreds[0];
  }

  return nullptr;
}

void LoopNormalizationPass::updateDominatorRelations(BasicBlock* newBlock, Loop* loop) {
  // 由于在getAnalysisUsage中声明了DominatorTree会失效，
  // PassManager会在本遍运行后自动将支配树结果标记为失效，
  // 后续需要支配树的Pass会触发重新计算，所以这里无需手动更新
  
  if (DEBUG) {
    BasicBlock* header = loop->getHeader();
    std::cout << "    DominatorTree marked for invalidation - new preheader " 
              << newBlock->getName() << " will dominate " << header->getName() 
              << " after recomputation by PassManager" << std::endl;
  }
}

void LoopNormalizationPass::redirectExternalPredecessors(Loop* loop, BasicBlock* preheader, BasicBlock* header, 
                                                         const std::vector<BasicBlock*>& externalPreds) {
  // std::vector<BasicBlock*> externalPreds = getExternalPredecessors(loop);
  
  if (DEBUG) {
    std::cout << "    Redirecting " << externalPreds.size() << " external predecessors" << std::endl;
  }

  for (BasicBlock* pred : externalPreds) {
    // 获取前驱块的终止指令
    auto termIt = pred->terminator();
    if (termIt == pred->end()) continue;
    
    Instruction* terminator = termIt->get();
    if (!terminator) continue;

    // 更新跳转目标
    if (auto* br = dynamic_cast<UncondBrInst*>(terminator)) {
      // 无条件跳转
      if (br->getBlock() == header) {
        if(DEBUG){
          std::cout << "      Updating unconditional branch from " << br->getBlock()->getName()
                    << " to " << preheader->getName() << std::endl;
        }
        // 需要更新操作数
        br->setOperand(0, preheader);
        // 更新CFG关系
        header->removePredecessor(pred);
        preheader->addPredecessor(pred);
        pred->removeSuccessor(header);
        pred->addSuccessor(preheader);
        
      }
    } else if (auto* condBr = dynamic_cast<CondBrInst*>(terminator)) {
      // 条件跳转
      bool updated = false;
      if (condBr->getThenBlock() == header) {
        condBr->setOperand(1, preheader);  // 第1个操作数是then分支
        updated = true;
      }
      if (condBr->getElseBlock() == header) {
        condBr->setOperand(2, preheader);  // 第2个操作数是else分支
        updated = true;
      }
      if (updated) {
        // 更新CFG关系
        header->removePredecessor(pred);
        preheader->addPredecessor(pred);
        pred->removeSuccessor(header);
        pred->addSuccessor(preheader);
        
        if (DEBUG) {
          std::cout << "      Updated conditional branch from " << pred->getName() 
                    << " to " << preheader->getName() << std::endl;
        }
      }
    }
  }
}

std::string LoopNormalizationPass::generatePreheaderName(Loop* loop) {
  std::ostringstream oss;
  oss << loop->getName() << "_preheader";
  return oss.str();
}

bool LoopNormalizationPass::validateNormalization(Loop* loop) {
  BasicBlock* header = loop->getHeader();
  if (!header) return false;

  // 检查循环是否现在有唯一的外部前驱
  std::vector<BasicBlock*> externalPreds = getExternalPredecessors(loop);
  if (externalPreds.size() != 1) {
    if (DEBUG)
      std::cout << "    Validation failed: Loop " << loop->getName() 
                << " has " << externalPreds.size() << " external predecessors (expected 1)" << std::endl;
    return false;
  }

  // 检查外部前驱是否适合作为前置块
  BasicBlock* preheader = externalPreds[0];
  if (!isSuitableAsPreheader(preheader, loop)) {
    if (DEBUG)
      std::cout << "    Validation failed: External predecessor " << preheader->getName() 
                << " is not suitable as preheader" << std::endl;
    return false;
  }

  // 额外验证：检查CFG连接性
  if (!preheader->hasSuccessor(header)) {
    if (DEBUG)
      std::cout << "    Validation failed: Preheader " << preheader->getName() 
                << " is not connected to header " << header->getName() << std::endl;
    return false;
  }

  if (!header->hasPredecessor(preheader)) {
    if (DEBUG)
      std::cout << "    Validation failed: Header " << header->getName() 
                << " does not have preheader " << preheader->getName() << " as predecessor" << std::endl;
    return false;
  }

  if (DEBUG)
    std::cout << "    Validation passed for loop " << loop->getName() << std::endl;
  
  return true;
}

std::vector<BasicBlock*> LoopNormalizationPass::getExternalPredecessors(Loop* loop) {
  std::vector<BasicBlock*> externalPreds;
  BasicBlock* header = loop->getHeader();
  if (!header) return externalPreds;

  for (BasicBlock* pred : header->getPredecessors()) {
    if (!loop->contains(pred)) {
      externalPreds.push_back(pred);
    }
  }

  return externalPreds;
}

bool LoopNormalizationPass::isSuitableAsPreheader(BasicBlock* block, Loop* loop) {
  if (!block) return false;

  // 检查该块是否只有一个后继，且后继是循环头部
  auto successors = block->getSuccessors();
  if (successors.size() != 1) {
    return false;
  }

  if (successors[0] != loop->getHeader()) {
    return false;
  }

  // 检查该块是否不包含复杂的控制流
  // 理想的前置块应该只包含简单的跳转指令
  size_t instCount = 0;
  for (const auto& inst : block->getInstructions()) {
    instCount++;
    // 如果指令过多，可能不适合作为前置块
    if (instCount > 10) { // 阈值可调整
      return false;
    }
  }

  return true;
}

void LoopNormalizationPass::updatePhiNodesForPreheader(BasicBlock* header, BasicBlock* preheader,
                                                      const std::vector<BasicBlock*>& oldPreds) {
  if (DEBUG) {
    std::cout << "    Updating PHI nodes in header " << header->getName() 
              << " for new preheader " << preheader->getName() << std::endl;
  }

  std::vector<PhiInst*> phisToRemove; // 需要删除的PHI节点
  
  for (auto& inst : header->getInstructions()) {
    if (auto* phi = dynamic_cast<PhiInst*>(inst.get())) {
      if (DEBUG) {
        std::cout << "      Processing PHI node: " << phi->getName() << std::endl;
      }

      // 收集来自外部前驱的值 - 需要保持原始的映射关系
      std::map<BasicBlock*, Value*> externalValues;
      for (BasicBlock* oldPred : oldPreds) {
        Value* value = phi->getValfromBlk(oldPred);
        if (value) {
          externalValues[oldPred] = value;
        }
      }

      // 处理PHI节点的更新
      if (externalValues.size() > 1) {
        // 多个外部前驱：在前置块中创建新的PHI节点
        builder->setPosition(preheader, preheader->getInstructions().begin());
        
        std::vector<Value*> values;
        std::vector<BasicBlock*> blocks;
        for (auto& [block, value] : externalValues) {
          values.push_back(value);
          blocks.push_back(block);
        }
        
        PhiInst* newPhi = builder->createPhiInst(phi->getType(), values, blocks);
        
        // 移除所有外部前驱的条目
        for (BasicBlock* oldPred : oldPreds) {
          phi->removeIncomingBlock(oldPred);
        }
        
        // 添加来自新前置块的条目
        phi->addIncoming(newPhi, preheader);
        
      } else if (externalValues.size() == 1) {
        // 单个外部前驱：直接重新映射
        Value* value = externalValues.begin()->second;
        
        // 移除旧的外部前驱条目
        for (BasicBlock* oldPred : oldPreds) {
          phi->removeIncomingBlock(oldPred);
        }
        
        // 添加来自新前置块的条目
        phi->addIncoming(value, preheader);
        
        // 检查PHI节点是否只剩下一个条目（只来自前置块）
        if (phi->getNumIncomingValues() == 1) {
          if (DEBUG) {
            std::cout << "      PHI node " << phi->getName() 
                      << " now has only one incoming value, scheduling for removal" << std::endl;
          }
          // 用单一值替换所有使用
          Value* singleValue = phi->getIncomingValue(0u);
          phi->replaceAllUsesWith(singleValue);
          phisToRemove.push_back(phi);
        }
      } else {
        // 没有外部值的PHI节点：检查是否需要更新
        // 这种PHI节点只有循环内的边，通常不需要修改
        // 但我们仍然需要检查是否只有一个条目
        if (phi->getNumIncomingValues() == 1) {
          if (DEBUG) {
            std::cout << "      PHI node " << phi->getName() 
                      << " has only one incoming value (no external), scheduling for removal" << std::endl;
          }
          // 用单一值替换所有使用
          Value* singleValue = phi->getIncomingValue(0u);
          phi->replaceAllUsesWith(singleValue);
          phisToRemove.push_back(phi);
        }
      }
      
      if (DEBUG && std::find(phisToRemove.begin(), phisToRemove.end(), phi) == phisToRemove.end()) {
        std::cout << "      Updated PHI node with " << externalValues.size() 
                  << " external values, total incoming: " << phi->getNumIncomingValues() << std::endl;
      }
    }
  }
  
  // 删除标记为移除的PHI节点
  for (PhiInst* phi : phisToRemove) {
    if (DEBUG) {
      std::cout << "      Removing redundant PHI node: " << phi->getName() << std::endl;
    }
    SysYIROptUtils::usedelete(phi);
  }
  
  // 更新统计信息
  stats.redundantPhisRemoved += phisToRemove.size();
  
  if (DEBUG && !phisToRemove.empty()) {
    std::cout << "    Removed " << phisToRemove.size() << " redundant PHI nodes" << std::endl;
  }
}

void LoopNormalizationPass::printStats(Function* F) {
  std::cout << "\n--- Loop Normalization Statistics for Function: " << F->getName() << " ---" << std::endl;
  std::cout << "Total loops analyzed: " << stats.totalLoops << std::endl;
  std::cout << "Loops needing preheader: " << stats.loopsNeedingPreheader << std::endl;
  std::cout << "Preheaders created: " << stats.preheadersCreated << std::endl;
  std::cout << "Loops successfully normalized: " << stats.loopsNormalized << std::endl;
  std::cout << "Redundant PHI nodes removed: " << stats.redundantPhisRemoved << std::endl;
  
  if (stats.totalLoops > 0) {
    double normalizationRate = (double)stats.loopsNormalized / stats.totalLoops * 100.0;
    std::cout << "Normalization rate: " << normalizationRate << "%" << std::endl;
  }
  
  std::cout << "---------------------------------------------------------------" << std::endl;
}

void LoopNormalizationPass::getAnalysisUsage(std::set<void *> &analysisDependencies, 
                                            std::set<void *> &analysisInvalidations) const {
  // LoopNormalization依赖的分析
  analysisDependencies.insert(&LoopAnalysisPass::ID);              // 循环结构分析
  analysisDependencies.insert(&DominatorTreeAnalysisPass::ID);     // 支配树分析

  // LoopNormalization会修改CFG结构，因此会使以下分析失效
  analysisInvalidations.insert(&DominatorTreeAnalysisPass::ID);    // 支配树需要重新计算
  
  // 注意：我们不让循环结构分析失效，原因如下：
  // 1. 循环规范化只添加前置块，不改变循环的核心结构（头部、体、回边）
  // 2. 我们会手动更新Loop对象的前置块信息（通过loop->setPreHeader()）
  // 3. 让循环分析失效并重新计算的成本较高且不必要
  // 4. 后续优化遍可以正确获取到更新后的前置块信息
  // 
  // 如果未来有更复杂的循环结构修改，可能需要考虑让循环分析失效：
  // analysisInvalidations.insert(&LoopAnalysisPass::ID);
}

} // namespace sysy
