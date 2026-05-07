#include "../include/utils/ConstantCounter.h"
#include <cassert>

auto ConstantCounter::getValue(unsigned index) const -> Constant {
  assert(index < __size);
  unsigned num = 0;
  for (unsigned i = 0; i < __counterNumbers.size(); i++) {
    if (num <= index && index < num + __counterNumbers[i]) {
      return __counterConstant[i];
    }
    num += __counterNumbers[i];
  }
  assert(false);
}

void ConstantCounter::push_back(int value, unsigned num) {
  if (__size != 0 && __counterConstant.back().intValue == value) {
    *(__counterNumbers.end() - 1) += num;
  } else {
    Constant tmp;
    tmp.intValue = value;
    __counterConstant.push_back(tmp);
    __counterNumbers.push_back(num);
  }
  __size += num;
}

void ConstantCounter::push_back(float value, unsigned num) {
  if (__size != 0 && __counterConstant.back().floatValue == value) {
    *(__counterNumbers.end() - 1) += num;
  } else {
    Constant tmp;
    tmp.floatValue = value;
    __counterConstant.push_back(tmp);
    __counterNumbers.push_back(num);
  }
  __size += num;
}