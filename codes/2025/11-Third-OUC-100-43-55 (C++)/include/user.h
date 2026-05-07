#ifndef IR_USER_H
#define IR_USER_H

#include "type.h"
#include "value.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace ir {

class User : public Value {
protected:
  std::vector<std::unique_ptr<Use>> operands;

public:
  using Value::Value;
  explicit User(std::unique_ptr<Type> type,
                const std::vector<Value *> &useOperands);
  // ~User() override = default;

  void addOperand(Value *v);
  void setOperand(size_t index, Value *v);
  Value *getOperand(size_t index) const;
  Value *getLastOperand() const;

  size_t getNumOperands() const { return operands.size(); }
  bool empty() const { return operands.empty(); }
  // Probably no use
  std::unique_ptr<Use> eraseOperand(size_t index);

  void replaceOperand(Value *oldVal, Value *newVal);
  void
  replaceAllOperands(const std::unordered_map<Value *, Value *> &replaceMap);

  using iterator = std::vector<std::unique_ptr<Use>>::iterator;
  using const_iterator = std::vector<std::unique_ptr<Use>>::const_iterator;

  iterator begin() { return operands.begin(); }
  iterator end() { return operands.end(); }
  const_iterator begin() const { return operands.begin(); }
  const_iterator end() const { return operands.end(); }
};

} // namespace ir

#endif // IR_USER_H
