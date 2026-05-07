#ifndef __USE_HPP__
#define __USE_HPP__

#include "type.hpp"

namespace IR {

class Use;
class Value;
class User;

using Uses = typename std::list<Use *>;
using Operands = typename std::vector<Use>;

class Use {
  friend class Value;
  friend class User;

protected:
  size_t _index;
  User *_user;
  Value *_value;

public:
  Use(size_t index, User *user, Value *value);
  size_t index();
  User *user();
  Value *value();
  void setIndex(size_t index);
  void setValue(Value *value);
  void setUser(User *user);
};

class Value {
protected:
  Type *_type;
  string _name;
  Uses _uses;

public:
  Value(Type *type, const string &name);
  virtual ~Value() = default;
  Type *type();
  string name();
  Uses &uses();
  void newUse(Use *use);
  void delUse(Use *use);
  void takeaway(Value *_value);  // 夺舍
  void rename(string name) {
    _name = name;
  }
};

class User : public Value {
protected:
  Operands _operands;

public:
  User(Type *type, const string &name);
  Operands &operands();
  Value *operand(size_t index);
  void newOperand(Value *value);
  void setOperand(size_t index, Value *value);
  template <typename Container>
  void newOperands(const Container &operands) {
    for (const auto &operand : operands) newOperand(operand);
  }
};

}  // namespace IR

#endif
