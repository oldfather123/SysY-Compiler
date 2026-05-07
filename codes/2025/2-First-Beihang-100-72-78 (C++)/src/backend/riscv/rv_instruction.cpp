#include "rv_instruction.hpp"

#include <sstream>

#include "log.hpp"
#include "rv_basic_block.hpp"
#include "rv_operand.hpp"

namespace backend::riscv {

const std::map<RVInstType, std::string> RV_INST_TO_STR = {
    {RVInstType::ADD, "add"},
    {RVInstType::SUB, "sub"},
    {RVInstType::SLL, "sll"},
    {RVInstType::SLT, "slt"},
    {RVInstType::SLTU, "sltu"},
    {RVInstType::XOR, "xor"},
    {RVInstType::SRL, "srl"},
    {RVInstType::SRA, "sra"},
    {RVInstType::OR, "or"},
    {RVInstType::AND, "and"},
    {RVInstType::ADDW, "addw"},
    {RVInstType::SUBW, "subw"},
    {RVInstType::SLLW, "sllw"},
    {RVInstType::SRLW, "srlw"},
    {RVInstType::SRAW, "sraw"},
    {RVInstType::MUL, "mul"},
    {RVInstType::MULH, "mulh"},
    {RVInstType::MULW, "mulw"},
    {RVInstType::DIV, "div"},
    {RVInstType::DIVW, "divw"},
    {RVInstType::REM, "rem"},
    {RVInstType::REMW, "remw"},
    {RVInstType::ADDI, "addi"},
    {RVInstType::SLTI, "slti"},
    {RVInstType::SLTIU, "sltiu"},
    {RVInstType::XORI, "xori"},
    {RVInstType::ORI, "ori"},
    {RVInstType::ANDI, "andi"},
    {RVInstType::SLLI, "slli"},
    {RVInstType::SRLI, "srli"},
    {RVInstType::SRAI, "srai"},
    {RVInstType::ADDIW, "addiw"},
    {RVInstType::SLLIW, "slliw"},
    {RVInstType::SRAIW, "sraiw"},
    {RVInstType::SRLIW, "srliw"},
    {RVInstType::LW, "lw"},
    {RVInstType::LH, "lh"},
    {RVInstType::LB, "lb"},
    {RVInstType::LHU, "lhu"},
    {RVInstType::LBU, "lbu"},
    {RVInstType::LD, "ld"},
    {RVInstType::JALR, "jalr"},
    {RVInstType::LI, "li"},
    {RVInstType::MV, "mv"},
    {RVInstType::SEXT_W, "sext.w"},
    {RVInstType::SW, "sw"},
    {RVInstType::SH, "sh"},
    {RVInstType::SB, "sb"},
    {RVInstType::SD, "sd"},
    {RVInstType::FSW, "fsw"},
    {RVInstType::FSD, "fsd"},
    {RVInstType::BEQ, "beq"},
    {RVInstType::BNE, "bne"},
    {RVInstType::BLT, "blt"},
    {RVInstType::BGE, "bge"},
    {RVInstType::BGT, "bgt"},
    {RVInstType::BLE, "ble"},
    {RVInstType::LUI, "lui"},
    {RVInstType::AUIPC, "auipc"},
    {RVInstType::CALL, "jal"},
    {RVInstType::J, "j"},
    {RVInstType::JR, "jr"},
    {RVInstType::RET, "ret"},
    {RVInstType::LLA, "lla"},
    {RVInstType::LA, "la"},
    {RVInstType::FADD, "fadd.s"},
    {RVInstType::FSUB, "fsub.s"},
    {RVInstType::FMUL, "fmul.s"},
    {RVInstType::FDIV, "fdiv.s"},
    {RVInstType::FEQ, "feq.s"},
    {RVInstType::FLT, "flt.s"},
    {RVInstType::FLE, "fle.s"},
    {RVInstType::FMV_S, "fmv.s"},
    {RVInstType::FCVT_S_W, "fcvt.s.w"},
    {RVInstType::FCVT_W_S, "fcvt.w.s"},
    {RVInstType::FLW, "flw"},
    {RVInstType::FLD, "fld"},
    {RVInstType::EPILOGUE, "epilogue"},
    {RVInstType::SH1ADD, "sh1add"},
    {RVInstType::SH2ADD, "sh2add"},
    {RVInstType::SH3ADD, "sh3add"},
};

std::string RVInstruction::to_string() const {
    std::stringstream ss;
    ss << "\t" << RV_INST_TO_STR.at(type);
    if (operands.empty()) {
        return ss.str();
    }
    ss << " ";

    auto op_str = [this](int i) { return operands[i]->to_string(); };

    switch (type) {
        // R-type: op rd, rs1, rs2
        case RVInstType::ADD:
        case RVInstType::SUB:
        case RVInstType::SLL:
        case RVInstType::SLT:
        case RVInstType::SLTU:
        case RVInstType::XOR:
        case RVInstType::SRL:
        case RVInstType::SRA:
        case RVInstType::OR:
        case RVInstType::AND:
        case RVInstType::ADDW:
        case RVInstType::SUBW:
        case RVInstType::SLLW:
        case RVInstType::SRLW:
        case RVInstType::SRAW:
        case RVInstType::MUL:
        case RVInstType::MULH:
        case RVInstType::MULW:
        case RVInstType::DIV:
        case RVInstType::DIVW:
        case RVInstType::REM:
        case RVInstType::REMW:
        // I-type (ALU): op rd, rs1, imm
        case RVInstType::ADDI:
        case RVInstType::SLTI:
        case RVInstType::SLTIU:
        case RVInstType::XORI:
        case RVInstType::ORI:
        case RVInstType::ANDI:
        case RVInstType::SLLI:
        case RVInstType::SRLI:
        case RVInstType::SRAI:
        case RVInstType::ADDIW:
        case RVInstType::SLLIW:
        case RVInstType::SRAIW:
        case RVInstType::SRLIW:
        // JALR: jalr rd, rs1, imm
        case RVInstType::JALR:
            ss << op_str(0) << ", " << op_str(1) << ", " << op_str(2);
            break;

        // Load: lw rd, offset(rs1) -> create(rd, rs1, offset)
        case RVInstType::LW:
        case RVInstType::LH:
        case RVInstType::LB:
        case RVInstType::LHU:
        case RVInstType::LBU:
        case RVInstType::LD:
        case RVInstType::FLW:
        case RVInstType::FLD:
            ss << op_str(0) << ", " << op_str(2) << "(" << op_str(1) << ")";
            break;

        // Store: sw rs2, offset(rs1) -> create(rs1, rs2, offset)
        case RVInstType::SW:
        case RVInstType::SH:
        case RVInstType::SB:
        case RVInstType::SD:
        case RVInstType::FSW:
        case RVInstType::FSD:
            ss << op_str(1) << ", " << op_str(2) << "(" << op_str(0) << ")";
            break;

        // B-type: beq rs1, rs2, offset
        case RVInstType::BEQ:
        case RVInstType::BNE:
        case RVInstType::BLT:
        case RVInstType::BGE:
        case RVInstType::BGT:
        case RVInstType::BLE:
            ss << op_str(0) << ", " << op_str(1) << ", " << op_str(2);
            break;

        // U-type: lui rd, imm
        case RVInstType::LUI:
        case RVInstType::AUIPC:
        // LI, MV, SEXT.W
        case RVInstType::LI:
        case RVInstType::MV:
        case RVInstType::SEXT_W:
            ss << op_str(0) << ", " << op_str(1);
            break;

        // J, JR
        case RVInstType::J:
        case RVInstType::JR:
            ss << op_str(0);
            break;
        case RVInstType::CALL:
            ss << op_str(1);
            break;
        // LLA: lla rd, symbol
        case RVInstType::LLA:
        case RVInstType::LA:
            ss << op_str(0) << ", " << op_str(1);
            break;

        // RET (no operands)
        case RVInstType::RET:
            // RET has no operands, just the instruction name
            break;

        // EPILOGUE (no operands)
        case RVInstType::EPILOGUE:
            // EPILOGUE has no operands, just the instruction name
            break;

        // FP
        case RVInstType::FADD:
        case RVInstType::FSUB:
        case RVInstType::FMUL:
        case RVInstType::FDIV:
        case RVInstType::FEQ:
        case RVInstType::FLT:
        case RVInstType::FLE:
            ss << op_str(0) << ", " << op_str(1) << ", " << op_str(2);
            break;
        case RVInstType::FMV_S:
        case RVInstType::FCVT_S_W:
            ss << op_str(0) << ", " << op_str(1);
            break;
        case RVInstType::FCVT_W_S:
            ss << op_str(0) << ", " << op_str(1) << ", rtz";
            break;
        case RVInstType::SH1ADD:
        case RVInstType::SH2ADD:
        case RVInstType::SH3ADD:
            ss << op_str(0) << ", " << op_str(1) << ", " << op_str(2);
            break;
    }
    return ss.str();
}

RVReg *RVInstruction::get_def() const {
    switch (type) {
        case RVInstType::ADD:
        case RVInstType::SUB:
        case RVInstType::SLL:
        case RVInstType::SLT:
        case RVInstType::SLTU:
        case RVInstType::XOR:
        case RVInstType::SRL:
        case RVInstType::SRA:
        case RVInstType::OR:
        case RVInstType::AND:
        case RVInstType::ADDW:
        case RVInstType::SUBW:
        case RVInstType::SLLW:
        case RVInstType::SRLW:
        case RVInstType::SRAW:
        case RVInstType::MUL:
        case RVInstType::MULH:
        case RVInstType::MULW:
        case RVInstType::DIV:
        case RVInstType::DIVW:
        case RVInstType::REM:
        case RVInstType::REMW:
        case RVInstType::ADDI:
        case RVInstType::SLTI:
        case RVInstType::SLTIU:
        case RVInstType::XORI:
        case RVInstType::ORI:
        case RVInstType::ANDI:
        case RVInstType::SLLI:
        case RVInstType::SRLI:
        case RVInstType::SRAI:
        case RVInstType::ADDIW:
        case RVInstType::SLLIW:
        case RVInstType::SRAIW:
        case RVInstType::SRLIW:
        case RVInstType::LW:
        case RVInstType::LH:
        case RVInstType::LB:
        case RVInstType::LHU:
        case RVInstType::LBU:
        case RVInstType::LD:
        case RVInstType::FLW:
        case RVInstType::FLD:
        case RVInstType::JALR:
        case RVInstType::LUI:
        case RVInstType::AUIPC:
        case RVInstType::CALL:
        case RVInstType::LI:
        case RVInstType::MV:
        case RVInstType::SEXT_W:
        case RVInstType::LLA:
        case RVInstType::LA:
        case RVInstType::FADD:
        case RVInstType::FSUB:
        case RVInstType::FMUL:
        case RVInstType::FDIV:
        case RVInstType::FEQ:
        case RVInstType::FLT:
        case RVInstType::FLE:
        case RVInstType::FMV_S:
        case RVInstType::FCVT_S_W:
        case RVInstType::FCVT_W_S:
        case RVInstType::SH1ADD:
        case RVInstType::SH2ADD:
        case RVInstType::SH3ADD:
            if (!operands.empty() && operands[0]) {
                if (auto *rv_reg = dynamic_cast<RVReg *>(operands[0])) {
                    return rv_reg;
                }
            }
            return nullptr;
        case RVInstType::SW:
        case RVInstType::SH:
        case RVInstType::SB:
        case RVInstType::SD:
        case RVInstType::FSW:
        case RVInstType::FSD:
        case RVInstType::BEQ:
        case RVInstType::BNE:
        case RVInstType::BLT:
        case RVInstType::BGE:
        case RVInstType::BGT:
        case RVInstType::BLE:
        case RVInstType::J:
        case RVInstType::JR:
        case RVInstType::RET:
        case RVInstType::EPILOGUE:
        default:
            return nullptr;
    }
}

std::vector<RVReg *> RVInstruction::get_uses() const {
    std::vector<RVReg *> uses;
    switch (type) {
        // S-type: sw, sh, sb, sd, fsw, fsd
        case RVInstType::SW:
        case RVInstType::SH:
        case RVInstType::SB:
        case RVInstType::SD:
        case RVInstType::FSW:
        case RVInstType::FSD:
            for (auto *op : operands) {
                if (auto *reg = dynamic_cast<RVReg *>(op)) {
                    uses.push_back(reg);
                }
            }
            break;
        // B-type: beq, bne, blt, bge, bgt, ble
        case RVInstType::BEQ:
        case RVInstType::BNE:
        case RVInstType::BLT:
        case RVInstType::BGE:
        case RVInstType::BGT:
        case RVInstType::BLE:
            for (auto *op : operands) {
                if (auto *reg = dynamic_cast<RVReg *>(op)) {
                    uses.push_back(reg);
                }
            }
            break;
        // JALR: rd, rs1, imm，rs1为use
        case RVInstType::JALR:
            if (operands.size() > 1) {
                if (auto *reg = dynamic_cast<RVReg *>(operands[1])) {
                    uses.push_back(reg);
                }
            }
            break;
        case RVInstType::JR:
            if (!operands.empty() && operands[0]) {
                if (auto *reg = dynamic_cast<RVReg *>(operands[0])) {
                    uses.push_back(reg);
                }
            }
            break;
        // JAL: rd, offset，是广义的 use，是用 add_call_used_reg 维护的 use
        case RVInstType::CALL:
            for (size_t i = 2; i < operands.size(); ++i) {
                if (auto *reg = dynamic_cast<RVReg *>(operands[i])) {
                    uses.push_back(reg);
                }
            }
            break;
        // LLA: lla rd, symbol，通常不直接使用寄存器
        case RVInstType::LLA:
        case RVInstType::LA:
            // LLA指令通常不直接使用寄存器，而是使用标签
            break;
        // J, RET, EPILOGUE: 伪指令，可能有特殊use
        case RVInstType::J:
        case RVInstType::RET:
        case RVInstType::EPILOGUE:
            // 通常无use
            break;
        // 其它类型，默认第一个为def，其余为use
        default:
            for (size_t i = 1; i < operands.size(); ++i) {
                if (auto *reg = dynamic_cast<RVReg *>(operands[i])) {
                    uses.push_back(reg);
                }
            }
            break;
    }
    return uses;
}

void RVInstruction::replace_def(RVReg *new_def) {
    auto *old_def = get_def();
    if (!old_def) return;

    bool has_use = false;
    for (auto *reg : get_uses()) {
        if (reg == old_def) {
            has_use = true;
            break;
        }
    }
    if (!has_use) {
        old_def->remove_graph_use(this);
    }
    new_def->add_graph_use(this);

    operands[0] = new_def;
}

void RVInstruction::replace_uses(RVReg *old_reg, RVReg *new_reg) {
    old_reg->remove_graph_use(this);
    new_reg->add_graph_use(this);

    auto *def = get_def();
    bool def_changed = false;
    if (def && def == old_reg) {
        def->add_graph_use(this);
        def_changed = true;
    }
    for (auto &op : operands) {
        if (op == old_reg) {
            op = new_reg;
        }
    }
    if (def_changed) {
        operands[0] = old_reg;
    }
}

void RVInstruction::replace_regs(RVReg *old_reg, RVReg *new_reg) {
    // 更新图使用关系
    old_reg->remove_graph_use(this);
    new_reg->add_graph_use(this);

    // 安全地遍历操作数
    for (auto &op : operands) {
        if (op == old_reg) {
            op = new_reg;
        }
    }
}

void RVInstruction::replace_target_label(RVLabel *new_label) {
    if (!new_label) return;

    switch (type) {
        case RVInstType::J:
            // J指令：operands[0]是目标标签
            operands[0] = new_label;
            break;
        case RVInstType::BEQ:
        case RVInstType::BNE:
        case RVInstType::BLT:
        case RVInstType::BGE:
        case RVInstType::BGT:
        case RVInstType::BLE:
            // 分支指令：operands[2]是目标标签
            operands[2] = new_label;
            break;
        case RVInstType::CALL:
            // CALL指令：operands[1]是目标标签
            operands[1] = new_label;
            break;
        default:
            // 其他指令类型不支持目标标签替换
            break;
    }
}

void RVInstruction::inverse() {
    switch (type) {
        case RVInstType::BEQ:
            type = RVInstType::BNE;
            break;
        case RVInstType::BNE:
            type = RVInstType::BEQ;
            break;
        case RVInstType::BGE:
            type = RVInstType::BLT;
            break;
        case RVInstType::BLT:
            type = RVInstType::BGE;
            break;
        case RVInstType::BLE:
            type = RVInstType::BGT;
            break;
        case RVInstType::BGT:
            type = RVInstType::BLE;
            break;
        default:
            // 非分支指令不支持反转
            break;
    }
}

RVInstruction *RVInstruction::clone() const {
    // 创建新的指令，复制所有操作数
    std::vector<RVOperand *> cloned_operands;
    cloned_operands.reserve(operands.size());
    for (auto *operand : operands) {
        cloned_operands.push_back(operand);
    }
    auto *new_inst = new RVInstruction(type, cloned_operands);
    new_inst->set_parent_block(parent_block);
    new_inst->set_inst_iterator(inst_iterator);
    // 注意：新指令的parent_block和inst_iterator需要重新设置
    // 这些会在指令被添加到基本块时自动设置，所以不需要复制

    return new_inst;
}

void RVInstruction::delete_graph_uses() {
    for (auto *operand : operands) {
        if (dynamic_cast<RVReg *>(operand)) {
            operand->remove_graph_use(this);
        }
    }
}

void RVInstruction::update_graph_uses() {
    for (auto *operand : operands) {
        if (dynamic_cast<RVReg *>(operand)) {
            operand->add_graph_use(this);
        }
    }
}

RVBasicBlock *RVInstruction::get_parent_block() const { return parent_block; }

RVBasicBlock::InstIterator RVInstruction::insert_inst_before_self(RVInstruction *new_inst) {
    // 使用基本块的insert_inst方法在当前指令位置插入新指令
    return parent_block->insert_inst(inst_iterator, new_inst);
}

RVBasicBlock::InstIterator RVInstruction::insert_inst_after_self(RVInstruction *new_inst) {
    // 使用基本块的insert_inst方法在下一个位置插入新指令
    return parent_block->insert_inst(std::next(inst_iterator), new_inst);
}

RVBasicBlock::InstIterator RVInstruction::delete_self() {
    // 从父基本块中移除指令（remove_inst 会自动删除指令并清理相关状态）
    return parent_block->remove_inst(inst_iterator);
}

RVBasicBlock::InstIterator RVInstruction::replace_self(RVInstruction *new_inst) {
    // 在当前指令位置插入新指令
    auto new_it = parent_block->insert_inst(inst_iterator, new_inst);

    // 删除当前指令（这会自动清理相关状态）
    parent_block->remove_inst(inst_iterator);

    return new_it;
}

RVReg *RVInstruction::get_memory_val() const {
    if (!is_memory_ins() || operands.size() < 3) {
        return nullptr;
    }

    // Load指令：val是目标寄存器(rd)，位于operands[0]
    // Store指令：val是源寄存器(rs2)，位于operands[1]
    size_t val_index = is_load_ins() ? 0 : 1;
    return dynamic_cast<RVReg *>(operands[val_index]);
}

RVReg *RVInstruction::get_memory_base() const {
    if (!is_memory_ins() || operands.size() < 3) {
        return nullptr;
    }

    // Load指令：base是基址寄存器(rs1)，位于operands[1]
    // Store指令：base是基址寄存器(rs1)，位于operands[0]
    size_t base_index = is_load_ins() ? 1 : 0;
    return dynamic_cast<RVReg *>(operands[base_index]);
}

RVInstruction::~RVInstruction() {
    // 清理迭代器引用
    clear_inst_iterator();

    // 清理父基本块引用
    parent_block = nullptr;

    // 安全地清理图使用关系
    this->delete_graph_uses();

    // 清理操作数向量
    operands.clear();
}

}  // namespace backend::riscv
