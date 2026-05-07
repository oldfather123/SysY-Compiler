#ifndef IR_NUMBER_H
#define IR_NUMBER_H

#include <variant>

namespace ir {

class Number {
private:
  std::variant<int, float> _value;

public:
  explicit Number(int value) : _value(value) {}
  explicit Number(float value) : _value(value) {}
  Number(const Number &other) = default;
  Number(Number &&other) noexcept : _value(other._value) {}
  int intValue() const {
    if (std::holds_alternative<int>(_value))
      return std::get<int>(_value);
    else
      return static_cast<int>(std::get<float>(_value));
  }

  float floatValue() const {
    if (std::holds_alternative<int>(_value))
      return static_cast<float>(std::get<int>(_value));
    else
      return std::get<float>(_value);
  }

  std::variant<int, float> getValue() const { return _value; }

  Number &operator=(const Number &other) = default;
  Number &operator=(Number &&other) noexcept {
    _value = other._value;
    return *this;
  }
};

} // namespace ir

#endif