#ifndef BACKEND_PEEPHOLE_CPP_
#define BACKEND_PEEPHOLE_CPP_

#include "peephole.h"

namespace backend {
    void optimizeWindow(std::vector<Ptr<Instruction>> &window, Ptr<RiscBasicBlock> &bb, bool &changed) {
        size_t size = window.size();
        std::vector<Ptr<Operand>> ops;

        if (size == 3) {
            if (window[0]->opcode == Opcode::LI &&
                window[1]->opcode == Opcode::ADD &&
                *window[0]->operands[0]->reg == *window[1]->operands[2]->reg &&
                window[0]->operands[0]->reg->isTmpReg() &&
                (window[2]->opcode == Opcode::SD || window[2]->opcode == Opcode::FSD || window[2]->opcode == Opcode::SW || window[2]->opcode == Opcode::FSW) &&
                *window[1]->operands[0]->reg == *window[2]->operands[1]->reg &&
                window[1]->operands[0]->reg->isTmpReg()) {
                int offset = window[2]->operands[1]->offset + std::stoi(window[0]->operands[1]->value);
                if (offset < 2048 && offset >= -2048) {
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, window[2]->operands[0]->reg),
                           makePtr<Operand>(Operand::Type::MEMORY, window[1]->operands[1]->reg, offset)};
                    bb->addInstruction(makePtr<Instruction>(InstructionType::S_TYPE, window[2]->opcode, ops,
                                                            Instruction::combineComment(Instruction::combineComment(window[0]->comment, window[1]->comment), window[2]->comment)));
                    changed = true;
                } else {
                    bb->addInstruction(window[0]);
                    bb->addInstruction(window[1]);
                    bb->addInstruction(window[2]);
                }
                window.clear();
            } else if (window[0]->opcode == Opcode::LI &&
                       window[1]->opcode == Opcode::ADD &&
                       *window[0]->operands[0]->reg == *window[1]->operands[2]->reg &&
                       window[0]->operands[0]->reg->isTmpReg() &&
                       (window[2]->opcode == Opcode::LD || window[2]->opcode == Opcode::FLD || window[2]->opcode == Opcode::LW || window[2]->opcode == Opcode::FLW) &&
                       *window[1]->operands[0]->reg == *window[2]->operands[1]->reg &&
                       window[1]->operands[0]->reg->isTmpReg()) {
                int offset = window[2]->operands[1]->offset + std::stoi(window[0]->operands[1]->value);
                if (offset < 2048 && offset >= -2048) {
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, window[2]->operands[0]->reg),
                           makePtr<Operand>(Operand::Type::MEMORY, window[1]->operands[1]->reg, offset)};
                    bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, window[2]->opcode, ops,
                                                            Instruction::combineComment(Instruction::combineComment(window[0]->comment, window[1]->comment), window[2]->comment)));
                    changed = true;
                } else {
                    bb->addInstruction(window[0]);
                    bb->addInstruction(window[1]);
                    bb->addInstruction(window[2]);
                }
                window.clear();
            } else if (window[0]->opcode == Opcode::LI && (window[1]->opcode == Opcode::ADD || window[1]->opcode == Opcode::ADDW || window[1]->opcode == Opcode::SUB || window[1]->opcode == Opcode::SUBW) &&
                       *window[0]->operands[0]->reg == *window[1]->operands[2]->reg &&
                       (window[0]->operands[0]->reg->isTmpReg() || *window[0]->operands[0]->reg == *window[1]->operands[0]->reg)) {
                int offset = std::stoi(window[0]->operands[1]->value);
                if (offset < 2048 && offset >= -2048) {
                    int multiplier;
                    Opcode opcode;
                    switch (window[1]->opcode) {
                        case Opcode::ADD:
                            multiplier = 1;
                            opcode = Opcode::ADDI;
                            break;
                        case Opcode::SUB:
                            multiplier = -1;
                            opcode = Opcode::ADDI;
                            break;
                        case Opcode::ADDW:
                            multiplier = 1;
                            opcode = Opcode::ADDIW;
                            break;
                        case Opcode::SUBW:
                            multiplier = -1;
                            opcode = Opcode::ADDIW;
                            break;
                        default:
                            dbgassert(false, "Unhandled case");
                            break;
                    }
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, window[1]->operands[0]->reg),
                           makePtr<Operand>(Operand::Type::REGISTER, window[1]->operands[1]->reg),
                           makePtr<Operand>(Operand::Type::IMMEDIATE, std::to_string(multiplier * std::stoi(window[0]->operands[1]->value)))};
                    bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, opcode, ops,
                                                            Instruction::combineComment(window[0]->comment, window[1]->comment)));
                    changed = true;
                } else {
                    bb->addInstruction(window[0]);
                    bb->addInstruction(window[1]);
                }
                window[0] = window[2];
                window.pop_back();
                window.pop_back();
            } else if ((window[0]->opcode == Opcode::LD || window[0]->opcode == Opcode::FLD || window[0]->opcode == Opcode::LW || window[0]->opcode == Opcode::FLW || window[0]->opcode == Opcode::LA || window[0]->opcode == Opcode::LI) &&
                       (window[1]->opcode == Opcode::MV || window[0]->opcode == Opcode::FMVS) &&
                       window[0]->operands[0]->reg->isTmpReg() &&
                       *window[1]->operands[1]->reg == *window[0]->operands[0]->reg) {
                ops = {window[1]->operands[0],
                       window[0]->operands[1]};
                bb->addInstruction(makePtr<Instruction>(window[0]->type, window[0]->opcode, ops,
                                                        Instruction::combineComment(window[0]->comment, window[1]->comment)));
                window[0] = window[2];
                window.pop_back();
                window.pop_back();
                changed = true;
            } else if ((window[2]->opcode == Opcode::ADDI || window[2]->opcode == Opcode::ADDIW) &&
                       window[2]->operands[2]->value == "0") {
                ops = {window[2]->operands[0],
                       window[2]->operands[1]};
                bb->addInstruction(window[0]);
                bb->addInstruction(window[1]);
                bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::MV, ops,
                                                        window[2]->comment));
                window.clear();
                changed = true;
            } else {
                bb->addInstruction(window[0]);
                window.erase(window.begin());
            }
        } else {
        }
    }

    void peephole(Ptr<RiscFunction> func, bool &changed) {
        Ptr<RiscBasicBlock> prevBB = nullptr;
        for (auto &bb : func->bbs()) {
            if (prevBB && prevBB->insts().size() > 0) {
                auto it = prevBB->insts().back();
                if (it->opcode == Opcode::J && it->operands[0]->value == bb->tag()) {
                    prevBB->removeBack();
                    changed = true;
                }
            }
            auto insts = bb->insts();
            bb->clearInstruction();
            std::vector<Ptr<Instruction>> window;
            const size_t windowSize = 3;

            for (auto it = insts.begin(); it != insts.end(); ++it) {
                window.push_back(*it);
                auto newInst = *it;
                if (newInst->opcode == Opcode::MV || newInst->opcode == Opcode::FMVS) {
                    if (*newInst->operands[0]->reg == *newInst->operands[1]->reg) {
                        window.pop_back();
                        changed = true;
                    }
                }

                if (window.size() == windowSize) {
                    optimizeWindow(window, bb, changed);
                }
            }

            if (!window.empty()) {
                for (auto inst : window) {
                    bb->addInstruction(inst);
                }
            }
            prevBB = bb;
        }
    }

} // namespace backend

#endif
