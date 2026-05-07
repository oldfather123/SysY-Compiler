#include "function.h"
#include "basic_block.h"
#include "core.h"
#include <cassert>
#include <sstream>

namespace ir {

std::string Argument::getName() const { return fmt::format("%{}", _name); }

std::string Argument::str() const {
  return fmt::format("{} {}", getType()->str(), getName());
}

void Function::addArg(std::unique_ptr<Argument> arg) {
  assert(arg && "Cannot add nullptr argument");
  _args.push_back(std::move(arg));
}

Argument *Function::getArg(size_t index) const {
  assert(index < _args.size() && "Arg index out of bounds");
  return _args[index].get();
}

std::vector<Argument *> Function::getArgs() const {
  std::vector<Argument *> result;
  result.reserve(_args.size());
  for (const auto &arg : _args) {
    result.push_back(arg.get());
  }
  return result;
}

void FuncBase::pushBlock(std::unique_ptr<BlockBase> block) {
  assert(block && "Cannot push nullptr block");
  block->setFunction(this);
  _blocks.push_back(std::move(block));
}

void FuncBase::setBlock(size_t index, std::unique_ptr<BlockBase> block) {
  assert(index < _blocks.size() && "Block index out of bounds");
  assert(block && "Cannot set nullptr block");
  block->setFunction(this);
  _blocks[index] = std::move(block);
}

BlockBase *FuncBase::getBlock(size_t index) const {
  assert(index < _blocks.size() && "Block index out of bounds");
  return _blocks[index].get();
}

BlockBase *FuncBase::getEntryBlock() const {
  if (_blocks.empty())
    return nullptr;
  return _blocks.front().get();
}

BlockBase *FuncBase::getExitBlock() const {
  if (_blocks.empty())
    return nullptr;
  return _blocks.back().get();
}

std::unique_ptr<BlockBase> FuncBase::eraseBlock(size_t index) {
  assert(index < _blocks.size() && "Block index out of bounds");
  auto it = _blocks.begin() + index;
  std::unique_ptr<BlockBase> removed = std::move(*it);
  _blocks.erase(it);
  return removed;
}

std::string FuncBase::getRawName() const { return _name; }

std::string Function::getName() const {
  return fmt::format("@{}", getRawName());
}

std::string Function::str() const {
  const bool isDeclare = empty();
  fmt::memory_buffer buf;
  fmt::format_to(std::back_inserter(buf), "{} {} @{}",
                 isDeclare ? "declare" : "define", getType()->str(),
                 getRawName());
  buf.push_back('(');
  for (size_t i = 0; i < _args.size(); ++i) {
    if (i > 0)
      buf.append(std::string_view(", "));

    if (isDeclare)
      buf.append(_args[i]->getType()->str());
    else
      buf.append(_args[i]->str());
  }
  buf.push_back(')');

  if (isDeclare)
    buf.push_back('\n');
  else {
    buf.append(std::string_view("{\n"));
    for (const auto &block : *this)
      buf.append(block->str());
    buf.append(std::string_view("}\n"));
  }

  return fmt::to_string(buf);
}

FuncBase::iterator FuncBase::begin() { return _blocks.begin(); }

FuncBase::iterator FuncBase::end() { return _blocks.end(); }

FuncBase::const_iterator FuncBase::begin() const { return _blocks.begin(); }

FuncBase::const_iterator FuncBase::end() const { return _blocks.end(); }

FuncBase::const_iterator FuncBase::cbegin() const { return _blocks.cbegin(); }

FuncBase::const_iterator FuncBase::cend() const { return _blocks.cend(); }

FuncBase::iterator FuncBase::insertBlock(iterator pos,
                                         std::unique_ptr<BlockBase> block) {
  assert(block && "Cannot insert nullptr block");
  block->setFunction(this);
  return _blocks.insert(pos, std::move(block));
}

void FuncBase::insertBlockAfter(BlockBase *target,
                                std::unique_ptr<BlockBase> block) {
  for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
    if (it->get() == target) {
      block->setFunction(this);
      insertBlock(std::next(it), std::move(block));
      return;
    }
  }
  throw std::runtime_error("Target not found!");
}

std::unique_ptr<BlockBase> FuncBase::eraseBlock(iterator pos) {
  std::unique_ptr<BlockBase> removed = std::move(*pos);
  _blocks.erase(pos);
  return removed;
}

BlockBase *FuncBase::getBlock(iterator pos) const { return pos->get(); }

} // namespace ir
