#include "instructions.h"
#include "core.h"
#include "function.h"
#include <cassert>
#include <sstream>

namespace ir {

//===---------------- Binary ----------------===//

std::string_view BinaryInst::opToString(BinaryOp op) {
  switch (op) {
  case BinaryOp::ADD:
    return "add";
  case BinaryOp::SUB:
    return "sub";
  case BinaryOp::MUL:
    return "mul";
  case BinaryOp::SDIV:
    return "sdiv";
  case BinaryOp::SREM:
    return "srem";
  case BinaryOp::XOR:
    return "xor";
  case BinaryOp::FADD:
    return "fadd";
  case BinaryOp::FSUB:
    return "fsub";
  case BinaryOp::FMUL:
    return "fmul";
  case BinaryOp::FDIV:
    return "fdiv";
  default:
    assert(false && "Unknown binary op");
  }
}

BinaryInst::BinaryInst(BinaryOp op, Value *lhs, Value *rhs)
    : Instruction(lhs->getType()->clone(), {lhs, rhs}), _op(op) {
  assert(*lhs->getType() == *rhs->getType() &&
         "Operands must have the same type");
}

BinaryOp BinaryInst::getOp() const { return _op; }

InstKind BinaryInst::getInstKind() const { return InstKind::Binary; }

std::string BinaryInst::str() const {
  auto lhs = getOperand(0);
  auto rhs = getOperand(1);
  if (!Type::isEqual(lhs->getType(), rhs->getType())) {
    throw std::runtime_error("Unmatched types in binary instruction!");
  }
  auto first = lhs->getName();
  auto second = rhs->getName();
  switch (_op) {
  case BinaryOp::ADD:
  case BinaryOp::FADD:
  case BinaryOp::MUL:
  case BinaryOp::FMUL:
  case BinaryOp::XOR:
    if (first < second) {
      std::swap(first, second);
    }
  }
  return fmt::format("{} = {} {} {}, {}", getName(), opToString(_op),
                     lhs->getType()->str(), first, second);
}

std::unique_ptr<Instruction> BinaryInst::cloneEmpty() const {
  auto cloned = std::make_unique<BinaryInst>(getType()->clone(), _op);
  cloned->_cloneTarget = const_cast<BinaryInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

//===---------------- Cmp ----------------===//

std::string_view CmpInst::opToString(CmpOp op) {
  switch (op) {
  case CmpOp::EQ:
    return "icmp eq";
  case CmpOp::NE:
    return "icmp ne";
  case CmpOp::SLT:
    return "icmp slt";
  case CmpOp::SGT:
    return "icmp sgt";
  case CmpOp::SLE:
    return "icmp sle";
  case CmpOp::SGE:
    return "icmp sge";
  case CmpOp::OEQ:
    return "fcmp oeq";
  case CmpOp::OLT:
    return "fcmp olt";
  case CmpOp::OGT:
    return "fcmp ogt";
  case CmpOp::OLE:
    return "fcmp ole";
  case CmpOp::OGE:
    return "fcmp oge";
  case CmpOp::UNE:
    return "fcmp une";
  default:
    assert(false && "Unknown cmp op");
  }
}

CmpInst::CmpInst(CmpOp op, Value *lhs, Value *rhs)
    : Instruction(std::make_unique<BasicType>(BasicKind::I1), {lhs, rhs}),
      _op(op) {}

bool CmpInst::isICmpInst() const {
  switch (_op) {
  case CmpOp::EQ:
  case CmpOp::NE:
  case CmpOp::SLT:
  case CmpOp::SGT:
  case CmpOp::SLE:
  case CmpOp::SGE:
    return true;
  case CmpOp::OEQ:
  case CmpOp::OLT:
  case CmpOp::OGT:
  case CmpOp::OLE:
  case CmpOp::OGE:
  case CmpOp::UNE:
    return false;
  default:
    assert(false && "Unknown cmp op");
  }
}

CmpOp CmpInst::getOp() const { return _op; }

InstKind CmpInst::getInstKind() const {
  if (isICmpInst()) {
    return InstKind::ICmp;
  } else {
    return InstKind::FCmp;
  }
}

std::string CmpInst::str() const {
  auto lhs = getOperand(0);
  auto rhs = getOperand(1);
  if (!Type::isEqual(lhs->getType(), rhs->getType())) {
    throw std::runtime_error("Unmatched types in binary instruction!");
  }
  return fmt::format("{} = {} {} {}, {}", getName(), opToString(_op),
                     lhs->getType()->str(), lhs->getName(), rhs->getName());
}

std::unique_ptr<Instruction> CmpInst::cloneEmpty() const {
  auto cloned = std::make_unique<CmpInst>(_op, nullptr, nullptr);
  cloned->_cloneTarget = const_cast<CmpInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

//===---------------- Cast ----------------===//

std::string_view CastInst::opToString(CastOp op) {
  switch (op) {
  case CastOp::BitCast:
    return "bitcast";
  case CastOp::FPToSI:
    return "fptosi";
  case CastOp::SIToFP:
    return "sitofp";
  case CastOp::SExt:
    return "sext";
  case CastOp::ZExt:
    return "zext";
  default:
    assert(false && "Unknown cast op");
  }
}

CastInst::CastInst(std::unique_ptr<Type> targetType, CastOp op, Value *val)
    : Instruction(std::move(targetType), {val}), _op(op) {}

CastOp CastInst::getCastOp() const { return _op; }

InstKind CastInst::getInstKind() const {
  switch (_op) {
  case CastOp::BitCast:
    return InstKind::BitCast;
  case CastOp::FPToSI:
    return InstKind::FPToSI;
  case CastOp::SIToFP:
    return InstKind::SIToFP;
  case CastOp::SExt:
    return InstKind::SExt;
  case CastOp::ZExt:
    return InstKind::ZExt;
  default:
    assert(false && "Invalid cast op");
  }
}

std::string CastInst::str() const {
  return fmt::format("{} = {} {} {} to {}", getName(), opToString(_op),
                     getOperand(0)->getType()->str(), getOperand(0)->getName(),
                     getType()->str());
}

std::unique_ptr<Instruction> CastInst::cloneEmpty() const {
  auto cloned = std::make_unique<CastInst>(getType()->clone(), _op, nullptr);
  cloned->_cloneTarget = const_cast<CastInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

//===---------------- Terminator ----------------===//

RetInst::RetInst(Value *retVal)
    : Instruction(std::make_unique<BasicType>(BasicKind::VOID), {retVal}) {}

RetInst::RetInst()
    : Instruction(std::make_unique<BasicType>(BasicKind::VOID)) {}

InstKind RetInst::getInstKind() const { return InstKind::Ret; }

std::string RetInst::str() const {
  if (empty()) {
    return "ret void";
  }
  const auto retVal = getOperand(0);
  return fmt::format("ret {} {}", retVal->getType()->str(), retVal->getName());
}

std::unique_ptr<Instruction> RetInst::cloneEmpty() const {
  auto cloned = std::make_unique<RetInst>(nullptr);
  cloned->_cloneTarget = const_cast<RetInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

bool RetInst::isTerminator() const { return true; }

BranchInst::BranchInst(Value *cond, BasicBlock *trueBlock,
                       BasicBlock *falseBlock)
    : Instruction(std::make_unique<BasicType>(BasicKind::VOID), {cond}) {
  addOperand(trueBlock);
  addOperand(falseBlock);
}

BranchInst::BranchInst(BasicBlock *target)
    : Instruction(std::make_unique<BasicType>(BasicKind::VOID)) {
  addOperand(target);
}

bool BranchInst::isConditional() const { return getNumOperands() > 1; }

InstKind BranchInst::getInstKind() const { return InstKind::Branch; }

std::string BranchInst::str() const {
  if (getNumOperands() == 1) {
    auto nextBlock = static_cast<BasicBlock *>(getOperand(0));
    return "br label " + nextBlock->getName();
  } else {
    auto condBlock = static_cast<BasicBlock *>(getOperand(0));
    auto trueBlock = static_cast<BasicBlock *>(getOperand(1));
    auto falseBlock = static_cast<BasicBlock *>(getOperand(2));
    return fmt::format("br i1 {}, label {}, label {}", condBlock->getName(),
                       trueBlock->getName(), falseBlock->getName());
  }
}

std::unique_ptr<Instruction> BranchInst::cloneEmpty() const {
  auto cloned = getNumOperands() > 1
                    ? std::make_unique<BranchInst>(nullptr, nullptr, nullptr)
                    : std::make_unique<BranchInst>(nullptr);
  cloned->_cloneTarget = const_cast<BranchInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

bool BranchInst::isTerminator() const { return true; }

//===---------------- Other Instructions ----------------===//

AllocaInst::AllocaInst(std::unique_ptr<Type> allocType)
    : Instruction(std::make_unique<PointerType>(std::move(allocType))) {}

InstKind AllocaInst::getInstKind() const { return InstKind::Alloca; }

std::string AllocaInst::str() const {
  assert(getType()->isArray() || getType()->isPointer());
  return fmt::format("{} = alloca {}", getName(),
                     getType()->getBaseType()->str());
}

std::unique_ptr<Instruction> AllocaInst::cloneEmpty() const {
  auto cloned = std::make_unique<AllocaInst>(getType()->clone());
  cloned->_cloneTarget = const_cast<AllocaInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

LoadInst::LoadInst(Value *ptr)
    : Instruction(ptr->isGlobal() ? ptr->getType()->clone()
                                  : ptr->getType()->getBaseType()->clone(),
                  {ptr}) {}

LoadInst::LoadInst(std::unique_ptr<Type> loadedType, Value *ptr)
    : Instruction(std::move(loadedType), {ptr}) {}

InstKind LoadInst::getInstKind() const { return InstKind::Load; }

std::string LoadInst::str() const {
  const auto ptr = getOperand(0);
  if (ptr->isGlobal())
    return fmt::format("{} = load {}, {}* {}", getName(), getType()->str(),
                       ptr->getType()->str(), ptr->getName());

  return fmt::format("{} = load {}, {} {}", getName(), getType()->str(),
                     ptr->getType()->str(), ptr->getName());
}

std::unique_ptr<Instruction> LoadInst::cloneEmpty() const {
  auto cloned = std::make_unique<LoadInst>(getType()->clone(), nullptr);
  cloned->_cloneTarget = const_cast<LoadInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

StoreInst::StoreInst(Value *val, Value *ptr)
    : Instruction(std::make_unique<BasicType>(BasicKind::VOID), {val, ptr}) {}

InstKind StoreInst::getInstKind() const { return InstKind::Store; }

// Not sure (ATTENTION)
std::string StoreInst::str() const {
  const auto val = getOperand(0);
  const auto ptr = getOperand(1);
  if (ptr->isGlobal()) {
    return fmt::format("store {} {}, {}* {}", val->getType()->str(),
                       val->getName(), ptr->getType()->str(), ptr->getName());
  }
  return fmt::format("store {} {}, {} {}", val->getType()->str(),
                     val->getName(), ptr->getType()->str(), ptr->getName());
}

std::unique_ptr<Instruction> StoreInst::cloneEmpty() const {
  auto cloned = std::make_unique<StoreInst>(nullptr, nullptr);
  cloned->_cloneTarget = const_cast<StoreInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

std::unique_ptr<Type> GetElementPtrInst::calcType(Value *value,
                                                  size_t indexSize) {
  auto type = value->getType();
  std::unique_ptr<PointerType> bType;
  if (value->isGlobal()) {
    bType = std::make_unique<PointerType>(type->clone());
    type = bType.get();
  }
  for (int i = 0; i < indexSize; i++) {
    type = type->getBaseType();
  }
  return std::make_unique<PointerType>(type->clone());
}

GetElementPtrInst::GetElementPtrInst(Value *base,
                                     const std::vector<Value *> &indices)
    : Instruction(calcType(base, indices.size()), {base}) {
  for (auto *idx : indices) {
    addOperand(idx);
  }
}

GetElementPtrInst::GetElementPtrInst(std::unique_ptr<Type> targetType)
    : Instruction(std::move(targetType)) {}

InstKind GetElementPtrInst::getInstKind() const { return InstKind::GEP; }

std::string GetElementPtrInst::str() const {
  fmt::memory_buffer buf;

  const auto ptr = getOperand(0);
  if (ptr->isGlobal())
    fmt::format_to(std::back_inserter(buf), "{} = getelementptr {}, {}* {}",
                   getName(), ptr->getType()->str(), ptr->getType()->str(),
                   ptr->getName());
  else
    fmt::format_to(std::back_inserter(buf), "{} = getelementptr {}, {} {}",
                   getName(), ptr->getType()->getBaseType()->str(),
                   ptr->getType()->str(), ptr->getName());

  for (int i = 1; i < getNumOperands(); i++) {
    const auto operand = getOperand(i);
    fmt::format_to(std::back_inserter(buf), ", {} {}",
                   operand->getType()->str(), operand->getName());
  }

  return fmt::to_string(buf);
}

std::unique_ptr<Instruction> GetElementPtrInst::cloneEmpty() const {
  auto cloned = std::make_unique<GetElementPtrInst>(getType()->clone());
  cloned->_cloneTarget = const_cast<GetElementPtrInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

CallInst::CallInst(Function *func, const std::vector<Value *> &args)
    : Instruction(func->getType()->clone()) {
  addOperand(func);
  for (auto arg : args) {
    addOperand(arg);
  }
}

InstKind CallInst::getInstKind() const { return InstKind::Call; }

std::string CallInst::str() const {
  fmt::memory_buffer buf;
  const auto type = getOperand(0)->getType();
  if (type->isBasic() &&
      static_cast<BasicType *>(type)->getBasicKind() == BasicKind::VOID)
    fmt::format_to(std::back_inserter(buf), "call {} {}", type->str(),
                   getOperand(0)->getName());
  else
    fmt::format_to(std::back_inserter(buf), "{} = call {} {}", getName(),
                   type->str(), getOperand(0)->getName());

  buf.push_back('(');
  for (size_t i = 1; i < getNumOperands(); i++) {
    if (i > 1)
      buf.append(std::string_view(", "));
    fmt::format_to(std::back_inserter(buf), "{} {}",
                   getOperand(i)->getType()->str(), getOperand(i)->getName());
  }
  buf.push_back(')');

  return fmt::to_string(buf);
}

std::unique_ptr<Instruction> CallInst::cloneEmpty() const {
  auto cloned = std::make_unique<CallInst>(nullptr, std::vector<Value *>{});
  cloned->_cloneTarget = const_cast<CallInst *>(this);
  cloned->_notRemapped = true;
  return cloned;
}

PhiInst::PhiInst(std::unique_ptr<Type> type) : Instruction(std::move(type)) {}

void PhiInst::addIncoming(Value *value, BasicBlock *block) {
  // assert(value->getType() == getType() && "Value type must match Phi type");
  addOperand(value);
  _incoming.push_back(block);
}

BasicBlock *PhiInst::getIncomingBlock(size_t index) const {
  assert(index < _incoming.size() && "Index out of range");
  return _incoming.at(index);
}

void PhiInst::setIncomingBlock(size_t index, BasicBlock *block) {
  assert(index < _incoming.size() && "Index out of range");
  _incoming.at(index) = block;
}

void PhiInst::removeIncoming(size_t index) {
  assert(index < _incoming.size() && "Index out of range");
  _incoming.erase(_incoming.begin() + index);
  eraseOperand(index);
  if (getNumOperands() == 1) {
    replaceAllUsesWith(getOperand(0));
    getBlock()->eraseInstruction(this);
  }
}

std::string PhiInst::str() const {
  fmt::memory_buffer buf;
  fmt::format_to(std::back_inserter(buf), "{} = phi {} ", getName(),
                 getType()->str());
  for (size_t i = 0; i < getNumOperands(); i++) {
    auto value = getOperand(i);
    auto block = _incoming[i];
    fmt::format_to(std::back_inserter(buf), "[{}, %{}], ", value->getName(),
                   block->getLabel());
  }
  // Remove the last comma and space
  if (buf.size() >= 2) {
    buf.resize(buf.size() - 2);
  }
  return fmt::to_string(buf);
}

MoveInst::MoveInst(PhiInst *target, Value *src)
    : Instruction(target->getType()->clone(), {src}), _target(target) {}

} // namespace ir
