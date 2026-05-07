#ifndef RISCV64_REGALLOC_H
#define RISCV64_REGALLOC_H

#include "RISCv64LLIR.h"
#include "RISCv64ISel.h" // 包含 RISCv64ISel.h 以访问 ISel 和 Value 类型
#include <set>
#include <vector>
#include <map>
#include <stack>

extern int DEBUG;
extern int DEEPDEBUG;
extern int DEBUGLENGTH; // 用于限制调试输出的长度
extern int DEEPERDEBUG; // 用于更深层次的调试输出
extern int optLevel;

namespace sysy {

class RISCv64RegAlloc {
public:
    RISCv64RegAlloc(MachineFunction* mfunc);

    // 模块主入口
    bool run(std::shared_ptr<std::atomic<bool>> stop_flag);

private:
    // 类型定义，与Python版本对应
    using VRegSet = std::set<unsigned>;
    using InterferenceGraph = std::map<unsigned, VRegSet>;
    using VRegStack = std::vector<unsigned>; // 使用vector模拟栈，方便遍历
    using MoveList = std::map<unsigned, std::set<const MachineInstr*>>;
    using AliasMap = std::map<unsigned, unsigned>;
    using ColorMap = std::map<unsigned, PhysicalReg>;
    using VRegMoveSet = std::set<const MachineInstr*>;

    // --- 核心算法流程 ---
    void initialize();
    void build();
    void makeWorklist();
    void simplify();
    void coalesce();
    void freeze();
    void selectSpill();
    void assignColors();
    void rewriteProgram();
    bool doAllocation();
    void applyColoring();
    void precolorByCallingConvention();
    void protectCrossCallVRegs();

    // --- 辅助函数 ---
    void dumpState(const std::string &stage);
    void getInstrUseDef(const MachineInstr* instr, VRegSet& use, VRegSet& def);
    void getInstrUseDef_Liveness(const MachineInstr *instr, VRegSet &use, VRegSet &def);
    void addEdge(unsigned u, unsigned v);
    VRegSet adjacent(unsigned n);
    VRegMoveSet nodeMoves(unsigned n);
    bool moveRelated(unsigned n);
    void decrementDegree(unsigned m);
    void enableMoves(const VRegSet& nodes);
    unsigned getAlias(unsigned n);
    void addWorklist(unsigned u);
    bool briggsHeuristic(unsigned u, unsigned v);
    bool georgeHeuristic(unsigned u, unsigned v);
    void combine(unsigned u, unsigned v);
    void freezeMoves(unsigned u);
    void collectUsedCalleeSavedRegs();
    bool isFPVReg(unsigned vreg) const;
    std::string regToString(PhysicalReg reg);
    std::string regIdToString(unsigned id);

    // --- 活跃性分析 ---
    void analyzeLiveness();

    MachineFunction* MFunc;
    RISCv64ISel* ISel;

    // --- 算法数据结构 ---
    // 寄存器池
    std::vector<PhysicalReg> allocable_int_regs;
    std::vector<PhysicalReg> allocable_fp_regs;
    int K_int; // 整数寄存器数量
    int K_fp;  // 浮点寄存器数量

    // 节点集合
    VRegSet precolored; // 预着色的节点 (物理寄存器)
    VRegSet initial;    // 初始的、所有待处理的虚拟寄存器节点
    VRegSet simplifyWorklist;
    VRegSet freezeWorklist;
    VRegSet spillWorklist;
    VRegSet spilledNodes;
    VRegSet coalescedNodes;
    VRegSet coloredNodes;
    VRegStack selectStack;

    // Move指令相关
    std::set<const MachineInstr*> coalescedMoves;
    std::set<const MachineInstr*> constrainedMoves;
    std::set<const MachineInstr*> frozenMoves;
    std::set<const MachineInstr*> worklistMoves;
    std::set<const MachineInstr*> activeMoves;

    // 数据结构
    InterferenceGraph adjSet;
    std::map<unsigned, VRegSet> adjList; // 邻接表
    std::map<unsigned, int> degree;
    MoveList moveList;
    AliasMap alias;
    ColorMap color_map;

    // 活跃性分析结果
    std::map<const MachineInstr*, VRegSet> live_in_map;
    std::map<const MachineInstr*, VRegSet> live_out_map;
    
    // VReg -> Value* 和 VReg -> Type* 的映射
    const std::map<unsigned, Value*>& vreg_to_value_map;
    const std::map<unsigned, Type*>& vreg_type_map;
};

} // namespace sysy

#endif // RISCV64_REGALLOC_H