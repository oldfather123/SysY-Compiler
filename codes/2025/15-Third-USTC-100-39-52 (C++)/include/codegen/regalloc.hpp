#ifndef REGALLOCA_HPP
#define REGALLOCA_HPP
#include "Function.hpp"
#include "Value.hpp"
#include "liverange.hpp"
#include "logging.hpp"

#include <iostream>
#include <string>
#include <regex>

using namespace LRA;

namespace RA {

//一共32个寄存器，只使用其中8个
#define MAXR 32
#define ARG_MAX_R 8

struct ActiveCMP {
    bool operator()(LiveInterval const &lhs, LiveInterval const &rhs) const {
        std::regex pattern_arg("arg\\d+");
        bool is_lhs_arg = std::regex_match(lhs.second->get_name(), pattern_arg);
        bool is_rhs_arg = std::regex_match(rhs.second->get_name(), pattern_arg);
        if(is_lhs_arg && !is_rhs_arg)
            return true;
        if(!is_lhs_arg && is_rhs_arg)
            return false;
        if (lhs.first.j != rhs.first.j)
            return lhs.first.j < rhs.first.j;
        else if (lhs.first.i != rhs.first.i)
            return lhs.first.i < rhs.first.i;
        else
            return lhs.second < rhs.second;
    }
};

class RegAllocator {
  private:
    Function *cur_func;  // 当前正在处理的函数
    const bool FLOAT;  // 是否为浮点寄存器分配器
    const uint R;  // 实际使用的寄存器数量
    bool used[MAXR + 1]; // index range: 1 ~ R // 当前哪些寄存器正在被使用
    map<Value *, int> regmap;  // 分配结果：Value* -> 物理寄存器编号
    set<LiveInterval, ActiveCMP> active;  // 当前活跃变量集合
    //TODO:添加你需要的变量
    map<Value *, int> stack_slot;  // spill 变量 -> 栈偏移
    int stack_offset = 0;          // 当前栈帧偏移量（逐步递增）
    vector<LiveInterval> intervals; // 可选：缓存排序后的活跃区间
    int spill_count = 0;            // 用于调试

    //int get_arg_id(Argument *arg,bool is_float);
    void reset(Function * = nullptr);   //重置分配器状态（用于处理多个函数）
    void ReserveForArg(const LVITS &);   //为函数参数提前分配固定寄存器（通常是 x10 ~ x17）
    void ExpireOldIntervals(LiveInterval);   //从 active 中清除已不再活跃的变量（按照线性扫描算法）
    void SpillAtInterval(LiveInterval);   //如果所有寄存器都被占用，需要将某个 active 中的变量溢出到栈中 
    void only_SpillAtInterval(LiveInterval liveint); //直接放到栈上
    //添加函数
    int get_free_reg();  // 获取一个可用寄存器编号（若无返回 -1）
    void alloc_stack_slot(Value *v); // 给变量分配一个 stack_slot 并记录
    int get_stack_offset(Value *v);  // 获取变量的栈偏移
    bool is_spilled(Value *v);  //判断变量 v 是否被“溢出”到了栈上
    //int get_stack_size() const;
  public:
    RegAllocator(const uint R_, bool fl) : FLOAT(fl), R(R_), used{false} {
        LOG_DEBUG << "RegAllocator initialize: R=" << R << "\n";
        assert(R <= MAXR);
    }                                        //构造函数，接受最多可分配的寄存器数 R_ 和是否为浮点寄存器分配标志 fl
    RegAllocator() = delete;
    int get_stack_size() const;
    int get_arg_id(Argument *arg,bool is_float);
    //判断当前v是否需要分配寄存器，需要则返回false
    static bool no_reg_alloca(Value *v);         //判断某个 Value* 是否不需要寄存器分配，比如 ConstantInt、GlobalVar 等
    void LinearScan(const LVITS &, Function *);        //主接口：对函数中的变量按照线性扫描进行寄存器分配
    const map<Value *, int> &get() const { return regmap; }  //获取最终的 Value* -> 寄存器号 映射表（可以用于后续代码生成）
    void print(string (*regname)(int)) {
        for (auto [op, reg] : regmap)
            // LOG_DEBUG << op->get_name() << " ~ " << regname(reg) << "\n";
            std::cout << op->get_name() << " ~ " << regname(reg) << std::endl;
    }                          //输出当前的寄存器分配表，regname 是一个函数指针用于转换寄存器号为字符串，比如 "x10"、"f5" 等
    const map<Value *, int> &get_spill() const { return stack_slot; }
    void assign(Value* v, int reg_id) {
        regmap[v] = reg_id;  // reg_map 是内部的 map
    }
};
} // namespace RA
#endif
//TODO: 这只是一个样例，你可以自行对框架进行修改以符合你自己的心意
//TODO: 对框架不满可尽情修改