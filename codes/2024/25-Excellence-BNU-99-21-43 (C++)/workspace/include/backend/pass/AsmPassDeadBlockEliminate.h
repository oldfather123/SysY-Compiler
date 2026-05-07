#pragma once
#include "AsmPassBasic.h"

namespace Backend
{
    class AsmPassDeadBlockEliminate : public AsmPassBasic
    {
    public:
        std::vector<AsmInstBasic *> run(std::vector<AsmInstBasic *> insts) override;
    };
}