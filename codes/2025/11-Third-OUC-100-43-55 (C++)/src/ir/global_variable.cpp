#include "global_variable.h"
#include "core.h"
#include <stdexcept>
#include <utility>

namespace ir {

GlobalVariable::GlobalVariable(std::unique_ptr<Type> type, std::string name,
                               bool isConst, std::unique_ptr<Constant> value)
    : Value(std::move(type)), _name(std::move(name)), _isConst(isConst),
      _value(std::move(value)) {}

bool GlobalVariable::isSingle() const { return this->getType()->isBasic(); }

bool GlobalVariable::isInBss() const { return _value && _value->isZero(); }

int GlobalVariable::getInt() const {
  assert(_value && "GlobalVariable has no value");

  switch (_value->getConstantKind()) {
  case ConstantKind::Number:
    return static_cast<ConstantNumber *>(_value.get())->intValue();
  case ConstantKind::Zero:
    return 0;
  default:
    throw std::runtime_error("getInt() called on non-scalar constant");
  }
}

float GlobalVariable::getFloat() const {
  assert(_value && "GlobalVariable has no value");

  switch (_value->getConstantKind()) {
  case ConstantKind::Number:
    return static_cast<ConstantNumber *>(_value.get())->floatValue();
  case ConstantKind::Zero:
    return 0.0f;
  default:
    throw std::runtime_error("getFloat() called on non-scalar constant");
  }
}

int GlobalVariable::getInt(size_t index) const {
  assert(getType()->isArray());
  auto arrayType = static_cast<ArrayType *>(getType());
  std::vector<size_t> dimensions = arrayType->getDimensions();
  dimensions.erase(dimensions.begin());
  dimensions.push_back(1);
  Constant *curValue = _value.get();
  for (int i = 0; i < dimensions.size(); i++) {
    const int dimension = dimensions[i];
    int num = 1;
    for (int j = i; j < dimensions.size(); j++) {
      num *= dimensions[j];
    }
    switch (curValue->getConstantKind()) {
    case ConstantKind::Array:
      curValue = static_cast<ConstantArray *>(curValue)->getValue(index / num);
      index = index % dimension;
      break;
    case ConstantKind::Zero:
      return 0;
    default:
      throw std::runtime_error("Unexpected value in getInt()");
    }
  }
  assert(curValue->isNumber() && "getInt() requires a ConstantNumber");
  return static_cast<ConstantNumber *>(curValue)->intValue();
}

// Careful! Might be a wrong implementation. (ATTENTION)
float GlobalVariable::getFloat(size_t index) const {
  assert(getType()->isArray());
  auto arrayType = static_cast<ArrayType *>(getType());
  std::vector<size_t> dimensions = arrayType->getDimensions();
  Constant *curValue = _value.get();
  for (int dimension : dimensions) {
    switch (curValue->getConstantKind()) {
    case ConstantKind::Array:
      curValue =
          static_cast<ConstantArray *>(curValue)->getValue(index / dimension);
      index = index % dimension;
      break;
    case ConstantKind::Zero:
      return 0;
    default:
      throw std::runtime_error("Unexpected value in getFloat()");
    }
  }
  assert(curValue->isNumber() && "getFloat() requires a ConstantNumber");
  return static_cast<ConstantNumber *>(curValue)->floatValue();
}

std::string GlobalVariable::getRawName() const { return _name; }

std::string GlobalVariable::getName() const {
  return fmt::format("@{}", _name);
}

std::string GlobalVariable::str() const {
  return fmt::format("{} = global {}", getName(), _value->str());
}

} // namespace ir
