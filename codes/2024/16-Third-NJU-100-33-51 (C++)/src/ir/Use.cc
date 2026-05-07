#include "Use.hh"

#include "Value.hh"

void Use::removeFromValue() {
  if (this == value->useHead) {
    value->useHead = next;
  }
  if (next) {
    next->pre = pre;
  }
  if (pre) {
    pre->next = next;
  }
  pre = next = 0;
}

void Use::replaceValue(Value* value_) {
  removeFromValue();
  value = value_;
  value->addUser(this);
}