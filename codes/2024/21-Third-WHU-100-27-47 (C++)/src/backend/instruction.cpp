#ifndef BACKEND_INSTRUCTION_CPP_
#define BACKEND_INSTRUCTION_CPP_

#include "instruction.h"
#include "instruction_selection.h"

namespace backend {
    InstructionType getInstructionType(Opcode op) {
        switch (op) {
            case Opcode::ADDI:
            case Opcode::ADDIW:
            case Opcode::ANDI:
            // case Opcode::CSRRC:
            // case Opcode::CSRRCI:
            // case Opcode::CSRRS:
            // case Opcode::CSRRSI:
            // case Opcode::CSRRW:
            // case Opcode::CSRRWI:
            // case Opcode::EBREAK:
            // case Opcode::ECALL:
            // case Opcode::FENCE:
            // case Opcode::FENCEI:
            case Opcode::JALR:
            case Opcode::LB:
            case Opcode::LD:
            case Opcode::LHU:
            case Opcode::LW:
            case Opcode::ORI:
            case Opcode::SLLI:
            case Opcode::SLLIW:
            case Opcode::SLTI:
            case Opcode::SLTIU:
            case Opcode::SRAI:
            case Opcode::SRAIW:
            case Opcode::SRLI:
            case Opcode::SRLIW:
            case Opcode::XORI:
            case Opcode::FLD:
            case Opcode::FLW:
                return InstructionType::I_TYPE;

            case Opcode::ADD:
            case Opcode::ADDW:
            case Opcode::AND:
            case Opcode::OR:
            case Opcode::SLL:
            case Opcode::SLLW:
            case Opcode::SLT:
            case Opcode::SLTU:
            case Opcode::SRA:
            case Opcode::SRAW:
            case Opcode::SRL:
            case Opcode::SRLW:
            case Opcode::SUB:
            case Opcode::SUBW:
            case Opcode::XOR:
            case Opcode::MUL:
            case Opcode::MULW:
            case Opcode::MULH:
            case Opcode::MULHU:
            case Opcode::MULHSU:
            case Opcode::DIV:
            case Opcode::DIVW:
            case Opcode::DIVU:
            case Opcode::REM:
            case Opcode::REMW:
            case Opcode::REMU:
            case Opcode::REMUW:
            case Opcode::FADDS:
            case Opcode::FADDD:
            case Opcode::FSUBS:
            case Opcode::FSUBD:
            case Opcode::FMULS:
            case Opcode::FMULD:
            case Opcode::FDIVS:
            case Opcode::FDIVD:
            case Opcode::FSQRTS:
            case Opcode::FSQRTD:
            case Opcode::FMADDS:
            case Opcode::FMADDD:
            case Opcode::FMSUBS:
            case Opcode::FMSUBD:
            case Opcode::FNMADDS:
            case Opcode::FNMADDD:
            case Opcode::FNMSUBS:
            case Opcode::FNMSUBD:
            case Opcode::FSGNJS:
            case Opcode::FSGNJD:
            case Opcode::FSGNJNS:
            case Opcode::FSGNJND:
            case Opcode::FSGNJXS:
            case Opcode::FSGNJXD:
            case Opcode::FMINS:
            case Opcode::FMIND:
            case Opcode::FMAXS:
            case Opcode::FMAXD:
            case Opcode::FEQS:
            case Opcode::FEQD:
            case Opcode::FLTS:
            case Opcode::FLTD:
            case Opcode::FLES:
            case Opcode::FLED:
            case Opcode::FCLASSS:
            case Opcode::FCLASSD:
            case Opcode::FMVSX:
            // case Opcode::FMVDX:
            case Opcode::FMVXS:
            // case Opcode::FMVXD:
            case Opcode::FCVTSD:
            case Opcode::FCVTDS:
            case Opcode::FCVTSW:
            case Opcode::FCVTDW:
            // case Opcode::FCVTSL:
            // case Opcode::FCVTDL:
            case Opcode::FCVTSWU:
            case Opcode::FCVTDWU:
            // case Opcode::FCVTSLU:
            // case Opcode::FCVTDLU:
            case Opcode::FCVTWS:
            case Opcode::FCVTWD:
            // case Opcode::FCVTLS:
            // case Opcode::FCVTLD:
            case Opcode::FCVTWUS:
            case Opcode::FCVTWUD:
                // case Opcode::FCVTLUS:
                // case Opcode::FCVTLUD:
                return InstructionType::R_TYPE;

            case Opcode::SB:
            case Opcode::SD:
            case Opcode::SH:
            case Opcode::SW:
            case Opcode::FSD:
            case Opcode::FSW:
                return InstructionType::S_TYPE;

            case Opcode::AUIPC:
            case Opcode::LUI:
                return InstructionType::U_TYPE;

            case Opcode::BEQ:
            case Opcode::BGE:
            case Opcode::BGEU:
            case Opcode::BLT:
            case Opcode::BLTU:
            case Opcode::BNE:
                return InstructionType::B_TYPE;

            case Opcode::JAL:
                return InstructionType::J_TYPE;

            case Opcode::BEQZ:
            case Opcode::BNEZ:
            case Opcode::FABSS:
            case Opcode::FABSD:
            case Opcode::FMVS:
            case Opcode::FMVD:
            case Opcode::FNEGS:
            case Opcode::FNEGD:
            case Opcode::J:
            case Opcode::JR:
            case Opcode::LA:
            case Opcode::LI:
            case Opcode::MV:
            case Opcode::NEG:
            case Opcode::NOP:
            case Opcode::NOT:
            case Opcode::RET:
            case Opcode::SEQZ:
            case Opcode::SNEZ:
            case Opcode::CALL:
                return InstructionType::PSEUDO;

            default:
                dbgassert(false, "Unknown instruction type");
                return InstructionType::UNKNOWN;
        }
    }

    std::string opToString(Opcode op) {
        switch (op) {
            // 64-bit instructions, for pointer calculations / register saving and restoring
            case Opcode::ADD:
                return "add";
            case Opcode::ADDI:
                return "addi";
            case Opcode::SLLI:
                return "slli";
            case Opcode::LD:
                return "ld";
            case Opcode::FLD:
                return "fld";
            case Opcode::SD:
                return "sd";
            case Opcode::FSD:
                return "fsd";
            case Opcode::MUL:
                return "mul";

            case Opcode::ADDW:
                return "addw";
            case Opcode::ADDIW:
                return "addiw";
            case Opcode::AND:
                return "and";
            case Opcode::ANDI:
                return "andi";
            case Opcode::AUIPC:
                return "auipc";
            case Opcode::BEQ:
                return "beq";
            case Opcode::BGE:
                return "bge";
            case Opcode::BLT:
                return "blt";
            case Opcode::BNE:
                return "bne";
            case Opcode::JAL:
                return "jal";
            case Opcode::JALR:
                return "jalr";
            case Opcode::LUI:
                return "lui";
            case Opcode::LW:
                return "lw";
            case Opcode::OR:
                return "or";
            case Opcode::ORI:
                return "ori";
            case Opcode::SLLW:
                return "sllw";
            case Opcode::SLLIW:
                return "slliw";
            case Opcode::SLT:
                return "slt";
            case Opcode::SLTI:
                return "slti";
            case Opcode::SRAW:
                return "sraw";
            case Opcode::SRAIW:
                return "sraiw";
            case Opcode::SRLW:
                return "srlw";
            case Opcode::SRLIW:
                return "srliw";
            case Opcode::SUBW:
                return "subw";
            case Opcode::SW:
                return "sw";
            case Opcode::XOR:
                return "xor";
            case Opcode::XORI:
                return "xori";

            case Opcode::MULW:
                return "mulw";
            case Opcode::DIVW:
                return "divw";
            case Opcode::REMW:
                return "remw";

            case Opcode::FLW:
                return "flw";
            case Opcode::FSW:
                return "fsw";
            case Opcode::FADDS:
                return "fadd.s";
            case Opcode::FDIVS:
                return "fdiv.s";
            case Opcode::FSUBS:
                return "fsub.s";
            case Opcode::FMULS:
                return "fmul.s";
            case Opcode::FEQS:
                return "feq.s";
            case Opcode::FLTS:
                return "flt.s";
            case Opcode::FLES:
                return "fle.s";
            case Opcode::FMVSX:
                return "fmv.s.x";
            case Opcode::FMVXS:
                return "fmv.x.s";
            case Opcode::FCVTSW:
                return "fcvt.s.w";
            case Opcode::FCVTWS:
                return "fcvt.w.s";

            case Opcode::BEQZ:
                return "beqz";
            case Opcode::BNEZ:
                return "bnez";
            case Opcode::FMVS:
                return "fmv.s";
            case Opcode::FNEGS:
                return "fneg.s";
            case Opcode::J:
                return "j";
            case Opcode::JR:
                return "jr";
            case Opcode::LI:
                return "li";
            case Opcode::LA:
                return "la";
            case Opcode::MV:
                return "mv";
            case Opcode::NEG:
                return "neg";
            case Opcode::NOP:
                return "nop";
            case Opcode::NOT:
                return "not";
            case Opcode::RET:
                return "ret";
            case Opcode::SEQZ:
                return "seqz";
            case Opcode::SNEZ:
                return "snez";

            case Opcode::CALL:
                return "call";

            default:
                return "unknown";
        }
    }

    std::string Operand::toString() const {
        switch (type) {
            case Type::REGISTER:
                // if (reg->getType() == Register::Type::GENERAL) {
                //     return value + "G";
                // } else {
                //     return value + "F";
                // }
                return reg->regString();
            case Type::IMMEDIATE:
            case Type::BASICBLOCK:
            case Type::FUNCTION:
            case Type::GLOBAL:
                return value;
            case Type::MEMORY:
                return std::to_string(offset) + "(" + reg->regString() + ")";
            case Type::MARKER:
                return "# " + value;
            default:
                return "invalid operand type";
        }
    }

    std::string Instruction::toString() const {
        std::string result = opToString(opcode) + "\t";

        for (size_t i = 0; i < operands.size(); ++i) {
            result += operands[i]->toString();
            if (i < operands.size() - 1) {
                result += ", ";
            }
        }

        result += "\t\t##" + std::to_string(this->line);

        if (!this->comment.empty()) {
            result += "\t\t# " + this->comment;
        }

        return result;
    }

    bool isFloatPtr(ir::Value op) {
        return op.dataType().isPointer() && op.dataType().baseDataType() == PrimaryDataType::Float;
    }

    bool isFloat(ir::Value op) {
        return !op.dataType().isPointer() && !op.dataType().isArray() && op.dataType().baseDataType() == PrimaryDataType::Float;
    }

    int32_t floatToSigned(const String &str) {
        float num = std::stof(str);
        int32_t result;
        std::memcpy(&result, &num, sizeof(num));
        return result;

        float f = std::stof(str);
        return *reinterpret_cast<uint32_t *>(&f);
    }

    Ptr<Instruction> intHandler(Ptr<RiscBasicBlock> &bb, const std::string &str, const Ptr<Register> &destReg) {
        std::vector<Ptr<Operand>> ops;
        Ptr<Operand> op1 = makePtr<Operand>(Operand::Type::REGISTER, destReg);
        Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::IMMEDIATE, str);
        ops = {op1, op2};
        auto liInst = makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Load immediate with intHandler");
        ops.clear();

        return bb->addInstruction(liInst);
    }

    Ptr<Instruction> floatHandler(Ptr<RiscBasicBlock> &bb, const std::string &str, const Ptr<Register> &destReg) {
        int32_t floatBits = floatToSigned(str);

        std::vector<Ptr<Operand>> ops;
        auto tmpReg = REG_TEMP_GENERAL[0];
        ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
               makePtr<Operand>(Operand::Type::IMMEDIATE, std::to_string(floatBits))};
        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops));
        ops = {makePtr<Operand>(Operand::Type::REGISTER, destReg),
               makePtr<Operand>(Operand::Type::REGISTER, tmpReg)};
        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FMVSX, ops, "Load float immediate with floatHandler"));
    }

    Ptr<Instruction> irMoveInstMapper(const Ptr<ir::MoveInst> &irMoveInst, Ptr<RiscBasicBlock> &bb) {
        if (irMoveInst->srcVal().dataType() == PrimaryDataType::Void) {
            return nullptr;
        }

        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();
        auto destVirtualReg = func->getOrCreateVirtualRegister(irMoveInst->destReg());
        Ptr<Operand> op1 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
        ops.push_back(op1);

        if (irMoveInst->srcVal().isLiteral()) {
            if (isFloat(irMoveInst->srcVal())) {
                return floatHandler(bb, irMoveInst->srcVal().toString(), destVirtualReg);
            } else {
                type = InstructionType::PSEUDO;
                opcode = Opcode::LI;
                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::IMMEDIATE, irMoveInst->srcVal().toString());
                ops.push_back(op2);
            }
        } else if (isFloat(irMoveInst->srcVal())) {
            if (isFloat(irMoveInst->destReg())) {
                type = InstructionType::PSEUDO;
                opcode = Opcode::FMVS;
            } else {
                type = InstructionType::R_TYPE;
                opcode = Opcode::FMVXS;
            }
            auto srcVirtualReg = func->getOrCreateVirtualRegister(irMoveInst->srcVal().getRegister());
            Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, srcVirtualReg);
            ops.push_back(op2);
        } else {
            if (isFloat(irMoveInst->destReg())) {
                type = InstructionType::R_TYPE;
                opcode = Opcode::FMVSX;
            } else {
                type = InstructionType::PSEUDO;
                opcode = Opcode::MV;
            }
            auto srcVirtualReg = func->getOrCreateVirtualRegister(irMoveInst->srcVal().getRegister());
            Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, srcVirtualReg);
            ops.push_back(op2);
        }

        return bb->addInstruction(makePtr<Instruction>(type, opcode, ops));
    }

    Ptr<Instruction> irOperInstMapper(const Ptr<ir::OperInst> &irOperInst, Ptr<RiscBasicBlock> &bb) {
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();

        auto destVirtualReg = func->getOrCreateVirtualRegister(irOperInst->destReg());
        Ptr<Operand> op1 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
        ops.push_back(op1);

        bool is64Bit = irOperInst->destReg()->dataType().is64Bit() || irOperInst->destReg()->dataType().isPointer();

        if (irOperInst->isUnary()) {
            dbgassert(false, "Unary operation is not supported here");
        } else {
            auto lhs = irOperInst->lhs();
            auto rhs = irOperInst->rhs();
            bool isImm = (lhs.isLiteral() || rhs.isLiteral());

            if (!isImm) {
                auto lhsVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                auto rhsVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, lhsVirtualReg);
                Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, rhsVirtualReg);
                ops.push_back(op2);
                ops.push_back(op3);
            } else if (lhs.isLiteral()) {
                // imm op register
                // li into destreg then
                // destreg = destreg op register
                auto rhsVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, rhsVirtualReg);
                ops.push_back(op2);
                ops.push_back(op3);
            }

            switch (irOperInst->op()) {
                case ir::OperInst::Operator::Shl:
                    if (isImm && !lhs.isLiteral()) {
                        opcode = is64Bit ? Opcode::SLLI : Opcode::SLLIW;
                        type = InstructionType::I_TYPE;
                        auto lhsVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                        Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, lhsVirtualReg);
                        Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::IMMEDIATE, rhs.getLiteral().toString());
                        ops.push_back(op2);
                        ops.push_back(op3);
                    } else {
                        if (lhs.isLiteral()) {
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                        }
                        opcode = Opcode::SLLW;
                        type = InstructionType::R_TYPE;
                    }
                    break;
                case ir::OperInst::Operator::Shr:
                    if (isImm && !lhs.isLiteral()) {
                        opcode = Opcode::SRAIW;
                        type = InstructionType::I_TYPE;
                        auto lhsVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                        Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, lhsVirtualReg);
                        Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::IMMEDIATE, rhs.getLiteral().toString());
                        ops.push_back(op2);
                        ops.push_back(op3);
                    } else {
                        if (lhs.isLiteral()) {
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                        }
                        opcode = Opcode::SRAW;
                        type = InstructionType::R_TYPE;
                    }
                    break;
                case ir::OperInst::Operator::BitwiseAnd:
                    if (isFloat(irOperInst->destReg())) {
                        throw std::runtime_error("float and is not supported");
                    } else if (isImm && !lhs.isLiteral()) {
                        opcode = Opcode::ANDI;
                        type = InstructionType::I_TYPE;
                        auto lhsVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                        Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, lhsVirtualReg);
                        Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::IMMEDIATE, rhs.getLiteral().toString());
                        ops.push_back(op2);
                        ops.push_back(op3);
                    } else {
                        if (lhs.isLiteral()) {
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                        }
                        opcode = Opcode::AND;
                        type = InstructionType::R_TYPE;
                    }
                    break;
                case ir::OperInst::Operator::BitwiseOr:
                    if (isFloat(irOperInst->destReg())) {
                        throw std::runtime_error("float or is not supported");
                    } else if (isImm && !lhs.isLiteral()) {
                        opcode = Opcode::ORI;
                        type = InstructionType::I_TYPE;
                        auto lhsVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                        Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, lhsVirtualReg);
                        Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::IMMEDIATE, rhs.getLiteral().toString());
                        ops.push_back(op2);
                        ops.push_back(op3);
                    } else {
                        if (lhs.isLiteral()) {
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                        }
                        opcode = Opcode::OR;
                        type = InstructionType::R_TYPE;
                    }
                    break;
                case ir::OperInst::Operator::Xor:
                case ir::OperInst::Operator::BitwiseXor:
                    if (isFloat(irOperInst->destReg())) {
                        throw std::runtime_error("float xor is not supported");
                    } else if (isImm && !lhs.isLiteral()) {
                        opcode = Opcode::XORI;
                        type = InstructionType::I_TYPE;
                        auto lhsVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                        Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, lhsVirtualReg);
                        Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::IMMEDIATE, rhs.getLiteral().toString());
                        ops.push_back(op2);
                        ops.push_back(op3);
                    } else {
                        if (lhs.isLiteral()) {
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                        }
                        opcode = Opcode::XOR;
                        type = InstructionType::R_TYPE;
                    }
                    break;
                case ir::OperInst::Operator::Add:
                    if (isFloat(irOperInst->destReg())) {
                        opcode = Opcode::FADDS;
                        type = InstructionType::R_TYPE;
                        if (isImm) {
                            std::shared_ptr<Register> newVirtualReg;
                            // imm + reg || reg + imm
                            if (lhs.isLiteral()) {
                                floatHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                            } else {
                                newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                                floatHandler(bb, rhs.getLiteral().toString(), destVirtualReg);

                                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg);
                                Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                                ops.push_back(op2);
                                ops.push_back(op3);
                            }
                        }
                    } else if (isImm) {
                        opcode = is64Bit ? Opcode::ADD : Opcode::ADDW;
                        type = InstructionType::R_TYPE;
                        if (lhs.isLiteral()) {
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                        } else {
                            auto lhsVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                            auto rImm = rhs.getLiteral().getInt();
                            if (rImm >= -2048 && rImm < 2048) {
                                opcode = is64Bit ? Opcode::ADDI : Opcode::ADDIW;
                                type = InstructionType::I_TYPE;
                                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, lhsVirtualReg);
                                Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::IMMEDIATE, rhs.getLiteral().toString());
                                ops.push_back(op2);
                                ops.push_back(op3);
                            } else {
                                intHandler(bb, rhs.getLiteral().toString(), destVirtualReg);
                                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, lhsVirtualReg);
                                Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                                ops.push_back(op2);
                                ops.push_back(op3);
                            }
                        }
                    } else {
                        opcode = is64Bit ? Opcode::ADD : Opcode::ADDW;
                        type = InstructionType::R_TYPE;
                    }
                    break;
                case ir::OperInst::Operator::Sub:
                    if (isFloat(irOperInst->destReg())) {
                        opcode = Opcode::FSUBS;
                        type = InstructionType::R_TYPE;
                        if (isImm) {
                            // imm + reg || reg + imm
                            if (lhs.isLiteral()) {
                                floatHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                            } else {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                                floatHandler(bb, rhs.getLiteral().toString(), destVirtualReg);

                                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg);
                                Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                                ops.push_back(op2);
                                ops.push_back(op3);
                            }
                        }
                    } else {
                        opcode = Opcode::SUBW;
                        type = InstructionType::R_TYPE;
                        if (lhs.isLiteral()) {
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                        } else if (rhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                            intHandler(bb, rhs.getLiteral().toString(), destVirtualReg);

                            Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg);
                            Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                            ops.push_back(op2);
                            ops.push_back(op3);
                        }
                    }
                    break;
                case ir::OperInst::Operator::Mul:
                    if (isFloat(irOperInst->destReg())) {
                        opcode = Opcode::FMULS;
                        type = InstructionType::R_TYPE;
                        if (isImm) {
                            // imm + reg || reg + imm
                            if (lhs.isLiteral()) {
                                floatHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                            } else {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                                floatHandler(bb, rhs.getLiteral().toString(), destVirtualReg);

                                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg);
                                Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                                ops.push_back(op2);
                                ops.push_back(op3);
                            }
                        }
                    } else {
                        opcode = is64Bit ? Opcode::MUL : Opcode::MULW;
                        type = InstructionType::R_TYPE;
                        if (lhs.isLiteral()) {
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                        } else if (rhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                            intHandler(bb, rhs.getLiteral().toString(), destVirtualReg);

                            Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg);
                            Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                            ops.push_back(op2);
                            ops.push_back(op3);
                        }
                    }
                    break;
                case ir::OperInst::Operator::Div:
                    if (isFloat(irOperInst->destReg())) {
                        opcode = Opcode::FDIVS;
                        type = InstructionType::R_TYPE;
                        if (isImm) {
                            // imm + reg || reg + imm
                            if (lhs.isLiteral()) {
                                floatHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                            } else {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                                floatHandler(bb, rhs.getLiteral().toString(), destVirtualReg);

                                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg);
                                Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                                ops.push_back(op2);
                                ops.push_back(op3);
                            }
                        }
                    } else {
                        opcode = Opcode::DIVW;
                        type = InstructionType::R_TYPE;
                        if (lhs.isLiteral()) {
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                        } else if (rhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                            intHandler(bb, rhs.getLiteral().toString(), destVirtualReg);

                            Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg);
                            Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                            ops.push_back(op2);
                            ops.push_back(op3);
                        }
                    }
                    break;
                case ir::OperInst::Operator::Mod:
                    if (isFloat(irOperInst->destReg())) {
                        throw std::runtime_error("float mod is not supported");
                    } else {
                        opcode = Opcode::REMW;
                        type = InstructionType::R_TYPE;
                        if (isImm) {
                            if (lhs.isLiteral()) {
                                intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                            } else {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                                intHandler(bb, rhs.getLiteral().toString(), destVirtualReg);

                                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg);
                                Ptr<Operand> op3 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
                                ops.push_back(op2);
                                ops.push_back(op3);
                            }
                        }
                    }
                    break;
                case ir::OperInst::Operator::Eq:
                    if (isFloat(lhs) || isFloat(rhs)) {
                        auto tmpFReg = REG_TEMP_FLOAT[0];
                        if (isImm) {
                            // imm + reg || reg + imm
                            if (lhs.isLiteral()) {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                                floatHandler(bb, lhs.getLiteral().toString(), tmpFReg);
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpFReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, tmpFReg)};
                            } else {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                                floatHandler(bb, rhs.getLiteral().toString(), tmpFReg);
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpFReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, tmpFReg)};
                            }
                        } else {
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpFReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(lhs.getRegister())),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(rhs.getRegister()))};
                        }
                        bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FSUBS, ops));
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                               makePtr<Operand>(Operand::Type::REGISTER, tmpFReg)};
                        auto fcvtwsInst = makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FMVXS, ops);
                        ops.clear();
                        bb->addInstruction(fcvtwsInst);
                    } else {
                        if (lhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg)};
                        } else if (rhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                            intHandler(bb, rhs.getLiteral().toString(), destVirtualReg);
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg)};
                        } else {
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(lhs.getRegister())),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(rhs.getRegister()))};
                        }
                        bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::SUBW, ops));
                    }
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                           makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg)};
                    return bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::SEQZ, ops));
                case ir::OperInst::Operator::Ne:
                    if (isFloat(lhs) || isFloat(rhs)) {
                        auto tmpFReg = REG_TEMP_FLOAT[0];
                        if (isImm) {
                            // imm + reg || reg + imm
                            if (lhs.isLiteral()) {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                                floatHandler(bb, lhs.getLiteral().toString(), tmpFReg);
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpFReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, tmpFReg)};
                            } else {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                                floatHandler(bb, rhs.getLiteral().toString(), tmpFReg);
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpFReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, tmpFReg)};
                            }
                        } else {
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpFReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(lhs.getRegister())),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(rhs.getRegister()))};
                        }
                        bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FSUBS, ops));
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                               makePtr<Operand>(Operand::Type::REGISTER, tmpFReg)};
                        auto fcvtwsInst = makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FMVXS, ops);
                        ops.clear();
                        bb->addInstruction(fcvtwsInst);
                    } else {
                        if (lhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg)};
                        } else if (rhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                            intHandler(bb, rhs.getLiteral().toString(), destVirtualReg);
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg)};
                        } else {
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(lhs.getRegister())),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(rhs.getRegister()))};
                        }
                        bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::SUBW, ops));
                    }
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                           makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg)};
                    return bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::SNEZ, ops));
                case ir::OperInst::Operator::Lt:
                    if (isFloat(lhs) || isFloat(rhs)) {
                        auto tmpFReg = REG_TEMP_FLOAT[0];
                        if (isImm) {
                            // imm + reg || reg + imm
                            if (lhs.isLiteral()) {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                                floatHandler(bb, lhs.getLiteral().toString(), tmpFReg);
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, tmpFReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg)};
                            } else {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                                floatHandler(bb, rhs.getLiteral().toString(), tmpFReg);
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, tmpFReg)};
                            }
                        } else {
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(lhs.getRegister())),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(rhs.getRegister()))};
                        }
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FLTS, ops));
                    } else {
                        if (lhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg)};
                        } else if (rhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                            intHandler(bb, rhs.getLiteral().toString(), destVirtualReg);
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg)};
                        } else {
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(lhs.getRegister())),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(rhs.getRegister()))};
                        }
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::SLT, ops));
                    }
                case ir::OperInst::Operator::Ge:
                    if (isFloat(lhs) || isFloat(rhs)) {
                        auto tmpFReg = REG_TEMP_FLOAT[0];
                        if (isImm) {
                            // imm + reg || reg + imm
                            if (lhs.isLiteral()) {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                                floatHandler(bb, lhs.getLiteral().toString(), tmpFReg);
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, tmpFReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg)};
                            } else {
                                auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                                floatHandler(bb, rhs.getLiteral().toString(), tmpFReg);
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, tmpFReg)};
                            }
                        } else {
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(lhs.getRegister())),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(rhs.getRegister()))};
                        }
                        bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FLTS, ops));
                    } else {
                        if (lhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(rhs.getRegister());
                            intHandler(bb, lhs.getLiteral().toString(), destVirtualReg);
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg)};
                        } else if (rhs.isLiteral()) {
                            auto newVirtualReg = func->getOrCreateVirtualRegister(lhs.getRegister());
                            intHandler(bb, rhs.getLiteral().toString(), destVirtualReg);
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, newVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg)};
                        } else {
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(lhs.getRegister())),
                                   makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(rhs.getRegister()))};
                        }
                        bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::SLT, ops));
                    }
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                           makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                           makePtr<Operand>(Operand::Type::IMMEDIATE, "1")};
                    return bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, Opcode::XORI, ops));
                case ir::OperInst::Operator::And:
                    dbgassert(!isFloat(lhs) && !isFloat(rhs), "float and is not supported");
                    {
                        std::shared_ptr<Register> newReg1;
                        std::shared_ptr<Register> newReg2;
                        if (isImm) {
                            if (lhs.isLiteral()) {
                                intHandler(bb, lhs.getLiteral().toString(), REG_TEMP_GENERAL[0]);
                                newReg2 = func->getOrCreateVirtualRegister(rhs.getRegister());
                                newReg1 = REG_TEMP_GENERAL[0];
                            } else {
                                intHandler(bb, rhs.getLiteral().toString(), REG_TEMP_GENERAL[0]);
                                newReg1 = func->getOrCreateVirtualRegister(lhs.getRegister());
                                newReg2 = REG_TEMP_GENERAL[0];
                            }
                        } else {
                            newReg1 = func->getOrCreateVirtualRegister(lhs.getRegister());
                            newReg2 = func->getOrCreateVirtualRegister(rhs.getRegister());
                        }
                        ops.clear();
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, newReg1),
                               makePtr<Operand>(Operand::Type::REGISTER, newReg1)};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::SNEZ, ops));
                        ops.clear();
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, newReg2),
                               makePtr<Operand>(Operand::Type::REGISTER, newReg2)};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::SNEZ, ops));
                        ops.clear();
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                               makePtr<Operand>(Operand::Type::REGISTER, newReg1),
                               makePtr<Operand>(Operand::Type::REGISTER, newReg2)};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::AND, ops));
                    }
                case ir::OperInst::Operator::Or:
                    dbgassert(!isFloat(lhs) && !isFloat(rhs), "float or is not supported");
                    {
                        std::shared_ptr<Register> newReg1;
                        std::shared_ptr<Register> newReg2;
                        if (isImm) {
                            if (lhs.isLiteral()) {
                                intHandler(bb, lhs.getLiteral().toString(), REG_TEMP_GENERAL[0]);
                                newReg2 = func->getOrCreateVirtualRegister(rhs.getRegister());
                                newReg1 = REG_TEMP_GENERAL[0];
                            } else {
                                intHandler(bb, rhs.getLiteral().toString(), REG_TEMP_GENERAL[0]);
                                newReg1 = func->getOrCreateVirtualRegister(lhs.getRegister());
                                newReg2 = REG_TEMP_GENERAL[0];
                            }
                        } else {
                            newReg1 = func->getOrCreateVirtualRegister(lhs.getRegister());
                            newReg2 = func->getOrCreateVirtualRegister(rhs.getRegister());
                        }
                        ops.clear();
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, newReg1),
                               makePtr<Operand>(Operand::Type::REGISTER, newReg1)};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::SNEZ, ops));
                        ops.clear();
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, newReg2),
                               makePtr<Operand>(Operand::Type::REGISTER, newReg2)};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::SNEZ, ops));
                        ops.clear();
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                               makePtr<Operand>(Operand::Type::REGISTER, newReg1),
                               makePtr<Operand>(Operand::Type::REGISTER, newReg2)};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::OR, ops));
                    }
                default:
                    dbgassert(false, "Unsupported operation type");
            }
        }

        return bb->addInstruction(makePtr<Instruction>(type, opcode, ops));
    }

    Ptr<Instruction> irCallInstMapper(const Ptr<ir::CallInst> &irCallInst, Ptr<RiscBasicBlock> &bb) {
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();

        Ptr<RiscFunction> callFunc = func->parentModule()->getFunction(irCallInst->function());

        ops = {makePtr<Operand>(Operand::Type::MARKER, "BEFORE_CALL"),
               makePtr<Operand>(Operand::Type::FUNCTION, callFunc)};
        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::NOP, ops));

        int generalArgCount = 0;
        int floatArgCount = 0;
        for (auto &arg : irCallInst->argList()) {
            bool isFloatArg = isFloat(arg);
            auto nextArgOperand = getNextArgOperand(func, generalArgCount, floatArgCount, isFloatArg);
            if (nextArgOperand->type == Operand::Type::REGISTER || (nextArgOperand->type == Operand::Type::MEMORY && nextArgOperand->offset >= -2048 && nextArgOperand->offset < 2048)) {
                storeValueTo64(bb, arg, nextArgOperand)->addComment("Pass argument");
            } else if (nextArgOperand->type == Operand::Type::MEMORY) {
                dbgassert(nextArgOperand->reg == sp, "Base register for memory argument should be sp");
                auto tmpReg = REG_TEMP_GENERAL[2];
                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                       makePtr<Operand>(Operand::Type::INNER_MEMARG_OFFSET, std::to_string(nextArgOperand->offset))};
                bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops));
                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                       makePtr<Operand>(Operand::Type::REGISTER, sp),
                       makePtr<Operand>(Operand::Type::REGISTER, tmpReg)};
                bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::ADD, ops));
                storeValueTo64(bb, arg, makePtr<Operand>(Operand::Type::MEMORY, tmpReg, 0))->addComment("Pass argument");
            } else {
                dbgassert(false, "Unhandled case");
            }
            if (isFloatArg) {
                ++floatArgCount;
            } else {
                ++generalArgCount;
            }
        }

        ops = {makePtr<Operand>(Operand::Type::FUNCTION, callFunc)};
        auto ret = makePtr<Instruction>(InstructionType::PSEUDO, Opcode::CALL, ops);
        bb->addInstruction(ret);

        // Return value handling
        if (!irCallInst->destReg()->isDiscard()) {
            auto destReg = func->getOrCreateVirtualRegister(irCallInst->destReg());
            if (irCallInst->destReg()->dataType() == PrimaryDataType::Float) {
                ops = {makePtr<Operand>(Operand::Type::REGISTER, destReg),
                       makePtr<Operand>(Operand::Type::REGISTER, REG_ARGS_FLOAT[0])};
                bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::FMVS, ops))->addComment("Handle return value");
            } else if (irCallInst->destReg()->dataType() == PrimaryDataType::Int) {
                ops = {makePtr<Operand>(Operand::Type::REGISTER, destReg),
                       makePtr<Operand>(Operand::Type::REGISTER, REG_ARGS_GENERAL[0])};
                //        makePtr<Operand>(Operand::Type::IMMEDIATE, "0")};
                // bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, Opcode::ADDIW, ops));
                bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::MV, ops))->addComment("Handle return value");
            } else {
                dbgassert(irCallInst->destReg()->dataType() == PrimaryDataType::Void, "Invalid return type");
            }
        }

        ops = {makePtr<Operand>(Operand::Type::MARKER, "AFTER_CALL"),
               makePtr<Operand>(Operand::Type::FUNCTION, callFunc)};
        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::NOP, ops));
        return ret;
    }

    Ptr<Instruction> irRetInstMapper(const Ptr<ir::RetInst> &irRetInst, Ptr<RiscBasicBlock> &bb) {
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();

        if (irRetInst->retVal().dataType() != PrimaryDataType::Void) {
            Ptr<Register> retValReg = isFloat(irRetInst->retVal()) ? REG_ARGS_FLOAT[0] : REG_ARGS_GENERAL[0];
            return storeValueTo64(bb, irRetInst->retVal(), makePtr<Operand>(Operand::Type::REGISTER, retValReg));
        }
        return nullptr;
    }

    Ptr<Instruction> irLoadInstMapper(const Ptr<ir::LoadInst> &irLoadInst, Ptr<RiscBasicBlock> &bb) {
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();

        auto destVirtualReg = func->getOrCreateVirtualRegister(irLoadInst->destReg());
        Ptr<Operand> op1 = makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg);
        auto srcVirtualReg = func->getOrCreateVirtualRegister(irLoadInst->srcAddrReg());
        Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::MEMORY, srcVirtualReg, 0);

        ops.push_back(op1);
        ops.push_back(op2);

        type = InstructionType::I_TYPE;

        if (isFloatPtr(irLoadInst->srcAddrReg()) && isFloat(irLoadInst->destReg())) {
            opcode = Opcode::FLW;
        } else if (!isFloatPtr(irLoadInst->srcAddrReg()) && !isFloat(irLoadInst->destReg())) {
            opcode = Opcode::LW;
        } else {
            throw std::runtime_error("Error: Mismatched types between load source and destination registers.");
        }

        return bb->addInstruction(makePtr<Instruction>(type, opcode, ops));
    }

    Ptr<Instruction> irStoreInstMapper(const Ptr<ir::StoreInst> &irStoreInst, Ptr<RiscBasicBlock> &bb) {
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();

        Ptr<Register> srcReg = nullptr;

        if (irStoreInst->srcVal().isLiteral()) {
            if (isFloat(irStoreInst->srcVal())) {
                srcReg = REG_TEMP_FLOAT[0];
                floatHandler(bb, irStoreInst->srcVal().getLiteral().toString(), srcReg);
            } else {
                srcReg = REG_TEMP_GENERAL[0];
                intHandler(bb, irStoreInst->srcVal().getLiteral().toString(), srcReg);
            }
        } else {
            srcReg = func->getOrCreateVirtualRegister(irStoreInst->srcVal().getRegister());
        }

        Ptr<Operand> op1 = makePtr<Operand>(Operand::Type::REGISTER, srcReg);
        ops.push_back(op1);

        auto destVirtualReg = func->getOrCreateVirtualRegister(irStoreInst->destAddrReg());
        Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::MEMORY, destVirtualReg, 0);
        ops.push_back(op2);

        type = InstructionType::S_TYPE;

        if (isFloatPtr(irStoreInst->destAddrReg()) && isFloat(irStoreInst->srcVal())) {
            opcode = Opcode::FSW;
        } else if (!isFloatPtr(irStoreInst->destAddrReg()) && !isFloat(irStoreInst->srcVal())) {
            opcode = Opcode::SW;
        } else {
            throw std::runtime_error("Error: Mismatched types between store source and destination registers.");
        }

        return bb->addInstruction(makePtr<Instruction>(type, opcode, ops));
    }

    Ptr<Instruction> irCastInstMapper(const Ptr<ir::CastInst> &irCastInst, Ptr<RiscBasicBlock> &bb) {
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();
        Ptr<Instruction> ret = nullptr;

        bool isImm = irCastInst->srcVal().isLiteral();
        auto destVirtualReg = func->getOrCreateVirtualRegister(irCastInst->destReg());
        if (!isImm) {
            auto srcVirtualReg = func->getOrCreateVirtualRegister(irCastInst->srcVal().getRegister());
            type = InstructionType::R_TYPE;
            if (isFloat(irCastInst->srcVal()) && !isFloat(irCastInst->destReg())) {
                // f32 -> i32/i1
                // First f32 -> i32
                ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                       makePtr<Operand>(Operand::Type::REGISTER, srcVirtualReg),
                       makePtr<Operand>(Operand::Type::IMMEDIATE, "rtz")}; // This is actually not immediate, but an rtz flag
                ret = bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FCVTWS, ops, "Cast f32 -> i32"));
                if (irCastInst->destReg()->dataType() == PrimaryDataType::Bool) {
                    // Then i32 -> i1
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                           makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg)};
                    ret = bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::SNEZ, ops, "Then cast i32 -> i1"));
                }
            } else if (!isFloat(irCastInst->srcVal()) && isFloat(irCastInst->destReg())) {
                // i32/i1 -> f32
                ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                       makePtr<Operand>(Operand::Type::REGISTER, srcVirtualReg)};
                ret = bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FCVTSW, ops, "Cast i32/i1 -> f32"));
            } else if (irCastInst->srcVal().dataType() == PrimaryDataType::Bool && irCastInst->destReg()->dataType() == PrimaryDataType::Int) {
                // i1 -> i32
                ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                       makePtr<Operand>(Operand::Type::REGISTER, srcVirtualReg)};
                //    makePtr<Operand>(Operand::Type::IMMEDIATE, "0")};
                // bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, Opcode::ADDIW, ops, "Cast i1 -> i32"));
                ret = bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::MV, ops));
            } else if (irCastInst->srcVal().dataType() == PrimaryDataType::Int && irCastInst->destReg()->dataType() == PrimaryDataType::Bool) {
                // i32 -> i1
                ops = {makePtr<Operand>(Operand::Type::REGISTER, destVirtualReg),
                       makePtr<Operand>(Operand::Type::REGISTER, srcVirtualReg)};
                ret = bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::SNEZ, ops, "Cast i32 -> i1"));
            }
        } else {
            dbgassert(false, "This should never happen");
        }
        return ret;
    }

    Ptr<Instruction> irAllocInstMapper(const Ptr<ir::AllocInst> &irAllocInst, Ptr<RiscBasicBlock> &bb) {
        // allocate stack
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();

        // pointer to allocated memory
        auto virtualReg = func->getOrCreateVirtualRegister(irAllocInst->destReg());
        virtualReg->memVarOffset = func->stackFrame.allocMemVar(irAllocInst->destReg()->dataType().derefType().bytes());

        ops = {makePtr<Operand>(Operand::Type::REGISTER, virtualReg),
               makePtr<Operand>(Operand::Type::MEMVAR_OFFSET, std::to_string(virtualReg->memVarOffset))};
        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Prepare pointer offset")); // 64-bit pointer addition
        ops = {makePtr<Operand>(Operand::Type::REGISTER, virtualReg),
               makePtr<Operand>(Operand::Type::REGISTER, s0),
               makePtr<Operand>(Operand::Type::REGISTER, virtualReg)};
        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::ADD, ops, "Then calculate pointer address")); // 64-bit pointer addition

        // If is array, call memset will follow, no need to do anything to initialize the array here
    }

    Ptr<Instruction> irGepInstMapper(const Ptr<ir::GEPInst> &irGepInst, Ptr<RiscBasicBlock> &bb) {
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();

        auto destReg = func->getOrCreateVirtualRegister(irGepInst->destReg());
        auto ptrReg = func->getOrCreateVirtualRegister(irGepInst->arrPtrReg());
        Ptr<Register> tmpOffsetReg = REG_TEMP_GENERAL[0]; // temporary register
        int dimensionSize = irGepInst->arrPtrReg()->dataType().derefType().elemType().bytes();
        if (irGepInst->indexVal().isLiteral()) {
            intHandler(bb, std::to_string(irGepInst->indexVal().getLiteral().getInt() * dimensionSize), tmpOffsetReg);
        } else {
            auto offsetReg = func->getOrCreateVirtualRegister(irGepInst->indexVal().getRegister());
            ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpOffsetReg),
                   makePtr<Operand>(Operand::Type::IMMEDIATE, std::to_string(dimensionSize))};
            bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops));
            ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpOffsetReg),
                   makePtr<Operand>(Operand::Type::REGISTER, offsetReg),
                   makePtr<Operand>(Operand::Type::REGISTER, tmpOffsetReg)};
            bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::MUL, ops));
        }
        ops = {makePtr<Operand>(Operand::Type::REGISTER, destReg),
               makePtr<Operand>(Operand::Type::REGISTER, ptrReg),
               makePtr<Operand>(Operand::Type::REGISTER, tmpOffsetReg)};
        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::ADD, ops));
    }

    Ptr<Instruction> irPhiInstMapper(const Ptr<ir::PhiInst> &irPhiInst, std::vector<Operand> &ops, InstructionType type) {
        dbgassert(false, "This should never happen");
        return nullptr;
    }

    Ptr<Instruction> irBrInstMapper(const Ptr<ir::ExitInst> &irBrInst, Ptr<RiscBasicBlock> &bb) {
        InstructionType type;
        Opcode opcode;
        std::vector<Ptr<Operand>> ops;
        Ptr<RiscFunction> func = bb->parentFunc();

        if (irBrInst->isCondBr()) {
            type = InstructionType::PSEUDO;
            opcode = Opcode::BEQZ;
            ir::BBPtr irTrueBB = irBrInst->trueTarget();
            ir::BBPtr irFalseBB = irBrInst->falseTarget();

            Ptr<RiscBasicBlock> trueBB = func->getBasicBlock(irTrueBB);
            Ptr<RiscBasicBlock> falseBB = func->getBasicBlock(irFalseBB);
            if (irBrInst->condition().isRegister()) {
                auto conVirtualReg = func->getOrCreateVirtualRegister(irBrInst->condition().getRegister());
                Ptr<Operand> op1 = makePtr<Operand>(Operand::Type::REGISTER, conVirtualReg);
                Ptr<Operand> op2 = makePtr<Operand>(Operand::Type::BASICBLOCK, falseBB);

                ops.push_back(op1);
                ops.push_back(op2);
                bb->addInstruction(makePtr<Instruction>(type, opcode, ops));

                ops = {makePtr<Operand>(Operand::Type::BASICBLOCK, trueBB)};
                return bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::J, ops));
            } else {
                if (irBrInst->condition().getLiteral().getBool() == false) {
                    Ptr<Operand> op = makePtr<Operand>(Operand::Type::BASICBLOCK, falseBB);
                    ops = {op};
                } else {
                    Ptr<Operand> op = makePtr<Operand>(Operand::Type::BASICBLOCK, trueBB);
                    ops = {op};
                }

                return bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::J, ops));
            }

        } else {
            type = InstructionType::PSEUDO;
            opcode = Opcode::J;
            Ptr<RiscBasicBlock> unconBB = func->getBasicBlock(irBrInst->unconditionalTarget());
            Ptr<Operand> op = makePtr<Operand>(Operand::Type::BASICBLOCK, unconBB);

            ops.push_back(op);
            return bb->addInstruction(makePtr<Instruction>(type, opcode, ops));
        }
    }
}

#endif
