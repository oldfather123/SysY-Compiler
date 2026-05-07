#include "use.hpp"

using namespace IR;

Use::Use(size_t index, User *user, Value *value) : _index(index), _user(user), _value(value) {
}

size_t Use::index() {
  return _index;
}

User *Use::user() {
  return _user;
}

Value *Use::value() {
  return _value;
}

void Use::setIndex(size_t index) {
  _index = index;
}

void Use::setValue(Value *value) {
  _value = value;
}

void Use::setUser(User *user) {
  _user = user;
}

Value::Value(Type *type, const string &name) : _type(type), _name(name), _uses() {
}

Type *Value::type() {
  return _type;
}

string Value::name() {
  return _name;
}

Uses &Value::uses() {
  return _uses;
}

void Value::newUse(Use *use) {
  _uses.push_back(use);
}

void Value::delUse(Use *use) {
  _uses.remove(use);
}

void Value::takeaway(Value *value) {
  for (auto &use : _uses) use->user()->setOperand(use->index(), value);
  _uses.clear();
}

User::User(Type *type, const string &name) : Value(type, name) {
}

Operands &User::operands() {
  return _operands;
}

Value *User::operand(size_t index) {
  return _operands[index].value();
}
void User::newOperand(Value *value) {
  _operands.emplace_back(_operands.size(), this, value);
  value->newUse(&_operands.back());
}

void User::setOperand(size_t _index, Value *_value) {
  assert(_index < _operands.size());
  _operands[_index].setValue(_value);
}
