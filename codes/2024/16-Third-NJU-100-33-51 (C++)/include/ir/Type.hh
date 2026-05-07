/**
 * Type: type of value
 * Create by Zhang Junbin at 2024/5/31
 */

#ifndef _TYPE_H_
#define _TYPE_H_
#include <memory>
#include <vector>

#include "Argument.hh"
using std::make_unique;
using std::unique_ptr;
using std::vector;

enum TypeTag {
  TT_VOID,
  TT_INT1,
  TT_INT32,
  TT_FLOAT,
  TT_ARRAY,
  TT_FUNCTION,
  TT_POINTER
};

class VoidType;
class Int1Type;
class Int32Type;
class FloatType;
class ArrayType;
class FuncType;
class PointerType;
class Constant;

class Type {
 private:
  TypeTag tTag;
  int width;

  // Singleton
  static VoidType* voidType;
  static Int1Type* int1Type;
  static Int32Type* int32Type;
  static FloatType* floatType;

 public:
  Type(TypeTag tt, int w) : tTag(tt), width(w) {}
  virtual string toString() const { return ""; }
  static inline Int1Type* getInt1Type() { return int1Type; }
  static inline Int32Type* getInt32Type() { return int32Type; }
  static inline FloatType* getFloatType() { return floatType; }
  static inline VoidType* getVoidType() { return voidType; }
  static PointerType* getPointerType(Type* elemType);
  static ArrayType* getArrayType(int n, Type* elemType);
  static FuncType* getFuncType(Type* retType, vector<Argument*>& args);
  static FuncType* getFuncType(Type* retType);
  TypeTag getTypeTag() const { return tTag; }
  virtual Constant* getZeroInit() { return nullptr; }
  size_t getWidth() const { return width; };
};

class VoidType : public Type {
 public:
  string toString() const override;
  VoidType() : Type(TT_VOID, 4) {}
};

class Int1Type : public Type {
 public:
  string toString() const override;
  Int1Type() : Type(TT_INT1, 4) {}
};

class Int32Type : public Type {
 public:
  string toString() const override;
  Int32Type() : Type(TT_INT32, 4) {}
  Constant* getZeroInit() override;
};

class FloatType : public Type {
 public:
  string toString() const override;
  FloatType() : Type(TT_FLOAT, 4) {}
  Constant* getZeroInit() override;
};

class ArrayType : public Type {
 private:
  int len;
  Type* elemType;

 public:
  string toString() const override;
  ArrayType(int l, Type* eT)
      : Type(TT_ARRAY, eT->getWidth() * l), len(l), elemType(eT) {}
  int getLen() const { return len; }
  Type* getElemType() const { return elemType; }
  Constant* getZeroInit() override;
};

class FuncType : public Type {
 private:
  Type* retType;
  unique_ptr<vector<unique_ptr<Argument>>> arguments;

 public:
  FuncType(Type* rType);
  void pushArgument(Argument* arg);
  Type* getRetType() const { return retType; }
  int getArgSize() const { return arguments->size(); }
  Argument* getArgument(int idx) { return arguments->at(idx).get(); }
  void deleteArgumentAt(int loc) { arguments->erase(arguments->begin() + loc); }
};

class PointerType : public Type {
 private:
  Type* elemType;

 public:
  string toString() const override;
  PointerType(Type* et) : Type(TT_POINTER, 4), elemType(et) {}
  Type* getElemType() const { return elemType; }
};

#endif