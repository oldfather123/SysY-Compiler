#ifndef __REGALLOC_HPP__
#define __REGALLOC_HPP__

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "riscv.hpp"

static const int SAFE_FIND_LIMIT = 3;

class UnionFind {
private:
    unordered_map<Value*, Value*> parent; // 存储每个 Value 的父结点

    /// @brief 获取 Value 的根结点，同时进行路径压缩
    /// @param val 对应的 Value
    /// @return 对应的根结点
    Value* get_parent(Value* val);
public:
    /// @brief 查询 Value 的根结点
    /// @param val 对应的 Value
    /// @return 对应的根结点
    Value* query(Value* val);

    /// @brief 合并两个 Value 的集合为同一集合
    /// @param val1 子结点
    /// @param val2 父结点
    void merge(Value* val1, Value* val2);
};

// 关于额外发射指令问题说明
// 举例：如果当前需要使用特定寄存器（以a0为例）以存取返回值
// 1. 如果当前变量在内存：a)
// a)
// 由regalloca发射一个sw指令（使用rbb中addInstrback函数）将现在的a0送回对应内存或栈上地址
// b) 由regalloca发射一个lw指令（使用rbb中addInstrback函数）将该变量移入a0中
// 2. 如果该变量在寄存器x中：
// a)
// 由regalloca发射一个sw指令（使用rbb中addInstrback函数）将现在的a0送回对应内存或栈上地址
// b) 由regalloca发射一个mv指令（使用rbb中addInstrback函数）将该变量从x移入a0中

// 举例：为当前一个未指定寄存器，或当前寄存器堆中没有存放该变量的寄存器现在需要为该变量找一个寄存器以进行运算
// 存在空寄存器：找一个空闲未分配寄存器，然后返回一个寄存器指针riscvOperand*
// 不存在空寄存器：
// a) 找一个寄存器
// b)
// 由regalloca发射一个sw指令（使用rbb中addInstrback函数）将现在该寄存器的数字送回对应内存或栈上地址
// c) 返回该寄存器指针riscvOperand*

// 注意区分指针类型（如*a0）和算数值（a0）的区别

// 每个变量会有一个固定的内存或栈地址，可能会被分配一个固定寄存器地址


// RegAlloca类被放置在**每个函数**内，每个函数内是一个新的寄存器分配类
// 因而约定x8-x9 x18-27、f8-9、f18-27
// 是约定的所有函数都要保护的寄存器，用完要恢复原值
// 其他的寄存器（除函数参数所用的a0-a7等寄存器）都视为是不安全的，可能会在之后的运算中发生变化
// TODO:3
// 在该类的实例生存周期内，使用到的需要保护的寄存器使用一个vector<Register*>
// 存储

// 寄存器分配（IR变量到汇编变量地址映射）
// 所有的临时变量均分配在栈上（从当前函数开始的地方开始计算栈地址，相对栈偏移地址），所有的全局变量放置在内存中（首地址+偏移量形式）
// 当存在需要寄存器保护的时候，直接找回原地址去进行
class RegAlloc {
private:
    int timestamp;
    int int_reg_id;
    int float_reg_id; // 寄存器 ID 从 18 开始
    unordered_map<RiscvValue*, int> reg_find_timestamp;
public:
    UnionFind* uf;
    unordered_map<Value*, RiscvValue*> val_to_rval; // Value 到 RiscvValue 的映射
    unordered_map<Value*, RiscvValue*> val_to_reg;  // Value 到寄存器的映射
    unordered_map<RiscvValue*, Value*> reg_to_val;  // 寄存器到 Value 的映射
    unordered_map<Value*, RiscvValue*> val_to_mem;  // Value 到内存位置的映射
    unordered_map<RiscvValue*, Value*> mem_to_val;  // 内存位置到 Value 的映射
    unordered_set<RiscvValue*> reg_used;
    RegAlloc();

    /// @brief 设置 Value 到 RiscvValue 的映射
    /// @param val 
    /// @param rval 
    void set_val_to_rval(Value* val, RiscvValue* rval);

    /// @brief 设置 Value 到寄存器操作数的映射
    /// @param val 
    /// @param reg
    void set_val_to_reg(Value* val, RiscvValue* reg);

    /// @brief 设置 Value 到内存位置的映射
    /// @param val 
    /// @param mem 
    void set_val_to_mem(Value* val, RiscvValue* mem);

    /// @brief 获取寄存器操作数内的 Value
    /// @param reg 
    Value* get_val_in_reg(RiscvValue* reg);

    /// @brief 获取 Value 对应的寄存器操作数
    /// @note 如果该 Value 没有寄存器分配，则返回 nullptr
    /// @param val
    RiscvValue* get_reg_of_val(Value* val);

    /// @brief 获取内存位置对应的 Value
    /// @param mem 
    /// @return 
    Value* get_val_in_mem(RiscvValue* mem);

    /// @brief 获取 Value 对应的内存位置
    /// @param val
    RiscvValue* get_mem_of_val(Value* val);

    /// @brief 更新 Value 到寄存器操作数的映射
    /// @param val 
    /// @param reg 
    /// @param rbb 
    /// @param rinst 
    /// @return 
    bool update_val_to_reg(Value* val, RiscvValue* reg, RiscvBasicBlock* rbb, RiscvInstruction* rinst);

    /// @brief 清理和寄存器操作数及其中包含的 Value 相关的映射信息
    /// @param reg 
    void clear_reg(RiscvValue* reg);

    /// @brief 获取 Value 对应的寄存器操作数
    /// @param val 
    /// @param rbb 
    /// @param rinst 
    /// @param load 
    /// @param specified_reg 
    /// @param direct 
    /// @return 
    RiscvValue* get_reg_op(Value* val, RiscvBasicBlock* rbb, RiscvInstruction* rinst = nullptr, bool load = true, RiscvValue* specified_reg = nullptr, bool direct = true);

    /// @brief 获取 Value 在内存中的位置
    /// @param val 
    /// @param rbb 
    /// @param rinst 
    /// @param direct 
    /// @return 
    RiscvValue* get_mem_op(Value* val, RiscvBasicBlock* rbb = nullptr, RiscvInstruction* rinst = nullptr, bool direct = true);

    /// @brief 获取特定别名的寄存器操作数
    /// @param val 
    /// @param reg_alias 
    /// @param rbb 
    /// @param rinst 
    /// @param direct 
    /// @return 
    RiscvValue* get_specified_reg_op(Value* val, string reg_alias, RiscvBasicBlock* rbb, RiscvInstruction* rinst = nullptr, bool direct = true);

    /// @brief 将寄存器操作数中的 Value 写回内存
    /// @param reg 
    /// @param rbb 
    /// @param rinst 
    /// @return 
    RiscvInstruction* write_back_reg(RiscvValue* reg, RiscvBasicBlock* rbb, RiscvInstruction* rinst = nullptr);

    /// @brief 将 Value 写回内存，释放对应的寄存器资源
    /// @param val 
    /// @param rbb 
    /// @param rinst 
    /// @return 
    RiscvInstruction* write_back_val(Value* val, RiscvBasicBlock* rbb, RiscvInstruction* rinst = nullptr);

    /// @brief 将所有寄存器中的值写回内存
    /// @param rbb
    /// @param rinst
    void write_back_all_regs(RiscvBasicBlock* rbb, RiscvInstruction* rinst = nullptr);
};

#endif // __REGALLOC_HPP__