#include "loopclone.h"

namespace IR
{

    LoopClone::LoopClone(BasicBlock *headBlock, const std::set<BasicBlock *> &loopBasicBlocks)
        : headBlock(headBlock), loopBasicBlocks(loopBasicBlocks)
    {
        nextLoopHeadSymbol = BasicBlock::get("nextLoopHeadSymbol");
        doLoopClone();
    }

    void LoopClone::setNextLoopHead(BasicBlock *nextLoopHead)
    {
        nextLoopHeadSymbol->replaceAllUsageTo(nextLoopHead);
    }

    void LoopClone::addEntry(BasicBlock *basicBlock, const std::map<PhiInstruction *, Value *> &valueMap)
    {
        if (valueMap.size() != headPhiMap.size())
        {
            Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid value map size");
        }
        for (const auto &[entryPhi, entryValue] : valueMap)
        {
            auto it = headPhiMap.find(entryPhi);
            if (it != headPhiMap.end())
            {
                it->second->insertIncomingValue(std::make_pair(entryValue, basicBlock));
            }
        }
    }

    BasicBlock *LoopClone::getClonedLoopHead() const
    {
        return static_cast<BasicBlock *>(getReplacedValue(headBlock));
    }

    const std::vector<BasicBlock *> &LoopClone::getClonedBasicBlocks() const
    {
        return clonedBasicBlocks;
    }

    Value *LoopClone::getClonedValue(Value *originalValue) const
    {
        return getReplacedValue(originalValue);
    }

    void LoopClone::doLoopClone()
    {
        // 构建占位符
        for (BasicBlock *basicBlock : loopBasicBlocks)
        {
            addBasicBlock(basicBlock);
            for (Instruction *instruction : basicBlock->getVectorInstructions())
            {
                addSymbolFor(instruction);
            }
        }

        // 构建函数体
        for (BasicBlock *basicBlock : loopBasicBlocks)
        {
            BasicBlock *clonedBlock = static_cast<BasicBlock *>(getReplacedValue(basicBlock));
            for (Instruction *instruction : basicBlock->getVectorInstructions())
            {
                // instruction->emitIR(std::cerr);
                Instruction *clonedInstruction;
                if (basicBlock == headBlock && dynamic_cast<PhiInstruction *>(instruction))
                {
                    clonedInstruction = PhiInstruction::create(instruction->getType(), std::vector<PVB>());
                    headPhiMap[static_cast<PhiInstruction *>(instruction)] = static_cast<PhiInstruction *>(clonedInstruction);
                }
                else if (auto jumpInst = dynamic_cast<BranchInstruction *>(instruction); jumpInst && jumpInst->isUnconditional() && jumpInst->getUnconditionalBlock() == headBlock)
                {
                    clonedInstruction = BranchInstruction::createBr(nextLoopHeadSymbol);
                }
                else
                {
                    clonedInstruction = cloneInstruction(instruction);
                }
                setValueMapKv(instruction, clonedInstruction);
                clonedBlock->InsertInstructionBack(clonedInstruction);
            }
            clonedBasicBlocks.push_back(clonedBlock);
        }
    }

}