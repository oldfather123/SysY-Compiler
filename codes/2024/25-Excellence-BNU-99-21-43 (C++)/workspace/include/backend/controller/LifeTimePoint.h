/*
LifeTimePoint: 表示一个寄存器在程序中的某个具体时间点上的使用（USE）或定义（DEF）。
(1) 标记 DEF 和 USE 点：通过 LifeTimePoint，可以精确标记寄存器的每个使用点和定义点，帮助追踪寄存器的活跃时间段。
(2) 可以区分“真实点”和无效点（如标签和跳转指令），有助于更准确地进行生命周期分析。
*/
#pragma once

#include "LifeTimeIndex.h"
#include "AsmInstBasic.h"

namespace Backend
{
    class LifeTimePoint
    {
    public:
        enum class Type
        {
            DEF,
            USE
        };

    private:
        LifeTimeIndex *ltindex;
        Type type;

    public:
        LifeTimePoint(LifeTimeIndex *lti, Type type) : ltindex(lti), type(type) {}

        static LifeTimePoint *getUse(LifeTimeIndex *lti);

        static LifeTimePoint *getDef(LifeTimeIndex *lti);

        Type getType() const;

        LifeTimeIndex *getIndex() const;

        bool isDef() const;

        bool isUse() const;

        bool isTruePoint() const;

        std::string toString() const;

        bool operator<(const LifeTimePoint &other) const;

        bool operator==(const LifeTimePoint &other) const;
    };
}