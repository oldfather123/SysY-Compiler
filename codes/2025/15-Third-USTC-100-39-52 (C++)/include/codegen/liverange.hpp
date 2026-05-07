#ifndef LIVERANGE_HPP
#define LIVERANGE_HPP

#include "Function.hpp"
#include "Module.hpp"
#include "Value.hpp"

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <regex>
#include <unordered_map>
#include <unordered_set>
using std::map;
using std::pair;
using std::set;
using std::string;
using std::to_string;
using std::vector;

#define UNINITIAL -1

#define __LRA_PRINT__

namespace LRA {

struct Interval {
    Interval(int a = UNINITIAL, int b = UNINITIAL , int c = UNINITIAL) : i(a), j(b), k(c) {}
    int i; // 0 means uninitialized
    int j;
    int k;
};

using LiveSet = set<Value *>; //某程序点处活跃的变量集合。
using PhiMap = map<BasicBlock*, std::vector<std::tuple<Value*, Value*, BasicBlock*>>>; //表示每个基本块中的 φ 语句（实际是copy-statement，SSA φ 函数的非 SSA 表达），在活跃变量分析中需要特殊处理。
using LiveInterval = pair<Interval, Value *>;  //表示某个变量的活跃区间和对应变量指针；
//对活跃变量进行排序，此处采用按活跃区间的起始点进行排序
struct LiveIntervalCMP {
    bool operator()(const LiveInterval &lhs, const LiveInterval &rhs) const {
        // std::regex pattern_arg("arg(\\d+)");
        // std::smatch lhs_match, rhs_match;
        // std::string lhs_name = lhs.second->get_name();
        // std::string rhs_name = rhs.second->get_name();
        // bool is_lhs_arg = std::regex_match(lhs_name, lhs_match, pattern_arg);
        // bool is_rhs_arg = std::regex_match(rhs_name, rhs_match, pattern_arg);
        // //printf("%d and %d\n",lhs.first.k,rhs.first.k);
        // if (is_lhs_arg && is_rhs_arg) {
        //     int lhs_idx = std::stoi(lhs_match[1].str());
        //     //printf("%d\n",lhs_idx);
        //     int rhs_idx = std::stoi(rhs_match[1].str());
        //     return lhs_idx < rhs_idx;
        // }
        // if (is_lhs_arg && !is_rhs_arg)
        //     return true;
        // if (!is_lhs_arg && is_rhs_arg)
        //     return false;
        // // 起始点优先
        if (lhs.first.i != rhs.first.i)
            return lhs.first.i < rhs.first.i;
        // 起点相同，结束点小的排前面
        if (lhs.first.j != rhs.first.j)
            return lhs.first.j < rhs.first.j;
        // fallback
        return lhs.second < rhs.second;
    }
};

using LVITS = set<LiveInterval, LiveIntervalCMP>;   //被放入有序集合中，以便于线性扫描算法处理。



class LiveRangeAnalyzer {
    friend class CodeGen;

  private:
    Module *m;                                      //整个程序模块
    map<Value *, Interval> intervalmap;             //每个变量对应的活跃区间
    vector<BasicBlock*> BB_DFS_Order;               //基本块的 DFS 顺序（用于编号）
    map<int, LiveSet> IN, OUT;                      //每个基本块的活跃变量入口/出口集合
    //phi_map: 标识在bb中的copy-statement
    PhiMap &phi_map;                          //phi 映射，用于处理复制关系
    LVITS liveIntervals;                            //所有变量的活跃区间列表
    //TODO:添加你需要的变量
    int cntline;
    std::unordered_set<Value*> phi_defs;
    // 存储自然循环信息：每个元素是一个回边 (loop header, back edge source)
    std::vector<std::pair<BasicBlock *, BasicBlock *>> circles;
    // 存储自然循环范围：[loop_start_instr_id, loop_end_instr_id]
    std::vector<std::pair<int, int>> circle_range;
    std::unordered_map<Instruction *, int> instr_id;
    std::unordered_map<BasicBlock *, bool> explored;
    void get_dfs_order(Function*); 
    void reverse_post_order(BasicBlock *bb, BasicBlock *father);                  //获取 DFS 顺序
    void make_id(Function *);                      //给每条指令编号
    void make_interval(Function *);                  //构造活跃区间
    bool in_loop(int instr_id);
    void build_phi_map(Function *func);   
    LiveSet joinFor(BasicBlock *bb);                    //合并后继块的 IN 集合
    void union_ip(LiveSet &dest, LiveSet &src) {        //	并集合的辅助函数
        LiveSet res;
        set_union(dest.begin(),
                  dest.end(),
                  src.begin(),
                  src.end(),
                  std::inserter(res, res.begin()));
        dest = res;
    }
    LiveSet transferFunction(Instruction *);            //活跃变量的传递函数
    std::unordered_map<Instruction*, std::pair<int, int>> call_info;
  public:
    LiveRangeAnalyzer(Module *m_, PhiMap &phi_map_)    
        : m(m_), phi_map(phi_map_) {}
    LiveRangeAnalyzer() = delete;

    // void run();
    void run(Function *);                         //主执行函数，对单个函数做活跃区间分析
    void clear();                                  //清除当前分析状态，便于多函数分析
    static string print_liveSet(const LiveSet &ls) { //把 LiveSet 打印为字符串（如调试用）
        string s = "[ "; 
        for (auto k : ls)
            s += k->get_name() + " ";
        s += "]";
        return s;
    }
    static string print_interval(const Interval &i) {  //把区间打印为 <i, j> 形式
        return "<" + to_string(i.i) + ", " + to_string(i.j) + ">";
    }
    //获取活跃区间 / 活跃变量集合的接口
    const LVITS &get() { return liveIntervals; }  
    const decltype(intervalmap) &get_interval_map() const {
        return intervalmap;
    }
    const std::unordered_map<Instruction *, int> &get_instr_id(){
        return instr_id;
    }
    
    const decltype(IN) &get_in_set() const { return IN; }
    const decltype(OUT) &get_out_set() const { return OUT; }
    const std::map<BasicBlock*, std::vector<std::tuple<Value*, Value*, BasicBlock*> > > &get_phi_map() const {
        return phi_map;
    }
};
} // namespace LRA
#endif
//TODO: 这只是一个样例，你可以自行对框架进行修改以符合你自己的心意
//TODO: 对框架不满可尽情修改