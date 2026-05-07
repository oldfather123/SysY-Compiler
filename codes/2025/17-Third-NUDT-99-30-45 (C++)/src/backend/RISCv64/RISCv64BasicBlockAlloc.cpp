#include "RISCv64BasicBlockAlloc.h"
#include "RISCv64Info.h"
#include "RISCv64AsmPrinter.h"
#include <iostream>
#include <algorithm>

// 外部调试级别控制变量
extern int DEBUG;
extern int DEEPDEBUG;

namespace sysy {

// 将 getInstrUseDef 的定义移到这里，因为它是一个全局的辅助函数
void getInstrUseDef(const MachineInstr* instr, std::set<unsigned>& use, std::set<unsigned>& def) {
    auto opcode = instr->getOpcode();
    const auto& operands = instr->getOperands();
    
    auto get_vreg_id_if_virtual = [&](const MachineOperand* op, std::set<unsigned>& s) {
        if (op->getKind() == MachineOperand::KIND_REG) {
            auto reg_op = static_cast<const RegOperand*>(op);
            if (reg_op->isVirtual()) s.insert(reg_op->getVRegNum());
        } else if (op->getKind() == MachineOperand::KIND_MEM) {
            auto mem_op = static_cast<const MemOperand*>(op);
            auto reg_op = mem_op->getBase();
            if (reg_op->isVirtual()) s.insert(reg_op->getVRegNum());
        }
    };

    if (op_info.count(opcode)) {
        const auto& info = op_info.at(opcode);
        for (int idx : info.first) if (idx < operands.size()) get_vreg_id_if_virtual(operands[idx].get(), def);
        for (int idx : info.second) if (idx < operands.size()) get_vreg_id_if_virtual(operands[idx].get(), use);
        // 内存操作数的基址寄存器总是use
        for (const auto& op : operands) if (op->getKind() == MachineOperand::KIND_MEM) get_vreg_id_if_virtual(op.get(), use);
    } else if (opcode == RVOpcodes::CALL) {
        if (!operands.empty() && operands[0]->getKind() == MachineOperand::KIND_REG) get_vreg_id_if_virtual(operands[0].get(), def);
        for (size_t i = 1; i < operands.size(); ++i) if (operands[i]->getKind() == MachineOperand::KIND_REG) get_vreg_id_if_virtual(operands[i].get(), use);
    }
}


RISCv64BasicBlockAlloc::RISCv64BasicBlockAlloc(MachineFunction* mfunc)
    : MFunc(mfunc), ISel(mfunc->getISel()) {
    // 初始化临时寄存器池
    int_temps = {PhysicalReg::T0, PhysicalReg::T1, PhysicalReg::T2, PhysicalReg::T3, PhysicalReg::T6};
    fp_temps = {PhysicalReg::F0, PhysicalReg::F1, PhysicalReg::F2, PhysicalReg::F3, PhysicalReg::F4};
    int_temp_idx = 0;
    fp_temp_idx = 0;

    // 构建ABI寄存器映射
    if (MFunc->getFunc()) {
        int int_arg_idx = 0;
        int fp_arg_idx = 0;
        for (Argument* arg : MFunc->getFunc()->getArguments()) {
            unsigned arg_vreg = ISel->getVReg(arg);
            if (arg->getType()->isFloat()) {
                if (fp_arg_idx < 8) {
                    auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::F10) + fp_arg_idx++);
                    abi_vreg_map[arg_vreg] = preg;
                }
            } else {
                if (int_arg_idx < 8) {
                    auto preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::A0) + int_arg_idx++);
                    abi_vreg_map[arg_vreg] = preg;
                }
            }
        }
    }
}

void RISCv64BasicBlockAlloc::run() {
    if (DEBUG) std::cerr << "===== [BB-Alloc] Running Stateful Greedy Allocator for function: " << MFunc->getName() << " =====\n";
    
    computeLiveness();
    assignStackSlotsForAllVRegs();

    for (auto& mbb : MFunc->getBlocks()) {
        processBasicBlock(mbb.get());
    }
    
    // 将ABI寄存器映射（如函数参数）合并到最终结果中
    MFunc->getFrameInfo().vreg_to_preg_map.insert(this->abi_vreg_map.begin(), this->abi_vreg_map.end());
}

PhysicalReg RISCv64BasicBlockAlloc::getNextIntTemp() {
    PhysicalReg reg = int_temps[int_temp_idx];
    int_temp_idx = (int_temp_idx + 1) % int_temps.size();
    return reg;
}

PhysicalReg RISCv64BasicBlockAlloc::getNextFpTemp() {
    PhysicalReg reg = fp_temps[fp_temp_idx];
    fp_temp_idx = (fp_temp_idx + 1) % fp_temps.size();
    return reg;
}

void RISCv64BasicBlockAlloc::computeLiveness() {
    // 这是一个必需的步骤，用于确定在块末尾哪些变量需要被写回栈
    // 为保持聚焦，此处暂时留空，但请确保您有一个有效的活性分析来填充 live_out 映射
}

void RISCv64BasicBlockAlloc::assignStackSlotsForAllVRegs() {
    if (DEBUG) std::cerr << "[BB-Alloc] Assigning stack slots for all vregs.\n";
    StackFrameInfo& frame_info = MFunc->getFrameInfo();
    int current_offset = frame_info.locals_end_offset;
    const auto& vreg_type_map = ISel->getVRegTypeMap();

    for (unsigned vreg = 1; vreg < ISel->getVRegCounter(); ++vreg) {
        if (this->abi_vreg_map.count(vreg) || frame_info.alloca_offsets.count(vreg) || frame_info.spill_offsets.count(vreg)) {
            continue;
        }

        Type* type = vreg_type_map.count(vreg) ? vreg_type_map.at(vreg) : Type::getIntType();
        int size = type->isPointer() ? 8 : 4;
        
        current_offset -= size;
        current_offset &= -size; // 按size对齐
        
        frame_info.spill_offsets[vreg] = current_offset;
    }
    frame_info.spill_size = -(current_offset - frame_info.locals_end_offset);
}

void RISCv64BasicBlockAlloc::processBasicBlock(MachineBasicBlock* mbb) {
    if (DEEPDEBUG) std::cerr << "  [BB-Alloc] Processing block " << mbb->getName() << "\n";
    
    vreg_to_preg.clear();
    preg_to_vreg.clear();
    dirty_pregs.clear();
    
    auto& instrs = mbb->getInstructions();
    std::vector<std::unique_ptr<MachineInstr>> new_instrs;
    const auto& vreg_type_map = ISel->getVRegTypeMap();

    for (auto& instr_ptr : instrs) {
        std::set<unsigned> use_vregs, def_vregs;
        getInstrUseDef(instr_ptr.get(), use_vregs, def_vregs);

        std::map<unsigned, PhysicalReg> current_instr_map;

        // 1. 确保所有use操作数都在物理寄存器中
        for (unsigned vreg : use_vregs) {
            current_instr_map[vreg] = ensureInReg(vreg, new_instrs);
        }

        // 2. 为所有def操作数分配物理寄存器
        for (unsigned vreg : def_vregs) {
            current_instr_map[vreg] = allocReg(vreg, new_instrs);
        }

        // 3. 重写指令，将vreg替换为preg
        for (const auto& pair : current_instr_map) {
            instr_ptr->replaceVRegWithPReg(pair.first, pair.second);
        }
        
        new_instrs.push_back(std::move(instr_ptr));
    }

    // 4. 在块末尾，写回所有被修改过的且在后续块中活跃(live-out)的vreg
    StackFrameInfo& frame_info = MFunc->getFrameInfo(); // **修正：获取frame_info引用**
    const auto& lo = live_out[mbb];
    for(auto const& [preg, vreg] : preg_to_vreg) {
        // **修正：简化逻辑，在此保底分配器中总是写回脏寄存器**
        if (dirty_pregs.count(preg)) {
             if (!frame_info.spill_offsets.count(vreg)) continue;
            Type* type = vreg_type_map.at(vreg);
            RVOpcodes store_op = type->isFloat() ? RVOpcodes::FSW : (type->isPointer() ? RVOpcodes::SD : RVOpcodes::SW);
            auto store = std::make_unique<MachineInstr>(store_op);
            store->addOperand(std::make_unique<RegOperand>(preg));
            store->addOperand(std::make_unique<MemOperand>(
                std::make_unique<RegOperand>(PhysicalReg::S0),
                std::make_unique<ImmOperand>(frame_info.spill_offsets.at(vreg))
            ));
            new_instrs.push_back(std::move(store));
        }
    }

    instrs = std::move(new_instrs);
}

PhysicalReg RISCv64BasicBlockAlloc::ensureInReg(unsigned vreg, std::vector<std::unique_ptr<MachineInstr>>& new_instrs) {
    if (abi_vreg_map.count(vreg)) {
        return abi_vreg_map.at(vreg);
    }
    if (vreg_to_preg.count(vreg)) {
        return vreg_to_preg.at(vreg);
    }
    
    PhysicalReg preg = allocReg(vreg, new_instrs);
    
    const auto& vreg_type_map = ISel->getVRegTypeMap();
    Type* type = vreg_type_map.count(vreg) ? vreg_type_map.at(vreg) : Type::getIntType();
    RVOpcodes load_op = type->isFloat() ? RVOpcodes::FLW : (type->isPointer() ? RVOpcodes::LD : RVOpcodes::LW);
    
    auto load = std::make_unique<MachineInstr>(load_op);
    load->addOperand(std::make_unique<RegOperand>(preg));
    load->addOperand(std::make_unique<MemOperand>(
        std::make_unique<RegOperand>(PhysicalReg::S0),
        std::make_unique<ImmOperand>(MFunc->getFrameInfo().spill_offsets.at(vreg))
    ));
    new_instrs.push_back(std::move(load));
    
    dirty_pregs.erase(preg);
    
    return preg;
}

PhysicalReg RISCv64BasicBlockAlloc::allocReg(unsigned vreg, std::vector<std::unique_ptr<MachineInstr>>& new_instrs) {
    if (abi_vreg_map.count(vreg)) {
        dirty_pregs.insert(abi_vreg_map.at(vreg)); // 如果参数被重定义，也标记为脏
        return abi_vreg_map.at(vreg);
    }

    bool is_fp = ISel->getVRegTypeMap().at(vreg)->isFloat();
    PhysicalReg preg = findFreeReg(is_fp);
    if (preg == PhysicalReg::INVALID) {
        preg = spillReg(is_fp, new_instrs);
    }

    if (preg_to_vreg.count(preg)) {
        vreg_to_preg.erase(preg_to_vreg.at(preg));
    }
    vreg_to_preg[vreg] = preg;
    preg_to_vreg[preg] = vreg;
    dirty_pregs.insert(preg);
    
    return preg;
}

PhysicalReg RISCv64BasicBlockAlloc::findFreeReg(bool is_fp) {
    // **修正：使用正确的成员变量名 int_temps 和 fp_temps**
    const auto& regs = is_fp ? fp_temps : int_temps;
    for (PhysicalReg preg : regs) {
        if (!preg_to_vreg.count(preg)) {
            return preg;
        }
    }
    return PhysicalReg::INVALID;
}

PhysicalReg RISCv64BasicBlockAlloc::spillReg(bool is_fp, std::vector<std::unique_ptr<MachineInstr>>& new_instrs) {
    // **修正**: 调用成员函数需要使用 this->
    PhysicalReg preg_to_spill = is_fp ? this->getNextFpTemp() : this->getNextIntTemp();
    
    if (preg_to_vreg.count(preg_to_spill)) {
        unsigned victim_vreg = preg_to_vreg.at(preg_to_spill);
        if (dirty_pregs.count(preg_to_spill)) {
            const auto& vreg_type_map = ISel->getVRegTypeMap();
            Type* type = vreg_type_map.count(victim_vreg) ? vreg_type_map.at(victim_vreg) : Type::getIntType();
            RVOpcodes store_op = type->isFloat() ? RVOpcodes::FSW : (type->isPointer() ? RVOpcodes::SD : RVOpcodes::SW);
            auto store = std::make_unique<MachineInstr>(store_op);
            store->addOperand(std::make_unique<RegOperand>(preg_to_spill));
            store->addOperand(std::make_unique<MemOperand>(
                std::make_unique<RegOperand>(PhysicalReg::S0),
                std::make_unique<ImmOperand>(MFunc->getFrameInfo().spill_offsets.at(victim_vreg))
            ));
            new_instrs.push_back(std::move(store));
        }
        vreg_to_preg.erase(victim_vreg);
        dirty_pregs.erase(preg_to_spill);
    }
    
    preg_to_vreg.erase(preg_to_spill);
    return preg_to_spill;
}

} // namespace sysy