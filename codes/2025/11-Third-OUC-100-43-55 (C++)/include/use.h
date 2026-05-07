#ifndef IR_USE_H
#define IR_USE_H

#include "type.h"

namespace ir {

class Value;
class User;

class Use {
private:
  User *_user;
  Value *_value;

public:
  Use(User *user, Value *value);
  ~Use();

  User *getUser() const { return _user; }
  Value *getValue() const { return _value; }

  void setValue(Value *newValue);
};

} // namespace ir

#endif // IR_USE_H
