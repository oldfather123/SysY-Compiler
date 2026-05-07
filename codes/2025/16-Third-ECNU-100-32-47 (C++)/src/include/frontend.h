/**
 * @file frontend.h
 *
 * 该头文件定义了前端相关的数据结构
 */
#ifndef AAAC_FRONTEND_H
#define AAAC_FRONTEND_H

#include "ADT/bitvec.h"
#include "AST/decl.h"
#include "AST/type.h"
#include "common.h"
#include "sym.h"
#include "utils/property_manager.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace aaac {
namespace frontend {

enum class NodeCode : uint8_t {
  Base,
  Assign,    // class AssignNode
  Load,      // class LoadNode
  Store,     // class StoreNode
  Call,      // class CallNode
  Condition, // class ConditionNode
  Label,     // class LabelNode
  Return,    // class ReturnNode
  Jump,      // class JumpNode
  Nop,       // class NopNode

  Phi, // class PhiNode
};
using Operand = std::variant<std::shared_ptr<sym::Var>, int, float>;
using OperandList = std::vector<Operand>;
class InsnNode;
DEF_LIST(InsnNode);
class BindNode;
DEF_LIST(BindNode);
class BasicBlock;
class ControlFlowGraph;
class Module;

/**
 * @brief 节点基类
 */
class InsnNode : public DebugDumpImpl {
protected:
  std::weak_ptr<BasicBlock> parent;
  NodeCode code;
  InsnNode(NodeCode code) : DebugDumpImpl(),parent(), code(code) {}

public:
  NodeCode getCode() const { return code; }
  bool isCTI() {
    switch (code) {
      case NodeCode::Jump:
      case NodeCode::Condition:
      case NodeCode::Return:
        return true;
      default:
        return false;
    }
  }
  bool isLabel() { return code == NodeCode::Label; }
  void setParent(std::shared_ptr<BasicBlock> bb){this->parent=bb;}
  std::shared_ptr<BasicBlock> getParent()const{return this->parent.lock();}
  virtual void emitInsn(backend::InsnList &list) = 0;
  virtual bool replaceDefWith(std::shared_ptr<sym::Var> new_def) = 0;
  virtual bool replaceUsesWith(Operand old_, Operand new_) = 0;
  virtual bool isUse(sym::Var *var) const = 0;
  virtual bool isDef(sym::Var *var) const = 0;
  virtual sym::VarList getDefs() const = 0;
  virtual sym::VarList getUses() const = 0;
  virtual ~InsnNode() override = default;
  // virtual ~InsnNode() override {
  //   std::cout << "release node " << this << "\n";
  // };
};

/**
 * @brief 标签节点
 *
 * 该节点可以作为JumpNode, ConditionNode的参数，指明这类跳转Node的目标位置
 */
class LabelNode final : public InsnNode, public Createable<LabelNode> {
private:
  static unsigned int temp_count;
  std::string label;

public:
  LabelNode(const std::string &label)
      : InsnNode(NodeCode::Label), label(label) {}
  std::string getName() const { return label; }
  // int emitInsn(backend::InsnList &list,
  //              std::shared_ptr<sym::Block> scope) override;
  void emitInsn(backend::InsnList &list) override;

  ~LabelNode() override = default;
  std::ostream &dump(std::ostream &os) const override;
  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    return false;
  }
  bool replaceUsesWith(Operand old_, Operand new_) override { return false; }
  bool isUse(sym::Var *var) const override { return false; }
  bool isDef(sym::Var *var) const override { return false; }
  sym::VarList getDefs() const override { return {}; }
  sym::VarList getUses() const override { return {}; }

  /**
   * @brief 用于快速生成一个标签
   *
   * @param prefix
   * 前缀，比如，使用"L_"作为前缀会生成一个形如L_1002的标签，但是使用不同的前缀，后面的计数器仍然相同且单调递增
   */
  static std::shared_ptr<LabelNode>
  generateTemp(const std::string &prefix = "L_") {
    return std::make_shared<LabelNode>(prefix + std::to_string(temp_count++));
  }
};
inline unsigned int LabelNode::temp_count = 100;

/**
 * @brief 运算符
 */
enum class OpCode : uint16_t {
  Add,          // 加法
  Sub,          // 减法
  Mul,          // 乘法
  Div,          // 除法
  Mod,          // 取余
  Assign,       // 赋值
  Negate,       // 负号
  LShift,       // 左移
  RShift,       // 右移
  BitOr,        // 按位或
  BitAnd,       // 按位与
  BitXor,       // 按位异或
  BitNot,       // 按位取反
  Interger_cst, // 转成int类型
  Float_cst,    // 转成float类型
  AddressOf,    // 获取某个变量的地址
  Eq,           // 相等
  Neq,          // 不相等
  Leq,          // 小于等于
  Lt,           // 小于
  Geq,          // 大于等于
  Gt,           // 大于
  LogNot,       // 逻辑非
};

/**
 * @brief 赋值节点
 *
 * 该节点拥有一个值(result)，这个值是OpCode中的某个运算的结果。
 */
class AssignNode : public InsnNode {
protected:
  OpCode opcode;
  std::shared_ptr<sym::Var> result;
  OperandList operands;

public:
  AssignNode(OpCode opcode, std::shared_ptr<sym::Var> result,
             const OperandList &operands)
      : InsnNode(NodeCode::Assign), opcode(opcode), result(result),
        operands(operands) {}
  std::shared_ptr<sym::Var> getResult() const { return result; }
  OpCode getOpCode() const { return opcode; }

  virtual bool isUse(sym::Var *var) const override {
    for (auto op : operands) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(op) &&
          std::get<std::shared_ptr<sym::Var>>(op).get() == var)
        return true;
    }
    return false;
  }
  virtual bool isDef(sym::Var *var) const override {
    return result.get() == var;
  }
  virtual sym::VarList getDefs() const override { return {result}; }
  virtual sym::VarList getUses() const override {
    sym::VarList uses;
    for (auto op : operands) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(op)) {
        uses.push_back(std::get<std::shared_ptr<sym::Var>>(op));
      }
    }
    return uses;
  }
  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    this->result = new_def;
    return true;
  }
  bool replaceUsesWith(Operand old_, Operand new_) override {
    bool replaced = false;

    // 遍历 operands 中的每个操作数
    for (auto &op : operands) {
      if (op == old_) {
        op = new_;
        replaced = true;
      }
    }

    return replaced;
  }
  OperandList getOperands() const { return operands; }
  Operand getNthOperand(size_t n) const { return operands[n]; }

  std::ostream &dump(std::ostream &os) const override;
  virtual ~AssignNode() override = default;
};

/**
 * @brief 定义AssignNode的子类
 *
 * @param classname 这个子类的名称
 * @param opcode    这个子类的操作码
 * @param numop     这个子类的操作数个数
 */
#define DEF_ASSIGN_NODE(classname, opcode, numop, ivariant, fvariant)          \
  class classname##AssignNode final                                            \
      : public AssignNode,                                                     \
        public Createable<classname##AssignNode> {                             \
  public:                                                                      \
    classname##AssignNode(std::shared_ptr<sym::Var> var,                       \
                          const OperandList &operands)                         \
        : AssignNode(OpCode::opcode, var, operands) {                          \
      Assert(operands.size() == numop, "Operand count mismatch ");             \
    }                                                                          \
    void emitInsn(backend::InsnList &list) override;                           \
  };

#include "./nodetype.def"

/**
 * @brief 加载节点
 *
 * 将内存中的值加载到寄存器中
 */
class LoadNode final : public InsnNode, public Createable<LoadNode> {
private:
  std::shared_ptr<sym::Var> var;
  std::shared_ptr<sym::Var> array;
  OperandList indexes;

public:
  LoadNode(std::shared_ptr<sym::Var> var, std::shared_ptr<sym::Var> array,
           OperandList indexes)
      : InsnNode(NodeCode::Load), var(var), array(array), indexes(indexes){};
  OperandList getIndexes() const { return indexes; }
  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    var = new_def;
    return true;
  }
  bool replaceUsesWith(Operand old_, Operand new_) override {
    bool replaced = false;
    for (auto &op : indexes) {
      if (op == old_) {
        op = new_;
        replaced = true;
      }
    }
    return replaced;
  }
  bool isDef(sym::Var *var) const override { return var == this->var.get(); }
  bool isUse(sym::Var *var) const override {
    return std::find_if(
               indexes.begin(), indexes.end(), [var](const Operand &op) {
                 return std::holds_alternative<std::shared_ptr<sym::Var>>(op) &&
                        std::get<std::shared_ptr<sym::Var>>(op).get() == var;
               }) != indexes.end();
  }
  sym::VarList getDefs() const override { return {var}; }
  sym::VarList getUses() const override {
    sym::VarList uses;
    for (auto &op : this->indexes) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(op)) {
        uses.push_back(std::get<std::shared_ptr<sym::Var>>(op));
      }
    }
    uses.push_back(array);
    return uses;
  }
  std::shared_ptr<sym::Var> getVar() const { return var; }
  std::shared_ptr<sym::Var> getArray() const { return array; }
  std::ostream &dump(std::ostream &os) const override;
  void emitInsn(backend::InsnList &list) override;
};

/**
 * @brief 存储节点
 *
 * 将寄存器中的值存入内存中
 */
class StoreNode final : public InsnNode, public Createable<StoreNode> {
private:
  std::shared_ptr<sym::Var> array;
  OperandList indexes;
  Operand value;

public:
  StoreNode(std::shared_ptr<sym::Var> array, OperandList indexes, Operand value)
      : InsnNode(NodeCode::Store), array(array), indexes(indexes),
        value(value) {}
  std::ostream &dump(std::ostream &os) const override;
  OperandList getIndexes() const { return indexes; }
  Operand getValue() const { return value; }
  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    return false;
  }
  bool replaceUsesWith(Operand old_, Operand new_) override {
    bool replaced = false;
    for (auto &op : this->indexes) {
      if (op == old_) {
        op = new_;
        replaced = true;
      }
    }
    if (value == old_) {
      value = new_;
      replaced = true;
    }
    return replaced;
  }
  bool isUse(sym::Var *var) const override {
    return std::find_if(
               indexes.begin(), indexes.end(),
               [var](const Operand &op) {
                 return std::holds_alternative<std::shared_ptr<sym::Var>>(op) &&
                        std::get<std::shared_ptr<sym::Var>>(op).get() == var;
               }) != indexes.end() ||
           (std::holds_alternative<std::shared_ptr<sym::Var>>(value) &&
            std::get<std::shared_ptr<sym::Var>>(value).get() == var);
  }

  bool isDef(sym::Var *var) const override {
    // StoreNode 不定义任何变量
    return false;
  }

  sym::VarList getDefs() const override {
    // StoreNode 不定义任何变量
    return {};
  }

  sym::VarList getUses() const override {
    sym::VarList uses;
    if (std::holds_alternative<std::shared_ptr<sym::Var>>(value)) {
      uses.push_back(std::get<std::shared_ptr<sym::Var>>(value));
    }
    for (auto &op : indexes) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(op)) {
        uses.push_back(std::get<std::shared_ptr<sym::Var>>(op));
      }
    }
    uses.push_back(array);
    return uses;
  }
  Operand getVal() const { return value; }
  std::shared_ptr<sym::Var> getArray() const { return array; }

  ~StoreNode() override = default;
  void emitInsn(backend::InsnList &list) override;
};

/**
 * @brief 函数调用节点
 *
 * 该Node的构造函数接收一个可选的返回值类型（一个Var的指针），如果给出了这个指针，那么说明返回值不会被丢弃，而是放到这个值当中，但是即使参数列表为空我们也应该提供，
 */
class CallNode final : public InsnNode, public Createable<CallNode> {
private:
  sym::Function *function;
  std::shared_ptr<sym::Var> return_value;
  OperandList args;

public:
  using iterator = OperandList::iterator;
  using const_iterator = OperandList::const_iterator;
  CallNode(sym::Function *function, std::shared_ptr<sym::Var> return_value,
           const OperandList &args)
      : InsnNode(NodeCode::Call), function(function),
        return_value(return_value), args(args) {}
  CallNode(sym::Function *function, const OperandList &args)
      : CallNode(function, nullptr, args) {}

  sym::Function *getFunction() const { return function; }
  // int emitInsn(backend::InsnList &list,
  //              std::shared_ptr<sym::Block> scope) override;
  bool returnValueDiscarded() const { return return_value == nullptr; }
  std::shared_ptr<sym::Var> getReturnValue() const { return return_value; }
  iterator begin() { return args.begin(); }
  iterator end() { return args.end(); }
  const_iterator begin() const { return args.begin(); }
  const_iterator end() const { return args.end(); }
  operator std::shared_ptr<sym::Var>() const {
    Assert(!this->returnValueDiscarded(),
           "Cannot convert discarded return value to Operand");
    return this->return_value; // 返回函数调用结果变量
  }
  std::ostream &dump(std::ostream &os) const override;
  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    if (return_value) {
      return_value = new_def;
      return true;
    } else {
      return false;
    }
  }
  bool replaceUsesWith(Operand old_, Operand new_) override {
    bool replaced = false;

    for (auto &arg : args) {
      if (arg == old_) {
        arg = new_;
        replaced = true;
      }
    }
    return replaced;
  }
  bool isUse(sym::Var *var) const override {
    for (const auto &arg : args) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(arg) &&
          std::get<std::shared_ptr<sym::Var>>(arg).get() == var)
        return true;
    }
    return false;
  }

  bool isDef(sym::Var *var) const override {
    return return_value && return_value.get() == var;
  }

  sym::VarList getDefs() const override {
    return return_value ? sym::VarList{return_value} : sym::VarList{};
  }

  sym::VarList getUses() const override {
    sym::VarList uses;
    for (const auto &arg : args) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(arg)) {
        uses.push_back(std::get<std::shared_ptr<sym::Var>>(arg));
      }
    }
    return uses;
  }
  ~CallNode() override = default;
  void emitInsn(backend::InsnList &list) override;
};

/**
 * @brief 条件节点
 *
 * 测试 op1 OP op2是否为真，根据值的真假跳转到不同的位置
 */
class ConditionNode : public InsnNode {
protected:
  OpCode opcode;
  OperandList operands;
  std::shared_ptr<LabelNode> true_label;
  std::shared_ptr<LabelNode> false_label;

public:
  ConditionNode(OpCode opcode, OperandList operands,
                std::shared_ptr<LabelNode> true_label,
                std::shared_ptr<LabelNode> false_label)
      : InsnNode(NodeCode::Condition), opcode(opcode), operands(operands),
        true_label(true_label), false_label(false_label) {}
  ConditionNode(OpCode opcode, OperandList operands)
      : ConditionNode(opcode, operands, nullptr, nullptr) {}

  void patchTrueLabel(std::shared_ptr<LabelNode> label) { true_label = label; }
  void patchFalseLabel(std::shared_ptr<LabelNode> label) {
    false_label = label;
  }
  OpCode getOpCode() const { return opcode; }
  OperandList getOperands() const { return operands; }
  Operand getNthOperand(size_t n) const { return operands[n]; }

  std::shared_ptr<LabelNode> getTrueLabel() const { return true_label; }
  std::shared_ptr<LabelNode> getFalseLabel() const { return false_label; }
  // int emitInsn(backend::InsnList &list,
  //              std::shared_ptr<sym::Block> scope) override;
  std::ostream &dump(std::ostream &os) const override;
  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    return false;
  }
  bool replaceUsesWith(Operand old_, Operand new_) override {
    bool replaced = false;

    // 遍历 operands 中的每个操作数
    for (auto &op : operands) {
      if (op == old_) {
        op = new_;
        replaced = true;
      }
    }

    return replaced;
  }
  virtual bool isUse(sym::Var *var) const override {
    for (auto op : operands) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(op) &&
          std::get<std::shared_ptr<sym::Var>>(op).get() == var)
        return true;
    }
    return false;
  }

  virtual bool isDef(sym::Var *var) const override { return false; }

  virtual sym::VarList getDefs() const override { return {}; }

  virtual sym::VarList getUses() const override {
    sym::VarList uses;
    for (auto op : operands) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(op)) {
        uses.push_back(std::get<std::shared_ptr<sym::Var>>(op));
      }
    }
    return uses;
  }
  virtual ~ConditionNode() override = default;
};

#define DEF_CONDITION_NODE(classname, opcode, numop, backendOp)                \
  class classname##ConditionNode final                                         \
      : public ConditionNode,                                                  \
        public Createable<classname##ConditionNode> {                          \
  public:                                                                      \
    classname##ConditionNode(OperandList operands,                             \
                             std::shared_ptr<LabelNode> true_label,            \
                             std::shared_ptr<LabelNode> false_label)           \
        : ConditionNode(OpCode::opcode, operands, true_label, false_label) {   \
      Assert(operands.size() == numop, "Operand count mismatch");              \
    }                                                                          \
    classname##ConditionNode(OperandList operands)                             \
        : classname##ConditionNode(operands, nullptr, nullptr) {}              \
    void emitInsn(backend::InsnList &list) override;                           \
  };
#include "nodetype.def"

/**
 * @brief 返回节点
 */
class ReturnNode final : public InsnNode, public Createable<ReturnNode> {
private:
  Operand value;

public:
  ReturnNode(std::shared_ptr<sym::Var> value)
      : InsnNode(NodeCode::Return), value(value) {}
  ReturnNode() : ReturnNode(nullptr) {}
  Operand getReturnValue() const { return value; }
  bool returnWithoutValue() const {
    return std::holds_alternative<std::shared_ptr<sym::Var>>(value) &&
           std::get<std::shared_ptr<sym::Var>>(value) == nullptr;
  }
  // int emitInsn(backend::InsnList &list,
  //              std::shared_ptr<sym::Block> scope) override;
  std::ostream &dump(std::ostream &os) const override;
  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    return false;
  }
  bool replaceUsesWith(Operand old_, Operand new_) override {
    Assert(!returnWithoutValue(), "Non value return");
    value = new_;
    return true;
  }
  bool isUse(sym::Var *var) const override {
    return this->getUses().size()!=0 && this->getUses().front().get()==var;
  }

  bool isDef(sym::Var *var) const override { return false; }

  sym::VarList getDefs() const override { return {}; }

  sym::VarList getUses() const override {
    sym::VarList uses;
    if(std::holds_alternative<std::shared_ptr<sym::Var>>(value)) {
      auto val = std::get<std::shared_ptr<sym::Var>>(value);
      if(val) {
        uses.push_back(val);
      }
    }
    return uses;
  }
  void emitInsn(backend::InsnList &list) override;
  ~ReturnNode() override = default;
};

/**
 * @brief 无条件跳转节点
 *
 * 虽然SysY的规范中暂时没有跳转语句，但是仍然存在continue,break等控制流语句
 */
class JumpNode final : public InsnNode, public Createable<JumpNode> {
private:
  std::shared_ptr<LabelNode> label;

public:
  JumpNode(std::shared_ptr<LabelNode> label)
      : InsnNode(NodeCode::Jump), label(label) {}
  std::shared_ptr<LabelNode> getLabel() const { return label; }
  void patchLabel(std::shared_ptr<LabelNode> patch) { label = patch; }
  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    return false;
  }
  bool replaceUsesWith(Operand old_, Operand new_) override { return false; }
  // int emitInsn(backend::InsnList &list,
  //              std::shared_ptr<sym::Block> scope) override;
  std::ostream &dump(std::ostream &os) const override;
  bool isUse(sym::Var *var) const override { return false; }
  bool isDef(sym::Var *var) const override { return false; }
  sym::VarList getDefs() const override { return {}; }
  sym::VarList getUses() const override { return {}; }
  void emitInsn(backend::InsnList &list) override;
  ~JumpNode() override = default;
};

/**
 * @brief 空节点
 */
class NopNode final : public InsnNode, public Createable<NopNode> {
public:
  NopNode() : InsnNode(NodeCode::Nop) {}
  // int emitInsn(backend::InsnList &list,
  //              std::shared_ptr<sym::Block> scope) override;
  std::ostream &dump(std::ostream &os) const override;
  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    return false;
  }
  bool replaceUsesWith(Operand old_, Operand new_) override { return false; }
  bool isUse(sym::Var *var) const override { return false; }
  bool isDef(sym::Var *var) const override { return false; }
  sym::VarList getDefs() const override { return {}; }
  sym::VarList getUses() const override { return {}; }
  void emitInsn(backend::InsnList &list) override{};
  ~NopNode() override = default;
};

/**
 * @brief SSA Phi节点
 *
 * 用于基本块交汇处的值合并，需在控制流图构建后插入
 */
class PhiNode final : public InsnNode, public Createable<PhiNode> {
private:
  std::shared_ptr<sym::Var> dest_var; // 目标变量
  std::unordered_map<BasicBlock *, Operand> operands;

public:
  PhiNode(std::shared_ptr<sym::Var> dest)
      : InsnNode(NodeCode::Phi), dest_var(dest) {}

  // 添加来源变量和对应基本块
  void addOperand(Operand var,
                  std::shared_ptr<BasicBlock> from_bb) {
    operands[from_bb.get()] = var;
  }

  // 获取目标变量
  std::shared_ptr<sym::Var> getDestVar() const { return dest_var; }

  // 获取操作数列表
  const auto &getOperands() const { return operands; }

  bool replaceDefWith(std::shared_ptr<sym::Var> new_def) override {
    dest_var = new_def;
    return true;
  }

  bool replaceUsesWith(Operand old_, Operand new_) override {
    bool replaced = false;

    // 遍历 operands 中的每个操作数
    for(auto &[bb, op] : this->operands) {
      if(op == old_) {
        op = new_;
        replaced = true;
      }
    }

    return replaced;
  }

  bool replaceSrcBlockWith(BasicBlock *old_,BasicBlock *new_){
    bool replaced = false;
    
    // 使用find查找需要修改的条目
    auto it = operands.find(old_);
    if(it != operands.end()) {
      // 保存操作数
      Operand op = it->second;
      // 删除旧条目
      operands.erase(it);
      // 插入新条目
      operands[new_] = op;
      replaced = true;
    }

    return replaced;
  }

  void removeOperandByBasicBlock(BasicBlock *idx){
    this->operands.erase(idx);
  }

  bool isUse(sym::Var *var) const override {
    for (const auto &[bb, var_node] : operands) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(var_node) && std::get<std::shared_ptr<sym::Var>>(var_node).get() == var)
        return true;
    }
    return false;
  }

  bool isDef(sym::Var *var) const override { return dest_var.get() == var; }

  sym::VarList getDefs() const override { return {dest_var}; }

  sym::VarList getUses() const override {
    sym::VarList uses;
    for (const auto &[bb, var_node] : operands) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(var_node)) {
        uses.push_back(std::get<std::shared_ptr<sym::Var>>(var_node));
      }
    }
    return uses;
  }
  Operand getValueBySrcBlock(BasicBlock *idx){
    return this->operands.at(idx);
  }
  std::ostream &dump(std::ostream &os) const override;
  void emitInsn(backend::InsnList &list) override {
    panic("No phinode after eliminatePHI");
  }
};

class LinearizedFunction : public DebugDumpImpl {
private:
  std::weak_ptr<sym::Function> function;
  std::weak_ptr<Module> module;
  sym::VarList decls;
  InsnNodeList nodelist;

public:
  LinearizedFunction(std::shared_ptr<sym::Function> function,
                     std::shared_ptr<Module> module, sym::VarList decls)
      : function(function), module(module), decls(decls), nodelist(){};

  void extend(LinearizedFunction &&other);
  std::shared_ptr<sym::Function> getFunction() const {
    return this->function.lock();
  }
  void appendNode(std::shared_ptr<InsnNode> node) {
    this->nodelist.push_back(node);
  }
  void addVariable(std::shared_ptr<sym::Var> var) { decls.push_back(var); }
  std::shared_ptr<sym::Var>
  generateTempVariable(std::shared_ptr<frontend::Type> type,
                       const std::string &prefix = "t_") {
    auto var = sym::Var::generateTemp(type, prefix);
    this->decls.push_back(var);
    return var;
  }
  std::shared_ptr<InsnNode> front() const { return nodelist.front(); }
  std::shared_ptr<InsnNode> back() const { return nodelist.back(); }
  InsnNodeList::iterator lastNodeIter() {
    auto iter = nodelist.end();
    if (nodelist.empty()) {
      return nodelist.end();
    } else {
      return --iter;
    }
  }
  InsnNodeList::const_iterator lastNodeIter() const {
    auto iter = nodelist.end();
    if (nodelist.empty()) {
      return nodelist.end();
    } else {
      return --iter;
    }
  }
  InsnNodeList::iterator begin() { return nodelist.begin(); }
  InsnNodeList::iterator end() { return nodelist.end(); }
  InsnNodeList::const_iterator begin() const { return nodelist.begin(); }
  InsnNodeList::const_iterator end() const { return nodelist.end(); }
  // 插入节点到指定位置前
  InsnNodeList::iterator insertNodeBefore(InsnNodeList::iterator pos,
                                          std::shared_ptr<InsnNode> node) {
    return nodelist.insert(pos, node);
  }

  // 插入节点到指定位置后
  InsnNodeList::iterator insertNodeAfter(InsnNodeList::iterator pos,
                                         std::shared_ptr<InsnNode> node) {
    return nodelist.insert(std::next(pos), node);
  }

  // 删除指定位置的节点
  InsnNodeList::iterator removeNode(InsnNodeList::iterator pos) {
    return nodelist.erase(pos);
  }

  /**
   * @brief 构建控制流图
   */
  std::shared_ptr<ControlFlowGraph> build_cfg();

  std::ostream &dump(std::ostream &os) const override;
};

class BindNode : public DebugDumpImpl {
protected:
  std::weak_ptr<Module> module;

public:
  BindNode() = default;
  void setModule(std::shared_ptr<Module> module) { this->module = module; }
  std::shared_ptr<Module> getModule() const { return module.lock(); }
  virtual void emitInsn(backend::InsnList &list) = 0;
  virtual ~BindNode() = default;
};

/**
 * @brief 函数定义
 *
 * 该节点与一个函数关联，由局部变量的定义和节点链表构成
 * 所有的节点应当由FunctionNode控制创建和释放。
 * implementation of a function
 */
class FunctionNode final : public BindNode, public Createable<FunctionNode> {
private:
  std::weak_ptr<sym::Function> function;
  sym::VarList decls;
  // std::vector<VarDecl *> escapeds;
  std::map<std::shared_ptr<sym::Var>, VarDecl *> esc2ref;
  sym::VarList escapes;
  InsnNodeList body;
  static inline unsigned int temp_counter = 1;

public:
  FunctionNode(std::shared_ptr<sym::Function> function)
      : BindNode(), function(function), decls(), body() {
    for (auto var : function->getParameters()) {
      decls.push_back(var);
    }
  }
  std::shared_ptr<sym::Function> getFunction() const {
    return this->function.lock();
  }
  void addNode(std::shared_ptr<InsnNode> node) { body.push_back(node); }

  void addEscape(VarDecl *decl, std::shared_ptr<sym::Var> var) {
    esc2ref[var] = decl;
    escapes.push_back(var);
  }
  sym::VarList getEscape() { return escapes; }

  /**
   * @brief 为当前的BindNode生成一个临时变量
   *
   * @param 前缀，同`LabelNode::generateTempLabel`中的前缀。
   */
  std::shared_ptr<sym::Var>
  generateTempVariable(std::shared_ptr<frontend::Type> type,
                       const std::string &prefix = "t_") {
    auto var = std::make_shared<sym::Var>(
        type, prefix + std::to_string(temp_counter++));
    decls.push_back(var);
    return var;
  }
  std::shared_ptr<sym::Var> createVariable(std::shared_ptr<frontend::Type> type,
                                           std::string var_name) {
    auto var = std::make_shared<sym::Var>(type, var_name);
    decls.push_back(var);
    return var;
  }

  /**
   * @brief 线性化绑定节点
   *
   * 完成以下操作：
   * -
   * 统一出口点，删除所有的ReturnNode，将这些ReturnNode转换为把返回值赋值给scope->containing_func->return_def然后跳转到程序的末尾。
   */
  std::unique_ptr<LinearizedFunction> linearize() const;
  // int emitInsn(backend::InsnList &list,
  //              std::shared_ptr<sym::Block> scope) override {
  //   Assert(0, "BindNode should be elimiated.");
  // };
  std::ostream &dump(std::ostream &os) const override;
  void emitInsn(backend::InsnList &list) override;
  ~FunctionNode() override = default;
};

/**
 * @brief 创建全局变量与值的绑定
 */
class InitializeNode : public BindNode, public Createable<InitializeNode> {
private:
  std::shared_ptr<sym::Var> var;
  std::vector<int> indexes;
  std::variant<int, float> value;

public:
  InitializeNode(std::shared_ptr<sym::Var> var, std::vector<int> indexes,
                 std::variant<int, float> value)
      : var(var), indexes(std::move(indexes)), value(value) {
    Assert(var->isGlobal(),
           "Non global variable cannot be binded using InitializeNode");
  }
  bool isDef(sym::Var *var) const { return var == this->var.get(); }
  bool isUse(sym::Var *var) const { return false; }
  sym::VarList getDefs() const { return {var}; }
  sym::VarList getUses() const { return sym::VarList{}; }
  std::shared_ptr<sym::Var> getVar() const { return var; }
  std::variant<int, float> getValue() const { return value; }
  std::vector<int> getIndexes() const { return indexes; }
  std::ostream &dump(std::ostream &os) const override;
  void emitInsn(backend::InsnList &list) override;
  ~InitializeNode() override = default;
};

/**
 * @brief 模块，当前我们可以简单的认为是一个文件的全局作用域
 *
 * 它持有一个文件的全局变量以及声明的函数，函数以BindNode的形式存储
 */
class Module : public std::enable_shared_from_this<Module>,
               public Createable<Module>,
               public DebugDumpImpl {
private:
  sym::VarList global_vars;
  BindNodeList bindings;
  std::unordered_map<FunctionNode *, std::shared_ptr<ControlFlowGraph>> cfg_map;

public:
  using Pass = std::function<void(ControlFlowGraph *)>;
  Module() = default;
  ~Module() = default;
  std::shared_ptr<sym::Var>
  createGlobalVar(std::shared_ptr<frontend::Type> type,
                  const std::string &var_name) {
    auto var = sym::Var::create(type, var_name);
    global_vars.push_back(var);
    var->prop<sym::BasicInfo>().is_global = true;
    return var;
  }

  void addBinding(std::shared_ptr<BindNode> bind) {
    bind->setModule(shared_from_this());
    bindings.push_back(bind);
  }
  void buildCFG() {
    for (auto bind : bindings) {
      if (auto function = std::dynamic_pointer_cast<FunctionNode>(bind)) {
        this->cfg_map[function.get()] = function->linearize()->build_cfg();
      }
    }
  }
  void executePass(Pass pass) {
    for (auto iter = this->cfg_map.begin(); iter != this->cfg_map.end();
         ++iter) {
      pass(iter->second.get());
    }
  }

  std::ostream &dump(std::ostream &os) const override;
  backend::InsnList generateInsnList();
};

} // namespace frontend
} // namespace aaac

// 无法添加dump的话我们直接重载<<运算符吧（）
std::ostream &operator<<(std::ostream &os, const aaac::frontend::Operand &o);

#endif // AAAC_FRONTEND_H
