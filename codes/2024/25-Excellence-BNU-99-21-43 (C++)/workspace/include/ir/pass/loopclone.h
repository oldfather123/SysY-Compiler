#pragma once

#include "IR.h"
#include "clonebase.h"

#include <vector>
#include <map>
#include <set>
#include <memory>

namespace IR
{
    class LoopClone : public CloneBase
    {
    public:
        LoopClone(BasicBlock *headBlock, const std::set<BasicBlock *> &loopBasicBlocks);

        void setNextLoopHead(BasicBlock *nextLoopHead);

        void addEntry(BasicBlock *basicBlock, const std::map<PhiInstruction *, Value *> &valueMap);

        BasicBlock *getClonedLoopHead() const;

        const std::vector<BasicBlock *> &getClonedBasicBlocks() const;

        Value *getClonedValue(Value *originalValue) const;

    private:
        BasicBlock *headBlock;
        std::set<BasicBlock *> loopBasicBlocks;

        std::vector<BasicBlock *> clonedBasicBlocks;
        std::map<PhiInstruction *, PhiInstruction *> headPhiMap;
        BasicBlock* nextLoopHeadSymbol;

        void doLoopClone();
    };

} // namespace IR