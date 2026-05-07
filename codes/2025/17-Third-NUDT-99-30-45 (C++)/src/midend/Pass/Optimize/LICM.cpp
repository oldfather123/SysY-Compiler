#include "LICM.h"
#include "IR.h"

extern int DEBUG;

namespace sysy {

void *LICM::ID = (void *)&LICM::ID;

bool LICMContext::run() { return hoistInstructions(); }

bool LICMContext::hoistInstructions() {
  bool changed = false;
  BasicBlock *preheader = loop->getPreHeader();
  if (!preheader || !chars)
    return false;

  // 1. 先收集所有可外提指令
  std::unordered_set<Instruction *> workSet(chars->invariantInsts.begin(), chars->invariantInsts.end());

  if (DEBUG) {
    std::cout << "LICM: Found " << workSet.size() << " candidate invariant instructions to hoist:" << std::endl;
    for (auto *inst : workSet) {
      std::cout << "  - " << inst->getName() << " (kind: " << static_cast<int>(inst->getKind()) 
                << ", in BB: " << inst->getParent()->getName() << ")" << std::endl;
    }
  }

  // 2. 计算每个指令被依赖的次数（入度）
  std::unordered_map<Instruction *, int> indegree;
  std::unordered_map<Instruction *, std::vector<Instruction *>> dependencies; // 记录依赖关系
  std::unordered_map<Instruction *, std::vector<Instruction *>> dependents;   // 记录被依赖关系
  
  for (auto *inst : workSet) {
    indegree[inst] = 0;
    dependencies[inst] = {};
    dependents[inst] = {};
  }
  
  if (DEBUG) {
    std::cout << "LICM: Analyzing dependencies between invariant instructions..." << std::endl;
  }
  
  for (auto *inst : workSet) {
    for (size_t i = 0; i < inst->getNumOperands(); ++i) {
      if (auto *dep = dynamic_cast<Instruction *>(inst->getOperand(i))) {
        if (workSet.count(dep)) {
          indegree[inst]++;
          dependencies[inst].push_back(dep);
          dependents[dep].push_back(inst);
          
          if (DEBUG) {
            std::cout << "  Dependency: " << inst->getName() << " depends on " << dep->getName() << std::endl;
          }
        }
      }
    }
  }

  if (DEBUG) {
    std::cout << "LICM: Initial indegree analysis:" << std::endl;
    for (auto &[inst, deg] : indegree) {
      std::cout << "  " << inst->getName() << ": indegree=" << deg;
      if (deg > 0) {
        std::cout << ", depends on: ";
        for (auto *dep : dependencies[inst]) {
          std::cout << dep->getName() << " ";
        }
      }
      std::cout << std::endl;
    }
  }

  // 3. Kahn拓扑排序
  std::vector<Instruction *> sorted;
  std::queue<Instruction *> q;
  
  if (DEBUG) {
    std::cout << "LICM: Starting topological sort..." << std::endl;
  }
  
  for (auto &[inst, deg] : indegree) {
    if (deg == 0) {
      q.push(inst);
      if (DEBUG) {
        std::cout << "  Initial zero-indegree instruction: " << inst->getName() << std::endl;
      }
    }
  }
  
  int sortStep = 0;
  while (!q.empty()) {
    auto *inst = q.front();
    q.pop();
    sorted.push_back(inst);
    
    if (DEBUG) {
      std::cout << "  Step " << (++sortStep) << ": Processing " << inst->getName() << std::endl;
    }
    
    if (DEBUG) {
      std::cout << "    Reducing indegree of dependents of " << inst->getName() << std::endl;
    }
    
    // 正确的拓扑排序：当处理一个指令时，应该减少其所有使用者（dependents）的入度
    for (auto *dependent : dependents[inst]) {
      indegree[dependent]--;
      if (DEBUG) {
        std::cout << "      Reducing indegree of " << dependent->getName() << " to " << indegree[dependent] << std::endl;
      }
      if (indegree[dependent] == 0) {
        q.push(dependent);
        if (DEBUG) {
          std::cout << "      Adding " << dependent->getName() << " to queue (indegree=0)" << std::endl;
        }
      }
    }
  }

  // 检查是否全部排序，若未全部排序，打印错误信息
  // 这可能是因为存在循环依赖或其他问题导致无法完成拓扑排序
  if (sorted.size() != workSet.size()) {
    if (DEBUG) {
      std::cout << "LICM: Topological sort failed! Sorted " << sorted.size() 
                << " instructions out of " << workSet.size() << " total." << std::endl;
      
      // 找出未被排序的指令（形成循环依赖的指令）
      std::unordered_set<Instruction *> remaining;
      for (auto *inst : workSet) {
        bool found = false;
        for (auto *sortedInst : sorted) {
          if (inst == sortedInst) {
            found = true;
            break;
          }
        }
        if (!found) {
          remaining.insert(inst);
        }
      }
      
      std::cout << "LICM: Instructions involved in dependency cycle:" << std::endl;
      for (auto *inst : remaining) {
        std::cout << "  - " << inst->getName() << " (indegree=" << indegree[inst] << ")" << std::endl;
        std::cout << "    Dependencies within cycle: ";
        for (auto *dep : dependencies[inst]) {
          if (remaining.count(dep)) {
            std::cout << dep->getName() << " ";
          }
        }
        std::cout << std::endl;
        std::cout << "    Dependents within cycle: ";
        for (auto *dependent : dependents[inst]) {
          if (remaining.count(dependent)) {
            std::cout << dependent->getName() << " ";
          }
        }
        std::cout << std::endl;
      }
      
      // 尝试找出一个具体的循环路径
      std::cout << "LICM: Attempting to trace a dependency cycle:" << std::endl;
      if (!remaining.empty()) {
        auto *start = *remaining.begin();
        std::unordered_set<Instruction *> visited;
        std::vector<Instruction *> path;
        
        std::function<bool(Instruction *)> findCycle = [&](Instruction *current) -> bool {
          if (visited.count(current)) {
            // 找到环
            auto it = std::find(path.begin(), path.end(), current);
            if (it != path.end()) {
              std::cout << "  Cycle found: ";
              for (auto cycleIt = it; cycleIt != path.end(); ++cycleIt) {
                std::cout << (*cycleIt)->getName() << " -> ";
              }
              std::cout << current->getName() << std::endl;
              return true;
            }
            return false;
          }
          
          visited.insert(current);
          path.push_back(current);
          
          for (auto *dep : dependencies[current]) {
            if (remaining.count(dep)) {
              if (findCycle(dep)) {
                return true;
              }
            }
          }
          
          path.pop_back();
          return false;
        };
        
        findCycle(start);
      }
    }
    return false;
  }

  // 4. 按拓扑序外提
  if (DEBUG) {
    std::cout << "LICM: Successfully completed topological sort. Hoisting instructions in order:" << std::endl;
  }
  
  for (auto *inst : sorted) {
    if (!inst)
      continue;
    BasicBlock *parent = inst->getParent();
    if (parent && loop->contains(parent)) {
      if (DEBUG) {
        std::cout << "  Hoisting " << inst->getName() << " from " << parent->getName() 
                  << " to preheader " << preheader->getName() << std::endl;
      }
      auto sourcePos = parent->findInstIterator(inst);
      auto targetPos = preheader->terminator();
      parent->moveInst(sourcePos, targetPos, preheader);
      changed = true;
    }
  }
  
  if (DEBUG && changed) {
    std::cout << "LICM: Successfully hoisted " << sorted.size() << " invariant instructions" << std::endl;
  }
  
  return changed;
}
// ---- LICM Pass Implementation ----

bool LICM::runOnFunction(Function *F, AnalysisManager &AM) {
  auto *loopAnalysis = AM.getAnalysisResult<LoopAnalysisResult, LoopAnalysisPass>(F);
  auto *loopCharsResult = AM.getAnalysisResult<LoopCharacteristicsResult, LoopCharacteristicsPass>(F);
  if (!loopAnalysis || !loopCharsResult)
    return false;

  bool changed = false;
  // 对每个函数内的所有循环做处理
  for (const auto &loop_ptr : loopAnalysis->getAllLoops()) {
    Loop *loop = loop_ptr.get();
    if (DEBUG) {
      std::cout << "LICM: Processing loop in function " << F->getName() << ": " << loop->getName() << std::endl;
    }
    const LoopCharacteristics *chars = loopCharsResult->getCharacteristics(loop);
    if (!chars || !loop->getPreHeader())
      continue; // 没有分析结果或没有前置块则跳过
    LICMContext ctx(F, loop, builder, chars);
    changed |= ctx.run();
  }
  return changed;
}

void LICM::getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {

  analysisDependencies.insert(&LoopAnalysisPass::ID);
  analysisDependencies.insert(&LoopCharacteristicsPass::ID);

  analysisInvalidations.insert(&LoopCharacteristicsPass::ID);
  analysisInvalidations.insert(&LivenessAnalysisPass::ID);
}

} // namespace sysy