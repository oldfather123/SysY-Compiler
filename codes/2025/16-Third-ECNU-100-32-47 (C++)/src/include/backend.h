/**
 * @file backend.h
 * 该头文件定义了后端的抽象数据结构
 */
#ifndef AAAC_BACKEND_H
#define AAAC_BACKEND_H

#include <memory>
#include <set>
#include <bitset>
#pragma once

#include "common.h"
#include "config.h"
#include "riscv.h"
#include "sym.h"
#include "frontend.h"

#include <cassert>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace aaac {
namespace backend {


struct Function;
class BasicBlock;

/**
 * @brief 基本块间连接关系类型枚举
 *
 * 描述基本块之间的连接类型，用于构建控制流图
 */
enum class BBConnectionType {
  NEXT, ///< 顺序执行关系 (fall-through)
  ELSE, ///< 条件分支的else路径
  THEN  ///< 条件分支的then路径
};

enum class OpCode : uint16_t {
  // =====================
  // Intermediate Use
  // =====================
  /** Generic placeholder for temporary use */
  Generic,

  // =====================
  // SSA Operations
  // =====================
  /** SSA Phi node for control flow merging */
  Phi,
  /** Unwound Phi node (address_of + store semantic) */
  UnwoundPhi,

  // =====================
  // Function Operations
  // =====================
  /** Function entry point definition */
  Define,
  /** Argument preparation for function calls */
  Push,
  /** Direct function invocation */
  Call,
  /** Indirect call via function pointer */
  Indirect,
  /** Explicit value return from function */
  Return,

  // =====================
  // Memory Operations
  // =====================
  /** Stack space allocation */
  Alloca,
  /** Value assignment operation */
  Assign,
  /** Constant value loading */
  LoadConstant,
  /** Data section constant address resolution */
  LoadDataAddress,
  /** Label address resolution (!!!src0 = 1, src1 = 1, src2 = named Label!!!) */
  LoadLabelAddress,
  /** Store to address marked by a Label (!!!src0 = 1, src1 = 1, src2 = named Label!!!) */
  // StoreLabelAddress,
  /** Escape Var address resolution (!!!src0 = 1, src1 = 1, src2 = var with fp_offset!!!) */
  LoadEscapeAddress,

  // =====================
  // Control Flow Graph
  // =====================
  /** Basic block label marker */
  Label,
  /** Conditional branch/jump */
  Branch,
  /** Unconditional jump */
  Jump,
  /** Function return value handling */
  FuncRet,
  /** Code block scope start */
  BlockStart,
  /** Code block scope end */
  BlockEnd,

  // =====================
  // Function Pointers
  // =====================
  /** Function entry address resolution */
  AddressOfFunc,
  /** Function pointer loading */
  LoadFunc,
  /** Global function pointer loading */
  GlobalLoadFunc,

  // =====================
  // Memory Addressing
  // =====================
  /** Variable address resolution (based on fp and RegAllocInfo's fp_offset) */
  AddressOf,
  /** Global variable address resolution */
  GlobalAddressOf,
  /** Stack value loading (based on fp) */
  Load,
  /** Stack value loading (based on sp) */
  Load_sp,
  /** Global value loading */
  GlobalLoad,
  /** Stack value storage (based on fp) */
  Store,
  /** Stack value storage (based on sp) */
  Store_sp,
  /** Global value storage */
  GlobalStore,
  /** Memory read operation */
  Read,
  /** Memory write operation */
  Write,

  // =====================
  // Arithmetic Operators
  // =====================
  Add32,     // Arithmetic addition
  Add64,     // Arithmetic addition
  Sub,     // Arithmetic subtraction
  Mul32,     // Arithmetic multiplication
  Mul64,     // Arithmetic multiplication
  Div,     // Signed integer division
  FloatAdd,
  FloatSub,
  FloatMul,
  FloatDiv,
  Mod,     // Modulo operation
  Ternary, // Conditional operator ?:
  LShift,  // Bitwise left shift
  RShift,  // Bitwise right shift

  // =====================
  // Logical Operators
  // =====================
  LogAnd, // Logical AND
  LogOr,  // Logical OR
  LogNot, // Logical NOT

  // =====================
  // Comparison Operators
  // =====================
  Eq,  // Equality comparison
  Neq, // Inequality comparison
  Lt,  // Signed less than
  Leq, // Signed less than or equal
  Gt,  // Signed greater than
  Geq, // Signed greater than or equal

  // =====================
  // Bitwise Operators
  // =====================
  BitOr,  // Bitwise OR
  BitAnd, // Bitwise AND
  BitXor, // Bitwise XOR
  BitNot, // Bitwise complement

  // =====================
  // Unary Operators
  // =====================
  Negate, // Arithmetic negation
  FloatNegate, // Float negation
  // =====================
  // System Operations
  // =====================
  /** State machine entry point */
  Start,
  // =====================
  // Cast Operations
  // =====================
  Int2Float,
  Float2Int,
  // =====================
  // Nop Operations
  // =====================
  Nop,
};
enum GlobalVarType {INT_T,INT_LIST,FLOAT_T,FLOAT_LIST};
class GlobalVarValue {
private:
   std::string name;
   GlobalVarType type;
   std::variant<int,std::vector<int>,float,std::vector<float>> value;
public:
    GlobalVarValue(std::string n,int v):name(n),type(INT_T),value(v){}
    GlobalVarValue(std::string n,std::vector<int> v):name(n),type(INT_LIST),value(v){}
    GlobalVarValue(std::string n,float v):name(n),type(FLOAT_T),value(v){}
    GlobalVarValue(std::string n,std::vector<float> v):name(n),type(FLOAT_LIST),value(v){}
    template<typename T> T get()const{return std::get<T>(value);}
    GlobalVarType get_type()const{return type;}
    const std::string& get_name()const{return name;}
};
/**
 * @brief 基本块连接关系描述类
 *
 * 封装基本块之间的前驱/后继关系，用于构建控制流图
 */
class BBConnection {
private:
  friend class BasicBlock;
  std::weak_ptr<BasicBlock> bb = {};              ///< 连接的目标基本块
  BBConnectionType type = BBConnectionType::NEXT; ///< 连接类型
public:
  BBConnection(std::shared_ptr<BasicBlock> bb, BBConnectionType type)
      : bb(bb), type(type) {}
  std::shared_ptr<BasicBlock> getParent()const{
      return bb.lock();
  }
  BBConnectionType getType()const{
      return type;
  }
};


/**
 * @brief 所有指令的基类
 *
 */
class Insn {
protected:
  friend class BasicBlock;
  OpCode opcode;                     ///< 指令操作码
  std::weak_ptr<BasicBlock> belongs; ///< 所属基本块
  bool useful = false;               ///< @todo: Used in DCE pass, strip it
public:
  Insn(OpCode op) : opcode(op), belongs({}){};
  Insn() : Insn(OpCode::Generic){};
  Insn(const Insn &) = default;
  virtual ~Insn() = default;
  OpCode getOpCode() const { return this->opcode; };
  std::weak_ptr<BasicBlock> getParent() const { return this->belongs; }

  // phase2ir.cc && codegen.cc
  virtual std::shared_ptr<sym::Var> getDest() const { return nullptr; }
  virtual frontend::Operand getSrc0() const { return {}; }
  virtual frontend::Operand getSrc1() const { return {}; }
  virtual frontend::Operand getSrcPseudo() const { return {}; }
};

DEF_LIST(Insn)
using InsnNode = InsnList::iterator;
using ConstInsnNode = InsnList::const_iterator;

/**
 * @brief 支配关系属性
 *
 * 用于记录基本块在控制流图中的支配关系信息
 */
struct Dominator : utils::BaseProperty {
  std::weak_ptr<BasicBlock> idom = {};        ///< Immediate dominator
  std::weak_ptr<BasicBlock> r_idom = {};      ///< Reverse immediate dominator
  std::weak_ptr<BasicBlock> dom_next[64]{};   ///< 支配树后继节点
  std::weak_ptr<BasicBlock> dom_prev = {};    ///< 支配树前驱节点
  std::weak_ptr<BasicBlock> rdom_next[256]{}; ///< 后支配树后继节点
  std::weak_ptr<BasicBlock> rdom_prev = {};   ///< 后支配树前驱节点
};

/**
 * @brief 活跃变量分析属性
 *
 * 记录基本块级别的变量活跃性信息
 */
struct Liveness : utils::BaseProperty {
  std::shared_ptr<sym::Var> live_gen[config::MAX_ANALYSIS_STACK];
  int live_gen_idx = 0;
  std::shared_ptr<sym::Var> live_kill[config::MAX_ANALYSIS_STACK];
  int live_kill_idx = 0;
  std::shared_ptr<sym::Var> live_in[config::MAX_ANALYSIS_STACK];
  int live_in_idx = 0;
  std::shared_ptr<sym::Var> live_out[config::MAX_ANALYSIS_STACK];
  int live_out_idx = 0;
};

/**
 * @brief 逆序遍历属性
 *
 * 用于记录逆后序遍历时的顺序信息
 */
struct ReverseOrder : utils::BaseProperty {
  std::weak_ptr<BasicBlock> rpo_next = {};
  std::weak_ptr<BasicBlock> r_rpo_next = {};
  int rpo = -1;
  int rpo_r = -1;
};

struct BasicBlockInfo : utils::BaseProperty {
  Function *belongs = nullptr;
  std::shared_ptr<InsnList> insn_list = nullptr;

  void setInsnList(std::shared_ptr<InsnList> new_insn_list) {
    insn_list = new_insn_list;
  }

  InsnList getInsnList() const {
    return insn_list ? *insn_list : InsnList();
  }
};

/**
 * @brief 定义了程序中的一个基本块
 * @todo 进一步完善设计
 *
 */
class BasicBlock :public std::enable_shared_from_this<BasicBlock>{
private:
  std::vector<BBConnection> prev;

  std::weak_ptr<BasicBlock> next_block = {};
  std::weak_ptr<BasicBlock> then_block = {};
  std::weak_ptr<BasicBlock> else_block = {};

  static inline std::unordered_set<std::type_index> allowed_types_ = {
      std::type_index(typeid(Dominator)), std::type_index(typeid(ReverseOrder)),
      std::type_index(typeid(Liveness)),
      std::type_index(typeid(BasicBlockInfo))};

public:
  BasicBlock() : prev(), next_block(), then_block(), else_block() {
  prev.reserve(config::MAX_BB_PRED);
}
  std::shared_ptr<BasicBlock> getNext() { return next_block.lock(); }
  std::shared_ptr<BasicBlock> getThen() { return then_block.lock(); }
  std::shared_ptr<BasicBlock> getElse() { return else_block.lock(); }
  void setInsnList(InsnList insn_list){
    prop<BasicBlockInfo>().insn_list = std::make_shared<InsnList>(insn_list);
  }
  void setNext(std::shared_ptr<BasicBlock> next) {
    next_block = next;
    next->prev.push_back(BBConnection(shared_from_this(),BBConnectionType::NEXT));
   }
   void setThen(std::shared_ptr<BasicBlock> then) {
    then_block = then;
    then->prev.push_back(BBConnection(shared_from_this(),BBConnectionType::THEN));
   }

   void setElse(std::shared_ptr<BasicBlock> else_) {
    else_block = else_;
    else_->prev.push_back(BBConnection(shared_from_this(),BBConnectionType::ELSE));
   }
   InsnList getInsnList()const{return prop<BasicBlockInfo>()->insn_list ? *prop<BasicBlockInfo>()->insn_list : InsnList();}
   // 在测试中可以用这个函数判断是否显式地在每个BasicBlock开头都加上了Label
   std::shared_ptr<Insn> getFirstInsn()const{return prop<BasicBlockInfo>()->insn_list->front();}
   std::shared_ptr<Insn> getLastInsn()const{return prop<BasicBlockInfo>()->insn_list->back();}

  template <typename T> static bool is_allowed() {
    return allowed_types_.find(std::type_index(typeid(T))) !=
           allowed_types_.end();
  }

  template <typename T> T &prop() {
    if (!is_allowed<T>()) {
#ifdef DEBUG_MODE
      LOG_INFO("Illegal property read caught\n")
#endif
      throw std::invalid_argument("Type not allowed for BasicBlock property");
    }
    return utils::g_property_mgr.init<T>(this);
  }

  template <typename T> const T *prop() const {
    if (!is_allowed<T>()) {
#ifdef DEBUG_MODE
      LOG_INFO("Illegal property read caught\n")
#endif
      throw std::invalid_argument("Type not allowed for BasicBlock property");
    }
    return utils::g_property_mgr.get<T>(this);
  }


  ~BasicBlock() {
    try {
      utils::g_property_mgr.remove_all(this);
    } catch (const std::exception &e) {
      LOG_ERROR("Property cleanup failed: %s", e.what())
    } catch (...) {
      LOG_ERROR("Exception occurred during BasicBlock destruction")
#ifdef DEBUG_MODE
      panic("This should never happen")
#endif
    }
  }
};

/**
 * @brief 函数体的定义
 *
 * 函数体应当与符号表中的函数一一对应，它主要维护了函数体中的基本块列表，
 *
 * @todo 将优化相关的字段分离出来
 */
 struct Function {
  std::string name;
  std::shared_ptr<BasicBlock> bbs = nullptr;
  std::shared_ptr<BasicBlock> exit_block = nullptr;
  sym::VarList var_list; ///< 变量列表
  sym::VarList escapes;
  int bb_cnt;
  int visited;
  std::weak_ptr<sym::Function> func = {};
  Function *next = nullptr;
  int stack_size = 0; ///< Extra stack space needed for spills
  std::bitset<REG_COUNT> reg_used;
  bool uses(RISCV_Reg r) const {
    return reg_used.test(static_cast<size_t>(r));
  }
  bool empty()const{
    if(!bbs)
      return true;
    else
      return false;
  }
  void setStartBasicBlock(std::shared_ptr<BasicBlock> bb){
    this->bbs = bb;
  }
  void setExitBasicBlock(std::shared_ptr<BasicBlock> bb){
    this->exit_block = bb;
  }
  std::shared_ptr<BasicBlock> getStartBasicBlock() const {
    return this->bbs;
  }
  std::shared_ptr<BasicBlock> getExitBasicBlock() const {
    return this->exit_block;
  }

  void addStackSize(int sz) { stack_size += sz; }
  int getStackSize() const { return stack_size; }
  void setStackSize(int sz) { stack_size = sz; }

  void addEscape(std::shared_ptr<sym::Var> esc) { escapes.push_back(esc); }
  sym::VarList getEscape() { return escapes; }
};

/**
 * @brief 特殊的指令，指示函数入口
 */
class StartInsn final : public Insn, public Createable<StartInsn> {
public:
  StartInsn() : Insn(OpCode::Start){};
};

/**
 * @brief 常量加载指令
 *
 * 有两个变体：加载int值和加载浮点值
 */
class ConstantLoadInsn : public Insn,public Createable<ConstantLoadInsn>  {
protected:
  std::shared_ptr<sym::Var> rd;
  frontend::Operand operand;

public:
  ConstantLoadInsn(std::shared_ptr<sym::Var> rd, int c)
      : Insn(OpCode::LoadConstant), rd(rd), operand(c) {}
  ConstantLoadInsn(std::shared_ptr<sym::Var> rd, float c)
      : Insn(OpCode::LoadConstant), rd(rd), operand(c) {}
  std::shared_ptr<sym::Var> getDestination() const { return rd; }

  bool isInt() const { return std::holds_alternative<int>(operand); }
  int  getInt()  const {
      if (!isInt()) throw std::logic_error("Not an int constant");
      return std::get<int>(operand);
  }
  bool isFloat() const { return std::holds_alternative<float>(operand); }
  float getFloat()const {
    if (!isFloat()) throw std::logic_error("Not a float constant");
    return std::get<float>(operand);
  }
  const frontend::Operand &getOperand() const { return operand; }
};


/**
 * @brief 通用格式指令
 *
 * 这里，我将可以用三地址的格式来表示的指令称为通用指令，它包含了绝大多数类型的指令
 */
class CommonInsn : public Insn,
                    public Createable<CommonInsn> {
protected:
  int idx; ///< @todo: Used in reg-alloc pass, strip it
  std::shared_ptr<sym::Var> rd = nullptr;
  frontend::OperandList operands; ///< { rs1, rs2, rs3(pseudo) }

public:
  CommonInsn(OpCode op, std::shared_ptr<sym::Var> rd, frontend::Operand rs1, frontend::Operand rs2, frontend::Operand pseudo = nullptr)
      : Insn(op), idx(0), rd(rd), operands{rs1,rs2, pseudo}{};

  CommonInsn(OpCode op)
      : CommonInsn(op, std::shared_ptr<sym::Var>{}, frontend::Operand{}, frontend::Operand{}) {}

  std::shared_ptr<sym::Var> getDest() const override { return this->rd; }

  frontend::Operand getSrc0() const override {
      if (operands.empty()) return frontend::Operand{};
      auto it = operands.begin();
      return *it;
  }

  frontend::Operand getSrc1() const override {
      if (operands.size() < 2) return frontend::Operand{};
      auto it = std::next(operands.begin(), 1);
      return *it;
  }

  frontend::Operand getSrcPseudo() const override {
      if (operands.size() < 3) return frontend::Operand{};
      auto it = std::next(operands.begin(), 2);
      return *it;
  }

  OpCode getOpcode() const { return opcode; }
  std::shared_ptr<sym::Var> getDestination() const { return this->rd; }
  std::shared_ptr<sym::Var> setDestination(std::shared_ptr<sym::Var> new_rd) {
    return this->rd = new_rd;
  }

  std::shared_ptr<sym::Var> getSrcVar(int i) const {
    auto it = std::next(operands.begin(), i);
    if (!std::holds_alternative<std::shared_ptr<sym::Var>>(*it))
      throw std::logic_error("Operand " + std::to_string(i) + " is not a Var");
    return std::get<std::shared_ptr<sym::Var>>(*it);
  }

  int getSrcInt(int i) const {
    auto it = std::next(operands.begin(), i);
    if (!std::holds_alternative<int>(*it))
      throw std::logic_error("Operand " + std::to_string(i) + " is not an int");
    return std::get<int>(*it);
  }

  float getSrcFloat(int i) const {
    auto it = std::next(operands.begin(), i);
    if (!std::holds_alternative<float>(*it))
      throw std::logic_error("Operand " + std::to_string(i) + " is not a float");
    return std::get<float>(*it);
  }

  frontend::Operand setSrc(int i, frontend::Operand operand) {
    return operands[i] = operand;
  }
};


/**
 * @brief 含标签指令
 *
 * 某些指令类型是含有标签的，如call
 * jump等，由于他们不会用到寄存器，因此只需要为他们分配标签名
 */
class LabelledInsn : public Insn, public Createable<LabelledInsn> {
private:
  std::string name;

public:
  LabelledInsn(OpCode op, std::string name) : Insn(op), name(name){};
  LabelledInsn(OpCode op) : LabelledInsn(op, ""){};
  std::string getLabel() const { return this->name; }
};

/**
 * @brief 条件跳转指令
 *
 * 它包含一个条件值和两个分支，我们既不能把它放在CommonInsn中也不能放在LabelledInsn中
 * 因此我们创建一个新的类型
 */
class BranchInsn : public Insn, public Createable<BranchInsn> {
private:
std::shared_ptr<sym::Var> cond;
  std::string true_label;
  std::string false_label;

public:
  BranchInsn(std::shared_ptr<sym::Var> cond,const std::string &true_label, const std::string &false_label)
      : Insn(OpCode::Branch), cond(cond),true_label(true_label), false_label(false_label) {
  }
  std::string getTrueLabel() const { return true_label; }
  void setTrueLabel(std::string label) { true_label = label; }
  std::string getFalseLabel() const { return false_label; }
  void setFalseLabel(std::string label) { false_label = label; }
  std::shared_ptr<sym::Var> getCond() const { return cond; }
  void setCond(std::shared_ptr<sym::Var> cond_) { cond = cond_; }
};

/**
 * @brief Phi指令
 *
 * SSA表示中的Phi指令，用于基本块间变量的传递
 *
 * @todo 或许要把它移动到frontend.h变成PhiNode?
 */
class PhiInsn final : public Insn, public Createable<PhiInsn> {
private:
  struct PhiOperand {
    std::shared_ptr<sym::Var> var = nullptr;
    std::weak_ptr<BasicBlock> from;
    RISCV_Reg reg = RISCV_Reg::INVALID;
    PhiOperand(std::shared_ptr<sym::Var> var, std::shared_ptr<BasicBlock> from,
               RISCV_Reg reg)
        : var(var), from(from), reg(reg) {}
    PhiOperand(std::shared_ptr<sym::Var> var, std::shared_ptr<BasicBlock> from)
        : PhiOperand(var, from, RISCV_Reg::__zero) {}
  };
  DEF_LIST(PhiOperand)
  PhiOperandList phi_operands;
  std::shared_ptr<sym::Var> destination = nullptr; // 目标变量
  RISCV_Reg dest_reg = RISCV_Reg::INVALID;         // 分配寄存器

public:
  PhiInsn() : Insn(OpCode::Phi){};
  ~PhiInsn() = default;

  /**
   * @brief 向Phi指令中添加一个PhiOperand
   *
   * @param var 添加的变量
   * @param from 这个变量的来源基本块
   */
  void appendOperand(std::shared_ptr<sym::Var> var,
                     std::shared_ptr<BasicBlock> from);

  /**
   * @brief 设置Phi指令的目标变量
   *
   * @param var 目标变量
   */
  void setDestination(std::shared_ptr<sym::Var> var) { destination = var; }

  /**
   * @brief 获取Phi指令的目标变量
   *
   * @return 目标变量
   */
  std::shared_ptr<sym::Var> getDestination() const { return destination; }

  /**
   * @brief 设置目标变量的寄存器
   *
   * @param reg 分配的寄存器
   */
  void setDestinationReg(RISCV_Reg reg) { dest_reg = reg; }

  /**
   * @brief 获取目标变量的寄存器
   *
   * @return 分配的寄存器
   */
  RISCV_Reg getDestinationReg() const { return dest_reg; }

  /**
   * @brief 获取源操作数的数量
   *
   * @return 源操作数数量
   */
  int getSourceCount() const { return phi_operands.size(); }

  /**
   * @brief 获取指定索引的源操作数变量
   *
   * @param idx 操作数索引
   * @return 源操作数变量，如果索引无效则返回nullptr
   */
  std::shared_ptr<sym::Var> getSource(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(phi_operands.size())) {
      return nullptr;
    }

    auto it = phi_operands.begin();
    std::advance(it, idx);
    return (*it)->var;
  }

  /**
   * @brief 获取指定索引的源操作数来源基本块
   *
   * @param idx 操作数索引
   * @return 源操作数来源基本块，如果索引无效则返回nullptr
   */
  std::weak_ptr<BasicBlock> getSourceBlock(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(phi_operands.size())) {
      return {};
    }

    auto it = phi_operands.begin();
    std::advance(it, idx);
    return (*it)->from;
  }

  /**
   * @brief 设置指定索引的源操作数寄存器
   *
   * @param idx 操作数索引
   * @param reg 分配的寄存器
   */
  void setSourceReg(int idx, RISCV_Reg reg) {
    if (idx < 0 || idx >= static_cast<int>(phi_operands.size())) {
      return;
    }

    auto it = phi_operands.begin();
    std::advance(it, idx);
    (*it)->reg = reg;
  }

  /**
   * @brief 获取指定索引的源操作数寄存器
   *
   * @param idx 操作数索引
   * @return 分配的寄存器，如果索引无效则返回 INVALID
   */
  RISCV_Reg getSourceReg(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(phi_operands.size())) {
      return RISCV_Reg::INVALID;
    }

    auto it = phi_operands.begin();
    std::advance(it, idx);
    return (*it)->reg;
  }

  /**
   * @brief 获取所有Phi操作数
   *
   * @return Phi操作数列表的引用
   */
  const PhiOperandList &getOperands() const { return phi_operands; }
};

void peephole();
void codegen(std::ostream &ofs);
void codegen(const std::string &filename);
void codegen();

} // namespace backend
} // namespace aaac

#endif // AAAC_BACKEND_H
