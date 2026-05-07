#ifndef AAAC_MEMORY_MANAGER_H
#define AAAC_MEMORY_MANAGER_H

#include "../common.h"

#include <list>

namespace aaac {
namespace utils {
/**
 * @brief 对象管理器
 *
 * @tparam T 需要被管理的对象的类型
 *
 * 这个工具类用于处理全局性的对象的内存管理，如Block，由于全局的作用域不属于其他任何一个对象，使用这个来管理更方便一些
 *
 * @deprecated
 * 改为使用shared_ptr,weak_ptr,unique_ptr来管理内存
 */
template <typename T> class MemoryManager {
private:
  std::list<T *> objects;

public:
  MemoryManager() = default;
  ~MemoryManager() {
    for (auto ptr : objects) {
      delete ptr;
    }
  }

  /**
   * @brief 创建一个新的T类型对象
   *
   * 这个对象的delete由MemoryManager管理，当MemoryManager析构时，会自动释放所有对象
   */
  template <typename... Args> T *createObject(Args &&...args) {
    T *obj = new T(std::forward<Args>(args)...);
    objects.push_back(obj);
    return obj;
  }
};
} // namespace utils
} // namespace aaac

#endif // AAAC_MEMORY_MANAGER_H
