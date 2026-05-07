#include "loopunrollpass.h"
#include "finalpass.h"
#include "simplifycodepass.h"

namespace IR
{
    LoopUnrollPass::LoopUnrollPass(Function *function)
    {
        this->function = function;
        loopForest = new LoopForest(function);
    }

    void LoopUnrollPass::run(Module *module)
    {
        IR::FinalPass finalPass;
        finalPass.run(module);

        for (auto func : module->getFunctionList())
        {
            if (func->isBuiltinFunction())
                continue;

            LoopUnrollPass pass(func);
            pass.runOnFunction(func);
        }

        IR::SimplifyCodePass simplifyCodePass;
        simplifyCodePass.run(module);
    }

    bool LoopUnrollPass::runOnFunction(Function *func)
    {
        bool changed = false;
        for (Loop *rootLoop : loopForest->getRootLoops())
        {
            // std::cerr << "try unroll loop" << ' ' << rootLoop->getHeaderBasicBlock()->getIRName() << std::endl;
            // for (auto bb : rootLoop->getBasicBlocks())
            // {
            //     std::cerr << '\t' << bb->getIRName() << std::endl;
            // }
            changed |= tryUnrollLoop(rootLoop);
        }
        return changed;
    }

    int LoopUnrollPass::collectLoopInfo(Loop *loop, std::set<BasicBlock *> &visitedBlocks)
    {
        int loopSize = 0;

        for (BasicBlock *basicBlock : loop->getBasicBlocks())
        {
            visitedBlocks.insert(basicBlock);
            loopSize += basicBlock->getInstruction().size();
        }

        for (Loop *subLoopL : loop->getSubLoops())
            loopSize += collectLoopInfo(subLoopL, visitedBlocks);

        return loopSize;
    }

    bool LoopUnrollPass::tryUnrollLoop(Loop *loop)
    {
        // unroll loop recursively
        for (Loop *subloop : loop->getSubLoops())
            if (tryUnrollLoop(subloop))
                return true;

        // unroll simple loop only
        if (!loop->isSimpleLoop())
            return false;

        IR::Instruction::CmpOp condition = static_cast<IR::Instruction::CmpOp>(loop->getSimpleLoopInfo()->loopExitCompareInstruction->getCmpCode());
        if (condition == IR::Instruction::CmpOp::Ne || condition == IR::Instruction::CmpOp::Eq)
            return false;

        std::set<BasicBlock *> loopBlocks;
        int loopSize = collectLoopInfo(loop, loopBlocks);

        if (loopSize > MAXIMUM_EXTRACTED_SIZE)
            return false;

        if (loop->getHeaderBasicBlock()->hasNoLoopUnrollTag)
            return false;

        LoopClone *remainderLoop = new LoopClone(loop->getHeaderBasicBlock(), loopBlocks);

        BasicBlock *loopExitBlock = loop->getSimpleLoopInfo()->loopNextBlock;

        std::map<Instruction *, PhiInstruction *> exitValueMapping;

        for (Instruction *instuction : loop->getHeaderBasicBlock()->getVectorInstructions())
            if (!instuction->getType()->isVoid())
            {
                PhiInstruction *exitPhi = nullptr;

                for (Use *use : instuction->getVectorUses())
                {
                    User *user = use->user;
                    if (!user->isInstruction())
                        Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid user");
                    Instruction *userInst = static_cast<Instruction *>(user);
                    // 遍历循环头块的使用，找到在循环结束BB中使用循环头块的phi指令
                    if (userInst->getOpcode() == IR::Instruction::Phi && userInst->getParentBB() == loopExitBlock)
                    {
                        exitPhi = static_cast<PhiInstruction *>(userInst);
                        break;
                    }
                }

                if (exitPhi == nullptr)
                {
                    exitPhi = PhiInstruction::create(instuction->getType(), std::vector<PVB>());
                    loopExitBlock->InsertInstructionFront(exitPhi);
                }

                exitValueMapping[instuction] = exitPhi;

                for (Use *use : instuction->getVectorUses())
                {
                    User *user = use->user;
                    if (!user->isInstruction())
                        Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid user");

                    Instruction *userInst = static_cast<Instruction *>(user);
                    // 更新循环外对头块中的使用
                    if (loopBlocks.find(userInst->getParentBB()) == loopBlocks.end())
                    {
                        use->setValue(exitPhi);
                    }
                }
            }

        LoopClone *firstClonedLoop = new LoopClone(loop->getHeaderBasicBlock(), loopBlocks);
        makeExitConditionExtracted(loop, firstClonedLoop);

        BasicBlock *clonedLoopHeadBlock = firstClonedLoop->getClonedLoopHead();
        if (clonedLoopHeadBlock == nullptr || clonedLoopHeadBlock->getInstruction().size() == 0)
            Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid cloned loop head");

        Instruction *terminateInst = dynamic_cast<Instruction *>(clonedLoopHeadBlock->getInstruction().rbegin());
        if (terminateInst == nullptr || terminateInst->getOpcode() != IR::Instruction::BR)
            Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid terminate instruction");

        BranchInstruction *firstLoopExitInstruction = static_cast<BranchInstruction *>(terminateInst);
        if (firstLoopExitInstruction->isUnconditional())
            Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid exit instruction");

        // 为第一个展开循环的终止指令设置新的条件
        firstLoopExitInstruction->setFalseBlock(remainderLoop->getClonedLoopHead());

        std::map<BasicBlock *, std::map<PhiInstruction *, Value *>> outerEntries;
        std::map<BasicBlock *, std::map<PhiInstruction *, Value *>> innerEntries;

        for (Use *use : loop->getHeaderBasicBlock()->getVectorUses())
        {
            User *user = use->user;
            if (!user->isInstruction())
                Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid user");

            Instruction *userInst = static_cast<Instruction *>(user);
            // std::cerr << "userInst: ";
            // userInst->emitIR(std::cerr);
            if (userInst->getOpcode() == IR::Instruction::BR || userInst->getOpcode() == IR::Instruction::Return)
            {
                BasicBlock *entryBlock = userInst->getParentBB();
                if (loopBlocks.find(entryBlock) != loopBlocks.end())
                {
                    innerEntries[entryBlock] = std::map<PhiInstruction *, Value *>();
                }
                else
                {
                    use->setValue(firstClonedLoop->getClonedLoopHead());
                    outerEntries[entryBlock] = std::map<PhiInstruction *, Value *>();
                }
            }
            // userInst->emitIR(std::cerr);
        }

        for (Instruction *inst : loop->getHeaderBasicBlock()->getVectorInstructions())
        {
            if (inst->getOpcode() == Instruction::Phi)
            {
                PhiInstruction *phiInst = static_cast<PhiInstruction *>(inst);
                std::vector<PVB> entries = phiInst->getDifferentPVB();
                for (auto [value, basicBlock] : entries)
                {
                    if (outerEntries.find(basicBlock) != outerEntries.end())
                    {
                        outerEntries[basicBlock][phiInst] = value;
                    }
                    else
                    {
                        innerEntries[basicBlock][phiInst] = value;
                    }
                }
            }
            else
            {
                break;
            }
        }

        for (auto [key, val] : outerEntries)
        {
            firstClonedLoop->addEntry(key, val);
        }

        for (BasicBlock *clonedBasicBlock : firstClonedLoop->getClonedBasicBlocks())
        {
            function->addBlock(clonedBasicBlock, false);
        }

        concatFalseExit(loop, firstClonedLoop, remainderLoop);

        LoopClone *lastClonedLoop = firstClonedLoop;
        for (int i = 1; i < UNROLLING_FACTOR; i++)
        {
            LoopClone *clonedLoop = new LoopClone(loop->getHeaderBasicBlock(), loopBlocks);

            Instruction *terminateInst = dynamic_cast<Instruction *>(clonedLoop->getClonedLoopHead()->getInstruction().rbegin());
            if (terminateInst == nullptr || terminateInst->getOpcode() != IR::Instruction::BR)
                Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid terminate instruction");

            BranchInstruction *loopExitInstruction = static_cast<BranchInstruction *>(terminateInst);
            if (loopExitInstruction->isUnconditional())
                Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid exit instruction");

            loopExitInstruction->setCondition(ConstantInt32::get(1));
            loopExitInstruction->setFalseBlock(remainderLoop->getClonedLoopHead());

            concatLoop(innerEntries, clonedLoop, lastClonedLoop);

            for (BasicBlock *clonedBasicBlock : clonedLoop->getClonedBasicBlocks())
            {
                function->addBlock(clonedBasicBlock, false);
            }

            concatFalseExit(loop, clonedLoop, remainderLoop);

            lastClonedLoop = clonedLoop;
        }

        concatLoop(innerEntries, firstClonedLoop, lastClonedLoop);

        concatLoop(innerEntries, remainderLoop, remainderLoop);

        for (BasicBlock *clonedBasicBlock : remainderLoop->getClonedBasicBlocks())
        {
            function->addBlock(clonedBasicBlock, false);
        }

        for (auto [headBlockInstruction, phiInst] : exitValueMapping)
        {
            PVB newPVB = std::make_pair(remainderLoop->getClonedValue(headBlockInstruction), remainderLoop->getClonedLoopHead());
            phiInst->insertIncomingValue(newPVB);
        }
        remainderLoop->getClonedLoopHead()->hasNoLoopUnrollTag = true;

        // // 删除原循环: 目的是在循环展开后彻底删除原始循环中的基本块和相关指令。通过清理出口信息、操作数以及移除无效块的步骤，确保原循环不再对程序的后续执行产生影响。
        // 1. 清理出口信息
        // for (BasicBlock *loopBlock : loopBlocks) {
        //     for (BasicBlock *exitBlock : loopBlock->getSuccBlock()) {
        //         exitBlock->removeEntryFromPhi(loopBlock);
        //     }
        //     // auto tempTerminate = loopBlock->getTerminateInstruction();
        //     // loopBlock->setTerminateInstruction(new UnreachableInst());
        //     tempTerminate.waste();
        // }

        // // // 2. 清理操作数
        // for (BasicBlock *loopBlock : loopBlocks) {
        //     for (Instruction *instruction : loopBlock->getVectorInstructions()) {
        //         instruction->clearOperands();
        //     }
        // }

        // // // 3. 移除无效块
        for (BasicBlock *loopBlock : loopBlocks)
        {
            // for (Instruction *instruction : loopBlock->getVectorInstructions()) {
            //     instruction->waste();
            // }
            loopBlock->waste();
        }

        return false;
    }

    void LoopUnrollPass::concatLoop(const std::map<BasicBlock *, std::map<PhiInstruction *, Value *>> &innerEntries, LoopClone *secondLoop, LoopClone *firstLoop)
    {
        firstLoop->setNextLoopHead(secondLoop->getClonedLoopHead());

        for (auto [key, val] : innerEntries)
        {
            std::map<PhiInstruction *, Value *> newEntries;

            for (auto [phiInst, value] : val)
            {
                newEntries[phiInst] = firstLoop->getClonedValue(value);
            }

            secondLoop->addEntry(dynamic_cast<BasicBlock *>(firstLoop->getClonedValue(key)), newEntries);
        }
    }

    void LoopUnrollPass::concatFalseExit(Loop *originalLoop, LoopClone *firstLoop, LoopClone *secondLoop)
    {
        std::map<PhiInstruction *, Value *> newMap;

        for (Instruction *leadingInstruction : originalLoop->getHeaderBasicBlock()->getVectorInstructions())
        {
            if (leadingInstruction->getOpcode() == IR::Instruction::Phi)
            {
                PhiInstruction *phiInst = static_cast<PhiInstruction *>(leadingInstruction);
                newMap[phiInst] = firstLoop->getClonedValue(phiInst);
            }
            else
            {
                break;
            }
        }

        secondLoop->addEntry(firstLoop->getClonedLoopHead(), newMap);
    }

    void LoopUnrollPass::makeExitConditionExtracted(Loop *loop, LoopClone *firstClonedLoop)
    {
        auto stepValue32 = firstClonedLoop->getClonedValue(loop->getSimpleLoopInfo()->loopIncrementInstruction->getOperand(1));
        auto beforeStepValue32 = firstClonedLoop->getClonedValue(loop->getSimpleLoopInfo()->loopIncrementInstruction->getOperand(0));

        // 创建一个新的乘法指令，将步长值乘以展开因子
        auto newStepValue32 = BinaryInstruction::createMul(BasicType::getInt32Type(), stepValue32, ConstantInt32::get(UNROLLING_FACTOR));

        BinaryInstruction *afterStepValue32 = nullptr;

        // 根据原始步长指令的类型，创建相应的加法或减法指令
        if (loop->getSimpleLoopInfo()->loopIncrementInstruction->getOpcode() == IR::Instruction::Add)
        {
            afterStepValue32 = BinaryInstruction::createAdd(BasicType::getInt32Type(), beforeStepValue32, newStepValue32);
        }
        else if (loop->getSimpleLoopInfo()->loopIncrementInstruction->getOpcode() == IR::Instruction::Sub)
        {
            afterStepValue32 = BinaryInstruction::createSub(BasicType::getInt32Type(), beforeStepValue32, newStepValue32);
        }
        else
        {
            Error::Error(__PRETTY_FUNCTION__, __LINE__, "Unknown step instruction");
        }

        auto limitValue32 = firstClonedLoop->getClonedValue(loop->getSimpleLoopInfo()->loopExitCompareInstruction->getOperand(1));

        // 创建新的比较指令，比较展开后的步长值与扩展后的条件值
        auto newCompareInst = ICmpInstruction::create(
            static_cast<IR::Instruction::CmpOp>(loop->getSimpleLoopInfo()->loopExitCompareInstruction->getOpcode()),
            afterStepValue32,
            limitValue32);

        // 获取第一个展开循环的头块，并添加新创建的指令
        auto firstLoopEntry = firstClonedLoop->getClonedLoopHead();
        auto terminator = firstLoopEntry->getInstruction().rbegin();
        if (terminator == firstLoopEntry->getInstruction().rend())
            Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid terminator");
        assert(dynamic_cast<Instruction *>(terminator) != nullptr);
        auto terminateInst = dynamic_cast<Instruction *>(terminator);
        firstLoopEntry->InsertInstruction(newStepValue32, terminateInst);   // 添加新的乘法指令（1. 步长值乘以展开因子，计算新的步长值）
        firstLoopEntry->InsertInstruction(afterStepValue32, terminateInst); // 添加展开后的步长值指令（2. 步长前值加上新的步长值，步进指令）
        firstLoopEntry->InsertInstruction(newCompareInst, terminateInst);   // 添加新的比较指令（3. 比较展开后的步长值与扩展后的条件值）

        // 更新第一个展开循环的终止指令条件为新的比较指令
        terminateInst = dynamic_cast<Instruction *>(firstLoopEntry->getInstruction().rbegin());
        assert(terminateInst != nullptr);
        // firstLoopEntry->emitIR(std::cerr);
        if (terminateInst == nullptr || terminateInst->getOpcode() != IR::Instruction::BR)
            Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid terminate instruction");

        BranchInstruction *firstLoopExitInstruction = static_cast<BranchInstruction *>(terminateInst);
        if (firstLoopExitInstruction->isUnconditional())
            Error::Error(__PRETTY_FUNCTION__, __LINE__, "Invalid exit instruction");

        BranchInstruction *newBranchInst = BranchInstruction::createCondBr(newCompareInst, firstLoopExitInstruction->getTrueBlock(), firstLoopExitInstruction->getFalseBlock());
        firstLoopEntry->InsertInstructionBack(newBranchInst); // 添加新的分支指令（4. 根据新的比较指令，更新分支指令）
        firstLoopExitInstruction->waste();
    }
}