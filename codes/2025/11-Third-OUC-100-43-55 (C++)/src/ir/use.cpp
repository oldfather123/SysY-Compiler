#include "use.h"
#include "value.h"

namespace ir {

Use::Use(User *user, Value *value) : _user(user), _value(value) {
  // Maybe value can be nullptr as a placeholder (ATTENTION)
  // assert(user && value);
  assert(user);
  if (value)
    value->addUse(this);
}

Use::~Use() {
  if (_value) {
    _value->removeUse(this);
  }
}

void Use::setValue(Value *newValue) {
  if (_value)
    _value->removeUse(this);
  _value = newValue;
  if (_value)
    _value->addUse(this);
}

} // namespace ir
