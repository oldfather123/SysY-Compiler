#ifndef GEECEECEE_RV_REG_ALLOCATOR_HPP
#define GEECEECEE_RV_REG_ALLOCATOR_HPP

#include <list>
#include <map>
#include <stack>
#include <vector>

#include "rv_basic_block.hpp"
#include "rv_module.hpp"

namespace backend::riscv {

class RVRegAllocator {
public:
    // 主入口：对整个模块分配寄存器
    void allocate(RVModule &module);
    ~RVRegAllocator();

    // 关键数据结构
    using OperandSet = std::set<RVReg *>;
    using OperandMap = std::map<RVReg *, OperandSet>;

private:
    // 单个函数的分配主流程
    void allocate_function(RVFunction *func);

    // 初始化函数
    void initialize(RVFunction *func);

    // 生成保护移动指令
    void gen_protection_move(RVInstruction *head_inst,
                             const std::vector<RVInstruction *> &tail_nodes,
                             RVFunction *func);

    // 图构建和工作表初始化
    void add_edge(RVReg *u, RVReg *v);
    void build_graph(RVFunction *func);
    void make_worklist(RVFunction *func);

    // 主循环
    void main_loop();

    // 主循环核心操作
    void simplify();
    void coalesce();
    void freeze();
    void select_spill();

    // 着色
    void assign_color();

    // 溢出重写
    void rewrite_program(RVFunction *func);

    // 辅助函数声明
    void decrement_degree(RVReg *m);
    bool move_related(RVReg *n);
    std::set<RVInstruction *> node_moves(RVReg *n);
    OperandSet adjacent(RVReg *n);

    // 新增辅助函数声明
    RVReg *get_alias(RVReg *n);
    void add_worklist(RVReg *u);
    bool ok(RVReg *t, RVReg *r);
    bool conservative(const OperandSet &nodes);
    void combine(RVReg *u, RVReg *v);
    void freeze_moves(RVReg *u);
    double get_prio_for_spill(RVReg *r);
    std::list<int> init_ok_colors() const;
    void enable_moves(const OperandSet &nodes);
    std::vector<RVPhyReg *> get_call_defs() const;

    OperandSet simplify_worklist;
    OperandSet freeze_worklist;
    OperandSet spill_worklist;
    OperandSet spilled_nodes;
    OperandSet coalesced_nodes;
    OperandSet colored_nodes;
    std::stack<RVReg *> select_stack;

    std::set<RVInstruction *> coalesced_moves;
    std::set<RVInstruction *> constrained_moves;
    std::set<RVInstruction *> frozen_moves;
    std::set<RVInstruction *> worklist_moves;
    std::set<RVInstruction *> active_moves;
    std::set<std::pair<RVReg *, RVReg *>> adj_set;
    OperandMap adj_list;
    std::map<RVOperand *, int> degree;
    std::map<RVOperand *, int> color;

    // 当前处理的寄存器类型
    enum class Phase { NONE, FLOAT, INT };
    Phase phase = Phase::NONE;

    // 常量定义
    static constexpr int K = 27;  // 可用寄存器数量（跳过zero,ra,sp,gp,tp）

    // 修正 move_list 类型定义
    std::map<RVReg *, std::set<RVInstruction *>> move_list;

    // 别名映射
    std::map<RVReg *, RVReg *> alias;

    // 活跃信息映射
    std::map<RVBasicBlock *, BlockLiveInfo *> live_info_map;

    // 当前处理的函数
    RVFunction *current_function = nullptr;

    // 寄存器溢出代价计算相关
    std::map<RVReg *, int> reg_spill_cost_map;

    // 存储因为溢出而生成的内存指令
    std::set<RVInstruction *> spilled_memory_instructions;

    // 计算寄存器溢出代价
    void build_spill_cost(const OperandSet &regs);

    // 获取按溢出代价排序的溢出序列
    std::vector<RVReg *> get_spill_array();

    // 获取溢出代价最小的寄存器
    RVReg *get_min_cost_spill_reg();

    // 计算单个寄存器的溢出代价
    void calculate_reg_cost(RVReg *reg);
};

}  // namespace backend::riscv

#endif
