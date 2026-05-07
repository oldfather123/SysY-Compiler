#ifndef BACKEND_INSTRUCTION_H_
#define BACKEND_INSTRUCTION_H_

#include "Common.h"
#include "machine_code.h"
#include "IR.h"

namespace backend {

    // RISC-V instruction types
    enum class InstructionType {
        R_TYPE,
        I_TYPE,
        S_TYPE,
        B_TYPE,
        U_TYPE,
        J_TYPE,
        PSEUDO,
        UNKNOWN,
    };

    // RISC-V opcode
    enum class Opcode : uint8_t {
        // RV64I
        ADD,
        SUB,
        SLL,
        SLT,
        SGT,
        SLTU,
        SGTU,
        XOR,
        SRL,
        SRA,
        OR,
        AND,
        SLLW,
        SRLW,
        SRAW,
        ADDW,
        SUBW,
        JALR,
        LB,
        LH,
        LW,
        LBU,
        LHU,
        ADDI,
        SLTI,
        SLTIU,
        XORI,
        ORI,
        ANDI,
        SLLI,
        SRLI,
        SRAI,
        SLLIW,
        SRLIW,
        SRAIW,
        ADDIW,
        LWU,
        LD,
        SB,
        SH,
        SW,
        SD,
        BEQ,
        BNE,
        BLT,
        BGE,
        BLTU,
        BGEU,
        LUI,
        AUIPC,
        JAL,

        // RV32M
        MUL,
        MULH,
        MULHU,
        MULHSU,
        MULW,
        DIV,
        DIVU,
        DIVW,
        DIVUW,
        REM,
        REMU,
        REMW,
        REMUW,

        // RV32F
        FLW,
        FSW,
        FLD,
        FSD,
        FADDS,
        FSUBS,
        FMULS,
        FDIVS,
        FSQRTS,
        FADDD,
        FSUBD,
        FMULD,
        FDIVD,
        FSQRTD,
        FMADDS,
        FMSUBS,
        FNMADDS,
        FNMSUBS,
        FMADDD,
        FMSUBD,
        FNMADDD,
        FNMSUBD,
        FMINS,
        FMAXS,
        FMIND,
        FMAXD,
        FEQS,
        FLTS,
        FLES,
        FGTS,
        FGES,
        FEQD,
        FLTD,
        FLED,
        FGTD,
        FGED,
        FCLASSS,
        FCLASSD,
        FSGNJS,
        FSGNJNS,
        FSGNJXS,
        FSGNJD,
        FSGNJND,
        FSGNJXD,
        FCVTSW,
        FCVTDW,
        FCVTSWU,
        FCVTDWU,
        FCVTWS,
        FCVTWD,
        FCVTWUS,
        FCVTWUD,
        FCVTSD,
        FCVTDS,
        FMVXS,
        FMVSX,

        // Pseudo instructions
        LA,
        LLA,
        LI,
        NOP,
        MV,
        NOT,
        NEG,
        NEGW,
        SEQZ,
        SNEZ,
        SLTZ,
        SGTZ,
        FMVS,
        FABSS,
        FNEGS,
        FMVD,
        FABSD,
        FNEGD,
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
        J,
        JR,
        RET,
        CALL,
        TAIL,

        null, // Null instruction
    };

    InstructionType getInstructionType(Opcode);

    // opcode to string
    std::string opToString(Opcode);

    class Operand {
    private:
        Ptr<Instruction> parentInst;

    public:
        enum class Type {
            REGISTER,
            IMMEDIATE,
            MEMORY,
            FUNCTION,
            BASICBLOCK,
            GLOBAL,
            MARKER, // Marker used with nop instruction

            // Stack offsets
            CALLEE_SAVED_REG_OFFSET,
            CALLER_SAVED_REG_OFFSET,
            SPILL_OFFSET,
            MEMVAR_OFFSET,
            INNER_MEMARG_OFFSET,
            MEMARG_OFFSET,
        };

        Type type;
        std::string value;

        Ptr<RiscFunction> func;
        Ptr<RiscBasicBlock> bb;
        Ptr<Register> reg;
        int offset = 0;

        Operand(Type t, const std::string &val) : type(t), value(val) { }
        Operand(Type t, const Ptr<RiscFunction> &func) : type(t), func(func), value(func->tag()) { }
        Operand(Type t, const Ptr<RiscBasicBlock> &bb) : type(t), bb(bb), value(bb->tag()) { }
        Operand(Type t, const Ptr<Register> &reg) : type(t), reg(reg), value(reg->regString()) { }
        Operand(Type t, const Ptr<Register> &reg, int n) : type(t), reg(reg), value(reg->regString()), offset(n) { }

        std::string toString() const;
    };

    class Instruction : public std::enable_shared_from_this<Instruction> {
    public:
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> operands;
        String comment;
        Vector<Ptr<Register>> regsWritten() {
            if (type == InstructionType::R_TYPE || type == InstructionType::I_TYPE || type == InstructionType::U_TYPE) {
                return {operands[0]->reg};
            } else if (type == InstructionType::PSEUDO) {
                if (opcode == Opcode::J || opcode == Opcode::JR || opcode == Opcode::RET || opcode == Opcode::BEQZ || opcode == Opcode::BNEZ) {
                    return {};
                } else if (opcode == Opcode::CALL) {
                    return {ra};
                } else if (opcode == Opcode::NOP) {
                    if (operands[0]->value == "MEMVAR_ALLOC") {
                        return {operands[1]->reg};
                    } else {
                        return {};
                    }
                } else {
                    return {operands[0]->reg};
                }
            } else {
                return {};
            }
        }
        Vector<Ptr<Register>> regsRead() {
            Vector<Ptr<Register>> res = {};
            if (type == InstructionType::R_TYPE) {
                if (opcode == Opcode::FCVTSW || opcode == Opcode::FCVTWS || opcode == Opcode::FMVS || opcode == Opcode::FMVSX || opcode == Opcode::FMVXS) {
                    return {operands[1]->reg};
                } else {
                    return {operands[1]->reg, operands[2]->reg};
                }
            } else if (type == InstructionType::I_TYPE) {
                return {operands[1]->reg};
            } else if (type == InstructionType::S_TYPE) {
                return {operands[0]->reg, operands[1]->reg};
            } else if (type == InstructionType::B_TYPE) {
                return {operands[0]->reg};
            } else if (type == InstructionType::PSEUDO) {
                if (opcode == Opcode::J || opcode == Opcode::JR || opcode == Opcode::RET || opcode == Opcode::CALL || opcode == Opcode::LI || opcode == Opcode::NOP || opcode == Opcode::LA) {
                    return {};
                } else if (opcode == Opcode::BEQZ || opcode == Opcode::BNEZ) {
                    return {operands[0]->reg};
                } else {
                    return {operands[1]->reg};
                }
            }
            return {};
        }

        int line = 0;

        Instruction(InstructionType type, Opcode opcode, const std::vector<Ptr<Operand>> &ops, const std::string &comment = "")
            : type(type), opcode(opcode), operands(ops), comment(comment) {
            dbgassert(type == getInstructionType(opcode), "Wrong instruction type for opcode");
        }

        static String combineComment(const String &comment1, const String &comment2) {
            if (comment1.empty()) {
                return comment2;
            }
            if (comment2.empty()) {
                return comment1;
            }
            return comment1 + "; " + comment2;
        }

        Ptr<Instruction> addComment(const String &comment) {
            this->comment = Instruction::combineComment(this->comment, comment);
            return thisPtr(Instruction);
        }

        virtual ~Instruction() { }

        virtual std::string toString() const;
    };

    bool isFloatPtr(ir::Value);

    bool isFloat(ir::Value);

    int32_t floatToSigned(const String &);

    Ptr<Instruction> intHandler(Ptr<RiscBasicBlock> &, const std::string &, const Ptr<Register> &);

    Ptr<Instruction> floatHandler(Ptr<RiscBasicBlock> &, const std::string &, const Ptr<Register> &);

    Ptr<Instruction> irMoveInstMapper(const Ptr<ir::MoveInst> &, Ptr<RiscBasicBlock> &);

    Ptr<Instruction> irOperInstMapper(const Ptr<ir::OperInst> &, Ptr<RiscBasicBlock> &);

    Ptr<Instruction> irCallInstMapper(const Ptr<ir::CallInst> &, Ptr<RiscBasicBlock> &);

    Ptr<Instruction> irRetInstMapper(const Ptr<ir::RetInst> &, Ptr<RiscBasicBlock> &);

    Ptr<Instruction> irLoadInstMapper(const Ptr<ir::LoadInst> &, Ptr<RiscBasicBlock> &);

    Ptr<Instruction> irStoreInstMapper(const Ptr<ir::StoreInst> &, Ptr<RiscBasicBlock> &);

    Ptr<Instruction> irCastInstMapper(const Ptr<ir::CastInst> &, Ptr<RiscBasicBlock> &);

    Ptr<Instruction> irAllocInstMapper(const Ptr<ir::AllocInst> &, Ptr<RiscBasicBlock> &);

    Ptr<Instruction> irGepInstMapper(const Ptr<ir::GEPInst> &, Ptr<RiscBasicBlock> &);

    Ptr<Instruction> irPhiInstMapper(const Ptr<ir::PhiInst> &, std::vector<Operand> &, RiscBasicBlock);

    Ptr<Instruction> irBrInstMapper(const Ptr<ir::ExitInst> &, Ptr<RiscBasicBlock> &);

}

#endif
