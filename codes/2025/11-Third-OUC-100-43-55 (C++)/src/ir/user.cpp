#include "user.h"
#include <cassert>

namespace ir {

User::User(std::unique_ptr<Type> type, const std::vector<Value *> &useOperands)
    : User(std::move(type)) {
  for (auto *v : useOperands) {
    addOperand(v);
  }
}

void User::addOperand(Value *v) {
  operands.push_back(std::make_unique<Use>(this, v));
}

void User::setOperand(size_t index, Value *v) {
  assert(index < operands.size());
  operands[index]->setValue(v);
}

std::unique_ptr<Use> User::eraseOperand(size_t index) {
  assert(index < operands.size());
  auto it = operands.begin() + index;
  auto erased = std::move(*it);
  operands.erase(it);
  return erased;
}

Value *User::getOperand(size_t index) const {
  assert(index < operands.size());
  return operands[index]->getValue();
}

Value *User::getLastOperand() const {
  assert(!operands.empty());
  return operands.back()->getValue();
}

void User::replaceOperand(Value *oldVal, Value *newVal) {
  for (auto &usePtr : operands) {
    auto use = usePtr.get();
    if (use->getValue() == oldVal) {
      use->setValue(newVal);
    }
  }
}

void User::replaceAllOperands(
    const std::unordered_map<Value *, Value *> &replaceMap) {
  for (auto &usePtr : operands) {
    auto use = usePtr.get();
    auto it = replaceMap.find(use->getValue());
    if (it != replaceMap.end()) {
      use->setValue(it->second);
    }
  }
}

} // namespace ir
