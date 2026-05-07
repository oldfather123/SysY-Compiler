#include "LifeTimeIndex.h"
#include "LifeTimeController.h"

namespace Backend
{
    LifeTimeIndex::Type LifeTimeIndex::getType() const
    {
        return type;
    }

    bool LifeTimeIndex::isIn() const
    {
        return type == LifeTimeIndex::Type::IN;
    }

    bool LifeTimeIndex::isOut() const
    {
        return type == LifeTimeIndex::Type::OUT;
    }

    LifeTimeController *LifeTimeIndex::getLifeTimeController() const
    {
        return ltc;
    }

    int LifeTimeIndex::getInstID() const
    {
        return getLifeTimeController()->getInstID(srcInst);
    }

    AsmInstBasic *LifeTimeIndex::getSrcInst()
    {
        return srcInst;
    }

    LifeTimeIndex *LifeTimeIndex::getInstIn(LifeTimeController *ltcontroller, AsmInstBasic *srcinst)
    {
        return new LifeTimeIndex(ltcontroller, srcinst, LifeTimeIndex::Type::IN);
    }

    LifeTimeIndex *LifeTimeIndex::getInstOut(LifeTimeController *ltcontroller, AsmInstBasic *srcinst)
    {
        return new LifeTimeIndex(ltcontroller, srcinst, LifeTimeIndex::Type::OUT);
    }

    bool LifeTimeIndex::operator<(const LifeTimeIndex &other) const
    {
        if (getInstID() != other.getInstID())
            return getInstID() < other.getInstID();
        return type == Type::IN && other.type == Type::OUT;
    }

    bool LifeTimeIndex::operator==(const LifeTimeIndex &other) const
    {
        return getInstID() == other.getInstID() && type == other.type;
    }

    bool LifeTimeIndex::operator!=(const LifeTimeIndex &other) const
    {
        return !(*this == other);
    }

    bool LifeTimeIndex::operator<=(const LifeTimeIndex &other) const
    {
        return *this < other || *this == other;
    }

    std::string LifeTimeIndex::toString() const
    {
        return std::to_string(getInstID()) + "." + (type == LifeTimeIndex::Type::IN ? "IN" : "OUT");
    }
}