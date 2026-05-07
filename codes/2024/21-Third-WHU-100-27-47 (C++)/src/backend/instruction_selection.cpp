#ifndef BACKEND_INSTRUCTIONSELECTION_CPP_
#define BACKEND_INSTRUCTIONSELECTION_CPP_

#include "Common.h"
#include "instruction_selection.h"

namespace backend {

    // Get the next argument register or memory for a function call
    Ptr<Operand> getNextArgOperand(Ptr<RiscFunction> func, int generalArgCount, int floatArgCount, bool isFloatArg) {
        if (isFloatArg) {
            if (floatArgCount < 8) {
                return makePtr<Operand>(Operand::Type::REGISTER, REG_ARGS_FLOAT[floatArgCount]);
            } else {
                func->stackFrame.updateMaxInnerMemArgSizeIfLarger(((floatArgCount + 1) - 8 + MAX(0, generalArgCount - 8)) * 8);
                return makePtr<Operand>(Operand::Type::MEMORY, sp, (floatArgCount - 8 + MAX(0, generalArgCount - 8)) * 8);
            }
        } else {
            if (generalArgCount < 8) {
                return makePtr<Operand>(Operand::Type::REGISTER, REG_ARGS_GENERAL[generalArgCount]);
            } else {
                func->stackFrame.updateMaxInnerMemArgSizeIfLarger(((generalArgCount + 1) - 8 + MAX(0, floatArgCount - 8)) * 8);
                return makePtr<Operand>(Operand::Type::MEMORY, sp, (generalArgCount - 8 + MAX(0, floatArgCount - 8)) * 8);
            }
        }
    }

    Ptr<Instruction> storeFromTo64(Ptr<RiscBasicBlock> &bb, const Ptr<Operand> &src, bool srcIsFloatImm, const Ptr<Operand> &dest) {
        return storeFromTo(bb, src, srcIsFloatImm, dest, true);
    }

    // Load an immediate or register or memory into a register or memory
    Ptr<Instruction> storeFromTo(Ptr<RiscBasicBlock> &bb, const Ptr<Operand> &src, bool srcIsFloatImm, const Ptr<Operand> &dest, bool dword) {
        std::vector<Ptr<Operand>> ops{};
        Ptr<RiscFunction> func = bb->parentFunc();
        if (src->type == Operand::Type::REGISTER) {
            // src is reg
            auto srcReg = src->reg;
            if (srcReg->getType() == Register::Type::FLOAT) {
                // src is float reg
                if (dest->type == Operand::Type::REGISTER) {
                    // src is float reg, dest is reg
                    auto destReg = dest->reg;
                    if (destReg->getType() == Register::Type::FLOAT) {
                        // src is float reg, dest is float reg (leaf 1)
                        ops = {dest, src};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::FMVS, ops, "Copy float from float reg to float reg"));
                    } else {
                        // src is float reg, dest is general reg (leaf 2)
                        ops = {dest, src};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FMVXS, ops, "Copy float from float reg to general reg"));
                    }
                } else if (dest->type == Operand::Type::MEMORY) {
                    // src is float reg, dest is memory (leaf 3)
                    ops = {src, dest};
                    return bb->addInstruction(makePtr<Instruction>(InstructionType::S_TYPE, dword ? Opcode::FSD : Opcode::FSW, ops, "Store float from float reg to memory"));
                } else {
                    dbgassert(false, "Unsupported operand type for dest");
                    return nullptr;
                }
            } else {
                // src is general reg
                if (dest->type == Operand::Type::REGISTER) {
                    // src is general reg, dest is reg
                    auto destReg = dest->reg;
                    if (destReg->getType() == Register::Type::FLOAT) {
                        // src is general reg, dest is float reg (leaf 4)
                        ops = {dest, src};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FMVSX, ops, "Copy float from general reg to float reg"));
                    } else {
                        // src is general reg, dest is general reg (leaf 5)
                        ops = {dest, src};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::MV, ops, "Copy integer from general reg to general reg"));
                    }
                } else if (dest->type == Operand::Type::MEMORY) {
                    // src is general reg, dest is memory (leaf 6)
                    ops = {src, dest};
                    return bb->addInstruction(makePtr<Instruction>(InstructionType::S_TYPE, dword ? Opcode::SD : Opcode::SW, ops, "Store integer from general reg to memory"));
                } else {
                    dbgassert(false, "Unsupported operand type for dest");
                    return nullptr;
                }
            }
        } else if (src->type == Operand::Type::IMMEDIATE) {
            // src is immediate
            auto imm = src->value;
            if (srcIsFloatImm) {
                // src is float immediate
                int32_t floatBits = floatToSigned(imm);

                if (dest->type == Operand::Type::REGISTER) {
                    // src is float immediate, dest is reg
                    auto destReg = dest->reg;
                    if (destReg->getType() == Register::Type::FLOAT) {
                        // src is float immediate, dest is float reg (leaf 7)
                        auto tmpReg = REG_TEMP_GENERAL[0];
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                               makePtr<Operand>(Operand::Type::IMMEDIATE, std::to_string(floatBits))};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Load float immediate into temp reg"));
                        ops = {dest,
                               makePtr<Operand>(Operand::Type::REGISTER, tmpReg)};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FMVSX, ops, "Then copy the float immediate to float reg"));
                    } else {
                        // src is float immediate, dest is general reg (leaf 8)
                        ops = {dest,
                               makePtr<Operand>(Operand::Type::IMMEDIATE, std::to_string(floatBits))};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Load float immediate into general reg"));
                    }
                } else if (dest->type == Operand::Type::MEMORY) {
                    // src is float immediate, dest is memory (leaf 9)
                    auto tmpReg = REG_TEMP_GENERAL[0];
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                           makePtr<Operand>(Operand::Type::IMMEDIATE, std::to_string(floatBits))};
                    bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Load float immediate into temp reg"));
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                           dest};
                    return bb->addInstruction(makePtr<Instruction>(InstructionType::S_TYPE, dword ? Opcode::SD : Opcode::SW, ops, "Then store float immediate to memory"));
                } else {
                    dbgassert(false, "Unsupported operand type for dest");
                    return nullptr;
                }
            } else {
                // src is integer immediate
                if (dest->type == Operand::Type::REGISTER) {
                    // src is integer immediate, dest is reg
                    auto destReg = dest->reg;
                    if (destReg->getType() == Register::Type::FLOAT) {
                        // src is integer immediate, dest is float reg (leaf 10)
                        // src should be an integer representation of a float immediate
                        auto tmpReg = REG_TEMP_GENERAL[0];
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                               makePtr<Operand>(Operand::Type::IMMEDIATE, imm)};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Load integer representation of float immediate into temp reg"));
                        ops = {dest,
                               makePtr<Operand>(Operand::Type::REGISTER, tmpReg)};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::FMVSX, ops, "Then copy the float immediate to float reg"));
                    } else {
                        // src is integer immediate, dest is general reg (leaf 11)
                        ops = {dest,
                               makePtr<Operand>(Operand::Type::IMMEDIATE, imm)};
                        return bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Load integer immediate into general reg"));
                    }
                } else if (dest->type == Operand::Type::MEMORY) {
                    // src is integer immediate, dest is memory (leaf 12)
                    auto tmpReg = REG_TEMP_GENERAL[0];
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                           makePtr<Operand>(Operand::Type::IMMEDIATE, imm)};
                    bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Load integer immediate into temp reg"));
                    ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                           dest};
                    return bb->addInstruction(makePtr<Instruction>(InstructionType::S_TYPE, dword ? Opcode::SD : Opcode::SW, ops, "Then store the integer immediate to memory"));
                } else {
                    dbgassert(false, "Unsupported operand type for dest");
                    return nullptr;
                }
            }
        } else if (src->type == Operand::Type::MEMORY) {
            // src is memory
            if (dest->type == Operand::Type::REGISTER) {
                // src is memory, dest is reg
                auto destReg = dest->reg;
                if (destReg->getType() == Register::Type::FLOAT) {
                    // src is memory, dest is float reg (leaf 13)
                    ops = {dest, src};
                    return bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, dword ? Opcode::FLD : Opcode::FLW, ops, "Load float from memory into float reg"));
                } else {
                    // src is memory, dest is general reg (leaf 14)
                    ops = {dest, src};
                    return bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, dword ? Opcode::LD : Opcode::LW, ops, "Load integer from memory into general reg"));
                }
            } else if (dest->type == Operand::Type::MEMORY) {
                // src is memory, dest is memory (leaf 15)
                auto tmpReg = REG_TEMP_GENERAL[0];
                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                       src};
                bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, dword ? Opcode::LD : Opcode::LW, ops, "Load integer from memory into temp reg"));
                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpReg),
                       dest};
                return bb->addInstruction(makePtr<Instruction>(InstructionType::S_TYPE, dword ? Opcode::SD : Opcode::SW, ops, "Then store the integer from temp reg to memory"));
            } else {
                dbgassert(false, "Unsupported operand type for dest");
                return nullptr;
            }
        } else {
            dbgassert(false, "Unsupported operand type for src");
            return nullptr;
        }
    }

    Ptr<Instruction> storeValueTo64(Ptr<RiscBasicBlock> &bb, const ir::Value &irValue, const Ptr<Operand> &dest) {
        return storeValueTo(bb, irValue, dest, true);
    }

    // Load a value into a register or memory
    Ptr<Instruction> storeValueTo(Ptr<RiscBasicBlock> &bb, const ir::Value &irValue, const Ptr<Operand> &dest, bool dword) {
        std::vector<Ptr<Operand>> ops{};
        Ptr<RiscFunction> func = bb->parentFunc();
        if (irValue.isRegister()) {
            return storeFromTo(bb, makePtr<Operand>(Operand::Type::REGISTER, func->getOrCreateVirtualRegister(irValue.getRegister())), false, dest, dword);
        } else if (irValue.isLiteral()) {
            return storeFromTo(bb, makePtr<Operand>(Operand::Type::IMMEDIATE, irValue.getLiteral().toString()), irValue.getLiteral().isFloat(), dest, dword);
        } else {
            dbgassert(false, "Unsupported literal type");
            return nullptr;
        }
    }

    // Represent arguments with virtual registers
    void mapArguments(Ptr<RiscFunction> func) {
        std::vector<Ptr<Operand>> ops;

        auto entryBB = func->bbs()[0];

        auto &irParams = func->sourceFunc()->paramList();
        int generalArgCount = 0;
        int floatArgCount = 0;
        for (int i = 0; i < irParams.size(); ++i) {
            auto irParam = irParams[i];
            bool isFloatArg = irParam->dataType() == PrimaryDataType::Float;
            auto argReg = func->getOrCreateVirtualRegister(irParam);
            auto argOp = getNextArgOperand(func, generalArgCount, floatArgCount, isFloatArg);
            if (argOp->type == Operand::Type::REGISTER) {
                argReg->assignedReg = argOp->reg;
                argReg->boundToReg = true;
            } else if (argOp->type == Operand::Type::MEMORY) {
                argReg->memArgOffset = argOp->offset;
            } else {
                dbgassert(false, "Invalid argument operand");
            }
            if (isFloatArg) {
                ++floatArgCount;
            } else {
                ++generalArgCount;
            }
        }
    }

    // main function to generate Risc code and store in machine level classes
    void generateInst(Ptr<RiscFunction> func) {
        for (auto &bb : func->bbs()) {
            ir::BBPtr irBB = bb->sourceBB();

            for (auto &irInst : irBB->getInstTopoList()) {
                instructionSelection(irInst, bb);
            }
        }
    }

    void fillOffsets(Ptr<RiscFunction> func) {
        Vector<Ptr<Operand>> ops;
        for (auto &bb : func->bbs()) {
            for (auto &inst : bb->insts()) {
                for (auto &operand : inst->operands) {
                    switch (operand->type) {
                        case Operand::Type::CALLEE_SAVED_REG_OFFSET: {
                            operand->type = Operand::Type::IMMEDIATE;
                            break;
                        }
                        case Operand::Type::CALLER_SAVED_REG_OFFSET: {
                            operand->type = Operand::Type::IMMEDIATE;
                            operand->value = std::to_string(func->stackFrame.getOffsetForCallerSavedReg(std::stoi(operand->value)));
                            break;
                        }
                        case Operand::Type::SPILL_OFFSET: {
                            operand->type = Operand::Type::IMMEDIATE;
                            operand->value = std::to_string(func->stackFrame.getOffsetForSpill(std::stoi(operand->value)));
                            break;
                        }
                        case Operand::Type::MEMVAR_OFFSET: {
                            operand->type = Operand::Type::IMMEDIATE;
                            operand->value = std::to_string(func->stackFrame.getOffsetForMemVar(std::stoi(operand->value)));
                            break;
                        }
                        case Operand::Type::INNER_MEMARG_OFFSET: {
                            operand->type = Operand::Type::IMMEDIATE;
                            break;
                        }
                        case Operand::Type::MEMARG_OFFSET: {
                            operand->type = Operand::Type::IMMEDIATE;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
        }
    }

    void saveAndRestoreCallSites(Ptr<RiscFunction> func) {
        Vector<Ptr<Operand>> ops;

        Map<Ptr<Instruction>, std::set<Ptr<Register>, Register::RegisterPtrHash>> callSiteSaveSet{}; // Physical registers to be saved for call site
        Map<Ptr<Instruction>, std::set<Ptr<Register>, Register::RegisterPtrHash>> callSiteLiveSet{}; // Live physical registers for call site, for deciding which saver registers cannot be used
        for (auto &bb : func->bbs()) {
            // Analyze live physical registers and gather physical registers to be saved at call sites
            // Physical registers needed to be saved at call sites are:
            // 1. Caller-saved registers that are live at the call instruction
            // 2. Registers both used as formal arguments and real arguments (because some may cause conflicts)
            {
                auto &lvOut = bb->getLiveOutSet();
                std::set<Ptr<Register>, Register::RegisterPtrHash> curLive{};
                // Initialize curLive
                for (auto liveVReg : lvOut) {
                    if (liveVReg->isAssigned()) {
                        curLive.insert(liveVReg->assignedReg);
                    }
                }
                std::set<Ptr<Register>, Register::RegisterPtrHash> curCallSiteLive{};
                std::set<Ptr<Register>, Register::RegisterPtrHash> callerSavedLiveAtCall{};
                std::set<Ptr<Register>, Register::RegisterPtrHash> paramRegs{};
                std::set<Ptr<Register>, Register::RegisterPtrHash> argRegs{};
                bool passingArgs = false;
                for (auto instIt = bb->insts().rbegin(); instIt != bb->insts().rend(); ++instIt) {
                    auto inst = *instIt;
                    if (inst->opcode == Opcode::CALL) {
                        callerSavedLiveAtCall.clear();
                        for (auto live : curLive) {
                            if (REG_CALLER_SAVED.find(live) != REG_CALLER_SAVED.end()) {
                                callerSavedLiveAtCall.insert(live);
                            }
                        }
                        passingArgs = true;
                        paramRegs.clear();
                        argRegs.clear();
                    } else if (inst->opcode == Opcode::NOP && inst->operands[0]->value == "AFTER_CALL") {
                        curCallSiteLive = curLive;
                        passingArgs = false;
                    } else if (inst->opcode == Opcode::NOP && inst->operands[0]->value == "BEFORE_CALL") {
                        auto &liveSet = callSiteLiveSet[inst];
                        liveSet = curCallSiteLive;
                        liveSet.merge(curLive);
                        auto &saveSet = callSiteSaveSet[inst];
                        saveSet = callerSavedLiveAtCall;
                        for (auto argReg : paramRegs) {
                            if (argRegs.find(argReg) != argRegs.end()) {
                                saveSet.insert(argReg);
                            }
                        }
                        passingArgs = false;
                    } else {
                        if (passingArgs) {
                            Ptr<Register> param = nullptr, arg = nullptr;
                            if (inst->type == InstructionType::S_TYPE) {
                                // sw/fsw/sd/fsd/...
                                if (inst->operands[0]->reg->isVirtual) {
                                    arg = inst->operands[0]->reg;
                                }
                            } else {
                                // mv/fmvs/fmvxs/li/la/...
                                param = inst->operands[0]->reg;
                                if (inst->operands[1]->type == Operand::Type::REGISTER && inst->operands[1]->reg->isVirtual) {
                                    arg = inst->operands[1]->reg;
                                }
                            }
                            if (param) {
                                dbgassert(!param->isVirtual, "Formal param should not be virtual");
                                paramRegs.insert(param);
                            }
                            if (arg) {
                                dbgassert(arg->isVirtual, "Real arg should be virtual");
                                auto argVReg = castPtr<VirtualRegister>(arg);
                                if (argVReg->isAssigned()) {
                                    argRegs.insert(argVReg->assignedReg);
                                    curCallSiteLive.insert(argVReg->assignedReg);
                                }
                            }
                        }
                        // Update live set
                        // Reverse iteration, so defs will be covered by uses
                        for (auto def : inst->regsWritten()) {
                            if (def->isVirtual && castPtr<VirtualRegister>(def)->isAssigned()) {
                                def = castPtr<VirtualRegister>(def)->assignedReg;
                                curLive.erase(def);
                            }
                        }
                        for (auto use : inst->regsRead()) {
                            if (use->isVirtual && castPtr<VirtualRegister>(use)->isAssigned()) {
                                use = castPtr<VirtualRegister>(use)->assignedReg;
                                curLive.insert(use);
                            }
                        }
                    }
                }
            }

            // Do save and restore
            // Lazy restore: only restore registers before read/write or reaching the end of bb
            {
                auto insts = bb->insts(); // Get a copy of insts
                bb->insts().clear();

                std::set<Ptr<Register>, Register::RegisterPtrHash> savedRegs{};                   // Currently saved registers
                std::map<Ptr<Register>, Ptr<Register>, Register::RegisterPtrHash> saveRegister{}; // Saver register for register-saved register
                std::map<Ptr<Register>, int, Register::RegisterPtrHash> saveOffset{};             // Offset for stack-saved register
                bool handlingRetVal = false;                                                      // If true, it's time to handle return value
                bool passingArgs = false;                                                         // If true, it's time to pass arguments
                Vector<Ptr<Instruction>> instBuffer{};                                            // Buffer for instructions outside of call sites

                // Allocation
                Set<int> callerSavedStackAllocSet{};
                auto allocStackForCallerSavedReg = [&]() {
                    const int size = 8; // 8-byte alignment
                    for (int offset = 0;; offset += size) {
                        if (callerSavedStackAllocSet.find(offset) == callerSavedStackAllocSet.end()) {
                            callerSavedStackAllocSet.insert(offset);
                            func->stackFrame.updateCallerSavedRegSizeIfLarger(offset + size);
                            return offset;
                        }
                    }
                };
                auto deallocStackForCallerSavedReg = [&](int offset) {
                    callerSavedStackAllocSet.erase(offset);
                };

                auto restore = [&](Ptr<Register> reg, bool restoreValue = true) {
                    dbgassert(saveRegister.find(reg) != saveRegister.end() || saveOffset.find(reg) != saveOffset.end(), "Saved register should be saved saver register or stack");
                    if (saveRegister.find(reg) != saveRegister.end()) {
                        // Restore from saver register
                        auto saver = saveRegister[reg];

                        if (restoreValue) {
                            // mv/fmvs saved, saver
                            InstructionType type;
                            Opcode opcode;
                            if (reg->getType() == Register::Type::FLOAT) {
                                type = InstructionType::PSEUDO;
                                opcode = Opcode::FMVS;
                            } else {
                                type = InstructionType::PSEUDO;
                                opcode = Opcode::MV;
                            }
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, reg),
                                   makePtr<Operand>(Operand::Type::REGISTER, saver)};
                            bb->addInstruction(makePtr<Instruction>(type, opcode, ops, "Restore caller-saved register from saver register"));
                        }

                        saveRegister.erase(reg);
                    } else {
                        // Restore from stack
                        auto offset = saveOffset[reg];

                        if (restoreValue) {
                            // ld saved, offset(s0)
                            auto tmpAddrReg = REG_TEMP_GENERAL[1];
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                                   makePtr<Operand>(Operand::Type::CALLER_SAVED_REG_OFFSET, std::to_string(offset))};
                            bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Prepare source stack offset for restoring site"));
                            ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                                   makePtr<Operand>(Operand::Type::REGISTER, s0),
                                   makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg)};
                            bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::ADD, ops, "Then calculate source address")); // 64-bit pointer addition
                            if (reg->getType() == Register::Type::FLOAT) {
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, reg),
                                       makePtr<Operand>(Operand::Type::MEMORY, tmpAddrReg, 0)};
                                bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, Opcode::FLD, ops, "Restore caller-saved float register from stack"));
                            } else {
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, reg),
                                       makePtr<Operand>(Operand::Type::MEMORY, tmpAddrReg, 0)};
                                bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, Opcode::LD, ops, "Restore caller-saved general register from stack"));
                            }
                        }

                        deallocStackForCallerSavedReg(offset);
                        saveOffset.erase(reg);
                    }

                    savedRegs.erase(reg);
                };

                auto getPhysicalReg = [&](Ptr<Register> reg) -> Ptr<Register> {
                    if (reg->isVirtual) {
                        // Get its physical register (if any)
                        auto vreg = castPtr<VirtualRegister>(reg);
                        if (!vreg->isAssigned()) {
                            return nullptr;
                        }
                        reg = vreg->assignedReg;
                    }
                    return reg;
                };

                auto isSaved = [&](Ptr<Register> reg) -> Ptr<Register> {
                    reg = getPhysicalReg(reg);
                    return reg && savedRegs.find(reg) != savedRegs.end() ? reg : nullptr;
                };

                auto isSaver = [&](Ptr<Register> reg) -> Ptr<Register> {
                    reg = getPhysicalReg(reg);
                    if (reg) {
                        for (auto [saved, saver] : saveRegister) {
                            if (*saver == *reg) {
                                return saved;
                            }
                        }
                    }
                    return nullptr;
                };

                auto insertBufferedInsts = [&]() {
                    for (auto inst : instBuffer) {
                        bb->addInstruction(inst);
                    }
                    instBuffer.clear();
                };

                // Reinsert instructions
                for (auto &inst : insts) {
                    if (inst->opcode == Opcode::NOP && inst->operands[0]->value == "BEFORE_CALL") {
                        // Empty the instruction buffer before entering a call site
                        insertBufferedInsts();

                        for (auto reg : callSiteSaveSet[inst]) {
                            savedRegs.insert(reg);
                        }
                        auto callee = inst->operands[1]->func;
                        auto saverGRegs = REG_SAVE_GENERAL; // General saver registers
                        auto saverFRegs = REG_SAVE_FLOAT;   // Float saver registers
                        for (auto live : callSiteLiveSet[inst]) {
                            saverGRegs.erase(live);
                            saverFRegs.erase(live);
                        }
                        for (auto [reg, saver] : saveRegister) {
                            saverGRegs.erase(saver);
                            saverFRegs.erase(saver);
                        }
                        for (auto reg : savedRegs) {
                            if (saveRegister.find(reg) != saveRegister.end() || saveOffset.find(reg) != saveOffset.end()) {
                                // Already saved, avoid double save
                                continue;
                            }
                            Ptr<Register> saver = nullptr;
                            if (reg->getType() == Register::Type::FLOAT) {
                                if (!saverFRegs.empty()) {
                                    saver = *saverFRegs.begin();
                                    saveRegister[reg] = saver;
                                    saverFRegs.erase(saver);
                                }
                            } else {
                                if (!saverGRegs.empty()) {
                                    saver = *saverGRegs.begin();
                                    saveRegister[reg] = saver;
                                    saverGRegs.erase(saver);
                                }
                            }
                            if (saver) {
                                // Use saver register to save

                                // mv/fmvs saver, saved
                                InstructionType type;
                                Opcode opcode;
                                if (reg->getType() == Register::Type::FLOAT) {
                                    type = InstructionType::PSEUDO;
                                    opcode = Opcode::FMVS;
                                } else {
                                    type = InstructionType::PSEUDO;
                                    opcode = Opcode::MV;
                                }
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, saver),
                                       makePtr<Operand>(Operand::Type::REGISTER, reg)};
                                bb->addInstruction(makePtr<Instruction>(type, opcode, ops, "Save caller-saved register to saver register"));
                            } else {
                                // Use stack to save
                                int offset = allocStackForCallerSavedReg();
                                saveOffset[reg] = offset;

                                // sd saved, offset(s0)
                                auto tmpAddrReg = REG_TEMP_GENERAL[1];
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                                       makePtr<Operand>(Operand::Type::CALLER_SAVED_REG_OFFSET, std::to_string(offset))};
                                bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Prepare destination stack offset for preserving site"));
                                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                                       makePtr<Operand>(Operand::Type::REGISTER, s0),
                                       makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg)};
                                bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::ADD, ops, "Then calculate destination address")); // 64-bit pointer addition
                                if (reg->getType() == Register::Type::FLOAT) {
                                    ops = {makePtr<Operand>(Operand::Type::REGISTER, reg),
                                           makePtr<Operand>(Operand::Type::MEMORY, tmpAddrReg, 0)};
                                    bb->addInstruction(makePtr<Instruction>(InstructionType::S_TYPE, Opcode::FSD, ops, "Save caller-saved float register to stack"));
                                } else {
                                    ops = {makePtr<Operand>(Operand::Type::REGISTER, reg),
                                           makePtr<Operand>(Operand::Type::MEMORY, tmpAddrReg, 0)};
                                    bb->addInstruction(makePtr<Instruction>(InstructionType::S_TYPE, Opcode::SD, ops, "Save caller-saved general register to stack"));
                                }
                            }
                        }
                        passingArgs = true;
                    } else if (inst->opcode == Opcode::NOP && inst->operands[0]->value == "AFTER_CALL") {
                        handlingRetVal = false;
                    } else {
                        if (handlingRetVal) {
                            dbgassert(!passingArgs, "Should not be passing arguments when handling return value");
                            // Should only be a move instruction after the call instruction, which handles the return value
                            dbgassert(inst->opcode == Opcode::MV || inst->opcode == Opcode::FMVS, "Should be a move instruction");
                            dbgassert(!inst->operands[1]->reg->isVirtual, "Return value register should not be virtual");
                            dbgassert(inst->operands[0]->reg->isVirtual, "Return value storer register should be virtual");
                            auto retValReg = inst->operands[1]->reg;                              // The return value register (a0/fa0)
                            auto retValStorer = castPtr<VirtualRegister>(inst->operands[0]->reg); // The register which holds the return value from retValReg, which should not be restored
                            // Should not restore the return value storer register afterwards, so erase it from the saved set if it's in it
                            if (auto saved = isSaved(retValStorer)) {
                                restore(saved, false);
                            }
                            // Restore the return value storer register if it is currently a saver
                            // If it saves the value of the return value register (a0/fa0) by coincidence, we have to introduce a temporary register
                            if (auto saved = isSaver(retValStorer)) {
                                if (*saved == *retValReg) {
                                    InstructionType type = InstructionType::PSEUDO;
                                    Opcode opcode = retValReg->getType() == Register::Type::FLOAT ? Opcode::FMVS : Opcode::MV;
                                    auto tmpRetValReg = retValReg->getType() == Register::Type::FLOAT ? REG_TEMP_FLOAT[1] : REG_TEMP_GENERAL[1];

                                    // mv/fmvs tmpRetValReg, retValReg
                                    ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpRetValReg),
                                           makePtr<Operand>(Operand::Type::REGISTER, retValReg)};
                                    bb->addInstruction(makePtr<Instruction>(type, opcode, ops, "Store return value to temporary register"));

                                    // mv/fmvs retValStorer, tmpRetValReg
                                    inst->operands[1]->reg = tmpRetValReg;
                                }
                                restore(saved);
                            }
                            bb->addInstruction(inst);
                        } else if (inst->opcode == Opcode::CALL) {
                            dbgassert(passingArgs && !handlingRetVal, "Should be passing arguments and not handling return value at the call instruction");
                            // After the call instruction, the return value is in a0/fa0, so we need to save it into the dest reg (if not saved) or the corresponding stack slot of the dest reg (if saved)
                            handlingRetVal = true;
                            passingArgs = false;
                            bb->addInstruction(inst);
                        } else if (passingArgs) {
                            dbgassert(!handlingRetVal, "Should not be handling return value when passing arguments");
                            // Replace saved registers as arguments with savers or stack loads
                            auto regsRead = inst->regsRead();
                            std::set<Ptr<Register>, Register::RegisterPtrHash> regsReadSet{regsRead.begin(), regsRead.end()};
                            for (auto &operand : inst->operands) {
                                if (operand->type == Operand::Type::REGISTER || operand->type == Operand::Type::MEMORY) {

                                    if (regsReadSet.find(operand->reg) != regsReadSet.end()) {
                                        // Used as a source operand

                                        Ptr<Register> use = nullptr;
                                        if (operand->reg->isVirtual) {
                                            use = castPtr<VirtualRegister>(operand->reg)->assignedReg;
                                        }
                                        if (!use) {
                                            continue;
                                        }

                                        if (saveRegister.find(use) != saveRegister.end()) {
                                            // Saved in saver register, so replace it with saved register
                                            auto saver = saveRegister[use];
                                            operand->reg = saver;
                                        } else if (saveOffset.find(use) != saveOffset.end()) {
                                            // Saved in stack, so replace it with stack load
                                            int offset = saveOffset[use];

                                            auto tmpAddrReg = REG_TEMP_GENERAL[1];
                                            ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                                                   makePtr<Operand>(Operand::Type::CALLER_SAVED_REG_OFFSET, std::to_string(offset))};
                                            bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, "Prepare source stack offset of saved register"));
                                            ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                                                   makePtr<Operand>(Operand::Type::REGISTER, s0),
                                                   makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg)};
                                            bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::ADD, ops, "Then calculate source address")); // 64-bit pointer addition
                                            Ptr<Register> tmpValueReg;
                                            if (use->getType() == Register::Type::FLOAT) {
                                                tmpValueReg = REG_TEMP_FLOAT[1];
                                                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpValueReg),
                                                       makePtr<Operand>(Operand::Type::MEMORY, tmpAddrReg, 0)};
                                                bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, Opcode::FLD, ops, "Load caller-saved float register from stack"));
                                            } else {
                                                tmpValueReg = REG_TEMP_GENERAL[1];
                                                ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpValueReg),
                                                       makePtr<Operand>(Operand::Type::MEMORY, tmpAddrReg, 0)};
                                                bb->addInstruction(makePtr<Instruction>(InstructionType::I_TYPE, Opcode::LD, ops, "Load caller-saved general register from stack"));
                                            }
                                            operand->reg = tmpValueReg;
                                        }
                                    }
                                }
                            }
                            bb->addInstruction(inst);
                        } else {
                            // Instruction outside of call sites
                            // Check if any used register is saved, and if so, restore it at the end of the latest call site (restoration instructions are inserted at the end of the latest call site, because the new instructions are buffered and not inserted yet)
                            for (auto reg : inst->regsRead()) {
                                if (auto saved = isSaved(reg)) {
                                    restore(saved);
                                }
                                if (auto saved = isSaver(reg)) {
                                    restore(saved);
                                }
                            }
                            for (auto reg : inst->regsWritten()) {
                                if (auto saved = isSaved(reg)) {
                                    restore(saved, false); // No need to restore the value of the register, because it is soon overwritten
                                }
                                if (auto saved = isSaver(reg)) {
                                    restore(saved);
                                }
                            }
                            // Buffer the instruction
                            instBuffer.push_back(inst);
                        }
                    }
                }
                // Get live out physical registers
                auto _savedRegs = savedRegs;
                auto &lvOutVRegs = bb->getLiveOutSet();
                std::set<Ptr<Register>, Register::RegisterPtrHash> lvOut{}; // Physical registers that are live out
                for (auto liveVReg : lvOutVRegs) {
                    if (liveVReg->isAssigned()) {
                        lvOut.insert(liveVReg->assignedReg);
                    }
                }
                // Restore remaining saved live out registers at the end of the latest call site
                for (auto reg : _savedRegs) {
                    if (lvOut.find(reg) != lvOut.end()) {
                        restore(reg);
                    }
                }
                // Empty the instruction buffer at the end of bb
                insertBufferedInsts();
            }
        }
    }

    // regard one Inst as a single node of DAG
    void instructionSelection(const ir::InstPtr &node, Ptr<RiscBasicBlock> &bb) {
        Ptr<Instruction> coreInst = nullptr;
        if (auto irMoveInst = castPtr<ir::MoveInst>(node)) {
            coreInst = irMoveInstMapper(irMoveInst, bb);
        } else if (auto irCallInst = castPtr<ir::CallInst>(node)) {
            coreInst = irCallInstMapper(irCallInst, bb);
        } else if (auto irLoadInst = castPtr<ir::LoadInst>(node)) {
            coreInst = irLoadInstMapper(irLoadInst, bb);
        } else if (auto irStoreInst = castPtr<ir::StoreInst>(node)) {
            coreInst = irStoreInstMapper(irStoreInst, bb);
        } else if (auto irCastInst = castPtr<ir::CastInst>(node)) {
            coreInst = irCastInstMapper(irCastInst, bb);
        } else if (auto irExitInst = castPtr<ir::ExitInst>(node)) {
            if (irExitInst->isBranch()) {
                coreInst = irBrInstMapper(irExitInst, bb);
            } else if (!irExitInst->isFuncExit()) {
                dbgassert(false, "Invalid exit instruction");
            }
        } else if (auto irRetInst = castPtr<ir::RetInst>(node)) {
            coreInst = irRetInstMapper(irRetInst, bb);
        } else if (auto irOperInst = castPtr<ir::OperInst>(node)) {
            coreInst = irOperInstMapper(irOperInst, bb);
        } else if (auto irAllocInst = castPtr<ir::AllocInst>(node)) {
            coreInst = irAllocInstMapper(irAllocInst, bb);
        } else if (auto irGepInst = castPtr<ir::GEPInst>(node)) {
            coreInst = irGepInstMapper(irGepInst, bb);
        }
        if (coreInst && !bb->insts().empty()) {
            coreInst->comment = Instruction::combineComment(coreInst->comment, node->toString());
        }
    }
}

#endif
