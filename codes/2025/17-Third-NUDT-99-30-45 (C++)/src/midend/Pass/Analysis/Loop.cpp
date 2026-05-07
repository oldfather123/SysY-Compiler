#include "Dom.h" // 确保包含 DominatorTreeAnalysisPass 的定义
#include "Loop.h" //
#include "AliasAnalysis.h" // 添加别名分析依赖
#include "SideEffectAnalysis.h" // 添加副作用分析依赖
#include <iostream>
#include <queue> // 用于 BFS 遍历设置循环层级

// 调试模式开关
#ifndef DEBUG
#define DEBUG 0
#endif

namespace sysy {

// 定义 Pass 的唯一 ID
void *LoopAnalysisPass::ID = (void *)&LoopAnalysisPass::ID;

// 定义 Loop 类的静态变量
int Loop::NextLoopID = 0;
// **实现 LoopAnalysisResult::print() 方法**


void LoopAnalysisResult::printBBSet(const std::string &prefix, const std::set<BasicBlock *> &s) const{
  if (!DEBUG) return;
  std::cout << prefix << "{";
  bool first = true;
  for (const auto &bb : s) {
    if (!first) std::cout << ", ";
    std::cout << bb->getName();
    first = false;
  }
  std::cout << "}";
}

// **辅助函数：打印 Loop 指针向量**
void LoopAnalysisResult::printLoopVector(const std::string &prefix, const std::vector<Loop *> &loops) const {
  if (!DEBUG) return;
  std::cout << prefix << "[";
  bool first = true;
  for (const auto &loop : loops) {
    if (!first) std::cout << ", ";
    std::cout << loop->getName(); // 假设 Loop::getName() 存在
    first = false;
  }
  std::cout << "]";
}

void LoopAnalysisResult::print() const {
  if (!DEBUG) return; // 只有在 DEBUG 模式下才打印

  std::cout << "\n--- Loop Analysis Results for Function: " << AssociatedFunction->getName() << " ---" << std::endl;

  if (AllLoops.empty()) {
    std::cout << "  No loops found." << std::endl;
    return;
  }

  std::cout << "Total Loops Found: " << AllLoops.size() << std::endl;

  // 1. 按层级分组循环
  std::map<int, std::vector<Loop*>> loopsByLevel;
  int maxLevel = 0;
  for (const auto& loop_ptr : AllLoops) {
      if (loop_ptr->getLoopLevel() != -1) { // 确保层级已计算
          loopsByLevel[loop_ptr->getLoopLevel()].push_back(loop_ptr.get());
          if (loop_ptr->getLoopLevel() > maxLevel) {
              maxLevel = loop_ptr->getLoopLevel();
          }
      }
  }

  // 2. 打印循环层次结构
  std::cout << "\n--- Loop Hierarchy ---" << std::endl;
  for (int level = 0; level <= maxLevel; ++level) {
      if (loopsByLevel.count(level)) {
          std::cout << "Level " << level << " Loops:" << std::endl;
          for (Loop* loop : loopsByLevel[level]) {
              std::string indent(level * 2, ' '); // 根据层级缩进
              std::cout << indent << "- Loop Header: " << loop->getName() << std::endl;
              std::cout << indent << "  Blocks: ";
              printBBSet("", loop->getBlocks());
              std::cout << std::endl;

              std::cout << indent << "  Exit Blocks: ";
              printBBSet("", loop->getExitBlocks());
              std::cout << std::endl;

              std::cout << indent << "  Pre-Header: " << (loop->getPreHeader() ? loop->getPreHeader()->getName() : "None") << std::endl;
              std::cout << indent << "  Parent Loop: " << (loop->getParentLoop() ? loop->getParentLoop()->getName() : "None (Outermost)") << std::endl;
              std::cout << indent << "  Nested Loops: ";
              printLoopVector("", loop->getNestedLoops());
              std::cout << std::endl;
          }
      }
  }

  // 3. 打印最外层/最内层循环摘要
  std::cout << "\n--- Loop Summary ---" << std::endl;
  std::cout << "Outermost Loops: ";
  printLoopVector("", getOutermostLoops());
  std::cout << std::endl;

  std::cout << "Innermost Loops: ";
  printLoopVector("", getInnermostLoops());
  std::cout << std::endl;

  std::cout << "-----------------------------------------------" << std::endl;
}

bool LoopAnalysisPass::runOnFunction(Function *F, AnalysisManager &AM) {
  if (F->getBasicBlocks().empty()) {
    CurrentResult = std::make_unique<LoopAnalysisResult>(F);
    return false; // 空函数，没有循环
  }

  if (DEBUG)
    std::cout << "Running LoopAnalysisPass on function: " << F->getName() << std::endl;

  // 获取支配树分析结果
  // 这是循环分析的关键依赖
  DominatorTree *DT = AM.getAnalysisResult<DominatorTree, DominatorTreeAnalysisPass>(F);
  if (!DT) {
    // 无法获取支配树，无法进行循环分析
    std::cerr << "Error: DominatorTreeAnalysisResult not available for function " << F->getName() << std::endl;
    CurrentResult = std::make_unique<LoopAnalysisResult>(F);
    return false;
  }

  // 获取别名分析结果 - 用于循环内存访问分析
  AliasAnalysisResult *aliasAnalysis = AM.getAnalysisResult<AliasAnalysisResult, SysYAliasAnalysisPass>(F);
  if (DEBUG && aliasAnalysis) {
    std::cout << "Loop Analysis: Using alias analysis results for enhanced memory pattern detection" << std::endl;
  }

  // 获取副作用分析结果 - 用于循环纯度分析
  SideEffectAnalysisResult *sideEffectAnalysis = AM.getAnalysisResult<SideEffectAnalysisResult, SysYSideEffectAnalysisPass>();
  if (DEBUG && sideEffectAnalysis) {
    std::cout << "Loop Analysis: Using side effect analysis results for loop purity detection" << std::endl;
  }

  CurrentResult = std::make_unique<LoopAnalysisResult>(F);
  bool changed = false; // 循环分析本身不修改IR，所以通常返回false

  // 步骤 1: 识别回边和对应的自然循环
  // 回边 (N -> D) 定义：D 支配 N
  std::vector<std::pair<BasicBlock *, BasicBlock *>> backEdges;
  for (auto &BB : F->getBasicBlocks()) {
    auto Block = BB.get();
    for (BasicBlock *Succ : Block->getSuccessors()) {
      if (DT->getDominators(Block) && DT->getDominators(Block)->count(Succ)) {
        // Succ 支配 Block，所以 (Block -> Succ) 是一条回边
        backEdges.push_back({Block, Succ});
        if (DEBUG)
          std::cout << "Found back edge: " << Block->getName() << " -> " << Succ->getName() << std::endl;
      }
    }
  }
  
  if (DEBUG)
    std::cout << "Total back edges found: " << backEdges.size() << std::endl;

  // 步骤 2: 为每条回边构建自然循环
  std::map<BasicBlock*, std::unique_ptr<Loop>> loopMap; // 按循环头分组
  
  for (auto &edge : backEdges) {
    BasicBlock *N = edge.first;  // 回边的尾部
    BasicBlock *D = edge.second; // 回边的头部 (循环头)

    // 检查是否已经为此循环头创建了循环
    if (loopMap.find(D) == loopMap.end()) {
      // 创建新的 Loop 对象
      loopMap[D] = std::make_unique<Loop>(D);
    }
    
    Loop* currentLoop = loopMap[D].get();

    // 收集此回边对应的循环体块：从 N 逆向遍历到 D
    std::set<BasicBlock *> loopBlocks; // 临时存储循环块
    std::queue<BasicBlock *> q;

    // 循环头总是循环体的一部分
    loopBlocks.insert(D);
    
    // 如果回边的尾部不是循环头本身，则将其加入队列进行遍历
    if (N != D) {
      q.push(N);
      loopBlocks.insert(N);
    }

    while (!q.empty()) {
      BasicBlock *current = q.front();
      q.pop();

      for (BasicBlock *pred : current->getPredecessors()) {
        // 如果前驱还没有被访问过，则将其加入循环体并继续遍历
        if (loopBlocks.find(pred) == loopBlocks.end()) {
          loopBlocks.insert(pred);
          q.push(pred);
        }
      }
    }

    // 将收集到的块添加到 Loop 对象中（合并所有回边的结果）
    for (BasicBlock *loopBB : loopBlocks) {
      currentLoop->addBlock(loopBB);
    }
  }

  // 处理每个合并后的循环
  for (auto &[header, currentLoop] : loopMap) {
    const auto &loopBlocks = currentLoop->getBlocks();

    // 步骤 3: 识别循环出口块 (Exit Blocks)
    for (BasicBlock *loopBB : loopBlocks) {
      for (BasicBlock *succ : loopBB->getSuccessors()) {
        if (loopBlocks.find(succ) == loopBlocks.end()) {
          // 如果后继不在循环体内，则 loopBB 是一个出口块
          currentLoop->addExitBlock(loopBB);
        }
      }
    }

    // 步骤 4: 识别循环前置块 (Pre-Header)
    BasicBlock *candidatePreHeader = nullptr;
    int externalPredecessorCount = 0;
    for (BasicBlock *predOfHeader : header->getPredecessors()) {
      // 使用 currentLoop->contains() 来检查前驱是否在循环体内
      if (!currentLoop->contains(predOfHeader)) {
        // 如果前驱不在循环体内，则是一个外部前驱
        externalPredecessorCount++;
        candidatePreHeader = predOfHeader;
      }
    }

    if (externalPredecessorCount == 1) {
      currentLoop->setPreHeader(candidatePreHeader);
    }
    CurrentResult->addLoop(std::move(currentLoop));
  }

  // 步骤 5: 处理嵌套循环 (确定父子关系和层级)
  const auto &allLoops = CurrentResult->getAllLoops();

  // 1. 首先，清除所有循环已设置的父子关系和嵌套子循环列表，确保重新计算
  for (const auto &loop_ptr : allLoops) {
      loop_ptr->setParentLoop(nullptr); // 清除父指针
      loop_ptr->clearNestedLoops();     // 清除子循环列表
      loop_ptr->setLoopLevel(-1);       // 重置循环层级
  }

  // 2. 遍历所有循环，为每个循环找到其直接父循环并建立关系
  for (const auto &innerLoop_ptr : allLoops) {
    Loop *innerLoop = innerLoop_ptr.get();
    Loop *immediateParent = nullptr; // 用于存储当前 innerLoop 的最近父循环

    for (const auto &outerLoop_ptr : allLoops) {
      Loop *outerLoop = outerLoop_ptr.get();

      // 一个循环不能是它自己的父循环
      if (outerLoop == innerLoop) {
        continue;
      }

      // 检查 outerLoop 是否包含 innerLoop 的所有条件：
      // Condition 1: outerLoop 的头支配 innerLoop 的头
      if (!(DT->getDominators(innerLoop->getHeader()) &&
            DT->getDominators(innerLoop->getHeader())->count(outerLoop->getHeader()))) {
        continue; // outerLoop 不支配 innerLoop 的头，因此不是一个外层循环
      }

      // Condition 2: innerLoop 的所有基本块都在 outerLoop 的基本块集合中
      bool allInnerBlocksInOuter = true;
      for (BasicBlock *innerBB : innerLoop->getBlocks()) {
        if (!outerLoop->contains(innerBB)) { //
          allInnerBlocksInOuter = false;
          break;
        }
      }
      if (!allInnerBlocksInOuter) {
        continue; // outerLoop 不包含 innerLoop 的所有块
      }

      // 到此为止，outerLoop 已经被确认为 innerLoop 的一个“候选父循环”（即它包含了 innerLoop）

      if (immediateParent == nullptr) {
        // 这是找到的第一个候选父循环
        immediateParent = outerLoop;
      } else {
        // 已经有了一个 immediateParent，需要判断哪个是更“紧密”的父循环
        // 更紧密的父循环是那个包含另一个候选父循环的。
        // 如果当前的 immediateParent 包含了 outerLoop 的头，那么 outerLoop 是更深的循环（更接近 innerLoop）
        if (immediateParent->contains(outerLoop->getHeader())) { //
          immediateParent = outerLoop; // outerLoop 是更紧密的父循环
        }
        // 否则（outerLoop 包含了 immediateParent 的头），说明 immediateParent 更紧密，保持不变
        // 或者它们互不包含（不应该发生，因为它们都包含了 innerLoop），也保持 immediateParent
      }
    }

    // 设置 innerLoop 的直接父循环，并添加到父循环的嵌套列表中
    if (immediateParent) {
      innerLoop->setParentLoop(immediateParent);
      immediateParent->addNestedLoop(innerLoop); 
    }
  }

  // 3. 计算循环层级 (Level)
  std::queue<Loop *> q_level;

  // 查找所有最外层循环（没有父循环的），设置其层级为0，并加入队列
  for (const auto &loop_ptr : allLoops) {
    if (loop_ptr->isOutermost()) {
      loop_ptr->setLoopLevel(0);
      q_level.push(loop_ptr.get());
    }
  }

  // 使用 BFS 遍历循环树，计算所有嵌套循环的层级
  while (!q_level.empty()) {
    Loop *current = q_level.front();
    q_level.pop();

    for (Loop *nestedLoop : current->getNestedLoops()) {
      nestedLoop->setLoopLevel(current->getLoopLevel() + 1);
      q_level.push(nestedLoop);
    }
  }

  if (DEBUG) {
    std::cout << "Loop Analysis completed for function: " << F->getName() << std::endl;
    std::cout << "Total loops found: " << CurrentResult->getLoopCount() << std::endl;
    std::cout << "Max loop depth: " << CurrentResult->getMaxLoopDepth() << std::endl;
    std::cout << "Innermost loops: " << CurrentResult->getInnermostLoops().size() << std::endl;
    std::cout << "Outermost loops: " << CurrentResult->getOutermostLoops().size() << std::endl;
    
    // 打印各深度的循环分布
    for (int depth = 1; depth <= CurrentResult->getMaxLoopDepth(); ++depth) {
      int count = CurrentResult->getLoopCountAtDepth(depth);
      if (count > 0) {
        std::cout << "Loops at depth " << depth << ": " << count << std::endl;
      }
    }
    
    // 输出缓存统计
    auto cacheStats = CurrentResult->getCacheStats();
    std::cout << "Cache statistics - Total cached queries: " << cacheStats.totalCachedQueries << std::endl;
  }

  return changed;
}

// ========== Loop 类的新增方法实现 ==========

bool Loop::mayHaveSideEffects(SideEffectAnalysisResult* sideEffectAnalysis) const {
  if (!sideEffectAnalysis) return true; // 保守假设
  
  for (BasicBlock* bb : LoopBlocks) {
    for (auto& inst : bb->getInstructions()) {
      if (sideEffectAnalysis->hasSideEffect(inst.get())) {
        return true;
      }
    }
  }
  return false;
}

bool Loop::accessesGlobalMemory(AliasAnalysisResult* aliasAnalysis) const {
  if (!aliasAnalysis) return true; // 保守假设
  
  for (BasicBlock* bb : LoopBlocks) {
    for (auto& inst : bb->getInstructions()) {
      if (auto* loadInst = dynamic_cast<LoadInst*>(inst.get())) {
        if (!aliasAnalysis->isLocalArray(loadInst->getPointer())) {
          return true;
        }
      } else if (auto* storeInst = dynamic_cast<StoreInst*>(inst.get())) {
        if (!aliasAnalysis->isLocalArray(storeInst->getPointer())) {
          return true;
        }
      }
    }
  }
  return false;
}

bool Loop::hasMemoryAliasConflicts(AliasAnalysisResult* aliasAnalysis) const {
  if (!aliasAnalysis) return true; // 保守假设
  
  std::vector<Value*> memoryAccesses;
  
  // 收集所有内存访问
  for (BasicBlock* bb : LoopBlocks) {
    for (auto& inst : bb->getInstructions()) {
      if (auto* loadInst = dynamic_cast<LoadInst*>(inst.get())) {
        memoryAccesses.push_back(loadInst->getPointer());
      } else if (auto* storeInst = dynamic_cast<StoreInst*>(inst.get())) {
        memoryAccesses.push_back(storeInst->getPointer());
      }
    }
  }
  
  // 检查两两之间是否有别名
  for (size_t i = 0; i < memoryAccesses.size(); ++i) {
    for (size_t j = i + 1; j < memoryAccesses.size(); ++j) {
      auto aliasType = aliasAnalysis->queryAlias(memoryAccesses[i], memoryAccesses[j]);
      if (aliasType == AliasType::SELF_ALIAS || aliasType == AliasType::POSSIBLE_ALIAS) {
        return true;
      }
    }
  }
  
  return false;
}

} // namespace sysy