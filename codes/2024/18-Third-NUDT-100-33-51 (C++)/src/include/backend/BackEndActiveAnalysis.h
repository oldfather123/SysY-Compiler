#pragma once

#include <map>
#include <set>
#include <vector>
#include "../backend/Riscv.h"

/**
 * @file BackEndActiveAnalysis.h
 *
 * 定义了后端的活跃变量分析的头文件
 */

namespace riscv {

class ActiveVarAnalysis {
 private:
  std::map<BasicBlock *, std::vector<std::set<SymRegister *>>>
      activeTable;  ///< 活跃信息表，存储每个基本块内的的活跃变量信息

 public:
  ActiveVarAnalysis() = default;

 public:
  void analyze(Module *pModule);                ///< 分析模块
  void analyzeOneFucntion(Function *function);  ///< 分析单个函数
  auto getActiveTable() const -> const std::map<BasicBlock *, std::vector<std::set<SymRegister *>>> & {
    return activeTable;
  }                                      ///< 获取活跃变量信息表
  void clear() { activeTable.clear(); }  ///< 清除内部变量
};
}  // namespace riscv
