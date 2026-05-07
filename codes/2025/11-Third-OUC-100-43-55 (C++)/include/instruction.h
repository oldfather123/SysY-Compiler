#ifndef IR_INSTRUCTION_H
#define IR_INSTRUCTION_H

#include "basic_block.h"
#include "user.h"
#include "value.h"
#include <string>
#include <unordered_map>

namespace ir {

class BlockBase;

class InstBase : public User {
private:
  int _id;
  BlockBase *_block = nullptr;

protected:
  // Only used during clone process
  InstBase *_cloneTarget = nullptr;
  bool _notRemapped = false;
    int _indexInBlock = -1;
  InstBase(std::unique_ptr<Type> type, int id);
  InstBase(std::unique_ptr<Type> type, const std::vector<Value *> &useOperands,
           int id);

public:
  // virtual ~InstBase() = default;

  BlockBase *getBlock() const { return _block; }
  // Used in optimization mostly
  void setBlock(BlockBase *block) { _block = block; }
  void setIndexInBlock(int index) { _indexInBlock = index; }

  int getID() const { return _id; }

  using ValueMap = std::unordered_map<Value *, Value *>;

  // Use _cloneTarget to find the old instruction, use the map oldVal -> newVal
  // to map the operands. Set _cloneTarget to nullptr after the process
  // and set notRemapped to false
  void remapValues(const ValueMap &map);
  bool isRemapped() const { return !_notRemapped; }

  virtual bool isTerminator() const { return false; }
};

enum class InstKind {
  Move,
  Alloca,
  Binary,
  BitCast,
  Branch,
  Call,
  FCmp,
  FPToSI,
  GEP,
  ICmp,
  Load,
  Phi,
  Ret,
  SExt,
  SIToFP,
  Store,
  ZExt,
};

class Instruction : public InstBase {
private:
  static int _counter;

public:
  Instruction(std::unique_ptr<Type> type);
  Instruction(std::unique_ptr<Type> type,
              const std::vector<Value *> &useOperands);

  virtual ~Instruction() = default;

  // Get the SSA name like %v1, %v2
  std::string getName() const override;
  ValueKind getValueKind() const override { return ValueKind::Inst; }
  bool isAlloca() { return getInstKind() == InstKind::Alloca; }
  // Create a empty copy without operands, set notRemapped to true. Other part
  // should be the same as the clone target
  virtual std::unique_ptr<Instruction> cloneEmpty() const = 0;
  virtual InstKind getInstKind() const = 0;
};

} // namespace ir

#endif // IR_INSTRUCTION_H
