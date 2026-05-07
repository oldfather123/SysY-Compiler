#ifndef IR_FUNCTION_H
#define IR_FUNCTION_H

#include "basic_block.h"
#include "value.h"
#include <memory>
#include <string>
#include <vector>

namespace ir {

class BlockBase;
class Argument : public Value {
private:
  const std::string _name;

public:
  Argument(std::unique_ptr<Type> type, std::string name)
      : Value(std::move(type)), _name(std::move(name)) {}
  ValueKind getValueKind() const override { return ValueKind::Arg; }
  std::string getRawName() const { return _name; }
  // Get the LLVM like SSA name, example: %a
  std::string getName() const override;
  // Get the LLVM like full name, example: i32 %a
  std::string str() const override;
};

class FuncBase : public Value {
private:
  std::string _name;
  std::vector<std::unique_ptr<BlockBase>> _blocks;

public:
  FuncBase(std::unique_ptr<Type> type, std::string name)
      : Value(std::move(type)), _name(std::move(name)) {}

  size_t size() const { return _blocks.size(); }
  bool empty() const { return _blocks.empty(); }

  void pushBlock(std::unique_ptr<BlockBase> block);
  void setBlock(size_t index, std::unique_ptr<BlockBase> block);
  std::unique_ptr<BlockBase> eraseBlock(size_t index);

  BlockBase *getBlock(size_t index) const;
  BlockBase *getEntryBlock() const;
  BlockBase *getExitBlock() const;

  // Get the function's name
  std::string getRawName() const;

  using iterator = std::vector<std::unique_ptr<BlockBase>>::iterator;
  using const_iterator =
      std::vector<std::unique_ptr<BlockBase>>::const_iterator;

  BlockBase *getBlock(iterator pos) const;
  // Insert using iterator, can be used in other insert functions
  iterator insertBlock(iterator pos, std::unique_ptr<BlockBase> block);
  // Erase using iterator, move out the ownership
  std::unique_ptr<BlockBase> eraseBlock(iterator pos);

  void insertBlockAfter(BlockBase *target, std::unique_ptr<BlockBase> block);
  BlockBase *getFirstBlock() const { return _blocks.front().get(); }

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;
};

class Function : public FuncBase {
private:
  std::vector<std::unique_ptr<Argument>> _args;

public:
  using FuncBase::FuncBase;
  ValueKind getValueKind() const override { return ValueKind::Function; }
  void addArg(std::unique_ptr<Argument> arg);
  Argument *getArg(size_t index) const;
  // Return a viewport instead of the actual ownership
  std::vector<Argument *> getArgs() const;
  // Get the LLVM like SSA name
  std::string getName() const override;
  // Form the .ll IR string for the whole block, call block->str() for the
  // block's .ll IR string including the label and all instructions
  std::string str() const override;
};

} // namespace ir

#endif // IR_FUNCTION_H
