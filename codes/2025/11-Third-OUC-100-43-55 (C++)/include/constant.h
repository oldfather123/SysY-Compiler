#ifndef IR_CONSTANT_H
#define IR_CONSTANT_H

#include "number.h"
#include "value.h"
#include <stdexcept>
#include <vector>

namespace ir {

enum class ConstantKind { Number, Zero, Array };

class Constant : public Value {
public:
  using Value::Value;
  ~Constant() override;

  virtual ConstantKind getConstantKind() const = 0;
  ValueKind getValueKind() const override { return ValueKind::Const; }

  bool isNumber() const { return getConstantKind() == ConstantKind::Number; }
  bool isZero() const { return getConstantKind() == ConstantKind::Zero; }
  bool isArray() const { return getConstantKind() == ConstantKind::Array; }
};

class ConstantNumber : public Constant {
private:
  Number _value;

  static std::unique_ptr<BasicType> determineType(const Number &num);
  // BasicKind getBasicKind() const;
  static BasicKind autoTypePromotion(BasicKind type1, BasicKind type2);

public:
  explicit ConstantNumber(bool value);
  explicit ConstantNumber(const Number &num);
  ConstantNumber(ConstantNumber &&other) noexcept;

  ValueKind getValueKind() const override { return ValueKind::ConstNum; }
  ConstantKind getConstantKind() const override { return ConstantKind::Number; }

  Number getValue() const;
  float floatValue() const;
  int intValue() const;

  ConstantNumber operator+(const ConstantNumber &rhs) const;
  ConstantNumber operator-(const ConstantNumber &rhs) const;
  ConstantNumber operator*(const ConstantNumber &rhs) const;
  ConstantNumber operator/(const ConstantNumber &rhs) const;
  ConstantNumber operator%(const ConstantNumber &rhs) const;
  ConstantNumber operator^(const ConstantNumber &rhs) const;
  ConstantNumber operator-() const;
  ConstantNumber operator!() const;
  ConstantNumber operator==(const ConstantNumber &rhs) const;
  ConstantNumber operator!=(const ConstantNumber &rhs) const;
  ConstantNumber operator>(const ConstantNumber &rhs) const;
  ConstantNumber operator>=(const ConstantNumber &rhs) const;
  ConstantNumber operator<(const ConstantNumber &rhs) const;
  ConstantNumber operator<=(const ConstantNumber &rhs) const;

  std::string getLiteralStr() const;
  std::string getName() const override;
  std::string str() const override;
};

class ConstantZero : public Constant {
public:
  using Constant::Constant;

  ConstantKind getConstantKind() const override { return ConstantKind::Zero; }
  ValueKind getValueKind() const override { return ValueKind::ConstZero; }
  std::string getName() const override;
  std::string str() const override;
};

class ConstantArray : public Constant {
private:
  std::vector<std::unique_ptr<Constant>> _values;

public:
  ConstantArray(std::unique_ptr<Type> type,
                std::vector<std::unique_ptr<Constant>> values)
      : Constant(std::move(type)), _values(std::move(values)) {}

  ConstantKind getConstantKind() const override { return ConstantKind::Array; }

  Constant *getValue(size_t index) const;

  std::string getName() const override;
  std::string str() const override;
};

} // namespace ir

#endif // IR_CONSTANT_H