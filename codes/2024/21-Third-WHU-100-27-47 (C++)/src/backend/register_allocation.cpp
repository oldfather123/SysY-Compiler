#ifndef BACKEND_REGISTER_ALLOCATION_CPP_
#define BACKEND_REGISTER_ALLOCATION_CPP_

#include "register_allocation.h"

namespace backend {

    void findReadWrittenRegs(Ptr<RiscFunction> func) {
        func->regsWritten.insert(s0);

        for (auto bb : func->bbs()) {
            for (auto inst : bb->insts()) {
                for (auto use : inst->regsRead()) {
                    if (auto vreg = castPtr<VirtualRegister>(use)) {
                        if (vreg->isAssigned()) {
                            func->regsRead.insert(vreg->assignedReg);
                        }
                    } else {
                        func->regsRead.insert(use);
                    }
                }
                for (auto def : inst->regsWritten()) {
                    if (auto vreg = castPtr<VirtualRegister>(def)) {
                        if (vreg->isAssigned()) {
                            func->regsWritten.insert(vreg->assignedReg);
                        }
                    } else {
                        func->regsWritten.insert(def);
                    }
                }
            }
        }

        for (auto reg : func->regsWritten) {
            if (REG_CALLEE_SAVED.find(reg) != REG_CALLEE_SAVED.end()) {
                func->stackFrame.allocCalleeSavedReg(8);
                func->modifiedCalleeSavedRegs.push_back(reg);
            }
        }
    }

    // Fill spill/array virtual registers with stack offsets, global data virtual registers with global data addresses
    void fillVirtualRegs(Ptr<RiscFunction> func) {
        for (auto bb : func->bbs()) {
            auto insts = bb->insts(); // Get a copy of insts
            bb->insts().clear();
            // Reinsert instructions
            for (auto inst : insts) {
                Vector<std::pair<Ptr<VirtualRegister>, Ptr<Register>>> loads{}, stores{};

                int operandIndex = -1;
                for (auto &operand : inst->operands) {
                    ++operandIndex;
                    if (operand->type == Operand::Type::REGISTER || operand->type == Operand::Type::MEMORY) {
                        auto vreg = castPtr<VirtualRegister>(operand->reg);
                        if (!vreg) {
                            continue;
                        }
                        // Fill in with assigned physical register
                        if (vreg->getNumber() == -1) { // ?
                            if (vreg->getType() == Register::Type::FLOAT) {
                                operand->reg = REG_SPILL_FLOAT[0];
                            } else {
                                operand->reg = REG_SPILL_GENERAL[0];
                            }
                        } else {
                            if (vreg->isAssigned()) {
                                if (vreg->getType() == Register::Type::GENERAL) {
                                    operand->reg = vreg->assignedReg;
                                } else if (vreg->getType() == Register::Type::FLOAT) {
                                    operand->reg = vreg->assignedReg;
                                }
                            } else {
                                dbgassert(vreg->isAllocated(), "Register not allocated");
                            }
                        }
                        // Fill in with memory address and add memory operations
                        if (vreg->isSpill() || (vreg->isArg() && vreg->memArgOffset != -1) || vreg->isGlobal()) {
                            Ptr<Register> tmpValueReg;
                            if (inst->type == InstructionType::S_TYPE) {
                                tmpValueReg = (vreg->getType() == Register::Type::GENERAL ? REG_TEMP_GENERAL : REG_TEMP_FLOAT)[operandIndex == 0 ? 1 : 2];
                            } else {
                                tmpValueReg = (vreg->getType() == Register::Type::GENERAL ? REG_TEMP_GENERAL : REG_TEMP_FLOAT)[operandIndex < 2 ? 1 : 2];
                            }
                            auto regsRead = inst->regsRead();
                            if (std::find(regsRead.begin(), regsRead.end(), vreg) != regsRead.end()) {
                                loads.push_back({vreg, tmpValueReg});
                            }
                            auto regsWritten = inst->regsWritten();
                            if (std::find(regsWritten.begin(), regsWritten.end(), vreg) != regsWritten.end()) {
                                stores.push_back({vreg, tmpValueReg});
                            }
                            operand->reg = tmpValueReg;
                        }
                    }
                }

                // Insert load operations
                for (auto &[vreg, tmpValueReg] : loads) {
                    enum class LoadType { Stack,
                                          Global } loadType;
                    Ptr<Register> baseAddrReg;
                    Operand::Type offsetType;
                    int offset;

                    if (vreg->isGlobal()) {
                        loadType = LoadType::Global;
                        baseAddrReg = nullptr;
                    } else if (vreg->isSpill()) {
                        loadType = LoadType::Stack;
                        baseAddrReg = s0;
                        offsetType = Operand::Type::SPILL_OFFSET;
                        offset = vreg->spillOffset; // should be < 0
                    } else if (vreg->isArg() && vreg->memArgOffset != -1) {
                        loadType = LoadType::Stack;
                        baseAddrReg = s0;
                        offsetType = Operand::Type::MEMARG_OFFSET;
                        offset = vreg->memArgOffset; // should be >= 0
                    } else {
                        dbgassert(false, "Unhandled case");
                    }

                    if (loadType == LoadType::Stack) {
                        auto tmpAddrReg = REG_TEMP_GENERAL[2];
                        Vector<Ptr<Operand>> ops;
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                               makePtr<Operand>(offsetType, std::to_string(offset))};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, (String) "Prepare " + (vreg->isSpill() ? "spilled variable" : "memory argument") + " stack offset"));
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                               makePtr<Operand>(Operand::Type::REGISTER, baseAddrReg),
                               makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg)};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::ADD, ops, (String) "Then calculate " + (vreg->isSpill() ? "spilled variable" : "memory argument") + " address")); // 64-bit pointer addition
                        storeFromTo64(bb, makePtr<Operand>(Operand::Type::MEMORY, tmpAddrReg, 0), false, makePtr<Operand>(Operand::Type::REGISTER, tmpValueReg));
                    } else if (loadType == LoadType::Global) {
                        // la tmpValueReg, globalLabel
                        Vector<Ptr<Operand>> ops;
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpValueReg),
                               makePtr<Operand>(Operand::Type::GLOBAL, vreg->getSourceReg()->name())};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LA, ops, (String) "Load global data address")); // 64-bit load address
                    } else {
                        dbgassert(false, "Unhandled case");
                    }
                }

                // Insert instruction
                bb->addInstruction(inst);

                // Insert store operations
                for (auto &[vreg, tmpValueReg] : stores) {
                    enum class StoreType { Stack } storeType;
                    Ptr<Register> baseAddrReg;
                    Operand::Type offsetType;
                    int offset;

                    if (vreg->isSpill()) {
                        storeType = StoreType::Stack;
                        baseAddrReg = s0;
                        offsetType = Operand::Type::SPILL_OFFSET;
                        offset = vreg->spillOffset; // should be < 0
                    } else if (vreg->isArg() && vreg->memArgOffset != -1) {
                        storeType = StoreType::Stack;
                        baseAddrReg = s0;
                        offsetType = Operand::Type::MEMARG_OFFSET;
                        offset = vreg->memArgOffset; // should be >= 0
                    } else {
                        dbgassert(false, "Unhandled case");
                    }

                    if (storeType == StoreType::Stack) {
                        auto tmpAddrReg = REG_TEMP_GENERAL[2];
                        Vector<Ptr<Operand>> ops;
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                               makePtr<Operand>(offsetType, std::to_string(offset))};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::PSEUDO, Opcode::LI, ops, (String) "Prepare " + (vreg->isSpill() ? "spilled variable" : "memory argument") + " stack offset"));
                        ops = {makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg),
                               makePtr<Operand>(Operand::Type::REGISTER, baseAddrReg),
                               makePtr<Operand>(Operand::Type::REGISTER, tmpAddrReg)};
                        bb->addInstruction(makePtr<Instruction>(InstructionType::R_TYPE, Opcode::ADD, ops, (String) "Then calculate " + (vreg->isSpill() ? "spilled variable" : "memory argument") + " address")); // 64-bit pointer addition
                        // Insert instruction to store variable value
                        storeFromTo64(bb, makePtr<Operand>(Operand::Type::REGISTER, tmpValueReg), false, makePtr<Operand>(Operand::Type::MEMORY, tmpAddrReg, 0));
                    } else {
                        dbgassert(false, "Unhandled case");
                    }
                }
            }
        }
    }

    // Compare intervals by start time
    bool comparePIntervals(const LiveInterval *a, const LiveInterval *b) {
        return a->start < b->start;
    }

    bool compareIntervals(const LiveInterval &a, const LiveInterval &b) {
        return a.start < b.start;
    }

    void allocateRegisters(Ptr<RiscFunction> func, ir::DFAContext &dfaCtx) {
        auto regMap = func->getIrToVirtualRegMap();
        for (auto &bb : func->bbs()) {
            auto liveIn = dfaCtx.getLVInSet(bb->sourceBB());
            auto liveOut = dfaCtx.getLVOutSet(bb->sourceBB());
            bb->initLive(liveIn, liveOut, regMap);
        }

        LinearScan allocator(func, 23, 29);
        auto intervals = allocator.computeLiveIntervals();
        allocator.allocateRegistersForType(intervals);

        for (const auto &interval : intervals) {
            auto vreg = interval.vreg;
            if (vreg->assignedReg == nullptr) {
                vreg->spillOffset = func->stackFrame.allocSpill(8);
                // dbgout << vreg->regString() << " spilled: " << vreg->spillOffset << std::endl;
            } else {
                // dbgout << vreg->regString() << " assigned reg: " << vreg->assignedReg->regString() << std::endl;
            }
        }
    }

    void LinearScan::allocateRegistersForType(std::vector<LiveInterval> &intervals) {
        std::vector<LiveInterval *> activeGeneral;
        std::vector<LiveInterval *> activeFloat;

        for (auto &interval : intervals) {
            auto vreg = interval.vreg;
            if (vreg->getType() == Register::Type::GENERAL) {
                expireOldIntervals(interval, activeGeneral);

                if (!vreg->isBound()) {
                    if (activeGeneral.size() >= getAvailableRegistersCount(REG_ALLOC, Register::Type::GENERAL)) {
                        spillAtInterval(interval, activeGeneral, REG_SPILL_GENERAL);
                    } else {
                        vreg->assignedReg = getFreeRegisters(activeGeneral, REG_ALLOC, Register::Type::GENERAL);
                        activeGeneral.push_back(&interval);
                    }
                } else {
                    activeGeneral.push_back(&interval);
                }
                std::sort(activeGeneral.begin(), activeGeneral.end(), comparePIntervals);
            } else if (vreg->getType() == Register::Type::FLOAT) {
                expireOldIntervals(interval, activeFloat);

                if (!vreg->isBound()) {
                    if (activeFloat.size() >= getAvailableRegistersCount(REG_ALLOC, Register::Type::FLOAT)) {
                        spillAtInterval(interval, activeFloat, REG_SPILL_FLOAT);
                    } else {
                        vreg->assignedReg = getFreeRegisters(activeFloat, REG_ALLOC, Register::Type::FLOAT);
                        activeFloat.push_back(&interval);
                    }
                } else {
                    activeFloat.push_back(&interval);
                }
                std::sort(activeFloat.begin(), activeFloat.end(), comparePIntervals);
            }
        }
    }

    int LinearScan::getAvailableRegistersCount(const std::vector<std::shared_ptr<Register>> &regPool, Register::Type type) {
        int count = 0;
        for (const auto &reg : regPool) {
            if (reg->getType() == type) {
                count++;
            }
        }
        return count;
    }

    void updateLiveInterval(Ptr<VirtualRegister> vreg, int position, std::unordered_map<Ptr<Register>, LiveInterval> &intervalMap) {
        dbgassert(vreg, "vreg should not be null");

        if (vreg->isAllocated() && !vreg->isBound()) {
            return;
        }

        if (intervalMap.find(vreg) == intervalMap.end()) {
            intervalMap[vreg] = LiveInterval(vreg, position, position);
        } else {
            auto &interval = intervalMap[vreg];
            interval.start = MIN(interval.start, position);
            interval.end = MAX(interval.end, position);
        }
    }

    std::vector<LiveInterval> LinearScan::computeLiveIntervals() {
        std::unordered_map<Ptr<Register>, LiveInterval> intervalMap;

        int position = 0; // 程序点，约定为一条指令的上方

        for (auto &bb : func->bbs()) {
            ++position;
            bb->entryLine = position;

            auto &liveInSet = bb->getLiveInSet();
            auto &liveOutSet = bb->getLiveOutSet();

            for (auto liveIn : liveInSet) {
                updateLiveInterval(liveIn, position, intervalMap);
            }

            for (auto inst : bb->insts()) {
                ++position;
                inst->line = position;

                for (auto use : inst->regsRead()) {
                    if (auto vreg = castPtr<VirtualRegister>(use)) {
                        updateLiveInterval(castPtr<VirtualRegister>(use), position + 1, intervalMap); // 活跃到当前指令的下方
                    }
                }
                for (auto def : inst->regsWritten()) {
                    if (auto vreg = castPtr<VirtualRegister>(def)) {
                        updateLiveInterval(castPtr<VirtualRegister>(def), position + 1, intervalMap); // 从当前指令下方开始活跃 // TODO 先留着，后面碰到 use 再转正
                    }
                }
            }

            for (auto liveOut : liveOutSet) {
                updateLiveInterval(liveOut, position + 1, intervalMap); // 最后一条指令的下方是 position + 1
            }
        }

        std::vector<LiveInterval> intervals;
        for (auto &[reg, interval] : intervalMap) {
            // dbgout << "Live interval of " << reg->regString() << ": " << interval.start << " ~ " << interval.end << std::endl;
            intervals.push_back(interval);
        }

        std::sort(intervals.begin(), intervals.end(), compareIntervals);
        return intervals;
    }

    void LinearScan::expireOldIntervals(const LiveInterval &interval, std::vector<LiveInterval *> &active) {
        active.erase(
            std::remove_if(active.begin(), active.end(), [&](LiveInterval *activeInterval) {
                return activeInterval->end <= interval.start;
            }),
            active.end());
    }

    Ptr<Register> LinearScan::getFreeRegisters(const std::vector<LiveInterval *> &active, const std::vector<std::shared_ptr<Register>> &regPool, Register::Type type) {
        std::vector<bool> usedRegisters(regPool.size(), false);

        for (const auto &interval : active) {
            auto vreg = interval->vreg;
            if (vreg->assignedReg != nullptr) {
                for (size_t i = 0; i < regPool.size(); ++i) {
                    if (regPool[i]->getNumber() == vreg->assignedReg->getNumber() && regPool[i]->getType() == type) {
                        usedRegisters[i] = true;
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < regPool.size(); ++i) {
            if (!usedRegisters[i] && regPool[i]->getType() == type) {
                return regPool[i];
            }
        }
        return nullptr; // This should never happen.
    }

    void LinearScan::spillAtInterval(LiveInterval &interval, std::vector<LiveInterval *> &active, const std::vector<std::shared_ptr<Register>> &spillPool) {
        auto vreg = interval.vreg;
        auto spill = active.back();
        if (spill->end > interval.end && !spill->vreg->isBound()) {
            vreg->assignedReg = spill->vreg->assignedReg;
            spill->vreg->assignedReg = nullptr; // Spill the old interval
            active.back() = &interval;
        } else {
            vreg->assignedReg = nullptr; // Spill the new interval
        }
    }
}

#endif
