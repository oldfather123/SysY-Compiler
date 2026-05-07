#ifndef __LOOP_H__
#define __LOOP_H__

#include <memory>
#include <unordered_map>
#include "opt.hpp"

struct Loop;
struct IVExp;

using Edge = pair<BasicBlock*, BasicBlock*>; // 控制流图的边
using LoopPtr = shared_ptr<Loop>;
using LoopVecPtr = shared_ptr<vector<LoopPtr>>;

// 循环结构
struct Loop {
    BasicBlock* header;         // 循环头，唯一入口
    BasicBlock* pre_header;     // 循环前置块，确保循环有唯一入口
    BasicBlock* latch;          // 循环唯一回边源
    vector<Edge*> back_edges;   // 回边，指向循环头的边
    vector<BasicBlock*> body;   // 循环体
    LoopPtr parent;             // 嵌套循环：外层循环
    LoopVecPtr children;        // 嵌套循环：内层循环
    unordered_map<Instruction*, bool> is_invar;        // 指令是否为循环不变量
    unordered_map<Value*, vector<IVExp*>> biv_to_divs; // 基本归纳变量到同族归纳变量的映射

    Loop(BasicBlock* header, vector<Edge*> back_edges, vector<BasicBlock*> body)
        : header(header), pre_header(nullptr), latch(nullptr), back_edges(back_edges), body(body), parent(nullptr), children(make_shared<vector<LoopPtr>>()) {}

    // 基本块是否一定执行
    bool is_always_executed(BasicBlock* target) const;
    // 当前 value 是否是循环不变量
    bool is_invariant(Value* val);
};

// 归纳变量线性表达式：i = i + c, j = k * i + b
struct IVExp {
    Instruction* inst;      // 表达式部分对应的指令
    Value* base;            // 基本归纳变量
    int k, b;               // 计算式参数，针对 biv，k = 1，b = 步长
    IVExp* pre_exp;         // 前置计算式，如果是 Phi 指令的话则为 nullptr
    vector<IVExp*> suc_exp; // 后置计算式，如果是终端指令的话则为 nullptr

    IVExp(Instruction* inst, Value* base, int k, int b, IVExp* pre_exp): inst(inst), base(base), k(k), b(b), pre_exp(pre_exp), suc_exp({}) {
        if(this->pre_exp) {
            this->pre_exp->suc_exp.push_back(this); // 设置后置计算式
        }
    }
    std::string print() const;
};

/************************************************  辅助函数  ************************************************/
void show_loop(const LoopPtr& loop); // 打印循环信息
void show_loops(const LoopVecPtr& loops); // 打印所有循环信息
void show_biv_to_divs(const unordered_map<Value*, vector<IVExp*>>& biv_to_divs); // 打印基本归纳变量到同族归纳变量的映射

#endif // __LOOP_H__