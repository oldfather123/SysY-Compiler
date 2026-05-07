/*
LifeTimeInterval: 表示一个寄存器的生命周期区间，包括生命周期的开始和结束索引，以及对应的寄存器。
(1) 管理生命周期区间：每个虚拟寄存器的生命周期区间可以通过 LifeTimeInterval 对象表示，从而确定该寄存器在何时开始使用和何时结束使用。
(2) 比较和排序：实现了 Comparable 接口，用于比较生命周期区间，以便在分配寄存器时进行冲突检测和优化。
*/
#pragma once

#include <utility>
#include "AsmOperandRegister.h"
#include "LifeTimeIndex.h"

namespace Backend
{

    class LifeTimeInterval
    {
    public:
        AsmOperandRegister *reg;
        std::pair<LifeTimeIndex *, LifeTimeIndex *> reglifetime;
        LifeTimeInterval(AsmOperandRegister *reg, std::pair<LifeTimeIndex *, LifeTimeIndex *> reglifetime)
            : reg(reg), reglifetime(reglifetime) {}

        int getVRegID() const;

        std::pair<LifeTimeIndex *, LifeTimeIndex *> getRegLifetime() const;

        bool operator<(const LifeTimeInterval &other) const;

        bool operator==(const LifeTimeInterval &other) const;

        bool operator!=(const LifeTimeInterval &other) const;

        std::string toString() const;
    };
}
