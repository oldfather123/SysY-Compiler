#pragma once
#include "AsmPassBasic.h"

namespace Backend
{
    class AsmPassAddiZeroEliminate : public AsmPassBasic
    {
    public:
        std::vector<AsmInstBasic *> run(std::vector<AsmInstBasic *> insts) override;
    };
}