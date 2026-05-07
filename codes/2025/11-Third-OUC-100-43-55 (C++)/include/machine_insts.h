#ifndef RISCV_MACHINE_INST_H
#define RISCV_MACHINE_INST_H

#define FMT_HEADER_ONLY
#include "global_variable.h"
#include "instruction.h"
#include "machine_func.h"
#include "register.h"
#include "registers.h"
#include "type.h"

#include "core.h"

#define MAKE_VOID std::make_unique<ir::BasicType>(ir::BasicKind::VOID)

namespace riscv {

class MachineBlock;
class MReg;

enum class MInstKind {
  Fake,
  LEA,
  Jump,
  Call,
  LI,
  LLA,
  LoadFrom,
  Load,
  RR,
  RRI,
  RRR,
  StoreTo,
  Store,
  PhiNode,
};

enum class RRIOp { ADDI, ANDI, SLLIW, SRAIW, SRLI, SRLIW, XORI, SLTI };

enum class RROp { CVT, FABS, MV, NEG, SEQZ, SNEZ };

enum class RRROp {
  ADD,
  ADDW,
  SUB,
  SUBW,
  MUL,
  MULW,
  DIV,
  DIVW,
  REMW,
  EQ,
  GE,
  GT,
  LE,
  LT,
  AND,
  XOR,
  SLT,
  SGT
};

enum class LoadItem { SPILL, INNER, OUTER, LOCAL };

class MachineInst : public ir::InstBase {
private:
  static int _counter;
  ir::Reg *_dest = nullptr;

public:
  MachineInst(std::unique_ptr<ir::Type> type);

  MachineInst(std::unique_ptr<ir::Type> type,
              const std::vector<Value *> &useOperands);

  MachineInst(ir::Reg *dest)
      : InstBase(dest->getRegType()->clone(), _counter++), _dest(dest) {}

  MachineInst(ir::Reg *dest, const std::vector<Value *> &useOperands)
      : InstBase(dest->getRegType()->clone(), useOperands, _counter++),
        _dest(dest) {}

  MachineInst() : InstBase(MAKE_VOID, _counter++) {}

  // virtual ~MachineInst() = default;

  // Get the dest register's name, virtual or real
  std::string getName() const override;
  ir::ValueKind getValueKind() const override {
    return ir::ValueKind::MachineInst;
  }
  ir::BasicKind getBasicKind() const {
    return static_cast<ir::BasicType *>(getType())->getBasicKind();
  }
  ir::Reg *getDest() const { return _dest; }
  void setDest(ir::Reg *dest) { _dest = dest; }
  ir::Reg *getSrc(size_t index) const {
    return static_cast<MachineInst *>(getOperand(index))->getDest();
  }
  void setSrc(size_t index, ir::Reg *src) {
    auto inst = static_cast<MachineInst *>(getOperand(index));
    inst->setDest(src);
  }

  std::vector<ir::Reg *> getRegs() const;
  virtual std::vector<ir::Reg *> getRead() const {
    std::vector<ir::Reg *> vec;
    for (size_t i = 0; i < getNumOperands(); i++) {
      auto inst = static_cast<MachineInst *>(getOperand(i));
      vec.push_back(inst->getDest());
    }
    return vec;
  }
  virtual std::vector<ir::Reg *> getWrite() const {
    if (_dest) {
      return {_dest};
    }
    return {};
  }
  virtual MInstKind getMInstKind() const { return MInstKind::Fake; }
  virtual bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block);
  virtual void replaceReg(std::unordered_map<ir::VReg *, MReg *> &replaceMap);
};

class ImmInst : public MachineInst {
private:
  int _imm;

public:
  ImmInst(std::unique_ptr<ir::Type> type, int imm)
      : MachineInst(std::move(type)), _imm(imm) {}

  ImmInst(std::unique_ptr<ir::Type> type,
          const std::vector<Value *> &useOperands, int imm)
      : MachineInst(std::move(type), useOperands), _imm(imm) {}

  ImmInst(ir::Reg *dest, int imm) : MachineInst(dest), _imm(imm) {}

  ImmInst(ir::Reg *dest, const std::vector<Value *> &useOperands, int imm)
      : MachineInst(dest, useOperands), _imm(imm) {}

  int getImm() const { return _imm; }
};

class LEA : public ImmInst {
public:
  LEA(std::unique_ptr<ir::Type> type, int imm)
      : ImmInst(std::move(type), imm) {}

  LEA(ir::Reg *dest, int imm) : ImmInst(dest, imm) {}

  MInstKind getMInstKind() const override { return MInstKind::LEA; }

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;

  std::string str() const override {
    return fmt::format("add\t{}, $local, #{}", getDest()->str(), getImm());
  }
};

enum class JumpOp { NUL, EQ, NE, LT, LE, GT, GE };

class Jump : public MachineInst {
private:
  JumpOp _op = JumpOp::NUL;
  MachineBlock *_target;

  std::string_view opToString() const {
    switch (_op) {
    case JumpOp::NUL:
      return "{null}";
    case JumpOp::EQ:
      return "eq";
    case JumpOp::NE:
      return "ne";
    case JumpOp::LT:
      return "lt";
    case JumpOp::LE:
      return "le";
    case JumpOp::GT:
      return "gt";
    case JumpOp::GE:
      return "ge";
    default:
      return "{unknown}";
    }
  }

public:
  Jump(JumpOp op, MachineInst *src0, MachineInst *src1, MachineBlock *target)
      : MachineInst(MAKE_VOID, {src0, src1}), _op(op), _target(target) {}

  Jump(MachineBlock *target) : _target(target) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  bool hasCond() const { return _op != JumpOp::NUL; }
  MInstKind getMInstKind() const override { return MInstKind::Jump; }
  std::string str() const override;
  MachineBlock *getTargetBlock() const { return _target; }
};

class Call : public MachineInst {
private:
  ir::Function *_func;

public:
  Call(ir::Function *func) : _func(func) {}

  std::vector<ir::Reg *> getRead() const override;
  std::vector<ir::Reg *> getWrite() const override;
  ir::Function *getFunction() const { return _func; }
  MInstKind getMInstKind() const override { return MInstKind::Call; }
  std::string str() const override {
    return fmt::format("call\t{}", _func->getRawName());
  }
};

class LI : public ImmInst {
public:
  LI(std::unique_ptr<ir::Type> type, int imm) : ImmInst(std::move(type), imm) {}
  LI(ir::Reg *dest, int imm) : ImmInst(dest, imm) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  MInstKind getMInstKind() const override { return MInstKind::LI; }
  std::string str() const override {
    return fmt::format("li\t{}, {}", getDest()->str(), getImm());
  }
};

class LLA : public MachineInst {
private:
  ir::GlobalVariable *_global;

public:
  LLA(std::unique_ptr<ir::Type> type, ir::GlobalVariable *global)
      : MachineInst(std::move(type)), _global(global) {}

  LLA(ir::Reg *dest, ir::GlobalVariable *global)
      : MachineInst(dest), _global(global) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  MInstKind getMInstKind() const override { return MInstKind::LLA; }
  // ATTENTION LA and LLA !
  std::string str() const override {
    return fmt::format("la\t{}, {}", getDest()->str(), _global->getRawName());
  }
};

class LoadFrom : public ImmInst {
private:
  LoadItem _item;

  std::string_view itemToString() const noexcept {
    switch (_item) {
    case LoadItem::SPILL:
      return "spill";
    case LoadItem::INNER:
      return "inner";
    case LoadItem::OUTER:
      return "outer";
    case LoadItem::LOCAL:
      return "local";
    default:
      return "incomplete{LoadFrom.itemToString}";
    }
  }

public:
  LoadFrom(LoadItem item, std::unique_ptr<ir::Type> type, int imm)
      : ImmInst(std::move(type), imm), _item(item) {}

  LoadFrom(LoadItem item, ir::Reg *dest, int imm)
      : ImmInst(dest, imm), _item(item) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  MInstKind getMInstKind() const override { return MInstKind::LoadFrom; }
  LoadItem getItem() const { return _item; }
  std::string str() const override {
    return fmt::format("load\t{}, {}(${})", getDest()->str(), getImm(),
                       itemToString());
  }
};

class Load : public ImmInst {
private:
  int _size;

public:
  Load(std::unique_ptr<ir::Type> type, MachineInst *src, int imm, int size)
      : ImmInst(std::move(type), {src}, imm), _size(size) {}

  Load(ir::Reg *dest, MachineInst *src, int imm, int size)
      : ImmInst(dest, {src}, imm), _size(size) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  MInstKind getMInstKind() const override { return MInstKind::Load; }
  std::string str() const override {
    std::string_view ldStrV;
    const auto destType = getDest()->getRegType()->getBasicKind();
    if (destType == ir::BasicKind::F32)
      ldStrV = "flw";
    else if (destType == ir::BasicKind::I32)
      if (_size == 4)
        ldStrV = "lw";
      else if (_size == 8)
        ldStrV = "ld";
      else
        throw std::runtime_error("Invalid size in Load::str");
    else
      throw std::runtime_error("Invalid type in Load::str");
    return fmt::format("{}\t{}, {}({})", ldStrV, getDest()->str(), getImm(),
                       getSrc(0)->str());
  }
};

class RR : public MachineInst {
private:
  RROp _op;

  std::string_view opToString() const noexcept {
    switch (_op) {
    case RROp::CVT:
      return "cvt";
    case RROp::FABS:
      return "fabs";
    case RROp::MV:
      return "mv";
    case RROp::NEG:
      return "neg";
    case RROp::SEQZ:
      return "seqz";
    case RROp::SNEZ:
      return "snez";
    default:
      return "incomplete{RR.opToString}";
    }
  }

public:
  RR(RROp op, std::unique_ptr<ir::Type> type, MachineInst *src)
      : MachineInst(std::move(type), {src}), _op(op) {}

  RR(RROp op, ir::Reg *dest, MachineInst *src)
      : MachineInst(dest, {src}), _op(op) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  MInstKind getMInstKind() const override { return MInstKind::RR; }
  std::string str() const override {
    const auto destType = getDest()->getRegType()->getBasicKind();
    const auto srcType = getSrc(0)->getRegType()->getBasicKind();
    switch (_op) {
    case RROp::CVT:
      if (destType == ir::BasicKind::F32 && srcType == ir::BasicKind::I32)
        return fmt::format("fcvt.s.w\t{}, {}", getDest()->str(),
                           getSrc(0)->str());
      if (destType == ir::BasicKind::I32 && srcType == ir::BasicKind::F32)
        return fmt::format("fcvt.w.s\t{}, {}, rtz", getDest()->str(),
                           getSrc(0)->str());
      throw std::runtime_error("Invalid types in RR::str, CVT");
    case RROp::FABS:
      return fmt::format("fabs.s\t{}, {}", getDest()->str(), getSrc(0)->str());
    case RROp::NEG:
      if (destType == ir::BasicKind::F32 && srcType == ir::BasicKind::F32)
        return fmt::format("fneg.s\t{}, {}", getDest()->str(),
                           getSrc(0)->str());
      if (destType == ir::BasicKind::I32 && srcType == ir::BasicKind::I32)
        return fmt::format("negw\t{}, {}", getDest()->str(), getSrc(0)->str());
      throw std::runtime_error("Invalid types in RR::str, NEG");
    case RROp::MV:
      if (destType == ir::BasicKind::F32)
        if (srcType == ir::BasicKind::F32)
          return fmt::format("fmv.s\t{}, {}", getDest()->str(),
                             getSrc(0)->str());
        else if (srcType == ir::BasicKind::I32)
          return fmt::format("fmv.w.x\t{}, {}", getDest()->str(),
                             getSrc(0)->str());

      if (destType == ir::BasicKind::I32)
        if (srcType == ir::BasicKind::F32)
          return fmt::format("fmv.x.w\t{}, {}", getDest()->str(),
                             getSrc(0)->str());
        else if (srcType == ir::BasicKind::I32)
          return fmt::format("mv\t{}, {}", getDest()->str(), getSrc(0)->str());
      throw std::runtime_error("Invalid types in RR::str, MV");
    case RROp::SEQZ:
    case RROp::SNEZ:
      return fmt::format("{}\t{}, {}", opToString(), getDest()->str(),
                         getSrc(0)->str());
    }
    throw std::runtime_error("Invalid operation in RR::str");
  }
};

class RRI : public ImmInst {
private:
  RRIOp _op;

  std::string_view opToString() const noexcept {
    switch (_op) {
    case RRIOp::ADDI:
      return "addi";
    case RRIOp::ANDI:
      return "andi";
    case RRIOp::SLLIW:
      return "slliw";
    case RRIOp::SRAIW:
      return "sraiw";
    case RRIOp::SRLIW:
      return "srliw";
    case RRIOp::SRLI:
      return "srli";
    case RRIOp::XORI:
      return "xori";
    case RRIOp::SLTI:
      return "slti";
    default:
      return "incomplete{RRI.opToString}";
    }
  }

public:
  RRI(RRIOp op, std::unique_ptr<ir::Type> type, MachineInst *src, int imm)
      : ImmInst(std::move(type), {src}, imm), _op(op) {}

  RRI(RRIOp op, ir::Reg *dest, MachineInst *src, int imm)
      : ImmInst(dest, {src}, imm), _op(op) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  MInstKind getMInstKind() const override { return MInstKind::RRI; }
  std::string str() const override {
    return fmt::format("{}\t{}, {}, {}", opToString(), getDest()->str(),
                       getSrc(0)->str(), getImm());
  }
};

class RRR : public MachineInst {
private:
  RRROp _op;

  std::string_view opToString() const noexcept {
    switch (_op) {
    case RRROp::ADD:
      return "add";
    case RRROp::ADDW:
      return "addw";
    case RRROp::SUB:
      return "sub";
    case RRROp::SUBW:
      return "subw";
    case RRROp::MUL:
      return "mul";
    case RRROp::MULW:
      return "mulw";
    case RRROp::DIV:
      return "div";
    case RRROp::DIVW:
      return "divw";
    case RRROp::REMW:
      return "remw";
    case RRROp::EQ:
      return "eq";
    case RRROp::GE:
      return "ge";
    case RRROp::GT:
      return "gt";
    case RRROp::LE:
      return "le";
    case RRROp::LT:
      return "lt";
    case RRROp::AND:
      return "and";
    case RRROp::XOR:
      return "xor";
    case RRROp::SLT:
      return "slt";
    case RRROp::SGT:
      return "sgt";
    default:
      return "incomplete{RRR.opToString}";
    }
  }

public:
  RRR(RRROp op, std::unique_ptr<ir::Type> type, MachineInst *src0,
      MachineInst *src1)
      : MachineInst(std::move(type), {src0, src1}), _op(op) {}

  RRR(RRROp op, ir::Reg *dest, MachineInst *src0, MachineInst *src1)
      : MachineInst(dest, {src0, src1}), _op(op) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  MInstKind getMInstKind() const override { return MInstKind::RRR; }
  std::string str() const override {
    bool useFpFormat = false, opIsValid = false;
    const auto destType = getDest()->getRegType()->getBasicKind();
    if (destType == ir::BasicKind::F32)
      switch (_op) {
      case RRROp::ADD:
      case RRROp::SUB:
      case RRROp::MUL:
      case RRROp::DIV:
        useFpFormat = true;
        opIsValid = true;
        break;
      default:;
      }
    else if (destType == ir::BasicKind::I32)
      switch (_op) {
      case RRROp::EQ:
      case RRROp::GE:
      case RRROp::GT:
      case RRROp::LE:
      case RRROp::LT:
        useFpFormat = true;
        opIsValid = true;
        break;
      case RRROp::ADD:
      case RRROp::ADDW:
      case RRROp::SUB:
      case RRROp::SUBW:
      case RRROp::MUL:
      case RRROp::MULW:
      case RRROp::DIV:
      case RRROp::DIVW:
      case RRROp::REMW:
      case RRROp::XOR:
      case RRROp::AND:
      case RRROp::SLT:
      case RRROp::SGT:
        opIsValid = true;
        break;
      }
    else
      throw std::runtime_error("Invalid dest type in RRR::str");

    if (!opIsValid)
      throw std::runtime_error("Invalid operation in RRR::str");

    const auto opStr = opToString();
    if (useFpFormat)
      return fmt::format("f{}.s\t{}, {}, {}", opStr, getDest()->str(),
                         getSrc(0)->str(), getSrc(1)->str());

    return fmt::format("{}\t{}, {}, {}", opStr, getDest()->str(),
                       getSrc(0)->str(), getSrc(1)->str());
  }
};

enum class StoreItem { LOCAL, CALL, INNER, OUTER, SPILL };

class StoreTo : public ImmInst {
private:
  StoreItem _item;

  std::string_view itemToString() const noexcept {
    switch (_item) {
    case StoreItem::LOCAL:
      return "local";
    case StoreItem::CALL:
      return "param_call";
    case StoreItem::INNER:
      return "param_inner";
    case StoreItem::OUTER:
      return "param_outer";
    case StoreItem::SPILL:
      return "spill";
    default:
      return "incomplete{StoreTo.itemToString}";
    }
  }

public:
  StoreTo(StoreItem item, MachineInst *src, int imm)
      : ImmInst(MAKE_VOID, {src}, imm), _item(item) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  MInstKind getMInstKind() const override { return MInstKind::StoreTo; }
  StoreItem getItem() const { return _item; }
  std::string str() const override {
    return fmt::format("store\t{}, {}(${})", getSrc(0)->str(), getImm(),
                       itemToString());
  }
};

class Store : public ImmInst {
private:
  int _size;

public:
  Store(MachineInst *src0, MachineInst *src1, int imm, int size)
      : ImmInst(MAKE_VOID, {src0, src1}, imm), _size(size) {}

  bool spill(ir::Reg *spilledReg, int offset, MachineBlock *block) override;
  MInstKind getMInstKind() const override { return MInstKind::Store; }
  std::string str() const override {
    std::string_view instStrV;
    auto srcType = getSrc(0)->getRegType()->getBasicKind();
    if (srcType == ir::BasicKind::F32)
      instStrV = "fsw";
    else if (srcType == ir::BasicKind::I32)
      if (_size == 4)
        instStrV = "sw";
      else if (_size == 8)
        instStrV = "sd";
      else
        throw std::runtime_error("Invalid size in Store::str");
    else
      throw std::runtime_error("Invalid source type in Store::str");

    return fmt::format("{}\t{}, {}({})", instStrV, getSrc(0)->str(), getImm(),
                       getSrc(1)->str());
  }
};

// PhiNode is used to hold a virtual register, no need to be printed
class PhiNode : public MachineInst {
public:
  PhiNode(std::unique_ptr<ir::Type> type) : MachineInst(std::move(type)) {}
  PhiNode(ir::Reg *dest) : MachineInst(dest) {}

  MInstKind getMInstKind() const override { return MInstKind::PhiNode; }
  // Printing a PhiNode is not allowed
  std::string str() const override { return ""; }
};

} // namespace riscv

#undef MAKE_VOID
#endif
