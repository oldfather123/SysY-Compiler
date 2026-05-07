#include "EliminateFrameIndices.h"
#include "RISCv64ISel.h"
#include <cassert>
#include <vector>

namespace sysy {

// getTypeSizeInBytes 是一个通用辅助函数，保持不变
unsigned EliminateFrameIndicesPass::getTypeSizeInBytes(Type* type) {
    if (!type) {
        assert(false && "Cannot get size of a null type.");
        return 0;
    }

    switch (type->getKind()) {
        case Type::kInt:
        case Type::kFloat:
            return 4;
        case Type::kPointer:
            return 8;
        case Type::kArray: {
            auto arrayType = type->as<ArrayType>();
            return arrayType->getNumElements() * getTypeSizeInBytes(arrayType->getElementType());
        }
        default:
            assert(false && "Unsupported type for size calculation.");
            return 0;
    }
}

void EliminateFrameIndicesPass::runOnMachineFunction(MachineFunction* mfunc) {
    StackFrameInfo& frame_info = mfunc->getFrameInfo();
    Function* F = mfunc->getFunc();
    RISCv64ISel* isel = mfunc->getISel();
    
    // 在这里处理栈传递的参数，以便在寄存器分配前就将数据流显式化，修复溢出逻辑的BUG。

    // 2. 只为局部变量(AllocaInst)分配栈空间和计算偏移量
    // 局部变量从 s0 下方（负偏移量）开始分配，紧接着为 ra 和 s0 预留的16字节之后
    int local_var_offset = 16;
    
    if(F) { // 确保函数指针有效
        for (auto& bb : F->getBasicBlocks()) {
            for (auto& inst : bb->getInstructions()) {
                if (auto alloca = dynamic_cast<AllocaInst*>(inst.get())) {
                    Type* allocated_type = alloca->getType()->as<PointerType>()->getBaseType();
                    int size = getTypeSizeInBytes(allocated_type);
                    
                    // 优化栈帧大小：对于大数组使用4字节对齐，小对象使用8字节对齐
                    if (size >= 256) {  // 大数组优化
                        size = (size + 3) & ~3;  // 4字节对齐
                    } else {
                        size = (size + 7) & ~7;  // 8字节对齐
                    }
                    if (size == 0) size = 4; // 最小4字节

                    local_var_offset += size;
                    unsigned alloca_vreg = isel->getVReg(alloca);
                    // 局部变量使用相对于s0的负向偏移
                    frame_info.alloca_offsets[alloca_vreg] = -local_var_offset;
                }
            }
        }
    }
    
    // 记录仅由AllocaInst分配的局部变量的总大小
    frame_info.locals_size = local_var_offset - 16;
    // 记录局部变量区域分配结束的最终偏移量
    frame_info.locals_end_offset = -local_var_offset;

    // 在函数入口为所有栈传递的参数插入load指令
    // 这个步骤至关重要：它在寄存器分配之前，为这些参数的vreg创建了明确的“定义(def)”指令。
    // 这解决了在高寄存器压力下，当这些vreg被溢出时，`rewriteProgram`找不到其定义点而崩溃的问题。
    if (F && isel && !mfunc->getBlocks().empty()) {
        MachineBasicBlock* entry_block = mfunc->getBlocks().front().get();
        std::vector<std::unique_ptr<MachineInstr>> arg_load_instrs;
        
        // 步骤 3.1: 生成所有加载栈参数的指令，暂存起来
        int arg_idx = 0;
        for (Argument* arg : F->getArguments()) {
            // 根据ABI，前8个整型/指针参数通过寄存器传递，这里只处理超出部分。
            if (arg_idx >= 8) {
                // 计算参数在调用者栈帧中的位置，该位置相对于被调用者的帧指针s0是正向偏移。
                // 第9个参数(arg_idx=8)位于 0(s0)，第10个(arg_idx=9)位于 8(s0)，以此类推。
                int offset = (arg_idx - 8) * 8; 
                unsigned arg_vreg = isel->getVReg(arg);
                Type* arg_type = arg->getType();

                // 根据参数类型选择正确的加载指令
                RVOpcodes load_op;
                if (arg_type->isFloat()) {
                    load_op = RVOpcodes::FLW; // 单精度浮点
                } else if (arg_type->isPointer()) {
                    load_op = RVOpcodes::LD;  // 64位指针
                } else {
                    load_op = RVOpcodes::LW;  // 32位整数
                }
                
                // 创建加载指令: lw/ld/flw vreg, offset(s0)
                auto load_instr = std::make_unique<MachineInstr>(load_op);
                load_instr->addOperand(std::make_unique<RegOperand>(arg_vreg));
                load_instr->addOperand(std::make_unique<MemOperand>(
                    std::make_unique<RegOperand>(PhysicalReg::S0), // 基址为帧指针
                    std::make_unique<ImmOperand>(offset)
                ));
                arg_load_instrs.push_back(std::move(load_instr));
            }
            arg_idx++;
        }
        
        //仅当有需要加载的栈参数时，才执行插入逻辑
        if (!arg_load_instrs.empty()) {
            auto& entry_instrs = entry_block->getInstructions();
            auto insertion_point = entry_instrs.begin(); // 默认插入点为块的开头
            auto last_arg_save_it = entry_instrs.end();

            // 步骤 3.2: 寻找一个安全的插入点。
            // 遍历入口块的指令，找到最后一条保存“寄存器传递参数”的伪指令。
            // 这样可以确保我们在所有 a0-a7 参数被保存之后，才执行可能覆盖它们的加载指令。
            for (auto it = entry_instrs.begin(); it != entry_instrs.end(); ++it) {
                MachineInstr* instr = it->get();
                // 寻找代表保存参数到栈的伪指令
                if (instr->getOpcode() == RVOpcodes::FRAME_STORE_W ||
                    instr->getOpcode() == RVOpcodes::FRAME_STORE_D ||
                    instr->getOpcode() == RVOpcodes::FRAME_STORE_F) {
                    
                    // 检查被保存的值是否是寄存器参数 (arg_no < 8)
                    auto& operands = instr->getOperands();
                    if (operands.empty() || operands[0]->getKind() != MachineOperand::KIND_REG) continue;
                    
                    unsigned src_vreg = static_cast<RegOperand*>(operands[0].get())->getVRegNum();
                    Value* ir_value = isel->getVRegValueMap().count(src_vreg) ? isel->getVRegValueMap().at(src_vreg) : nullptr;
                    
                    if (auto ir_arg = dynamic_cast<Argument*>(ir_value)) {
                        if (ir_arg->getIndex() < 8) {
                            last_arg_save_it = it; // 找到了一个保存寄存器参数的指令，更新位置
                        }
                    }
                }
            }

            // 如果找到了这样的保存指令，我们的插入点就在它之后
            if (last_arg_save_it != entry_instrs.end()) {
                insertion_point = std::next(last_arg_save_it);
            }

            // 步骤 3.3: 在计算出的安全位置，一次性插入所有新创建的参数加载指令
            entry_instrs.insert(insertion_point,
                                std::make_move_iterator(arg_load_instrs.begin()),
                                std::make_move_iterator(arg_load_instrs.end()));
        }
    }

    // 4. 遍历所有机器指令，将访问局部变量的伪指令展开为真实指令
    for (auto& mbb : mfunc->getBlocks()) {
        std::vector<std::unique_ptr<MachineInstr>> new_instructions;
        for (auto& instr_ptr : mbb->getInstructions()) {
            RVOpcodes opcode = instr_ptr->getOpcode();

            if (opcode == RVOpcodes::FRAME_LOAD_W || opcode == RVOpcodes::FRAME_LOAD_D || opcode == RVOpcodes::FRAME_LOAD_F) {
                RVOpcodes real_load_op;
                if (opcode == RVOpcodes::FRAME_LOAD_W) real_load_op = RVOpcodes::LW;
                else if (opcode == RVOpcodes::FRAME_LOAD_D) real_load_op = RVOpcodes::LD;
                else real_load_op = RVOpcodes::FLW;

                auto& operands = instr_ptr->getOperands();
                unsigned dest_vreg = static_cast<RegOperand*>(operands[0].get())->getVRegNum();
                unsigned alloca_vreg = static_cast<RegOperand*>(operands[1].get())->getVRegNum();
                int offset = frame_info.alloca_offsets.at(alloca_vreg);
                auto addr_vreg = isel->getNewVReg(Type::getPointerType(Type::getIntType()));

                // 展开为: addi addr_vreg, s0, offset
                auto addi = std::make_unique<MachineInstr>(RVOpcodes::ADDI);
                addi->addOperand(std::make_unique<RegOperand>(addr_vreg));
                addi->addOperand(std::make_unique<RegOperand>(PhysicalReg::S0));
                addi->addOperand(std::make_unique<ImmOperand>(offset));
                new_instructions.push_back(std::move(addi));

                // 展开为: lw/ld/flw dest_vreg, 0(addr_vreg)
                auto load_instr = std::make_unique<MachineInstr>(real_load_op);
                load_instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                load_instr->addOperand(std::make_unique<MemOperand>(
                    std::make_unique<RegOperand>(addr_vreg),
                    std::make_unique<ImmOperand>(0)));
                new_instructions.push_back(std::move(load_instr));

            } else if (opcode == RVOpcodes::FRAME_STORE_W || opcode == RVOpcodes::FRAME_STORE_D || opcode == RVOpcodes::FRAME_STORE_F) {
                RVOpcodes real_store_op;
                if (opcode == RVOpcodes::FRAME_STORE_W) real_store_op = RVOpcodes::SW;
                else if (opcode == RVOpcodes::FRAME_STORE_D) real_store_op = RVOpcodes::SD;
                else real_store_op = RVOpcodes::FSW;

                auto& operands = instr_ptr->getOperands();
                unsigned src_vreg = static_cast<RegOperand*>(operands[0].get())->getVRegNum();
                unsigned alloca_vreg = static_cast<RegOperand*>(operands[1].get())->getVRegNum();
                int offset = frame_info.alloca_offsets.at(alloca_vreg);
                auto addr_vreg = isel->getNewVReg(Type::getPointerType(Type::getIntType()));
                
                // 展开为: addi addr_vreg, s0, offset
                auto addi = std::make_unique<MachineInstr>(RVOpcodes::ADDI);
                addi->addOperand(std::make_unique<RegOperand>(addr_vreg));
                addi->addOperand(std::make_unique<RegOperand>(PhysicalReg::S0));
                addi->addOperand(std::make_unique<ImmOperand>(offset));
                new_instructions.push_back(std::move(addi));

                // 展开为: sw/sd/fsw src_vreg, 0(addr_vreg)
                auto store_instr = std::make_unique<MachineInstr>(real_store_op);
                store_instr->addOperand(std::make_unique<RegOperand>(src_vreg));
                store_instr->addOperand(std::make_unique<MemOperand>(
                    std::make_unique<RegOperand>(addr_vreg),
                    std::make_unique<ImmOperand>(0)));
                new_instructions.push_back(std::move(store_instr));

            } else if (instr_ptr->getOpcode() == RVOpcodes::FRAME_ADDR) { 
                auto& operands = instr_ptr->getOperands();
                unsigned dest_vreg = static_cast<RegOperand*>(operands[0].get())->getVRegNum();
                unsigned alloca_vreg = static_cast<RegOperand*>(operands[1].get())->getVRegNum();
                int offset = frame_info.alloca_offsets.at(alloca_vreg);

                // 将 `frame_addr rd, rs` 展开为 `addi rd, s0, offset`
                auto addi = std::make_unique<MachineInstr>(RVOpcodes::ADDI);
                addi->addOperand(std::make_unique<RegOperand>(dest_vreg));
                addi->addOperand(std::make_unique<RegOperand>(PhysicalReg::S0));
                addi->addOperand(std::make_unique<ImmOperand>(offset));
                new_instructions.push_back(std::move(addi));

            } else {
                new_instructions.push_back(std::move(instr_ptr));
            }
        }
        mbb->getInstructions() = std::move(new_instructions);
    }
}

} // namespace sysy