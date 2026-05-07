#pragma once

/**
 * @file Mid2EndDAG.h
 * @brief 中端到后端DAG的头文件
 *
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "../utils/ConstantCounter.h"

namespace mid2EndDAG {
// 中端的DAG表示(默认将非全局标量看成一个scalar，其不涉及load、store、memset、la操作，与数组和全局变量区分开来)
// 考虑中端已经实现的优化，可以减少部分优化步骤(如常量合并、公用子表达式删除)
class Mid2EndDAG;
class Function;
/**
 * @brief 中端DAG节点
 *
 */
class Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 public:
  /// 节点类型
  enum NodeType : uint64_t {
    // 中端节点
    // 数据类型
    INT = 0x1UL << 0,
    FLOAT = 0x1UL << 1,
    SCALAR = 0x1UL << 2,
    POINTER = 0x1UL << 3,
    // 指令类型
    ADD = 0x1UL << 4,
    SUB = 0x1UL << 5,
    MUL = 0x1UL << 6,
    DIV = 0x1UL << 7,
    REM = 0x1UL << 8,
    ICMPEQ = 0x1UL << 9,
    ICMPNE = 0x1UL << 10,
    ICMPLT = 0x1UL << 11,
    ICMPGT = 0x1UL << 12,
    ICMPLE = 0x1UL << 13,
    ICMPGE = 0x1UL << 14,
    FADD = 0x1UL << 15,
    FSUB = 0x1UL << 16,
    FMUL = 0x1UL << 17,
    FDIV = 0x1UL << 18,
    FCMPEQ = 0x1UL << 19,
    FCMPNE = 0x1UL << 20,
    FCMPLT = 0x1UL << 21,
    FCMPGT = 0x1UL << 22,
    FCMPLE = 0x1UL << 23,
    FCMPGE = 0x1UL << 24,
    // AND = 0x1UL << 25,
    // OR = 0x1UL << 26,
    // 包含整型与浮点的min/max
    MIN = 0x1UL << 25,
    MAX = 0x1UL << 26,
    NEG = 0x1UL << 27,
    NOT = 0x1UL << 28,
    FNEG = 0x1UL << 29,
    FNOT = 0x1UL << 30,
    FTOI = 0x1UL << 31,
    ITOF = 0x1UL << 32,
    CALL = 0x1UL << 33,
    CONDBR = 0x1UL << 34,
    BR = 0x1UL << 35,
    RETURN = 0x1UL << 36,
    LOAD = 0x1UL << 37,
    STORE = 0x1UL << 38,
    LA = 0x1UL << 39,
    MEMSET = 0x1UL << 40,
    // Riscv节点
    BEQ = 0x1UL << 41,
    BNE = 0x1UL << 42,
    BLT = 0x1UL << 43,
    BGE = 0x1UL << 44,
    BLTU = 0x1UL << 45,
    BGEU = 0x1UL << 46,
    FMADD = 0x1UL << 47,
    FMSUB = 0x1UL << 48,
    FNMSUB = 0x1UL << 49,
    FNMADD = 0x1UL << 50,
    FEQ = 0x1UL << 51,
    FLT = 0x1UL << 52,
    FLE = 0x1UL << 53,
    FNZERO = 0x1UL << 54,
    // 地址计算使用
    UINT64 = 0x1UL << 55,
    ADD64 = 0x1UL << 56,
    MUL64 = 0x1UL << 57,
    RVLOAD = 0x1UL << 58,
    RVSTORE = 0x1UL << 59,
    SEXTW = 0x1UL << 60,
    GETSUBARRAY = 0x1UL << 61,
    // 按位转换
    BIT_ITOF = 0x1UL << 62,
    BIT_FTOI = 0x1UL << 63,
  };

 protected:
  Mid2EndDAG *parent;                 ///< 父DAg
  NodeType nodeType;                  ///< 节点类型
  std::list<Mid2EndDAGNode *> users;  ///< 使用者列表
  std::list<Mid2EndDAGNode *> useds;  ///< 被使用者列表

 protected:
  explicit Mid2EndDAGNode(NodeType nodeType, Mid2EndDAG *parent) : parent(parent), nodeType(nodeType) {}

 public:
  virtual ~Mid2EndDAGNode() {
    for (const auto &user : users) {
      user->removeUsed(this);
    }
    for (const auto &used : useds) {
      used->removeUser(this);
    }
  }

 public:
  void addUser(Mid2EndDAGNode *node) { users.emplace_back(node); }  ///< 添加使用者
  void addUsed(Mid2EndDAGNode *node) { useds.emplace_back(node); }  ///< 添加被使用者
  template <typename ContainerT>
  void addUsers(const ContainerT &nodes) {
    for (const auto &node : nodes) {
      users.emplace_back(node);
    }
  }  ///< 添加多个使用者
  template <typename ContainerT>
  void addUseds(const ContainerT &nodes) {
    for (const auto &node : nodes) {
      useds.emplace_back(node);
    }
  }  ///< 添加多个被使用者

  void removeUser(Mid2EndDAGNode *targetNode) {
    auto predicate = [&](const Mid2EndDAGNode *node) -> bool { return node == targetNode; };
    users.remove_if(predicate);
  }  ///< 移除使用者

  void removeUsed(Mid2EndDAGNode *targetNode) {
    auto predicate = [&](const Mid2EndDAGNode *node) -> bool { return node == targetNode; };
    useds.remove_if(predicate);
  }  ///< 移除被使用者

  void replaceUsed(Mid2EndDAGNode *oldNode, Mid2EndDAGNode *newNode) {
    auto predicate = [&](const Mid2EndDAGNode *node) { return node == oldNode; };
    std::replace_if(useds.begin(), useds.end(), predicate, newNode);
  }  ///< 替换被使用者

 public:
  auto getUser(unsigned index) const -> Mid2EndDAGNode * { return *std::next(users.begin(), index); }  ///< 获取使用者
  auto getUsed(unsigned index) const -> Mid2EndDAGNode * { return *std::next(useds.begin(), index); }  ///< 获取被使用者
  auto getUsers() const -> const std::list<Mid2EndDAGNode *> & { return users; }  ///< 获取使用者列表
  auto getUseds() const -> const std::list<Mid2EndDAGNode *> & { return useds; }  ///< 获取被使用者列表
  auto getNumUsers() const -> unsigned { return users.size(); }                   ///< 获取使用者数量
  auto getNumUseds() const -> unsigned { return useds.size(); }                   ///< 获取被使用者数量
  auto getParent() const -> Mid2EndDAG * { return parent; }                       ///< 获取父DAG
  auto getNodeType() const -> NodeType { return nodeType; }                       ///< 获取节点类型
  auto isCommutative() const -> bool {
    constexpr auto mask = ADD | MUL | ICMPEQ | ICMPNE | FADD | FMUL | FCMPEQ | FCMPNE | MIN | MAX | FEQ | ADD64 | MUL64;
    return (nodeType & mask) != 0;
  }  ///< 是否是可交换的
};
/**
 * @brief 中端Int节点
 *
 */
class Mid2EndIntNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;
  friend class Mid2EndDAGSimplifier;

 protected:
  int value;  ///< int值

 protected:
  Mid2EndIntNode(int value, Mid2EndDAG *parent) : Mid2EndDAGNode(NodeType::INT, parent), value(value) {}

 public:
  auto getInt() const -> int { return value; }  ///< 获取int值
};
/**
 * @brief 中端uint64节点
 *
 */
class Mid2EndUint64Node : public Mid2EndDAGNode {
  friend class Mid2EndDAGSimplifier;

 protected:
  uint64_t value;  ///< uint64值

 protected:
  Mid2EndUint64Node(uint64_t value, Mid2EndDAG *parent) : Mid2EndDAGNode(NodeType::UINT64, parent), value(value) {}

 public:
  auto getUint64() const -> uint64_t { return value; }  ///< 获取uint64值
};
/**
 * @brief 中端float节点
 *
 */
class Mid2EndFloatNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;
  friend class Mid2EndDAGSimplifier;

 protected:
  float value;  ///< float值

 protected:
  explicit Mid2EndFloatNode(float value, Mid2EndDAG *parent) : Mid2EndDAGNode(NodeType::FLOAT, parent), value(value) {}

 public:
  auto getFloat() const -> float { return value; }  ///< 获取float值
};
/**
 * @brief 中端Pointer节点
 *
 */
class Mid2EndPointerNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;
  friend class Module;

 protected:
  bool isInt;           ///< 是否是整型指针
  std::list<int> dims;  ///< 维度数组

 protected:
  Mid2EndPointerNode(bool isInt, std::list<int> dims, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::POINTER, parent), isInt(isInt), dims(std::move(dims)) {}

 public:
  auto isIntPointer() const -> bool { return isInt; }              ///< 是否是整型指针
  auto getDims() const -> const std::list<int> & { return dims; }  ///< 获取维度数组
};
/**
 * @brief 中端二元运算节点
 *
 */
class Mid2EndBinaryOpNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;
  friend class Mid2EndDAGSimplifier;

 protected:
  Mid2EndBinaryOpNode(NodeType nodeType, Mid2EndDAGNode *lhs, Mid2EndDAGNode *rhs, Mid2EndDAG *parent)
      : Mid2EndDAGNode(nodeType, parent) {
    addUsed(lhs);
    addUsed(rhs);
    lhs->addUser(this);
    rhs->addUser(this);
  }

 public:
  auto getLhs() const -> Mid2EndDAGNode * { return getUsed(0); }  ///< 获取左操作数节点
  auto getRhs() const -> Mid2EndDAGNode * { return getUsed(1); }  ///< 获取右操作数节点
};
/**
 * @brief 中端一元运算节点
 *
 */
class Mid2EndUnaryOpNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;
  friend class Mid2EndDAGSimplifier;

 protected:
  Mid2EndUnaryOpNode(NodeType nodeType, Mid2EndDAGNode *hs, Mid2EndDAG *parent) : Mid2EndDAGNode(nodeType, parent) {
    addUsed(hs);
    hs->addUser(this);
  }

 public:
  auto getHs() const -> Mid2EndDAGNode * { return getUsed(0); }  ///< 获取操作数节点
};
/**
 * @brief 中端Call节点
 *
 */
class Mid2EndCallNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 protected:
  unsigned numParams;  ///< 参数数量
  Function *callee;    ///< 被调用函数

 protected:
  template <typename ContainerT>
  explicit Mid2EndCallNode(Function *function, const ContainerT &params, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::CALL, parent) {
    addUseds(params);
    for (auto &param : params) {
      param->addUser(this);
    }
    callee = function;
    numParams = params.size();
  }

 public:
  auto getParams() const -> std::vector<Mid2EndDAGNode *> {
    std::vector<Mid2EndDAGNode *> result;
    for (unsigned int i = 0; i < numParams; i++) {
      result.emplace_back(getUsed(i));
    }
    return result;
  }                                                                                   ///< 获取参数列表
  auto getParam(unsigned index) const -> Mid2EndDAGNode * { return getUsed(index); }  ///< 获取参数
  auto getCallee() const -> Function * { return callee; }                             ///< 获取被调用函数
};
/**
 * @brief 中端条件跳转节点
 *
 */
class Mid2EndCondBrNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 protected:
  Mid2EndDAG *thenDAG;  ///< true分支DAG
  Mid2EndDAG *elseDAG;  ///< false分支DAG

 protected:
  Mid2EndCondBrNode(Mid2EndDAGNode *cond, Mid2EndDAG *thenDAG, Mid2EndDAG *elseDAG, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::CONDBR, parent), thenDAG(thenDAG), elseDAG(elseDAG) {
    addUsed(cond);
    cond->addUser(this);
  }

 public:
  auto getCond() const -> Mid2EndDAGNode * { return getUsed(0); }  ///< 获取条件
  auto getThenDAG() const -> Mid2EndDAG * { return thenDAG; }      ///< 获取true分支DAG
  auto getElseDAG() const -> Mid2EndDAG * { return elseDAG; }      ///< 获取false分支DAG
};
/**
 * @brief Riscv条件跳转节点
 *
 */
class Mid2EndRiscvCondBrNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;
  friend class Mid2EndDAGSimplifier;

 protected:
  Mid2EndDAG *thenDAG;  ///< true分支DAG

 protected:
  Mid2EndRiscvCondBrNode(Mid2EndDAGNode *lhs, Mid2EndDAGNode *rhs, Mid2EndDAG *thenDAG, NodeType nodeType,
                         Mid2EndDAG *parent)
      : Mid2EndDAGNode(nodeType, parent), thenDAG(thenDAG) {
    addUsed(lhs);
    addUsed(rhs);
    lhs->addUser(this);
    rhs->addUser(this);
  }

 public:
  auto getLhsNode() const -> Mid2EndDAGNode * { return getUsed(0); }  ///< 获取左操作数
  auto getRhsNode() const -> Mid2EndDAGNode * { return getUsed(1); }  ///< 获取右操作数
  auto getThenDAG() const -> Mid2EndDAG * { return thenDAG; }         ///< 获取true分支DAG
};
/**
 * @brief 中端无条件跳转节点
 *
 */
class Mid2EndBrNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;
  friend class Mid2EndDAGSimplifier;

 protected:
  Mid2EndDAG *thenDAG;  ///< 目标DAG

 protected:
  explicit Mid2EndBrNode(Mid2EndDAG *thenDAG, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::BR, parent), thenDAG(thenDAG) {}

 public:
  auto getThenDAG() const -> Mid2EndDAG * { return thenDAG; }  ///< 获取目标DAG
};
/**
 * @brief 中端Return节点
 *
 */
class Mid2EndReturnNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 protected:
  bool isHasReturnValue;  ///< 是否有返回值

 protected:
  explicit Mid2EndReturnNode(Mid2EndDAGNode *val, Mid2EndDAG *parent) : Mid2EndDAGNode(NodeType::RETURN, parent) {
    if (val != nullptr) {
      addUsed(val);
      val->addUser(this);
      isHasReturnValue = true;
    } else {
      isHasReturnValue = false;
    }
  }

 public:
  auto getVal() const -> Mid2EndDAGNode * {
    if (isHasReturnValue) {
      return getUsed(0);
    }
    return nullptr;
  }  ///< 获取返回值节点
};
/**
 * @brief 标量节点
 *
 */
class Mid2EndScalarNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 protected:
  bool isInt;                    ///< 是否是整型
  bool is64;                     ///< 是否是64位
  Mid2EndPointerNode *ancestor;  ///< 祖先指针节点
  std::list<int> dims;           ///< 维度数组

 protected:
  explicit Mid2EndScalarNode(bool isInt, Mid2EndDAG *parent, bool is64 = false, Mid2EndPointerNode *ancestor = nullptr,
                             std::list<int> dims = {})
      : Mid2EndDAGNode(NodeType::SCALAR, parent), isInt(isInt), is64(is64), ancestor(ancestor), dims(std::move(dims)) {}

 public:
  auto isIntScalar() const -> bool { return isInt; }                     ///< 是否是整型标量
  auto is64Scalar() const -> bool { return is64; }                       ///< 是否是64位标量
  auto getAncestor() const -> Mid2EndPointerNode * { return ancestor; }  ///< 获取祖先指针节点
  auto getDims() const -> const std::list<int> & { return dims; }        ///< 获取维度数组
};
/**
 * @brief GetSubArray节点
 *
 */
class Mid2EndGetSubArrayNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 protected:
  Mid2EndPointerNode *ancestor{};  ///< 祖先指针节点
  std::list<int> dims;             ///< 维度数组

 protected:
  template <typename ContainerT>
  Mid2EndGetSubArrayNode(Mid2EndDAGNode *pointer, const ContainerT &indices, Mid2EndPointerNode *ancestor,
                         Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::GETSUBARRAY, parent), ancestor(ancestor) {
    assert(ancestor);
    addUsed(pointer);
    addUseds(indices);
    pointer->addUser(this);
    for (auto &index : indices) {
      index->addUser(this);
    }

    std::list<int> oldDims;
    if (pointer->getNodeType() == Mid2EndDAGNode::GETSUBARRAY) {
      oldDims = dynamic_cast<Mid2EndGetSubArrayNode *>(pointer)->getDims();
    } else if (pointer->getNodeType() == Mid2EndDAGNode::POINTER) {
      oldDims = dynamic_cast<Mid2EndPointerNode *>(pointer)->getDims();
    } else if (pointer->getNodeType() == Mid2EndDAGNode::SCALAR) {
      oldDims = dynamic_cast<Mid2EndScalarNode *>(pointer)->getDims();
    }
    auto iter = std::next(oldDims.begin(), indices.size());
    while (iter != oldDims.end()) {
      dims.emplace_back(*iter);
      iter++;
    }
  }

 public:
  auto getPointer() const -> Mid2EndDAGNode * { return getUsed(0); }    ///< 获取指针节点
  auto getNumIndices() const -> unsigned { return getNumUseds() - 1; }  ///< 获取索引数量
  auto getIndices() const -> std::vector<Mid2EndDAGNode *> {
    std::vector<Mid2EndDAGNode *> indices;
    for (unsigned i = 1; i < getNumUseds(); i++) {
      indices.emplace_back(getUsed(i));
    }
    return indices;
  }                                                                      ///< 获取索引列表
  auto getDims() const -> const std::list<int> & { return dims; }        ///< 获取维度数组
  auto getNumDims() const -> unsigned { return dims.size(); }            ///< 获取维度数组数量
  auto getAncestor() const -> Mid2EndPointerNode * { return ancestor; }  ///< 获取祖先指针节点
};
/**
 * @brief 中端load节点
 *
 */
class Mid2EndLoadNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 protected:
  unsigned numIndex;  ///< 索引数量

 protected:
  template <typename ContainerT>
  Mid2EndLoadNode(Mid2EndDAGNode *pointer, const ContainerT &indices, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::LOAD, parent) {
    addUsed(pointer);
    addUseds(indices);
    pointer->addUser(this);
    for (auto &index : indices) {
      index->addUser(this);
    }
    numIndex = indices.size();
  }

 public:
  auto getPointer() const -> Mid2EndDAGNode * { return getUsed(0); }  ///< 获取指针
  auto getIndices() const -> std::vector<Mid2EndDAGNode *> {
    std::vector<Mid2EndDAGNode *> indices;
    for (unsigned i = 1; i < numIndex + 1; i++) {
      indices.emplace_back(getUsed(i));
    }
    return indices;
  }                                                            ///< 获取索引数组
  auto getNumIndices() const -> unsigned { return numIndex; }  ///< 获取索引数量
  auto getAncestorPointer() const -> Mid2EndPointerNode * {
    if (getPointer()->getNodeType() == GETSUBARRAY) {
      return dynamic_cast<Mid2EndGetSubArrayNode *>(getPointer())->getAncestor();
    }
    if (getPointer()->getNodeType() == SCALAR) {
      return dynamic_cast<Mid2EndScalarNode *>(getPointer())->getAncestor();
    }
    return dynamic_cast<Mid2EndPointerNode *>(getPointer());
  }  ///< 获取祖先指针节点
};
/**
 * @brief Riscv Load节点
 *
 */
class Mid2EndRVLoadNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGSimplifier;

 protected:
  bool isint;  ///< load值是否为整型

 protected:
  Mid2EndRVLoadNode(Mid2EndDAGNode *pointer, Mid2EndDAGNode *offset, bool isint, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::RVLOAD, parent), isint(isint) {
    addUsed(pointer);
    addUsed(offset);
    pointer->addUser(this);
    offset->addUser(this);
  }

 public:
  auto getPointer() const -> Mid2EndDAGNode * { return getUsed(0); }  ///< 获取指针
  auto getOffset() const -> Mid2EndDAGNode * { return getUsed(1); }   ///< 获取偏移量
  auto isIntValue() -> bool { return isint; }                         ///< load值是否为整型
};
/**
 * @brief 中端Store节点
 *
 */
class Mid2EndStoreNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 protected:
  unsigned numIndex;  ///< 索引数量

 protected:
  template <typename ContainerT>
  Mid2EndStoreNode(Mid2EndDAGNode *pointer, Mid2EndDAGNode *value, const ContainerT &indices, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::STORE, parent) {
    addUsed(pointer);
    addUsed(value);
    addUseds(indices);
    pointer->addUser(this);
    value->addUser(this);
    for (auto &index : indices) {
      index->addUser(this);
    }
    numIndex = indices.size();
  }

 public:
  auto getPointer() const -> Mid2EndDAGNode * { return getUsed(0); }  ///< 获取指针
  auto getValue() const -> Mid2EndDAGNode * { return getUsed(1); }    ///< 获取要存的Value
  auto getIndices() const -> std::vector<Mid2EndDAGNode *> {
    std::vector<Mid2EndDAGNode *> indices;
    for (unsigned i = 2; i < numIndex + 2; i++) {
      indices.emplace_back(getUsed(i));
    }
    return indices;
  }                                                            ///< 获取索引列表
  auto getNumIndices() const -> unsigned { return numIndex; }  ///< 获取索引数量
  auto getAncestorPointer() const -> Mid2EndPointerNode * {
    if (getPointer()->getNodeType() == GETSUBARRAY) {
      return dynamic_cast<Mid2EndGetSubArrayNode *>(getPointer())->getAncestor();
    }
    if (getPointer()->getNodeType() == SCALAR) {
      return dynamic_cast<Mid2EndScalarNode *>(getPointer())->getAncestor();
    }
    return dynamic_cast<Mid2EndPointerNode *>(getPointer());
  }  ///< 获取祖先指针节点
};
/**
 * @brief Riscv Store节点
 *
 */
class Mid2EndRVStoreNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGSimplifier;

 protected:
  Mid2EndRVStoreNode(Mid2EndDAGNode *pointer, Mid2EndDAGNode *value, Mid2EndDAGNode *offset, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::RVSTORE, parent) {
    addUsed(pointer);
    addUsed(value);
    addUsed(offset);
    pointer->addUser(this);
    value->addUser(this);
    offset->addUser(this);
  }

 public:
  auto getPointer() const -> Mid2EndDAGNode * { return getUsed(0); }  ///< 获取指针
  auto getValue() const -> Mid2EndDAGNode * { return getUsed(1); }    ///< 获取值
  auto getOffset() const -> Mid2EndDAGNode * { return getUsed(2); }   ///< 获取偏移量
};
/**
 * @brief 中端La指令
 *
 */
class Mid2EndLaNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 protected:
  template <typename ContainerT>
  Mid2EndLaNode(Mid2EndPointerNode *pointer, const ContainerT &indices, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::LA, parent) {
    addUsed(pointer);
    addUseds(indices);
    pointer->addUser(this);
    for (auto &index : indices) {
      index->addUser(this);
    }
  }

 public:
  auto getPointer() const -> Mid2EndPointerNode * {
    return dynamic_cast<Mid2EndPointerNode *>(getUsed(0));
  }  ///< 获取指针
  auto getIndices() const -> std::vector<Mid2EndDAGNode *> {
    std::vector<Mid2EndDAGNode *> indices;
    for (unsigned i = 1; i < getNumUseds(); i++) {
      indices.emplace_back(getUsed(i));
    }
    return indices;
  }  ///< 获取索引列表
};
/**
 * @brief 中端Memset节点
 *
 */
class Mid2EndMemsetNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;

 protected:
  unsigned begin;  ///< 开始位置
  unsigned size;   ///< 待设置元素数量

 protected:
  Mid2EndMemsetNode(Mid2EndDAGNode *pointer, Mid2EndDAGNode *value, unsigned begin, unsigned size, Mid2EndDAG *parent)
      : Mid2EndDAGNode(NodeType::MEMSET, parent), begin(begin), size(size) {
    addUsed(pointer);
    addUsed(value);
    pointer->addUser(this);
    value->addUser(this);
  }

 public:
  auto getPointer() const -> Mid2EndDAGNode * { return getUsed(0); }  ///< 获取指针
  auto getValue() const -> Mid2EndDAGNode * { return getUsed(1); }    ///< 获取值
  auto getBegin() const -> unsigned { return begin; }                 ///< 获取开始位置
  auto getSize() const -> unsigned { return size; }                   ///< 获取待设置元素数量
};
/**
 * @brief riscv 三操作数指令节点
 *
 */
class Mid2EndF3OpNode : public Mid2EndDAGNode {
  friend class Mid2EndDAGBuilder;
  friend class Mid2EndDAGSimplifier;

 protected:
  Mid2EndF3OpNode(NodeType nodeType, Mid2EndDAGNode *hs0, Mid2EndDAGNode *hs1, Mid2EndDAGNode *hs2, Mid2EndDAG *parent)
      : Mid2EndDAGNode(nodeType, parent) {
    addUsed(hs0);
    addUsed(hs1);
    addUsed(hs2);
    hs0->addUser(this);
    hs1->addUser(this);
    hs2->addUser(this);
  }

 public:
  auto getHs(unsigned index) const -> Mid2EndDAGNode * { return getUsed(index); }  ///< 获取操作数
};
/**
 * @brief 中端DAG
 *
 */
class Mid2EndDAG {
  friend class Function;
  friend class Mid2EndDAGBuilder;
  friend class Mid2EndDAGSimplifier;

 protected:
  Function *parent;                                         ///< 父函数
  std::string name;                                         ///< 名字
  std::list<std::unique_ptr<Mid2EndDAGNode>> nodes;         ///< 节点列表
  std::set<Mid2EndScalarNode *> inputNodes;                 ///< 输入节点集合
  std::set<Mid2EndDAGNode *> outputNodes;                   ///< 输出节点集合
  std::map<int, Mid2EndIntNode *> intConstants;             ///< int常量表
  std::map<float, Mid2EndFloatNode *> floatConstants;       ///< float常量表
  std::map<uint64_t, Mid2EndUint64Node *> uint64Constants;  ///< uint64_t常量表
  Mid2EndDAGNode *terminatorNode = nullptr;                 ///< 终止子

  std::set<Mid2EndDAG *> predecessors;  ///<前驱集合
  std::set<Mid2EndDAG *> successors;    ///< 后继集合

 protected:
  explicit Mid2EndDAG(Function *parent, std::string name = "") : parent(parent), name(std::move(name)) {}

 public:
  ~Mid2EndDAG() {
    for (const auto &pred : predecessors) {
      pred->removeSuccessor(this);
    }

    for (const auto &succ : successors) {
      succ->removePredecessor(this);
    }
  }

 protected:
  auto addMidDAGNode(Mid2EndDAGNode *node) -> Mid2EndDAGNode * {
    nodes.emplace_back(node);
    if (node->getNodeType() == Mid2EndDAGNode::INT) {
      auto intNode = dynamic_cast<Mid2EndIntNode *>(node);
      if (intConstants.find(intNode->getInt()) == intConstants.end()) {
        intConstants.emplace(intNode->getInt(), intNode);
      }
    }
    if (node->getNodeType() == Mid2EndDAGNode::FLOAT) {
      auto floatNode = dynamic_cast<Mid2EndFloatNode *>(node);
      if (floatConstants.find(floatNode->getFloat()) == floatConstants.end()) {
        floatConstants.emplace(floatNode->getFloat(), floatNode);
      }
    }
    if (node->getNodeType() == Mid2EndDAGNode::UINT64) {
      auto uint64Node = dynamic_cast<Mid2EndUint64Node *>(node);
      if (uint64Constants.find(uint64Node->getUint64()) == uint64Constants.end()) {
        uint64Constants.emplace(uint64Node->getUint64(), uint64Node);
      }
    }
    return node;
  }                                                                     ///< 添加节点
  void addPredecessor(Mid2EndDAG *pred) { predecessors.insert(pred); }  ///< 添加前驱
  void addSuccessor(Mid2EndDAG *succ) { successors.insert(succ); }      ///< 添加后继
  void removeMidDAGNode(Mid2EndDAGNode *targetNode) {
    // 目前仅限于DAG层次的删除
    inputNodes.erase(dynamic_cast<Mid2EndScalarNode *>(targetNode));
    outputNodes.erase(targetNode);
    if (targetNode->getNodeType() == Mid2EndDAGNode::INT) {
      intConstants.erase(dynamic_cast<Mid2EndIntNode *>(targetNode)->getInt());
    }
    if (targetNode->getNodeType() == Mid2EndDAGNode::FLOAT) {
      floatConstants.erase(dynamic_cast<Mid2EndFloatNode *>(targetNode)->getFloat());
    }
    if (targetNode->getNodeType() == Mid2EndDAGNode::UINT64) {
      uint64Constants.erase(dynamic_cast<Mid2EndUint64Node *>(targetNode)->getUint64());
    }
    auto predicate = [&](const std::unique_ptr<Mid2EndDAGNode> &node) -> bool { return node.get() == targetNode; };
    nodes.remove_if(predicate);
    if (terminatorNode == targetNode) {
      terminatorNode = nullptr;
    }
  }                                                                     ///< 删除节点
  void removePredecessor(Mid2EndDAG *DAG) { predecessors.erase(DAG); }  ///< 删除前驱
  void removeSuccessor(Mid2EndDAG *DAG) { successors.erase(DAG); }      ///< 删除后继
  void setName(const std::string &newName) { name = newName; }          ///< 设置名字

 public:
  auto getMidDAGNodes() const -> const std::list<std::unique_ptr<Mid2EndDAGNode>> & { return nodes; }  ///< 获取节点列表
  auto getNumMidDAGNodes() const -> unsigned { return nodes.size(); }                         ///< 获取节点数量
  auto getName() const -> const std::string & { return name; }                                ///< 获取名字
  auto getParent() const -> Function * { return parent; }                                     ///< 获取父函数
  auto getPredecessors() const -> const std::set<Mid2EndDAG *> & { return predecessors; }     ///< 获取前驱集合
  auto getSuccessors() const -> const std::set<Mid2EndDAG *> & { return successors; }         ///< 获取后继集合
  auto getInputNodes() const -> const std::set<Mid2EndScalarNode *> & { return inputNodes; }  ///< 获取输入节点集合
  auto getOutputNodes() const -> const std::set<Mid2EndDAGNode *> & { return outputNodes; }  ///< 获取输出节点集合
  void setTerminatorNode(Mid2EndDAGNode *node) { terminatorNode = node; }                    ///< 设置终止子
  auto getTerminatorNode() const -> Mid2EndDAGNode * { return terminatorNode; }              ///< 获取终止子
  auto getIntConstants() const -> const std::map<int, Mid2EndIntNode *> & { return intConstants; }  ///< 获取int常量表
  auto getFloatConstants() const -> const std::map<float, Mid2EndFloatNode *> & {
    return floatConstants;
  }  ///< 获取float常量表
  auto getUint64Constants() const -> const std::map<uint64_t, Mid2EndUint64Node *> & {
    return uint64Constants;
  }                                                                            ///< 获取uint64常量表
  auto getNumPredecessors() const -> unsigned { return predecessors.size(); }  ///< 返回前驱数量
  auto getNumSuccessors() const -> unsigned { return successors.size(); }      ///< 返回后继数量
};

class Module;
/**
 * @brief 函数
 *
 */
class Function {
  friend class Module;
  friend class Mid2EndDAGBuilder;
  friend class Mid2EndDAGSimplifier;
  friend class DAG2EndBuilder;

 public:
  enum ReturnType { INT, FLOAT, VOID };  ///< 返回类型

 protected:
  ReturnType returnType;                                       ///< 返回类型
  Module *parent;                                              ///< 父模块
  std::string name;                                            ///< 名字
  std::list<std::unique_ptr<Mid2EndDAG>> DAGs;                 ///< DAG列表
  std::list<Mid2EndDAGNode *> params;                          ///< 参数列表，使用头部DAG中的节点表示
  std::list<std::unique_ptr<Mid2EndPointerNode>> localArrays;  ///< 局部数组列表
  std::map<Mid2EndDAGNode *, uint64_t> inputBind;              ///< 输入节点-标号绑定
  std::map<Mid2EndDAGNode *, std::set<uint64_t>> outputBind;   ///< 输出节点-标号绑定
  std::map<uint64_t, uint64_t> varTypeMap;                     ///<标号-类型对应，0表示int，2表示float

 protected:
  Function(ReturnType returnType, Module *parent, std::string name = "")
      : returnType(returnType), parent(parent), name(std::move(name)) {}

 protected:
  void addParam(Mid2EndDAGNode *node) { params.emplace_back(node); }  ///< 添加参数
  template <typename ContainerT>
  void addParams(const ContainerT &nodes) {
    for (const auto &node : nodes) {
      addParam(node);
    }
  }                                                                                 ///< 添加多个参数
  void addLocalArray(Mid2EndPointerNode *node) { localArrays.emplace_back(node); }  ///< 添加局部数组

 protected:
  auto addMidDAG(std::string name) -> Mid2EndDAG * {
    auto result = new Mid2EndDAG(this, std::move(name));
    DAGs.emplace_back(result);
    return result;
  }  ///< 添加DAg
  void removeMidDAG(Mid2EndDAG *targetDAG) {
    for (const auto &input : targetDAG->getInputNodes()) {
      inputBind.erase(input);
    }
    for (const auto &output : targetDAG->getOutputNodes()) {
      outputBind.erase(output);
    }
    auto predicate = [&](const std::unique_ptr<Mid2EndDAG> &DAG) { return DAG.get() == targetDAG; };
    DAGs.remove_if(predicate);
  }  ///< 删除DAG

 public:
  auto getMidDAGs() const -> const std::list<std::unique_ptr<Mid2EndDAG>> & { return DAGs; }  ///> 获取DAG列表
  auto getNumMidDAGs() const -> unsigned { return DAGs.size(); }                              ///< 获取DAG数量
  auto getParams() const -> const std::list<Mid2EndDAGNode *> & { return params; }            ///< 获取参数列表
  auto getParam(unsigned index) const -> Mid2EndDAGNode * { return *std::next(params.begin(), index); }  ///< 获取参数
  auto getNumParams() const -> unsigned { return params.size(); }          ///< 获取参数数量
  auto getName() const -> const std::string & { return name; }             ///< 获取名字
  auto getParent() const -> const Module * { return parent; }              ///< 获取父模块
  auto getEntryDAG() const -> Mid2EndDAG * { return DAGs.front().get(); }  ///< 获取entryDAG
  auto getOutputBind() const -> const std::map<Mid2EndDAGNode *, std::set<uint64_t>> & {
    return outputBind;
  }  ///< 获取输出绑定
  auto getInputBind() const -> const std::map<Mid2EndDAGNode *, uint64_t> & { return inputBind; }  ///< 获取输入绑定
  auto getVarTypeMap() const -> const std::map<uint64_t, uint64_t> & { return varTypeMap; }  ///< 获取标号-类型绑定
  auto getLocalArrays() const -> const std::list<std::unique_ptr<Mid2EndPointerNode>> & {
    return localArrays;
  }                                                                ///< 获取局部数组列表
  auto getReturnType() const -> ReturnType { return returnType; }  ///< 获取返回值类型
  void sortDAGs();                                                 ///< 排列DAG
  auto renameDAGs(uint64_t begin) -> uint64_t;                     ///< 重命名DAG
};
/**
 * @brief 模块
 *
 */
class Module {
  friend class Mid2EndDAGBuilder;

 protected:
  std::map<std::string, std::unique_ptr<Function>> functionMap;          ///< 函数表
  std::map<std::string, std::unique_ptr<Function>> externalFunctionMap;  ///< 外部函数表
  std::map<std::string, std::unique_ptr<Mid2EndPointerNode>> globalMap;  ///<全局变量表
  std::map<std::string, ConstantCounter> globalInitMap;                  ///< 全局变量初始化值表

 protected:
  Module() = default;

 protected:
  auto createFunction(const std::string &name, Function::ReturnType returnType) -> Function * {
    auto function = new Function(returnType, this, name);
    functionMap.emplace(name, function);
    return function;
  }  ///< 创建函数

  auto createExternalFunction(const std::string &name, Function::ReturnType returnType) -> Function * {
    auto function = new Function(returnType, this, name);
    externalFunctionMap.emplace(name, function);
    return function;
  }  ///< 创建外部函数

  auto createGlobal(bool isInt, const std::list<int> &dims, ConstantCounter &init, const std::string &name)
      -> Mid2EndPointerNode * {
    auto global = new Mid2EndPointerNode(isInt, dims, nullptr);
    globalInitMap.emplace(name, init);
    globalMap.emplace(name, global);
    return global;
  }  ///< 创建全局变量

 public:
  auto getFunctions() const -> const std::map<std::string, std::unique_ptr<Function>> & {
    return functionMap;
  }  ///< 获取函数表
  auto getExternalFunctions() const -> const std::map<std::string, std::unique_ptr<Function>> & {
    return externalFunctionMap;
  }  ///< 获取外部函数表
  auto getGlobals() const -> const std::map<std::string, std::unique_ptr<Mid2EndPointerNode>> & {
    return globalMap;
  }  ///< 获取全局变量表
  auto getFunction(const std::string &name) const -> Function * {
    if (functionMap.find(name) != functionMap.end()) {
      return functionMap.at(name).get();
    }
    if (externalFunctionMap.find(name) != externalFunctionMap.end()) {
      return externalFunctionMap.at(name).get();
    }
    assert(false);
  }  ///< 获取函数
  auto getGlobal(const std::string &name) const -> Mid2EndPointerNode * {
    return globalMap.at(name).get();
  }  ///< 获取全局变量
  auto getGlobalInit(const std::string &name) const -> const ConstantCounter & {
    return globalInitMap.at(name);
  }  ///< 获取全局变量初始值
};

}  // namespace mid2EndDAG