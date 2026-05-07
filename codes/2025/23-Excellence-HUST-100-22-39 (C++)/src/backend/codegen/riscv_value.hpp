#ifndef __RISCV_VALUE_HPP__
#define __RISCV_VALUE_HPP__

#include <string>
#include "riscv.hpp"

enum RiscvTypeID {
    RVoidTy,                // 无类型
    RIntImmTy, RFloatImmTy, // 立即数值
    RIntRegTy, RFloatRegTy, // 寄存器数值
    RIntMemTy, RFloatMemTy, // 内存数值
    RLabelTy, RFunctionTy   // 基本块和函数
};

class RiscvValue {
public:
    RiscvTypeID rtid;   // RISC-V 类型 ID
    string name;        // 名称：用于全局变量、基本块和函数三个类，其余未用到

    /// @brief 构造函数
    /// @param rtid RISC-V 类型 ID
    /// @param name 名称
    explicit RiscvValue(RiscvTypeID rtid, const string& name = "");
    virtual string print() = 0; // 打印该值的字符串表示

    /// @brief 判断该值是否为寄存器类型（RIntRegTy/RFloatRegTy）
    /// @return 如果是寄存器类型，返回 true；否则返回 false
    bool is_reg();
};

// 常量，即立即数
class RiscvConst: public RiscvValue {
public:
    int int_val;        // 整型常量值
    float float_val;    // 浮点型常量值

    /// @brief 构造函数
    /// @param val 整型常量值
    explicit RiscvConst(int val);

    /// @brief 构造函数
    /// @param val 浮点型常量值
    explicit RiscvConst(float val);
    string print() override;
};

// 整型寄存器操作数
class RiscvIntReg: public RiscvValue {
public:
    Register* reg;  // 对应的寄存器

    /// @brief 构造函数
    /// @param reg 对应的寄存器
    explicit RiscvIntReg(Register* reg);
    string print() override;
};

// 浮点型寄存器操作数
class RiscvFloatReg: public RiscvValue {
public:
    Register* reg;  // 对应的寄存器

    /// @brief 构造函数
    /// @param reg 对应的寄存器
    explicit RiscvFloatReg(Register* reg);
    string print() override;
};

// 整型内存操作数
class RiscvIntMem: public RiscvValue {
public:
    Register* base;     // 基地址寄存器
    string base_name;   // 基地址名称（如果没有寄存器则使用名称）
    int offset;         // 偏移量
    bool is_global;     // 是否为全局变量

    /// @brief 以寄存器形式存在的整型数值
    /// @param base 基地址寄存器
    /// @param offset 偏移量
    /// @param is_global 是否为全局变量
    RiscvIntMem(Register* base, int offset = 0, bool is_global = false);

    /// @brief 以内存的形式存在的整型数值
    /// @param base_name 基地址名称
    /// @param offset 偏移量
    /// @param is_global 是否为全局变量
    RiscvIntMem(const string& base_name, int offset = 0, bool is_global = false);
    string print() override;
};

// 浮点型内存操作数
class RiscvFloatMem: public RiscvValue {
public:
    Register* base;     // 基地址寄存器
    string base_name;   // 基地址名称（如果没有寄存器则使用名称）
    int offset;         // 偏移量
    bool is_global;     // 是否为全局变量

    /// @brief 以寄存器形式存在的浮点数值
    /// @param base 基地址寄存器
    /// @param offset 偏移量
    /// @param is_global 是否为全局变量
    RiscvFloatMem(Register* base, int offset = 0, bool is_global = false);
    
    /// @brief 以内存的形式存在的浮点数值
    /// @param base_name 基地址名称
    /// @param offset 偏移量
    /// @param is_global 是否为全局变量
    RiscvFloatMem(const string& base_name, int offset = 0, bool is_global = false);
    string print() override;
};

// 全局变量
class RiscvGlobalVariable: public RiscvValue {
public:
    bool is_const;      // 是否为常量
    int element_num;    // 元素数量（数组）
    Const* init_val;    // 初始值

    /// @brief 构造函数
    /// @param type 类型
    /// @param name 名称
    /// @param is_const 是否为常量
    /// @param init_val 初始值
    /// @param element_num 数组元素数量，默认为 1，代表不是数组
    RiscvGlobalVariable(RiscvTypeID type, const string& name, bool is_const, Const* init_val, int element_num = 1);
    string print() override;
    string print(bool print_name, Const* init_val); // 打印全局变量定义
};

#endif // __RISCV_VALUE_HPP__