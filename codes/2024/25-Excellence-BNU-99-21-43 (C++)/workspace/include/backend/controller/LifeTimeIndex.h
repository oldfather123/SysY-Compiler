/*
LifeTimeIndex: 表示指令在程序中的位置，并区分是进入指令前（IN）还是离开指令后（OUT）。
(1) 标记指令的位置：通过 LifeTimeIndex，可以知道某个寄存器的使用或定义发生在指令的哪一部分（进入或离开）。
(2) 比较指令顺序：实现了 Comparable 接口，用于比较两个 LifeTimeIndex 对象，以便排序和管理寄存器的生命周期。
*/
#pragma once

#include "AsmInstBasic.h"

namespace Backend
{
    class LifeTimeController;

    class LifeTimeIndex
    {
        enum class Type
        {
            IN,
            OUT
        };

    private:
        LifeTimeController *ltc;
        AsmInstBasic *srcInst;
        Type type;

    public:
        LifeTimeIndex(LifeTimeController *ltcontroller, AsmInstBasic *srcinst, Type type)
            : ltc(ltcontroller), srcInst(srcinst), type(type) {}

        Type getType() const;

        bool isIn() const;

        bool isOut() const;

        int getInstID() const;

        AsmInstBasic *getSrcInst();

        static LifeTimeIndex *getInstIn(LifeTimeController *ltcontroller, AsmInstBasic *srcinst);

        static LifeTimeIndex *getInstOut(LifeTimeController *ltcontroller, AsmInstBasic *srcinst);

        bool operator<(const LifeTimeIndex &other) const;

        bool operator==(const LifeTimeIndex &other) const;

        bool operator!=(const LifeTimeIndex &other) const;

        bool operator<=(const LifeTimeIndex &other) const;

        LifeTimeController *getLifeTimeController() const;

        std::string toString() const;
    };
}