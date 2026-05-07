#ifndef RISCV64_SIMPLE_REGALLOC_H
#define RISCV64_SIMPLE_REGALLOC_H

#include "RISCv64LLIR.h"
#include "RISCv64ISel.h"
#include <set>
#include <vector>
#include <map>

// 外部调试级别控制变量的声明
extern int DEBUG;
extern int DEEPDEBUG;

namespace sysy {

class RISCv64AsmPrinter; // 前向声明

/**
 * @class RISCv64SimpleRegAlloc
 * @brief 一个简单的一次性图着色寄存器分配器。
 * * 该分配器遵循一个线性的、非迭代的流程：
 * 1. 活跃性分析
 * 2. 构建冲突图
 * 3. 贪心图着色
 * 4. 重写函数代码，插入溢出指令
 * * 它与新版后端流水线兼容，但保留了旧版分配器的核心逻辑。
 * 溢出处理使用硬编码的物理寄存器。
 */
class RISCv64SimpleRegAlloc {
public:
    RISCv64SimpleRegAlloc(MachineFunction* mfunc);

    /**
     * @brief 运行寄存器分配的主函数。
     */
    void run();

private:
    using LiveSet = std::set<unsigned>;
    using InterferenceGraph = std::map<unsigned, LiveSet>;

    // --- 分配流程的各个阶段 ---
    void unifyArgumentVRegs();
    void handleCallingConvention();
    void analyzeLiveness();
    void buildInterferenceGraph();
    void colorGraph();
    void rewriteFunction();

    // --- 辅助函数 ---

    /**
     * @brief 获取指令的Use/Def集合，包含物理寄存器，用于活跃性分析。
     * @param instr 机器指令。
     * @param use 输出参数，存储使用的寄存器ID。
     * @param def 输出参数，存储定义的寄存器ID。
     */
    void getInstrUseDef_Liveness(const MachineInstr* instr, LiveSet& use, LiveSet& def);

    /**
     * @brief 根据vreg的类型信息返回其大小和类型种类。
     * @param vreg 虚拟寄存器号。
     * @return 一个包含类型信息和大小（字节）的pair。
     */
    std::pair<Type::Kind, unsigned> getTypeAndSize(unsigned vreg);

    /**
     * @brief 打印调试用的活跃集信息。
     */
    void printLiveSet(const LiveSet& s, const std::string& name, std::ostream& os, const RISCv64AsmPrinter& printer);

    /**
     * @brief 将寄存器ID（虚拟或物理）转换为可读字符串。
     */
    std::string regIdToString(unsigned id, const RISCv64AsmPrinter& printer) const;

    // --- 成员变量 ---
    MachineFunction* MFunc;
    RISCv64ISel* ISel;

    // 可分配的寄存器池
    std::vector<PhysicalReg> allocable_int_regs;
    std::vector<PhysicalReg> allocable_fp_regs;

    // 硬编码的溢出专用物理寄存器
    const PhysicalReg INT_SPILL_REG = PhysicalReg::T2;   // 用于 32-bit int
    const PhysicalReg PTR_SPILL_REG = PhysicalReg::T3;   // 用于 64-bit pointer
    const PhysicalReg FP_SPILL_REG = PhysicalReg::F4;    // 用于 32-bit float (ft4)

    // 活跃性分析结果
    std::map<const MachineInstr*, LiveSet> live_in_map;
    std::map<const MachineInstr*, LiveSet> live_out_map;

    // 冲突图
    InterferenceGraph interference_graph;

    // 着色结果和溢出列表
    std::map<unsigned, PhysicalReg> color_map;
    std::set<unsigned> spilled_vregs;

    // 映射：将物理寄存器ID映射到它们在冲突图中的特殊虚拟ID
    std::map<PhysicalReg, unsigned> preg_to_vreg_id_map;
};

} // namespace sysy

#endif // RISCV64_SIMPLE_REGALLOC_H