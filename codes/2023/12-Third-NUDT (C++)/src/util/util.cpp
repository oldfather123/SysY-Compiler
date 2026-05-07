#include "util.hpp"

#include <algorithm>
#include <iostream>

#include "error.hpp"

namespace Util {

int32_t parseInt(const std::string &literal) {
  const int64_t INTMAX = 2147483647ll;
  const int64_t INTMIN = -2147483648ll;
  const std::string oct = "0";
  const std::string hex = "0X";
  std::string int_literal(literal);
  std::transform(literal.begin(), literal.end(), int_literal.begin(), ::toupper);
  int64_t ret = 0;
  if (std::equal(hex.begin(), hex.end(), int_literal.begin())) {
    for (size_t i = 2; i < int_literal.length(); i++) {
      ret *= 16;
      if ('0' <= int_literal[i] && int_literal[i] <= '9')
        ret += int_literal[i] - '0';
      else
        ret += int_literal[i] - 'A' + 10;
      if (ret > INTMAX)
        throw LogicError("int overflow");
      else if (ret < INTMIN)
        throw LogicError("int downflow");
    }
  } else if (std::equal(oct.begin(), oct.end(), int_literal.begin())) {
    for (size_t i = 1; i < int_literal.length(); i++) {
      ret = ret * 8 + int_literal[i] - '0';
      if (ret > INTMAX)
        throw LogicError("int overflow");
      else if (ret < INTMIN)
        throw LogicError("int downflow");
    }
  } else {
    for (auto ch : int_literal) {
      ret = ret * 10 + ch - '0';
      if (ret > INTMAX)
        throw LogicError("int overflow");
      else if (ret < INTMIN)
        throw LogicError("int downflow");
    }
  }
  return static_cast<int32_t>(ret);
}

}  // namespace Util
