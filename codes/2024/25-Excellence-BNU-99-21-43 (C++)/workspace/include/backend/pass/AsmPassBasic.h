#pragma once

#include "AsmInstHeaders.h"
#include "AsmOperandHeaders.h"

namespace Backend
{
    class AsmPassBasic
    {
    public:
        virtual ~AsmPassBasic() = default;
        virtual std::vector<AsmInstBasic *> run(std::vector<AsmInstBasic *> insts) = 0;
    };
}