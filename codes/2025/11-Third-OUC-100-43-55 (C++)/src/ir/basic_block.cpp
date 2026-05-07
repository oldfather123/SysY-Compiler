#include "basic_block.h"
#include "core.h"
#include "function.h"
#include "instruction.h"
#include <cassert>
#include <sstream>

namespace ir {

BlockBase::BlockBase(int id)
    : Value(std::make_unique<BasicType>(BasicKind::VOID)), _id(id) {}

BasicBlock::BasicBlock() : BlockBase(_counter++) {}

bool BlockBase::hasTerminator() const {
  return !empty() && _instructions.back()->isTerminator();
}

InstBase *BlockBase::pushInstruction(std::unique_ptr<InstBase> inst) {
  assert(inst && "Cannot insert nullptr instruction");
  // Maybe need assert here? (ATTENTION)
  if (hasTerminator()) {
    inst->setBlock(this);
    auto ret = inst.get();
    _unreachables.push_back(std::move(inst));
    return ret;
  }
  inst->setBlock(this);
  auto ret = inst.get();
  _instructions.push_back(std::move(inst));
  return ret;
}

InstBase *BlockBase::getTerminator() const {
  if (hasTerminator())
    return _instructions.back().get();
  return nullptr;
}

InstBase *BlockBase::getLastInstruction() const {
  if (empty()) {
    return nullptr;
  }
  return _instructions.back().get();
}

InstBase *BlockBase::getInstruction(size_t index) const {
  assert(index < _instructions.size());
  return _instructions[index].get();
}

InstBase *BlockBase::setInstruction(size_t index,
                                    std::unique_ptr<InstBase> inst) {
  assert(index < _instructions.size());
  assert(inst && "Cannot set nullptr instruction");
  inst->setBlock(this);
  auto ret = inst.get();
  _instructions[index] = std::move(inst);
  return ret;
}

InstBase *BlockBase::insertInstruction(size_t index,
                                       std::unique_ptr<InstBase> inst) {
  assert(index <= _instructions.size());
  inst->setBlock(this);
  auto ret = inst.get();
  _instructions.insert(_instructions.begin() + index, std::move(inst));
  return ret;
}

std::unique_ptr<InstBase> BlockBase::eraseInstruction(size_t index) {
  assert(index < _instructions.size());
  auto it = _instructions.begin() + index;
  std::unique_ptr<InstBase> erased = std::move(*it);
  if (erased->getBlock() == this) {
    erased->setBlock(nullptr);
  }
  _instructions.erase(it);
  return erased;
}

std::unique_ptr<InstBase> BlockBase::eraseInstruction(InstBase *inst) {
  for (auto it = _instructions.begin(); it != _instructions.end(); ++it) {
    if (it->get() == inst) {
      std::unique_ptr<InstBase> erased = std::move(*it);
      if (erased->getBlock() == this) {
        erased->setBlock(nullptr);
      }
      _instructions.erase(it);
      return erased;
    }
  }
  return nullptr;
}

std::unique_ptr<InstBase> BlockBase::getOwnership(size_t index) {
  assert(index < _instructions.size());
  auto it = _instructions.begin() + index;
  std::unique_ptr<InstBase> inst = std::move(*it);
  if (inst->getBlock() == this) {
    inst->setBlock(nullptr);
  }
  return inst;
}

std::unique_ptr<InstBase> BlockBase::getOwnership(InstBase *inst) {
  for (auto it = _instructions.begin(); it != _instructions.end(); ++it) {
    if (it->get() == inst) {
      std::unique_ptr<InstBase> instPtr = std::move(*it);
      if (instPtr->getBlock() == this) {
        instPtr->setBlock(nullptr);
      }
      return instPtr;
    }
  }
  return nullptr;
}

void BlockBase::setIndexInBlock() const {
  for (size_t i = 0; i < _instructions.size(); i++) {
    _instructions[i]->setIndexInBlock(i);
  }
}

InstBase *BlockBase::getInstruction(iterator pos) const { return pos->get(); }

BlockBase::iterator
BlockBase::insertInstruction(iterator pos, std::unique_ptr<InstBase> inst) {
  inst->setBlock(this);
  return _instructions.insert(pos, std::move(inst));
}

std::unique_ptr<InstBase> BlockBase::eraseInstruction(iterator pos) {
  std::unique_ptr<InstBase> erased = std::move(*pos);
  if (erased->getBlock() == this) {
    erased->setBlock(nullptr);
  }
  _instructions.erase(pos);
  return erased;
}

BlockBase::iterator BlockBase::begin() { return _instructions.begin(); }

BlockBase::iterator BlockBase::end() { return _instructions.end(); }

BlockBase::const_iterator BlockBase::begin() const {
  return _instructions.begin();
}

BlockBase::const_iterator BlockBase::end() const { return _instructions.end(); }

BlockBase::const_iterator BlockBase::cbegin() const {
  return _instructions.cbegin();
}

BlockBase::const_iterator BlockBase::cend() const {
  return _instructions.cend();
}

int BasicBlock::_counter = 0;

std::string BasicBlock::getLabel() const {
  return fmt::format("bb{}", getID());
}

std::string BlockBase::str() const {
  fmt::memory_buffer buf;
  fmt::format_to(std::back_inserter(buf), "{}:\n", getLabel());
  for (const auto &instPtr : *this) {
    auto instStr = instPtr->str();
    if (!instStr.empty())
      fmt::format_to(std::back_inserter(buf), "  {}\n", instStr);
  }
  return fmt::to_string(buf);
}

} // namespace ir
