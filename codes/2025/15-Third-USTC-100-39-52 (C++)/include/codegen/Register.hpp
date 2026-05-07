#pragma once

#include <cassert>
#include <string>

/* General-purpose Register Convention:
 * Name         Alias       Meaning
 * $r0          $zero       constant 0
 * $r1          $ra         return address
 * $r2          $tp         thread pointer
 * $r3          $sp         stack pointer
 * $r4 - $r5    $a0 - $a1   argument, return value
 * $r6 - $r11   $a2 - $a7   argument
 * $r12 - $r20  $t0 - $t8   temporary
 * $r21                     saved
 * $r22         $fp / $s9   frame pointer
 * $r23 - $r31  $s0 - $s8   static
 *
 * Floating-point Register Convention
 * Name	        Alias	    Meaning
 * $f0-$f1      $fa0-$fa1   argument/return value
 * $f2-$f7      $fa2-$fa7   argument
 * $f8-$f23     $ft0-$ft15  temporary
 * $f24-$f31    $fs0-$fs7   static
 */
/*
| 寄存器编号       | 别名            | 说明                           |
| ----------- | ------------- | ---------------------------- |
| `$r0`       | `$zero`       | 固定为常数0，读出永远是0，写入无效           |
| `$r1`       | `$ra`         | Return Address，函数返回地址        |
| `$r2`       | `$tp`         | Thread Pointer，线程指针          |
| `$r3`       | `$sp`         | Stack Pointer，栈顶指针           |
| `$r4-$r5`   | `$a0-$a1`     | 参数寄存器（前两个），也是返回值寄存器          |
| `$r6-$r11`  | `$a2-$a7`     | 参数寄存器                        |
| `$r12-$r20` | `$t0-$t8`     | 临时寄存器，不需在函数间保留               |
| `$r21`      | 无别名           | 预留/保留，部分实现可用于特殊用途            |
| `$r22`      | `$fp` / `$s9` | 帧指针 Frame Pointer，常用于访问局部变量  |
| `$r23-$r31` | `$s0-$s8`     | 保存寄存器（callee-saved），函数调用时需保留 |

| 编号          | 别名           | 说明                |
| ----------- | ------------ | ----------------- |
| `$f0-$f1`   | `$fa0-$fa1`  | 浮点参数 & 返回值（第1-2个） |
| `$f2-$f7`   | `$fa2-$fa7`  | 浮点参数              |
| `$f8-$f23`  | `$ft0-$ft15` | 浮点临时寄存器           |
| `$f24-$f31` | `$fs0-$fs7`  | 浮点保存寄存器，函数调用间必须保存 |

| 类型   | 寄存器范围                   | 用途说明                      |
| ---- | ----------------------- | ------------------------- |
| 返回值  | `$a0-$a1`, `$fa0-$fa1`  | 整数/浮点返回值                  |
| 实参   | `$a0-$a7`, `$fa0-$fa7`  | 函数参数最多 8 个                |
| 临时   | `$t0-$t8`, `$ft0-$ft15` | 可直接用，函数结束不用恢复             |
| 保存   | `$s0-$s8`, `$fs0-$fs7`  | 函数内使用需 **保存/恢复**          |
| 栈管理  | `$sp`、`$fp`             | `$sp` 指向栈顶，`$fp` 指向当前栈帧底部 |
| 返回地址 | `$ra`                   | 函数返回用 `jalr $ra`          |



寄存器	别名	全称	说明
X0	                zero	    零寄存器	    可做源寄存器(rs)或目标寄存器(rd)
X1	                ra	        链接寄存器	    保存函数返回地址
X2	                sp	        栈指针寄存器	指向栈的地址
X3	                gp	        全局寄存器	    用于链接器松弛优化
X4	                tp	        线程寄存器	    常用于在OS中保存指向进程控制块(task_struct)数据结构的指针
X5 ~ X7 X28 ~ X31	t0 ~ t6	    临时寄存器	
X8	                s0/fp	    帧指针寄存器	用于函数调用，被调用函数需保存数据
X9	                s1		                   用于函数调用 ，被调用函数需要保存的数据
X10 ~ X17	        a0 ~ a7     函数的参数		用于函数调用，传递参数和返回值
X18 ~ X27	        s2 ~ s11   保存寄存器        用于函数调用 ，被调用函数需要保存的数据
*/
struct Reg {
    unsigned id;

    explicit Reg(unsigned i) : id(i) { assert(i <= 31); }
    bool operator==(const Reg &other) const { return id == other.id; }

    std::string print() const;

    // 常用寄存器构造器
    static Reg zero() { return Reg(0); }   // x0
    static Reg ra() { return Reg(1); }     // x1
    static Reg sp() { return Reg(2); }     // x2
    static Reg gp() { return Reg(3); }     // x3
    static Reg tp() { return Reg(4); }     // x4
    static Reg fp() { return Reg(8); }     // x8 (s0 / fp)

    // 参数寄存器 a0 ~ a7 → x10 ~ x17
    static Reg a(unsigned i) {
        assert(i <= 7);
        return Reg(i + 10);
    }

    // 临时寄存器 t0 ~ t6 → x5 ~ x7, x28 ~ x31
    static Reg t(unsigned i) {
        assert(i <= 6);
        if (i <= 2)
            return Reg(i + 5);    // t0~t2: x5~x7
        else
            return Reg(i + 25);   // t3~t6: x28~x31
    }

    // 保存寄存器 s0 ~ s11 → x8, x9, x18~x27
    static Reg s(unsigned i) {
        assert(i <= 11);
        if (i <= 1)
            return Reg(i + 8);    // s0/s1: x8/x9
        else
            return Reg(i + 16);   // s2~s11: x18~x27
    }
};

struct FReg {
    unsigned id;

    explicit FReg(unsigned i) : id(i) { assert(i <= 31); }
    bool operator==(const FReg &other) const { return id == other.id; }

    std::string print() const;

    // fa0 ~ fa7 → f10 ~ f17
    static FReg fa(unsigned i) {
        assert(i <= 7);
        return FReg(i + 10);
    }

    // ft0 ~ ft11 → f0~f7, f28~f31
    static FReg ft(unsigned i) {
        assert(i <= 11);
        if (i <= 7)
            return FReg(i);         // f0~f7
        else
            return FReg(i + 17);    // f28~f31
    }

    // fs0 ~ fs11 → f8, f9, f18~f27
    static FReg fs(unsigned i) {
        assert(i <= 11);
        if (i <= 1)
            return FReg(i + 8);     // f8, f9
        else
            return FReg(i + 7);     // f18~f27
    }
};
// std::string FReg::print() const {
//     static const std::string names[] = {
//         "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",   // f0~f7
//         "fs0", "fs1",                                             // f8~f9
//         "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7",   // f10~f17
//         "fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11", // f18~f27
//         "ft8", "ft9", "ft10", "ft11"                              // f28~f31
//     };
//     return "$" + names[id];
// }


struct CFReg {
    unsigned id;

    explicit CFReg(unsigned i) : id(i) { assert(i <= 7); }
    bool operator==(const CFReg &other) { return id == other.id; }

    std::string print() const { return "fcc" + std::to_string(id); }
};
// struct CFReg {
//     unsigned id;

//     explicit CFReg(unsigned i) : id(i) { assert(i == 0); }
//     bool operator==(const CFReg &other) const { return id == other.id; }

//     std::string print() const { return "$cf" + std::to_string(id); }
// };
