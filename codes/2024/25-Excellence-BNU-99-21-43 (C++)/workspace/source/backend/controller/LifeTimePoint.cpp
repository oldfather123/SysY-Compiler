#include "LifeTimePoint.h"

namespace Backend
{
    LifeTimePoint *LifeTimePoint::getUse(LifeTimeIndex *lti)
    {
        return new LifeTimePoint(lti, Type::USE);
    }

    LifeTimePoint *LifeTimePoint::getDef(LifeTimeIndex *lti)
    {
        return new LifeTimePoint(lti, Type::DEF);
    }

    LifeTimePoint::Type LifeTimePoint::getType() const
    {
        return type;
    }

    LifeTimeIndex *LifeTimePoint::getIndex() const
    {
        return ltindex;
    }

    bool LifeTimePoint::isDef() const
    {
        return type == Type::DEF;
    }

    bool LifeTimePoint::isUse() const
    {
        return type == Type::USE;
    }

    bool LifeTimePoint::isTruePoint() const
    {
        AsmInstBasic *inst = getIndex()->getSrcInst();
        if ((inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstJump) || (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel))
            return false;
        return true;
    }

    std::string LifeTimePoint::toString() const
    {
        return getIndex()->toString() + ":" + (type == Type::DEF ? "DEF" : "USE");
    }

    bool LifeTimePoint::operator<(const LifeTimePoint &other) const
    {
        return *(this->ltindex) < *(other.getIndex());
    }

    bool LifeTimePoint::operator==(const LifeTimePoint &other) const
    {
        return *(this->ltindex) == *(other.getIndex());
    }
}