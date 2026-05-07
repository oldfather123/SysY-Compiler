/**
 * @file priority_dequeue.h
 *
 * @brief 定义双向优先队列的头文件
 */

#pragma once
#include <deque>

namespace sysy {
template <typename value_type, bool (*less)(const value_type &, const value_type &)>
class priority_dequeue {
 private:
  using ContainerT = std::deque<value_type>;
  using iterator = typename ContainerT::iterator;
  ContainerT data;

 public:
  void push(const value_type &value) {
    unsigned begin = 0;
    unsigned end = data.size();

    while (begin != end) {
      auto mid = (begin + end) / 2;
      if (less(data[mid], value)) {
        begin = mid + 1;
      } else {
        end = mid;
      }
    }

    data.insert(begin + data.begin(), value);
  }

  template <typename... Args>
  auto emplace(Args... args) {
    return data.emplace(args...);
  }

  auto front() const -> const value_type & { return data.front(); }
  auto pop_front() { data.pop_front(); }
  auto back() const -> const value_type & { return data.back(); }
  void pop_back() { data.pop_back(); }

  auto size() const -> unsigned { return data.size(); }
  auto empty() const -> bool { return data.empty(); }
  auto begin() { return data.begin(); }
  auto end() { return data.end(); }
  auto erase(iterator pos) { data.erase(pos); }
};
}  // namespace sysy
