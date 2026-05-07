/**
 * GlobalVariable ir
 * Create by Zhang Junbin at 2024/6/2
 */
#ifndef _GLOBAL_VARIABLE_H_
#define _GLOBAL_VARIABLE_H_

#include "Constant.hh"

class GlobalVariable : public GlobalValue {
 private:
  Constant *initValue;

 public:
  // zeroinit
  GlobalVariable(Type *type, string name);
  GlobalVariable(Type *type, Constant *initValue, string name);
  void setInitValue(Constant *initValue_) { initValue = initValue_; }
  void printIR(ostream &stream) const override;
  string toString() const override { return "@" + getName(); }
  Constant *getInitValue() const { return initValue; }
};

#endif