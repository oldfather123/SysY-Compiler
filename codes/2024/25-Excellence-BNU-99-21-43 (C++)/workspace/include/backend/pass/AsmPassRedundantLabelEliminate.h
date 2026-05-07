#pragma once
#include "AsmPassBasic.h"

namespace Backend
{
    class AsmPassRedundantLabelEliminate : public AsmPassBasic
    {
    public:
        std::vector<AsmInstBasic *> run(std::vector<AsmInstBasic *> insts) override;
    };
}