#pragma once
#include "BasicBlock.hpp"
#include "MachineBasicBlock.hpp"
#include "Operand.hpp"
#include <locale>
#include <memory>
#include <string>
#include <vector>

class Operand;
class Register;
class MachineBasicBlock;
class MachineInstr : public std::enable_shared_from_this<MachineInstr> {
  public:
    enum class Suffix { BYTE, HALF, WORD, DWORD, NONE };
    enum class Tag {
        // RV64I Base Instruction Set
        LUI,
        AUIPC,
        ADDI,
        SLTI,
        SLTIU,
        XORI,
        ORI,
        ANDI,
        SLLI,
        SRLI,
        SRAI,
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
        JAL,
        JALR,
        BEQ,
        BNE,
        BLT,
        BGE,
        BLTU,
        BGEU,
        ADDIW,
        SLLIW,
        SRLIW,
        SRAIW,
        ADDW,
        SUBW,
        SLLW,
        SRLW,
        SRAW,
        LWU,
        // FIXME: LD and ST are inherited from loongarch
        LD,
        ST,
        // LB,
        // LH,
        // LW,
        // LBU,
        // LHU,
        // SB,
        // SH,
        // SW,V
        // LD,
        // SD,
        // RV64M Standard Extension
        MUL,
        MULH,
        MULHSU,
        MULHU,
        DIV,
        DIVU,
        REM,
        REMU,
        MULW,
        DIVW,
        DIVUW,
        REMW,
        // RV64F Standard Extension
        FCVT_L_S,
        FCVT_LU_S,
        FCVT_S_L,
        FCVT_S_LU,
        // RV64D Standard Extension
        FMADD_S,
        FMSUB_S,
        FNMSUB_S,
        FNMADD_S,
        FADD_S,
        FSUB_S,
        FMUL_S,
        FDIV_S,
        FSQRT_S,
        FSGNJ_S,
        FSGNJN_S,
        FSGNJX_S,
        FMIN_S,
        FMAX_S,
        FCVT_W_S,
        FCVT_WU_S,
        FMV_X_W,
        FEQ_S,
        FLT_S,
        FLE_S,
        FCLASS_S,
        FCVT_S_W,
        FCVT_S_WU,
        FMV_W_X,
        FMADD_D,
        FMSUB_D,
        FNMSUB_D,
        FNMADD_D,
        FADD_D,
        FSUB_D,
        FMUL_D,
        FDIV_D,
        FSQRT_D,
        FSGNJ_D,
        FSGNJN_D,
        FSGNJX_D,
        FMIN_D,
        FMAX_D,
        FCVT_S_D,
        FCVT_D_S,
        FEQ_D,
        FLT_D,
        FLE_D,
        FCLASS_D,
        FCVT_W_D,
        FCVT_WU_D,
        FCVT_D_W,
        FCVT_D_WU,
        FLW,
        FSW,
        FLD,
        FSD,
        FCVT_L_D,
        FCVT_LU_D,
        FCVT_X_D,
        FCVT_D_L,
        FCVT_D_LU,
        FMV_D_X,
        // pseudo instruction
        LI,
        LA,
        LLA,
        NEG,
        NOT,
        J,
        JR,
        SEQZ,
        SNEZ,
        BEQZ,
        BNEZ,
        BLEZ,
        BGEZ,
        BLTZ,
        BGTZ,
        BGT,
        BLE,
        BGTU,
        BLEU,
        SH2ADD,
        CALL, // Call far-away subroutine, combined with AUIPC and JALR
        TAIL, // Tail call far-away subroutine, combined with AUIPC and JALR
        RET,
        NOP,
        // MV,
        // FIXME: MV and MOV make some chaos in the register allocation.
        MOV
    };
    enum Flag {
        IS_FUNC_ARGS_SET = 1 << 0,
        IS_FRAME_SET = 1 << 1,
        IS_PHI_MOV = 1 << 2,
        IS_RESERVED = 1 << 3
    };
    bool is_func_args_set() const { return flag & Flag::IS_FUNC_ARGS_SET; }
    bool is_frame_set() const { return flag & Flag::IS_FRAME_SET; }
    bool is_phi_mov() const { return flag & Flag::IS_PHI_MOV; }
    bool is_reserved() const { return flag & Flag::IS_RESERVED; }

    bool has_dst() const;
    bool is_branch() const;
    std::shared_ptr<Operand> get_operand(unsigned index) const;
    std::shared_ptr<Register> get_dst() const;

    std::weak_ptr<MachineBasicBlock> get_parent() const { return parent; }
    void set_comment(const std::string &comment) { this->comment = comment; }

    Tag get_tag() const { return tag; }
    Suffix get_suffix() const { return suffix; }

    std::string print() const;
    std::string print_mov() const;

    MachineInstr(std::weak_ptr<MachineBasicBlock> parent, Tag tag,
                 std::vector<std::shared_ptr<Operand>> operands, Suffix suffix,
                 unsigned flag)
        : flag(flag), parent(parent), suffix(suffix), tag(tag),
          operands(operands) {}

    std::vector<std::shared_ptr<Register>> get_use();
    std::vector<std::shared_ptr<Register>> get_def();
    std::vector<std::shared_ptr<Register>> get_all_reg();
    void replace_def(std::shared_ptr<Register> old_reg,
                     std::shared_ptr<Register> new_reg);
    void replace_use(std::shared_ptr<Register> old_reg,
                     std::shared_ptr<Register> new_reg);

    void colorize();

  private:
    unsigned flag;
    std::shared_ptr<MachineBasicBlock> parent;
    std::string comment;

    Suffix suffix;
    Tag tag;

    std::vector<std::shared_ptr<Operand>> operands;
};
