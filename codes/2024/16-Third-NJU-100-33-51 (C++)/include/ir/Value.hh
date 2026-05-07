/**
 * Value: base class of ir
 * Create by Zhang Junbin at 2024/5/31
 */

#ifndef _VALUE_H_
#define _VALUE_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Use.hh"
class Type;
using std::endl;
using std::make_unique;
using std::ostream;
using std::string;
using std::unique_ptr;
using std::vector;

enum ValueTag {
  VT_VALUE,
  VT_INSTR,
  VT_ALLOCA,
  VT_BOP,
  VT_BR,
  VT_CALL,
  VT_ICMP,
  VT_FCMP,
  VT_FPTOSI,
  VT_GEP,
  VT_JUMP,
  VT_LOAD,
  VT_PHI,
  VT_RET,
  VT_SITOFP,
  VT_STORE,
  VT_ZEXT,
  VT_BB,
  VT_CONSTANT,
  VT_INTCONST,
  VT_FLOATCONST,
  VT_ARRCONST,
  VT_BOOLCONST,
  VT_GLOBALVAL,
  VT_GLOBALVAR,
  VT_FUNC,
  VT_ARG,
};

class Value {
 private:
  string vName;
  Type* vType{nullptr};
  // Record subclass types
  ValueTag vTag{VT_VALUE};
  Use* useHead = 0;
  friend Use;

 public:
  Value() {};
  Value(Type* vt, ValueTag vtag) : vType(vt), vTag(vtag) {};
  Value(Type* vt, string vn, ValueTag vtag)
      : vType(vt), vName(vn), vTag(vtag) {};

  virtual void printIR(ostream& stream) const {};
  string getName() const { return vName; }
  void setName(string name_) { vName = name_; }
  ValueTag getValueTag() const { return vTag; }
  void setType(Type* type) { vType = type; }
  Type* getType() const { return vType; }
  virtual string toString() const { return "%" + vName; }
  bool isa(ValueTag vt_) const { return vt_ == vTag; }
  bool isPointer() const;
  bool isFloat() const;
  bool isInteger() const;
  bool isArray() const;

  Use* getUseHead() const { return useHead; }
  uint32_t getUserSize() const {
    int userSize = 0;
    for (Use* use = getUseHead(); use; use = use->next) {
      userSize++;
    }
    return userSize;
  }
  void addUser(Use* use) {
    if (useHead) useHead->pre = use;
    use->next = useHead;
    useHead = use;
    use->pre = nullptr;
  }

  void replaceAllUsesWith(Value* value) {
    vector<Use*> tmpUse;
    for (Use* use = getUseHead(); use; use = use->next) {
      use->value = value;
      tmpUse.push_back(use);
    }
    for (Use* use : tmpUse) {
      value->addUser(use);
    }
    useHead = nullptr;
  }
};

class GlobalValue : public Value {
 public:
  GlobalValue(Type* t, string name, ValueTag vt) : Value(t, name, vt) {}
};

#endif