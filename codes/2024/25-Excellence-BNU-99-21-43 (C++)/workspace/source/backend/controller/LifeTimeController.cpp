#include "LifeTimeController.h"

namespace Backend
{
    void LifeTimeController::init() // 初始化，清空所有字段【输入：无】【输出：无】
    {
        lifeTimeRange.clear();
        lifeTimeIntervals.clear();
        lifeTimePoints.clear();
        blocks.clear();
        blockMap.clear();
        instIDMap.clear();
    }

    std::set<AsmOperandRegister *> LifeTimeController::difference(std::set<AsmOperandRegister *> a, std::set<AsmOperandRegister *> b) // 寄存器集合a-寄存器集合b【输入：寄存器集合a,寄存器集合b】【输出：寄存器集合a-寄存器集合b】
    {
        std::set<AsmOperandRegister *> result;
        for (const auto &x : a)
            if (b.find(x) == b.end())
                result.insert(x);
        return result;
    }

    bool LifeTimeController::setInsert(std::set<AsmOperandRegister *> &dst, std::set<AsmOperandRegister *> src)
    {
        bool hasChange = false;
        for (auto x : src)
            if (dst.find(x) == dst.end())
            {
                hasChange = true;
                dst.insert(x);
            }
        return hasChange;
    }

    void LifeTimeController::buildBlocks(AsmInstBasicList instructionList) // 将给定的源指令列表构建成基本块【输入：源指令列表】【输出：无】
    {
        Block *now = nullptr;
        const auto &list = instructionList.getList();

        for (int i = 0; i < (int)list.size(); i++)
        {
            if (list[i]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel)
            {
                now = new Block(dynamic_cast<AsmInstLabel *>(list[i]));
                now->l = i;
                blocks.push_back(now);
                blockMap[now->blockName] = now;
            }
            if (!now)
                continue;
            if (list[i]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstJump)
            {
                if (now->startBranch == -1)
                    now->startBranch = i;
                for (int j = 0; j < 3; ++j)
                    if (list[i]->getOperand(j) && list[i]->getOperand(j)->isLabel())
                        now->nextBlockName.insert(list[i]->getOperand(j)->toString());
            }


            std::set<Backend::AsmOperandRegister *> temp1 = AsmInstructions::getReadRegSet(list[i]);
            for (auto reg : temp1)
                if (now->def.find(reg) == now->def.end())
                    now->in.insert(reg);

            std::set<Backend::AsmOperandRegister *> temp2 = AsmInstructions::getWriteRegSet(list[i]);
            now->def.insert(temp2.begin(), temp2.end());

            // std::cerr << "    [" << i << "] " << list[i]->toString() << std::endl;
            // std::cerr << "      : <read> ";
            // for (auto x : temp1)
            //     std::cerr << x->toString() << " ";
            // std::cerr << std::endl;
            // std::cerr << "      : <write> ";
            // for (auto x : temp2)
            //     std::cerr << x->toString() << " ";
            // std::cerr << std::endl;


            blocks[blocks.size() - 1]->r = i;
        }

        if (function->getReturnReg() != nullptr)
            blocks[blocks.size() - 1]->in.insert(function->getReturnReg());

        // std::cerr << "\n\n[LifeTimeController] Blocks: " << blocks.size() << std::endl;
        // for (int i = 0; i < (int)blocks.size(); i++)
        // {
        //     std::cerr << "  Block " << i << std::endl;
        //     std::cerr << "          : <name> " << blocks[i]->blockName << std::endl;
        //     std::cerr << "          : <l,r,stt_branch>" << blocks[i]->l << " " << blocks[i]->r << " " << blocks[i]->startBranch << std::endl;
        //     std::cerr << "          : <in> ";
        //     for (auto x : blocks[i]->in)
        //         std::cerr << x->toString() << " ";
        //     std::cerr << std::endl;
        //     std::cerr << "          : <out> ";
        //     for (auto x : blocks[i]->out)
        //         std::cerr << x->toString() << " ";
        //     std::cerr << std::endl;
        //     std::cerr << "          : <def> ";
        //     for (auto x : blocks[i]->def)
        //         std::cerr << x->toString() << " ";
        //     std::cerr << std::endl;
        //     std::cerr << "          : <next> ";
        //     for (auto x : blocks[i]->nextBlockName)
        //         std::cerr << x << " ";
        //     std::cerr << std::endl;
        //     std::cerr << "          : <insts> \n";
        //     for (int j = blocks[i]->l; j <= blocks[i]->r; j++)
        //         std::cerr << "              [" << j << "] " << list[j]->toString() << std::endl;
        // }
    }

    void LifeTimeController::iterateActiveReg() // in集合和out集合更新的不动集算法【输入：无】【输出：无】//TODO
    {
        while (true)
        {
            bool hasChange = false;
            for (int i = 0; i < (int)blocks.size() - 1; i++)
            {
                Block *b = blocks[i];

                for (auto nextBlockName : b->nextBlockName)
                {
                    Block *nextBlock = blockMap[nextBlockName];
                    if (nextBlock == nullptr)
                    {
                        std::string s;
                        for (auto [key, value] : blockMap)
                            s += key + ",";
                        Error::Error(__PRETTY_FUNCTION__, "Block not found, block name: " + nextBlockName + " all block names: " + s);
                    }

                    hasChange |= setInsert(b->out, nextBlock->in);
                }
                hasChange |= setInsert(b->in, difference(b->out, b->def));
            }
            if (!hasChange)
                break;
        }
    }

    void LifeTimeController::buildLifeTimePoints(AsmInstBasicList instructionList) // 构建所有寄存器的生命周期点【输入：源指令列表】【输出：无】
    {
        LifeTimeIndex *functionStartIndex = LifeTimeIndex::getInstOut(this, instructionList.getList()[0]); // 获取第一个指令的LifeTimeIndex并存储在functionStartIndex中

        for (auto reg : function->getArgRegs())                                  // 遍历函数的参数寄存器列表
            insertLifeTimePoint(reg, LifeTimePoint::getDef(functionStartIndex)); // 为每个参数寄存器在functionStartIndex处插入[定义点]

        for (int i = 0; i < (int)blocks.size() - 1; ++i) // 遍历所有基本块
        {
            auto b = blocks[i];
            for (auto x : b->in) // 对于当前基本块b的所有入口寄存器b.in
            {
                LifeTimeIndex *index = LifeTimeIndex::getInstOut(this, instructionList.getList()[b->l]); // 获取基本块[开始]处(b.l)指令的LifeTimeIndex
                insertLifeTimePoint(x, LifeTimePoint::getDef(index));                                    // 在该位置插入[定义点]
            }
            for (int j = b->l; j <= b->r; ++j) // 遍历基本块内的指令
            {
                auto inst = instructionList.getList()[j];

                for (auto x : AsmInstructions::getReadRegSet(inst)) // 指令的读寄存器集合
                {
                    LifeTimeIndex *index = LifeTimeIndex::getInstIn(this, instructionList.getList()[j]); // 获取指令[入口]的LifeTimeIndex
                    insertLifeTimePoint(x, LifeTimePoint::getUse(index));                                // 在该位置插入[使用点]
                }
                for (auto x : AsmInstructions::getWriteRegSet(inst))
                {                                                                                         // 指令的写寄存器集合
                    LifeTimeIndex *index = LifeTimeIndex::getInstOut(this, instructionList.getList()[j]); // 获取指令[出口]的LifeTimeIndex
                    insertLifeTimePoint(x, LifeTimePoint::getDef(index));                                 // 在该位置插入[定义点]
                }
            }
            for (auto x : b->out) // 对于当前基本块b的所有[出口]寄存器b.out
            {
                LifeTimeIndex *index = LifeTimeIndex::getInstIn(this, instructionList.getList()[b->startBranch]); // 获取基本块[分支]处指令的LifeTimeIndex
                insertLifeTimePoint(x, LifeTimePoint::getUse(index));                                             // 在该位置插入[使用点]
            }
        }

        for (auto constantReg : Registers::CONSTANT_REGISTERS) // 删除常量寄存器的生命周期点
            removeReg(constantReg);
    }

    void LifeTimeController::buildLifeTimeMessage(AsmInstBasicList instructionList)
    {
        buildLifeTimePoints(instructionList);
        std::vector<AsmOperandRegister *> removeList;

        std::set<Backend::AsmOperandRegister *> temp = getRegKeySet();
        for (auto x : temp)
            if (!constructInterval(x))
                removeList.push_back(x);

        for (auto x : removeList)
            removeReg(x);
    }

    AsmOperandRegister *LifeTimeController::getVReg(int index) // 获取分配给函数的虚拟寄存器【输入：源虚拟寄存器基于vregMap的索引】【输出：分配给函数的虚拟寄存器】
    {
        return function->getRegisterAllocator()->get(index);
    }

    void LifeTimeController::buildInstID(AsmInstBasicList instructionList) // 创建insIDMap【输入：源指令列表】【输出：无】
    {
        idSourceInst = instructionList;
        instIDMap.clear();
        for (int i = 0; i < (int)instructionList.getList().size(); i++)
            instIDMap[instructionList.getList()[i]] = i;
    }

    int LifeTimeController::getInstID(AsmInstBasic *inst) // 获取指令对应的编号【输入：源指令】【输出：基于instIDMap源指令对应的编号】
    {
        return instIDMap[inst];
    }

    bool LifeTimeController::containsInst(AsmInstBasic *inst) const // 确认instIDMap是否包含某指令【输入：源指令】【输出：是否包含的布尔值】
    {
        if (instIDMap.find(inst) == instIDMap.end())
            return false;
        return true;
    }

    void LifeTimeController::replaceInst(AsmInstBasic *replacedInst, AsmInstBasic *newInst) // 从instIDMap中把某指令换成新的指令【输入：待更换的旧指令，新指令】【输出：无】
    {
        int id = instIDMap[replacedInst];
        instIDMap[newInst] = id;
        instIDMap.erase(replacedInst);
    }

    std::set<int> LifeTimeController::getVRegKeySet() // 获取基于lifeTimePoints的键值[虚拟]寄存器的绝对寄存器索引集合【输入：无】【输出：基于lifeTimePoints的键值[虚拟]寄存器的绝对寄存器索引集合】
    {
        std::set<int> res;
        for (auto it : lifeTimePoints)
            if (it.first->isVirtualRegister())
                res.insert(it.first->getAbsRegIndex());
        return res;
    }

    std::set<AsmOperandRegister *> LifeTimeController::getRegKeySet() // 获取基于lifeTimePoints的键值[所有]寄存器集合【输入：无】【输出：基于lifeTimePoints的[所有]虚拟寄存器集合】
    {
        std::set<AsmOperandRegister *> res;
        for (auto it : lifeTimePoints)
            res.insert(it.first);
        return res;
    }

    std::pair<LifeTimeIndex *, LifeTimeIndex *> LifeTimeController::getLifeTimeRange(int index) // 获取源虚拟寄存器的生命周期范围【输入：源虚拟寄存器基于vregMap的索引】【输出：源虚拟寄存器的生命周期范围】
    {
        return lifeTimeRange[getVReg(index)];
    }

    std::pair<LifeTimeIndex *, LifeTimeIndex *> LifeTimeController::getLifeTimeRange(AsmOperandRegister *reg) // 获取源虚拟寄存器的生命周期范围【输入：源虚拟寄存器】【输出：源虚拟寄存器的生命周期范围】
    {
        return lifeTimeRange[reg];
    }

    void LifeTimeController::insertLifeTimePoint(AsmOperandRegister *reg, LifeTimePoint *p) // 在lifeTimePoints中根据源寄存器[插入]生命周期点【输入：源寄存器,源生命周期点】【输出：无】
    {
        if (lifeTimePoints.find(reg) == lifeTimePoints.end())
            lifeTimePoints[reg] = std::vector<LifeTimePoint *>();
        lifeTimePoints[reg].push_back(p);
    }

    void LifeTimeController::removeLifeTimePoint(AsmOperandRegister *reg, LifeTimePoint *p) // 在lifeTimePoints中根据源寄存器[删除]生命周期点【输入：源寄存器,源生命周期点】【输出：无】
    {
        auto &it = lifeTimePoints[reg];
        it.erase(std::remove(it.begin(), it.end(), p), it.end());
    }

    void LifeTimeController::removeReg(AsmOperandRegister *reg) // 根据待删除的源寄存器删除相应的生命周期信息记录【输入：源寄存器】【输出：无】
    {
        lifeTimePoints.erase(reg);
        lifeTimeRange.erase(reg);
        lifeTimeIntervals.erase(reg);
    }

    void LifeTimeController::mergePoints(AsmOperandRegister *x, AsmOperandRegister *y) // 把y的引用点集合合并入x
    {
        if (*x == *y || lifeTimePoints.find(y) == lifeTimePoints.end())
            return;
        if (lifeTimePoints.find(x) != lifeTimePoints.end())
        {
            lifeTimePoints[x].insert(lifeTimePoints[x].end(), lifeTimePoints[y].begin(), lifeTimePoints[y].end());
            std::sort(lifeTimePoints[x].begin(), lifeTimePoints[x].end(), [](LifeTimePoint const *a, LifeTimePoint const *b)
                      { return (*a) < (*b); });
        }
        else
            lifeTimePoints[x] = lifeTimePoints[y];
        removeReg(y);
    }

    void LifeTimeController::mergePoints(int x, int y)
    {
        mergePoints(getVReg(x), getVReg(y));
    }

    bool LifeTimeController::constructInterval(AsmOperandRegister *x) // 为源寄存器构建生命周期区间【输入：源寄存器】【输出：成功构建的布尔值】
    {
        if (lifeTimePoints.find(x) == lifeTimePoints.end())
            throw std::invalid_argument("Invalid register");

        // 获取 lifeTimePoints 中的 x 对应的向量
        auto &points = lifeTimePoints[x];

        // 使用 std::remove_if 移除不符合条件的元素
        points.erase(std::remove_if(points.begin(), points.end(), [&](LifeTimePoint *point)
                                    {
            // 如果 point 的索引的源指令不在 instIDMap 中，则移除该 point
            if (instIDMap.find(point->getIndex()->getSrcInst()) == instIDMap.end()) {
                return true;
            }

            // 获取 point 的源指令
            AsmInstBasic* instr = point->getIndex()->getSrcInst();

            // 如果源指令是 AsmLabel 或 AsmJump 类型，则保留该 point
            if ((instr->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstJump) || (instr->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel)) {
                return false;
            }

            // 如果指令中不包含寄存器 x，则移除该 point
            return !AsmInstructions::instContainsReg(instr, x); }),
                     points.end());

        if (lifeTimePoints[x].empty())
            return false;

        std::sort(lifeTimePoints[x].begin(), lifeTimePoints[x].end(), [](LifeTimePoint const *a, LifeTimePoint const *b)
                  { return (*a) < (*b); });

        lifeTimeRange[x] = std::pair<LifeTimeIndex *, LifeTimeIndex *>(lifeTimePoints[x][0]->getIndex(), lifeTimePoints[x][lifeTimePoints[x].size() - 1]->getIndex());

        std::vector<LifeTimeInterval *> intervals;

        for (auto p : lifeTimePoints[x])
        {
            if (p->isDef())
                intervals.push_back(new LifeTimeInterval(x, std::pair<LifeTimeIndex *, LifeTimeIndex *>(p->getIndex(), nullptr)));

            if (intervals.empty())
                throw std::runtime_error("F!");

            intervals.back()->reglifetime.second = p->getIndex();
        }
        lifeTimeIntervals[x] = intervals;
        return true;
    }

    std::vector<LifeTimeInterval *> LifeTimeController::getInterval(int x)
    {
        return lifeTimeIntervals[getVReg(x)];
    }

    std::vector<LifeTimeInterval *> LifeTimeController::getInterval(AsmOperandRegister *reg)
    {
        return lifeTimeIntervals[reg];
    }

    std::vector<LifeTimePoint *> LifeTimeController::getPoints(int x)
    {
        return lifeTimePoints[getVReg(x)];
    }

    std::vector<LifeTimePoint *> LifeTimeController::getPoints(AsmOperandRegister *reg)
    {
        return lifeTimePoints[reg];
    }

    void LifeTimeController::getAllRegLifeTime(AsmInstBasicList instlist)
    {
        init();
        buildInstID(instlist);
        buildBlocks(instlist);
        iterateActiveReg();
        buildLifeTimeMessage(instlist);
    }

    void LifeTimeController::rebuildLifeTimeInterval(AsmInstBasicList instlist)
    {
        buildInstID(instlist);

        std::vector<AsmOperandRegister *> removeList;
        for (auto x : getRegKeySet())
            if (!constructInterval(x))
                removeList.push_back(x);

        for (auto x : removeList)
            removeReg(x);
    }
}