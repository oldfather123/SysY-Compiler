#ifndef __TYPE_HPP__
#define __TYPE_HPP__

#include <bits/stdc++.h>

#include "error.hpp"
#include "typedef.hpp"

namespace IR {

using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

class Type;
class PointerType;
class ArrayType;
class FunctionType;

class Type {
protected:
  BType _btype;
  Type(BType btype);
  virtual ~Type() = default;

public:
  static Type *void_t();
  static Type *char_t();
  static Type *i1_t();
  static Type *i32_t();
  static Type *float_t();
  static Type *label_t();
  static Type *dynamic_t();
  static Type *undefine_t();
  static Type *pointer_t(Type *base);
  static Type *array_t(Type *base, size_t dim);
  static Type *function_t(Type *ret_type, const vector<Type *> &param_types = {});
  bool in(const vector<Type *> &types);
  bool notin(const vector<Type *> &types);
  bool is(Type *type);
  bool isnot(Type *type);
  bool isVoid();
  bool isChar();
  bool isI1();
  bool isI32();
  bool isFloat();
  bool isLabel();
  bool isDynamic();
  bool isUndefine();
  bool isPointer();
  bool isArray();
  bool isFunction();
  virtual Type *btype();
  virtual string name();
  virtual size_t sizeOf();
  virtual size_t size();
  virtual size_t refSize();
  virtual Type *deref(size_t deref);
  virtual size_t dim();
  virtual Type *toPointer();
  virtual Type *retType();
  virtual vector<Type *> &paramTypes();
};

class PointerType : public Type {
protected:
  Type *_base;
  PointerType(Type *base);

public:
  static PointerType *get(Type *base);
  Type *btype() override;
  size_t refSize() override;
  Type *deref(size_t deref_size) override;
};

class ArrayType : public Type {
protected:
  Type *_base;
  size_t _dim;
  size_t _size;
  ArrayType(Type *base, size_t dim);

public:
  static ArrayType *get(Type *base, size_t dim);
  Type *btype() override;
  string name() override;
  size_t sizeOf() override;
  size_t size() override;
  size_t refSize() override;
  Type *deref(size_t deref_size) override;
  size_t dim() override;
  Type *toPointer() override;
};

class FunctionType : public Type {
protected:
  Type *_ret_type;
  vector<Type *> _param_types;
  FunctionType(Type *ret, const vector<Type *> &param_types);

public:
  Type *retType() override;
  vector<Type *> &paramTypes() override;
  static FunctionType *get(Type *ret, const vector<Type *> &param_types);
  string name() override;
};

}  // namespace IR

#endif
