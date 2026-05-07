/*
LifeTimeController: 核心管理类，负责整体生命周期分析和管理，包括初始化、构建生命周期区间、合并生命周期点、维护指令ID等。
(1) 生命周期分析：通过方法如 getAllRegLifeTime，对所有指令进行遍历，构建寄存器的生命周期区间和使用点。
(2) 管理和优化：提供合并、插入和删除生命周期点的方法，帮助在寄存器分配过程中进行优化。
(3) 维护指令信息：维护指令ID和相关映射，确保生命周期分析的准确性和一致性。
(4) 构建基本块：通过 buildBlocks 方法，将指令分组为基本块，进行局部优化和寄存器分配。
(5) 初始化和标记：LifeTimeIndex 和 LifeTimePoint 标记指令的位置及寄存器的使用和定义点。
(6) 构建和比较区间：LifeTimeInterval 构建寄存器的生命周期区间，并提供比较和排序功能。
(7) 管理和优化：LifeTimeController 负责整体管理，包括初始化、分析、合并和优化生命周期信息。
(8) 寄存器分配：通过准确的生命周期分析，帮助编译器在寄存器分配过程中进行冲突检测、寄存器重用和优化，最终生成高效的目标代码。
*/

#pragma once

#include "AsmBasicBlock.h"
#include "AsmFunction.h"
#include "AsmInstBasic.h"
#include "AsmInstJump.h"
#include "AsmInstLabel.h"
#include "AsmOperandRegister.h"
#include "LifeTimeIndex.h"
#include "LifeTimeInterval.h"
#include "LifeTimePoint.h"
#include "AsmInstructions.h"
#include "Registers.h"
#include <utility>
#include <vector>
#include <set>
#include <iterator>
#include <algorithm>

namespace Backend
{
    class Block // 基本块
    {
    public:
        std::string blockName;               // 基本块名
        int l, r, startBranch;               // 基本块起始位置的索引，基本块结束位置的索引，分支指令的位置索引(默认为-1)
        std::set<AsmOperandRegister *> in;   // 进入基本块时活跃的寄存器集合
        std::set<AsmOperandRegister *> out;  // 离开基本块时活跃的寄存器集合
        std::set<AsmOperandRegister *> def;  // 基本块中定义的寄存器集合
        std::set<std::string> nextBlockName; // 后继基本块名集合

        Block(AsmInstLabel *label) // 用label初始化一个基本块
        {
            blockName = label->getLabelName();
            startBranch = -1;
        }
    };

    class LifeTimeController
    {
    private:
        AsmFunction *function;                                                                               // 函数
        std::unordered_map<AsmOperandRegister *, std::pair<LifeTimeIndex *, LifeTimeIndex *>> lifeTimeRange; // 生命周期范围
        std::unordered_map<AsmOperandRegister *, std::vector<LifeTimePoint *>> lifeTimePoints;               // 生命周期间隔
        std::unordered_map<AsmOperandRegister *, std::vector<LifeTimeInterval *>> lifeTimeIntervals;         // 生命周期点【记录寄存器在程序执行过程中的所有引用点（定义点和使用点）】
        std::unordered_map<AsmInstBasic *, int> instIDMap;                                                   // 指令对应的编号（指令到编号的映射的列表）
        AsmInstBasicList idSourceInst;                                                                       // 编号对应的指令（指令列表）

        std::vector<Block *> blocks;
        std::unordered_map<std::string, Block *> blockMap;

        void init();
        std::set<AsmOperandRegister *> difference(std::set<AsmOperandRegister *> a, std::set<AsmOperandRegister *> b);
        void buildBlocks(AsmInstBasicList instructionList);
        void iterateActiveReg();
        void buildLifeTimePoints(AsmInstBasicList instructionList);
        void buildLifeTimeMessage(AsmInstBasicList instructionList);

    public:
        LifeTimeController(AsmFunction *function) : function(function) {}

        AsmOperandRegister *getVReg(int index);

        void buildInstID(AsmInstBasicList instructionList);

        int getInstID(AsmInstBasic *inst);

        bool containsInst(AsmInstBasic *inst) const;

        void replaceInst(AsmInstBasic *replacedInst, AsmInstBasic *newInst);

        std::set<int> getVRegKeySet();

        std::set<AsmOperandRegister *> getRegKeySet();

        std::pair<LifeTimeIndex *, LifeTimeIndex *> getLifeTimeRange(int index);

        std::pair<LifeTimeIndex *, LifeTimeIndex *> getLifeTimeRange(AsmOperandRegister *reg);

        void insertLifeTimePoint(AsmOperandRegister *reg, LifeTimePoint *p);

        void removeLifeTimePoint(AsmOperandRegister *reg, LifeTimePoint *p);

        void removeReg(AsmOperandRegister *reg);

        void mergePoints(AsmOperandRegister *x, AsmOperandRegister *y);

        void mergePoints(int x, int y);

        bool constructInterval(AsmOperandRegister *x);

        std::vector<LifeTimeInterval *> getInterval(int x);

        std::vector<LifeTimeInterval *> getInterval(AsmOperandRegister *reg);

        std::vector<LifeTimePoint *> getPoints(int x);

        std::vector<LifeTimePoint *> getPoints(AsmOperandRegister *reg);

        void getAllRegLifeTime(AsmInstBasicList inst);

        void rebuildLifeTimeInterval(AsmInstBasicList instructionList);

        bool setInsert(std::set<AsmOperandRegister *> &dst, std::set<AsmOperandRegister *> src);
    };
}