#pragma once

#include <cassert>
#include <string>

/*
编号        别名       名称                用途简述
x0          zero      零寄存器             始终为 0，读取恒为 0，写入无效
x1          ra        返回地址             函数返回地址
x2          sp        栈指针               指向当前栈顶
x3          gp        全局指针             全局变量基址
x4          tp        线程指针             指向当前线程
x5 - x7     t0 - t2   临时寄存器           临时使用（不会保存）
x8          s0/fp     保存寄存器0/帧指针   可被调用者保存；也可作帧指针使用
x9 - x10	s1/s2     保存寄存器           函数间传递变量时使用
x10 - x17   a0 - a7   参数                函数参数 / 返回值
x18 - x27   s2 - s11  保存寄存器           调用者可依赖其值不被破坏
x28 - x31   t3 - t6   临时寄存器           临时使用

编号     寄存器名   ABI名     用途
f0-f7    f0-f7     ft0-ft7   临时寄存器
f8-f9    f8-f9     fs0-ft1   被调用者保存
f10      f10       fa0       函数参数 / 返回值 0
f11      f11       fa1       函数参数 / 返回值 1
f12-f17  f12-f17   fa2-ft7   函数参数
f18-f27  f18-f27   fs2-ft11  被调用者保存
f28-f31  f28-f31   ft8-ft11  临时寄存器
*/


struct Reg {
    unsigned id;

    explicit Reg(unsigned i) : id(i) { assert(i <= 31); }
    bool operator==(const Reg &other) { return id == other.id; }

    std::string print() const;

    static Reg zero() { return Reg(0); }
    static Reg ra() { return Reg(1); }
    static Reg sp() { return Reg(2); }
    static Reg fp() { return Reg(8); }
    static Reg a(unsigned i) {
        assert(i <= 7);
        return Reg(i + 10);
    }
    static Reg t(unsigned i) {
        assert(i <= 6);
        if(i <=2 ) return Reg(5 + i);       // t0–t2
        return Reg(28 + i - 3);             // t3–t6
    }
    static Reg s(unsigned i) {
        assert(i <= 11);
        if (i <= 1) return Reg(8 + i);      // s0, s1
        return Reg(i + 16);                 // s2–s11 
    }
};

struct FReg {
    unsigned id;

    explicit FReg(unsigned i) : id(i) { assert(i <= 31); }
    bool operator==(const FReg &other) { return id == other.id; }

    std::string print() const;

    static FReg fa(unsigned i) {
        assert(i <= 7);           // fa0–fa7
        return FReg(10 + i);      // f10–f17
    }

    static FReg ft(unsigned i) {
        assert(i <= 11);          // ft0–ft11
        if (i <= 7) return FReg(i);       // ft0–ft7 → f0–f7
        return FReg(28 + i - 8);          // ft8–ft11 → f28–f31
    }

    static FReg fs(unsigned i) {
        assert(i <= 11);          // fs0–fs11
        if (i <= 1) return FReg(8 + i);   // fs0–fs1 → f8–f9
        return FReg(16 + i);              // fs2–fs11 → f18–f27
    }
};