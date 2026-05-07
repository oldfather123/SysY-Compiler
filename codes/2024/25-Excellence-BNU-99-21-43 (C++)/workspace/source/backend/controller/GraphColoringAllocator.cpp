#include "GraphColoringAllocator.h"

namespace Backend
{
    GraphColoringAllocator::GraphColoringAllocator(AsmFunction *function, AsmStackAllocator *allocator)
        : RegisterControl(function, allocator), ltc(function->getLifeTimeController())
    // 这里缺一个函数
    { }

    void GraphColoringAllocator::Cost(std::vector<AsmOperandRegister *> aors)
    {
        auto irFunction = function->getIRFunction();
        auto loopForest = IR::LoopForest::buildOver(irFunction);
        auto basicBlockLoopMap = loopForest->getBasicBlockLoopMap();
        std::map<LifeTimeIndex *, IR::BasicBlock *> indexBlockMap;
        std::vector<LifeTimeIndex *> blockIndexList;

        // 构建 indexBlockMap 和 blockIndexList
        for (const auto &instr : instList.getList()) // 假设 instList.getList() 返回指令列表
        {
            if (auto label = dynamic_cast<AsmInstLabel *>(instr); label)
            {
                auto index = LifeTimeIndex::getInstIn(ltc, instr);
                blockIndexList.push_back(index);
                indexBlockMap[index] = function->getBB(label)->getIRBB();
            }
        }

        for (auto reg : aors)
        {
            int nowID = 0;
            spillCosts[reg] = 0;

            for (const auto &point : ltc->getPoints(reg))
            {
                // 如果寄存器在该指令中没有被使用，跳过
                if (AsmInstructions::getInstRegID(point->getIndex()->getSrcInst(), reg).empty())
                {
                    continue;
                }

                // 更新 nowID 以找到对应的基本块索引
                while (nowID + 1 < (int)blockIndexList.size() && *blockIndexList[nowID + 1] < *point->getIndex())
                {
                    nowID += 1;
                }

                auto index = blockIndexList[nowID];
                auto block = indexBlockMap[index];
                auto loop = basicBlockLoopMap[block];
                int val = 1;

                // 如果当前指令在循环内，则根据循环深度增加 spillCost
                if (loop)
                {
                    int loopDepth = loop->getLoopDepth();
                    for (int i = 0; i <= loopDepth; i++)
                    {
                        val = std::min(1ll * inf, val * 10ll);
                    }
                }

                spillCosts[reg] = std::min(inf, spillCosts[reg] + val);
            }

            // 如果寄存器不是虚拟寄存器，移除其 spillCost
            if (reg->getRegIndex() >= 0)
            {
                spillCosts.erase(reg);
            }
        }
    }

    void GraphColoringAllocator::addInterfaceEdge(AsmOperandRegister *u, AsmOperandRegister *v)
    {
        if (u != v)
        {
            coalescentEdges[u].erase(v);
            coalescentEdges[v].erase(u);
            interferenceEdges[u].insert(v);
            interferenceEdges[v].insert(u);
        }
    }

    void GraphColoringAllocator::addCoalesceEdge(AsmOperandRegister *u, AsmOperandRegister *v)
    {
        if (u != v && interferenceEdges.find(v) == interferenceEdges.end())
        {
            coalescentEdges[u].insert(v);
            coalescentEdges[v].insert(u);
        }
    }

    void GraphColoringAllocator::removeEdge(AsmOperandRegister *u, AsmOperandRegister *v)
    {
        if (interferenceEdges.find(v) != interferenceEdges.end())
        {
            interferenceEdges[u].erase(v);
            interferenceEdges[v].erase(u);
        }
        if (coalescentEdges.find(v) != interferenceEdges.end())
        {
            coalescentEdges[u].erase(v);
            coalescentEdges[v].erase(u);
        }
    }

    LifeTimeInterval *GraphColoringAllocator::getIntervalValue(LifeTimeInterval *interval)
    {
        // 查找 interval 是否已经存在于 intervalValues 中
        auto it = intervalValues.find(interval);
        if (it == intervalValues.end())
        {
            // 如果不存在，将 interval 插入到 intervalValues 中
            intervalValues[interval] = interval;
        }
        // 返回 interval 的值
        return intervalValues[interval];
    }

    void GraphColoringAllocator::GraphCreater(bool freeze, std::vector<AsmOperandRegister *> &rs)
    {
        // 清空现有的数据
        intervals.clear();
        interferenceEdges.clear();
        coalescentEdges.clear();

        // 处理寄存器
        for (auto reg : rs)
        {
            auto regIntervals = ltc->getInterval(reg);                                   // 获取寄存器的生命周期区间
            intervals.insert(intervals.end(), regIntervals.begin(), regIntervals.end()); // 添加到 intervals

            interferenceEdges[reg] = std::set<AsmOperandRegister *>(); // 初始化干涉边集合
            coalescentEdges[reg] = std::set<AsmOperandRegister *>();   // 初始化合并边集合
        }

        // 对生命周期区间进行排序
        std::sort(intervals.begin(), intervals.end(), [](const LifeTimeInterval *a, const LifeTimeInterval *b)
                  { return *a < *b; });

        std::set<LifeTimeInterval *> activeSet;
        std::map<AsmOperandRegister *, LifeTimeInterval *> lastActive;

        // 处理指令
        for (auto inst : instList.getList())
        {
            if (auto asmMove = dynamic_cast<AsmInstMove *>(inst); asmMove && !freeze)
            {
                auto regPair = AsmInstructions::getMoveReg(asmMove);
                if (std::find(rs.begin(), rs.end(), regPair.first) != rs.end() &&
                    std::find(rs.begin(), rs.end(), regPair.second) != rs.end())
                {
                    addCoalesceEdge(regPair.first, regPair.second);
                }
            }
        }

        intervalValues.clear();

        // 处理常量寄存器
        for (auto reg : Registers::CONSTANT_REGISTERS)
        {
            std::pair<LifeTimeIndex *, LifeTimeIndex *> constlifetime = {nullptr, nullptr};
            lastActive[reg] = new LifeTimeInterval(reg, constlifetime);
        }

        // 处理生命周期区间
        for (auto &now : intervals)
        {
            // 移除过期的区间
            for (auto it = activeSet.begin(); it != activeSet.end();)
            {
                if ((*(*it)->getRegLifetime().second) < (*now->getRegLifetime().first))
                {
                    it = activeSet.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            auto defInst = now->reglifetime.first->getSrcInst();
            if (auto asmMove = dynamic_cast<AsmInstMove *>(defInst); asmMove && !freeze)
            {
                auto sourceReg = AsmInstructions::getMoveReg(asmMove).second;
                intervalValues[now] = getIntervalValue(lastActive[sourceReg]);
            }

            for (auto last : activeSet)
            {
                auto ru = now->reg;
                auto rv = last->reg;
                if (*getIntervalValue(last) == *getIntervalValue(now))
                {
                    addCoalesceEdge(ru, rv);
                }
                else
                {
                    addInterfaceEdge(ru, rv);
                }
            }

            activeSet.insert(now);
            lastActive[now->reg] = now;
        }

        // 清空未着色和可合并寄存器
        uncoloredReg.clear();
        coalescentReg.clear();

        // 分类寄存器
        for (auto reg : rs)
        {
            if (reg->getRegIndex() < 0)
            { // 检测虚拟寄存器
                if (coalescentEdges[reg].empty())
                {
                    uncoloredReg.insert(reg);
                }
                else
                {
                    coalescentReg.insert(reg);
                }
            }
        }
    }

    // As a STOPPER
    void GraphColoringAllocator::getNeedReturnAfterCall() /* 保留call后还要用的寄存器 */
    {
        // 清空寄存器集合，用于存储在函数调用后仍需使用的寄存器
        RegAcrossC.clear();
        // int intervalId = 0;                     // 初始化区间ID，用于跟踪当前处理的生存期区间
        std::set<LifeTimeInterval *> activeSet; // 用于存储当前活动的生存期区间

        // 遍历指令列表
        for (int i = 0; i < (int)instList.getList().size(); ++i)
        {
            // 获取当前指令
            AsmInstBasic *inst = instList.getList()[i];

            // 获取当前指令的入集合生命周期索引
            LifeTimeIndex *inIndex = LifeTimeIndex::getInstIn(ltc, inst);

            // 移除那些生命周期结束小于等于 inIndex 的区间
            for (auto it = activeSet.begin(); it != activeSet.end();)
            {
                if ((*(*it)->reglifetime.second) <= (*inIndex))
                {
                    it = activeSet.erase(it); // 移除已过期的生存期区间
                }
                else
                {
                    ++it; // 继续检查下一个区间
                }
            }

            // 检查当前指令是否为函数调用指令
            if (dynamic_cast<AsmInstCall *>(inst))
            {
                // 将所有当前活动的区间对应的寄存器加入 RegAcrossC
                for (LifeTimeInterval *interval : activeSet)
                {
                    RegAcrossC.insert(interval->reg);
                }
            }

            // 获取当前指令的出集合生命周期索引
            LifeTimeIndex *outIndex = LifeTimeIndex::getInstOut(ltc, inst);

            auto intervalIt = intervals.begin();
            // 将新的生命周期区间加入 activeSet，直到处理完当前指令的所有相关区间
            while (intervalIt != intervals.end() && (*((*intervalIt)->getRegLifetime()).first) <= (*outIndex))
            {
                activeSet.insert(*intervalIt);
                ++intervalIt; // 移动到下一个区间
            }
        }
    }

    bool GraphColoringAllocator::Color()
    {
        std::queue<AsmOperandRegister *> queue; // 用于存储干涉边数小于物理寄存器数的寄存器
        std::set<AsmOperandRegister *> visited; // 用于跟踪已经访问过的寄存器

        // 将所有干涉边数小于物理寄存器数的虚拟寄存器加入队列
        for (AsmOperandRegister *x : uncoloredReg)
        {
            if (interferenceEdges[x].size() < physicRegs.size())
            {
                queue.push(x);     // 将寄存器加入队列
                visited.insert(x); // 标记为已访问
            }
        }

        // 处理队列中的寄存器
        while (!queue.empty())
        {
            AsmOperandRegister *v = queue.front(); // 获取队列前端的寄存器
            queue.pop();                           // 将该寄存器从队列中移除
            RegStack.push(v);                      // 将寄存器压入栈中，用于后续着色
            uncoloredReg.erase(v);                 // 从未着色寄存器集合中移除该寄存器

            std::set<AsmOperandRegister *> edges = interferenceEdges[v]; // 获取与该寄存器干涉的所有寄存器
            for (AsmOperandRegister *u : edges)
            {
                removeEdge(u, v); // 移除寄存器 u 和 v 之间的干涉边
                if (interferenceEdges[u].size() < physicRegs.size())
                {
                    // 如果寄存器 u 的干涉边数小于物理寄存器数，并且 u 还没有被着色或访问过
                    if (uncoloredReg.count(u) > 0 && visited.count(u) == 0)
                    {
                        queue.push(u);     // 将寄存器 u 加入队列
                        visited.insert(u); // 标记寄存器 u 为已访问
                    }
                }
            }
        }

        // 如果未着色寄存器集合和联合寄存器集合都为空，返回 true，否则返回 false
        return uncoloredReg.empty() && coalescentReg.empty();
    }

    void GraphColoringAllocator::colorToRegisterMap(std::vector<AsmOperandRegister *> &rs)
    {
        // 构建干涉图并获取跨调用的寄存器
        GraphCreater(true, rs);   // 构建干涉图
        getNeedReturnAfterCall(); // 获取在函数调用后仍需保留的寄存器

        // 初始化物理寄存器映射
        for (AsmOperandRegister *reg : rs)
        {
            if (reg->getRegIndex() >= 0) // 如果寄存器不是虚拟寄存器
            {
                physicRegMap[reg] = reg; // 将其映射到其自身
            }
        }

        std::set<AsmOperandRegister *> usedRegister; // 用于存储已使用的物理寄存器

        // 分配寄存器
        while (!RegStack.empty())
        {
            AsmOperandRegister *v = RegStack.top(); // 获取栈顶的寄存器
            RegStack.pop();                         // 将栈顶寄存器移除

            std::map<AsmOperandRegister *, int> selectScore; // 用于记录每个物理寄存器的选择得分
            for (AsmOperandRegister *reg : physicRegs)
            {
                selectScore[reg] = 0; // 初始化所有物理寄存器的得分为0
            }

            // 减少干涉寄存器的得分
            if (interferenceEdges.find(v) == interferenceEdges.end())
            {
                Error::Error(__PRETTY_FUNCTION__, "interferenceEdges.find(v) == interferenceEdges.end(), v = " + v->toString());
            }
            for (AsmOperandRegister *u : interferenceEdges[v])
            {
                if (physicRegMap.find(u) != physicRegMap.end())
                {
                    selectScore[physicRegMap[u]] = -4; // 对于干涉的物理寄存器，减少其得分
                }
            }

            // 增加保存跨调用寄存器的得分
            for (AsmOperandRegister *pReg : physicRegs)
            {
                // 如果寄存器 v 在调用后需保留且物理寄存器 pReg 是调用后保存的，或者相反
                if ((RegAcrossC.find(v) != RegAcrossC.end() && Registers::isPreservedAcrossCalls(pReg)) ||
                    (RegAcrossC.find(v) == RegAcrossC.end() && !Registers::isPreservedAcrossCalls(pReg)))
                {
                    selectScore[pReg] += 2; // 增加其得分
                }
                if (usedRegister.count(pReg)) // 如果物理寄存器 pReg 已经被使用过
                {
                    selectScore[pReg] += 1; // 进一步增加其得分
                }
            }

            AsmOperandRegister *result = nullptr;
            for (AsmOperandRegister *pReg : physicRegs)
            {
                if (result == nullptr || selectScore[result] < selectScore[pReg])
                {
                    result = pReg; // 选择得分最高的物理寄存器
                }
            }

            physicRegMap[v] = result;    // 将虚拟寄存器 v 映射到选择的物理寄存器 result
            usedRegister.insert(result); // 将 result 标记为已使用
        }
    }

    bool GraphColoringAllocator::BriggsCheck(AsmOperandRegister *u, AsmOperandRegister *v)
    {
        // 如果寄存器 v 的干涉度小于寄存器 u 的干涉度，则交换 u 和 v
        if (interferenceEdges[u].size() < interferenceEdges[v].size())
        {
            std::swap(u, v);
        }

        // 计算寄存器 u 的初始干涉度
        int degree = interferenceEdges[u].size();

        // 遍历寄存器 v 的所有干涉寄存器
        for (AsmOperandRegister *x : interferenceEdges[v])
        {
            // 如果干涉度已经达到或超过物理寄存器的数量，则无需继续检查
            if (degree >= (int)physicRegs.size())
            {
                break;
            }

            // 如果寄存器 x 不在寄存器 u 的干涉边中，则增加 degree 计数
            // 这是因为 x 的干涉会增加寄存器 u 的干涉度
            if (interferenceEdges[u].find(x) == interferenceEdges[u].end())
            {
                ++degree;
            }
        }

        // 返回判断结果：干涉度是否小于物理寄存器的数量
        return degree < (int)physicRegs.size();
    }

    bool GraphColoringAllocator::GeorgeCheck(AsmOperandRegister *u, AsmOperandRegister *v)
    {
        // 遍历寄存器 v 的所有干涉寄存器
        for (AsmOperandRegister *x : interferenceEdges[v])
        {
            // 如果寄存器 x 不在寄存器 u 的干涉边中，返回 false
            if (interferenceEdges[u].find(x) == interferenceEdges[u].end())
            {
                return false;
            }
        }
        // 如果寄存器 v 的所有干涉寄存器都在寄存器 u 的干涉边中，返回 true
        return true;
    }

    void GraphColoringAllocator::mergeEdges(AsmOperandRegister *u, AsmOperandRegister *v)
    {
        // 移除 u 和 v 之间的干涉边
        removeEdge(u, v);

        // 合并 v 的干涉边到 u
        auto vInterferenceEdges = interferenceEdges[v];
        for (AsmOperandRegister *x : vInterferenceEdges)
        {
            addInterfaceEdge(u, x);
            removeEdge(v, x);
        }

        // 合并 v 的合并边到 u
        auto vCoalescentEdges = coalescentEdges[v];
        for (AsmOperandRegister *x : vCoalescentEdges)
        {
            addCoalesceEdge(u, x);
            removeEdge(v, x);
        }

        // 移除寄存器 v 的所有干涉边和合并边
        interferenceEdges.erase(v);
        coalescentEdges.erase(v);
    }

    bool GraphColoringAllocator::CheckCoalesce(AsmOperandRegister *u, AsmOperandRegister *v)
    {
        // 不是虚拟寄存器
        if (v->getRegIndex() > 0 || coalescentEdges.find(u) == coalescentEdges.end() || coalescentEdges[u].find(v) == coalescentEdges[u].end())
        {
            return false;
        }
        return BriggsCheck(u, v) || GeorgeCheck(u, v);
    }

    void GraphColoringAllocator::practiceCoalesce(AsmOperandRegister *u, AsmOperandRegister *v, std::vector<AsmOperandRegister *> &rs)
    {
        // 遍历寄存器 v 的所有生命周期点
        for (const auto &point : ltc->getPoints(v))
        {
            // 获取生命周期点所在的指令
            auto inst = point->getIndex()->getSrcInst();

            // 遍历指令中与寄存器 v 相关的操作数
            for (int id : AsmInstructions::getInstRegID(inst, v))
            {
                // 用寄存器 u 替换指令中的寄存器 v
                inst->setOperand(id, dynamic_cast<AsmRegisterInterface *>(inst->getOperand(id))->withRegister(u));
            }
        }

        // 合并寄存器 u 和 v 的生命周期点
        ltc->mergePoints(u, v);

        // 处理寄存器 u 的所有生命周期点
        for (auto point : ltc->getPoints(u))
        {
            auto inst = point->getIndex()->getSrcInst();

            // 检查指令是否是 AsmMove 类型
            if (auto iMove = dynamic_cast<AsmInstMove *>(inst))
            {
                auto regs = AsmInstructions::getMoveReg(iMove);
                // 如果移动指令中的两个寄存器相同
                if (regs.first == regs.second)
                {
                    // 从生命周期点中移除寄存器 u
                    ltc->removeLifeTimePoint(u, point);

                    // 如果指令仍在生命周期控制器中
                    if (ltc->containsInst(inst))
                    {
                        int idx = ltc->getInstID(inst);

                        // 替换指令为 AsmNop
                        auto nop = new AsmInstNop();
                        ltc->replaceInst(inst, nop);
                        instList.getList()[idx] = nop;
                    }
                }
            }
        }

        // 构建寄存器 u 的生命周期区间
        ltc->constructInterval(u);

        // 合并寄存器 u 和 v 的干涉边
        mergeEdges(u, v);

        // 如果寄存器 u 不再有合并边且是虚拟寄存器，将其移到未着色寄存器集合
        if (coalescentEdges[u].empty() && u->getRegIndex() < 0)
        {
            coalescentReg.erase(u);
            uncoloredReg.insert(u);
        }

        // 从合并寄存器集合中移除寄存器 v
        coalescentReg.erase(v);

        // 从寄存器列表中移除寄存器 v
        rs.erase(std::remove(rs.begin(), rs.end(), v), rs.end());

        // 从溢出成本计算中移除寄存器 v
        spillCosts.erase(v);
    }

    // 17:20
    bool GraphColoringAllocator::coalesce(std::vector<AsmOperandRegister *> &rs)
    {
        std::vector<std::pair<AsmOperandRegister *, AsmOperandRegister *>> coalescePair;

        // 构建要合并的寄存器对
        for (AsmOperandRegister *u : coalescentReg)
        {
            for (AsmOperandRegister *v : coalescentEdges[u])
            {
                coalescePair.push_back(std::make_pair(u, v));
            }
        }

        bool coalesced = false;

        // 尝试合并寄存器对
        for (auto &p : coalescePair)
        {
            if (CheckCoalesce(p.first, p.second))
            {
                practiceCoalesce(p.first, p.second, rs);
                coalesced = true;
            }
            else if (CheckCoalesce(p.second, p.first))
            {
                practiceCoalesce(p.second, p.first, rs);
                coalesced = true;
            }
        }

        return coalesced;
    }

    bool GraphColoringAllocator::freeze()
    {
        AsmOperandRegister *freezeReg = nullptr;

        // 找到接合寄存器集中干涉边最少的寄存器
        for (AsmOperandRegister *v : coalescentReg)
        {
            if (freezeReg == nullptr || interferenceEdges[v].size() < interferenceEdges[freezeReg].size())
            {
                freezeReg = v;
            }
        }

        if (freezeReg != nullptr)
        {
            // 从其他寄存器的接合边集中移除 freezeReg
            for (AsmOperandRegister *u : coalescentEdges[freezeReg])
            {
                coalescentEdges[u].erase(freezeReg);
            }

            // 将 freezeReg 从接合寄存器集中移到未着色寄存器集
            coalescentReg.erase(freezeReg);
            uncoloredReg.insert(freezeReg);

            return true;
        }

        return false;
    }

    double GraphColoringAllocator::costFunction(AsmOperandRegister *reg)
    {
        // 如果 reg 不在 spillCost 中，返回无穷大的两倍
        if (spillCosts.find(reg) == spillCosts.end())
        {
            return inf * 2;
        }

        // 获取干涉边的数量
        double degree = interferenceEdges[reg].size();

        // 计算并返回 spillCost / degree
        return spillCosts[reg] / degree;
    }

    void GraphColoringAllocator::practiseSpill(std::map<AsmOperandRegister *, AsmOperandStackVariable *> spilledRegs, std::vector<AsmOperandRegister *> &rs)
    {
        for (const auto &regPair : spilledRegs)
        {
            if (regPair.first->getRegIndex() > 0)
            {
                throw std::runtime_error("spilled physical register");
            }
        }

        // 从寄存器列表中移除所有溢出的寄存器
        // rs.erase(std::remove_if(rs.begin(), rs.end(),
        //                         [&spilledRegs](AsmOperandRegister *reg)
        //                         {
        //                             return spilledRegs.find(reg) != spilledRegs.end();
        //                         }),
        //          rs.end());

        for (auto it = rs.begin(); it != rs.end();)
        {
            if (spilledRegs.find(*it) != spilledRegs.end())
            // find 函数专门找的就是 key
            {
                it = rs.erase(it);
            }
            else
            {
                ++it;
            }
        }
        std::vector<AsmInstBasic *> spilledInstrList; // 用于存储插入溢出处理指令后的指令列表

        // 遍历指令列表处理每一条指令
        for (size_t i = 0; i < instList.getList().size(); ++i)
        {
            AsmInstBasic *inst = instList.getList()[i];

            // 处理指令中的读取寄存器
            for (AsmOperandRegister *reg : AsmInstructions::getReadRegSet(inst))
            {
                if (spilledRegs.find(reg) != spilledRegs.end())
                {
                    AsmOperandRegister *rLoad = function->getRegisterAllocator()->allocateRegister(reg);
                    std::vector<AsmInstBasic *> tmpl = loadFromStackVar(rLoad, spilledRegs[reg], addrReg);
                    workOnRegisterValue(rLoad, tmpl.back(), inst);

                    // 更新指令中的寄存器操作数
                    for (int id : AsmInstructions::getReadVRegId(inst))
                    {
                        AsmOperandBasic *operand = inst->getOperand(id);
                        if (AsmRegisterInterface *rp = dynamic_cast<AsmRegisterInterface *>(operand))
                        {
                            if (rp->getRegister() == reg)
                            {
                                inst->setOperand(id, rp->withRegister(rLoad));
                            }
                        }
                    }

                    // 插入加载指令到指令列表
                    spilledInstrList.insert(spilledInstrList.end(), tmpl.begin(), tmpl.end());
                    rs.push_back(rLoad); // 新分配的寄存器加入寄存器列表
                }
            }

            // 将原指令加入到新的指令列表中
            spilledInstrList.push_back(inst);

            // 处理指令中的写寄存器
            for (AsmOperandRegister *reg : AsmInstructions::getWriteRegSet(inst))
            {
                if (spilledRegs.find(reg) != spilledRegs.end())
                {
                    AsmOperandRegister *rStore = function->getRegisterAllocator()->allocateRegister(reg);
                    std::vector<AsmInstBasic *> tmpl = saveToStackVar(rStore, spilledRegs[reg], addrReg);
                    workOnRegisterValue(rStore, inst, tmpl.back());

                    // 更新指令中的寄存器操作数
                    for (int id : AsmInstructions::getWriteVRegId(inst))
                    {
                        AsmOperandBasic *operand = inst->getOperand(id);
                        if (AsmRegisterInterface *rp = dynamic_cast<AsmRegisterInterface *>(operand))
                        {
                            if (rp->getRegister() == reg)
                            {
                                inst->setOperand(id, rp->withRegister(rStore));
                            }
                        }
                    }

                    // 插入存储指令到指令列表
                    spilledInstrList.insert(spilledInstrList.end(), tmpl.begin(), tmpl.end());
                    rs.push_back(rStore); // 新分配的寄存器加入寄存器列表
                }
            }
        }

        // 更新指令列表为新的指令列表
        instList.setList(spilledInstrList);

        // 移除所有溢出的寄存器并重建生命周期间隔
        for (const auto &regPair : spilledRegs)
        {
            ltc->removeReg(regPair.first);
        }
        ltc->rebuildLifeTimeInterval(instList);
    }

    void GraphColoringAllocator::workOnRegisterValue(AsmOperandRegister *tmpReg, AsmInstBasic *last, AsmInstBasic *next)
    {
        // 获取指令 last 的输出点索引
        LifeTimeIndex *lastIndex = LifeTimeIndex::getInstOut(ltc, last);

        // 将 tmpReg 的定义点插入到生命时间控制器中
        ltc->insertLifeTimePoint(tmpReg, LifeTimePoint::getDef(lastIndex));

        // 获取指令 next 的输入点索引
        LifeTimeIndex *nextIndex = LifeTimeIndex::getInstIn(ltc, next);

        // 将 tmpReg 的使用点插入到生命时间控制器中
        ltc->insertLifeTimePoint(tmpReg, LifeTimePoint::getUse(nextIndex));
    }

    AsmOperandRegister *GraphColoringAllocator::getSpillDest()
    {
        // 初始化一个空指针用于保存目标寄存器
        AsmOperandRegister *dest = nullptr;

        // 遍历所有未上色的寄存器
        for (auto &reg : uncoloredReg)
        {
            // 如果当前目标寄存器为空，或当前寄存器的代价小于目标寄存器的代价，则更新目标寄存器
            if (dest == nullptr || costFunction(reg) < costFunction(dest))
            {
                dest = reg;
            }
        }

        // 如果没有找到合适的寄存器，则抛出异常
        if (dest == nullptr)
        {
            throw std::runtime_error("no uncolored register to spill");
        }

        // 返回选择的寄存器
        return dest;
    }

    void GraphColoringAllocator::spill(std::vector<AsmOperandRegister *> &rs)
    {
        // 存储将被溢出的寄存器列表
        std::vector<AsmOperandRegister *> spilledList;

        // 当还有未上色的寄存器时进行循环
        while (!uncoloredReg.empty())
        {
            std::queue<Backend::AsmOperandRegister *> queue;

            // 选择一个待溢出的寄存器
            Backend::AsmOperandRegister *dest = getSpillDest(); // 假设 getSpillDest() 是一个已定义的函数
            spilledList.push_back(dest);
            queue.push(dest);

            // 移除已溢出的寄存器
            uncoloredReg.erase(dest);

            // BFS 遍历溢出寄存器的干扰边
            while (!queue.empty())
            {
                AsmOperandRegister *u = queue.front();
                queue.pop();

                for (auto v : std::set<Backend::AsmOperandRegister *> (interferenceEdges[u]))
                {
                    removeEdge(u, v);
                    if (interferenceEdges[v].size() < physicRegs.size() &&
                        std::find(uncoloredReg.begin(), uncoloredReg.end(), v) != uncoloredReg.end())
                    {
                        queue.push(v);
                        // uncoloredReg.erase(std::remove(uncoloredReg.begin(), uncoloredReg.end(), v), uncoloredReg.end());
                        uncoloredReg.erase(v);
                    }
                }
            }
        }

        // 重建图以包含刚刚溢出的寄存器
        GraphCreater(true, spilledList);

        // 存储已保存的栈变量集合
        std::set<AsmOperandStackVariable *> savedSet;
        // 存储每个溢出寄存器对应的栈变量
        std::map<AsmOperandRegister *, AsmOperandStackVariable *> spilledRegSaved;

        for (auto u : spilledList)
        {
            // 查找与当前寄存器有冲突的寄存器所使用的栈变量
            std::set<AsmOperandStackVariable *> occupied;
            for (auto v : interferenceEdges[u])
            {
                if (spilledRegSaved.find(v) != spilledRegSaved.end())
                {
                    occupied.insert(spilledRegSaved[v]);
                }
            }

            // 为当前寄存器分配一个可用的栈变量
            for (auto stk : savedSet)
            {
                if (occupied.find(stk) == occupied.end())
                {
                    spilledRegSaved[u] = stk;
                    break;
                }
            }

            // 如果没有找到可用的栈变量，从栈池中获取一个新的栈变量
            if (spilledRegSaved.find(u) == spilledRegSaved.end())
            {
                AsmOperandStackVariable *save = stackPool.pop(); // 使用 StackPool 的 pop() 方法
                spilledRegSaved[u] = save;
                savedSet.insert(save);
            }
        }

        // 执行实际的溢出处理
        practiseSpill(spilledRegSaved, rs);
    }

    AsmInstBasicList GraphColoringAllocator::allocatePhysicalRegisters(AsmInstBasicList instructionList, std::vector<AsmOperandRegister *> &registers)
    {
        instList = instructionList;

        // 获取寄存器的使用成本
        Cost(registers);

        // 构建图
        GraphCreater(false, registers);


        // 清空栈
        while (!RegStack.empty())
        {
            RegStack.pop(); // 移除栈顶元素，直到栈为空
        }

        // 尝试为寄存器上色，并处理可能的溢出
        while (!Color())
        {
            if (!coalesce(registers))
            { // 如果寄存器无法合并
                if (!freeze())
                {                                   // 如果不能冻结寄存器
                    spill(registers);               // 执行溢出处理
                    GraphCreater(false, registers); // 重新构建干扰图
                    while (!RegStack.empty())
                    {
                        RegStack.pop(); // 移除栈顶元素，直到栈为空
                    }
                }
            }
        }

        // 将寄存器的颜色映射到物理寄存器上
        colorToRegisterMap(registers);

        // 返回更新后的指令列表
        return instList;
    }

    std::vector<AsmInstBasic *> GraphColoringAllocator::replacePhysicRegisters(AsmInstBasicList instructionList)
    {
        // 创建一个保存物理寄存器与栈变量对应关系的映射
        std::map<AsmOperandRegister *, AsmOperandStackVariable *> regSavedLoc;
        // 清空生命周期区间容器
        intervals.clear();

        // 更新寄存器保护状态
        for (const auto &regPair : physicRegMap)
        {
            updateRegisterPreserve(regPair.second); // values
        }

        // 收集所有生命周期区间
        for (const auto &regPair : physicRegMap)
        {
            auto regIntervals = ltc->getInterval(regPair.first);
            intervals.insert(intervals.end(), regIntervals.begin(), regIntervals.end());
        }

        // 对生命周期区间进行排序，按生命周期开始时间排序
        std::sort(intervals.begin(), intervals.end(), [](const LifeTimeInterval *a, const LifeTimeInterval *b)
                  { return *a < *b; });

        int intervalId = 0;
        std::set<LifeTimeInterval *> activeSet;

        // 创建映射以存储指令在生命周期中的入口和出口点
        std::map<LifeTimeIndex *, std::vector<AsmInstBasic *>> newInstList;
        std::map<LifeTimeIndex *, std::vector<std::pair<AsmOperandRegister *, LifeTimePoint *>>> pointsOnIndex;
        std::map<AsmInstBasic *, LifeTimeIndex *> instIn, instOut;

        // 初始化数据结构
        for (const auto &inst : instructionList.getList())
        {
            instIn[inst] = LifeTimeIndex::getInstIn(ltc, inst);
            instOut[inst] = LifeTimeIndex::getInstOut(ltc, inst);
            newInstList[instIn[inst]] = std::vector<AsmInstBasic *>();
            newInstList[instOut[inst]] = std::vector<AsmInstBasic *>();
            pointsOnIndex[instIn[inst]] = std::vector<std::pair<AsmOperandRegister *, LifeTimePoint *>>();
            pointsOnIndex[instOut[inst]] = std::vector<std::pair<AsmOperandRegister *, LifeTimePoint *>>();
        }

        // 创建一个映射以存储只定义一次的寄存器
        std::map<AsmOperandRegister *, LifeTimeIndex *> defOnce;
        for (const auto &regPair : physicRegMap)
        {
            auto reg = regPair.first;
            if (reg->getRegIndex() >= 0) // 不是虚拟寄存器
            {
                defOnce[reg] = nullptr;
            }
            for (auto p : ltc->getPoints(reg))
            {
                auto inst = p->getIndex()->getSrcInst();
                auto pReg = physicRegMap[reg];
                if (p->getIndex()->isIn())
                {
                    pointsOnIndex[instIn[inst]].emplace_back(pReg, p);
                }
                else
                {
                    pointsOnIndex[instOut[inst]].emplace_back(pReg, p);
                }
                if (p->isDef() && p->isTruePoint())
                {
                    if (defOnce.find(pReg) != defOnce.end())
                    {
                        defOnce[pReg] = nullptr;
                    }
                    else
                    {
                        defOnce[pReg] = p->getIndex();
                    }
                }
            }
        }

        // 创建映射以存储最后一次定义的寄存器和保存状态
        std::map<AsmOperandRegister *, LifeTimeIndex *> lastDef;
        std::map<AsmOperandRegister *, AsmOperandStackVariable *> preserved;
        std::map<AsmOperandRegister *, LifeTimeIndex *> lastPreservedIn;

        // 遍历指令列表进行处理
        for (const auto &inst : instructionList.getList())
        {
            auto inIndex = LifeTimeIndex::getInstIn(ltc, inst);

            // 使用临时容器来存储符合条件的元素，从 activeSet 中删除符合条件的元素
            std::set<Backend::LifeTimeInterval *> toRemove;
            for (const auto &interval : activeSet)
            {
                if ((*interval->getRegLifetime().second) <= (*inIndex))
                {
                    toRemove.insert(interval);
                }
            }
            for (auto interval : toRemove) // 注意这里不使用 const
            {
                activeSet.erase(interval);
            }

            // 处理入口点上的使用情况
            for (const auto &p : pointsOnIndex[instIn[inst]])
            {
                auto pReg = p.first;
                auto point = p.second;
                if (point->isUse() && preserved.find(pReg) != preserved.end())
                {
                    auto tmpl = loadFromStackVar(pReg, preserved[pReg], addrReg);
                    newInstList[instIn[inst]].insert(newInstList[instIn[inst]].end(), tmpl.begin(), tmpl.end());
                    preserved.erase(pReg);
                }
            }

            // 处理指令的操作数
            for (int i = 0; i < 3; i++)
            {
                // 获取操作数并尝试转换为 AsmRegisterInterface 类型
                auto operand = inst->getOperand(i);
                if (!operand)
                    continue;

                auto rp = dynamic_cast<AsmRegisterInterface *>(operand);

                // 如果操作数是虚拟寄存器，替换为物理寄存器
                if (rp && rp->getRegister()->isVirtualRegister())
                {
                    auto physicRegister = physicRegMap[rp->getRegister()];
                    // 使用物理寄存器更新操作数
                    inst->setOperand(i, rp->withRegister(physicRegister));
                }
            }

            // 处理调用指令
            if (dynamic_cast<AsmInstCall *>(inst))
            {
                // std::cerr << "    Call Instruction" << std::endl;
                for (const auto &interval : activeSet)
                {
                    auto physicReg = physicRegMap[interval->reg];

                    // std::cerr << "        " << physicReg->toString() 
                            // << " [check] isPreservedAcrossCalls: " 
                            // << Registers::isPreservedAcrossCalls(physicReg) 
                            // << ", preserved.find(physicReg) == preserved.end(): " 
                            // << (preserved.find(physicReg) == preserved.end()) << std::endl;
                    if (!Registers::isPreservedAcrossCalls(physicReg) && preserved.find(physicReg) == preserved.end())
                    {
                        if (regSavedLoc.find(physicReg) == regSavedLoc.end())
                        {
                            auto stk = stackPool.pop();
                            regSavedLoc[physicReg] = stk;
                        }
                        preserved[physicReg] = regSavedLoc[physicReg];
                        if (lastDef[physicReg] != lastPreservedIn[physicReg])
                        {
                            auto tmpl = saveToStackVar(physicReg, regSavedLoc[physicReg], addrReg);
                            // std::cerr << "        saveToStackVar: " << physicReg->toString() << " -> " << regSavedLoc[physicReg]->toString() << std::endl;
                            newInstList[lastDef[physicReg]].insert(newInstList[lastDef[physicReg]].end(), tmpl.begin(), tmpl.end());
                            lastPreservedIn[physicReg] = lastDef[physicReg];
                        }
                    }
                }
            }

            // 更新 activeSet 和 lastDef
            auto outIndex = LifeTimeIndex::getInstOut(ltc, inst);
            while (intervalId < (int)intervals.size() && (*(intervals[intervalId]->getRegLifetime().first)) <= (*outIndex))
            {
                activeSet.insert(intervals[intervalId]);
                intervalId += 1;
            }

            // 处理出口点上的定义情况
            for (const auto &p : pointsOnIndex[instOut[inst]])
            {
                auto reg = p.first;
                auto point = p.second;
                if (point->isDef() && (defOnce[reg] == nullptr || point->isTruePoint()))
                {
                    lastDef[reg] = instOut[inst];
                    preserved.erase(reg);
                }
            }
        }

        // 合并指令列表并返回
        std::vector<AsmInstBasic *> instructList;
        for (const auto &inst : instructionList.getList())
        {
            auto inIndex = instIn[inst];
            auto outIndex = instOut[inst];
            instructList.insert(instructList.end(), newInstList[inIndex].begin(), newInstList[inIndex].end());
            instructList.push_back(inst);
            instructList.insert(instructList.end(), newInstList[outIndex].begin(), newInstList[outIndex].end());
        }
        return instructList;
    }

    AsmInstBasicList GraphColoringAllocator::work(AsmInstBasicList instructionList)
    {
        std::vector<AsmOperandRegister *> intRegList;
        std::vector<AsmOperandRegister *> floatRegList;
        std::vector<AsmOperandRegister *> intPRegList;
        std::vector<AsmOperandRegister *> floatPRegList;

        // 清除之前的物理寄存器映射
        physicRegMap.clear();

        // 将寄存器按类型分配到相应的列表中
        for (AsmOperandRegister *reg : ltc->getRegKeySet())
        {
            if (dynamic_cast<AsmOperandRegisterInt *>(reg))
            {
                intRegList.push_back(reg);
            }
            else
            {
                floatRegList.push_back(reg);
            }
        }

        // 添加物理寄存器到相应的列表中
        for (int i = 0; i <= 31; ++i)
        {
            AsmOperandRegister *reg = AsmOperandRegisterInt::getPhysical(i);
            if (!Registers::CONSTANT_REGISTERS.count(reg))
            {
                intPRegList.push_back(reg);
            }
            reg = AsmOperandRegisterFloat::getPhysical(i);
            if (!Registers::CONSTANT_REGISTERS.count(reg))
            {
                floatPRegList.push_back(reg);
            }
        }

        // 更新：保留地址寄存器
        auto addressRegIter = std::find(intPRegList.begin(), intPRegList.end(), addrReg);
        if (addressRegIter != intPRegList.end())
        {
            intPRegList.erase(addressRegIter);
        }

        // 分配整数物理寄存器
        physicRegs = intPRegList;
        instructionList = allocatePhysicalRegisters(instructionList, intRegList);

        // 分配浮点物理寄存器
        physicRegs = floatPRegList;
        instructionList = allocatePhysicalRegisters(instructionList, floatRegList);

        // 替换物理寄存器
        instructionList.setList(replacePhysicRegisters(instructionList));

        return instructionList;
    }
}
