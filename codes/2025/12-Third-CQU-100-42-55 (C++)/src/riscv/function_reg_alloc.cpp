#include "../../include/riscv/function_reg_alloc.h"
#include "../../include/riscv/graph_coloring_alloc.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <climits>

namespace llvm_ir {

// 寄存器分配算法选择
// 可以通过修改这个宏来选择不同的分配算法
#define USE_GRAPH_COLORING_ALLOC 0  // 设置为1启用图着色算法，设置为0使用线性扫描

// 辅助函数声明
static int getTypeSize(Type type);
static bool isFloatType(Type type);
static bool isPointerType(Type type);
static bool isArrayType(Type type);
static bool isFunctionType(Type type);
static int getAlignment(Type type);

// StackAllocator 方法实现
int FunctionRegAllocator::StackAllocator::allocateSlot(Value* vreg, int size, int alignment, Type type, bool is_spilled) {
    // 计算对齐后的偏移
    int aligned_offset = current_offset;
    if (aligned_offset % alignment != 0) {
        aligned_offset = ((aligned_offset / alignment) + 1) * alignment;
    }
    
    // 创建栈槽
    slots.emplace_back(vreg, aligned_offset, size, alignment, type, is_spilled);
    
    // 更新当前偏移
    current_offset = aligned_offset + size;
    
    return aligned_offset;
}

// FunctionRegAllocator 方法实现
FunctionRegAllocResult FunctionRegAllocator::allocate() {
    std::cout << "[FunctionRegAlloc] Starting allocation for function: " << func->name << std::endl;
    
    // 1. 执行寄存器分配（根据编译时选择使用不同算法）
#if USE_GRAPH_COLORING_ALLOC
    std::cout << "[FunctionRegAlloc] Using graph coloring register allocation" << std::endl;
    auto alloc_result = graph_coloring_allocate(func);
#else
    std::cout << "[FunctionRegAlloc] Using linear scan register allocation" << std::endl;
    auto alloc_result = linear_scan_allocate(func);
#endif
    
    // 2. 保存分配结果和优先级信息
    result.reg_alloc_map = alloc_result.reg_alloc_map;
    result.vreg_priorities = alloc_result.vreg_priorities;
    result.vreg_freq_info = alloc_result.vreg_freq_info;
    result.intervals = alloc_result.intervals; // 保存活跃区间信息
    result.spilled_vregs = alloc_result.spilled_vregs; // 保存溢出变量集合
    result.caller_save_vregs = alloc_result.caller_save_vregs; // 保存caller-save虚拟寄存器
    result.used_caller_save_regs = alloc_result.used_caller_save_regs; // 保存使用的caller-save物理寄存器
    
    // 2.5. 处理额外需要溢出的参数
    for (Value* param : extra_spilled_params) {
        std::cout << "[FunctionRegAlloc] Adding extra spilled parameter: " << param->name << std::endl;
        // 将参数加入到溢出变量集合中
        result.spilled_vregs.insert(param);
        // 为溢出参数创建分配结果，标记为非寄存器分配
        if(!result.reg_alloc_map.count(param)){

            result.reg_alloc_map[param] = {false, riscv::x0, 0};
        }
        else {
            std::cout<< "[FunctionRegAlloc] Starting allocation for function: 给函数形参分配了s";
            abort();
        }
    }
    
    // 3. 静态分配栈槽
    allocateStackSlots();

    //printAllocationInfo();

    // 4. 计算总溢出的变量使用的栈大小
    result.total_stack_size = stack_allocator.getTotalSize();
    
    // 5. 更新结果
    result.stack_slots = stack_allocator.getSlots();
    return result;
}

FunctionRegAllocResult FunctionRegAllocator::post_allocate(int tot_sub_pre) {
    std::cout<<"[post_allocate]: tot_sub_pre " << tot_sub_pre << std::endl;
    for(auto &[x, y]: result.reg_alloc_map) {
        y.stack_offset = tot_sub_pre - y.stack_offset - getTypeSize(x->type);
    }
    // 6. 初始化
    // resolvePhysicalRegisterConflicts();

    // 7. 输出分配信息
    //std::cout<<"tot_sub_pre :"<< tot_sub_pre << std::endl;
    printAllocationInfo();
    return result;
}


void FunctionRegAllocator::allocateStackSlots() {
    std::cout << "[FunctionRegAlloc] Allocating stack slots for spilled variables..." << std::endl;
    
    // 只对 linear_scan_allocate 传递下来的 spilled_vregs 分配栈槽
    std::vector<Value*> int_spilled, float_spilled, pointer_spilled, array_spilled, function_spilled;
    
    for (Value* vreg : result.spilled_vregs) {
        if (isFloatType(vreg->type)) {
            float_spilled.push_back(vreg);
        } else if (isPointerType(vreg->type)) {
            pointer_spilled.push_back(vreg);
        } else if (isArrayType(vreg->type)) {
            array_spilled.push_back(vreg);
        } else if (isFunctionType(vreg->type)) {
            function_spilled.push_back(vreg);
        } else {
            int_spilled.push_back(vreg);
        }
    }
    
    // 按优先级分配栈槽（从对齐要求低到高）
    
    // 1. 先分配整数变量（对齐要求最低）
    for (Value* vreg : int_spilled) {
        int size = getTypeSize(vreg->type);
        int alignment = getAlignment(vreg->type);
        int offset = stack_allocator.allocateSlot(vreg, size, alignment, vreg->type, true);
        // 修复：分配后加断言，确保 offset 合理
        assert(offset >= 0 && "Spilled int variable offset must >= 0");
        // 更新寄存器分配映射中的栈偏移
        result.reg_alloc_map[vreg].stack_offset = offset;
        std::cout << "  Allocated int slot: " << vreg->name << " (" << vreg->getTypeName() 
                  << ") at offset " << offset << " (size: " << size << "B, align: " << alignment << "B)" << std::endl;
    }
    
    // 2. 分配指针变量
    for (Value* vreg : pointer_spilled) {
        int size = getTypeSize(vreg->type);
        int alignment = getAlignment(vreg->type);
        int offset = stack_allocator.allocateSlot(vreg, size, alignment, vreg->type, true);
        assert(offset >= 0 && "Spilled pointer variable offset must >= 0");
        result.reg_alloc_map[vreg].stack_offset = offset;
        
        std::cout << "  Allocated pointer slot: " << vreg->name << " (" << vreg->getTypeName() 
                  << ") at offset " << offset << " (size: " << size << "B, align: " << alignment << "B)" << std::endl;
    }
    
    // 3. 分配数组指针变量
    for (Value* vreg : array_spilled) {
        int size = getTypeSize(vreg->type);
        int alignment = getAlignment(vreg->type);
        int offset = stack_allocator.allocateSlot(vreg, size, alignment, vreg->type, true);
        assert(offset >= 0 && "Spilled array variable offset must >= 0");
        result.reg_alloc_map[vreg].stack_offset = offset;
        
        std::cout << "  Allocated array slot: " << vreg->name << " (" << vreg->getTypeName() 
                  << ") at offset " << offset << " (size: " << size << "B, align: " << alignment << "B)" << std::endl;
    }
    
    // 4. 分配函数指针变量
    for (Value* vreg : function_spilled) {
        int size = getTypeSize(vreg->type);
        int alignment = getAlignment(vreg->type);
        int offset = stack_allocator.allocateSlot(vreg, size, alignment, vreg->type, true);
        assert(offset >= 0 && "Spilled function variable offset must >= 0");
        result.reg_alloc_map[vreg].stack_offset = offset;
        
        std::cout << "  Allocated function slot: " << vreg->name << " (" << vreg->getTypeName() 
                  << ") at offset " << offset << " (size: " << size << "B, align: " << alignment << "B)" << std::endl;
    }
    
    // 5. 最后分配浮点变量（对齐要求最高）
    for (Value* vreg : float_spilled) {
        int size = getTypeSize(vreg->type);
        int alignment = getAlignment(vreg->type);
        int offset = stack_allocator.allocateSlot(vreg, size, alignment, vreg->type, true);
        assert(offset >= 0 && "Spilled float variable offset must >= 0");
        result.reg_alloc_map[vreg].stack_offset = offset;
        
        std::cout << "  Allocated float slot: " << vreg->name << " (" << vreg->getTypeName() 
                  << ") at offset " << offset << " (size: " << size << "B, align: " << alignment << "B)" << std::endl;
    }


    /*for(Value* vreg : result.spilled_vregs){
        // 确保所有溢出变量的 is_reg 字段都设置为 false
        auto tmp = result.reg_alloc_map[vreg];
        std::cout<< vreg->name <<" "<< tmp.regid <<" "<< tmp.is_reg <<" "<< tmp.stack_offset << std::endl;
    }
    abort(); // 调试用，查看分配结果*/
}

void FunctionRegAllocator::resolvePhysicalRegisterConflicts() {
    std::cout << "[FunctionRegAlloc] Resolving physical register conflicts..." << std::endl;
    
    for (auto& [vreg, alloc] : result.reg_alloc_map) {
        std::cout<< vreg->name << " is allocated to physical register " << alloc.regid << std::endl;
        alloc.is_reg = false;
    }
    
}

int FunctionRegAllocator::calculateStackAlignment() const {
    int max_alignment = 1;
    for (const auto& slot : result.stack_slots) {
        max_alignment = std::max(max_alignment, slot.alignment);
    }
    return max_alignment;
}

std::string FunctionRegAllocator::generateStackFrameSetup() const {
    std::ostringstream oss;
    
    if (result.total_stack_size > 0) {
        oss << "    # Stack frame setup for " << result.function_name << std::endl;
        oss << "    addi sp, sp, -" << result.total_stack_size << "  # Allocate stack frame" << std::endl;
        
        // 生成栈槽初始化代码（如果需要）
        for (const auto& slot : result.stack_slots) {
            if (slot.is_spilled) {
                oss << "    # Slot for " << slot.vreg->name << " at offset " << slot.offset << std::endl;
                // 这里可以添加初始化代码，比如清零等
            }
        }
    }
    
    return oss.str();
}

std::string FunctionRegAllocator::generateStackFrameCleanup() const {
    std::ostringstream oss;
    
    if (result.total_stack_size > 0) {
        oss << "    # Stack frame cleanup for " << result.function_name << std::endl;
        oss << "    addi sp, sp, " << result.total_stack_size << "  # Deallocate stack frame" << std::endl;
    }
    
    return oss.str();
}

void FunctionRegAllocator::printAllocationInfo() const {
    for(auto [x,y] : result.reg_alloc_map) {
        bool is_spilled = (result.spilled_vregs.find(x) != result.spilled_vregs.end());
        std::cout <<"is_reg: "<< y.is_reg
                  <<" regid: "<< riscv::to_string(y.regid)
                  <<" vreg溢出在栈上的地址: "<<y.stack_offset 
                  <<" 是否溢出 "<< is_spilled
                  <<" name: " << x->name << std::endl;
    }
    //abort(); // 调试用，查看分配结果
}

// 全局函数实现
FunctionRegAllocResult allocateFunctionRegisters(Function* func) {
    FunctionRegAllocator allocator(func);
    return allocator.allocate();
}

// 辅助函数实现（从linear_scan_alloc.cpp复制过来）
static int getTypeSize(Type type) {
    switch (type) {
        case Type::I1:   return 4;   // bool
        case Type::I8:   return 4;   // char
        case Type::I32:  return 4;   // int
        case Type::I64:  return 8;   // long
        case Type::Float: return 4;  // float
        case Type::Double: return 8; // double
        case Type::Ptr:  return 8;   // pointer (64位系统)
        case Type::Array: return 8;  // array pointer
        case Type::Function: return 8; // function pointer
        case Type::Void: return 0;   // void (不应该分配内存)
        default: return 4;           // 默认4字节对齐
    }
}

static bool isFloatType(Type type) {
    return type == Type::Float || type == Type::Double;
}

static bool isPointerType(Type type) {
    return type == Type::Ptr;
}

static bool isArrayType(Type type) {
    return type == Type::Array;
}

static bool isFunctionType(Type type) {
    return type == Type::Function;
}

static int getAlignment(Type type) {
    switch (type) {
        case Type::I1:   return 4;
        case Type::I8:   return 4;
        case Type::I32:  return 4;
        case Type::I64:  return 8;
        case Type::Float: return 4;
        case Type::Double: return 8;
        case Type::Ptr:  return 8;
        case Type::Array: return 8;
        case Type::Function: return 8;
        case Type::Void: return 0;
        default: return 4;
    }
}

} // namespace llvm_ir 