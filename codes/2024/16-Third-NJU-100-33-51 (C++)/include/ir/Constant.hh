/**
 * Constant ir
 * Create by Zhang Junbin at 2024/6/2
 */

#ifndef _CONSTANT_H_
#define _CONSTANT_H_

#include <map>

#include "Type.hh"
#include "Value.hh"
using std::map;

class Constant : public Value {
 private:
  bool zeroInit;

 public:
  Constant(bool zI) : zeroInit(zI), Value(nullptr, VT_CONSTANT) {}
  Constant(bool zI, Type *t, ValueTag vt) : zeroInit(zI), Value(t, vt) {}
  static Constant *getZeroConstant(Type *t);
  bool isIntConst() const { return getValueTag() == VT_INTCONST; }
  bool isFloatConst() const { return getValueTag() == VT_FLOATCONST; }
  bool isArrayConst() const { return getValueTag() == VT_ARRCONST; }
  bool isZeroInit() const { return zeroInit; }
  void setZeroInit(bool zI) { zeroInit = zI; }
};

class IntegerConstant : public Constant {
 private:
  int value = 0;
  static map<int, IntegerConstant *> constBuffer;

 public:
  IntegerConstant() : Constant(true, Type::getInt32Type(), VT_INTCONST) {}
  IntegerConstant(int v)
      : Constant(false, Type::getInt32Type(), VT_INTCONST), value(v) {}
  string toString() const override { return std::to_string(value); }
  static IntegerConstant *getConstInt(int num);

  int getValue() const { return value; }
};

class FloatConstant : public Constant {
 private:
  float value = 0;
  static map<float, FloatConstant *> constBuffer;

 public:
  FloatConstant() : Constant(true, Type::getFloatType(), VT_FLOATCONST) {}
  FloatConstant(float v)
      : Constant(false, Type::getFloatType(), VT_FLOATCONST), value(v) {}
  string toString() const override;
  static FloatConstant *getConstFloat(float num);

  float getValue() const { return value; }
};

class ArrayConstant : public Constant {
 private:
  unique_ptr<map<int, Value *>> elems;

 public:
  ArrayConstant(Type *type) : Constant(true, type, VT_ARRCONST) {
    elems = make_unique<map<int, Value *>>();
  }

  string toString() const override;
  void put(int loc, Value *v);

  Constant *getElemInit(int loc) {
    auto it = elems->find(loc);
    if (it != elems->end()) {
      return (Constant *)it->second;
    } else {
      return Constant::getZeroConstant(
          static_cast<ArrayType *>(getType())->getElemType());
    }
  }

  bool isZeorInit() { return elems->empty(); }

  map<int, Value *> *getElementMap() { return elems.get(); }

  static ArrayConstant *getConstArray(ArrayType *arrType) {
    return new ArrayConstant(arrType);
  }
};

class BoolConstant : public Constant {
 private:
  bool value = 0;
  static BoolConstant* zero;
  static BoolConstant* one;

 public:
  BoolConstant() : Constant(true, Type::getInt1Type(), VT_BOOLCONST) {}
  BoolConstant(bool v)
      : Constant(false, Type::getInt1Type(), VT_BOOLCONST), value(v) {}
  string toString() const override { return std::to_string((int)value); }
  static BoolConstant *getConstBool(bool val);

  bool getValue() const { return value; }
};

#endif