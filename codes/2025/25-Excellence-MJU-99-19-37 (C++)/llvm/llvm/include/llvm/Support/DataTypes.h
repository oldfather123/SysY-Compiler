#ifndef LLVM_SUPPORT_DATATYPES_H
#define LLVM_SUPPORT_DATATYPES_H

// 包含标准C++类型定义
#include <cstddef>    // for size_t, ptrdiff_t
#include <cstdint>    // for uintptr_t, uint32_t, uint64_t, etc.

// 确保我们有必要的类型定义
using std::size_t;
using std::ptrdiff_t;
using std::uintptr_t;
using std::intptr_t;

// 常用的整数类型
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

#endif // LLVM_SUPPORT_DATATYPES_H
