#pragma once

#include "ast.h"
#include <memory>
#include <new>
#include <string>
#include <vector>

// ===--------------------------------------------------------------------=== //
// 类型系统
// ===--------------------------------------------------------------------=== //

class Type {
public:
  virtual ~Type() = default;
  virtual std::string toString() const = 0;
  virtual bool equals(const Type &other) const = 0;
};

class BasicType : public Type {
public:
  BaseType type;

  explicit BasicType(BaseType t) : type(t) {}

  std::string toString() const override {
    switch (type) {
    case BaseType::INT:
      return "int";
    case BaseType::FLOAT:
      return "float";
    case BaseType::VOID:
      return "void";
    case BaseType::STRING:
      return "string";
    default:
      return "unknown";
    }
  }

  bool equals(const Type &other) const override {
    auto *other_basic = dynamic_cast<const BasicType *>(&other);
    return other_basic && other_basic->type == this->type;
  }
};

class ArrayType : public Type {
public:
  std::shared_ptr<Type> element_type;
  std::vector<int> dimensions; // 各维度大小，-1表示未知大小

  ArrayType(std::shared_ptr<Type> elem_type, std::vector<int> dims)
      : element_type(std::move(elem_type)), dimensions(std::move(dims)) {}

  std::string toString() const override {
    std::string result = element_type->toString();
    for (int dim : dimensions) {
      if (dim >= 0) {
        result += "[" + std::to_string(dim) + "]";
      } else {
        result += "[]";
      }
    }
    return result;
  }

  bool equals(const Type &other) const override {
    auto *other_array = dynamic_cast<const ArrayType *>(&other);
    if (!other_array)
      return false;

    if (!element_type->equals(*other_array->element_type))
      return false;

    if (dimensions.size() != other_array->dimensions.size())
      return false;

    for (size_t i = 0; i < dimensions.size(); ++i) {
      if (dimensions[i] != other_array->dimensions[i])
        return false;
    }

    return true;
  }
};

class FunctionType : public Type {
public:
  std::shared_ptr<Type> return_type;
  std::vector<std::shared_ptr<Type>> parameter_types;
  bool is_variadic; // 是否支持可变参数

  FunctionType(std::shared_ptr<Type> ret_type,
               std::vector<std::shared_ptr<Type>> param_types,
               bool variadic = false)
      : return_type(std::move(ret_type)),
        parameter_types(std::move(param_types)), is_variadic(variadic) {}

  std::string toString() const override {
    std::string result = return_type->toString() + "(";
    for (size_t i = 0; i < parameter_types.size(); ++i) {
      if (i > 0)
        result += ", ";
      result += parameter_types[i]->toString();
    }
    if (is_variadic) {
      if (!parameter_types.empty())
        result += ", ";
      result += "...";
    }
    result += ")";
    return result;
  }

  bool equals(const Type &other) const override {
    auto *other_func = dynamic_cast<const FunctionType *>(&other);
    if (!other_func)
      return false;

    if (!return_type->equals(*other_func->return_type))
      return false;

    if (is_variadic != other_func->is_variadic)
      return false;

    if (parameter_types.size() != other_func->parameter_types.size())
      return false;

    for (size_t i = 0; i < parameter_types.size(); ++i) {
      if (!parameter_types[i]->equals(*other_func->parameter_types[i])) {
        return false;
      }
    }

    return true;
  }
};

// 辅助函数
inline std::shared_ptr<Type> makeBasicType(BaseType type) {
  return std::make_shared<BasicType>(type);
}

inline std::shared_ptr<Type> makeArrayType(std::shared_ptr<Type> element_type,
                                           std::vector<int> dimensions) {
  return std::make_shared<ArrayType>(std::move(element_type),
                                     std::move(dimensions));
}

inline std::shared_ptr<Type>
makeFunctionType(std::shared_ptr<Type> return_type,
                 std::vector<std::shared_ptr<Type>> parameter_types,
                 bool is_variadic = false) {
  return std::make_shared<FunctionType>(std::move(return_type),
                                        std::move(parameter_types),
                                        static_cast<bool>(is_variadic));
}