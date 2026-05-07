#include "simpleloopinformation.h"
#include "loop.h"

namespace IR
{
    SimpleLoopInformation::Builder::Builder() : loopEntryBlock(nullptr), initValue(nullptr), exitValue(nullptr) {}

    SimpleLoopInformation *SimpleLoopInformation::Builder::buildLoopInformation(Loop *loop)
    {
        loopEntryBlock = loop->getHeaderBasicBlock();
        getAllBlocksAndInstructions(loop);
        // 获取循环的条件比较指令(已标准化，即比较的左操作数为循环变量，右操作数为循环上界)
        auto [loopExitCompareInstruction, exitBlock] = getLoopExitCompareInstruction();
        if (loopExitCompareInstruction == nullptr)
            return nullptr;
        auto lop = loopExitCompareInstruction->getOperand(0);
        // lop应当是phi
        if (!lop->isInstruction() || static_cast<Instruction *>(lop)->getOpcode() != Instruction::Phi)
            return nullptr;
        // lop->emitIR(std::cerr);
        analyzePhiInstruction(static_cast<PhiInstruction *>(lop)); // 分析phi指令获取循环变量的初始值和退出值
        if (initValue == nullptr || exitValue == nullptr)
            return nullptr;
        BinaryInstruction *loopIncrementInstruction = getLoopIncrementInstruction(); // 获取循环的增量指令(已标准化，即形式为 b = a + 1 or b = a - 1)
        if (loopIncrementInstruction == nullptr)
            return nullptr;

        return new SimpleLoopInformation(loopIncrementInstruction, loopExitCompareInstruction, initValue, exitBlock);
    }

    IR::BinaryInstruction *SimpleLoopInformation::Builder::getLoopIncrementInstruction()
    {
        // 找到exitValue即可，要求对其标准化，即形式为 b = a + 1 or b = a - 1
        assert(exitValue != nullptr);
        if (!exitValue->isInstruction() ||
            (static_cast<Instruction *>(exitValue)->getOpcode() != Instruction::Add &&
             static_cast<Instruction *>(exitValue)->getOpcode() != Instruction::Sub))
        {
            // exitValue->emitIR(std::cerr);
            return nullptr;
        }
        Instruction *exitInstr = static_cast<Instruction *>(exitValue);
        auto operand0 = exitInstr->getOperand(0);
        auto operand1 = exitInstr->getOperand(1);
        bool isOperand0Dynamic = operand0->isInstruction() && allInstructions.find(static_cast<Instruction *>(operand0)) != allInstructions.end();
        bool isOperand1Dynamic = operand1->isInstruction() && allInstructions.find(static_cast<Instruction *>(operand1)) != allInstructions.end();
        if (isOperand0Dynamic && isOperand1Dynamic)
            return nullptr;
        else if (isOperand0Dynamic)
        {
            if (!operand1->isConstantInt32())
                return nullptr;
            return static_cast<BinaryInstruction *>(exitInstr);
        }
        else if (isOperand1Dynamic) // 交换
        {
            if (!operand0->isConstantInt32())
                return nullptr;
            if (exitInstr->getOpcode() == Instruction::Add)
            {
                BinaryInstruction *newInstr = BinaryInstruction::createAdd(BasicType::getInt32Type(), operand1, operand0);
                exitInstr->replaceAllUsageTo(newInstr);
                exitInstr->parent->InsertInstructionBack(newInstr);
                allInstructions.insert(newInstr);
                allInstructions.erase(exitInstr);
                exitInstr->waste();
                return newInstr;
            }
            else if (exitInstr->getOpcode() == Instruction::Sub)
            {
                return nullptr;
            }
            else
            {
                Error::Error("SimpleLoopInformation::Builder::getLoopIncrementInstruction", "Unknown opcode");
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }

    std::pair<ICmpInstruction *, BasicBlock *> SimpleLoopInformation::Builder::getLoopExitCompareInstruction()
    {
        BranchInstruction *exitBrInstr = nullptr;
        // 遍历循环入口块的指令，找到比较指令

        for (auto instr : allInstructions)
        {
            if (instr->getOpcode() == Instruction::BR)
            {
                auto brInstr = static_cast<BranchInstruction *>(instr);
                if (brInstr->isUnconditional())
                    continue;
                auto condValue = brInstr->getCondition();
                if (!condValue->isInstruction() ||
                    !(static_cast<Instruction *>(condValue)->getOpcode() >= Instruction::Ne &&
                      static_cast<Instruction *>(condValue)->getOpcode() <= Instruction::Eq))
                    continue;
                if (exitBrInstr == nullptr)
                    exitBrInstr = brInstr;
                else
                    return std::make_pair(nullptr, nullptr);
            }
        }
        if (exitBrInstr->parent != loopEntryBlock)
            return std::make_pair(nullptr, nullptr);

        if (exitBrInstr == nullptr)
            return std::make_pair(nullptr, nullptr);
        assert(exitBrInstr->isConditional());
        // 如果true分支是循环体外的块，则交换true和false分支
        if (allBlocks.find(exitBrInstr->getTrueBlock()) == allBlocks.end())
        {
            // 创建一个新的ICmpInstruction, 其比较结果与原ICmpInstruction相反
            auto exitCmpInstr = static_cast<ICmpInstruction *>(exitBrInstr->getCondition());
            auto notCmpInstr = ICmpInstruction::createNotICmp(exitCmpInstr);
            // 将新的比较指令插入到循环入口块的指令序列中
            loopEntryBlock->InsertInstruction(notCmpInstr, exitBrInstr);
            allInstructions.insert(notCmpInstr);
            // 创建一个新的BranchInstruction，其条件为notCmpInstr
            auto newBrInstr = BranchInstruction::createCondBr(notCmpInstr, exitBrInstr->getFalseBlock(), exitBrInstr->getTrueBlock());
            exitBrInstr->replaceAllUsageTo(newBrInstr);

            loopEntryBlock->InsertInstruction(newBrInstr, exitBrInstr);
            allInstructions.insert(newBrInstr);

            exitBrInstr->waste();
            allInstructions.erase(exitBrInstr);

            exitBrInstr = newBrInstr;
            if (exitCmpInstr->useList.empty())
            {
                exitCmpInstr->waste();
                allInstructions.erase(exitCmpInstr);
            }
        }
        assert(allBlocks.find(exitBrInstr->getTrueBlock()) != allBlocks.end());
        assert(allBlocks.find(exitBrInstr->getFalseBlock()) == allBlocks.end());

        // 标准化比较指令，即比较的左操作数为循环变量，右操作数为循环上界
        ICmpInstruction *exitCmpInstr = static_cast<ICmpInstruction *>(exitBrInstr->getCondition());
        auto operand0 = exitCmpInstr->getOperand(0);
        auto operand1 = exitCmpInstr->getOperand(1);
        bool isOperand0Dynamic = operand0->isInstruction() && allInstructions.find(static_cast<Instruction *>(operand0)) != allInstructions.end();
        bool isOperand1Dynamic = operand1->isInstruction() && allInstructions.find(static_cast<Instruction *>(operand1)) != allInstructions.end();
        if (isOperand0Dynamic && isOperand1Dynamic)
            return std::make_pair(nullptr, nullptr);
        else if (isOperand0Dynamic)
        {
            if (!operand1->isConstantInt32())
                return std::make_pair(nullptr, nullptr);
            return std::make_pair(exitCmpInstr, exitBrInstr->getFalseBlock());
        }
        else if (isOperand1Dynamic) // 交换
        {
            if (!operand0->isConstantInt32())
                return std::make_pair(nullptr, nullptr);
            auto newCmpInstr = ICmpInstruction::createNotICmp(exitCmpInstr, false);
            loopEntryBlock->InsertInstruction(newCmpInstr, exitCmpInstr);
            allInstructions.insert(newCmpInstr);
            exitCmpInstr->replaceAllUsageTo(newCmpInstr);
            if (exitCmpInstr->useList.empty())
            {
                exitCmpInstr->waste();
                allInstructions.erase(exitCmpInstr);
            }
            return std::make_pair(newCmpInstr, exitBrInstr->getFalseBlock());
        }
        else
            return std::make_pair(nullptr, nullptr);
    }

    void SimpleLoopInformation::Builder::analyzePhiInstruction(PhiInstruction *instr)
    {
        if (instr->getParentBB() != loopEntryBlock)
            return;
        Value *initVal = nullptr;
        Value *exitVal = nullptr;
        for (int i = 0; i < instr->getValueBBSize(); i++)
        {
            auto [value, bb] = instr->getValueBB(i);
            if (allBlocks.find(bb) == allBlocks.end())
            {
                // std::cerr << "value: " << value->getIRName() << std::endl;
                // std::cerr << "bb: " << bb->getIRName() << std::endl;
                if (!value->isConstantInt32())
                    return;

                if (initVal == nullptr)
                    initVal = value;
                else
                    return;
            }
            else // 循环变量的退出值
            {
                if (!value->isInstruction() || (allInstructions.find(static_cast<Instruction *>(value)) == allInstructions.end()))
                    return;

                if (exitVal == nullptr)
                    exitVal = value;
                else
                    return;
            }
        }
        if (initVal == nullptr || exitVal == nullptr)
            return;
        initValue = initVal;
        exitValue = exitVal;
        return;
    }

    void SimpleLoopInformation::Builder::getAllBlocksAndInstructions(Loop *loop)
    {
        auto loopBlocks = loop->getBasicBlocks();
        allBlocks.insert(loopBlocks.begin(), loopBlocks.end());
        for (auto bb : loopBlocks)
        {
            for (ListNode *i = bb->getInstruction().begin(); i != bb->getInstruction().end(); i = i->nextNode())
            {
                auto instr = static_cast<Instruction *>(i);
                allInstructions.insert(instr);
            }
        }
        for (Loop *subLoop : loop->getSubLoops())
        {
            getAllBlocksAndInstructions(subLoop);
        }
    }

    SimpleLoopInformation::SimpleLoopInformation(BinaryInstruction *loopIncrementInstruction, ICmpInstruction *loopExitCompareInstruction,
                                                 Value *initValue, BasicBlock *exitBlock)
        : loopIncrementInstruction(loopIncrementInstruction), loopExitCompareInstruction(loopExitCompareInstruction),
          initValue(initValue), loopNextBlock(exitBlock) {}

    int SimpleLoopInformation::getLoopNumbs()
    {
        if (!initValue->isConstantInt32())
            return -1; // 初始值不是常数
        if (!loopExitCompareInstruction->getOperand(1)->isConstantInt32())
            return -1; // 循环上界不是常数
        if (!loopIncrementInstruction->getOperand(1)->isConstantInt32())
            return -1; // 循环增量不是常数

        int init = static_cast<ConstantInt32 *>(initValue)->getValue();
        int inc = static_cast<ConstantInt32 *>(loopIncrementInstruction->getOperand(1))->getValue();
        if (loopIncrementInstruction->getOpcode() == Instruction::Sub)
            inc = -inc;

        int exitValue = static_cast<ConstantInt32 *>(loopExitCompareInstruction->getOperand(1))->getValue();
        auto cmp = loopExitCompareInstruction->getOpcode();
        if (cmp == ICmpInstruction::Eq)
        {
            if (init != exitValue)
                return 0;
            else
            {
                if (inc == 0)
                    return -1;
                else
                    return 1;
            }
        }
        else if (cmp == ICmpInstruction::Ne)
        {
            if (init == exitValue)
                return 0;
            if (inc == 0)
                return -1;
            if ((exitValue - init) % inc != 0)
                return -1;
            int loopNumbs = (exitValue - init) / inc;
            if (loopNumbs < 0)
                return -1;
            else
                return loopNumbs;
        }
        else if (cmp == ICmpInstruction::Lt) // a < b
        {
            if (inc <= 0)
                return -1;
            if (init >= exitValue)
                return 0;
            return (exitValue - init + inc - 1) / inc;
        }
        else if (cmp == ICmpInstruction::Le) // a <= b
        {
            if (inc <= 0)
                return -1;
            if (init > exitValue)
                return 0;
            return (exitValue - init) / inc + 1;
        }
        else if (cmp == ICmpInstruction::Gt) // a > b
        {
            if (inc >= 0)
                return -1;
            if (init <= exitValue)
                return 0;
            return (init - exitValue + inc - 1) / (-inc);
        }
        else if (cmp == ICmpInstruction::Ge) // a >= b
        {
            if (inc >= 0)
                return -1;
            if (init < exitValue)
                return 0;
            return (init - exitValue) / (-inc) + 1;
        }
        else
        {
            return -1;
        }
    }

    SimpleLoopInformation *SimpleLoopInformation::analyze(Loop *loop)
    {
        Builder builder;
        return builder.buildLoopInformation(loop);
    }
}