#ifndef GENERAL_H
#define GENERAL_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stdlib.h>
#include <unordered_map>
#include <utility>

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

template <typename T>
struct Array {
    uint64 len = 0;
    uint64 cap = 0;
    T* a = nullptr;
    Array() = default;
    Array(uint64 length) {
        setLen(length);
    }
    ~Array() {}
    void clear() {
        len = 0;
    }
    uint64 size() {
        return len;
    }
    bool empty() {
        return len == 0;
    }
    T& operator[](uint64 i) {
        return a[i];
    }
    Array<T> operator+(const Array& arr) const {
        Array<T> result(len + arr.len);
        if(len > 0) {
            memmove(result.a, a, sizeof(T) * len);
        }
        if(arr.len > 0) {
            memmove(result.a + len, arr.a, sizeof(T) * arr.len);
        }
        return result;
    }
    void push(T obj) {
        maygrow(1);
        a[len++] = obj;
    }
    void pop() {
        len--;
    }
    T front() {
        return a[0];
    }
    T back() {
        return a[len - 1];
    }
    T* begin() {
        return a;
    }
    T* end() {
        return a + len;
    }
    int find(T obj) {
        for(uint64 f = 0; f < len; f++) {
            if(a[f] == obj)
                return f;
        }
        return -1;
    }
    void insert(uint64 i, T obj) {
        maygrow(1);
        memmove(a + i + 1, a + i, sizeof(T) * (len - i));
        a[i] = obj;
        len++;
    }
    void remove(T obj) {
        uint64 p = find(obj);
        if(p != -1) {
            memmove(a + p, a + p + 1, sizeof(T) * (len - p - 1));
            len--;
        }
    }
    void setLen(uint64 newLen) {
        if(newLen == 0)
            return;
        if(cap < newLen)
            setCap(newLen);
        if(a) {
            for(int i = len; i < newLen; i++) {
                new(&a[i]) T;
            }
            len = newLen;
        } else {
            assert(false);
        }
    }
    void setCap(uint64 newCap) {
        grow(0, newCap);
    }
    void maygrow(uint64 moreLen) {
        if(!a || len + moreLen > cap)
            grow(moreLen, 0);
    }
    void grow(uint64 moreLen, uint64 minCap) {
        uint64 newLen = len + moreLen;
        uint64 newCap = minCap;
        if(newLen > newCap)
            newCap = newLen;
        if(newCap < cap * 2)
            newCap = cap * 2;
        if(newCap < 4)
            newCap = 4;
        if(newCap <= cap)
            return;
        cap = newCap;
        if(!a)
            len = 0;
        a = (T*)realloc(a, sizeof(T) * newCap);
    }
};

// NOLINTBEGIN
struct StringBuilder {
    Array<char> buffer;
    uint64 size() {
        return buffer.len;
    }
    char* c_str() {
        return buffer.a;
    }
    void addTerminator() {
        buffer.push('\0');
    }
    void append(const char* s, ...) {
        char buf[8192];
        va_list args;
        va_start(args, s);
        uint64 len = vsprintf(buf, s, args);
        va_end(args);
        buffer.maygrow(len + 1);  // +1 for \0
        strcpy(buffer.a + buffer.len, buf);
        buffer.len += len;
    }
};

#endif

// NOLINTEND
