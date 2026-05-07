#ifndef LINEAR_SCAN_H
#define LINEAR_SCAN_H
#include"../alloc_help.h"
#include<queue>
#include"../../inst_process/inst_select/inst_select.h"
#include "../../inst_process/inst_print/inst_print.h"

bool IntervalsPrioCmp(LiveInterval a, LiveInterval b);
//合并FastLinearScan与SpillCodeGen
class FastLinearScan {
private:
    MachineBlock *cur_block;//方便spill_code_gen
    MachineUnit *unit;
    MachineFunction* current_func;
    std::priority_queue<LiveInterval, std::vector<LiveInterval>, decltype(IntervalsPrioCmp) *> unalloc_queue;
    // 计算溢出权重
    double CalculateSpillWeight(LiveInterval);
    void UpdateIntervalsInCurrentFunc();
    void ComputeLoopDepth(MachineCFG* C);
    std::queue<MachineFunction *> not_allocated_funcs;

    std::map<Register, LiveInterval> intervals;
    PhysicalRegistersAllocTools *phy_regs_tools;
    std::map<MachineFunction *, std::map<Register, AllocResult>> alloc_result;

    std::map<int, InstructionNumberEntry> numbertoins;
    // 将分配结果写入alloc_result
    // SpillCodeGen/VregRewrite使用alloc_result的信息完成后续操作
    void AllocPhyReg(MachineFunction *mfun, Register vreg, int phyreg) {
        Assert(vreg.is_virtual);
        alloc_result[mfun][vreg].in_mem = false;//如果分配了物理寄存器，那么in_mem为false,即不在内存中
        alloc_result[mfun][vreg].phy_reg_no = phyreg;
    }
    void AllocStack(MachineFunction *mfun, Register vreg, int offset) {
        Assert(vreg.is_virtual);
        alloc_result[mfun][vreg].in_mem = true;
        alloc_result[mfun][vreg].stack_offset = offset;
    }
    void swapAllocResult(MachineFunction *mfun, Register v1, Register v2) {
        Assert(v1.is_virtual && v2.is_virtual);
        AllocResult tmp = alloc_result[mfun][v1];
        alloc_result[mfun][v1] = alloc_result[mfun][v2];
        alloc_result[mfun][v2] = tmp;
    }
    int getAllocResultInReg(MachineFunction *mfun, Register vreg) {
        Assert(alloc_result[mfun][vreg].in_mem == false);
        Assert(vreg.is_virtual);
        return alloc_result[mfun][vreg].phy_reg_no;
    }
    int getAllocResultInMem(MachineFunction *mfun, Register vreg) {
        Assert(alloc_result[mfun][vreg].in_mem == true);
        Assert(vreg.is_virtual);
        return alloc_result[mfun][vreg].stack_offset;
    }


protected:
    // 寄存器分配, 返回是否溢出
    bool DoAllocInCurrentFunc();

public:
    FastLinearScan(MachineUnit *unit, PhysicalRegistersAllocTools *phy)
        : unit(unit), phy_regs_tools(phy), unalloc_queue(IntervalsPrioCmp) {}
    void Execute();
    //相当于spiller->ExecuteInFunc的功能
    // 在当前函数中执行溢出代码生成
    void SpillCodeGen(MachineFunction *function, std::map<Register, AllocResult> *alloc_result);
    // 生成从栈中读取溢出寄存器的指令
    Register GenerateReadCode(std::list<MachineBaseInstruction *>::iterator &it, int raw_stk_offset,
        MachineDataType type);
    // 生成将溢出寄存器写入栈的指令
    Register GenerateWriteCode(std::list<MachineBaseInstruction *>::iterator &it, int raw_stk_offset,
        MachineDataType type);
    //相当于VirtualRegisterRewrite
    void VirtualRegisterRewrite();
    void RewriteInFunc();
	void ShowAllAllocResult();
};


class InstructionNumber {
private:
    MachineUnit *unit;
    std::map<int, InstructionNumberEntry> &numbertoins;

public:
    // 给指令编号，并记录指令编号与指令的映射
    InstructionNumber(MachineUnit *unit, std::map<int, InstructionNumberEntry> &number2ins)
        : unit(unit), numbertoins(number2ins) {}
    void Execute();
    void ExecuteInFunc(MachineFunction *func);
};

class UnionFind {
private:
    std::map<Register, Register> parent;

public:
    // 初始化虚拟寄存器集合
    void initialize(const std::map<Register, LiveInterval> intervals) {
        for (const auto& [reg, interval] : intervals) {
            if (reg.is_virtual) parent[reg] = reg;
        }
    }

    // 带路径压缩的查找根节点
    Register findRoot(Register vreg) {
        if (parent[vreg] == vreg) 
            return vreg;
        
        // 路径压缩
        return parent[vreg] = findRoot(parent[vreg]);
    }

    // 合并两个集合
    void unionSets(Register a, Register b) {
        Register rootA = findRoot(a);
        Register rootB = findRoot(b);
        if (!(rootA == rootB)) {
            parent[rootB] = rootA;
        }
    }
};
//寄存器分配类，记录全局变量及全局函数、维护物理寄存器分配、代码溢出等信息
//关键函数是，更新当前函数各个寄存器的活跃区间（维护intervals）
// class RegisterAllocation : public MachinePass {
// private:
//     void UpdateIntervalsInCurrentFunc();
//     std::queue<MachineFunction *> not_allocated_funcs;
//     SpillCodeGen *spiller;

// protected:
//     std::map<int, InstructionNumberEntry> numbertoins;
//     // 将分配结果写入alloc_result
//     // SpillCodeGen/VregRewrite使用alloc_result的信息完成后续操作
//     void AllocPhyReg(MachineFunction *mfun, Register vreg, int phyreg) {
//         Assert(vreg.is_virtual);
//         alloc_result[mfun][vreg].in_mem = false;//如果分配了物理寄存器，那么in_mem为false,即不在内存中
//         alloc_result[mfun][vreg].phy_reg_no = phyreg;
//     }
//     void AllocStack(MachineFunction *mfun, Register vreg, int offset) {
//         Assert(vreg.is_virtual);
//         alloc_result[mfun][vreg].in_mem = true;
//         alloc_result[mfun][vreg].stack_offset = offset;
//     }
//     void swapAllocResult(MachineFunction *mfun, Register v1, Register v2) {
//         Assert(v1.is_virtual && v2.is_virtual);
//         AllocResult tmp = alloc_result[mfun][v1];
//         alloc_result[mfun][v1] = alloc_result[mfun][v2];
//         alloc_result[mfun][v2] = tmp;
//     }
//     int getAllocResultInReg(MachineFunction *mfun, Register vreg) {
//         Assert(alloc_result[mfun][vreg].in_mem == false);
//         Assert(vreg.is_virtual);
//         return alloc_result[mfun][vreg].phy_reg_no;
//     }
//     int getAllocResultInMem(MachineFunction *mfun, Register vreg) {
//         Assert(alloc_result[mfun][vreg].in_mem == true);
//         Assert(vreg.is_virtual);
//         return alloc_result[mfun][vreg].stack_offset;
//     }

// protected:
//     std::map<Register, LiveInterval> intervals;
//     PhysicalRegistersAllocTools *phy_regs_tools;
    
//     // 在当前函数中完成寄存器分配
//     virtual bool DoAllocInCurrentFunc() = 0;
//     std::map<MachineFunction *, std::map<Register, AllocResult>> alloc_result;

// public:
//     RegisterAllocation(MachineUnit *unit, PhysicalRegistersAllocTools *phy, SpillCodeGen *spiller)
//         : MachinePass(unit), phy_regs_tools(phy), spiller(spiller) {}
//     // 对所有函数进行寄存器分配
//     void Execute();
// };

// bool IntervalsPrioCmp(LiveInterval a, LiveInterval b);
// class FastLinearScan : public RegisterAllocation {
// private:
//     std::priority_queue<LiveInterval, std::vector<LiveInterval>, decltype(IntervalsPrioCmp) *> unalloc_queue;
//     // 计算溢出权重
//     double CalculateSpillWeight(LiveInterval);

// protected:
//     // 寄存器分配, 返回是否溢出
//     bool DoAllocInCurrentFunc();

// public:
//     FastLinearScan(MachineUnit *unit, PhysicalRegistersAllocTools *phy, SpillCodeGen *spiller);
// };
//根据分配结果，为虚拟寄存器指定物理寄存器
// class VirtualRegisterRewrite : public MachinePass {
// private:
//     const std::map<MachineFunction *, std::map<Register, AllocResult>> &alloc_result;

// public:
//     // 根据分配结果重写指令中的虚拟寄存器
//     VirtualRegisterRewrite(MachineUnit *unit,
//                             const std::map<MachineFunction *, std::map<Register, AllocResult>> &alloc_result)
//         : MachinePass(unit), alloc_result(alloc_result) {}
//     void Execute();
//     void ExecuteInFunc();
// };
// 指令编号PASS
// 记录单个函数中，指令编号到指令的映射
// 尽管Execute可遍历全局中所有函数，对其进行编号，可是numbertoins只存放单个函数的映射
// class InstructionNumber : public MachinePass {
// private:
//     std::map<int, InstructionNumberEntry> &numbertoins;

// public:
//     // 给指令编号，并记录指令编号与指令的映射
//     InstructionNumber(MachineUnit *unit, std::map<int, InstructionNumberEntry> &number2ins)
//         : MachinePass(unit), numbertoins(number2ins) {}
//     void Execute();
//     void ExecuteInFunc(MachineFunction *func);
// };

// //PHI指令销毁PASS
// class MachinePhiDestruction : public MachinePass {
// private:
//     void PhiDestructionInCurrentFunction();

// public:
//     void Execute();
//     MachinePhiDestruction(MachineUnit *unit) : MachinePass(unit) {}
// };
// class SpillCodeGen {
// private:
//     std::map<Register, AllocResult> *alloc_result;
//     // 生成从栈中读取溢出寄存器的指令
//     virtual Register GenerateReadCode(std::list<MachineBaseInstruction *>::iterator &it, int raw_stk_offset,
//                                         MachineDataType type) = 0;
//     // 生成将溢出寄存器写入栈的指令
//     virtual Register GenerateWriteCode(std::list<MachineBaseInstruction *>::iterator &it, int raw_stk_offset,
//                                         MachineDataType type) = 0;

// protected:
//     MachineFunction *function;
//     MachineBlock *cur_block;

// public:
//     // 在当前函数中执行溢出代码生成
//     void ExecuteInFunc(MachineFunction *function, std::map<Register, AllocResult> *alloc_result);
// };
// class RiscV64Spiller : public SpillCodeGen {
// private:
//     // 生成从栈中读取溢出寄存器的指令
//     Register GenerateReadCode(std::list<MachineBaseInstruction *>::iterator &it, int raw_stk_offset,
//                                 MachineDataType type);
//     // 生成将溢出寄存器写入栈的指令
//     Register GenerateWriteCode(std::list<MachineBaseInstruction *>::iterator &it, int raw_stk_offset,
//                                 MachineDataType type);
// };
// class VirtualRegisterRewrite  {
// private:
//     MachineUnit *unit;
//     MachineFunction *current_func;
//     const std::map<MachineFunction *, std::map<Register, AllocResult>> &alloc_result;

// public:
//     // 根据分配结果重写指令中的虚拟寄存器
//     VirtualRegisterRewrite(MachineUnit *unit,
//                             const std::map<MachineFunction *, std::map<Register, AllocResult>> &alloc_result)
//         : unit(unit), alloc_result(alloc_result) {}
//     void Execute();
//     void ExecuteInFunc();
// };
#endif