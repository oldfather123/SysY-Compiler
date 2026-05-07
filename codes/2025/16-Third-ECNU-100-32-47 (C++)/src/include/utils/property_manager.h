#ifndef AAAC_PROPERTY_MANAGER_H
#define AAAC_PROPERTY_MANAGER_H

#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>

namespace aaac {
namespace utils {

class BaseProperty {
public:
  virtual ~BaseProperty() = default;
  template <typename T> T &as() { return *dynamic_cast<T *>(this); }
};

class PropertyManager {
  using key_type = void *;
  std::unordered_map<
      key_type,
      std::unordered_map<std::type_index, std::unique_ptr<BaseProperty>>>
      props_;
  mutable std::shared_mutex mutex_;

public:
  ~PropertyManager() {
    for (auto it = props_.begin(); it != props_.end(); it++) {
      it->second.clear();
    }
    props_.clear();
  }

  template <typename T, typename Owner, typename... Args>
  T &init(Owner *owner, Args &&...args) {
    std::unique_lock lock(mutex_);
    auto &type_props = props_[owner];
    auto it = type_props.find(typeid(T));
    if (it == type_props.end()) {
      auto prop = std::make_unique<T>(std::forward<Args>(args)...);
      auto &ref = *prop;
      type_props.emplace(typeid(T), std::move(prop));
      return ref;
    }
    return static_cast<T &>(*it->second);
  }

  template <typename T, typename Owner> T *get(Owner *owner) {
    std::shared_lock lock(mutex_);
    void *non_const_owner =
        const_cast<void *>(static_cast<const void *>(owner));
    if (auto it = props_.find(non_const_owner); it != props_.end()) {
      if (auto type_it = it->second.find(typeid(T));
          type_it != it->second.end()) {
        return &type_it->second->template as<T>();
      }
    }
    return nullptr;
  }

  template <typename T, typename Owner> void remove(Owner *owner) {
    std::unique_lock lock(mutex_);
    if (auto it = props_.find(owner); it != props_.end()) {
      it->second.erase(typeid(T));
    }
  }

  template <typename Owner> void remove_all(Owner *owner) {
    std::unique_lock lock(mutex_);
    if (auto it = props_.find(owner); it != props_.end()) {
      it->second.clear();
      // props_.erase(it);
    }
  }
};

inline PropertyManager g_property_mgr;
} // namespace utils
} // namespace aaac
#endif // AAAC_PROPERTY_MANAGER_H
