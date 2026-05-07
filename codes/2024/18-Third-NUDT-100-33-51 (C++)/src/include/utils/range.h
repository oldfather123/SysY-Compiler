#pragma once

#include <iterator>

/**
 * @file range.h
 *
 * @brief 自定义迭代器的头文件
 */

namespace sysy {

/*!
 * \defgroup utility Utilities
 * @{
 */

/*!
 * \brief `range` 是一个简单的关于迭代器对[begin, end)的包装
 *
 * 使用示例：
 *
 * ```cpp
 *    vector<int> v = {1,2,3};
 *    auto rg = make_range(v);
 *    for (auto v : rg)
 *      cout << v << '\n';
 * ```
 */
template <typename IterT>
struct range {
  using iterator = IterT;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using value_type = typename std::iterator_traits<iterator>::value_type;
  using reference = typename std::iterator_traits<iterator>::reference;

 private:
  iterator b;  ///< 开头迭代器
  iterator e;  ///< 结尾迭代器

 public:
  explicit range(iterator b, iterator e) : b(b), e(e) {}
  auto rbegin() -> reverse_iterator {
    reverse_iterator iter(e);
    return iter;
  }
  auto rend() -> reverse_iterator {
    reverse_iterator iter(b);
    return iter;
  }
  auto begin() -> iterator { return b; }
  auto end() -> iterator { return e; }
  auto size() const { return std::distance(b, e); }
  auto empty() const { return b == e; }
};

//! 从寄存器对[begin, end)创建`range`对象
template <typename IterT>
auto make_range(IterT b, IterT e) -> range<IterT> {
  return range(b, e);
}
//! 从一个拥有`begin()`和`end()`方法的容器创建 `range`
template <typename ContainerT>
auto make_range(ContainerT &c) -> range<typename ContainerT::iterator> {
  return make_range(c.begin(), c.end());
}
//! 从一个拥有`begin()`和`end()`方法的容器创建 常量`range`
template <typename ContainerT>
auto make_range(const ContainerT &c) -> range<typename ContainerT::const_iterator> {
  return make_range(c.begin(), c.end());
}

/*!
 * @}
 */

}  // namespace sysy
