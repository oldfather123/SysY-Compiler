#pragma once
#include "../ir/llvm_ir.h"
#include "../ir/live_interval.h"
#include "../ir/live_analysis.h"
#include "linear_scan_alloc.h"
#include "riscv.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stack>
#include <set>
#include <algorithm>

namespace llvm_ir {

// 干扰图节点
struct InterferenceNode {
    Value* vreg;                                      // 对应的虚拟寄存器
    std::unordered_set<InterferenceNode*> neighbors;  // 邻接节点（冲突的虚拟寄存器）
    std::unordered_set<InterferenceNode*> move_list;  // move相关的节点
    int degree;                                       // 当前度数
    bool is_spilled;                                  // 是否被溢出
    bool is_colored;                                  // 是否已着色
    bool is_precolored;                              // 是否预着色（如函数参数）
    riscv::reg color;                                // 分配的物理寄存器
    double spill_cost;                               // 溢出代价
    
    InterferenceNode(Value* v) : vreg(v), degree(0), is_spilled(false), 
                                is_colored(false), is_precolored(false), 
                                color(riscv::x0), spill_cost(0.0) {}
};

// Move指令表示
struct MoveInstr {
    InterferenceNode* src;
    InterferenceNode* dst;
    bool is_coalesced;
    bool is_worklist_move;  // 是否在worklist中
    
    MoveInstr(InterferenceNode* s, InterferenceNode* d) 
        : src(s), dst(d), is_coalesced(false), is_worklist_move(false) {}
};

// 图着色寄存器分配器
class GraphColoringAllocator {
private:
    Function* func;
    LiveInfo live_info;
    LiveIntervalMap intervals;
    
    // 干扰图
    std::unordered_map<Value*, InterferenceNode*> nodes;
    std::vector<std::unique_ptr<InterferenceNode>> all_nodes;
    std::vector<MoveInstr> move_instrs;
    
    // 工作列表
    std::vector<InterferenceNode*> simplify_worklist;
    std::vector<InterferenceNode*> freeze_worklist;
    std::vector<InterferenceNode*> spill_worklist;
    std::vector<InterferenceNode*> spilled_nodes;
    std::vector<InterferenceNode*> colored_nodes;
    std::vector<InterferenceNode*> coalesced_nodes;
    
    // Move相关的工作列表
    std::vector<MoveInstr*> worklist_moves;
    std::vector<MoveInstr*> active_moves;
    std::vector<MoveInstr*> frozen_moves;
    std::vector<MoveInstr*> coalesced_moves;
    std::vector<MoveInstr*> constrained_moves;
    
    // 选择栈
    std::stack<InterferenceNode*> select_stack;
    
    // 可用寄存器
    std::vector<riscv::reg> int_regs;
    std::vector<riscv::reg> float_regs;
    int K_int, K_float;  // 可用寄存器数量
    
    // 别名映射（用于合并）
    std::unordered_map<InterferenceNode*, InterferenceNode*> alias;
    
public:
    GraphColoringAllocator(Function* f);
    
    // 主要分配函数
    LinearScanResult allocate();
    
private:
    // 初始化
    void initialize();
    void initializeRegisters();
    
    // 构建干扰图
    void buildInterferenceGraph();
    void addEdge(InterferenceNode* u, InterferenceNode* v);
    bool interferes(Value* v1, Value* v2);
    
    // 检测move指令
    void detectMoveInstructions();
    bool isMoveInstruction(Instruction* inst);
    
    // 计算溢出代价
    void calculateSpillCosts();
    double calculateNodeSpillCost(InterferenceNode* node);
    
    // 工作列表管理
    void makeWorklist();
    void addToWorklist(InterferenceNode* node);
    
    // 图着色的各个阶段
    void simplify();
    void coalesce();
    void freeze();
    void selectSpill();
    void select();
    
    // 合并相关的辅助函数
    bool canCoalesce(InterferenceNode* u, InterferenceNode* v);
    void combine(InterferenceNode* u, InterferenceNode* v);
    InterferenceNode* getAlias(InterferenceNode* node);
    bool conservative(InterferenceNode* u, InterferenceNode* v);
    bool ok(InterferenceNode* t, InterferenceNode* r);
    
    // Move相关的辅助函数
    std::vector<MoveInstr*> nodeMoves(InterferenceNode* node);
    bool moveRelated(InterferenceNode* node);
    void enableMoves(const std::vector<InterferenceNode*>& nodes);
    void enableMoves(InterferenceNode* node);
    
    // 度数管理
    void decrementDegree(InterferenceNode* node);
    
    // 辅助函数
    bool isFloatType(Value* vreg);
    int getK(Value* vreg);
    std::vector<riscv::reg>& getAvailableRegs(Value* vreg);
    bool adjacent(InterferenceNode* n, InterferenceNode* m);
    std::vector<InterferenceNode*> getAdjList(InterferenceNode* node);
    
    // 着色相关
    riscv::reg selectColor(InterferenceNode* node);
    std::set<riscv::reg> getUsedColors(InterferenceNode* node);
    
    // 结果生成
    LinearScanResult generateResult();
    
    // 调试输出
    void printInterferenceGraph();
    void printWorklistStats();
};

// 主要接口函数，保持与线性扫描算法一致的接口
LinearScanResult graph_coloring_allocate(Function* func);

} // namespace llvm_ir
