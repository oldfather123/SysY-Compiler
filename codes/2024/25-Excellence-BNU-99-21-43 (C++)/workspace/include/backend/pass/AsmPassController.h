#pragma once

#include "AsmPassUselessMoveEliminate.h"
#include "AsmPassRedundantUnconditionalJumpEliminate.h"
#include "AsmPassRedundantLabelEliminate.h"
#include "AsmPassLiAddToAddi.h"
#include "AsmPassDeadBlockEliminate.h"
#include "AsmPassBlockInline.h"
#include "AsmPassAddiZeroEliminate.h"
#include "AsmPassRedundantLAEliminate.h"

namespace Backend
{
    class AsmPassController
    {
    private:
        std::vector<AsmPassBasic *> passesBefore;
        std::vector<AsmPassBasic *> passesAfter;
    public:
        AsmPassController()
        {
            passesAfter.push_back(new AsmPassUselessMoveEliminate());
            passesAfter.push_back(new AsmPassRedundantUnconditionalJumpEliminate());
            passesAfter.push_back(new AsmPassRedundantLabelEliminate());
            passesAfter.push_back(new AsmPassLiAddToAddi());
            passesAfter.push_back(new AsmPassAddiZeroEliminate());
            passesAfter.push_back(new AsmPassDeadBlockEliminate());
            passesAfter.push_back(new AsmPassBlockInline());
            passesAfter.push_back(new AsmPassRedundantLAEliminate());
        }

        ~AsmPassController()
        {
            for (auto pass : passesBefore)
                delete pass;

            for (auto pass : passesAfter)
                delete pass;
        }

        std::vector<AsmInstBasic *> runBeforeRegisterAlloc(std::vector<AsmInstBasic *> insts)
        {
            for (auto pass : passesBefore)
                insts = pass->run(insts);
            return insts;
        }

        std::vector<AsmInstBasic *> runAfterRegisterAlloc(std::vector<AsmInstBasic *> insts)
        {
            for (auto pass : passesAfter)
                insts = pass->run(insts);
            return insts;
        }
    };
}