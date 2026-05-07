#pragma once
#include "AsmPassBasic.h"

namespace Backend
{
    class AsmPassUselessMoveEliminate : public AsmPassBasic
    {
    private:
        std::vector<AsmInstBasic *> _removeMoveA2A(std::vector<AsmInstBasic *> insts);
        std::vector<AsmInstBasic *> _removeMoveA2BB2A(std::vector<AsmInstBasic *> insts);

    public:
        std::vector<AsmInstBasic *> run(std::vector<AsmInstBasic *> insts) override;
    };
}