#pragma once
#include "AsmPassBasic.h"
#include <map>
#include <vector>

namespace Backend
{
    class AsmPassBlockInline : public AsmPassBasic
    {
    private:
        const int threshold = 20;  // the threshold of the block size
    public:
        std::vector<AsmInstBasic *> run(std::vector<AsmInstBasic *> insts) override;
    private:
        std::map<AsmOperandLabel *, std::vector<AsmInstBasic *>> _getBlockMap(std::vector<AsmInstBasic *> insts);
        std::map<AsmOperandLabel *, AsmOperandLabel *> _getfallThroughMap(std::vector<AsmInstBasic *> insts);
    };
}