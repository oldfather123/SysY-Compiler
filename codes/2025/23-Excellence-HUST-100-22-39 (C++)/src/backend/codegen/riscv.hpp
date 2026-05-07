#ifndef __RISCV_HPP__
#define __RISCV_HPP__

#include "ir.hpp"

// riscv_value.hpp
class RiscvValue;
class RiscvConst;
class RiscvIntReg;
class RiscvFloatReg;
class RiscvIntMem;
class RiscvFloatMem;
class RiscvGlobalVariable;

// register.hpp
class Register;
class RegPool;
class RegAlloc;

// riscv_instruction.hpp
class RiscvInstruction;
class RiscvBinaryInst;
class RiscvUnaryInst;
class RiscvMoveInst;
class RiscvPushInst;
class RiscvPopInst;
class RiscvCallInst;
class RiscvReturnInst;
class RiscvLoadInst;
class RiscvStoreInst;
class RiscvICmpInst;
class RiscvICmpSetInst;
class RiscvFCmpInst;
class RiscvFpToSiInst;
class RiscvSiToFpInst;
class RiscvLoadAddrInst;
class RiscvBranchInst;

// riscv_basic_block.hpp
class RiscvBasicBlock;

// riscv_function.hpp
class RiscvFunction;

// riscv_generator.hpp
class RiscvGenerator;

const int VARIABLE_ALIGN_BYTE = 8;

/// @brief 计算类型的大小（字节数）
/// @param type 需要计算的类型
/// @return 返回类型的大小（字节数）
int calculate_type_size(Type* type);

/// @brief 获取类型最终元素的数据类型，如数组最内层元素
/// @param type 将要查找的类型
/// @return 返回最终的数值类型
Type* get_element_type(Type* type);

#endif // __RISCV_HPP__