#include "PrologueEpilogueInsertion.h"
#include "RISCv64LLIR.h" // 假设包含了 PhysicalReg, RVOpcodes 等定义
#include "RISCv64ISel.h"
#include <algorithm>
#include <vector>
#include <set>

namespace sysy {

char PrologueEpilogueInsertionPass::ID = 0;

void PrologueEpilogueInsertionPass::runOnMachineFunction(MachineFunction* mfunc) {
    StackFrameInfo& frame_info = mfunc->getFrameInfo();
    Function* F = mfunc->getFunc();
    RISCv64ISel* isel = mfunc->getISel();
    
    // 1. 清理 KEEPALIVE 伪指令
    for (auto& mbb : mfunc->getBlocks()) {
        auto& instrs = mbb->getInstructions();
        instrs.erase(
            std::remove_if(instrs.begin(), instrs.end(),
                [](const std::unique_ptr<MachineInstr>& instr) {
                    return instr->getOpcode() == RVOpcodes::PSEUDO_KEEPALIVE;
                }
            ),
            instrs.end()
        );
    }
    
    // 2. 确定需要保存的被调用者保存寄存器 (callee-saved)
    auto& vreg_to_preg_map = frame_info.vreg_to_preg_map;
    std::set<PhysicalReg> used_callee_saved_regs_set;
    const auto& callee_saved_int = getCalleeSavedIntRegs();
    const auto& callee_saved_fp = getCalleeSavedFpRegs();

    for (const auto& pair : vreg_to_preg_map) {
        PhysicalReg preg = pair.second;
        bool is_int_cs = std::find(callee_saved_int.begin(), callee_saved_int.end(), preg) != callee_saved_int.end();
        bool is_fp_cs = std::find(callee_saved_fp.begin(), callee_saved_fp.end(), preg) != callee_saved_fp.end();
        if ((is_int_cs && preg != PhysicalReg::S0) || is_fp_cs) {
            used_callee_saved_regs_set.insert(preg);
        }
    }
    frame_info.callee_saved_regs_to_store.assign(
        used_callee_saved_regs_set.begin(), used_callee_saved_regs_set.end()
    );
    std::sort(frame_info.callee_saved_regs_to_store.begin(), frame_info.callee_saved_regs_to_store.end());
    frame_info.callee_saved_size = frame_info.callee_saved_regs_to_store.size() * 8;

    // 3. 计算最终的栈帧总大小，包含栈溢出保护
    int total_stack_size = frame_info.locals_size + 
                           frame_info.spill_size + 
                           frame_info.callee_saved_size + 
                           16;
    
    // 栈溢出保护：增加最大栈帧大小以容纳大型数组
    const int MAX_STACK_FRAME_SIZE = 8192; // 8KB to handle large arrays like 256*4*2 = 2048 bytes
    if (total_stack_size > MAX_STACK_FRAME_SIZE) {
        // 如果仍然超过限制，尝试优化对齐方式
        std::cerr << "Warning: Stack frame size " << total_stack_size 
                  << " exceeds recommended limit " << MAX_STACK_FRAME_SIZE << " for function " 
                  << mfunc->getName() << std::endl;
    }
    
    // 优化：减少对齐开销，使用16字节对齐而非更大的对齐
    int aligned_stack_size = (total_stack_size + 15) & ~15;
    frame_info.total_size = aligned_stack_size;

    if (aligned_stack_size > 0) {
        // --- 4. 插入完整的序言 ---
        MachineBasicBlock* entry_block = mfunc->getBlocks().front().get();
        auto& entry_instrs = entry_block->getInstructions();
        std::vector<std::unique_ptr<MachineInstr>> prologue_instrs;

        // 4.1. 分配栈帧
        auto alloc_stack = std::make_unique<MachineInstr>(RVOpcodes::ADDI);
        alloc_stack->addOperand(std::make_unique<RegOperand>(PhysicalReg::SP));
        alloc_stack->addOperand(std::make_unique<RegOperand>(PhysicalReg::SP));
        alloc_stack->addOperand(std::make_unique<ImmOperand>(-aligned_stack_size));
        prologue_instrs.push_back(std::move(alloc_stack));

        // 4.2. 保存 ra 和 s0
        auto save_ra = std::make_unique<MachineInstr>(RVOpcodes::SD);
        save_ra->addOperand(std::make_unique<RegOperand>(PhysicalReg::RA));
        save_ra->addOperand(std::make_unique<MemOperand>(
            std::make_unique<RegOperand>(PhysicalReg::SP),
            std::make_unique<ImmOperand>(aligned_stack_size - 8)
        ));
        prologue_instrs.push_back(std::move(save_ra));
        auto save_fp = std::make_unique<MachineInstr>(RVOpcodes::SD);
        save_fp->addOperand(std::make_unique<RegOperand>(PhysicalReg::S0));
        save_fp->addOperand(std::make_unique<MemOperand>(
            std::make_unique<RegOperand>(PhysicalReg::SP),
            std::make_unique<ImmOperand>(aligned_stack_size - 16)
        ));
        prologue_instrs.push_back(std::move(save_fp));
        
        // 4.3. 设置新的帧指针 s0
        auto set_fp = std::make_unique<MachineInstr>(RVOpcodes::ADDI);
        set_fp->addOperand(std::make_unique<RegOperand>(PhysicalReg::S0));
        set_fp->addOperand(std::make_unique<RegOperand>(PhysicalReg::SP));
        set_fp->addOperand(std::make_unique<ImmOperand>(aligned_stack_size));
        prologue_instrs.push_back(std::move(set_fp));
        
        // 4.4. 保存所有使用到的被调用者保存寄存器
        int next_available_offset = -(16 + frame_info.locals_size + frame_info.spill_size);
        for (const auto& reg : frame_info.callee_saved_regs_to_store) {
            // 改为“先更新，后使用”逻辑
            next_available_offset -= 8; // 先为当前寄存器分配下一个可用槽位
            RVOpcodes store_op = isFPR(reg) ? RVOpcodes::FSD : RVOpcodes::SD;
            auto save_cs_reg = std::make_unique<MachineInstr>(store_op);
            save_cs_reg->addOperand(std::make_unique<RegOperand>(reg));
            save_cs_reg->addOperand(std::make_unique<MemOperand>(
                std::make_unique<RegOperand>(PhysicalReg::S0),
                std::make_unique<ImmOperand>(next_available_offset) // 使用新计算出的正确偏移
            ));
            prologue_instrs.push_back(std::move(save_cs_reg));
            // 不再需要在循环末尾递减
        }

        // 4.5. 将所有生成的序言指令一次性插入到函数入口
        entry_instrs.insert(entry_instrs.begin(), 
                            std::make_move_iterator(prologue_instrs.begin()),
                            std::make_move_iterator(prologue_instrs.end()));

        // --- 5. 插入完整的尾声 ---
        for (auto& mbb : mfunc->getBlocks()) {
            for (auto it = mbb->getInstructions().begin(); it != mbb->getInstructions().end(); ++it) {
                if ((*it)->getOpcode() == RVOpcodes::RET) {
                    std::vector<std::unique_ptr<MachineInstr>> epilogue_instrs;
                    
                    // 5.1. 恢复被调用者保存寄存器
                    int next_available_offset_restore = -(16 + frame_info.locals_size + frame_info.spill_size);
                    for (const auto& reg : frame_info.callee_saved_regs_to_store) {
                        next_available_offset_restore -= 8; // 为下一个寄存器准备偏移
                        RVOpcodes load_op = isFPR(reg) ? RVOpcodes::FLD : RVOpcodes::LD;
                        auto restore_cs_reg = std::make_unique<MachineInstr>(load_op);
                        restore_cs_reg->addOperand(std::make_unique<RegOperand>(reg));
                        restore_cs_reg->addOperand(std::make_unique<MemOperand>(
                            std::make_unique<RegOperand>(PhysicalReg::S0),
                            std::make_unique<ImmOperand>(next_available_offset_restore) // 使用当前偏移
                        ));
                        epilogue_instrs.push_back(std::move(restore_cs_reg));
                    }

                    // 5.2. 恢复 ra 和 s0
                    auto restore_ra = std::make_unique<MachineInstr>(RVOpcodes::LD);
                    restore_ra->addOperand(std::make_unique<RegOperand>(PhysicalReg::RA));
                    restore_ra->addOperand(std::make_unique<MemOperand>(
                        std::make_unique<RegOperand>(PhysicalReg::SP),
                        std::make_unique<ImmOperand>(aligned_stack_size - 8)
                    ));
                    epilogue_instrs.push_back(std::move(restore_ra));
                    auto restore_fp = std::make_unique<MachineInstr>(RVOpcodes::LD);
                    restore_fp->addOperand(std::make_unique<RegOperand>(PhysicalReg::S0));
                    restore_fp->addOperand(std::make_unique<MemOperand>(
                        std::make_unique<RegOperand>(PhysicalReg::SP),
                        std::make_unique<ImmOperand>(aligned_stack_size - 16)
                    ));
                    epilogue_instrs.push_back(std::move(restore_fp));

                    // 5.3. 释放栈帧
                    auto dealloc_stack = std::make_unique<MachineInstr>(RVOpcodes::ADDI);
                    dealloc_stack->addOperand(std::make_unique<RegOperand>(PhysicalReg::SP));
                    dealloc_stack->addOperand(std::make_unique<RegOperand>(PhysicalReg::SP));
                    dealloc_stack->addOperand(std::make_unique<ImmOperand>(aligned_stack_size));
                    epilogue_instrs.push_back(std::move(dealloc_stack));

                    // 将尾声指令插入到 RET 指令之前
                    mbb->getInstructions().insert(it, 
                                                  std::make_move_iterator(epilogue_instrs.begin()),
                                                  std::make_move_iterator(epilogue_instrs.end()));
                    
                    goto next_block;
                }
            }
            next_block:;
        }
    }
}

} // namespace sysy