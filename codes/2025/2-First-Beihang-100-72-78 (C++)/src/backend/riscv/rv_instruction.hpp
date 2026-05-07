#ifndef GEECEECEE_RV_INSTRUCTION_HPP
#define GEECEECEE_RV_INSTRUCTION_HPP

#pragma once
#include <list>
#include <map>
#include <string>
#include <vector>

#include "rv_basic_block.hpp"
#include "rv_operand.hpp"

namespace backend::riscv {

class RVInstruction;
class RVBasicBlock;

// 前向声明指令列表类型（这些类型在rv_basic_block.hpp中定义）
// using InstList = std::list<RVInstruction*>;
// using InstIterator = InstList::iterator;

/* --- 指令枚举，与字符串映射 --- */
enum class RVInstType {
    // R-type
    ADD,
    SUB,
    SLL,
    SLT,
    SLTU,
    XOR,
    SRL,
    SRA,
    OR,
    AND,
    ADDW,
    SUBW,
    SLLW,
    SRLW,
    SRAW,
    MUL,
    MULH,
    MULW,
    DIV,
    DIVW,
    REM,
    REMW,
    // I-type
    ADDI,
    SLTI,
    SLTIU,
    XORI,
    ORI,
    ANDI,
    SLLI,
    SRLI,
    SRAI,
    ADDIW,
    SLLIW,
    SRAIW,
    SRLIW,
    LW,
    LH,
    LB,
    LHU,
    LBU,
    LD,
    JALR,
    LI,
    MV,
    SEXT_W,
    // S-type
    SW,
    SH,
    SB,
    SD,
    FSW,
    FSD,
    SH1ADD,
    SH2ADD,
    SH3ADD,
    // B-type
    BEQ,
    BNE,
    BLT,
    BGE,
    BGT,
    BLE,
    // U-type
    LUI,
    AUIPC,
    // J-type
    CALL,
    J,
    JR,
    RET,
    // 伪指令
    LLA,
    LA,
    // Floating-point
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FEQ,
    FLT,
    FLE,
    FMV_S,
    FCVT_S_W,
    FCVT_W_S,
    FLW,
    FLD,
    // 伪指令等可再补充
    EPILOGUE,  // 栈帧恢复指令
};

extern const std::map<RVInstType, std::string> RV_INST_TO_STR;

/* --- 指令基类 --- */
class RVInstruction {
public:
    virtual ~RVInstruction();
    RVInstType get_type() const { return type; }
    const auto &get_operands() const { return operands; }
    virtual std::string to_string() const;  // 形如：add x3, x1, x2

    RVReg *get_def() const;
    std::vector<RVReg *> get_uses() const;
    void add_call_used_reg(RVPhyReg *phy_reg) { operands.push_back(phy_reg); }  // 只有 JAL 指令能用

    void replace_def(RVReg *new_def);
    void replace_uses(RVReg *old_reg, RVReg *new_reg);
    void replace_regs(RVReg *old_reg, RVReg *new_reg);

    // 替换跳转和分支指令的目标标签
    void replace_target_label(RVLabel *new_label);

    // 反转分支指令的条件
    void inverse();

    // 克隆指令
    RVInstruction *clone() const;

    // 图使用关系管理
    void delete_graph_uses();  // 取消对所有操作数的使用关系
    void update_graph_uses();  // 建立对所有操作数的使用关系

    // 指令在基本块中的位置管理
    void set_inst_iterator(RVBasicBlock::InstIterator it) { inst_iterator = it; }
    RVBasicBlock::InstIterator get_inst_iterator() const { return inst_iterator; }
    void clear_inst_iterator() { inst_iterator = RVBasicBlock::InstIterator(); }

    // 获取指令所属的基本块
    RVBasicBlock *get_parent_block() const;
    void set_parent_block(RVBasicBlock *block) { parent_block = block; }

    RVBasicBlock::InstIterator insert_inst_before_self(RVInstruction *new_inst);
    RVBasicBlock::InstIterator insert_inst_after_self(RVInstruction *new_inst);
    RVBasicBlock::InstIterator delete_self();
    RVBasicBlock::InstIterator replace_self(RVInstruction *new_inst);

    // 判断该指令是否为 64 位整数操作
    bool is_64_ins() const {
        switch (type) {
            case RVInstType::ADD:
            case RVInstType::SUB:
            case RVInstType::MUL:
            case RVInstType::ADDI:
            case RVInstType::SLT:
            case RVInstType::SLTI:
            case RVInstType::SLTIU:
            case RVInstType::SLTU:
            case RVInstType::REM:
                return true;
            default:
                return false;
        }
    }

    // 判断该指令是否为分支指令
    bool is_branch_ins() const {
        switch (type) {
            case RVInstType::BEQ:
            case RVInstType::BNE:
            case RVInstType::BLT:
            case RVInstType::BGE:
            case RVInstType::BGT:
            case RVInstType::BLE:
                return true;
            default:
                return false;
        }
    }

    // 判断该指令是否为跳转指令
    bool is_jump_ins() const {
        switch (type) {
            case RVInstType::J:
            case RVInstType::CALL:
            case RVInstType::JR:
            case RVInstType::RET:
                return true;
            default:
                return false;
        }
    }

    // 判断该指令是否为加载指令
    bool is_load_ins() const {
        switch (type) {
            case RVInstType::LW:
            case RVInstType::LH:
            case RVInstType::LB:
            case RVInstType::LHU:
            case RVInstType::LBU:
            case RVInstType::LD:
            case RVInstType::FLW:
            case RVInstType::FLD:
                return true;
            default:
                return false;
        }
    }

    // 判断该指令是否为存储指令
    bool is_store_ins() const {
        switch (type) {
            case RVInstType::SW:
            case RVInstType::SH:
            case RVInstType::SB:
            case RVInstType::SD:
            case RVInstType::FSW:
            case RVInstType::FSD:
                return true;
            default:
                return false;
        }
    }

    bool is_memory_ins() const { return is_load_ins() || is_store_ins(); }

    // 获取内存访问指令的val寄存器（load的目标寄存器或store的源寄存器）
    RVReg *get_memory_val() const;

    // 获取内存访问指令的base寄存器（基址寄存器）
    RVReg *get_memory_base() const;

    // 判断该指令是否为移动指令
    bool is_move_ins() const {
        switch (type) {
            case RVInstType::MV:
            case RVInstType::FMV_S:
                return true;
            default:
                return false;
        }
    }

    // 判断该指令是否为U型指令
    bool is_u_type_ins() const {
        switch (type) {
            case RVInstType::LUI:
            case RVInstType::AUIPC:
                return true;
            default:
                return false;
        }
    }

    // 判断该指令是否为ADDI/ADDIW指令
    bool is_addi_ins() const {
        switch (type) {
            case RVInstType::ADDI:
            case RVInstType::ADDIW:
                return true;
            default:
                return false;
        }
    }

    // 判断该指令是否为ADD/SUB/ADDW/SUBW指令
    bool is_add_sub_ins() const {
        switch (type) {
            case RVInstType::ADD:
            case RVInstType::SUB:
            case RVInstType::ADDW:
            case RVInstType::SUBW:
                return true;
            default:
                return false;
        }
    }

    // 判断该指令是否为二元运算指令
    bool is_binary_ins() const {
        switch (type) {
            case RVInstType::ADDW:
            case RVInstType::ADD:
            case RVInstType::ADDIW:
            case RVInstType::ADDI:
            case RVInstType::SUBW:
            case RVInstType::SUB:
            case RVInstType::SLLIW:
            case RVInstType::SRAIW:
            case RVInstType::MULW:
            case RVInstType::MUL:
            case RVInstType::DIVW:
            case RVInstType::REMW:
            case RVInstType::FADD:
            case RVInstType::FSUB:
            case RVInstType::FMUL:
            case RVInstType::FDIV:
            case RVInstType::FEQ:
            case RVInstType::FLT:
            case RVInstType::FLE:
            case RVInstType::SLTIU:
            case RVInstType::AND:
            case RVInstType::ANDI:
            case RVInstType::OR:
            case RVInstType::ORI:
            case RVInstType::XOR:
            case RVInstType::XORI:
            case RVInstType::SLT:
            case RVInstType::SLTI:
            case RVInstType::SLTU:
            case RVInstType::SRLIW:
            case RVInstType::SLLI:
            case RVInstType::MULH:
            case RVInstType::SRLI:
            case RVInstType::REM:
            case RVInstType::SH1ADD:
            case RVInstType::SH2ADD:
            case RVInstType::SH3ADD:
                return true;
            default:
                return false;
        }
    }

protected:
    RVInstruction(RVInstType t, std::vector<RVOperand *> ops)
        : type(t), operands(std::move(ops)), parent_block(nullptr) {}
    RVInstType type;
    std::vector<RVOperand *> operands;
    RVBasicBlock::InstIterator inst_iterator;  // 指向该指令在指令列表中的位置
    RVBasicBlock *parent_block = nullptr;      // 指向该指令所属的基本块
};

/* --- 带 create 的具体指令模板 --- */
template <RVInstType Ty, size_t N>
class RVInstImpl final : public RVInstruction {
public:
    template <typename... Ops>
    static RVInstImpl *create(Ops &&...ops) {
        static_assert(sizeof...(Ops) == N, "operand number mismatch with riscv instruction spec");
        return new RVInstImpl(std::forward<Ops>(ops)...);
    }

private:
    template <typename... Ops>
    explicit RVInstImpl(Ops &&...ops) : RVInstruction(Ty, std::vector<RVOperand *>{std::forward<Ops>(ops)...}) {}
};

// R-type (3 operands: rd, rs1, rs2)
using RVAdd = RVInstImpl<RVInstType::ADD, 3>;    // add rd, rs1, rs2
using RVSub = RVInstImpl<RVInstType::SUB, 3>;    // sub rd, rs1, rs2
using RVSll = RVInstImpl<RVInstType::SLL, 3>;    // sll rd, rs1, rs2
using RVSlt = RVInstImpl<RVInstType::SLT, 3>;    // slt rd, rs1, rs2
using RVSltu = RVInstImpl<RVInstType::SLTU, 3>;  // sltu rd, rs1, rs2
using RVXor = RVInstImpl<RVInstType::XOR, 3>;    // xor rd, rs1, rs2
using RVSrl = RVInstImpl<RVInstType::SRL, 3>;    // srl rd, rs1, rs2
using RVSra = RVInstImpl<RVInstType::SRA, 3>;    // sra rd, rs1, rs2
using RVOr = RVInstImpl<RVInstType::OR, 3>;      // or rd, rs1, rs2
using RVAnd = RVInstImpl<RVInstType::AND, 3>;    // and rd, rs1, rs2
using RVAddw = RVInstImpl<RVInstType::ADDW, 3>;  // addw rd, rs1, rs2
using RVSubw = RVInstImpl<RVInstType::SUBW, 3>;  // subw rd, rs1, rs2
using RVSllw = RVInstImpl<RVInstType::SLLW, 3>;  // sllw rd, rs1, rs2
using RVSrlw = RVInstImpl<RVInstType::SRLW, 3>;  // srlw rd, rs1, rs2
using RVSraw = RVInstImpl<RVInstType::SRAW, 3>;  // sraw rd, rs1, rs2
using RVMul = RVInstImpl<RVInstType::MUL, 3>;    // mul rd, rs1, rs2
using RVMulh = RVInstImpl<RVInstType::MULH, 3>;  // mulh rd, rs1, rs2
using RVMulw = RVInstImpl<RVInstType::MULW, 3>;  // mulw rd, rs1, rs2
using RVDiv = RVInstImpl<RVInstType::DIV, 3>;    // div rd, rs1, rs2
using RVDivw = RVInstImpl<RVInstType::DIVW, 3>;  // divw rd, rs1, rs2
using RVRem = RVInstImpl<RVInstType::REM, 3>;    // rem rd, rs1, rs2
using RVRemw = RVInstImpl<RVInstType::REMW, 3>;  // remw rd, rs1, rs2

// I-type (3 operands: rd, rs1, imm)
using RVAddi = RVInstImpl<RVInstType::ADDI, 3>;    // addi rd, rs1, imm
using RVSlti = RVInstImpl<RVInstType::SLTI, 3>;    // slti rd, rs1, imm
using RVSltiu = RVInstImpl<RVInstType::SLTIU, 3>;  // sltiu rd, rs1, imm
using RVXori = RVInstImpl<RVInstType::XORI, 3>;    // xori rd, rs1, imm
using RVOri = RVInstImpl<RVInstType::ORI, 3>;      // ori rd, rs1, imm
using RVAndi = RVInstImpl<RVInstType::ANDI, 3>;    // andi rd, rs1, imm
using RVSlli = RVInstImpl<RVInstType::SLLI, 3>;    // slli rd, rs1, shamt
using RVSrli = RVInstImpl<RVInstType::SRLI, 3>;    // srli rd, rs1, shamt
using RVSrai = RVInstImpl<RVInstType::SRAI, 3>;    // srai rd, rs1, shamt
using RVAddiw = RVInstImpl<RVInstType::ADDIW, 3>;  // addiw rd, rs1, imm
using RVSlliw = RVInstImpl<RVInstType::SLLIW, 3>;  // slliw rd, rs1, shamt
using RVSraiw = RVInstImpl<RVInstType::SRAIW, 3>;  // sraiw rd, rs1, shamt
using RVSrliw = RVInstImpl<RVInstType::SRLIW, 3>;  // srliw rd, rs1, shamt

// Load instructions (3 operands: rd, offset(rs1))
using RVLw = RVInstImpl<RVInstType::LW, 3>;          // lw rd, offset(rs1)
using RVLh = RVInstImpl<RVInstType::LH, 3>;          // lh rd, offset(rs1)
using RVLb = RVInstImpl<RVInstType::LB, 3>;          // lb rd, offset(rs1)
using RVLhu = RVInstImpl<RVInstType::LHU, 3>;        // lhu rd, offset(rs1)
using RVLbu = RVInstImpl<RVInstType::LBU, 3>;        // lbu rd, offset(rs1)
using RVLd = RVInstImpl<RVInstType::LD, 3>;          // ld rd, offset(rs1)
using RVJalr = RVInstImpl<RVInstType::JALR, 3>;      // jalr rd, rs1, imm (通常imm=0)
using RVLi = RVInstImpl<RVInstType::LI, 2>;          // li rd, imm (伪指令)
using RVMv = RVInstImpl<RVInstType::MV, 2>;          // mv rd, rs1 (伪指令: addi rd, rs1, 0)
using RVSext_w = RVInstImpl<RVInstType::SEXT_W, 2>;  // sext.w rd, rs1 (伪指令)

// S-type (3 operands: rs2, offset(rs1))
using RVSw = RVInstImpl<RVInstType::SW, 3>;          // sw rs2, offset(rs1)
using RVSh = RVInstImpl<RVInstType::SH, 3>;          // sh rs2, offset(rs1)
using RVSb = RVInstImpl<RVInstType::SB, 3>;          // sb rs2, offset(rs1)
using RVSd = RVInstImpl<RVInstType::SD, 3>;          // sd rs2, offset(rs1)
using RVFsw = RVInstImpl<RVInstType::FSW, 3>;        // fsw fs2, offset(rs1)
using RVFsd = RVInstImpl<RVInstType::FSD, 3>;        // fsd fs2, offset(rs1)
using RVSh1add = RVInstImpl<RVInstType::SH1ADD, 3>;  // sh1add rd, rs1, rs2
using RVSh2add = RVInstImpl<RVInstType::SH2ADD, 3>;  // sh2add rd, rs1, rs2
using RVSh3add = RVInstImpl<RVInstType::SH3ADD, 3>;  // sh3add rd, rs1, rs2

// B-type (3 operands: rs1, rs2, offset)
using RVBeq = RVInstImpl<RVInstType::BEQ, 3>;  // beq rs1, rs2, offset
using RVBne = RVInstImpl<RVInstType::BNE, 3>;  // bne rs1, rs2, offset
using RVBlt = RVInstImpl<RVInstType::BLT, 3>;  // blt rs1, rs2, offset
using RVBge = RVInstImpl<RVInstType::BGE, 3>;  // bge rs1, rs2, offset
using RVBgt = RVInstImpl<RVInstType::BGT, 3>;  // bgt rs1, rs2, offset
using RVBle = RVInstImpl<RVInstType::BLE, 3>;  // ble rs1, rs2, offset

// U-type (2 operands: rd, imm)
using RVLui = RVInstImpl<RVInstType::LUI, 2>;      // lui rd, imm
using RVAuipc = RVInstImpl<RVInstType::AUIPC, 2>;  // auipc rd, imm

// J-type (varied operands)
using RVCall = RVInstImpl<RVInstType::CALL, 2>;  // jal rd, offset (call label)
using RVJ = RVInstImpl<RVInstType::J, 1>;        // j offset (伪指令: jal x0, offset)
using RVJr = RVInstImpl<RVInstType::JR, 1>;      // jr rs1 (伪指令: jalr x0, rs1, 0)
using RVRet = RVInstImpl<RVInstType::RET, 0>;    // ret (伪指令: jalr x0, ra, 0)
using RVLla = RVInstImpl<RVInstType::LLA, 2>;    // lla rd, symbol (伪指令: 加载标签地址)
using RVLa = RVInstImpl<RVInstType::LA, 2>;      // la rd, symbol (伪指令: 加载标签地址)

// Floating-point instructions
using RVFadd = RVInstImpl<RVInstType::FADD, 3>;          // fadd.s fd, fs1, fs2
using RVFsub = RVInstImpl<RVInstType::FSUB, 3>;          // fsub.s fd, fs1, fs2
using RVFmul = RVInstImpl<RVInstType::FMUL, 3>;          // fmul.s fd, fs1, fs2
using RVFdiv = RVInstImpl<RVInstType::FDIV, 3>;          // fdiv.s fd, fs1, fs2
using RVFeq = RVInstImpl<RVInstType::FEQ, 3>;            // feq.s rd, fs1, fs2
using RVFlt = RVInstImpl<RVInstType::FLT, 3>;            // flt.s rd, fs1, fs2
using RVFle = RVInstImpl<RVInstType::FLE, 3>;            // fle.s rd, fs1, fs2
using RVFmv_s = RVInstImpl<RVInstType::FMV_S, 2>;        // fmv.s fd, fs1
using RVFcvt_s_w = RVInstImpl<RVInstType::FCVT_S_W, 2>;  // fcvt.s.w fd, rs1
using RVFcvt_w_s = RVInstImpl<RVInstType::FCVT_W_S, 2>;  // fcvt.w.s fd, rs1
using RVFlw = RVInstImpl<RVInstType::FLW, 3>;            // flw fs1, offset(rs1)
using RVFld = RVInstImpl<RVInstType::FLD, 3>;            // fld fs1, offset(rs1)

// Epilogue instruction
using RVEpilogue = RVInstImpl<RVInstType::EPILOGUE, 0>;  // epilogue (伪指令: 栈帧恢复), 已经弃用

}  // namespace backend::riscv

#endif
