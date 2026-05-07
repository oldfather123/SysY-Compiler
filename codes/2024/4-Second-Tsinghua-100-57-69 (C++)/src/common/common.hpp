#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <stack>
#include <tuple>

#include "common/type.hpp"

#define TypeCase(res, type, expr) if (auto res = dynamic_cast<type>(expr))

enum class UnaryOp { Add, Sub, Not };

enum class BinaryOp {
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Eq,
  Neq,
  Lt,
  Gt,
  Leq,
  Geq,
  And,
  Or,
  Shr,
  Shl,
  Ashl,
  NR_OPS
};

// 此ConstValue特指标量
struct ConstValue {
  int type;
  union { int iv; float fv; };

  ConstValue() {}
  ConstValue(int v) : type{Int} { iv = v; }
  ConstValue(float v) : type{Float} { fv = v; }

  inline ConstValue getNot() const { return type == Int ? (int)(iv == 0) : (int)(fv == 0); }
  inline ConstValue getNeg() const { return type == Int ? ConstValue(-iv) : ConstValue(-fv); }
  bool operator == (const ConstValue &b) const {
    if (type!= b.type) return false;
    if (type == Int) return iv == b.iv;
    return fv == b.fv;
  }
  bool operator != (const ConstValue &b) const { return !(this->operator==(b)); }
  std::string to_string() const { return type == Int ? std::to_string(iv) : std::to_string(fv); }
};

namespace std {
template <> class hash<ConstValue> {
public:
  size_t operator()(const ConstValue &r) const { return r.iv; }
};
} // namespace std

// variable or constant
struct Var {
  Type type;
  std::optional<ConstValue> val;
  std::unique_ptr<std::map<int, ConstValue>>
      arr_val; // index -> value，未记录的项全部初始化为0

  Var() {}
  Var(Type type_) : type{std::move(type_)} {}
  Var(Type type_, std::optional<ConstValue> value)
      : type{std::move(type_)}, val{std::move(value)} {}
};

namespace std {
template <class T> struct hash<vector<T>> {
  size_t operator()(const vector<T> &r) const {
    size_t res = 0;
    for (auto t : r) {
      res = res * 1221821 + hash<T>()(t);
    }
    return res;
  }
};
template <> class hash<Type> {
public:
  size_t operator()(const Type &r) const {
    return hash<int>()(r.base_type) * 17 + hash<std::vector<int>>()(r.dims);
  }
};
} // namespace std

inline bool is_power_of_2(int x) { return (x & (x - 1)) == 0; }
