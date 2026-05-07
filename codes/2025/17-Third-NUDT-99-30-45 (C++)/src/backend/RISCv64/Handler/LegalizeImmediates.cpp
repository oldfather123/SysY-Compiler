#include "LegalizeImmediates.h"
#include "RISCv64ISel.h" // 需要包含它以调用 getNewVReg()
#include "RISCv64AsmPrinter.h"
#include <vector>
#include <iostream>


// 声明外部调试控制变量
extern int DEBUG;
extern int DEEPDEBUG;

namespace sysy {

char LegalizeImmediatesPass::ID = 0;

// 辅助函数：检查一个立即数是否在RISC-V的12位有符号范围内
static bool isLegalImmediate(int64_t imm) {
    return imm >= -2048 && imm <= 2047;
}

void LegalizeImmediatesPass::runOnMachineFunction(MachineFunction* mfunc) {
    if (DEBUG) {
        std::cerr << "===== Running Legalize Immediates Pass on function: " << mfunc->getName() << " =====\n";
    }

    // 定义我们保留的、用于暂存的物理寄存器
    const PhysicalReg TEMP_REG = PhysicalReg::T5;
    
    // 创建一个临时的AsmPrinter用于打印指令，方便调试
    RISCv64AsmPrinter temp_printer(mfunc);
    if (DEEPDEBUG) {
        temp_printer.setStream(std::cerr);
    }

    for (auto& mbb : mfunc->getBlocks()) {
        if (DEEPDEBUG) {
            std::cerr << "--- Processing Basic Block: " << mbb->getName() << " ---\n";
        }
        // 创建一个新的指令列表，用于存放合法化后的指令
        std::vector<std::unique_ptr<MachineInstr>> new_instructions;
        
        for (auto& instr_ptr : mbb->getInstructions()) {
            if (DEEPDEBUG) {
                std::cerr << "  Checking: ";
                // 打印指令时末尾会带换行符，所以这里不用 std::endl
                temp_printer.printInstruction(instr_ptr.get(), true);
            }

            bool legalized = false; // 标记当前指令是否已被展开处理

            switch (instr_ptr->getOpcode()) {
                case RVOpcodes::ADDI:
                case RVOpcodes::ADDIW: {
                    auto& operands = instr_ptr->getOperands();
                    // 确保操作数足够多，以防万一
                    if (operands.size() < 3) break;
                    auto imm_op = static_cast<ImmOperand*>(operands.back().get());
                    
                    if (!isLegalImmediate(imm_op->getValue())) {
                        if (DEEPDEBUG) {
                            std::cerr << "    >> ILLEGAL immediate (" << imm_op->getValue() << "). Expanding...\n";
                        }
                        // 立即数超出范围，需要展开
                        auto rd_op = std::make_unique<RegOperand>(*static_cast<RegOperand*>(operands[0].get()));
                        auto rs1_op = std::make_unique<RegOperand>(*static_cast<RegOperand*>(operands[1].get()));
                        
                        // 1. li t5, immediate
                        auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                        li->addOperand(std::make_unique<RegOperand>(TEMP_REG));
                        li->addOperand(std::make_unique<ImmOperand>(imm_op->getValue()));

                        // 2. add/addw rd, rs1, t5
                        auto new_op = (instr_ptr->getOpcode() == RVOpcodes::ADDI) ? RVOpcodes::ADD : RVOpcodes::ADDW;
                        auto add = std::make_unique<MachineInstr>(new_op);
                        add->addOperand(std::move(rd_op));
                        add->addOperand(std::move(rs1_op));
                        add->addOperand(std::make_unique<RegOperand>(TEMP_REG));

                        if (DEEPDEBUG) {
                            std::cerr << "    New sequence:\n      ";
                            temp_printer.printInstruction(li.get(), true);
                            std::cerr << "      ";
                            temp_printer.printInstruction(add.get(), true);
                        }
                        
                        new_instructions.push_back(std::move(li));
                        new_instructions.push_back(std::move(add));
                        
                        legalized = true;
                    }
                    break;
                }
                
                // 处理所有内存加载/存储指令
                case RVOpcodes::LB: case RVOpcodes::LH: case RVOpcodes::LW: case RVOpcodes::LD:
                case RVOpcodes::LBU: case RVOpcodes::LHU: case RVOpcodes::LWU:
                case RVOpcodes::SB: case RVOpcodes::SH: case RVOpcodes::SW: case RVOpcodes::SD:
                case RVOpcodes::FLW: case RVOpcodes::FSW: case RVOpcodes::FLD: case RVOpcodes::FSD: {
                    auto& operands = instr_ptr->getOperands();
                    auto mem_op = static_cast<MemOperand*>(operands.back().get());
                    auto offset_op = mem_op->getOffset();
                    
                    if (!isLegalImmediate(offset_op->getValue())) {
                        if (DEEPDEBUG) {
                            std::cerr << "    >> ILLEGAL immediate offset (" << offset_op->getValue() << "). Expanding...\n";
                        }
                        // 偏移量超出范围，需要展开
                        auto data_reg_op = std::make_unique<RegOperand>(*static_cast<RegOperand*>(operands[0].get()));
                        auto base_reg_op = std::make_unique<RegOperand>(*mem_op->getBase());
                        
                        // 1. li t5, offset
                        auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                        li->addOperand(std::make_unique<RegOperand>(TEMP_REG));
                        li->addOperand(std::make_unique<ImmOperand>(offset_op->getValue()));
                        
                        // 2. add t5, base_reg, t5  (计算最终地址，结果也放在t5)
                        auto add = std::make_unique<MachineInstr>(RVOpcodes::ADD);
                        add->addOperand(std::make_unique<RegOperand>(TEMP_REG));
                        add->addOperand(std::move(base_reg_op));
                        add->addOperand(std::make_unique<RegOperand>(TEMP_REG));

                        // 3. lw/sw data_reg, 0(t5)
                        auto mem_instr = std::make_unique<MachineInstr>(instr_ptr->getOpcode());
                        mem_instr->addOperand(std::move(data_reg_op));
                        mem_instr->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(TEMP_REG),
                            std::make_unique<ImmOperand>(0)
                        ));
                        
                        if (DEEPDEBUG) {
                            std::cerr << "    New sequence:\n      ";
                            temp_printer.printInstruction(li.get(), true);
                            std::cerr << "      ";
                            temp_printer.printInstruction(add.get(), true);
                            std::cerr << "      ";
                            temp_printer.printInstruction(mem_instr.get(), true);
                        }

                        new_instructions.push_back(std::move(li));
                        new_instructions.push_back(std::move(add));
                        new_instructions.push_back(std::move(mem_instr));

                        legalized = true;
                    }
                    break;
                }

                default:
                    // 其他指令不需要处理
                    break;
            }

            if (!legalized) {
                if (DEEPDEBUG) {
                    std::cerr << "    -- Immediate is legal. Skipping.\n";
                }
                // 如果当前指令不需要合法化，直接将其移动到新列表中
                new_instructions.push_back(std::move(instr_ptr));
            }
        }

        // 用新的、已合法化的指令列表替换旧的列表
        mbb->getInstructions() = std::move(new_instructions);
    }

    if (DEBUG) {
        std::cerr << "===== Finished Legalize Immediates Pass =====\n\n";
    }
}

} // namespace sysy