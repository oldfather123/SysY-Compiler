#pragma once

#include "AsmOperandRegister.h"
#include "AsmRegisterInterface.h"
#include "AsmInstHeaders.h"

#include <set>
#include <map>
#include <utility>   // std::pair
#include <stdexcept> // std::invalid_argument
#include <vector>
#include <typeindex>

namespace Backend
{
    class AsmInstructions
    {
    private:
        AsmInstructions() = default; // 私有构造函数，防止实例化

    public:
        // 获取表示指令写入的寄存器的操作数索引集合
        static std::set<int> getWriteRegId(AsmInstBasic *inst);

        // 获取表示指令写入的虚拟寄存器的操作数索引集合
        static std::set<int> getWriteVRegId(AsmInstBasic *inst);

        // 获取表示指令读取的寄存器的操作数索引集合
        static std::set<int> getReadRegId(AsmInstBasic *inst);

        // 获取表示指令读取的虚拟寄存器的操作数索引集合
        static std::set<int> getReadVRegId(AsmInstBasic *inst);

        // 获取指令中所有虚拟寄存器（读和写）的操作数索引集合
        static std::set<int> getVRegId(AsmInstBasic *inst);

        // 获取指令写入的寄存器集合
        static std::set<AsmOperandRegister *> getWriteRegSet(AsmInstBasic *inst);

        // 获取指令写入的虚拟寄存器索引集合
        static std::set<int> getWriteVRegSet(AsmInstBasic *inst);

        // 获取指令读取的寄存器集合
        static std::set<AsmOperandRegister *> getReadRegSet(AsmInstBasic *inst);

        // 获取指令读取的虚拟寄存器索引集合
        static std::set<int> getReadVRegSet(AsmInstBasic *inst);

        // 检查指令是否是虚拟寄存器之间的移动操作
        static bool isMoveVToV(AsmInstBasic *inst);

        // 获取移动指令中的虚拟寄存器索引
        static std::pair<int, int> getMoveVReg(AsmInstBasic *inst);

        // 获取移动指令中的寄存器
        static std::pair<AsmOperandRegister *, AsmOperandRegister *> getMoveReg(AsmInstMove *instr);

        // 获取指令中匹配给定寄存器的操作数索引集合
        static std::set<int> getInstRegID(AsmInstBasic *instr, AsmOperandRegister *reg);

        // 检查指令是否包含给定的寄存器
        static bool instContainsReg(AsmInstBasic *instr, AsmOperandRegister *reg);

        // 计算指令类型的ID
        static int getCalculateInstCode(AsmInstBasic *instruction);

        // 判断给定操作码是否是可交换的计算指令
        static bool isExchangeableCalculateInst(int opcode);
    };
}
