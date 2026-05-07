#include "AsmPassBlockInline.h"

namespace Backend
{
    /**
     * @brief inline the target block of unconditional jump if the size of the block is less than threshold
     */
    std::vector<AsmInstBasic *> AsmPassBlockInline::run(std::vector<AsmInstBasic *> insts)
    {
        bool changed;  // whether the function has been changed

        // do the inline until no block is inlined in the last iteration
        do
        {
            changed = false;
            std::map<AsmOperandLabel *, std::vector<AsmInstBasic *>> blockMap = _getBlockMap(insts);
            std::map<AsmOperandLabel *, AsmOperandLabel *> fallThroughMap = _getfallThroughMap(insts);
            std::vector<AsmInstBasic *> newInsts;

            for (auto inst: insts)
            {
                bool inlined = false;  // do inline for the current inst

                if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstJump
                    && dynamic_cast<AsmInstJump *>(inst)->getCondition() == AsmInstJump::Condition::UNCONDITIONAL
                ) // 是否为跳转 && 是否为无条件跳转
                {
                    AsmOperandLabel *targetLabel = dynamic_cast<AsmInstJump *>(inst)->getTargetLabel(); // 获取跳转的目标
                    std::vector<AsmInstBasic *> targetBlock = blockMap[targetLabel]; // 获取跳转的目标块

                    if ((int) targetBlock.size() <= threshold) // 判断是否需要 inline（跳转目标的 size 小于等于阈值）
                    {
                        inlined = true;
                        changed = true;

                        newInsts.insert(newInsts.end(), targetBlock.begin(), targetBlock.end()); // 后插入前

                        if (fallThroughMap.find(targetLabel) != fallThroughMap.end())
                        {
                            AsmOperandLabel *fallThroughLabel = fallThroughMap[targetLabel];
                            newInsts.push_back(AsmInstJump::createUnconditional(fallThroughLabel));
                        }
                    }
                }

                // if the block is not inlined, add the inst to the new insts
                if (!inlined)
                    newInsts.push_back(inst);
            }

            insts = newInsts;

        } while (changed);

        return insts;
    }

    /**
     * @brief split the insts into blocks
     * @param insts the instructions to be split
     * @return a map from the label of the block to the instructions in the block
     */
    std::map<AsmOperandLabel *, std::vector<AsmInstBasic *>> AsmPassBlockInline::_getBlockMap(std::vector<AsmInstBasic *> insts)
    {
        std::map<AsmOperandLabel *, std::vector<AsmInstBasic *>> blockMap;
        AsmOperandLabel *currentLabel = nullptr;

        for (auto inst : insts)
        {
            // @note: 结束指令不在块中
            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstBlockEnd)
                continue;

            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel)
            {
                // 一个 Label 下很多指令，调整后续指令的 Label
                currentLabel = dynamic_cast<AsmInstLabel *>(inst)->getAsmOperandLabel();
                blockMap[currentLabel] = std::vector<AsmInstBasic *>();
            }
            else
                blockMap[currentLabel].push_back(inst);
        }

        return blockMap;
    }

    /**
     * @brief get the fall through map(which block is the fall through block of the block)
     * @param insts the instructions vector of the function
     * @return a map [block -> fall through block]
     */
    std::map<AsmOperandLabel *, AsmOperandLabel *> AsmPassBlockInline::_getfallThroughMap(std::vector<AsmInstBasic *> insts)
    {
        std::map<AsmOperandLabel *, AsmOperandLabel *> fallThroughMap;  // block -> fall through block
        // Label 的调整，Label -> label (have inlined)
        
        AsmOperandLabel *currentLabel = nullptr;
        bool isFallThrough = true;
        
        for (auto inst : insts)
        {
            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstBlockEnd)
                continue;

            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel)
            {
                AsmOperandLabel *newLabel = dynamic_cast<AsmInstLabel *>(inst)->getAsmOperandLabel();

                if (currentLabel && isFallThrough)
                    fallThroughMap[currentLabel] = newLabel;

                currentLabel = newLabel;
                isFallThrough = true;
            }
            else if (inst->willNeverReturn())  // return or unconditional jump
            {
                isFallThrough = true;
            }
        }

        return fallThroughMap;
    }
}