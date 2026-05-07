#pragma once

#include "AsmFunction.h"
#include "AsmStackAllocator.h"
#include "AsmOperandHeaders.h"
#include "AsmInstHeaders.h"
#include "AsmOperandHeaders.h"
#include "AsmOperandRegister.h"
#include "RegisterControl.h"
#include "basicblock.h"
#include "AsmBasicBlock.h"
#include "LifeTimeController.h"
#include "StackPool.h"
#include "loopforest.h"
#include "AsmBasicBlock.h"
#include <stack>

/*
寄存器分配：
build -> simplify -> coalesce -> freeze -> select -> spill ->
 /\         /\          |           |                    |
 |           |          |           |                    |
 |           |          |           |                    |
 |           |-----<----|           |                    |
 |           |-----------<----------|                    |
 ---<----------<--------------<---<---<---rewrite-<------|

*/

namespace Backend
{
    class GraphColoringAllocator : public RegisterControl
    {
    private:
        std::vector<AsmOperandRegister *> physicRegs;
        LifeTimeController *ltc;

        std::vector<LifeTimeInterval *> intervals;
        std::map<LifeTimeInterval *, LifeTimeInterval *> intervalValues;

        std::set<AsmOperandRegister *> uncoloredReg;
        std::set<AsmOperandRegister *> coalescentReg;
        std::set<AsmOperandRegister *> RegAcrossC;

        std::stack<AsmOperandRegister *> RegStack;

        std::map<AsmOperandRegister *, AsmOperandRegister *> physicRegMap;

        std::map<AsmOperandRegister *, int> spillCosts;
        AsmInstBasicList instList;
        std::map<AsmOperandRegister *, std::set<AsmOperandRegister *>> interferenceEdges;
        std::map<AsmOperandRegister *, std::set<AsmOperandRegister *>> coalescentEdges;
        AsmOperandRegisterInt *addrReg = AsmOperandRegisterInt::getPhysical(5); // ?

        int inf{0x3f3f3f3f};

    public:
        GraphColoringAllocator(AsmFunction *function, AsmStackAllocator *allocator); /* 构造函数 */
        void Cost(std::vector<AsmOperandRegister *> aors);                           /* 计算 spill 的代价 */
        void addInterfaceEdge(AsmOperandRegister *u, AsmOperandRegister *v);         /* 加干涉边 */
        void addCoalesceEdge(AsmOperandRegister *u, AsmOperandRegister *v);          /* 加接合边 */
        void removeEdge(AsmOperandRegister *u, AsmOperandRegister *v);               /* 删边 */
        LifeTimeInterval *getIntervalValue(LifeTimeInterval *interval);              /*  */
        void GraphCreater(bool freeze, std::vector<AsmOperandRegister *> &rs);
        void getNeedReturnAfterCall(); /* 保留call后还要用的寄存器 */
        bool Color();
        void colorToRegisterMap(std::vector<AsmOperandRegister *> &rs);

        bool BriggsCheck(AsmOperandRegister *u, AsmOperandRegister *v); // 干涉度是否小于物理寄存器的数量，是则 1
        bool GeorgeCheck(AsmOperandRegister *u, AsmOperandRegister *v); // 确定寄存器 v 的干涉寄存器是否完全包含在寄存器 u 的干涉寄存器中。
        void mergeEdges(AsmOperandRegister *u, AsmOperandRegister *v);

        bool CheckCoalesce(AsmOperandRegister *u, AsmOperandRegister *v); // 检查寄存器 u 和寄存器 v 是否可以合并
        void practiceCoalesce(AsmOperandRegister *u, AsmOperandRegister *v, std::vector<AsmOperandRegister *> &rs);
        bool coalesce(std::vector<AsmOperandRegister *> &rs);

        bool freeze();
        double costFunction(AsmOperandRegister *reg);
        void practiseSpill(std::map<AsmOperandRegister *, AsmOperandStackVariable *> spilledRegs, std::vector<AsmOperandRegister *> &registers);
        void workOnRegisterValue(AsmOperandRegister *tmpReg, AsmInstBasic *last, AsmInstBasic *next);
        AsmOperandRegister *getSpillDest();
        void spill(std::vector<AsmOperandRegister *> &rs);
        AsmInstBasicList allocatePhysicalRegisters(AsmInstBasicList instructionList, std::vector<AsmOperandRegister *> &registers);
        std::vector<AsmInstBasic *> replacePhysicRegisters(AsmInstBasicList instructionList);
        AsmInstBasicList work(AsmInstBasicList instructionList) override;
    };
}