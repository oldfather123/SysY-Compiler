#include "../include/backend/BackEndActiveAnalysis.h"
#include <algorithm>
#include "../include/backend/Riscv.h"

/**
 * @file BackEndActiveAnalysis.cpp
 *
 * 定义了后端的活跃变量分析的源文件
 */

namespace riscv {
/**
 * @brief 分析单个函数
 *
 * @param [in] function 所要分析的函数
 * @return 无返回值
 */
void ActiveVarAnalysis::analyzeOneFucntion(Function *function) {
  for (const auto &block : function->getBlocks()) {
    activeTable.emplace(block.get(), std::vector<std::set<SymRegister *>>{});
    for (unsigned i = 0; i < block->getNumInstructions() + 1; i++) {
      activeTable.at(block.get()).emplace_back();
    }
  }

  bool changed;
  do {
    changed = false;
    for (const auto &block : function->getBlocks()) {
      std::set<SymRegister *> activeSet;
      for (const auto &suc : block->getSuccessors()) {
        auto sucActiveSet = activeTable.at(suc).front();
        activeSet.merge(sucActiveSet);
      }
      activeTable.at(block.get()).back() = activeSet;
      auto instIter = block->getInstructions().rbegin();
      for (unsigned i = 0; i < block->getNumInstructions(); i++) {
        auto defineSymbol = dynamic_cast<SymRegister *>(instIter->get()->getDestination());
        if (defineSymbol != nullptr) {
          activeSet.erase(defineSymbol);
        }
        for (const auto &reg : instIter->get()->getSources()) {
          auto useSymbol = dynamic_cast<SymRegister *>(reg);
          if (useSymbol != nullptr) {
            activeSet.insert(useSymbol);
          }
        }
        auto &oldActiveSet = *(activeTable.at(block.get()).rbegin() + i + 1);
        if (!std::equal(activeSet.begin(), activeSet.end(), oldActiveSet.begin(), oldActiveSet.end())) {
          changed = true;
          oldActiveSet = activeSet;
        }
        instIter++;
      }
    }
  } while (changed);
}
/**
 * @brief 分析模块
 *
 * @param [in] pModule 所要分析的模块
 * @return 无返回值
 */
void ActiveVarAnalysis::analyze(Module *pModule) {
  for (const auto &functionItem : pModule->getFunctions()) {
    if (!functionItem.second->isExternalFucntion()) {
      analyzeOneFucntion(functionItem.second.get());
    }
  }
}
}  // namespace riscv
