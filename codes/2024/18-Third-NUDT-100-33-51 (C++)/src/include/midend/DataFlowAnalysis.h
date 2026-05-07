#pragma once

#include <vector>
#include "../frontend/IR.h"

/**
 * @file DataFlowAnalysis.h
 *
 * @brief 定义数据流基础类型的头文件
 */

namespace sysy {
/**
 * @defgroup dataAnalysis DataAnalysis
 *
 * @{
 */

/**
 * @brief 数据流分析基类
 *
 * 所有数据流分析的基类
 */
class DataFlowAnalysis {
 public:
  virtual ~DataFlowAnalysis() = default;

 public:
  virtual void init(Module *pModule) {}                                              ///< 分析器初始化
  virtual auto analyze(Module *pModule, BasicBlock *block) -> bool { return true; }  ///< 分析动作，若完成则返回true;
  virtual void clear() {}                                                            ///< 清空
};

/**
 * @brief 数据流管理类
 *
 * 该类提供正序与倒序两种指令遍历模式，通过传入处理函数，即可获得相应的
 * 数据流分析。
 */
class DataFlowManager {
 private:
  std::vector<DataFlowAnalysis *> forwardAnalysisList;   ///< 前向数据流分析器列表
  std::vector<DataFlowAnalysis *> backwardAnalysisList;  ///< 后向数据流分析器列表

 public:
  DataFlowManager() = default;
  DataFlowManager(DataFlowAnalysis *forwardDataFlow, DataFlowAnalysis *backwardDataFlow) {
    forwardAnalysisList.emplace_back(forwardDataFlow);
    backwardAnalysisList.emplace_back(backwardDataFlow);
  }
  DataFlowManager(const std::vector<DataFlowAnalysis *> &forwardDataFlowList,
                  const std::vector<DataFlowAnalysis *> &backwardDataFlowList) {
    forwardAnalysisList = forwardDataFlowList;
    backwardAnalysisList = backwardDataFlowList;
  }

 public:
  void addDataFlow(DataFlowAnalysis *forwardDataFlow, DataFlowAnalysis *backwardDataFlow) {
    forwardAnalysisList.emplace_back(forwardDataFlow);
    backwardAnalysisList.emplace_back(backwardDataFlow);
  }  ///< 添加单个前向与后向数据流分析器
  void addDataFlow(const std::vector<DataFlowAnalysis *> &forwardDataFlowList,
                   const std::vector<DataFlowAnalysis *> &backwardDataFlowList) {
    for (const auto &dataFlow : forwardDataFlowList) {
      forwardAnalysisList.emplace_back(dataFlow);
    }
    for (const auto &dataFlow : backwardDataFlowList) {
      backwardAnalysisList.emplace_back(dataFlow);
    }
  }  ///< 添加多个前向与后向数据流分析器
  void addForwardDataFlow(DataFlowAnalysis *forwardDataFlow) {
    forwardAnalysisList.emplace_back(forwardDataFlow);
  }  ///< 添加单个前向数据流分析器
  void addBackwardDataFlow(DataFlowAnalysis *backwardDataFlow) {
    backwardAnalysisList.emplace_back(backwardDataFlow);
  }  ///< 添加单个后向数据流分析器
  void setDataFlowList(const std::vector<DataFlowAnalysis *> &forwardDataFlowList,
                       const std::vector<DataFlowAnalysis *> &backwardDataFlowList) {
    forwardAnalysisList = forwardDataFlowList;
    backwardAnalysisList = backwardDataFlowList;
  }  ///< 设置前向与后向数据流分析器列表
  void setForwardDataFlowList(const std::vector<DataFlowAnalysis *> &forwardDataFlowList) {
    forwardAnalysisList = forwardDataFlowList;
  }  ///< 设置前向数据流分析器列表
  void setBackwardDataFlowList(const std::vector<DataFlowAnalysis *> &backwardDataFlowList) {
    backwardAnalysisList = backwardDataFlowList;
  }  ///< 设置后向数据流分析器列表
  void clearDataFlowList() {
    forwardAnalysisList.clear();
    backwardAnalysisList.clear();
  }  ///< 清空前向与后向数据流分析器列表
  auto getForwardDataFlowList() const -> const std::vector<DataFlowAnalysis *> & {
    return forwardAnalysisList;
  }  ///< 返回前向数据流分析器列表
  auto getBackwardDataFlowList() const -> const std::vector<DataFlowAnalysis *> & {
    return backwardAnalysisList;
  }  ///< 返回后向数据流分析器列表
  auto copyForwardDataFlowList() -> std::vector<DataFlowAnalysis *> {
    return forwardAnalysisList;
  }  ///< 返回前向数据流分析器列表的拷贝
  auto copyBackwardDataFlowList() -> std::vector<DataFlowAnalysis *> {
    return backwardAnalysisList;
  }  ///< 返回后向数据流分析器列表的拷贝

 public:
  void forwardAnalyze(Module *pModule);   ///< 前向分析
  void backwardAnalyze(Module *pModule);  ///< 后向分析
};
/**
 * @}
 */
}  // namespace sysy
