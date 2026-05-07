#pragma once
#include "llvm_ir.h"
#include <vector>
#include <unordered_map>
#include <set>

namespace llvm_ir {

// 单个活跃区间
struct LiveRange {
    int start, end; // [start, end)
    LiveRange(int s, int e) : start(s), end(e) {}
    bool operator<(const LiveRange& rhs) const { return start < rhs.start || (start == rhs.start && end < rhs.end); }
};

// 一个虚拟寄存器的活跃区间（强制合并为单个连续区间）
struct LiveInterval {
    std::vector<LiveRange> ranges;  // 实际上只会有一个区间
    void add_range(int s, int e);   // 强制合并策略，确保只有一个连续区间
};

// 所有虚拟寄存器的活跃区间
using LiveIntervalMap = std::unordered_map<Value*, LiveInterval>;

// 计算所有虚拟寄存器的活跃区间（支持多区间）
LiveIntervalMap compute_live_intervals(Function* func);

}
