#ifndef __LOOP_INIT_H__
#define __LOOP_INIT_H__

#include <map>
#include "opt.hpp"
#include "Loop.hpp"

// 循环优化前置准备：循环检测、嵌套设置、循环简化、基本归纳变量标准化
class LoopInitializer: public Optimization {
public:
    explicit LoopInitializer(Module* m): Optimization(m) {}
    void execute() override;
    unordered_map<Function*, LoopVecPtr>& get_loops() { return loops; } // 获取循环

private:
    unordered_map<Function*, LoopVecPtr> loops; // 函数->循环，循环信息唯一储存处，其余 Pass 使用其引用

    //* ======================================================================================
    //* Loop Detection: 检测自然循环，构建循环嵌套关系
    // 获取函数的回边，注意合并相同循环头的回边
    map<BasicBlock*, vector<Edge*>> get_back_edges(Function* func);
    // 根据回边获取循环体
    vector<BasicBlock*> get_loop_body(Edge* back_edge);
    // 构建循环树，设置嵌套关系
    void build_loop_tree(Function* func);
    // 检测函数中的循环
    void detect_loops(Function* func);
    // 按照从内到外的顺序排列循环，确保内层循环先处理
    void order_loops(Function* func);
    //* ======================================================================================

    //* ======================================================================================
    //* Loop Simplify: 简化循环，确保每个循环有唯一入口 header 和唯一出口 latch，且存在 pre-header
    // 设置循环前置块
    void set_loop_pre_header(LoopPtr loop);
    // 设置循环出口
    void set_loop_latch(LoopPtr loop);
    //* ======================================================================================
    
    //* ======================================================================================
    //* Loop Induction Variable Analysis Preparation: 循环归纳变量分析前置准备
    // 查找基本归纳变量
    void set_basic_indu_vars(LoopPtr loop);
    // 查找基本归纳变量的同族归纳变量：biv -> divs
    void set_derived_indu_vars(LoopPtr loop);
    //* ======================================================================================
};

#endif // __LOOP_INIT_H__