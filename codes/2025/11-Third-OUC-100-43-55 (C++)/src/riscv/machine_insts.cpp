#include "machine_insts.h"
#include "registers.h"
#include <stdexcept>
using namespace ir;

#define MAKE_VOID std::make_unique<BasicType>(BasicKind::VOID)

namespace riscv {
int MachineInst::_counter = 1;

MachineInst::MachineInst(std::unique_ptr<Type> type)
    : InstBase(std::move(type), _counter++) {
  assert(getType()->isBasic() && "MachineInst can only have a basic type.");
  switch (getBasicKind()) {
  case BasicKind::I32:
  case BasicKind::F32:
    _dest = new VReg(static_cast<BasicType *>(getType()), getID());
  default:
    return;
  }
}

MachineInst::MachineInst(std::unique_ptr<Type> type,
                         const std::vector<Value *> &useOperands)
    : InstBase(std::move(type), useOperands, _counter++) {
  assert(getType()->isBasic() && "MachineInst can only have a basic type.");
  switch (getBasicKind()) {
  case BasicKind::I32:
  case BasicKind::F32:
    _dest = new VReg(static_cast<BasicType *>(getType()), getID());
  default:
    return;
  }
}

std::string MachineInst::getName() const {
  if (_dest) {
    return _dest->str();
  }
  throw std::runtime_error("No available reg, getName should not be called");
}

std::vector<Reg *> MachineInst::getRegs() const {
  std::vector<Reg *> regs;
  auto readRegs = getRead();
  auto writeRegs = getWrite();

  regs.insert(regs.end(), readRegs.begin(), readRegs.end());
  regs.insert(regs.end(), writeRegs.begin(), writeRegs.end());
  return std::move(regs);
}

bool MachineInst::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
  return true;
}

void MachineInst::replaceReg(std::unordered_map<VReg *, MReg *> &replaceMap) {
  for (size_t i = 0; i < getNumOperands(); i++) {
    if (auto vreg = dynamic_cast<VReg *>(getSrc(i))) {
      auto it = replaceMap.find(vreg);
      if (it != replaceMap.end()) {
        setSrc(i, it->second);
      }
    }
  }
  if (!_dest) {
    return;
  }
  if (auto vreg = dynamic_cast<ir::VReg *>(_dest)) {
    auto it = replaceMap.find(vreg);
    if (it != replaceMap.end()) {
      setDest(it->second);
    }
  }
}

std::string Jump::str() const {
  if (!hasCond()) {
    return fmt::format("j\t{}", _target->getLabel());
  }
  return fmt::format("b{}\t{}, {}, {}", opToString(), getSrc(0)->str(),
                     getSrc(1)->str(), _target->getLabel());
}

std::vector<Reg *> Call::getRead() const {
  std::vector<Reg *> regs;

  size_t iSize = 0, fSize = 0;
  for (const auto &arg : _func->getArgs()) {
    if (arg->getType()->isF32()) {
      if (fSize < MReg::fCallerRegs.size())
        regs.push_back(MReg::fCallerRegs[fSize]);
      ++fSize;
    } else {
      if (iSize < MReg::iCallerRegs.size())
        regs.push_back(MReg::iCallerRegs[iSize]);
      ++iSize;
    }
  }
  return regs;
}

std::vector<Reg *> Call::getWrite() const {
  std::vector<Reg *> regs;
  regs.insert(regs.end(), MReg::iCallerRegs.begin(), MReg::iCallerRegs.end());
  regs.insert(regs.end(), MReg::fCallerRegs.begin(), MReg::fCallerRegs.end());
  return regs;
}

bool LEA::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (spilledReg != getDest()) {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    auto ptr = block->pushMInst(std::make_unique<LEA>(
        std::make_unique<BasicType>(BasicKind::I32), offset));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr, offset));
    return false;
  }
}

bool Jump::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (!hasCond()) {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  }
  if (getSrc(0) == spilledReg && getSrc(1) == spilledReg) {
    auto ptr1 = block->pushMInst(
        std::make_unique<LoadFrom>(LoadItem::SPILL, getSrc(0), offset));
    auto ptr2 = block->pushMInst(
        std::make_unique<LoadFrom>(LoadItem::SPILL, getSrc(1), offset));
    block->pushInstruction(std::make_unique<Jump>(_op, ptr1, ptr2, _target));
    return false;
  } else if (getSrc(0) == spilledReg) {
    auto ptr = block->pushInstruction(
        std::make_unique<LoadFrom>(LoadItem::SPILL, getSrc(0), offset));
    setOperand(0, ptr);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else if (getSrc(1) == spilledReg) {
    const auto ptr = block->pushInstruction(
        std::make_unique<LoadFrom>(LoadItem::SPILL, getSrc(1), offset));
    setOperand(1, ptr);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  }
}

bool LI::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (spilledReg != getDest()) {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    auto ptr = block->pushMInst(std::make_unique<LI>(spilledReg, getImm()));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr, offset));
    return false;
  }
}

bool LLA::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (spilledReg != getDest()) {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    auto ptr = block->pushMInst(std::make_unique<LLA>(getDest(), _global));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr, offset));
    return false;
  }
}

bool LoadFrom::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (spilledReg != getDest()) {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    auto ptr = block->pushMInst(
        std::make_unique<LoadFrom>(_item, getDest(), getImm()));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr, offset));
    return false;
  }
}

bool Load::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (getDest() == spilledReg && getSrc(0) == spilledReg) {
    auto ptr1 = block->pushMInst(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    auto ptr2 = block->pushMInst(std::make_unique<Load>(
        spilledReg->getRegType()->clone(), ptr1, getImm(), _size));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr2, offset));
    return false;
  } else if (getDest() == spilledReg) {
    auto ptr = block->pushMInst(std::make_unique<Load>(
        spilledReg->getRegType()->clone(),
        static_cast<MachineInst *>(getOperand(0)), getImm(), _size));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr, offset));
    return false;
  } else if (getSrc(0) == spilledReg) {
    const auto ptr = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    setOperand(0, ptr);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  }
}

bool RR::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (getDest() == spilledReg && getSrc(0) == spilledReg) {
    auto ptr1 = block->pushMInst(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    auto ptr2 = block->pushMInst(
        std::make_unique<RR>(_op, spilledReg->getRegType()->clone(), ptr1));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr2, offset));
    return false;
  } else if (getDest() == spilledReg) {
    auto ptr = block->pushMInst(
        std::make_unique<RR>(_op, spilledReg->getRegType()->clone(),
                             static_cast<MachineInst *>(getOperand(0))));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr, offset));
    return false;
  } else if (getSrc(0) == spilledReg) {
    const auto ptr = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    setOperand(0, ptr);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  }
}

bool RRI::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (getDest() == spilledReg && getSrc(0) == spilledReg) {
    auto ptr1 = block->pushMInst(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    auto ptr2 = block->pushMInst(std::make_unique<RRI>(
        _op, spilledReg->getRegType()->clone(), ptr1, getImm()));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr2, offset));
    return false;
  } else if (getDest() == spilledReg) {
    auto ptr = block->pushMInst(std::make_unique<RRI>(
        _op, spilledReg->getRegType()->clone(),
        static_cast<MachineInst *>(getOperand(0)), getImm()));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr, offset));
    return false;
  } else if (getSrc(0) == spilledReg) {
    const auto ptr = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    setOperand(0, ptr);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  }
}

bool RRR::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (getDest() == spilledReg && getSrc(0) == spilledReg &&
      getSrc(1) == spilledReg) {
    auto ptr1 = block->pushMInst(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    auto ptr2 = block->pushMInst(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    auto ptr3 = block->pushMInst(std::make_unique<RRR>(
        _op, spilledReg->getRegType()->clone(), ptr1, ptr2));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr3, offset));
    return false;
  } else if (getDest() == spilledReg && getSrc(0) == spilledReg) {
    auto ptr1 = block->pushMInst(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    auto ptr2 = block->pushMInst(
        std::make_unique<RRR>(_op, spilledReg->getRegType()->clone(), ptr1,
                              static_cast<MachineInst *>(getOperand(1))));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr2, offset));
    return false;
  } else if (getDest() == spilledReg && getSrc(1) == spilledReg) {
    auto ptr1 = block->pushMInst(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    auto ptr2 = block->pushMInst(
        std::make_unique<RRR>(_op, spilledReg->getRegType()->clone(),
                              static_cast<MachineInst *>(getOperand(0)), ptr1));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr2, offset));
    return false;
  } else if (getSrc(0) == spilledReg && getSrc(1) == spilledReg) {
    const auto ptr1 = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    const auto ptr2 = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    setOperand(0, ptr1);
    setOperand(1, ptr2);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else if (getDest() == spilledReg) {
    auto ptr = block->pushMInst(
        std::make_unique<RRR>(_op, spilledReg->getRegType()->clone(),
                              static_cast<MachineInst *>(getOperand(0)),
                              static_cast<MachineInst *>(getOperand(1))));
    block->pushInstruction(
        std::make_unique<StoreTo>(StoreItem::SPILL, ptr, offset));
    return false;
  } else if (getSrc(0) == spilledReg) {
    const auto ptr = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    setOperand(0, ptr);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else if (getSrc(1) == spilledReg) {
    const auto ptr = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    setOperand(1, ptr);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  }
}

bool StoreTo::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (getSrc(0) != spilledReg) {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    auto ptr = block->pushMInst(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    block->pushInstruction(std::make_unique<StoreTo>(_item, ptr, getImm()));
    return false;
  }
}

bool Store::spill(Reg *spilledReg, int offset, MachineBlock *block) {
  if (getSrc(0) == spilledReg && getSrc(1) == spilledReg) {
    const auto ptr1 = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    const auto ptr2 = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    setOperand(0, ptr1);
    setOperand(1, ptr2);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else if (getSrc(0) == spilledReg) {
    const auto ptr = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    setOperand(0, ptr);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else if (getSrc(1) == spilledReg) {
    const auto ptr = block->pushInstruction(std::make_unique<LoadFrom>(
        LoadItem::SPILL, spilledReg->getRegType()->clone(), offset));
    setOperand(1, ptr);
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  } else {
    block->pushInstruction(getBlock()->getOwnership(_indexInBlock));
    return true;
  }
}

} // namespace riscv

#undef MAKE_VOID
