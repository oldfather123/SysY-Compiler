#pragma once

#include <vector>

class ConstantCounter {
 private:
  union Constant {
    int intValue;
    float floatValue;
  };
  unsigned __size{};                        ///< 总的字面值数量
  std::vector<Constant> __counterConstant;  ///< 记录的字面值列表(无重复元素)
  std::vector<unsigned> __counterNumbers;   ///< 记录的字面值重复数量列表

 public:
  ConstantCounter() = default;

 public:
  auto size() const -> unsigned { return __size; }  ///< 返回总的字面值数量
  auto getValue(unsigned index) const -> Constant;  ///< 根据位置index获取Constant
  auto getValues() const -> const std::vector<Constant> & { return __counterConstant; }  ///< 获取互异Value *列表
  auto getNumbers() const -> const std::vector<unsigned> & { return __counterNumbers; }  ///< 获取Value *重复数量列表
  void push_back(int value, unsigned num = 1);                                           ///< 向后插入num个value
  void push_back(float value, unsigned num = 1);                                         ///< 向后插入num个value
  void clear() {
    __size = 0;
    __counterConstant.clear();
    __counterNumbers.clear();
  }  ///< 清空ValueCounter
};