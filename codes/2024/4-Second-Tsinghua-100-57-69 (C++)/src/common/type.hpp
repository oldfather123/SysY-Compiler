#pragma once

#include <string>
#include <vector>
#include <assert.h>

enum ScalarType {
  Int,
  Float,
  String,
  Void
};

struct Type {
  int base_type;
  bool is_const;
  std::vector<int> dims; // 数组第一维可以是0

  int nr_dims() const { return dims.size(); }
  bool is_array() const { return dims.size() > 0; }
  int nr_elems() const {
    int count = 1;
    for (int n : dims)
      count *= n;
    return count;
  }
  int size() const { return nr_elems() * 4; }
  bool operator==(const Type &b) const {
    if (base_type != b.base_type)
      return false;
    if (nr_dims() != b.nr_dims())
      return false;
    for (int i = 0; i < nr_dims(); i++) {
      if (dims[i] != b.dims[i])
        return false;
    }
    return true;
  }
  bool operator!=(const Type &b) const { return !this->operator==(b); }
  Type get_pointer_type() const {
    Type new_type = *this;
    new_type.dims.insert(new_type.dims.begin(), 0);
    return new_type;
  }
  Type array2pointer() const {
    Type new_type = *this;
    new_type.dims[0] = 0;
    return new_type;
  }
  Type get_lower_dim() const {
    Type new_type = *this;
    if (new_type.dims.size() > 1)
      new_type.dims.erase(new_type.dims.begin() + 1);
    return new_type;
  }
  bool is_pointer() const { return dims.size() > 0 && dims[0] == 0; }
  bool is_pointer_to_scalar() const { return dims.size() == 1 && dims[0] == 0; }
  bool is_gp() const { return base_type == Int || is_array(); }

  Type() {}
  Type(int btype) : base_type{btype}, is_const{false} {}
  Type(int btype, bool const_qualified)
      : base_type{btype}, is_const{const_qualified} {}
  Type(int btype, std::vector<int> dimensions)
      : base_type{btype}, is_const{false}, dims{std::move(dimensions)} {}
  Type(Type type, std::vector<int> dimensions)
      : base_type{type.base_type}, is_const{false}, dims{
                                                        std::move(dimensions)} {
    assert(!type.is_array());
  }
};

inline std::string type_string(Type type) {
    if (type.base_type == Void)
        return "void";
    std::string s;
    if(type.base_type == Int)
        s = "i32";
    else if(type.base_type == Float)
        s = "float";
    else
        s = "unknown"; // string
    if (!type.is_array())
        return s;
    bool pointer = false;
    int i = 0;
    if(type.dims[0] == 0){
        pointer = true;
        i = 1;
    }
    if (type.dims.size() != 1 || !pointer) {
        for (; i < type.nr_dims(); i++)
            s += "[" + std::to_string(type.dims[i]) + "]";
    }
    if (pointer)
        s += "*";
    return s;
}

inline std::string type_llvm_string(Type type, bool no_ptr = false) {
    if (type.base_type == Void)
        return "void";
    std::string s;
    if(type.base_type == Int)
        s = "i32";
    else if(type.base_type == Float)
        s = "float";
    else
        s = "i32"; // string
    if (!type.is_array())
        return s;
    bool pointer = false;
    int i = 0;
    if(type.dims[0] == 0){
        pointer = true;
        i = 1;
    }
    if (pointer && !no_ptr)
        s += "*";
    return s;
}