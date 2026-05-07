#pragma once
#include "../backend/DAGBuilder.h"
#include "../backend/Mid2EndDAG.h"
/**
 * @file DAGPrinter.h
 * @brief 中端到后端DAG打印器的头文件
 *
 */

namespace mid2EndDAG {
// 中端到后端的DAG的打印器
/**
 * @brief 中端DAG打印器
 *
 */
class Mid2EndDAGPrinter {
 protected:
  Module *pModule;                                                           ///< 中端DAG模块
  std::map<Function *, std::map<uint64_t, std::string>> localScalarNameMap;  ///< 标号-标量名字对应
  std::map<Function *, std::map<Mid2EndPointerNode *, std::string>>
      localArrayNameMap;  ///< DAG pointer节点-数组名字对应

  uint64_t tmpIndex = 0;
  std::map<Mid2EndDAGNode *, uint64_t> tmpIndexMap;      ///< DAG节点-临时变量编号对应
  std::map<Mid2EndDAGNode *, std::string> specialNames;  ///< DAG节点-非临时变量名字对应

  // DAG层次的临时区
  std::map<uint64_t, Mid2EndDAGNode *> varInputMap;  ///< 标号-输入节点对应
  std::map<std::string, std::string> movMap;         ///< 转移表
  std::map<Mid2EndDAGNode *, bool> isVisited;        ///< 节点访问记录表
  std::map<Mid2EndDAGNode *, bool> isAddToVisited;   ///< 节点添加记录表
  std::list<Mid2EndDAGNode *> toVisit;               ///< 待访问队列

 public:
  explicit Mid2EndDAGPrinter(Mid2EndDAGBuilder &builder) {
    pModule = builder.getModule();
    localArrayNameMap = builder.getLocalArrayNameMap();
    localScalarNameMap = builder.getLocalScalarNameMap();
  }

 protected:
  void interpreteConstant(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);       ///< 打印Constant节点
  void interpreteScalarNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);     ///< 打印Scalar节点
  void interpreteIntBinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 打印IntBinary节点
  void interpreteUint64BinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 打印Uint64Binary节点
  void interpreteFloatBinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 打印FloatBinary节点
  void interpreteIntUnaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);     ///< 打印IntUnary节点
  void interpreteFloatUnaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 打印FloatUnary节点
  void interpreteCallNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);        ///< 打印Call节点
  void interpreteCondBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);      ///< 打印CondBr节点
  void interpreteBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);          ///< 打印Br节点
  void interpreteReturnNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);      ///< 打印Return节点
  void interpreteLoadNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);        ///< 打印Load节点
  void interpreteStoreNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);       ///< 打印Store节点
  void interpreteLaNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);          ///< 打印La节点
  void interpreteGetSubArrayNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 打印GetSubArray节点
  void interpreteMemsetNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);       ///< 打印Memset节点
  void interpreteRiscvCondBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 打印RiscvCondBr节点
  void interpreteRiscvF3OpNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);    ///< 打印RiscvF3Op节点
  void interpreteRiscvLoadNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);    ///< 打印RiscvLoad节点
  void interpreteRiscvStoreNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 打印RiscvStore节点

 protected:
  auto getOperandName(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) -> std::string;  ///< 获取操作数名字
  auto getOutputNames(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG)
      -> std::list<std::string>;  ///< 获取输出变量名字列表

 public:
  void printNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 打印节点
  void printDAG(Mid2EndDAG *DAG);                                             ///< 打印DAG
  void printFunction(Function *function);                                     ///< 打印函数
  void printModule();                                                         ///< 打印Module
};
}  // namespace mid2EndDAG