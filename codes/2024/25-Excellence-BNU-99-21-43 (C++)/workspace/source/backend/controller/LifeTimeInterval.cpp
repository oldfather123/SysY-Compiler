#include "LifeTimeInterval.h"

namespace Backend
{
    int LifeTimeInterval::getVRegID() const
    {
        if (!reg->isVirtualRegister())
            throw std::runtime_error("Unsupported operation: Register is not virtual.");
        return reg->getAbsRegIndex();
    }

    std::pair<LifeTimeIndex *, LifeTimeIndex *> LifeTimeInterval::getRegLifetime() const
    {
        return reglifetime;
    }

    bool LifeTimeInterval::operator<(const LifeTimeInterval &other) const
    {
        auto [start1, end1] = reglifetime;
        auto [start2, end2] = other.reglifetime;

        if (*start1 != *start2 || *end1 != *end2)
        {
            if (*start1 != *start2)
                return *start1 < *start2;
            else
                return *end1 < *end2;
        }

        return reg->getRegIndex() < other.reg->getRegIndex();
    }

    bool LifeTimeInterval::operator==(const LifeTimeInterval &other) const
    {
        auto [start1, end1] = reglifetime;
        auto [start2, end2] = other.reglifetime;
        return *reg == *(other.reg) && *start1 == *start2 && *end1 == *end2;
    }

    bool LifeTimeInterval::operator!=(const LifeTimeInterval &other) const
    {
        return !(*this == other);
    }

    std::string LifeTimeInterval::toString() const
    {
        return reg->toString() + ":" + "(" + reglifetime.first->toString() + "," + reglifetime.second->toString() + ")";
    }

}