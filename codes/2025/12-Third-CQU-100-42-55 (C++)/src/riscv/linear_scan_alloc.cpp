#include "../../include/riscv/linear_scan_alloc.h"
#include "../../include/ir/loop_analysis.h"
#include "../../include/ir/cfg.h"
#include <vector>
#include <algorithm>
#include <set>
#include <unordered_map>

namespace llvm_ir {

// 辅助函数：检查寄存器是否为临时寄存器
static bool is_temp_reg(riscv::reg r) {
    // 整数临时寄存器: t0-t6 (x5, x6, x7, x28, x29, x30, x31)
    if (r == riscv::x5 || r == riscv::x6 || r == riscv::x7 || 
        r == riscv::x28 || r == riscv::x29 || r == riscv::x30 || r == riscv::x31) {
        return true;
    }
    // 浮点临时寄存器: ft0-ft11 (f0-f7, f28-f31)
    if ((r >= riscv::f0 && r <= riscv::f7) || 
        (r >= riscv::f28 && r <= riscv::f31)) {
        return true;
    }
    return false;
}

// 修正的寄存器池配置 - 按照RISC-V ABI和优先级排序
// 前5个临时寄存器(t0-t4, ft0-ft4)保留给代码生成器，剩余临时寄存器参与分配
static const std::vector<riscv::reg> kIntRegs = {
    // 保存寄存器 (s0-s11) - 最高优先级，callee-save
    riscv::x8, //s0 
    riscv::x9, riscv::x18, riscv::x19, riscv::x20, riscv::x21, 
    riscv::x22, riscv::x23, riscv::x24, riscv::x25, riscv::x26, riscv::x27,
    // 可用临时寄存器 (t5-t6) - 次优先级，caller-save
    riscv::x30, riscv::x31,
    // 参数寄存器 (a0-a7) - 保留给函数调用
    //riscv::x10, riscv::x11, riscv::x12, riscv::x13, riscv::x14, riscv::x15, riscv::x16, riscv::x17
    // 保留临时寄存器 (t0-t4) - 保留给代码生成器临时使用：x5, x6, x7, x28, x29
};

static const std::vector<riscv::reg> kFloatRegs = {
    // 保存浮点寄存器 (fs0-fs11) - 最高优先级，callee-save
    riscv::f8, riscv::f9, riscv::f18, riscv::f19, riscv::f20, riscv::f21, 
    riscv::f22, riscv::f23, riscv::f24, riscv::f25, riscv::f26, riscv::f27,
    // 可用临时浮点寄存器 (ft5-ft11) - 次优先级，caller-save
    riscv::f5, riscv::f6, riscv::f7, riscv::f28, riscv::f29, riscv::f30, riscv::f31,
    // 参数浮点寄存器 (fa0-fa7) - 保留给函数调用
    //riscv::f10, riscv::f11, riscv::f12, riscv::f13, riscv::f14, riscv::f15, riscv::f16, riscv::f17
    // 保留临时浮点寄存器 (ft0-ft4) - 保留给代码生成器临时使用：f0, f1, f2, f3, f4
};

// 根据类型判断是否为浮点类型
static bool isFloatType(Type type) {
    return type == Type::Float || type == Type::Double;
}

// 辅助函数：判断寄存器是否为caller-save寄存器
static bool is_caller_save_reg(riscv::reg r) {
    // 临时寄存器 t5-t6: x30, x31
    if (r == riscv::x30 || r == riscv::x31) {
        return true;
    }
    // 临时浮点寄存器 ft5-ft11: f5, f6, f7, f28, f29, f30, f31
    if (r == riscv::f5 || r == riscv::f6 || r == riscv::f7 ||
        r == riscv::f28 || r == riscv::f29 || r == riscv::f30 || r == riscv::f31) {
        return true;
    }
    // 参数寄存器也是caller-save
    if ((r >= riscv::x10 && r <= riscv::x17) || (r >= riscv::f10 && r <= riscv::f17)) {
        return true;
    }
    return false;
}

// 辅助函数：判断寄存器是否为callee-save寄存器
static bool is_callee_save_reg(riscv::reg r) {
    // 保存寄存器 s0-s11: x8, x9, x18-x27
    if (r == riscv::x8 || r == riscv::x9 || (r >= riscv::x18 && r <= riscv::x27)) {
        return true;
    }
    // 保存浮点寄存器 fs0-fs11: f8, f9, f18-f27
    if (r == riscv::f8 || r == riscv::f9 || (r >= riscv::f18 && r <= riscv::f27)) {
        return true;
    }
    return false;
}

// 访问频率分析结果已在头文件中定义

// 分析变量的访问频率
std::unordered_map<Value*, AccessFrequencyInfo> analyzeAccessFrequency(llvm_ir::LiveIntervalMap &intervals, Function* func) {
    std::unordered_map<Value*, AccessFrequencyInfo> freq_info;
    
    // 初始化所有变量的访问信息
    for (auto& bb_ptr : func->basicBlocks) {
        for (auto& inst_ptr : bb_ptr->instructions) {
            Instruction* inst = inst_ptr.get();
            
            // 分析指令的定义
            if (inst) {
                auto& info = freq_info[inst];
                info.total_accesses++;
                info.def_count++;
            }
            
            // 分析指令的操作数使用
            for (auto* operand : inst->operands) {
                if (operand && dynamic_cast<Instruction*>(operand)) {
                    auto& info = freq_info[operand];
                    info.total_accesses++;
                    info.use_count++;
                }
            }
        }
    }
    
    // 计算访问密度
    // auto intervals = compute_live_intervals(func);
    for (auto& [vreg, info] : freq_info) {
        auto it = intervals.find(vreg);
        if (it != intervals.end()) {
            int total_lifetime = 0;
            for (const auto& range : it->second.ranges) {
                total_lifetime += (range.end - range.start);
            }
            if (total_lifetime > 0) {
                info.access_density = static_cast<float>(info.total_accesses) / total_lifetime;
            }
        }
    }
    
    return freq_info;
}

// 分析循环深度
int analyzeLoopDepth(const LiveInterval& interval, Function* func) {
    // 简化实现：基于活跃区间的长度估算循环深度
    // 在实际实现中，应该基于控制流图分析循环结构
    int total_lifetime = 0;
    for (const auto& range : interval.ranges) {
        total_lifetime += (range.end - range.start);
    }
    
    // 假设长生命周期的变量更可能在循环中
    if (total_lifetime > 100) return 3;  // 深度循环
    if (total_lifetime > 50) return 2;   // 中等循环
    if (total_lifetime > 20) return 1;   // 浅层循环
    return 0;  // 无循环
}

std::unordered_map<Value*, int> loop_depth(Function* func) {
    // 构建支配树
    cfg::DominatorTree DT;
    DT.runOnFunction(*func);
    
    // 分析循环
    LoopAnalysis LA;
    LA.runOnFunction(*func, DT);

    std::map<llvm_ir::BasicBlock *, int> loop_nest;

    // Initialize all blocks to nesting depth 0
    for (auto& bb : func->basicBlocks) {
        loop_nest[bb.get()] = 0;
    }

    // For each loop, increment nesting depth of all contained blocks
    for (auto& loop : LA.loops) {
        int loopDepth = 1;

        // Calculate the nesting depth of this loop
        Loop* parent = loop->getParent();
        while (parent) {
            loopDepth++;
            parent = parent->getParent();
        }

        // Set nesting depth for all blocks in this loop
        for (BasicBlock* bb : loop->getBlocks()) {
            loop_nest[bb] = std::max(loop_nest[bb], loopDepth);
        }
    }

    // 为每个虚拟寄存器计算循环深度
    std::unordered_map<Value*, int> vreg_loop_depth;
    
    // 遍历所有指令，找到虚拟寄存器的定义和使用位置
    for (auto& bb_ptr : func->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        int bb_loop_depth = loop_nest[bb];
        
        // 处理普通指令
        for (auto& inst_ptr : bb->instructions) {
            Instruction* inst = inst_ptr.get();
            if (!inst) continue;
            
            // 跳过常量、UndefValue和全局变量
            if (dynamic_cast<Constant*>(inst) || 
                dynamic_cast<UndefValue*>(inst) || 
                inst->name[0] == '@') {
                continue;
            }
            
            // 指令本身是一个虚拟寄存器（定义位置），记录其循环深度
            auto& current_depth = vreg_loop_depth[inst];
            current_depth = std::max(current_depth, bb_loop_depth);
            
            // 处理指令的操作数（使用位置）
            for (auto* operand : inst->operands) {
                if (operand && dynamic_cast<Instruction*>(operand)) {
                    // 跳过常量、UndefValue和全局变量
                    if (dynamic_cast<Constant*>(operand) || 
                        dynamic_cast<UndefValue*>(operand) || 
                        operand->name[0] == '@') {
                        continue;
                    }
                    
                    // 更新操作数的循环深度（使用位置）
                    auto& operand_depth = vreg_loop_depth[operand];
                    operand_depth = std::max(operand_depth, bb_loop_depth);
                }
            }
        }
    }
    
    return vreg_loop_depth;
}

// 多因素优先级计算
int calculateSpillPriority(const LiveInterval& interval, const Value* vreg, 
                          const AccessFrequencyInfo& freq_info, Function* func, std::unordered_map<Value*, int> vreg_loop_depth) {
    int priority = 0;
    
    // 1. 访问频率权重 (40%) - 访问次数越多，优先级越高
    priority += freq_info.total_accesses * 40;
    
    // 2. 访问密度权重 (25%) - 高密度访问的变量优先级更高
    priority += static_cast<int>(freq_info.access_density * 100) * 25;

    // 3. 循环深度权重 (20%) - 循环内的变量优先级更高
    int loop_depth = 0;
    auto it = vreg_loop_depth.find(const_cast<Value*>(vreg));
    if (it != vreg_loop_depth.end()) {
        loop_depth = it->second;
    } else {
        // std::cout << "vreg: " << vreg->name << " not found in vreg_loop_depth" << std::endl;
    }
    // std::cout << "vreg: " << vreg->name << " loop_depth: " << loop_depth << std::endl;
    priority += loop_depth * 60;

    // 4. 生命周期权重 (10%) - 生命周期短的变量优先级稍高
    int total_lifetime = 0;
    for (const auto& range : interval.ranges) {
        total_lifetime += (range.end - range.start);
    }
    priority += (1000 - total_lifetime) * 10;  // 生命周期短优先级高
    
    // 5. 变量类型权重 (5%) - 浮点变量优先级稍高（寄存器更稀缺）
    priority += (isFloatType(vreg->type) ? 5 : 0);
    
    return priority;
}

LinearScanResult linear_scan_allocate(Function* func) {
    LinearScanResult result;
    result.stack_size_int = 0; 
    result.stack_size_float = 0;
    auto intervals = compute_live_intervals(func);
    
    // 将活跃区间信息保存到结果中
    result.intervals = intervals;
    
    // 分析访问频率
    auto freq_info = analyzeAccessFrequency(intervals, func);
    
    // 调试输出：打印每个虚拟寄存器的活跃区间和访问频率
    //std::cout << "[RegAlloc] Function: " << func->name << std::endl;
    for (auto& [v, interval] : intervals) {
        std::cout << "  vreg: " << v->toString() << " (" << v->getTypeName() << ") range: ";
        if (!interval.ranges.empty()) {
            auto& r = interval.ranges[0]; // 只有一个区间
            std::cout << "[" << r.start << "," << r.end << ")";
        }
        
        auto freq_it = freq_info.find(v);
        if (freq_it != freq_info.end()) {
            std::cout << " | accesses: " << freq_it->second.total_accesses 
                      << " | density: " << freq_it->second.access_density;
        }
        std::cout << std::endl;
    }
    
    // 1. 收集所有区间端点，按虚拟寄存器分组
    struct IntervalInfo {
        Value* vreg;
        LiveRange range;
        bool is_float;
        int priority; // 基于访问频率的优先级
        AccessFrequencyInfo freq_info;
    };
    std::vector<IntervalInfo> all_intervals;

    auto vreg_loop_depth = loop_depth(func);

    for (auto& [v, interval] : intervals) {
        // 跳过UndefValue和常量
        if (dynamic_cast<llvm_ir::UndefValue*>(v) || dynamic_cast<llvm_ir::Constant*>(v) || v->name[0] == '@') continue;
        
        // 跳过函数形参（形参寄存器由caller管理，不需要分配寄存器）
        bool is_function_param = false;
        for (auto* param : func->parameters) {
            if (param == v) {
                is_function_param = true;
                break;
            }
        }
        if (is_function_param) continue;
        
        bool is_float = isFloatType(v->type);
        
        // 获取访问频率信息
        auto freq_it = freq_info.find(v);
        AccessFrequencyInfo freq = (freq_it != freq_info.end()) ? freq_it->second : AccessFrequencyInfo{};
        
        // 计算基于访问频率的优先级
        int priority = calculateSpillPriority(interval, v, freq, func, vreg_loop_depth);
        
        // 保存优先级和访问频率信息
        result.vreg_priorities[v] = priority;
        result.vreg_freq_info[v] = freq;
        
        // 由于每个变量只有一个活跃区间，直接使用第一个（也是唯一的）区间
        if (!interval.ranges.empty()) {
            all_intervals.push_back({v, interval.ranges[0], is_float, priority, freq});
        }
    }
    
    // 2. 按区间起点排序
    std::sort(all_intervals.begin(), all_intervals.end(), [](const IntervalInfo& a, const IntervalInfo& b) {
        return a.range.start < b.range.start;
    });
    
    // 3. 线性扫描分配
    struct ActiveInterval {
        Value* vreg;
        LiveRange range;
        riscv::reg regid;
        int priority;
        bool is_float;
        AccessFrequencyInfo freq_info;
    };
    
    std::vector<ActiveInterval> active_int, active_float;
    
    for (auto& interval : all_intervals) {
        auto& active = interval.is_float ? active_float : active_int;
        const auto& reg_pool = interval.is_float ? kFloatRegs : kIntRegs;
        
        // 过期区间移除
        for (auto it = active.begin(); it != active.end(); ) {
            if (it->range.end <= interval.range.start) {
                it = active.erase(it);
            } else {
                ++it;
            }
        }
        
        // 尝试分配寄存器
        riscv::reg alloc_reg = riscv::NONE;
        
        // 检查是否有可用寄存器
        std::set<riscv::reg> used_regs;
        for (auto& a : active) {
            used_regs.insert(a.regid);
        }
        
        // 从寄存器池中找到第一个可用的寄存器
        for (auto r : reg_pool) {
            if (used_regs.find(r) == used_regs.end()) {
                alloc_reg = r;
                break;
            }
        }
        
        // 如果没有可用寄存器，找到当前区间和所有活跃区间中优先级最低的一个溢出
        if (alloc_reg == riscv::NONE) {
            // 比较当前区间和所有活跃区间的优先级，找到最低的
            int lowest_priority = interval.priority;
            Value* spill_vreg = interval.vreg;
            auto spill_candidate = active.end(); // 初始化为end()表示选择当前区间
            bool spill_current = true;
            
            // 检查活跃区间中是否有更低优先级的
            for (auto it = active.begin(); it != active.end(); ++it) {
                if (it->priority < lowest_priority) {
                    lowest_priority = it->priority;
                    spill_candidate = it;
                    spill_vreg = it->vreg;
                    spill_current = false;
                }
            }
            
            if (spill_current) {
                // 溢出当前区间到栈
                std::cout << "   SPILL CURRENT: vreg " << interval.vreg->name 
                          << " (" << interval.vreg->getTypeName() << ")"
                          << " to stack (priority: " << interval.priority 
                          << ", accesses: " << interval.freq_info.total_accesses << ")" << std::endl;
                // 记录溢出变量
                result.spilled_vregs.insert(interval.vreg);
                // 更新当前变量的分配信息为栈分配
                result.reg_alloc_map[interval.vreg].is_reg = false;
                // 当前区间不加入active列表，直接继续下一个区间
                continue;
            } else {
                // 溢出活跃区间中优先级最低的
                alloc_reg = spill_candidate->regid;
                std::cout << "    FORCE SPILL: vreg " << spill_candidate->vreg->name 
                          << " (" << spill_candidate->vreg->getTypeName() << ")"
                          << " from reg " << riscv::to_string(alloc_reg) 
                          << " to stack (priority: " << spill_candidate->priority 
                          << ", accesses: " << spill_candidate->freq_info.total_accesses << ")" << std::endl;
                // 记录溢出变量
                result.spilled_vregs.insert(spill_candidate->vreg);
                // 更新被溢出变量的分配信息为栈分配
                result.reg_alloc_map[spill_candidate->vreg].is_reg = false;
                active.erase(spill_candidate);
            }
        }
        
        // 分配决策
        std::cout << "    try alloc vreg: " << interval.vreg->name 
                  << " (" << interval.vreg->getTypeName() << ")"
                  << " [" << interval.range.start << "," << interval.range.end << ") "
                  << "(priority: " << interval.priority 
                  << ", accesses: " << interval.freq_info.total_accesses << ") ";
        
        std::cout << "-> reg: " << riscv::to_string(alloc_reg) << std::endl;
        active.push_back({interval.vreg, interval.range, alloc_reg, interval.priority, interval.is_float, interval.freq_info});
        // 更新分配映射 - 无论之前是否分配过，都要更新为当前的寄存器分配状态
        result.reg_alloc_map[interval.vreg] = {true, alloc_reg, 0}; // is_reg = true, 分配到寄存器
        
        // 记录使用的caller-save寄存器和需要caller-save的虚拟寄存器
        if (is_caller_save_reg(alloc_reg)) {
            result.used_caller_save_regs.insert(alloc_reg);
            result.caller_save_vregs.insert(interval.vreg);
            std::cout << "    CALLER-SAVE: vreg " << interval.vreg->name 
                      << " allocated to caller-save register " << riscv::to_string(alloc_reg) << std::endl;
        }
    }
    
    // 统计溢出变量数量和访问频率信息
    int spilled_count = 0;
    int total_spilled_accesses = 0;
    for (const auto& [vreg, alloc] : result.reg_alloc_map) {
        if (!alloc.is_reg) {
            spilled_count++;
            auto freq_it = freq_info.find(vreg);
            if (freq_it != freq_info.end()) {
                total_spilled_accesses += freq_it->second.total_accesses;
            }
        }
    }
    
    std::cout << "[RegAlloc] Register allocation completed. " 
              << spilled_count << " variables spilled to stack." << std::endl;
    std::cout << "[RegAlloc] Total accesses in spilled variables: " 
              << total_spilled_accesses << std::endl;
    
    return result;
}

}
