#ifndef AAAC_TYPE_H
#define AAAC_TYPE_H

#include "common.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#pragma once

namespace aaac {
namespace frontend {

class Type : public DebugDumpImpl {
public:
  Type() : size(4) {}
  Type(int size) : size(size) {}
  virtual std::string getTypePrefix() const = 0;
  virtual std::string getTypePostfix() const = 0;
  virtual void PrintTo(std::ostream &os) const;
  virtual std::string getAsString() const;
  virtual std::ostream &dump(std::ostream &os) const;
  int getTypeSize() const { return size; }
  virtual bool isBuiltinType() const { return false; }
  virtual bool isArrayType() const { return false; }
  virtual bool isFunctionType() const { return false; }
  virtual ~Type() = default;

  // Type obeys the singlton rule, so we can compare two types only by their
  // address
  bool operator==(const Type &rhs) const { return this == &rhs; }

  bool operator!=(const Type &rhs) const { return !(this == &rhs); }

private:
  int size;
};

class BuiltInType : public Type {
public:
  BuiltInType(const std::string &type) : Type(), btype(type) {}
  BuiltInType(const std::string &type, int size) : Type(size), btype(type) {}
  std::string getTypePrefix() const override;
  std::string getTypePostfix() const override;
  bool isBuiltinType() const override { return true; }
  virtual ~BuiltInType()=default;

private:
  std::string btype;
};

class ArrayType : public Type {
public:
  ArrayType(std::shared_ptr<Type> base, uint64_t len)
      : Type(base->getTypeSize() * len), base(base), length(len),
        element(NULL) {
    if (base->isBuiltinType()) {
      element = base;
    } else {
      element = std::static_pointer_cast<ArrayType>(base)->element;
    }
  }
  std::string getTypePrefix() const override;
  std::string getTypePostfix() const override;
  bool isArrayType() const override { return true; }

  std::shared_ptr<Type> getBaseType() const { return base; };
  std::shared_ptr<Type> getElementType() const { return element; };
  int getArrayLen() const { return length; }
  virtual ~ArrayType()=default;

private:
  std::shared_ptr<Type> base;
  uint64_t length;
  std::shared_ptr<Type> element;
};

class FunctionType : public Type {
public:
  FunctionType(std::vector<std::shared_ptr<Type>> formals,
               std::shared_ptr<Type> ret)
      : formals(formals), ret(ret) {}
  std::string getTypePrefix() const override;
  std::string getTypePostfix() const override;
  bool isFunctionType() const override { return true; }

  std::vector<std::shared_ptr<Type>> &getFormals() { return formals; };
  std::shared_ptr<Type> getRetType() const { return ret; }
  virtual ~FunctionType()=default;

private:
  std::vector<std::shared_ptr<Type>> formals;
  std::shared_ptr<Type> ret;
};

class TypeFactory {
public:
  static std::shared_ptr<TypeFactory> getTypeFactory();
  std::shared_ptr<ArrayType> getArrayType(std::shared_ptr<Type> base,
                                          uint64_t subscript);
  std::shared_ptr<FunctionType>
  getFuncType(std::vector<std::shared_ptr<Type>> formals,
              std::shared_ptr<Type> ret);
  std::shared_ptr<BuiltInType> getBuiltinType(std::string type);
  static std::shared_ptr<BuiltInType> VoidTy;
  static std::shared_ptr<BuiltInType> IntTy;
  static std::shared_ptr<BuiltInType> FloatTy;

private:
  TypeFactory() = default;
  TypeFactory(const TypeFactory &) = delete;
  TypeFactory &operator=(const TypeFactory &) = delete;

  std::map<std::string, std::shared_ptr<Type>> Bases;
  static std::shared_ptr<TypeFactory> s_TypeFactory;
};

inline std::shared_ptr<BuiltInType> TypeFactory::VoidTy =
    std::make_shared<BuiltInType>(std::string("void"), 0);
inline std::shared_ptr<BuiltInType> TypeFactory::IntTy =
    std::make_shared<BuiltInType>(std::string("int"));
inline std::shared_ptr<BuiltInType> TypeFactory::FloatTy =
    std::make_shared<BuiltInType>(std::string("float"));

} // namespace frontend
} // namespace aaac

#endif