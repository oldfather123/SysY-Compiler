#ifndef IR_VALUE_H
#define IR_VALUE_H

#include "use.h"
#include "type.h"
#include <memory>
#include <string>
#include <unordered_set>

namespace ir {

enum class ValueKind {
  Const,
  ConstNum,
  ConstZero,
  Inst,
  Global,
  Block,
  Function,
  Arg,
  MachineInst,
  MachineBlock,
  MachineFunc
};

class Value {
private:
  const std::unique_ptr<Type> _type;
  std::unordered_set<Use *> _uses;

public:
  explicit Value(std::unique_ptr<Type> type);
  virtual ~Value();

  // Return a viewport instead of the actual ownership
  Type *getType() const { return _type.get(); }
  // Only used when a machine instruction is created
  std::unique_ptr<BasicType> makeRegType() const;
  size_t getTypeSize() const { return _type->getSize(); }
  virtual ValueKind getValueKind() const = 0;
  bool isConstNum() const { return getValueKind() == ValueKind::ConstNum; }
  bool isConstZero() const { return getValueKind() == ValueKind::ConstZero; }
  bool isGlobal() const { return getValueKind() == ValueKind::Global; }
  bool isArg() const { return getValueKind() == ValueKind::Arg; }
  bool isInst() const { return getValueKind() == ValueKind::Inst; }

  void addUse(Use *use);
  void removeUse(Use *use);
  void replaceAllUsesWith(Value *newValue);

  const std::unordered_set<Use *> &getUses() const { return _uses; }

  virtual std::string str() const { return "Unknown"; }
  virtual std::string getName() const = 0;
};

} // namespace ir

#endif // IR_VALUE_H
