#pragma once
#include "../ir/llvm_ir.h"
#include "../ir/live_interval.h"
#include "linear_scan_alloc.h"
#include <unordered_map>
#include <vector>
#include <set>

namespace llvm_ir {

// 栈槽信息
struct StackSlot {
    Value* vreg;           // 对应的虚拟寄存器
    int offset;            // 栈偏移
    int size;              // 大小（字节）
    int alignment;         // 对齐要求
    Type type;             // 类型
    bool is_spilled;       // 是否为溢出变量
    
    StackSlot(Value* v, int off, int sz, int align, Type t, bool spilled = false)
        : vreg(v), offset(off), size(sz), alignment(align), type(t), is_spilled(spilled) {}
};

// 函数寄存器分配结果
struct FunctionRegAllocResult {
    // 寄存器分配映射
    VRegAllocMap reg_alloc_map;
    // 新增：物理寄存器到虚拟寄存器的映射
    std::unordered_map<riscv::reg, llvm_ir::Value*> reg_to_vreg_map;

    // 栈槽信息
    std::vector<StackSlot> stack_slots;
    
    // 栈帧大小
    int total_stack_size;
    
    // 溢出变量集合
    std::set<Value*> spilled_vregs; // 新增
    
    // 溢出变量统计
    int spilled_int_count;
    int spilled_float_count;
    int spilled_pointer_count;
    int spilled_array_count;
    int spilled_function_count;
    
    // 优先级信息（从线性扫描分配阶段获取）
    std::unordered_map<Value*, int> vreg_priorities;
    std::unordered_map<Value*, AccessFrequencyInfo> vreg_freq_info;
    std::unordered_map<Value*, LiveInterval> intervals; // 新增：活跃区间信息
    
    // Caller-save相关信息
    std::set<Value*> caller_save_vregs; // 需要caller-save的虚拟寄存器
    std::set<riscv::reg> used_caller_save_regs; // 使用的caller-save物理寄存器
    
    // 调试信息
    std::string function_name;
    
    FunctionRegAllocResult() : total_stack_size(0), spilled_int_count(0), spilled_float_count(0), 
                               spilled_pointer_count(0), spilled_array_count(0), spilled_function_count(0) {}
};

// 函数寄存器分配器
class FunctionRegAllocator {
private:
    Function* func;
    FunctionRegAllocResult result;
    std::vector<Value*> extra_spilled_params;  // 额外需要溢出的参数
    
    // 栈分配器
    struct StackAllocator {
        int current_offset;
        std::vector<StackSlot> slots;
        
        StackAllocator() : current_offset(0) {}
        
        // 分配栈槽
        int allocateSlot(Value* vreg, int size, int alignment, Type type, bool is_spilled = false);
        
        // 获取总栈大小
        int getTotalSize() const { return current_offset; }
        
        // 获取所有栈槽
        const std::vector<StackSlot>& getSlots() const { return slots; }
    };
    
    StackAllocator stack_allocator;

public:
    FunctionRegAllocator(Function* f, const std::vector<Value*>& spilled_params = {}) 
        : func(f), extra_spilled_params(spilled_params) {
        result.function_name = f->name;
    }
    
    // 执行函数寄存器分配
    FunctionRegAllocResult allocate();

    // 后处理：更新结果
    FunctionRegAllocResult post_allocate(int tot_sub_pre);
    
    // 生成栈帧设置代码（用于代码生成阶段）
    std::string generateStackFrameSetup() const;
    
    // 生成栈帧清理代码
    std::string generateStackFrameCleanup() const;
    
    // 获取溢出变量的栈偏移
    int getSpilledVarOffset(Value* vreg) const;
    
    // 检查变量是否溢出
    bool isSpilled(Value* vreg) const;
    
    // 获取调试信息
    void printAllocationInfo() const;

private:
    // 收集溢出变量
    void collectSpilledVariables();
    
    // 解决物理寄存器冲突：只有时间戳最小的虚拟寄存器保持is_reg=true
    void resolvePhysicalRegisterConflicts();
    
    // 静态分配栈槽
    void allocateStackSlots();
    
    // 计算栈对齐
    int calculateStackAlignment() const;
    
    // 生成栈槽分配代码
    std::string generateSlotAllocationCode() const;
};

// 全局函数：执行函数寄存器分配
FunctionRegAllocResult allocateFunctionRegisters(Function* func);

} // namespace llvm_ir 