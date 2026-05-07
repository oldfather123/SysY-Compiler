#pragma once

#include <map>
#include <set>
#include <vector>
#include "../frontend/IR.h"
#include "../midend/DataFlowAnalysis.h"

/**
 * @file ActiveVarAnalysis.h
 *
 * @brief 活跃变量分析的头文件
 */

namespace sysy {
/**
 * @ingroup dataAnalysis
 *
 * @{
 */
/**
 * @brief 活跃变量分析
 * 可使用定值-使用链进行优化
 *
 */
class ActiveVarAnalysis : public DataFlowAnalysis {
 private:
  std::map<BasicBlock *, std::vector<std::set<User *>>> activeTable;  ///< 活跃信息表，存储每个基本块内的的活跃变量信息

 public:
  ActiveVarAnalysis() = default;
  ~ActiveVarAnalysis() override = default;

 public:
  static auto getUsedSet(Instruction *inst) -> std::set<User *>;  ///< 获取指令使用的变量集合
  static auto getDefine(Instruction *inst) -> User *;             ///< 获取定义的变量

 public:
  void init(Module *pModule) override;
  auto analyze(Module *pModule, BasicBlock *block) -> bool override;                             ///< 分析动作
  auto getActiveTable() const -> const std::map<BasicBlock *, std::vector<std::set<User *>>> &;  ///< 返回活跃信息表
  void clear() override;  ///< 清空活跃信息表
};

/**
 * @}
 */
}  // namespace sysy
