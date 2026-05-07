#pragma once
#include "AsmPassBasic.h"

namespace Backend
{
    class AsmPassLiAddToAddi : public AsmPassBasic
    {
    public:
        std::vector<AsmInstBasic *> run(std::vector<AsmInstBasic *> insts) override;
    };
}