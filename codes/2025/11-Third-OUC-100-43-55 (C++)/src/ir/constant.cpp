#include "constant.h"
#include "type.h"
#include "value.h"
#include "core.h"

#include <cassert>
#include <utility>

namespace ir {

Constant::~Constant() = default;

std::unique_ptr<BasicType> ConstantNumber::determineType(const Number &num) {
  const auto &value = num.getValue();
  if (std::holds_alternative<int>(value))
    return std::make_unique<BasicType>(BasicKind::I32);
  else if (std::holds_alternative<float>(value))
    return std::make_unique<BasicType>(BasicKind::F32);
  else
    throw std::runtime_error("Unsupported number type");
}

BasicKind ConstantNumber::autoTypePromotion(BasicKind type1, BasicKind type2) {
  if (type1 == BasicKind::F32 || type2 == BasicKind::F32)
    return BasicKind::F32;
  if (type1 == BasicKind::I32 || type2 == BasicKind::I32)
    return BasicKind::I32;
  return BasicKind::I1;
}

ConstantNumber::ConstantNumber(bool value)
    : Constant(std::make_unique<BasicType>(BasicKind::I1)),
      _value(value ? 1 : 0) {}

ConstantNumber::ConstantNumber(const Number &num)
    : Constant(determineType(num)), _value(num) {}

// Probably should just move the type (ATTENTION)
ConstantNumber::ConstantNumber(ConstantNumber &&other) noexcept
    : Constant(std::move(other.getType()->clone())),
      _value(std::move(other._value)) {}

Number ConstantNumber::getValue() const { return _value; }

int ConstantNumber::intValue() const { return _value.intValue(); }

float ConstantNumber::floatValue() const { return _value.floatValue(); }

// BasicKind ConstantNumber::getBasicKind() const {
//   assert(getType()->isBasic());
//   return static_cast<BasicType *>(getType())->getBasicKind();
// }

std::string ConstantNumber::getLiteralStr() const {
  auto basicKind = static_cast<BasicType *>(getType())->getBasicKind();
  switch (basicKind) {
  case BasicKind::I1:
    return intValue() ? "true" : "false";
  case BasicKind::I32:
    return std::to_string(intValue());
  case BasicKind::F32: {
    auto value = static_cast<double>(floatValue());
    return fmt::format("0x{:X}", *reinterpret_cast<unsigned long long *>(&value));
    // return std::to_string(floatValue());
  }
  default:
    throw std::runtime_error("Unexpected type");
  }
}

std::string ConstantNumber::str() const {
  return fmt::format("{} {}", getType()->str(), getLiteralStr());
}

std::string ConstantNumber::getName() const { return getLiteralStr(); }

// General operators for integer/float

#define DEFINE_BINARY_OP(OPNAME, OP)                                           \
  ConstantNumber ConstantNumber::operator OPNAME(const ConstantNumber &rhs)    \
      const {                                                                  \
    auto basicKind1 = static_cast<BasicType *>(getType())->getBasicKind();     \
    auto basicKind2 = static_cast<BasicType *>(rhs.getType())->getBasicKind(); \
    auto basicKind = autoTypePromotion(basicKind1, basicKind2);                \
    switch (basicKind) {                                                       \
    case BasicKind::I1:                                                        \
    case BasicKind::I32:                                                       \
      return ConstantNumber(Number(intValue() OP rhs.intValue()));             \
    case BasicKind::F32:                                                       \
      return ConstantNumber(Number(floatValue() OP rhs.floatValue()));         \
    default:                                                                   \
      throw std::runtime_error("Unsupported type in " #OPNAME);                \
    }                                                                          \
  }

DEFINE_BINARY_OP(+, +)
DEFINE_BINARY_OP(-, -)
DEFINE_BINARY_OP(*, *)
DEFINE_BINARY_OP(/, /)
DEFINE_BINARY_OP(==, ==)
DEFINE_BINARY_OP(!=, !=)
DEFINE_BINARY_OP(>, >)
DEFINE_BINARY_OP(>=, >=)
DEFINE_BINARY_OP(<, <)
DEFINE_BINARY_OP(<=, <=)

// Operators for only integer

ConstantNumber ConstantNumber::operator%(const ConstantNumber &rhs) const {
  auto basicKind = static_cast<BasicType *>(getType())->getBasicKind();
  switch (basicKind) {
  case BasicKind::I1:
  case BasicKind::I32:
    return ConstantNumber(Number(intValue() % rhs.intValue()));
  default:
    throw std::runtime_error("Modulo not supported for this type");
  }
}

ConstantNumber ConstantNumber::operator^(const ConstantNumber &rhs) const {
  auto basicKind = static_cast<BasicType *>(getType())->getBasicKind();
  switch (basicKind) {
  case BasicKind::I1:
    return ConstantNumber((intValue() ^ rhs.intValue()) != 0);
  case BasicKind::I32:
    return ConstantNumber(Number(intValue() ^ rhs.intValue()));
  default:
    throw std::runtime_error("Bitwise XOR not supported for this type");
  }
}

// Unary
ConstantNumber ConstantNumber::operator-() const {
  auto basicKind = static_cast<BasicType *>(getType())->getBasicKind();
  switch (basicKind) {
  case BasicKind::I1:
  case BasicKind::I32:
    return ConstantNumber(Number(-intValue()));
  case BasicKind::F32:
    return ConstantNumber(Number(-floatValue()));
  default:
    throw std::runtime_error("Unary minus not supported for this type");
  }
}

ConstantNumber ConstantNumber::operator!() const {
  auto basicKind = static_cast<BasicType *>(getType())->getBasicKind();
  switch (basicKind) {
  case BasicKind::I1:
  case BasicKind::I32:
    return ConstantNumber(intValue() == 0);
  case BasicKind::F32:
    return ConstantNumber(floatValue() == 0.0f);
  default:
    throw std::runtime_error("Logical not not supported for this type");
  }
}

std::string ConstantZero::getName() const { return "zeroinitializer"; }

std::string ConstantZero::str() const {
  return fmt::format("{} {}", getType()->str(), getName());
}

Constant *ConstantArray::getValue(size_t index) const {
  assert(index < _values.size());
  return _values[index].get();
}

std::string ConstantArray::getName() const {
  fmt::memory_buffer buf;
  buf.push_back('[');

  for (size_t i = 0; i < _values.size(); ++i) {
    if (i > 0) {
      buf.append(std::string_view(", "));
    }
    buf.append(_values[i]->str());
  }

  buf.push_back(']');
  return fmt::to_string(buf);
}

std::string ConstantArray::str() const {
  return fmt::format("{} {}", getType()->str(), getName());
}

#undef DEFINE_BINARY_OP
} // namespace ir
