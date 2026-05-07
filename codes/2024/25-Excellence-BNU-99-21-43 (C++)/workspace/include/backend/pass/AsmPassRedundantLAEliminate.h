#pragma once
#include "AsmPassBasic.h"
#include <set>

namespace Backend
{
    class AsmPassRedundantLAEliminate : public AsmPassBasic
    {
    
    private:
        using Record = std::pair<AsmOperandRegister *, AsmOperandLabel *>;
        struct RecordCompare
        {
            bool operator()(const Record &lhs, const Record &rhs) const
            {
                auto [r1, l1] = lhs;
                auto [r2, l2] = rhs;
                if (*r1 != *r2) return *r1 < *r2;
                else return *l1 < *l2;
            }
        };
        std::set<Record, RecordCompare> records;

    public:
        std::vector<AsmInstBasic *> run(std::vector<AsmInstBasic *> insts) override;
    };
}