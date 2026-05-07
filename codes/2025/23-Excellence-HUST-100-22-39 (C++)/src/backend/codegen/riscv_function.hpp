#ifndef __RISCVFUNCTION_HPP__
#define __RISCVFUNCTION_HPP__

#include <vector>
#include <unordered_map>
#include "riscv.hpp"
#include "riscv_value.hpp"

class RiscvFunction : public RiscvValue {
public:
    vector<RiscvValue*> args; // 函数参数列表
    vector<RiscvBasicBlock*> rbb_list; // 函数的基本块列表
    int base; // 栈帧基地址
    unordered_map<RiscvValue*, int> stored_environment; // 栈中要保护的地址。该部分需要在函数结束的时候全部恢复
    unordered_map<RiscvValue*, int> args_offset; // 函数使用到的参数（含调用参数、局部变量和返回值）在栈中位置。需满足字对齐（4的倍数），届时将根据该函数的参数情况决定sp下移距离
    RegAlloc* reg_alloc; // 寄存器池，用于寄存器分配

    /// @brief 构造函数
    /// @param name 函数名
    RiscvFunction(const string& name);
    string print() override;
    
    void add_arg(RiscvValue* arg); // 在栈上新增操作数映射
    
    void add_temp_var(RiscvValue* val); // 在栈上新增临时变量映射
    
    void store_array(int byte_size); // 根据元素字节大小，进行 8 字节对齐存储，sp 指针相应减去对齐后的大小
        
    void add_rbb(RiscvBasicBlock* rbb);
        
    bool is_libfunc();
};

#endif // __RISCVFUNCTION_HPP__