#pragma once
#include "../ir/llvm_ir.h"
#include "../ir/live_interval.h"
#include <unordered_map>
#include "../riscv/riscv.h"

namespace llvm_ir {

// 访问频率分析结果
struct AccessFrequencyInfo {
    int total_accesses;      // 总访问次数
    int def_count;           // 定义次数
    int use_count;           // 使用次数
    int loop_accesses;       // 循环内访问次数
    int critical_accesses;   // 关键路径访问次数
    float access_density;    // 访问密度 (访问次数/生命周期长度)
    
    AccessFrequencyInfo() : total_accesses(0), def_count(0), use_count(0), 
                           loop_accesses(0), critical_accesses(0), access_density(0.0f) {}
};

struct RegAllocResult {
    bool is_reg;
    riscv::reg regid; // is_reg==true
    int stack_offset; // is_reg==false
};

inline bool is_float_reg(riscv::reg r) { return r >= riscv::f0 && r <= riscv::f31; }
inline bool is_int_reg(riscv::reg r) { return r >= riscv::x0 && r <= riscv::x31; }

// 每个函数的虚拟寄存器分配表
typedef std::unordered_map<Value*, RegAllocResult> VRegAllocMap;

// 访问频率分析函数
std::unordered_map<Value*, AccessFrequencyInfo> analyzeAccessFrequency(Function* func);

// 循环深度分析函数
int analyzeLoopDepth(const LiveInterval& interval, Function* func);

// 循环深度分析函数 - 返回每个虚拟寄存器的循环深度
std::unordered_map<Value*, int> loop_depth(Function* func);

// 多因素优先级计算函数
int calculateSpillPriority(const LiveInterval& interval, const Value* vreg, 
                          const AccessFrequencyInfo& freq_info, Function* func);

// 线性扫描分配结果
struct LinearScanResult {
    VRegAllocMap reg_alloc_map;
    std::unordered_map<Value*, int> vreg_priorities;
    std::unordered_map<Value*, AccessFrequencyInfo> vreg_freq_info;
    std::unordered_map<Value*, LiveInterval> intervals; // 新增：活跃区间信息
    int stack_size_int;
    int stack_size_float;
    std::set<Value*> spilled_vregs; // 新增
    std::set<Value*> caller_save_vregs; // 新增：需要caller-save的虚拟寄存器
    std::set<riscv::reg> used_caller_save_regs; // 新增：使用的caller-save物理寄存器
};

// 线性扫描分配主接口，返回分配结果
LinearScanResult linear_scan_allocate(Function* func);

} // namespace llvm_ir
