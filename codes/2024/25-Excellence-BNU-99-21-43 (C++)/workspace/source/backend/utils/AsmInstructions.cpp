#include "AsmInstructions.h"

namespace Backend
{
    std::set<int> AsmInstructions::getWriteRegId(AsmInstBasic *inst)
    {
        // 对于存储指令，检查第一个操作数（目标地址）是否是寄存器
        if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstStore)
        {
            if (inst->getOperand(1)->isRegister())
                return {1};
            return {};
        }

        // 处理跳转和返回指令，这些指令不写入任何寄存器
        if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstJump ||
            inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstReturn)
        {
            return {};
        }

        // 对于其他指令，检查第一个操作数（目标寄存器）是否是寄存器容器
        AsmOperandBasic *op = inst->getOperand(0);
        if (op != nullptr && op->isRegisterReplaceable())
        {
            return {0};
        }

        return {};
    }

    // 获取表示指令写入的虚拟寄存器的操作数索引集合
    std::set<int> AsmInstructions::getWriteVRegId(AsmInstBasic *inst)
    {
        std::set<int> result;
        std::set<int> regId = getWriteRegId(inst);
        for (int x : regId)
        {
            AsmRegisterInterface *regInterface = dynamic_cast<AsmRegisterInterface *>(inst->getOperand(x));
            auto *reg = regInterface->getRegister();
            if (reg->isVirtualRegister())
                result.insert(x);
        }
        return result;
    }

    // 获取表示指令读取的寄存器的操作数索引集合
    std::set<int> AsmInstructions::getReadRegId(AsmInstBasic *inst)
    {
        std::set<int> result;
        for (int i = 0; i < 3; ++i)
        {
            AsmOperandBasic *op = inst->getOperand(i);
            if (op != nullptr && op->isRegisterReplaceable())
            {
                // 存储指令读取第一个和第二个操作数（源地址和数据），其他指令读取第二和第三个操作数
                bool flag = (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstStore) ? (i == 0 || (i == 1 && !inst->getOperand(i)->isRegister())) : (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstJump || inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstReturn || (i > 0));
                if (flag)
                {
                    result.insert(i);
                }
            }
        }
        return result;
    }

    // 获取表示指令读取的虚拟寄存器的操作数索引集合
    std::set<int> AsmInstructions::getReadVRegId(AsmInstBasic *inst)
    {
        std::set<int> result;
        std::set<int> regId = getReadRegId(inst);
        for (int x : regId)
        {
            AsmRegisterInterface *regInterface = dynamic_cast<AsmRegisterInterface *>(inst->getOperand(x));
            auto *reg = regInterface->getRegister();
            if (reg->isVirtualRegister())
                result.insert(x);
        }
        return result;
    }

    // 获取指令中所有虚拟寄存器（读和写）的操作数索引集合
    std::set<int> AsmInstructions::getVRegId(AsmInstBasic *inst)
    {
        std::set<int> result;
        auto readVRegId = getReadVRegId(inst);
        auto writeVRegId = getWriteVRegId(inst);
        result.insert(readVRegId.begin(), readVRegId.end());
        result.insert(writeVRegId.begin(), writeVRegId.end());
        return result;
    }

    // 获取指令写入的寄存器集合
    std::set<AsmOperandRegister *> AsmInstructions::getWriteRegSet(AsmInstBasic *inst)
    {
        std::set<AsmOperandRegister *> result;
        std::set<int> regId = getWriteRegId(inst);
        for (int i : regId)
        {
            AsmRegisterInterface *op = dynamic_cast<AsmRegisterInterface *>(inst->getOperand(i));
            result.insert(op->getRegister());
        }

        // 如果是调用指令，包含返回寄存器
        if (auto *call = dynamic_cast<AsmInstCall *>(inst))
        {
            if (AsmOperandRegister *retReg = call->getReturnRegister())
            {
                result.insert(retReg);
            }
        }
        return result;
    }

    // 获取指令写入的虚拟寄存器索引集合
    std::set<int> AsmInstructions::getWriteVRegSet(AsmInstBasic *inst)
    {
        std::set<int> result;
        std::set<int> writeVRegId = getWriteVRegId(inst);
        for (int i : writeVRegId)
        {
            AsmRegisterInterface *op = dynamic_cast<AsmRegisterInterface *>(inst->getOperand(i));
            result.insert(op->getRegister()->getAbsRegIndex());
        }
        return result;
    }

    // 获取指令读取的寄存器集合
    std::set<AsmOperandRegister *> AsmInstructions::getReadRegSet(AsmInstBasic *inst)
    {
        std::set<AsmOperandRegister *> result;
        std::set<int> readRegId = getReadRegId(inst);
        for (int i : readRegId)
        {
            AsmOperandBasic *op_tmp = inst->getOperand(i);
            AsmRegisterInterface *op = dynamic_cast<AsmRegisterInterface *>(op_tmp);
            result.insert(op->getRegister());
        }

        // 如果是调用指令，包含参数寄存器
        if (auto *call = dynamic_cast<AsmInstCall *>(inst))
        {
            std::vector<AsmOperandRegister *> paramRegList = call->getParamRegList();
            result.insert(paramRegList.begin(), paramRegList.end());
        }
        return result;
    }

    // 获取指令读取的虚拟寄存器索引集合
    std::set<int> AsmInstructions::getReadVRegSet(AsmInstBasic *inst)
    {
        std::set<int> result;
        std::set<int> readVRegId = getReadVRegId(inst);
        for (int i : readVRegId)
        {
            AsmRegisterInterface *op = dynamic_cast<AsmRegisterInterface *>(inst->getOperand(i));
            result.insert(op->getRegister()->getAbsRegIndex());
        }
        return result;
    }

    // 检查指令是否是虚拟寄存器之间的移动操作
    bool AsmInstructions::isMoveVToV(AsmInstBasic *inst)
    {
        return dynamic_cast<AsmInstMove *>(inst) &&
               dynamic_cast<AsmRegisterInterface *>(inst->getOperand(0))->getRegister()->isVirtualRegister() &&
               dynamic_cast<AsmRegisterInterface *>(inst->getOperand(1))->getRegister()->isVirtualRegister();
    }

    // 获取移动指令中的虚拟寄存器索引
    std::pair<int, int> AsmInstructions::getMoveVReg(AsmInstBasic *inst)
    {
        if (!isMoveVToV(inst))
        {
            throw std::invalid_argument("Instruction is not a move between virtual registers");
        }
        auto writeVRegSet = getWriteVRegSet(inst);
        auto readVRegSet = getReadVRegSet(inst);
        return {*writeVRegSet.begin(), *readVRegSet.begin()};
    }

    // 获取移动指令中的寄存器
    std::pair<AsmOperandRegister *, AsmOperandRegister *> AsmInstructions::getMoveReg(AsmInstMove *instr)
    {
        auto writeRegSet = getWriteRegSet(instr);
        auto readRegSet = getReadRegSet(instr);

        // 确保 writeRegSet 和 readRegSet 非空
        if (writeRegSet.empty() || readRegSet.empty())
        {
            throw std::runtime_error("Register sets are empty");
        }

        // return {*writeRegSet.begin(), *readRegSet.begin()};
        return {*writeRegSet.begin(), *readRegSet.begin()};
    }
    // 获取指令中匹配给定寄存器的操作数索引集合
    std::set<int> AsmInstructions::getInstRegID(AsmInstBasic *instr, AsmOperandRegister *reg)
    {
        std::set<int> res;
        for (int i = 0; i < 3; ++i)
        {
            if (dynamic_cast<AsmRegisterInterface *>(instr->getOperand(i)) &&
                dynamic_cast<AsmRegisterInterface *>(instr->getOperand(i))->getRegister() == reg)
            {
                res.insert(i);
            }
        }
        return res;
    }

    // 检查指令是否包含给定的寄存器
    bool AsmInstructions::instContainsReg(AsmInstBasic *instr, AsmOperandRegister *reg)
    {
        return getReadRegSet(instr).count(reg) > 0 || getWriteRegSet(instr).count(reg) > 0;
    }

    // 计算指令类型的ID
    int AsmInstructions::getCalculateInstCode(AsmInstBasic *instruction)
    {
        static std::map<std::type_index, int> CalculateInstClassID{
            {typeid(AsmInstAdd), 0},
            {typeid(AsmInstAnd), 1},
            {typeid(AsmInstFloatDivide), 2},
            {typeid(AsmInstMax), 3},
            {typeid(AsmInstMin), 4},
            {typeid(AsmInstMul), 5},
            {typeid(AsmInstShiftLeft), 6},
            {typeid(AsmInstShiftRightArithmetic), 7},
            {typeid(AsmInstShiftRightLogical), 8},
            {typeid(AsmInstSignedIntDivide), 9},
            {typeid(AsmInstSignedIntRemainder), 10}};

        auto it = CalculateInstClassID.find(typeid(*instruction));
        if (it != CalculateInstClassID.end())
        {
            return it->second;
        }
        else if (instruction->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstShiftLeftAdd)
        {
            return 12 + static_cast<AsmInstShiftLeftAdd *>(instruction)->getShiftLength();
        }
        return -1;
    }

    // 判断给定操作码是否是可交换的计算指令
    bool AsmInstructions::isExchangeableCalculateInst(int opcode)
    {
        return (opcode == 0 || opcode == 1 || opcode == 3 || opcode == 4 || opcode == 5);
    }
}