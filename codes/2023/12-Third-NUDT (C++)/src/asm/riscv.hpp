#ifndef __RISCV_HPP__
#define __RISCV_HPP__

#include <bits/stdc++.h>

namespace RISCV {

constexpr size_t x0 = 0;    // Hardwired zero
constexpr size_t x1 = 1;    // Return address
constexpr size_t x2 = 2;    // Stack pointer
constexpr size_t x3 = 3;    // Global pointer
constexpr size_t x4 = 4;    // Thread pointer
constexpr size_t x5 = 5;    // Temporary
constexpr size_t x6 = 6;    // Temporary
constexpr size_t x7 = 7;    // Temporary
constexpr size_t x8 = 8;    // Saved register, frame pointer
constexpr size_t x9 = 9;    // Saved register
constexpr size_t x10 = 10;  // Function argument, return value
constexpr size_t x11 = 11;  // Function argument, return value
constexpr size_t x12 = 12;  // Function argument
constexpr size_t x13 = 13;  // Function argument
constexpr size_t x14 = 14;  // Function argument
constexpr size_t x15 = 15;  // Function argument
constexpr size_t x16 = 16;  // Function argument
constexpr size_t x17 = 17;  // Function argument
constexpr size_t x18 = 18;  // Saved register
constexpr size_t x19 = 19;  // Saved register
constexpr size_t x20 = 20;  // Saved register
constexpr size_t x21 = 21;  // Saved register
constexpr size_t x22 = 22;  // Saved register
constexpr size_t x23 = 23;  // Saved register
constexpr size_t x24 = 24;  // Saved register
constexpr size_t x25 = 25;  // Saved register
constexpr size_t x26 = 26;  // Saved register
constexpr size_t x27 = 27;  // Saved register
constexpr size_t x28 = 28;  // Temporary
constexpr size_t x29 = 29;  // Temporary
constexpr size_t x30 = 30;  // Temporary
constexpr size_t x31 = 31;  // Temporary

constexpr size_t zero = x0;  // Hardwired zero
constexpr size_t ra = x1;    // Return address
constexpr size_t sp = x2;    // Stack pointer
constexpr size_t gp = x3;    // Global pointer
constexpr size_t tp = x4;    // Thread pointer
constexpr size_t t0 = x5;    // Temporary
constexpr size_t t1 = x6;    // Temporary
constexpr size_t t2 = x7;    // Temporary
constexpr size_t s0 = x8;    // Saved register
constexpr size_t fp = x8;    // frame pointer
constexpr size_t s1 = x9;    // Saved register
constexpr size_t a0 = x10;   // Function argument, return value
constexpr size_t a1 = x11;   // Function argument, return value
constexpr size_t a2 = x12;   // Function argument
constexpr size_t a3 = x13;   // Function argument
constexpr size_t a4 = x14;   // Function argument
constexpr size_t a5 = x15;   // Function argument
constexpr size_t a6 = x16;   // Function argument
constexpr size_t a7 = x17;   // Function argument
constexpr size_t s2 = x18;   // Saved register
constexpr size_t s3 = x19;   // Saved register
constexpr size_t s4 = x20;   // Saved register
constexpr size_t s5 = x21;   // Saved register
constexpr size_t s6 = x22;   // Saved register
constexpr size_t s7 = x23;   // Saved register
constexpr size_t s8 = x24;   // Saved register
constexpr size_t s9 = x25;   // Saved register
constexpr size_t s10 = x26;  // Saved register
constexpr size_t s11 = x27;  // Saved register
constexpr size_t t3 = x28;   // Temporary
constexpr size_t t4 = x29;   // Temporary
constexpr size_t t5 = x30;   // Temporary
constexpr size_t t6 = x31;   // Temporary

using Reg = enum Reg : size_t {
  r0 = 0,
  r1 = 1,
  r2 = 2,
  r3 = 3,
  r4 = 4,
  r5 = 5,
  r6 = 6,
  r7 = 7,
  r8 = 8,
  r9 = 9,
  r10 = 10,
  r11 = 11,
  r12 = 12,
  r13 = 13,
  r14 = 14,
  r15 = 15,
  r16 = 16,
  r17 = 17,
  r18 = 18,
  r19 = 19,
  r20 = 20,
  r21 = 21,
  r22 = 22,
  r23 = 23,
  r24 = 24,
  r25 = 25,
  r26 = 26,
  r27 = 27,
  r28 = 28,
  r29 = 29,
  r30 = 30,
  r31 = 31,
};

}  // namespace RISCV

#endif
