#ifndef RISCV64_BASICBLOCKALLOC_H
#define RISCV64_BASICBLOCKALLOC_H

#include "RISCv64LLIR.h"
#include "RISCv64ISel.h"
#include <set>
#include <map>
#include <vector>

namespace sysy {

/**
 * @class RISCv64BasicBlockAlloc
 * @brief 一个有状态的、基本块级的贪心寄存器分配器。
 *
 * 该分配器作为简单但可靠的实现，它逐个处理基本块，并在块内尽可能地
 * 将虚拟寄存器的值保留在物理寄存器中，以减少不必要的内存访问。
 */
class RISCv64BasicBlockAlloc {
public:
    RISCv64BasicBlockAlloc(MachineFunction* mfunc);
    void run();

private:
    void computeLiveness();
    void processBasicBlock(MachineBasicBlock* mbb);
    void assignStackSlotsForAllVRegs();
    
    // 核心分配函数
    PhysicalReg ensureInReg(unsigned vreg, std::vector<std::unique_ptr<MachineInstr>>& new_instrs);
    PhysicalReg allocReg(unsigned vreg, std::vector<std::unique_ptr<MachineInstr>>& new_instrs);
    PhysicalReg findFreeReg(bool is_fp);
    PhysicalReg spillReg(bool is_fp, std::vector<std::unique_ptr<MachineInstr>>& new_instrs);

    // 状态跟踪（每个基本块开始时都会重置）
    std::map<unsigned, PhysicalReg> vreg_to_preg; // 当前vreg到物理寄存器的映射
    std::map<PhysicalReg, unsigned> preg_to_vreg; // 反向映射
    std::set<PhysicalReg> dirty_pregs;      // 被修改过、需要写回的物理寄存器

    // 分配器全局信息
    MachineFunction* MFunc;
    RISCv64ISel* ISel;
    std::map<unsigned, PhysicalReg> abi_vreg_map; // 函数参数的ABI寄存器映射

    // 寄存器池和循环索引
    std::vector<PhysicalReg> int_temps;
    std::vector<PhysicalReg> fp_temps;
    int int_temp_idx = 0;
    int fp_temp_idx = 0;

    // 辅助函数
    PhysicalReg getNextIntTemp();
    PhysicalReg getNextFpTemp();

    // 活性分析结果
    std::map<const MachineBasicBlock*, std::set<unsigned>> live_out;
};

} // namespace sysy

#endif // RISCV64_BASICBLOCKALLOC_H