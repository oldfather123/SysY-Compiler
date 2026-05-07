#include "machine_instruction.h"

// 定义静态成员变量
RiscV64InstructionConstructor RiscV64InstructionConstructor::instance;

// 定义全局变量
RiscV64InstructionConstructor *rvconstructor = RiscV64InstructionConstructor::GetConstructor();