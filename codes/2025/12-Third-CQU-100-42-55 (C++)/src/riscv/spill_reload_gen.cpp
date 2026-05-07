#include "../../include/riscv/spill_reload_gen.h"
#include <iostream>
#include <sstream>

// 修复：添加isFloatType实现
namespace llvm_ir {
static bool isFloatType(Type type) {
    return type == Type::Float || type == Type::Double;
}
}

namespace llvm_ir {

void SpillReloadGenerator::generateSpillReloadInstructions() {
    std::cout << "[SpillReload] Generating spill/reload instructions..." << std::endl;
    
    int instruction_pos = 0;
    
    // 遍历所有基本块和指令
    for (auto& bb_ptr : func->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        
        // 处理phi指令
        for (auto& phi_ptr : bb->phi_instructions) {
            analyzeInstruction(phi_ptr.get(), instruction_pos++);
        }
        
        // 处理普通指令
        for (auto& inst_ptr : bb->instructions) {
            analyzeInstruction(inst_ptr.get(), instruction_pos++);
        }
    }
    
    std::cout << "[SpillReload] Generated " << spill_reload_insts.size() 
              << " spill/reload instructions" << std::endl;
}

void SpillReloadGenerator::analyzeInstruction(Instruction* inst, int pos) {
    if (!inst) return;
    
    // 检查指令的定义（结果）是否需要溢出
    if (needsSpill(inst, pos)) {
        auto it = alloc_result.reg_alloc_map.find(inst);
        if (it != alloc_result.reg_alloc_map.end() && !it->second.is_reg) {
            // 需要溢出：在指令后插入store指令
            spill_reload_insts.emplace_back(
                SpillReloadType::SPILL,
                inst,
                riscv::NONE, // 临时寄存器，需要分配
                it->second.stack_offset,
                inst->type,
                pos + 1  // 在指令后插入
            );
        }
    }
    
    // 检查指令的操作数是否需要重载
    for (Value* operand : inst->operands) {
        if (needsReload(operand, pos)) {
            auto it = alloc_result.reg_alloc_map.find(operand);
            if (it != alloc_result.reg_alloc_map.end() && !it->second.is_reg) {
                // 需要重载：在指令前插入load指令
                spill_reload_insts.emplace_back(
                    SpillReloadType::RELOAD,
                    operand,
                    riscv::NONE, // 临时寄存器，需要分配
                    it->second.stack_offset,
                    operand->type,
                    pos  // 在指令前插入
                );
            }
        }
    }
}

bool SpillReloadGenerator::needsSpill(Value* vreg, int pos) const {
    // 检查虚拟寄存器是否溢出
    auto it = alloc_result.reg_alloc_map.find(vreg);
    if (it == alloc_result.reg_alloc_map.end()) return false;
    
    // 如果分配到栈，则需要溢出
    return !it->second.is_reg;
}

bool SpillReloadGenerator::needsReload(Value* vreg, int pos) const {
    // 检查虚拟寄存器是否溢出
    auto it = alloc_result.reg_alloc_map.find(vreg);
    if (it == alloc_result.reg_alloc_map.end()) return false;
    
    // 如果分配到栈，则需要重载
    return !it->second.is_reg;
}

std::string SpillReloadGenerator::generateRiscvCode() const {
    std::ostringstream oss;
    
    for (const auto& inst : spill_reload_insts) {
        if (inst.type == SpillReloadType::SPILL) {
            oss << generateSpillCode(inst);
        } else {
            oss << generateReloadCode(inst);
        }
    }
    
    return oss.str();
}

std::string SpillReloadGenerator::generateSpillCode(const SpillReloadInst& inst) const {
    std::ostringstream oss;
    
    // 分配临时寄存器（这里简化处理，实际需要更复杂的寄存器分配）
    std::string temp_reg = "t0"; // 临时使用t0
    
    oss << "    # SPILL: " << inst.vreg->name << " to stack offset " << inst.stack_offset << std::endl;
    
    if (isFloatType(inst.data_type)) {
        // 浮点类型
        if (inst.data_type == Type::Float) {
            oss << "    fsw " << temp_reg << ", " << inst.stack_offset << "(sp)" << std::endl;
        } else { // Double
            oss << "    fsd " << temp_reg << ", " << inst.stack_offset << "(sp)" << std::endl;
        }
    } else {
        // 整数类型
        if (inst.data_type == Type::I64) {
            oss << "    sd " << temp_reg << ", " << inst.stack_offset << "(sp)" << std::endl;
        } else {
            oss << "    sw " << temp_reg << ", " << inst.stack_offset << "(sp)" << std::endl;
        }
    }
    
    return oss.str();
}

std::string SpillReloadGenerator::generateReloadCode(const SpillReloadInst& inst) const {
    std::ostringstream oss;
    
    // 分配临时寄存器（这里简化处理，实际需要更复杂的寄存器分配）
    std::string temp_reg = "t0"; // 临时使用t0
    
    oss << "    # RELOAD: " << inst.vreg->name << " from stack offset " << inst.stack_offset << std::endl;
    
    if (isFloatType(inst.data_type)) {
        // 浮点类型
        if (inst.data_type == Type::Float) {
            oss << "    flw " << temp_reg << ", " << inst.stack_offset << "(sp)" << std::endl;
        } else { // Double
            oss << "    fld " << temp_reg << ", " << inst.stack_offset << "(sp)" << std::endl;
        }
    } else {
        // 整数类型
        if (inst.data_type == Type::I64) {
            oss << "    ld " << temp_reg << ", " << inst.stack_offset << "(sp)" << std::endl;
        } else {
            oss << "    lw " << temp_reg << ", " << inst.stack_offset << "(sp)" << std::endl;
        }
    }
    
    return oss.str();
}

std::string SpillReloadGenerator::getRegName(riscv::reg reg) const {
    return riscv::to_string(reg);
}

std::string SpillReloadGenerator::getLoadStoreOp(Type type) const {
    if (isFloatType(type)) {
        return (type == Type::Float) ? "flw" : "fld";
    } else {
        return (type == Type::I64) ? "ld" : "lw";
    }
}

void SpillReloadGenerator::printInstructions() const {
    std::cout << "\n[SpillReload] Generated Instructions:" << std::endl;
    
    for (const auto& inst : spill_reload_insts) {
        std::cout << "  " << (inst.type == SpillReloadType::SPILL ? "SPILL" : "RELOAD")
                  << " " << inst.vreg->name << " (" << inst.vreg->getTypeName() << ")"
                  << " at pos " << inst.instruction_pos
                  << " offset " << inst.stack_offset << std::endl;
    }
}

// 全局函数实现
std::vector<SpillReloadInst> generateSpillReloadInstructions(Function* func, const FunctionRegAllocResult& alloc_result) {
    SpillReloadGenerator generator(func, alloc_result);
    generator.generateSpillReloadInstructions();
    return generator.getInstructions();
}

} // namespace llvm_ir 