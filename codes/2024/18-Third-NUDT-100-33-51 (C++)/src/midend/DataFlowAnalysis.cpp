#include "../include/midend/DataFlowAnalysis.h"
#include <algorithm>
#include <map>
#include <utility>
#include "../include/frontend/IR.h"

/**
 * @file DataFlowAnalysis.cpp
 *
 * @brief 定义数据流基础类型的源文件
 */

namespace sysy {

/**
 * @brief 前向数据流分析
 *
 * @param [in] pModule 所要分析的模块
 * @return 无返回值
 */
void DataFlowManager::forwardAnalyze(Module *pModule) {
  std::map<DataFlowAnalysis *, bool> workAnalysis;
  for (auto &dataflow : forwardAnalysisList) {
    dataflow->init(pModule);
  }

  for (const auto &function : pModule->getFunctions()) {
    for (auto &dataflow : forwardAnalysisList) {
      workAnalysis.emplace(dataflow, false);
    }
    while (!workAnalysis.empty()) {
      for (const auto &block : function.second->getBasicBlocks()) {
        for (auto &elem : workAnalysis) {
          if (elem.first->analyze(pModule, block.get())) {
            elem.second = true;
          }
        }
      }
      std::map<DataFlowAnalysis *, bool> tmp;
      std::remove_copy_if(workAnalysis.begin(), workAnalysis.end(), std::inserter(tmp, tmp.end()),
                          [](const std::pair<DataFlowAnalysis *, bool> &elem) -> bool { return !elem.second; });
      workAnalysis.swap(tmp);

      for (auto &elem : workAnalysis) {
        elem.second = false;
      }
    }
  }
}
/**
 * @brief 后向数据流分析
 *
 * @param [in] pModule 所要分析的模块
 * @return 无返回值
 */
void DataFlowManager::backwardAnalyze(Module *pModule) {
  std::map<DataFlowAnalysis *, bool> workAnalysis;
  for (auto &dataflow : backwardAnalysisList) {
    dataflow->init(pModule);
  }

  for (const auto &function : pModule->getFunctions()) {
    for (auto &dataflow : backwardAnalysisList) {
      workAnalysis.emplace(dataflow, false);
    }
    while (!workAnalysis.empty()) {
      for (const auto &block : function.second->getBasicBlocks()) {
        for (auto &elem : workAnalysis) {
          if (elem.first->analyze(pModule, block.get())) {
            elem.second = true;
          }
        }
      }
      std::map<DataFlowAnalysis *, bool> tmp;
      std::remove_copy_if(workAnalysis.begin(), workAnalysis.end(), std::inserter(tmp, tmp.end()),
                          [](const std::pair<DataFlowAnalysis *, bool> &elem) -> bool { return !elem.second; });
      workAnalysis.swap(tmp);

      for (auto &elem : workAnalysis) {
        elem.second = false;
      }
    }
  }
}

}  // namespace sysy
