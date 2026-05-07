#pragma once

#include <vector>
#include "../backend/Mid2EndDAG.h"
#include "../frontend/IR.h"

/**
 * @file DAGBuilder.h
 * @brief 中端到后端构建器的头文件
 *
 */

namespace mid2EndDAG {
/**
 * @brief 中端DAG构建器
 *
 */
class Mid2EndDAGBuilder {
 protected:
  // 结果区
  std::unique_ptr<Module> pModule;                                           ///< DAG模块
  std::map<Function *, std::map<uint64_t, std::string>> localScalarNameMap;  ///< 标号-标量名字对应
  std::map<Function *, std::map<Mid2EndPointerNode *, std::string>>
      localArrayNameMap;  ///< Pointer节点-局部数组名字对应

  // 模块级别临时区
  std::map<sysy::BasicBlock *, Mid2EndDAG *> blockDAGMap;  ///< block-DAG对应

  // 块级别临时区
  std::map<sysy::Value *, Mid2EndDAGNode *> valueDAGNodeMap;             ///< value-DAG节点对应
  std::map<sysy::Value *, Mid2EndDAGNode *> valueScalarOutputMap;        ///< value-标量输出节点对应
  std::map<Mid2EndPointerNode *, Mid2EndDAGNode *> pointerLoadStoreMap;  ///< LoadStore节点-目标Pointer节点对应
  Mid2EndCallNode *lastCallNode = nullptr;                               ///< 最近一次调用对应的Call节点
  // 函数级别临时区
  std::map<sysy::Value *, Mid2EndPointerNode *> valueLocalArrayMap;                  ///< value-Pointer节点对应
  std::map<Mid2EndScalarNode *, sysy::Value *> inputValueMap;                        ///< 输入节点-Value对应
  std::map<Mid2EndDAG *, std::map<sysy::Value *, Mid2EndDAGNode *>> valueOutputMap;  ///< value-输出节点对应

 public:
  Mid2EndDAGBuilder() = default;

 protected:
  auto getOrEmplaceDAGNode(sysy::Value *value, Mid2EndDAG *DAG) -> Mid2EndDAGNode *;  ///< 获取或创建DAG节点
  auto getNodeKind(sysy::Instruction::Kind kind) -> Mid2EndDAGNode::NodeType;  ///< 获取指令对应的节点类型

 public:
  void initBasicElements(sysy::Module *midModule);  ///< 初始化基本元素
  void buildDAG(sysy::Module *midModule);           ///< 构建DAG
  void calInputNode();                              ///< 计算输入节点
  void calOutputNode(Mid2EndDAG *DAG);              /// 计算输出节点
  void initParams(sysy::Function *function);        ///< 初始化函数参数
  void bindInputOutput(Function *function);         ///< 绑定输出输出节点
  void clear() {
    pModule.reset();
    localArrayNameMap.clear(), localScalarNameMap.clear();
    blockDAGMap.clear();
  }  ///< 清除内部变量

 public:
  auto getModule() const -> Module * { return pModule.get(); }  ///< 获取DAG模块
  auto getLocalScalarNameMap() const -> const std::map<Function *, std::map<uint64_t, std::string>> & {
    return localScalarNameMap;
  }  ///< 获取标号-标量名字对应
  auto getLocalArrayNameMap() const -> const std::map<Function *, std::map<Mid2EndPointerNode *, std::string>> & {
    return localArrayNameMap;
  }  ///< 获取Pointer节点-局部数组名字对应
};

/**
 * @brief 中端DAG化简器
 *
 */
class Mid2EndDAGSimplifier {
 protected:
  Module *pModule{};  ///< DAG模块

 public:
  Mid2EndDAGSimplifier() = default;

 protected:
  auto getOrEmplaceConstant(int value, Mid2EndDAG *DAG) -> Mid2EndIntNode *;        ///< 获取int常量节点
  auto getOrEmplaceConstant(float value, Mid2EndDAG *DAG) -> Mid2EndFloatNode *;    ///< 获取float常量节点
  auto getOrEmplaceUint64(uint64_t value, Mid2EndDAG *DAG) -> Mid2EndUint64Node *;  ///< 获取uint64常量节点
  auto getOperandType(Mid2EndDAGNode *node)
      -> unsigned int;  ///< 获取节点类型，其中0表示int, 1表示float, 2表示void, 3表示uint64

  auto constantCombNode(Mid2EndDAGNode *node, Mid2EndDAG *DAG, Function *function,
                        std::set<Mid2EndDAGNode *> &deletedNodes, std::set<Mid2EndDAGNode *> &addedNodes)
      -> bool;  ///< 常量节点合并

  auto getOrEmplaceBinaryNode(Mid2EndDAGNode::NodeType nodeType, Mid2EndDAGNode *lhsNode, Mid2EndDAGNode *rhsNode,
                              Mid2EndDAG *DAG) -> Mid2EndBinaryOpNode *;  ///< 获取或创建二元节点
  auto getOrEmplaceUnaryNode(Mid2EndDAGNode::NodeType nodeType, Mid2EndDAGNode *hsNode, Mid2EndDAG *DAG)
      -> Mid2EndUnaryOpNode *;  ///< 获取或创建一元节点
  template <typename ContainerT>
  auto calculateIndices(std::list<int> dims, const ContainerT &indices, Mid2EndDAG *DAG)
      -> Mid2EndDAGNode *;  ///< 计算索引

 protected:
  auto deleteOneNode(Mid2EndDAGNode *node) -> bool;                     ///< 删除单个节点
  void deletDeadDAGs();                                                 ///< 删除死DAG
  void deleteUselessNodes();                                            ///< 删除无用节点
  void constantComb();                                                  ///< 常量合并
  void combCondBr();                                                    ///< 跳转合并
  void combFloatArithmeticOp();                                         ///< 浮点算术运算合并
  void expandIndicesNodes();                                            ///< 索引计算展开
  auto exprEval(Mid2EndDAGNode *root1, Mid2EndDAGNode *root2) -> bool;  ///< 比较两个不同基本块中的表达式是否等价
  void generateMinMax();                                                ///< 生成Min/Max节点

 public:
  void init(Module *inputModule) { pModule = inputModule; }  ///< 初始化
  void run() {
    constantComb();
    // generateMinMax();
    //  combFloatArithmeticOp();
    expandIndicesNodes();
    combCondBr();
    constantComb();
  }  ///< 化简DAG
};

/**
 * @brief 用于浮点运算指令合并的状态机
 *
 */
class FInstReduceDFA {
 public:
  /// 自动机状态
  enum State {
    Mul,
    NegMul,
    FMADD,
    FMSUB,
    FNMADD,
    FNMSUB,
    END,
  };

 protected:
  State state = END;                         ///< 状态
  State opType = END;                        ///< 合并后的节点类型
  std::vector<Mid2EndDAGNode *> operands;    ///< 合并后的操作数列表
  std::list<Mid2EndDAGNode *> reducedNodes;  ///< 被合并的节点列表
  Mid2EndDAGNode *prevNode = nullptr;        ///< 前一个节点
  Mid2EndDAGNode *nextNode = nullptr;        ///< 下一个节点

 public:
  FInstReduceDFA() = default;

 public:
  void init(Mid2EndBinaryOpNode *fMulNode) {
    assert(fMulNode->getNodeType() == Mid2EndDAGNode::FMUL);
    operands.emplace_back(fMulNode->getLhs());
    operands.emplace_back(fMulNode->getRhs());
    prevNode = fMulNode;
    auto &outputNodes = fMulNode->getParent()->getOutputNodes();
    if (prevNode->getNumUsers() == 1 && outputNodes.find(prevNode) == outputNodes.end()) {
      nextNode = prevNode->getUser(0);
      reducedNodes.emplace_back(prevNode);
      state = Mul;
    } else {
      state = END;
    }
  }                                                                                 ///< 初始化
  void act(Mid2EndDAG *DAG);                                                        ///< 自动机运行
  auto isCanReduced() -> bool { return operands.size() == 3; }                      ///< 是否成功合并
  auto getOperands() -> const std::vector<Mid2EndDAGNode *> & { return operands; }  ///< 获取合并后的操作数列表
  auto getReducedNodes() -> const std::list<Mid2EndDAGNode *> & { return reducedNodes; }  ///< 获取被合并的节点
  auto getOpType() -> State { return opType; }  ///< 获取合并后的节点类型
  void clear() {
    operands.clear();
    reducedNodes.clear();
    prevNode = nullptr;
    nextNode = nullptr;
    state = END;
    opType = END;
  }  ///< 清空内部变量
};
}  // namespace mid2EndDAG