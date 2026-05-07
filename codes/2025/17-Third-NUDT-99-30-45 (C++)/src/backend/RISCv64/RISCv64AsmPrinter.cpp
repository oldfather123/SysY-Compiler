#include "RISCv64AsmPrinter.h"
#include "RISCv64ISel.h"
#include <stdexcept>
#include <sstream>
#include <iostream>
namespace sysy {


RISCv64AsmPrinter::RISCv64AsmPrinter(MachineFunction* mfunc) : MFunc(mfunc) {}

void RISCv64AsmPrinter::run(std::ostream& os, bool debug) {
    OS = &os;

    *OS << ".globl " << MFunc->getName() << "\n";

    for (auto& mbb : MFunc->getBlocks()) {
        printBasicBlock(mbb.get(), debug);
    }
}

void RISCv64AsmPrinter::printBasicBlock(MachineBasicBlock* mbb, bool debug) {
    if (!mbb->getName().empty()) {
        *OS << mbb->getName() << ":\n";
    }
    for (auto& instr : mbb->getInstructions()) {
        printInstruction(instr.get(), debug);
    }
}

void RISCv64AsmPrinter::printInstruction(MachineInstr* instr, bool debug) {
    auto opcode = instr->getOpcode();
    
    if (opcode == RVOpcodes::LABEL) {
        // 标签直接打印，不加缩进
        printOperand(instr->getOperands()[0].get());
        *OS << ":\n";
        return; // 处理完毕，直接返回
    }
    
    // 对于所有非标签指令，先打印缩进
    *OS << "    ";
    
    switch (opcode) {
        case RVOpcodes::ADD:   *OS << "add ";   break; case RVOpcodes::ADDI:  *OS << "addi ";  break;
        case RVOpcodes::ADDW:  *OS << "addw ";  break; case RVOpcodes::ADDIW: *OS << "addiw "; break;
        case RVOpcodes::SUB:   *OS << "sub ";   break; case RVOpcodes::SUBW:  *OS << "subw ";  break;
        case RVOpcodes::MUL:   *OS << "mul ";   break; case RVOpcodes::MULW:  *OS << "mulw ";  break; case RVOpcodes::MULH:  *OS << "mulh ";  break;
        case RVOpcodes::DIV:   *OS << "div ";   break; case RVOpcodes::DIVW:  *OS << "divw ";  break;
        case RVOpcodes::REM:   *OS << "rem ";   break; case RVOpcodes::REMW:  *OS << "remw ";  break;
        case RVOpcodes::XOR:   *OS << "xor ";   break; case RVOpcodes::XORI:  *OS << "xori ";  break;
        case RVOpcodes::OR:    *OS << "or ";    break; case RVOpcodes::ORI:   *OS << "ori ";   break;
        case RVOpcodes::AND:   *OS << "and ";   break; case RVOpcodes::ANDI:  *OS << "andi ";  break;
        case RVOpcodes::SLL:   *OS << "sll ";   break; case RVOpcodes::SLLI:  *OS << "slli ";  break;
        case RVOpcodes::SLLW:  *OS << "sllw ";  break; case RVOpcodes::SLLIW: *OS << "slliw "; break;
        case RVOpcodes::SRL:   *OS << "srl ";   break; case RVOpcodes::SRLI:  *OS << "srli ";  break;
        case RVOpcodes::SRLW:  *OS << "srlw ";  break; case RVOpcodes::SRLIW: *OS << "srliw "; break;
        case RVOpcodes::SRA:   *OS << "sra ";   break; case RVOpcodes::SRAI:  *OS << "srai ";  break;
        case RVOpcodes::SRAW:  *OS << "sraw ";  break; case RVOpcodes::SRAIW: *OS << "sraiw "; break;
        case RVOpcodes::SLT:   *OS << "slt ";   break; case RVOpcodes::SLTI:  *OS << "slti ";  break;
        case RVOpcodes::SLTU:  *OS << "sltu ";  break; case RVOpcodes::SLTIU: *OS << "sltiu "; break;
        case RVOpcodes::LW:    *OS << "lw ";    break; case RVOpcodes::LH:    *OS << "lh ";    break;
        case RVOpcodes::LB:    *OS << "lb ";    break; case RVOpcodes::LWU:   *OS << "lwu ";   break;
        case RVOpcodes::LHU:   *OS << "lhu ";   break; case RVOpcodes::LBU:   *OS << "lbu ";   break;
        case RVOpcodes::SW:    *OS << "sw ";    break; case RVOpcodes::SH:    *OS << "sh ";    break;
        case RVOpcodes::SB:    *OS << "sb ";    break; case RVOpcodes::LD:    *OS << "ld ";    break;
        case RVOpcodes::SD:    *OS << "sd ";    break; case RVOpcodes::FLW:   *OS << "flw ";   break;
        case RVOpcodes::FSW:   *OS << "fsw ";   break; case RVOpcodes::FLD:   *OS << "fld ";   break;
        case RVOpcodes::FSD:   *OS << "fsd ";   break; 
        case RVOpcodes::J:     *OS << "j ";     break; case RVOpcodes::JAL:   *OS << "jal ";   break;
        case RVOpcodes::JALR:  *OS << "jalr ";  break; case RVOpcodes::RET:   *OS << "ret";    break;
        case RVOpcodes::BEQ:   *OS << "beq ";   break; case RVOpcodes::BNE:   *OS << "bne ";   break;
        case RVOpcodes::BLT:   *OS << "blt ";   break; case RVOpcodes::BGE:   *OS << "bge ";   break;
        case RVOpcodes::BLTU:  *OS << "bltu ";  break; case RVOpcodes::BGEU:  *OS << "bgeu ";  break;
        case RVOpcodes::LI:    *OS << "li ";    break; case RVOpcodes::LA:    *OS << "la ";    break;
        case RVOpcodes::MV:    *OS << "mv ";    break; case RVOpcodes::NEG:   *OS << "neg ";   break;
        case RVOpcodes::NEGW:  *OS << "negw ";  break; case RVOpcodes::SEQZ:  *OS << "seqz ";  break;
        case RVOpcodes::SNEZ:  *OS << "snez ";  break; 
        case RVOpcodes::FADD_S:  *OS << "fadd.s ";  break;
        case RVOpcodes::FSUB_S:  *OS << "fsub.s ";  break;
        case RVOpcodes::FMUL_S:  *OS << "fmul.s ";  break;
        case RVOpcodes::FDIV_S:  *OS << "fdiv.s ";  break;
        case RVOpcodes::FMADD_S: *OS << "fmadd.s "; break;
        case RVOpcodes::FNEG_S:  *OS << "fneg.s ";  break;
        case RVOpcodes::FEQ_S:   *OS << "feq.s ";   break;
        case RVOpcodes::FLT_S:   *OS << "flt.s ";   break;
        case RVOpcodes::FLE_S:   *OS << "fle.s ";   break;
        case RVOpcodes::FCVT_S_W: *OS << "fcvt.s.w "; break;
        case RVOpcodes::FCVT_W_S: *OS << "fcvt.w.s "; break;
        case RVOpcodes::FCVT_W_S_RTZ: *OS << "fcvt.w.s "; break;
        case RVOpcodes::FMV_S:    *OS << "fmv.s ";    break;
        case RVOpcodes::FMV_W_X:  *OS << "fmv.w.x ";  break;
        case RVOpcodes::FMV_X_W:  *OS << "fmv.x.w ";  break;
        case RVOpcodes::FSRMI:   *OS << "fsrmi ";   break;
        case RVOpcodes::CALL: { // 为CALL指令添加特殊处理逻辑
            *OS << "call ";
            // 遍历所有操作数，只寻找并打印函数名标签
            for (const auto& op : instr->getOperands()) {
                if (op->getKind() == MachineOperand::KIND_LABEL) {
                    printOperand(op.get());
                    break; // 找到标签后即可退出
                }
            }
            *OS << "\n";
            return; // 处理完毕，直接返回，不再执行后续的通用操作数打印
        }
        case RVOpcodes::LABEL:
            break;
        case RVOpcodes::FRAME_LOAD_W:
            // It should have been eliminated by RegAlloc
            if (!debug) throw std::runtime_error("FRAME pseudo-instruction not eliminated before AsmPrinter");
            *OS << "frame_load_w ";  break;
        case RVOpcodes::FRAME_LOAD_D:
            // It should have been eliminated by RegAlloc
            if (!debug) throw std::runtime_error("FRAME pseudo-instruction not eliminated before AsmPrinter");
            *OS << "frame_load_d ";  break;
        case RVOpcodes::FRAME_STORE_W:
            // It should have been eliminated by RegAlloc
            if (!debug) throw std::runtime_error("FRAME pseudo-instruction not eliminated before AsmPrinter");
            *OS << "frame_store_w ";  break;
        case RVOpcodes::FRAME_STORE_D:
            // It should have been eliminated by RegAlloc
            if (!debug) throw std::runtime_error("FRAME pseudo-instruction not eliminated before AsmPrinter");
            *OS << "frame_store_d ";  break;
        case RVOpcodes::FRAME_ADDR:
            // It should have been eliminated by RegAlloc
            if (!debug) throw std::runtime_error("FRAME pseudo-instruction not eliminated before AsmPrinter");
            *OS << "frame_addr "; break;
        case RVOpcodes::FRAME_LOAD_F:
            if (!debug) throw std::runtime_error("FRAME_LOAD_F not eliminated before AsmPrinter");
            *OS << "frame_load_f ";  break;
        case RVOpcodes::FRAME_STORE_F:
            if (!debug) throw std::runtime_error("FRAME_STORE_F not eliminated before AsmPrinter");
            *OS << "frame_store_f ";  break;
        case RVOpcodes::PSEUDO_KEEPALIVE:
            if (!debug) throw std::runtime_error("PSEUDO_KEEPALIVE not eliminated before AsmPrinter");
            *OS << "keepalive ";  break;
        default: 
            throw std::runtime_error("Unknown opcode in AsmPrinter");
    }

    const auto& operands = instr->getOperands();
    if (!operands.empty()) {
        if (isMemoryOp(opcode)) {
            printOperand(operands[0].get());
            *OS << ", ";
            printOperand(operands[1].get());
        } else {
            for (size_t i = 0; i < operands.size(); ++i) {
                printOperand(operands[i].get());
                if (i < operands.size() - 1) {
                    *OS << ", ";
                }
            }
        }
    }
    *OS << "\n";
}

void RISCv64AsmPrinter::printOperand(MachineOperand* op) {
    if (!op) return;
    switch(op->getKind()) {
        case MachineOperand::KIND_REG: {
            auto reg_op = static_cast<RegOperand*>(op);
            if (reg_op->isVirtual()) {
                *OS << "%vreg" << reg_op->getVRegNum();
            } else {
                *OS << regToString(reg_op->getPReg());
            }
            break;
        }
        case MachineOperand::KIND_IMM:
            *OS << static_cast<ImmOperand*>(op)->getValue();
            break;
        case MachineOperand::KIND_LABEL:
            *OS << static_cast<LabelOperand*>(op)->getName();
            break;
        case MachineOperand::KIND_MEM: {
            auto mem_op = static_cast<MemOperand*>(op);
            printOperand(mem_op->getOffset());
            *OS << "(";
            printOperand(mem_op->getBase());
            *OS << ")";
            break;
        }
    }
}

std::string RISCv64AsmPrinter::regToString(PhysicalReg reg) const {
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

std::string RISCv64AsmPrinter::formatInstr(const MachineInstr* instr) {
    if (!instr) return "(null instr)";
    
    // 使用 stringstream 作为临时的输出目标
    std::stringstream ss;
    
    // 关键: 临时将类成员 'OS' 指向我们的 stringstream
    std::ostream* old_os = this->OS;
    this->OS = &ss;
    
    // 修正: 调用正确的内部打印函数 printMachineInstr
    printInstruction(const_cast<MachineInstr*>(instr), false);
    
    // 恢复旧的 ostream 指针
    this->OS = old_os;
    
    // 获取stringstream的内容并做一些清理
    std::string result = ss.str();
    size_t endpos = result.find_last_not_of(" \t\n\r");
    if (std::string::npos != endpos) {
        result = result.substr(0, endpos + 1);
    }
    
    return result;
}

} // namespace sysy