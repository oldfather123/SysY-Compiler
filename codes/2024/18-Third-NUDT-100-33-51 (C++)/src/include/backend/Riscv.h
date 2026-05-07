#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "../utils/ConstantCounter.h"

/**
 * @file RISCV.h
 *
 * @brief 定义RISC-V汇编相关的类型与操作的头文件
 */

namespace riscv {
/**
 * @brief 标签(表示特定地址)
 *
 */
class Label {
 protected:
  std::string name;
  uint64_t address;

 protected:
  Label() = default;
  explicit Label(std::string name) : name(std::move(name)) {}
  virtual ~Label() = default;

 public:
  auto getAddr() const -> uint64_t { return address; }
  auto getName() const -> const std::string & { return name; }
  void setName(std::string name) { this->name = std::move(name); }
  void setAddr(uint64_t address) { this->address = address; }
};

/**
 * @brief 定义了全局区数据
 *
 */
class Global : public Label {
 private:
  bool isInt;                  ///< 全局数据类型是否为int
  std::vector<unsigned> dims;  ///< 全局数据的维度
  ConstantCounter init;        ///< 初始化值

 public:
  Global(bool isInt, std::vector<unsigned> dims, ConstantCounter init, const std::string &name)
      : Label(name), isInt(isInt), dims(std::move(dims)), init(std::move(init)) {}

 public:
  auto isIntType() const -> bool { return isInt; }
  auto getDims() const -> const std::vector<unsigned> & { return dims; }
  auto getInit() const -> const ConstantCounter & { return init; }
};

class Instruction;

/**
 * @brief 寄存器类型的基类
 *
 */
class Register {
 protected:
  Register() = default;
  virtual ~Register() = default;
};
class PhyRegister : virtual public Register {};
class SymRegister : virtual public Register {
 private:
  std::string name;

 protected:
  explicit SymRegister(std::string name) : name(std::move(name)) {}

 public:
  auto getName() const -> const std::string & { return name; }
};
class IntRegister : virtual public Register {};
class FloatRegister : virtual public Register {};

/**
 * @brief 定义了整型寄存器类型
 *
 */
class IntPhyRegister : public PhyRegister, public IntRegister {
 public:
  /**
   * @brief 定义了risc-v使用的基本寄存器种类
   *
   */
  enum IntRegisterKind {
    zero,
    ra,
    sp,
    gp,
    tp,
    t0,
    t1,
    t2,
    fp,
    s1,
    a0,
    a1,
    a2,
    a3,
    a4,
    a5,
    a6,
    a7,
    s2,
    s3,
    s4,
    s5,
    s6,
    s7,
    s8,
    s9,
    s10,
    s11,
    t3,
    t4,
    t5,
    t6,
  };

 private:
  IntRegisterKind reg;

 private:
  explicit IntPhyRegister(IntRegisterKind reg) : reg(reg) {}

 public:
  static auto getIntPhyRegister(IntRegisterKind reg) -> IntPhyRegister *;
  auto getIntPhyRegisterKind() const -> IntRegisterKind { return reg; }
};

/**
 * @brief 定义了物理浮点型寄存器类型
 *
 */
class FloatPhyRegister : public PhyRegister, public FloatRegister {
 public:
  /**
   * @brief 定义了risc-v使用的基本寄存器种类
   *
   */
  enum FloatRegisterKind {
    ft0,
    ft1,
    ft2,
    ft3,
    ft4,
    ft5,
    ft6,
    ft7,
    fs0,
    fs1,
    fa0,
    fa1,
    fa2,
    fa3,
    fa4,
    fa5,
    fa6,
    fa7,
    fs2,
    fs3,
    fs4,
    fs5,
    fs6,
    fs7,
    fs8,
    fs9,
    fs10,
    fs11,
    ft8,
    ft9,
    ft10,
    ft11
  };

 private:
  FloatRegisterKind reg;

 private:
  explicit FloatPhyRegister(FloatRegisterKind reg) : reg(reg) {}

 public:
  static auto getFloatPhyRegister(FloatRegisterKind reg) -> FloatPhyRegister *;
  auto getFloatRegisterKind() const -> FloatRegisterKind { return reg; }
};

/**
 * @brief 符号整型寄存器
 *
 */
class IntSymRegister : public SymRegister, public IntRegister {
 private:
  bool is64;

 public:
  explicit IntSymRegister(bool is64, std::string name = "") : SymRegister(std::move(name)), is64(is64) {}
  auto is64Value() const -> bool { return is64; }
};
/**
 * @brief 符号浮点型寄存器
 *
 */
class FloatSymRegister : public SymRegister, public FloatRegister {
 public:
  explicit FloatSymRegister(std::string name = "") : SymRegister(std::move(name)) {}
};

class BasicBlock;

/**
 * @brief 基本汇编指令
 *
 */
class Instruction {
 public:
  enum InstKind {
    // shifts
    kSll,
    kSlli,
    kSrl,
    kSrli,
    kSra,
    kSrai,
    kSllw,
    kSlliw,
    kSrlw,
    kSrliw,
    kSraw,
    kSraiw,
    // arithmetic
    kAdd,
    kAddi,
    kSub,
    kAddw,
    kAddiw,
    kSubw,
    kLui,
    kAuipc,
    // logical
    kXor,
    kXori,
    kOr,
    kOri,
    kAnd,
    kAndi,
    // compare
    kSlt,
    kSlti,
    kSltu,
    kSltiu,
    // branches
    kBeq,
    kBne,
    kBlt,
    kBge,
    kBltu,
    kBgeu,
    // jump&links
    kJal,
    kJalr,
    // loads
    kLb,
    kLh,
    kLbu,
    kLhu,
    kLw,
    kLwu,
    kLd,
    // stores
    kSb,
    kSh,
    kSw,
    kSd,
    // multiply
    kMul,
    kMulh,
    kMulhsu,
    kMulhu,
    kMulw,
    // divs
    kDiv,
    kDivu,
    kDivw,
    // remainder
    kRem,
    kRemu,
    kRemw,
    kRemuw,

    // float & double
    // move
    kFmv_w_x,
    kFmv_x_w,
    kFmv_d_x,
    kFmv_x_d,
    // convert
    kFcvt_s_w,
    kFcvt_s_wu,
    kFcvt_w_s,
    kFcvt_wu_s,
    kFcvt_s_l,
    kFcvt_s_lu,
    kFcvt_l_s,
    kFcvt_lu_s,
    // load
    kFlw,
    // store
    kFsw,
    // arithmetic
    kFadd_s,
    kFsub_s,
    kFmul_s,
    kFdiv_s,
    // mul-add
    kFmadd_s,
    kFmsub_s,
    kFnmsub_s,
    kFnmadd_s,
    // sign injection
    kFsgnj_s,
    kFsgnjn_s,
    kFsgnjx_s,
    // min&max
    kFmin_s,
    kFmax_s,
    // compare
    kFeq_s,
    kFlt_s,
    kFle_s,
    // categorize
    kFclass_s,

    // atomic instruction
    kAmoMin,
    kAmoMax,

    // 伪指令
    kLla,
    kLi,
    kSext_w,
    kCall
  };

 private:
  BasicBlock *parent;               ///< 父基本块
  InstKind kind;                    ///< 指令类别
  std::vector<Register *> sources;  ///< 源操作数
  Register *destination{};          ///< 目的操作数

 public:
  bool ifdelete = false;

 protected:
  explicit Instruction(InstKind kind, BasicBlock *parent) : parent(parent), kind(kind) {}

 public:
  virtual ~Instruction() = default;

 public:
  auto getParent() const -> BasicBlock * { return parent; }
  auto getKind() const -> InstKind { return kind; }
  auto getSource(unsigned index) const -> Register * { return sources.at(index); }
  auto getSources() const -> const std::vector<Register *> & { return sources; }
  auto getDestination() const -> Register * { return destination; }
  auto getNumSources() const -> unsigned int { return sources.size(); }
  void addSource(Register *source) { sources.emplace_back(source); }
  void setDestination(Register *inputDestination) { destination = inputDestination; }
  void setSource(unsigned index, Register *reg) { sources[index] = reg; }
  auto isRInst() const -> bool {
    return kind == kSll || kind == kSllw || kind == kSrl || kind == kSrlw || kind == kSra || kind == kSraw ||
           kind == kAdd || kind == kAddw || kind == kSub || kind == kSubw || kind == kXor || kind == kOr ||
           kind == kAnd || kind == kSlt || kind == kSltu || kind == kMul || kind == kMulh || kind == kMulhsu ||
           kind == kMulhu || kind == kMulw || kind == kDiv || kind == kDivu || kind == kDivw || kind == kRem ||
           kind == kRemu || kind == kRemuw || kind == kRemw;
  }
  auto isIInst() const -> bool {  //< jalr not in
    return kind == kSlli || kind == kSlliw || kind == kSrli || kind == kSrliw || kind == kSrai || kind == kSraiw ||
           kind == kAddi || kind == kAddiw || kind == kXori || kind == kOri || kind == kAndi || kind == kSlti ||
           kind == kSltiu || kind == kLb || kind == kLbu || kind == kLh || kind == kLhu || kind == kLw ||
           kind == kLwu || kind == kLd;
  }
  auto isSInst() const -> bool { return kind == kSb || kind == kSh || kind == kSd || kind == kSw; }
  auto isBInst() const -> bool {
    return kind == kBeq || kind == kBne || kind == kBlt || kind == kBltu || kind == kBge || kind == kBgeu;
  }
  auto isUInst() const -> bool { return kind == kLui || kind == kAuipc; }
  auto isJumpInst() const -> bool { return kind == kJal || kind == kJalr; }
  auto isFRInst() const -> bool {
    return kind == kFadd_s || kind == kFsub_s || kind == kFmul_s || kind == kFdiv_s || kind == kFsgnj_s ||
           kind == kFsgnjn_s || kind == kFsgnjx_s;
  }
  auto isFR2IInst() const -> bool { return kind == kFeq_s || kind == kFle_s || kind == kFlt_s; }
  auto isF2IInst() const -> bool {
    return kind == kFmv_x_w || kind == kFmv_x_d || kind == kFcvt_w_s || kind == kFcvt_wu_s || kind == kFcvt_l_s ||
           kind == kFcvt_lu_s;
  }
  auto isI2FInst() const -> bool {
    return kind == kFmv_w_x || kind == kFmv_d_x || kind == kFcvt_s_l || kind == kFcvt_s_lu || kind == kFcvt_s_w ||
           kind == kFcvt_s_wu;
  }
  auto isFLInst() const -> bool { return kind == kFlw; }
  auto isFSInst() const -> bool { return kind == kFsw; }
  auto isFlongInst() const -> bool {
    return kind == kFmadd_s || kind == kFmsub_s || kind == kFnmadd_s || kind == kFnmsub_s;
  }
  auto isPseudoInst() const -> bool { return isLlaInst() || isLiInst() || isSext_wInst() || isCallInst(); }
  auto isLlaInst() const -> bool { return kind == kLla; }
  auto isLiInst() const -> bool { return kind == kLi; }
  auto isSext_wInst() const -> bool { return kind == kSext_w; }
  auto isCallInst() const -> bool { return kind == kCall; }
  auto isAddrInst() const -> bool {
    return kind == kFlw || kind == kFsw || kind == kSb || kind == kSh || kind == kSd || kind == kSw || kind == kLb ||
           kind == kLbu || kind == kLh || kind == kLhu || kind == kLw || kind == kLwu || kind == kLd;
  }
  auto isLoadInst() const -> bool {
    return kind == kFlw || kind == kLb || kind == kLbu || kind == kLh || kind == kLhu || kind == kLw || kind == kLwu ||
           kind == kLd;
  }
  auto isStoreInst() const -> bool { return kind == kFsw || kind == kSb || kind == kSh || kind == kSd || kind == kSw; }
  auto isPcInst() const -> bool { return isLlaInst() || isCallInst() || isBInst() || isJumpInst(); }

  void setParent(BasicBlock *block) { parent = block; }
};

class RInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  RInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2, InstKind kind, BasicBlock *parent)
      : Instruction(kind, parent) {
    assert(rd);
    assert(rs1);
    assert(rs2);
    addSource(rs1);
    addSource(rs2);
    setDestination(rd);
  }
};

class IInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  int imm;

 protected:
  IInst(IntRegister *rd, IntRegister *rs1, int imm, InstKind kind, BasicBlock *parent)
      : Instruction(kind, parent), imm(imm) {
    assert(rs1);
    assert(rd);
    addSource(rs1);
    setDestination(rd);
  }

 public:
  auto getImm() const -> int { return imm; }
  void setImm(int newImm) {
    assert(newImm >= -2048 && newImm <= 2047);
    imm = newImm;
  }
};

class SInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  int imm;

 protected:
  SInst(IntRegister *rs1, IntRegister *rs2, int imm, InstKind kind, BasicBlock *parent)
      : Instruction(kind, parent), imm(imm) {
    assert(rs1);
    assert(rs2);
    addSource(rs1);
    addSource(rs2);
  }

 public:
  auto getImm() const -> int { return imm; }
};

class BInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  BasicBlock *thenBlock;

 protected:
  BInst(IntRegister *rs1, IntRegister *rs2, BasicBlock *thenBlock, InstKind kind, BasicBlock *parent)
      : Instruction(kind, parent), thenBlock(thenBlock) {
    assert(rs1);
    assert(rs2);
    assert(this->thenBlock);
    addSource(rs1);
    addSource(rs2);
  }

 public:
  auto getThenBlock() const -> BasicBlock * { return thenBlock; }
  void setThenBlock(BasicBlock *newBlock) { thenBlock = newBlock; }
};

class UInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  unsigned imm;

 protected:
  UInst(IntRegister *rd, unsigned imm, InstKind kind, BasicBlock *parent) : Instruction(kind, parent), imm(imm) {
    assert(rd);
    setDestination(rd);
  }

 public:
  auto getImm() const -> unsigned { return imm; }
};

class JInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  Label *label;

 protected:
  JInst(IntRegister *rd, Label *label, InstKind kind, BasicBlock *parent) : Instruction(kind, parent), label(label) {
    assert(rd);
    assert(this->label);
    setDestination(rd);
  }

 public:
  auto getLabel() const -> Label * { return label; }
  void setLabel(Label *newLabel) { label = newLabel; }
};

class FRInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  FRInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2, InstKind kind, BasicBlock *parent)
      : Instruction(kind, parent) {
    assert(rd);
    assert(rs1);
    assert(rs2);
    addSource(rs1);
    addSource(rs2);
    setDestination(rd);
  }
};

class FR2IInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  FR2IInst(IntRegister *rd, FloatRegister *rs1, FloatRegister *rs2, InstKind kind, BasicBlock *parent)
      : Instruction(kind, parent) {
    assert(rd);
    assert(rs1);
    assert(rs2);
    addSource(rs1);
    addSource(rs2);
    setDestination(rd);
  }
};

class F2IInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  F2IInst(IntRegister *rd, FloatRegister *rs1, InstKind kind, BasicBlock *parent) : Instruction(kind, parent) {
    assert(rs1);
    assert(rd);
    addSource(rs1);
    setDestination(rd);
  }
};

class I2FInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  I2FInst(FloatRegister *rd, IntRegister *rs1, InstKind kind, BasicBlock *parent) : Instruction(kind, parent) {
    assert(rd);
    assert(rs1);
    addSource(rs1);
    setDestination(rd);
  }
};

class FLInst : public Instruction {
  friend class RiscvBuilder;

 private:
  int imm;

 protected:
  FLInst(FloatRegister *rd, IntRegister *rs1, int imm, InstKind kind, BasicBlock *parent)
      : Instruction(kind, parent), imm(imm) {
    assert(rd);
    assert(rs1);
    addSource(rs1);
    setDestination(rd);
  }

 public:
  auto getImm() const -> int { return imm; }
};

class FSInst : public Instruction {
  friend class RiscvBuilder;

 private:
  int imm;

 protected:
  FSInst(FloatRegister *rs1, IntRegister *rs2, int imm, InstKind kind, BasicBlock *parent)
      : Instruction(kind, parent), imm(imm) {
    assert(rs1);
    assert(rs2);
    addSource(rs1);
    addSource(rs2);
  }

 public:
  auto getImm() const -> int { return imm; }
};

class FLongInst : public Instruction {
  friend class RiscvBuilder;

 protected:
  FLongInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2, FloatRegister *rs3, InstKind kind,
            BasicBlock *parent)
      : Instruction(kind, parent) {
    assert(rd);
    assert(rs1);
    assert(rs2);
    assert(rs3);
    addSource(rs1);
    addSource(rs2);
    addSource(rs3);
    setDestination(rd);
  }
};

class PseudoInst : public Instruction {
 protected:
  PseudoInst(InstKind kind, BasicBlock *parent) : Instruction(kind, parent) {}
};

// 伪指令
class LlaInst : public PseudoInst {
  friend class RiscvBuilder;

 private:
  Global *symbol;

 protected:
  LlaInst(IntRegister *rd, Global *symbol, BasicBlock *parent) : PseudoInst(InstKind::kLla, parent), symbol(symbol) {
    assert(rd);
    assert(this->symbol);
    setDestination(rd);
  }

 public:
  auto getSymbol() const -> Global * { return symbol; }
};

class LiInst : public PseudoInst {
  friend class RiscvBuilder;

 private:
  uint64_t imm;

 protected:
  LiInst(IntRegister *rd, uint64_t imm, BasicBlock *parent) : PseudoInst(InstKind::kLi, parent), imm(imm) {
    assert(rd);
    setDestination(rd);
  }

 public:
  auto getImm() const -> uint64_t { return imm; }
};

class Sext_wInst : public PseudoInst {
  friend class RiscvBuilder;

 protected:
  Sext_wInst(IntRegister *rd, IntRegister *rs1, BasicBlock *parent) : PseudoInst(InstKind::kSext_w, parent) {
    assert(rd);
    assert(rs1);
    setDestination(rd);
    addSource(rs1);
  }
};

class CallInst : public PseudoInst {
  friend class RiscvBuilder;

 private:
  Label *label;

 protected:
  CallInst(Label *label, BasicBlock *parent) : PseudoInst(InstKind::kCall, parent), label(label) {}

 public:
  auto getLabel() const -> Label * { return label; }
};

class Function;
/**
 * @brief 基本块
 *
 */
class BasicBlock : public Label {
  friend class Function;
  friend class BasicBlock;
  friend class RiscvBuilder;

 public:
  using iterator = std::list<std::unique_ptr<Instruction>>::iterator;

 private:
  Function *parent;                                      ///< 父函数
  std::list<std::unique_ptr<Instruction>> instructions;  ///< 指令序列
  std::set<BasicBlock *> predecessors;                   ///< 前驱块集合
  std::set<BasicBlock *> successors;                     ///< 后继块集合

 private:
  explicit BasicBlock(Function *parent, const std::string &name = "") : Label(name), parent(parent) {}

 public:
  ~BasicBlock() override {
    for (const auto &pred : predecessors) {
      pred->removeSuccessor(this);
    }
    for (const auto &succ : successors) {
      succ->removePredecessor(this);
    }
  }

 public:
  auto getParent() const -> Function * { return parent; }
  auto getInstructions() -> std::list<std::unique_ptr<Instruction>> & { return instructions; }
  auto getPredecessors() const -> const std::set<BasicBlock *> & { return predecessors; }
  auto getSuccessors() const -> const std::set<BasicBlock *> & { return successors; }
  auto getNumInstructions() const -> unsigned int { return instructions.size(); }
  auto getNumPredecessors() const -> unsigned int { return predecessors.size(); }
  auto getNumSuccessors() const -> unsigned int { return successors.size(); }
  auto front() -> const std::unique_ptr<Instruction> & { return instructions.front(); }
  auto back() -> const std::unique_ptr<Instruction> & { return instructions.back(); }
  auto begin() -> iterator { return instructions.begin(); }
  auto end() -> iterator { return instructions.end(); }
  void addPredecessor(BasicBlock *block) { predecessors.insert(block); }
  void addSuccessor(BasicBlock *block) { successors.insert(block); }
  void removePredecessor(BasicBlock *block) { predecessors.erase(block); }
  void removeSuccessor(BasicBlock *block) { successors.erase(block); }
  auto removeInst(iterator pos) -> iterator { return instructions.erase(pos); }
  void mergeBlock(BasicBlock *block);
};

class Module;
/**
 * @brief 函数
 *
 */

class Function : public Label {
  friend class Module;

 public:
  enum ReturnType { VOID, INT, FLOAT };

 private:
  ReturnType returnType;                          /// 返回值类型
  Module *parent;                                 ///< 父模块
  std::list<std::unique_ptr<BasicBlock>> blocks;  ///< 基本块集合
  bool isExternal;                                ///< 是否为外部函数；

 private:
  explicit Function(Module *parent, const std::string &name, ReturnType returnType, bool isExternal = false)
      : Label(name), returnType(returnType), parent(parent), isExternal(isExternal) {}

 public:
  auto getParent() const -> const Module * { return parent; }
  auto getEntryBlock() const -> BasicBlock * { return blocks.front().get(); }
  auto getBlocks() -> std::list<std::unique_ptr<BasicBlock>> & { return blocks; }
  auto getReturnType() const -> ReturnType { return returnType; }
  auto addBasicBlock(const std::string &name = "") -> BasicBlock *;
  auto isExternalFucntion() const -> bool { return isExternal; }
  void sortBlocks();
  auto renameBlocks(uint64_t begin = 0) -> uint64_t;
  void removeBlock(BasicBlock *targetBlock) {
    auto predicate = [&](const std::unique_ptr<BasicBlock> &block) -> bool { return block.get() == targetBlock; };
    blocks.remove_if(predicate);
  }
};

/**
 * @brief 最顶层的后端结构
 *
 */
class Module {
 private:
  std::map<std::string, std::unique_ptr<Global>> globals;      ///< 全局数据列表
  std::map<std::string, std::unique_ptr<Function>> functions;  ///< 函数列表

 public:
  Module() = default;

 public:
  auto createFunction(const std::string &name, Function::ReturnType returnType, bool isExternal = false) -> Function *;
  auto createGlobal(bool isInt, const std::vector<unsigned> &dims, const ConstantCounter &init, const std::string &name)
      -> Global *;
  auto getFunction(const std::string &name) const -> Function *;
  auto getFunctions() -> std::map<std::string, std::unique_ptr<Function>> & { return functions; }
  auto getGlobal(const std::string &name) const -> Global *;
  auto getGlobals() const -> const std::map<std::string, std::unique_ptr<Global>> & { return globals; }
  void renameGlobals();
};
}  // namespace riscv
