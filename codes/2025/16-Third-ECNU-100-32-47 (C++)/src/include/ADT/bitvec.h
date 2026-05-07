#ifndef AAAC_ADT_BITMAP_H
#define AAAC_ADT_BITMAP_H

#include "common.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace aaac {
namespace ADT {
class BitVec {
  friend BitVec operator&(const BitVec& lhs, const BitVec& rhs);
friend BitVec operator|(const BitVec& lhs, const BitVec& rhs);

private:
  std::vector<uint64_t> bits;

public:
  BitVec(size_t size) : bits(size / 64 + 1, 0) {} // 默认是clear all
  void set(size_t idx) { bits[idx / 64] |= (1ULL << (idx % 64)); }
  void clear(size_t idx) { bits[idx / 64] &= ~(1ULL << (idx % 64)); }
  bool test(size_t idx) { return bits[idx / 64] & (1ULL << (idx % 64)); }
  size_t size() { return bits.size(); }
  void resize(size_t size) { bits.resize(size / 64 + 1, 0); }
  void clear_all() {
    for (auto &b : bits)
      b = 0;
  }
  void set_all() {
    for (auto &b : bits)
      b = 0xFFFFFFFFFFFFFFFF;
  }

    // 交集操作符 &=
    BitVec& operator&=(const BitVec& other) {
      Assert(bits.size()==other.bits.size(), "Bitvec size mismatch");
      for (size_t i = 0; i < bits.size(); ++i) {
        bits[i] &= other.bits[i];
      }
      return *this;
    }
  
    // 并集操作符 |=
    BitVec& operator|=(const BitVec& other) {
      Assert(bits.size()==other.bits.size(), "Bitvec size mismatch");
      for (size_t i = 0; i < bits.size(); ++i) {
        bits[i] |= other.bits[i];
      }
      return *this;
    }
  
    // 补集操作符 ~（返回新对象）
    BitVec operator~() const {
      BitVec result(*this);
      for (auto& block : result.bits) {
        block = ~block;
      }
      return result;
    }

    bool operator==(const BitVec& other) const {
      if (bits.size() != other.bits.size()) {
        return false;
      }
      for (size_t i = 0; i < bits.size(); ++i) {
        if (bits[i] != other.bits[i]) {
          return false;
        }
      }
      return true;
    }

    bool operator!=(const BitVec& other) const {
      return !(*this == other);
    }
    
};

inline BitVec operator&(const BitVec& lhs, const BitVec& rhs) {
  BitVec result = lhs;
  result &= rhs;
  return result;
}

inline BitVec operator|(const BitVec& lhs, const BitVec& rhs) {
  BitVec result = lhs;
  result |= rhs; 
  return result;
}
} // namespace ADT
} // namespace aaac

#endif // AAAC_ADT_BITMAP_H
