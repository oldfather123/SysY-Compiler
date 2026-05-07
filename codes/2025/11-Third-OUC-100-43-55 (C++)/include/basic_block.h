#ifndef IR_BASIC_BLOCK_H
#define IR_BASIC_BLOCK_H

#define FMT_HEADER_ONLY
#include "core.h"
#include "function.h"
#include "instruction.h"
#include "value.h"
#include <string>
#include <vector>

namespace ir {

class FuncBase;
class Function;
class InstBase;

class BlockBase : public Value {
private:
  int _id;
  FuncBase *_function;
  std::vector<std::unique_ptr<InstBase>> _instructions;
  std::vector<std::unique_ptr<InstBase>> _unreachables;

public:
  explicit BlockBase(int id);
  void setFunction(FuncBase *func) { _function = func; }
  FuncBase *getFunction() const { return _function; }
  int getID() const { return _id; }
  virtual std::string getLabel() const = 0;
  // Form the .ll IR string for the whole block, including the label and string
  // for all instructions
  std::string str() const override;

  bool empty() const { return _instructions.empty(); }
  // Check the last instruction
  bool hasTerminator() const;
  InstBase *pushInstruction(std::unique_ptr<InstBase> inst);
  InstBase *insertInstruction(size_t index, std::unique_ptr<InstBase> inst);
  InstBase *getTerminator() const;
  InstBase *getLastInstruction() const;
  InstBase *getInstruction(size_t index) const;
  InstBase *setInstruction(size_t index, std::unique_ptr<InstBase> inst);
  std::unique_ptr<InstBase> eraseInstruction(size_t index);
  std::unique_ptr<InstBase> eraseInstruction(InstBase *inst);
  std::unique_ptr<InstBase> getOwnership(size_t index);
  std::unique_ptr<InstBase> getOwnership(InstBase *inst);
  size_t size() const { return _instructions.size(); }
  void clear() {
    _instructions.clear();
    _unreachables.clear();
  }
  void clearUnreachables() { _unreachables.clear(); }
  void setIndexInBlock() const;

  using iterator = std::vector<std::unique_ptr<InstBase>>::iterator;
  using const_iterator = std::vector<std::unique_ptr<InstBase>>::const_iterator;

  InstBase *getInstruction(iterator pos) const;
  // Insert using iterator, can be used in other insert functions
  iterator insertInstruction(iterator pos, std::unique_ptr<InstBase> inst);
  // Erase using iterator, move out the ownership
  std::unique_ptr<InstBase> eraseInstruction(iterator pos);

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;
};

class BasicBlock : public BlockBase {
private:
  static int _counter;

public:
  explicit BasicBlock();
  ValueKind getValueKind() const override { return ValueKind::Block; }
  // Get the LLVM like block label, example: bb0 bb1
  std::string getLabel() const override;
  std::string getName() const override {
    return fmt::format("%{}", getLabel());
  }
};

} // namespace ir

#endif // IR_BASIC_BLOCK_H
