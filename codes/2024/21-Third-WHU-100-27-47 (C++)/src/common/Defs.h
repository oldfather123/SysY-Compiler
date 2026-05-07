#pragma once

/* 常用头文件 */
#include "antlr4-runtime.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <stdint.h>
#include <set>
#include <stdexcept>
#include <stack>
#include <bitset>
#include <limits>
#include <cstring>
#include <queue>

#pragma region 调试输出

class NullBuffer : public std::streambuf {
public:
    int overflow(int c) override {
        return c;
    }
};
class NullStream : public std::ostream {
public:
    NullStream() : std::ostream(&m_buffer) { }

private:
    NullBuffer m_buffer;
};
extern NullStream null_out;

#ifdef DBGOUT_DISABLED        // 在 Common.h 的开头定义该宏即可将 dbgout 禁用（重定向到空输出流）
#define dbgout 0 && std::clog // 丢弃输出
#else
#define dbgout std::clog
#endif

#pragma endregion

#pragma region 断言

#ifdef assert
#undef assert
#endif

// 调试断言，发生在调用者非法操作时，发生时直接中止程序
#ifdef NDEBUG
#define dbgassert(condition, message) 0
#else
#define dbgassert(condition, message)                                        \
    (!(condition)) ? (dbgout << "Assertion failed: (" << #condition << "), " \
                             << "function " << __FUNCTION__                  \
                             << ", file " << __FILE__                        \
                             << ", line " << __LINE__ << "."                 \
                             << std::endl                                    \
                             << message << std::endl,                        \
                      abort(), 0)                                            \
                   : 1
#endif

// 异常断言，发生在用户输入有误时，发生时抛出异常，必须用在 try-catch 逻辑里
#define excassert(condition, message) \
    (!(condition)) ? throw(std::runtime_error(message)), 0 : 1

#pragma endregion

#pragma region 别名

#define makePtr std::make_shared
#define thisPtr(T) std::enable_shared_from_this<T>::shared_from_this()

#define castPtr std::dynamic_pointer_cast
using String = std::string;
template <typename T>
using Ptr = std::shared_ptr<T>;
template <typename T>
using Vector = std::vector<T>;
template <typename T>
using Set = std::unordered_set<T>;
template <typename T>
using OrderedSet = std::set<T>;
template <typename TKey, typename TValue>
using Map = std::unordered_map<TKey, TValue>;
template <typename TKey, typename TValue>
using OrderedMap = std::map<TKey, TValue>;

#define BITSET_SIZE 100000
using Bitset = std::bitset<BITSET_SIZE>;

#pragma endregion

#pragma region 方法

template <typename T>
Set<T> toSet(const Vector<T> &vec) {
    return Set<T>(vec.begin(), vec.end());
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#ifdef ANTLR_4_9_3
#define AS(obj, T) obj.as<T>()
#define IS(T) is<T>()
#else
#define AS(obj, T) std::any_cast<T>(obj)
#define IS(T) type() == typeid(T)
#endif

template <class T>
inline void hashCombine(std::size_t &s, const T &v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

#pragma endregion

#pragma region 通用接口

#pragma endregion

#pragma region 前向声明

namespace ir {
    class BasicBlock;
    class Function;
    class Instruction;
    class Register;
    using BBPtr = Ptr<BasicBlock>;
    using FuncPtr = Ptr<Function>;
    using InstPtr = Ptr<Instruction>;
    using RegPtr = Ptr<Register>;
}

namespace opt {
    class peephole;
}

namespace backend {
    // riscv
    class Register;
    class GeneralRegister;
    class FloatRegister;

    class RiscBasicBlock;
    class RiscFunction;
    class RiscModule;
    class Instruction;
    class Operand;

    // arm
    enum class ArmReg;
    class ArmInstruction;
    class ArmBasicBlock;
    enum class ArmCond;
    struct ArmShift;
}

#pragma endregion
