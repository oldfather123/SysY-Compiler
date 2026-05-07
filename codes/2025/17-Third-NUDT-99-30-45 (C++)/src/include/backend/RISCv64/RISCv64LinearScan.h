#ifndef RISCV64_LINEARSCAN_H
#define RISCV64_LINEARSCAN_H

#include "RISCv64LLIR.h"
#include "RISCv64ISel.h"
#include <vector>
#include <map>
#include <set>
#include <algorithm>

namespace sysy {

// 前向声明
class MachineBasicBlock;
class MachineFunction;
class RISCv64ISel;

/**
 * @brief 表示一个虚拟寄存器的活跃区间。
 * 包含起始和结束指令编号。为了简化，我们不处理有“洞”的区间。
 */
struct LiveInterval {
    unsigned vreg = 0;
    int start = -1;
    int end = -1;
    bool crosses_call = false;
    
    LiveInterval(unsigned vreg) : vreg(vreg) {}

    // 用于排序，按起始点从小到大
    bool operator<(const LiveInterval& other) const {
        return start < other.start;
    }
};

class RISCv64LinearScan {
public:
    RISCv64LinearScan(MachineFunction* mfunc);
    bool run();

private:
    // --- 核心算法流程 ---
    void linearizeBlocks();
    void computeLiveIntervals();
    bool linearScan();
    void rewriteProgram();
    void applyAllocation();
    void spillAtInterval(LiveInterval* current);
    
    // --- 辅助函数 ---
    bool isFPVReg(unsigned vreg) const;
    void collectUsedCalleeSavedRegs();

    MachineFunction* MFunc;
    RISCv64ISel* ISel;

    // --- 线性扫描数据结构 ---
    std::vector<MachineBasicBlock*> linear_order_blocks;
    std::map<const MachineInstr*, int> instr_numbering;
    std::map<unsigned, LiveInterval> live_intervals;
    
    std::vector<LiveInterval*> unhandled;
    std::vector<LiveInterval*> active; // 活跃且已分配物理寄存器的区间
    
    std::set<unsigned> spilled_vregs; // 记录在本轮被决定溢出的vreg

    bool conservative_spill_mode = false;
    const PhysicalReg SPILL_TEMP_REG = PhysicalReg::T4;

    // --- 寄存器池和分配结果 ---
    std::vector<PhysicalReg> allocable_int_regs;
    std::vector<PhysicalReg> allocable_fp_regs;
    std::map<unsigned, PhysicalReg> vreg_to_preg_map;
    std::map<unsigned, PhysicalReg> abi_vreg_map;
    
    const std::map<unsigned, Type*>& vreg_type_map;
};

} // namespace sysy

#endif // RISCV64_LINEARSCAN_H