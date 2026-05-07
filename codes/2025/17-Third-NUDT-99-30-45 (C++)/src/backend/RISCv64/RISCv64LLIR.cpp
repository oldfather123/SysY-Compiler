#include "RISCv64LLIR.h"
#include "RISCv64Info.h"
#include <vector>
#include <iostream> // 用于 std::ostream 和 std::cerr
#include <string>   // 用于 std::string

namespace sysy {

// 辅助函数：将 PhysicalReg 枚举转换为可读的字符串
std::string regToString(PhysicalReg reg) {
    switch (reg) {
        case PhysicalReg::ZERO: return "x0";  case PhysicalReg::RA: return "ra";
        case PhysicalReg::SP: return "sp";    case PhysicalReg::GP: return "gp";
        case PhysicalReg::TP: return "tp";    case PhysicalReg::T0: return "t0";
        case PhysicalReg::T1: return "t1";    case PhysicalReg::T2: return "t2";
        case PhysicalReg::S0: return "s0";    case PhysicalReg::S1: return "s1";
        case PhysicalReg::A0: return "a0";    case PhysicalReg::A1: return "a1";
        case PhysicalReg::A2: return "a2";    case PhysicalReg::A3: return "a3";
        case PhysicalReg::A4: return "a4";    case PhysicalReg::A5: return "a5";
        case PhysicalReg::A6: return "a6";    case PhysicalReg::A7: return "a7";
        case PhysicalReg::S2: return "s2";    case PhysicalReg::S3: return "s3";
        case PhysicalReg::S4: return "s4";    case PhysicalReg::S5: return "s5";
        case PhysicalReg::S6: return "s6";    case PhysicalReg::S7: return "s7";
        case PhysicalReg::S8: return "s8";    case PhysicalReg::S9: return "s9";
        case PhysicalReg::S10: return "s10";  case PhysicalReg::S11: return "s11";
        case PhysicalReg::T3: return "t3";    case PhysicalReg::T4: return "t4";
        case PhysicalReg::T5: return "t5";    case PhysicalReg::T6: return "t6";
        case PhysicalReg::F0: return "f0";    case PhysicalReg::F1: return "f1";
        case PhysicalReg::F2: return "f2";    case PhysicalReg::F3: return "f3";
        case PhysicalReg::F4: return "f4";    case PhysicalReg::F5: return "f5";
        case PhysicalReg::F6: return "f6";    case PhysicalReg::F7: return "f7";
        case PhysicalReg::F8: return "f8";    case PhysicalReg::F9: return "f9";
        case PhysicalReg::F10: return "f10";  case PhysicalReg::F11: return "f11";
        case PhysicalReg::F12: return "f12";  case PhysicalReg::F13: return "f13";
        case PhysicalReg::F14: return "f14";  case PhysicalReg::F15: return "f15";
        case PhysicalReg::F16: return "f16";  case PhysicalReg::F17: return "f17";
        case PhysicalReg::F18: return "f18";  case PhysicalReg::F19: return "f19";
        case PhysicalReg::F20: return "f20";  case PhysicalReg::F21: return "f21";
        case PhysicalReg::F22: return "f22";  case PhysicalReg::F23: return "f23";
        case PhysicalReg::F24: return "f24";  case PhysicalReg::F25: return "f25";
        case PhysicalReg::F26: return "f26";  case PhysicalReg::F27: return "f27";
        case PhysicalReg::F28: return "f28";  case PhysicalReg::F29: return "f29";
        case PhysicalReg::F30: return "f30";  case PhysicalReg::F31: return "f31";
        default: return "UNKNOWN_REG";
    }
}

// 打印栈帧信息的完整实现
void MachineFunction::dumpStackFrameInfo(std::ostream& os) const {
    const StackFrameInfo& info = frame_info;

    os << "--- Stack Frame Info for function '" << getName() << "' ---\n";

    // 打印尺寸信息
    os << "  Sizes:\n";
    os << "    Total Size:          " << info.total_size << " bytes\n";
    os << "    Locals Size:         " << info.locals_size << " bytes\n";
    os << "    Spill Size:          " << info.spill_size << " bytes\n";
    os << "    Callee-Saved Size:   " << info.callee_saved_size << " bytes\n";
    os << "\n";

    // 打印 Alloca 变量的偏移量
    os << "  Alloca Offsets (vreg -> offset from FP):\n";
    if (info.alloca_offsets.empty()) {
        os << "    (None)\n";
    } else {
        for (const auto& pair : info.alloca_offsets) {
            os << "    %vreg" << pair.first << " -> " << pair.second << "\n";
        }
    }
    os << "\n";

    // 打印溢出变量的偏移量
    os << "  Spill Offsets (vreg -> offset from FP):\n";
    if (info.spill_offsets.empty()) {
        os << "    (None)\n";
    } else {
        for (const auto& pair : info.spill_offsets) {
            os << "    %vreg" << pair.first << " -> " << pair.second << "\n";
        }
    }
    os << "\n";

    // 打印使用的被调用者保存寄存器
    os << "  Used Callee-Saved Registers:\n";
    if (info.used_callee_saved_regs.empty()) {
        os << "    (None)\n";
    } else {
        os << "    { ";
        for (const auto& reg : info.used_callee_saved_regs) {
            os << regToString(reg) << " ";
        }
        os << "}\n";
    }
    os << "\n";

    // 打印需要保存/恢复的被调用者保存寄存器 (有序)
    os << "  Callee-Saved Registers to Store/Restore:\n";
    if (info.callee_saved_regs_to_store.empty()) {
        os << "    (None)\n";
    } else {
        os << "    [ ";
        for (const auto& reg : info.callee_saved_regs_to_store) {
            os << regToString(reg) << " ";
        }
        os << "]\n";
    }
    os << "\n";

    // 打印最终的寄存器分配结果
    os << "  Final Register Allocation Map (vreg -> preg):\n";
    if (info.vreg_to_preg_map.empty()) {
        os << "    (None)\n";
    } else {
        for (const auto& pair : info.vreg_to_preg_map) {
            os << "    %vreg" << pair.first << " -> " << regToString(pair.second) << "\n";
        }
    }

    os << "---------------------------------------------------\n";
}

/**
 * @brief （为紧急溢出模式添加）将指令中所有对特定虚拟寄存器的引用替换为指定的物理寄存器。
 */
void MachineInstr::replaceVRegWithPReg(unsigned old_vreg, PhysicalReg preg) {
    for (auto& op : operands) {
        if (op->getKind() == MachineOperand::KIND_REG) {
            auto reg_op = static_cast<RegOperand*>(op.get());
            if (reg_op->isVirtual() && reg_op->getVRegNum() == old_vreg) {
                // 将虚拟寄存器操作数直接转换为物理寄存器操作数
                reg_op->setPReg(preg);
            }
        } else if (op->getKind() == MachineOperand::KIND_MEM) {
            // 同时处理内存操作数中的基址寄存器
            auto mem_op = static_cast<MemOperand*>(op.get());
            auto base_reg = mem_op->getBase();
            if (base_reg->isVirtual() && base_reg->getVRegNum() == old_vreg) {
                base_reg->setPReg(preg);
            }
        }
    }
}

/**
 * @brief （为常规溢出模式添加）根据提供的映射表，重映射指令中的虚拟寄存器。
 * 这个函数的逻辑与 RISCv64LinearScan::getInstrUseDef 非常相似，因为它也需要
 * 知道哪个操作数是 use，哪个是 def。
 */
void MachineInstr::remapVRegs(const std::map<unsigned, unsigned>& use_remap, const std::map<unsigned, unsigned>& def_remap) {
    auto opcode = getOpcode();

    // 辅助lambda，用于替换寄存器操作数
    auto remap_reg_op = [](RegOperand* reg_op, const std::map<unsigned, unsigned>& remap) {
        if (reg_op->isVirtual() && remap.count(reg_op->getVRegNum())) {
            reg_op->setVRegNum(remap.at(reg_op->getVRegNum()));
        }
    };
    
    // 根据指令信息表（op_info）来确定 use 和 def
    if (op_info.count(opcode)) {
        const auto& info = op_info.at(opcode);
        // 替换 def 操作数
        for (int idx : info.first) {
            if (idx < operands.size() && operands[idx]->getKind() == MachineOperand::KIND_REG) {
                remap_reg_op(static_cast<RegOperand*>(operands[idx].get()), def_remap);
            }
        }
        // 替换 use 操作数
        for (int idx : info.second) {
            if (idx < operands.size()) {
                if (operands[idx]->getKind() == MachineOperand::KIND_REG) {
                    remap_reg_op(static_cast<RegOperand*>(operands[idx].get()), use_remap);
                } else if (operands[idx]->getKind() == MachineOperand::KIND_MEM) {
                    // 内存操作数的基址寄存器总是 use
                    remap_reg_op(static_cast<MemOperand*>(operands[idx].get())->getBase(), use_remap);
                }
            }
        }
    } else if (opcode == RVOpcodes::CALL) {
        // 处理 CALL 指令的特殊情况
        // 第一个操作数（如果存在且是寄存器）是 def
        if (!operands.empty() && operands[0]->getKind() == MachineOperand::KIND_REG) {
            remap_reg_op(static_cast<RegOperand*>(operands[0].get()), def_remap);
        }
        // 其余寄存器操作数是 use
        for (size_t i = 1; i < operands.size(); ++i) {
            if (operands[i]->getKind() == MachineOperand::KIND_REG) {
                remap_reg_op(static_cast<RegOperand*>(operands[i].get()), use_remap);
            }
        }
    }
}

}
