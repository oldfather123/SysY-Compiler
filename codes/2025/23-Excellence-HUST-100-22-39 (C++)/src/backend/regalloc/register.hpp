#ifndef __REGISTER_HPP__
#define __REGISTER_HPP__

#include <string>
#include <vector>
#include "riscv.hpp"

// 寄存器类型：整型寄存器、浮点型寄存器，专用寄存器类别针对编译器透明，开发时注意使用
enum RegType { IntReg, FloatReg };

class Register {
public:
    RegType reg_type;
    int reg_id; // 寄存器编号：0~31
    string alias; // 寄存器别名，如 x0, ra, ft0 等等

    /// @brief 构造函数
    /// @note 构造函数通过类型和编号来确定寄存器的别名
    /// @param reg_type 寄存器类型：int/float
    /// @param reg_id 寄存器编号：0~31
    Register(RegType reg_type, int reg_id);
};

extern vector<Register*> regs;              // 寄存器列表，包含所有寄存器
extern vector<RiscvValue*> reg_ops;         // 寄存器操作数
extern vector<RiscvValue*> reg_protected;   // 需要保护的寄存器操作数

/// @brief 通过寄存器别名获取具体的寄存器对象
/// @param alias 寄存器的别名
/// @return 寄存器对象指针，如果未找到则报错，说明寄存器别名格式存在问题
Register* get_reg_named(const string& alias);

/// @brief 通过寄存器别名获取该寄存器的操作数对象
/// @param alias 寄存器的别名
/// @return 寄存器操作数对象指针，如果未找到则报错，说明寄存器别名格式存在问题
RiscvValue* get_reg_op_named(const string& alias);

/// @brief 通过寄存器类型和编号获取该寄存器的操作数对象
/// @param reg_type 寄存器类型
/// @param reg_id 寄存器编号
/// @return 寄存器操作数对象指针，如果未找到则报错，说明寄存器类型或编号格式存在问题
RiscvValue* get_reg_op_with_type_id(RegType reg_type, int reg_id);

#endif // __REGISTER_HPP__