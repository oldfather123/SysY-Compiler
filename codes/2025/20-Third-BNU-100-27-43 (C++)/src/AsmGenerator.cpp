#include "AsmGenerator.h"
#include <iomanip>
// 是否需要生成块标签：函数不需要（或者只有main函数需要），其他需要
std::stringstream ss;
std::string AsmGenerator::generate(const MyIR::IRUnit &unit) {
    // std::stringstream ss;
    // 全局变量
    ss << ".data\n";
    for (const auto &gv : unit.global_variables) {
        genGlobal(gv.get(), ss);
    }

    ss << ".text\n";
    // 函数
    for (const auto &func : unit.functions) {
        if (!func->is_declaration())
            genFunction(func.get(), ss);
    }

    // 添加只读数据段，存放浮点常量
    if (!float_constants_.empty()) {
        ss << ".section .rodata\n";
        for (const auto& pair : float_constants_) {
            ss << pair.second << ":\n";
            ss << "    .float " << std::setprecision(12) << pair.first << "\n";
        }
    }

    return ss.str();
}

void AsmGenerator::genGlobal(const MyIR::GlobalVariable *gv, std::stringstream &ss) {
    ss << ".globl " << gv->get_name() << "\n";
    ss << gv->get_name() << ":\n";

    auto value_type = std::dynamic_pointer_cast<MyIR::PointerType>(gv->get_type())->get_element_type();
    if (auto array_type = std::dynamic_pointer_cast<MyIR::ArrayType>(value_type)) {
        // Check if it's a float array
        bool is_float_array = array_type->get_element_type()->get_type_id() == MyIR::TypeID::Float;
        
        // Handle array initializer
        if (auto array_init = std::dynamic_pointer_cast<MyIR::ConstantArray>(gv->get_initializer())) {
            // Uninitialized array (zero initialization)
            if (array_init->is_zero_initializer()) {
                size_t total_size = 1;
                auto current_type = array_type;
                
                // Calculate total number of elements
                while (current_type) {
                    total_size *= current_type->get_num_elements();
                    auto element_type = current_type->get_element_type();
                    current_type = std::dynamic_pointer_cast<MyIR::ArrayType>(element_type);
                }
                
                // Calculate total bytes: float uses 8 bytes, int uses 4 bytes
                size_t bytes_per_element = 4;
                ss << "    .zero " << (total_size * bytes_per_element) << "\n";
            } else {
                // Handle explicit initialization
                genArrayInitializer(array_init, ss);
            }
        }
    } else {
        // Handle non-array global variables
        if (auto init = std::dynamic_pointer_cast<MyIR::ConstantInt>(gv->get_initializer())) {
            ss << "    .word " << init->get_value() << "\n";
        } else if (auto float_init = std::dynamic_pointer_cast<MyIR::ConstantFloat>(gv->get_initializer())) {
            ss << "    .float " << std::setprecision(12) << float_init->get_value() << "\n";
        } else {
            // Uninitialized scalar: int uses .word, float uses .double
            if (value_type->get_type_id() == MyIR::TypeID::Float) {
                ss << "    .float 0.0\n";
            } else {
                ss << "    .word 0\n";
            }
        }
    }
}

// 新增：递归处理数组初始化器
void AsmGenerator::genArrayInitializer(const std::shared_ptr<MyIR::ConstantArray>& array_init, std::stringstream& ss) {
    const auto& elements = array_init->get_elements();
    auto array_type = std::dynamic_pointer_cast<MyIR::ArrayType>(array_init->get_type());
    bool is_float_array = array_type->get_element_type()->get_type_id() == MyIR::TypeID::Float;
    
    for (const auto& element : elements) {
        if (auto const_array = std::dynamic_pointer_cast<MyIR::ConstantArray>(element)) {
            // 递归处理子数组
            genArrayInitializer(const_array, ss);
            
            // 补齐未初始化的元素为0
            size_t array_size = std::dynamic_pointer_cast<MyIR::ArrayType>(const_array->get_type())->get_num_elements();
            size_t init_size = const_array->get_elements().size();
            if (init_size < array_size) {
                size_t padding = array_size - init_size;
                for (size_t i = 0; i < padding; i++) {
                    if (is_float_array) {
                        ss << "    .float 0.0\n";  // 4字节对齐
                    } else {
                        ss << "    .word 0\n";      // 4字节对齐
                    }
                }
            }
        } else if (auto const_int = std::dynamic_pointer_cast<MyIR::ConstantInt>(element)) {
            ss << "    .word " << const_int->get_value() << "\n";
        } else if (auto const_float = std::dynamic_pointer_cast<MyIR::ConstantFloat>(element)) {
            ss << "    .float " << std::setprecision(12) << const_float->get_value() << "\n";
        }
    }
}

void AsmGenerator::allocStack(const MyIR::Function *func) {
    stack_size_ = 0;
    stack_offset_.clear();
    
    // 分配返回地址存储空间
    stack_size_ += 8;  // ra 寄存器
    
    // 为参数分配栈空间
    // for (const auto& arg : func->get_arguments()) {
    //     stack_offset_[arg.get()] = stack_size_;
    //     stack_size_ += 8;  // 每个参数分配8字节对齐
    // }
    
    // 为局部变量分配栈空间
    for (const auto& bb : func->get_basic_blocks()) {
        for (const auto& inst : bb->get_instructions()) {
            if (inst->get_opcode() == MyIR::Opcode::Alloca) {
                auto alloca = std::dynamic_pointer_cast<MyIR::AllocaInst>(inst);
                auto allocated_type = alloca->get_allocated_type();
                
                size_t alloc_size;
                if (auto array_type = std::dynamic_pointer_cast<MyIR::ArrayType>(allocated_type)) {
                    // 数组类型：计算总大小
                    bool is_float_array = array_type->get_element_type()->get_type_id() == MyIR::TypeID::Float;
                    alloc_size = 4;  // 基础大小
                    auto current_type = array_type;
                    while (current_type) {
                        alloc_size *= current_type->get_num_elements();
                        auto element_type = current_type->get_element_type();
                        current_type = std::dynamic_pointer_cast<MyIR::ArrayType>(element_type);
                    }
                } else {
                    // 标量类型
                    alloc_size = 8;
                }
                
                // 记录变量在栈上的偏移量
                stack_offset_[inst.get()] = stack_size_ - 8;
                stack_size_ += alloc_size;
            }
        }
    }
    
    // 16字节对齐
    stack_size_ = (stack_size_ + 15) & ~15;
}

std::string AsmGenerator::getLabel(const MyIR::BasicBlock *bb) {
    if (!bb) return "";

    std::string name = bb->get_name();
    if (name.empty()) {
        // 为没有名字的基本块生成标签
        name = "bb." + std::to_string(reinterpret_cast<uintptr_t>(bb));
    }
    return name;
}

void AsmGenerator::genFunction(const MyIR::Function *func, std::stringstream &ss) {
    // 在函数入口处初始化寄存器池
    initRegisterPool();
    value2reg_.clear();
    reg2value_.clear();
    spill_locations_.clear();
    used_regs_.clear();

    // 函数标签
    ss << ".globl " << func->get_name() << "\n";
    ss << func->get_name() << ":\n";
    
    // 栈帧分配
    allocStack(func);
    int sz = stack_size_;
    while (sz > 2048) {
        ss << "  addi sp, sp, -2048\n";
        sz -= 2048;
    }
    ss << "  addi sp, sp, -" << sz << "\n";
    
    if (stack_size_ - 8 >= 2048) {
        std::string reg = allocTempReg(false);
        ss << "  li " << reg << ", " << stack_size_ - 8 << '\n';
        ss << "  add " << reg << ", " << "sp, " << reg << '\n';
        ss << "  sd ra, " << "0(" << reg << ")\n";
        freeReg(reg);
    } else {
        ss << "  sd ra, " << stack_size_ - 8 << "(sp)" << "\n";
    }
    
    // 分类计算参数序号以及caller栈中参数的偏移量
    int float_idx = 0, int_idx = 0, offset = 0;
    const auto &args = func->get_arguments();
    for (const auto &arg : args) {
        if (arg->get_type()->get_type_id() == MyIR::TypeID::Float) {
            float_arg_idx[arg.get()] = float_idx++;
            if (float_arg_idx[arg.get()] >= 8) arg_offset[arg.get()] = offset, offset += 8;
        } else {
            int_arg_idx[arg.get()] = int_idx++;
            if (int_arg_idx[arg.get()] >= 8) arg_offset[arg.get()] = offset, offset += 8;
        }
    }
    
    // 生成基本块
    for (const auto &bb : func->get_basic_blocks()) {
        genBasicBlock(bb.get(), ss);
    }
    // 函数尾声
    ss << '_' << func->get_name() << "_exit:\n";

    if (stack_size_ - 8 >= 2048) {
        std::string reg = allocTempReg(false);
        ss << "  li " << reg << ", " << stack_size_ - 8 << '\n';
        ss << "  add " << reg << ", " << "sp, " << reg << '\n';
        ss << "  ld ra, " << "0(" << reg << ")\n";
        freeReg(reg);
    } else {
        ss << "  ld ra, " << stack_size_ - 8 << "(sp)" << "\n";
    }

    sz = stack_size_;
    while (sz >= 2048) {
        ss << "  addi sp, sp, 2047\n";
        sz -= 2047;
    }
    ss << "  addi sp, sp, " << sz << "\n";
    ss << "  ret\n";
}

int counter = 0;

void AsmGenerator::genBasicBlock(const MyIR::BasicBlock *bb, std::stringstream &ss) {
    // Generate block label
    std::string name = bb->get_name();
    if (!name.empty()) {
        if (name == "entry") {
            if (bb->get_parent()->get_name() == "main") ss << name << ":\n";
        } else {
            ss << name << ":\n";
        }
    }
    
    // Find call instructions with more than 16 parameters
    struct CallBlock {
        const MyIR::CallInst* call;
        std::vector<const MyIR::Instruction*> related_insts;
    };
    std::vector<CallBlock> large_calls;

    // First pass: identify calls with >16 params and their related instructions
    for (const auto& inst : bb->get_instructions()) {
        if (auto call = dynamic_cast<const MyIR::CallInst*>(inst.get())) {
            if (call->get_num_operands() > 17) { // >16 params (operand[0] is callee)
                CallBlock block{call, {}};
                
                // Track back to find instructions generating parameters
                for (size_t i = 1; i < call->get_num_operands(); i++) {
                    if (auto param_inst = dynamic_cast<const MyIR::Instruction*>(call->get_operand(i))) {
                        block.related_insts.push_back(param_inst);
                    }
                }
                if (block.related_insts.size() > 16) large_calls.push_back(block);
            }
        }
    }

    struct GEPBlock {
        const MyIR::GetElementPtrInst* gep;
        std::vector<const MyIR::Instruction*> related_insts;
    };
    std::vector<GEPBlock> large_geps;

    // Second pass: identify gep with >16 indices and their related instructions
    for (const auto& inst : bb->get_instructions()) {
        if (auto gep = dynamic_cast<const MyIR::GetElementPtrInst*>(inst.get())) {
            auto indices = gep->get_indices();
            if (indices.size() > 16) { // >16 indices
                GEPBlock block{gep, {}};
                
                // Track back to find instructions generating indices
                for (size_t i = 0; i < indices.size(); i++) {
                    if (auto indice_inst = dynamic_cast<const MyIR::Instruction*>(indices[i])) {
                        block.related_insts.push_back(indice_inst);
                    }
                }
                if (block.related_insts.size() > 16) large_geps.push_back(block);
            }
        }
    }

    // Third pass: generate instructions
    for (const auto& inst : bb->get_instructions()) {
        bool in_special_call_block = false;
        bool is_first_inst_of_call_block = false;
        int block_idx = -1;
        // Check if this instruction is part of a special call block
        for (int i = 0; i < large_calls.size(); i++) {
            // 修改 find 的用法，改用普通循环
            const auto & block = large_calls[i];
            bool found = false;
            for (const auto* inst_ptr : block.related_insts) {
                if (inst_ptr == inst.get()) {
                    found = true;
                    break;
                }
            }
            if (found || inst.get() == block.call) {
                if (inst.get() == block.related_insts[0]) is_first_inst_of_call_block = true;
                in_special_call_block = true;
                block_idx = i;
                break;
            }
        }

        bool in_special_gep_block = false;
        bool is_first_inst_of_gep_block = false;
        // Check if this instruction is part of a special gep block
        for (const auto& block : large_geps) {
            // 修改 find 的用法，改用普通循环
            bool found = false;
            for (const auto* inst_ptr : block.related_insts) {
                if (inst_ptr == inst.get()) {
                    found = true;
                    break;
                }
            }
            if (found || inst.get() == block.gep) {
                if (inst.get() == block.related_insts[0]) is_first_inst_of_gep_block = true;
                in_special_gep_block = true;
                break;
            }
        }

        // Add special block marker if needed
        // arg_stack_size = 0;
        if (is_first_inst_of_call_block) {
            saveActiveTempRegs(ss);
            // auto call_block = large_calls[0];
            auto& call_block = large_calls[block_idx];
            // ss << "  # Begin special block for large parameter call\n";
            // genCallPrelogue(large_calls[0]->call);
            arg_stack_size = (call_block.call->get_num_operands() - 9) * 8;
            int sz = arg_stack_size;
            while (sz > 2048) {
                ss << "  addi sp, sp, -2048\n";
                sz -= 2048;
            }
            ss << "  addi sp, sp, -" << sz << '\n';
        }

        if (is_first_inst_of_gep_block) {
            auto &gep_block = large_geps[0];
            std::string rd = getReg(gep_block.gep);
            auto base = gep_block.gep->get_operand(0);
            
            // Get array element type
            auto ptr_type = std::dynamic_pointer_cast<MyIR::PointerType>(base->get_type());
            auto array_type = std::dynamic_pointer_cast<MyIR::ArrayType>(ptr_type->get_element_type());
            // bool is_float_array = array_type->get_element_type()->get_type_id() == MyIR::TypeID::Float;
            // ss << "ok\n";
            // return;
            // 基地址计算
            if (auto global = dynamic_cast<const MyIR::GlobalVariable*>(base)) {
                ss << "  la " << rd << ", " << global->get_name() << "\n";
            } else if (array_base_.count(base)) {
                int base_offset = array_base_[base] + arg_stack_size + caller_save_offset_;
                if (base_offset >= 2048) {
                    std::string tmp = allocTempReg(false);
                    ss << "  li " << tmp << ", " << base_offset << "\n";
                    ss << "  add " << rd << ", sp, " << tmp << "\n";
                    freeReg(tmp);
                } else {
                    ss << "  addi " << rd << ", sp, " << base_offset << "\n";
                }
            } else {
                std::string base_reg = getReg(base);
                ss << "  mv " << rd << ", " << base_reg << "\n";
                freeReg(base_reg);
            }
        }

        // Generate instruction
        if (!in_special_call_block && !in_special_gep_block) genInstruction(inst.get(), ss);

        if (in_special_gep_block) {
            auto& gep_block = large_geps[0];
            if (inst.get() != gep_block.gep) {
                genInstruction(inst.get(), ss);

                int ind_no = -1;
                // 查找当前指令在索引列表中的位置
                for (size_t i = 0; i < gep_block.related_insts.size(); ++i) {
                    if (gep_block.related_insts[i] == inst.get()) {
                        ind_no = i;
                        break;
                    }
                }
                auto dims = getArrayDimensions(gep_block.gep);
        
                // 计算步长
                int stride = 4;
                for (int i = ind_no + 1; i < dims.size(); i++) stride *= (int) dims[i];
                std::string rd = getReg(gep_block.gep);
                std::string idx_reg = getReg(inst.get());
                std::string tmp = allocTempReg(false);
                ss << "  li " << tmp << ", " << stride << "\n";
                ss << "  mul " << tmp << ", " << idx_reg << ", " << tmp << "\n";
                ss << "  add " << rd << ", " << rd << ", " << tmp << "\n";
                freeReg(idx_reg);
                freeReg(tmp);
            }
        }

        // Close special block marker if needed
        if (in_special_call_block) {
            auto& call_block = large_calls[block_idx];
            if (inst.get() != call_block.call) {
                genInstruction(inst.get(), ss);
                // ss << "  # End special block for large parameter call\n";
                int arg_no = -1;
                // 查找当前指令在参数列表中的位置
                for (size_t i = 0; i < call_block.related_insts.size(); ++i) {
                    if (call_block.related_insts[i] == inst.get()) {
                        arg_no = i;
                        break;
                    }
                }
                
                if (arg_no != -1) {
                    auto arg = call_block.call->get_operand(arg_no + 1);
                    int arg_idx = 0;
                    std::string arg_reg = getReg(inst.get());
                    if (arg->get_type()->get_type_id() == MyIR::TypeID::Float) {
                        for (int i = 1; i < arg_no + 1; i++) {
                            auto arg = call_block.call->get_operand(i);
                            arg_idx += arg->get_type()->get_type_id() == MyIR::TypeID::Float;
                        }
                        if (arg_idx < 8) ss << "  fmv.s fa" << arg_idx << ", " << arg_reg << "\n";
                        else {
                            int offset = counter * 8;
                            counter++;
                            ss << "  fsw " << arg_reg << ", " << offset << "(sp)\n";
                        }
                    } else {
                        for (int i = 1; i < arg_no + 1; i++) {
                            auto arg = call_block.call->get_operand(i);
                            arg_idx += arg->get_type()->get_type_id() != MyIR::TypeID::Float;
                        }
                        if (arg_idx < 8) {
                            ss << "  mv a" << arg_idx << ", " << arg_reg << "\n";
                        }
                        else {
                            int offset = counter * 8;
                            counter++;
                            if (arg->get_type()->get_type_id() == MyIR::TypeID::Pointer) {
                                ss << "  sd " << arg_reg << ", " << offset << "(sp)\n";
                            } else {
                                ss << "  sw " << arg_reg << ", " << offset << "(sp)\n";
                            }
                        }
                    }
                    freeReg(arg_reg);
                }
            } else {
                counter = 0;
                for (size_t i = 1; i < call_block.call->get_num_operands(); i++) {
                    if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(call_block.call->get_operand(i))) {
                        std::string temp = allocTempReg(false);
                        ss << "  li " << temp << ", " << const_int->get_value() << '\n';
                        ss << "  sw " << temp << ", " << arg_stack_size - 8 << "(sp)\n";
                        freeReg(temp);
                    }
                }
                ss << "  call " << call_block.call->get_operand(0)->get_name() << "\n";
                int sz = arg_stack_size;
                while (sz >= 2048) {
                    ss << "  addi sp, sp, 2047\n";
                    sz -= 2047;
                }
                ss << "  addi sp, sp, " << sz << "\n";
                arg_stack_size = 0;
                
                restoreActiveTempRegs(ss);
                
                if (call_block.call->get_type()->get_type_id() != MyIR::TypeID::Void) {
                    std::string rd = getReg(inst.get());
                    if (call_block.call->get_type()->get_type_id() == MyIR::TypeID::Float) {
                        ss << "  fmv.s " << rd << ", fa0\n";
                    } else {
                        ss << "  mv " << rd << ", a0\n";
                    }
                }
            }
            
        }
    }
}

void analyzeCallInstOperands(const MyIR::CallInst* call) {
    ss << "Analyzing call instruction operands:\n";
    
    for (size_t i = 1; i < call->get_num_operands(); i++) {
        MyIR::Value* arg = call->get_operand(i);
        ss << "Argument " << i << ": ";
        
        if (auto inst = dynamic_cast<const MyIR::Instruction*>(arg)) {
            ss << "Defined by instruction: " << inst->get_name() 
                     << " (opcode: " << static_cast<int>(inst->get_opcode()) << ")\n";
        } else if (auto constant = dynamic_cast<const MyIR::Constant*>(arg)) {
            ss << "Is a constant\n";
        } else if (auto global = dynamic_cast<const MyIR::GlobalVariable*>(arg)) {
            ss << "Is global variable: " << global->get_name() << "\n";
        } else if (auto argument = dynamic_cast<const MyIR::Argument*>(arg)) {
            ss << "Is function argument #" << argument->get_arg_no() << "\n";
        }
    }
}


void AsmGenerator::genInstruction(const MyIR::Instruction *inst, std::stringstream &ss) {
    switch (inst->get_opcode()) {
        case MyIR::Opcode::Add: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1;
            
            // 处理第一个操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                rs1 = allocTempReg(false);
                ss << "  li " << rs1 << ", " << const_int->get_value() << "\n";
            } else {
                rs1 = getReg(bin->get_operand(0));
            }
            
            // 第二个操作数尝试使用立即数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(1))) {
                int value = const_int->get_value();
                if (value >= -2048 && value <= 2047) {
                    ss << "  addiw " << rd << ", " << rs1 << ", " << value << "\n";
                } else {
                    std::string rs2 = allocTempReg(false);
                    ss << "  li " << rs2 << ", " << value << "\n";
                    ss << "  addw " << rd << ", " << rs1 << ", " << rs2 << "\n";
                    freeReg(rs2);
                }
            } else {
                std::string rs2 = getReg(bin->get_operand(1));
                ss << "  addw " << rd << ", " << rs1 << ", " << rs2 << "\n";
                freeReg(rs2);  // 释放第二个操作数寄存器
            }
            
            freeReg(rs1);  // 释放第一个操作数寄存器
            break;
        }

        case MyIR::Opcode::Sub: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1;
            
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                rs1 = allocTempReg(false);
                ss << "  li " << rs1 << ", " << const_int->get_value() << "\n";
            } else {
                rs1 = getReg(bin->get_operand(0));
            }
            
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(1))) {
                int value = const_int->get_value();
                if (value >= -2047 && value <= 2048) {
                    ss << "  addi " << rd << ", " << rs1 << ", " << (-value) << "\n";
                } else {
                    std::string rs2 = allocTempReg(false);
                    ss << "  li " << rs2 << ", " << value << "\n";
                    ss << "  sub " << rd << ", " << rs1 << ", " << rs2 << "\n";
                    freeReg(rs2);
                }
            } else {
                std::string rs2 = getReg(bin->get_operand(1));
                ss << "  sub " << rd << ", " << rs1 << ", " << rs2 << "\n";
                freeReg(rs2);
            }
            
            freeReg(rs1);
            break;
        }

        case MyIR::Opcode::Mul: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1;
            
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                rs1 = allocTempReg(false);
                ss << "  li " << rs1 << ", " << const_int->get_value() << "\n";
            } else {
                rs1 = getReg(bin->get_operand(0));
            }
            
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(1))) {
                std::string rs2 = allocTempReg(false);
                ss << "  li " << rs2 << ", " << const_int->get_value() << "\n";
                ss << "  mulw " << rd << ", " << rs1 << ", " << rs2 << "\n";
                freeReg(rs2);
            } else {
                std::string rs2 = getReg(bin->get_operand(1));
                ss << "  mulw " << rd << ", " << rs1 << ", " << rs2 << "\n";
                freeReg(rs2);
            }
            
            freeReg(rs1);
            break;
        }
        case MyIR::Opcode::SDiv: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2;
            
            // 处理第一个操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                rs1 = allocTempReg(false);
                ss << "  li " << rs1 << ", " << const_int->get_value() << "\n";
            } else {
                rs1 = getReg(bin->get_operand(0));
            }
            
            // 处理第二个操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(1))) {
                rs2 = allocTempReg(false);
                ss << "  li " << rs2 << ", " << const_int->get_value() << "\n";
            } else {
                rs2 = getReg(bin->get_operand(1));
            }
            
            ss << "  div " << rd << ", " << rs1 << ", " << rs2 << "\n";
            // 释放临时寄存器
            // if (dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                freeReg(rs1);
            // }
            // if (dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(1))) {
                freeReg(rs2);
            // }
            break;
        }
        case MyIR::Opcode::SRem: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2;
            
            // 处理第一个操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                rs1 = allocTempReg(false);
                ss << "  li " << rs1 << ", " << const_int->get_value() << "\n";
            } else {
                rs1 = getReg(bin->get_operand(0));
            }
            
            // 处理第二个操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(1))) {
                rs2 = allocTempReg(false);
                ss << "  li " << rs2 << ", " << const_int->get_value() << "\n";
            } else {
                rs2 = getReg(bin->get_operand(1));
            }
            
            // RISC-V 使用 rem 指令进行有符号整数取模运算
            ss << "  rem " << rd << ", " << rs1 << ", " << rs2 << "\n";
            // 释放临时寄存器
            // if (dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                freeReg(rs1);
            // }
            // if (dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(1))) {
                freeReg(rs2);
            // }
            break;
        }
        case MyIR::Opcode::And: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2;

            // 处理第一个操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                rs1 = allocTempReg(false);
                ss << "  li " << rs1 << ", " << const_int->get_value() << "\n";
            } else {
                rs1 = getReg(bin->get_operand(0));
            }
            
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(1))) {
                int value = const_int->get_value();
                if (value >= -2048 && value <= 2047) {
                    ss << "  andi " << rd << ", " << rs1 << ", " << value << "\n";
                } else {
                    rs2 = allocTempReg(false);
                    ss << "  li " << rs2 << ", " << value << "\n";
                    ss << "  and " << rd << ", " << rs1 << ", " << rs2 << "\n";
                    freeReg(rs2);
                }
            } else {
                rs2 = getReg(bin->get_operand(1));
                ss << "  and " << rd << ", " << rs1 << ", " << rs2 << "\n";
                freeReg(rs2);
            }

            // 释放临时寄存器
            // if (dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                freeReg(rs1);
            // }
            break;
        }
         case MyIR::Opcode::Or: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2;

            // 处理第一个操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                rs1 = allocTempReg(false);
                ss << "  li " << rs1 << ", " << const_int->get_value() << "\n";
            } else {
                rs1 = getReg(bin->get_operand(0));
            }
            
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(1))) {
                int value = const_int->get_value();
                if (value >= -2048 && value <= 2047) {
                    ss << "  ori " << rd << ", " << rs1 << ", " << value << "\n";
                } else {
                    rs2 = allocTempReg(false);
                    ss << "  li " << rs2 << ", " << value << "\n";
                    ss << "  or " << rd << ", " << rs1 << ", " << rs2 << "\n";
                    freeReg(rs2);
                }
            } else {
                rs2 = getReg(bin->get_operand(1));
                ss << "  or " << rd << ", " << rs1 << ", " << rs2 << "\n";
                freeReg(rs2);
            }
            // 释放临时寄存器
            // if (dynamic_cast<const MyIR::ConstantInt*>(bin->get_operand(0))) {
                freeReg(rs1);
            // }
            break;
        }
        case MyIR::Opcode::Load: {
            auto load = dynamic_cast<const MyIR::LoadInst *>(inst);
            std::string rd = getReg(inst);
            auto addr = load->get_operand(0);
            bool is_pointer = inst->get_type()->get_type_id() == MyIR::TypeID::Pointer;
            
            if (auto global = dynamic_cast<const MyIR::GlobalVariable *>(addr)) {
                std::string temp = allocTempReg(false);
                ss << "  la " << temp << ", " << addr->get_name() << "\n";
                if (is_pointer) {
                    ss << "  ld " << rd << ", 0(" << temp << ")\n";
                } else if (inst->get_type()->get_type_id() == MyIR::TypeID::Float) {
                    ss << "  flw " << rd << ", 0(" << temp << ")\n";
                } else {
                    ss << "  lw " << rd << ", 0(" << temp << ")\n";
                }
                freeReg(temp);
            } else if (auto alloca = dynamic_cast<const MyIR::AllocaInst *>(addr)) {
                // ss << arg_stack_size << '\n';
                int offset = stack_offset_[addr] + arg_stack_size + caller_save_offset_;
                if (is_pointer) {
                    if (offset >= 2048) {
                        std::string reg = allocTempReg(false);
                        ss << "  li " << reg << ", " << offset << '\n';
                        ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                        ss << "  ld " << rd << ", " << "0(" << reg << ")\n";
                        freeReg(reg);
                    } else ss << "  ld " << rd << ", " << offset << "(sp)\n";
                } else if (inst->get_type()->get_type_id() == MyIR::TypeID::Float) {
                    if (offset >= 2048) {
                        std::string reg = allocTempReg(true);
                        ss << "  li " << reg << ", " << offset << '\n';
                        ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                        ss << "  flw " << rd << ", " << "0(" << reg << ")\n";
                        freeReg(reg);
                    } else ss << "  flw " << rd << ", " << offset << "(sp)\n";
                } else {
                    if (offset >= 2048) {
                        std::string reg = allocTempReg(false);
                        ss << "  li " << reg << ", " << offset << '\n';
                        ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                        ss << "  lw " << rd << ", " << "0(" << reg << ")\n";
                        freeReg(reg);
                    } else ss << "  lw " << rd << ", " << offset << "(sp)\n";
                }
            } else {
                std::string addr_reg = getReg(addr);
                if (is_pointer) {
                    ss << "  ld " << rd << ", 0(" << addr_reg << ")\n";
                } else if (inst->get_type()->get_type_id() == MyIR::TypeID::Float) {
                    // std::string float_temp = allocTempReg(true);
                    ss << "  flw " << rd << ", 0(" << addr_reg << ")\n";
                    // ss << "  flw " << float_temp << ", 0(" << addr_reg << ")\n";
                    // ss << "  fcvt.d.s " << rd << ", " << float_temp << "\n";
                    // freeReg(float_temp);
                } else {
                    ss << "  lw " << rd << ", 0(" << addr_reg << ")\n";
                }
                freeReg(addr_reg);
            }
            break;
        }

        case MyIR::Opcode::Alloca: {
            auto alloca = dynamic_cast<const MyIR::AllocaInst *>(inst);
            auto allocated_type = alloca->get_allocated_type();
            // 检查是否是数组类型
            if (allocated_type->get_type_id() == MyIR::TypeID::Array) {
                // 对于数组，只需记录其在栈中的位置，不分配寄存器
                array_base_[inst] = stack_offset_[inst];
            } else {
                // 非数组类型，为其分配一个寄存器
                // ss << "not array\n";
                // std::string reg = getReg(inst);
                // local_var_map[inst->get_name()] = reg;
                // 令该寄存器指向变量对应栈帧
                // ss << "  addi " << reg << ", sp, " << stack_offset_[inst] << '\n';
                }
            // ss << temp_reg_counter_ << '\n';
            break;
        }
        case MyIR::Opcode::Store: {
            auto store = dynamic_cast<const MyIR::StoreInst *>(inst);
            auto val = store->get_operand(0);
            auto ptr = store->get_operand(1);
            bool is_pointer = val->get_type()->get_type_id() == MyIR::TypeID::Pointer;
            bool is_float = val->get_type()->get_type_id() == MyIR::TypeID::Float;
            
            if (auto arg = dynamic_cast<const MyIR::Argument*>(val)) {
                // 获取参数在参数列表中的位置
                // size_t arg_idx = arg->get_arg_no();
                int arg_idx;
                int offset = stack_offset_[ptr];
                if (is_pointer) {
                    arg_idx = int_arg_idx[arg];
                    if (arg_idx < 8) {
                        if (offset >= 2048) {
                            std::string reg = allocTempReg(false);
                            ss << "  li " << reg << ", " << offset << '\n';
                            ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                            ss << "  sd a" << arg_idx << ", " << "0(" << reg << ")\n";
                            freeReg(reg);
                        } else ss << "  sd a" << arg_idx << ", " << offset << "(sp)\n";
                    } else {
                        int val_offset = stack_size_ + arg_offset[arg];
                        std::string reg = allocTempReg(false);
                        if (val_offset >= 2048) {
                            std::string temp = allocTempReg(false);
                            ss << "  li " << temp << ", " << val_offset << '\n';
                            ss << "  add " << temp << ", " << "sp, " << temp << '\n';
                            ss << "  ld " << reg << ", " << "0(" << temp << ")\n";
                            freeReg(temp);
                        } else ss << "  ld " << reg << ", " << val_offset << "(sp)\n";
                        if (offset >= 2048) {
                            std::string temp = allocTempReg(false);
                            ss << "  li " << temp << ", " << offset << '\n';
                            ss << "  add " << temp << ", " << "sp, " << temp << '\n';
                            ss << "  sd " << reg << ", " << "0(" << temp << ")\n";
                            freeReg(temp);
                        } else ss << "  sd " << reg << ", " << offset << "(sp)\n"; 
                        freeReg(reg);
                    }
                } else if (is_float) {
                    // 浮点型参数
                    arg_idx = float_arg_idx[arg];
                    if (arg_idx < 8) {
                        if (offset >= 2048) {
                            std::string reg = allocTempReg(false);
                            ss << "  li " << reg << ", " << offset << '\n';
                            ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                            ss << "  fsw fa" << arg_idx << ", " << "0(" << reg << ")\n";
                            freeReg(reg);
                        } else ss << "  fsw fa" << arg_idx << ", " << offset << "(sp)\n";
                    } else {
                        int val_offset = stack_size_ + arg_offset[arg];
                        std::string reg = allocTempReg(true);
                        if (val_offset >= 2048) {
                            std::string temp = allocTempReg(false);
                            ss << "  li " << temp << ", " << val_offset << '\n';
                            ss << "  add " << temp << ", " << "sp, " << temp << '\n';
                            ss << "  flw " << reg << ", " << "0(" << temp << ")\n";
                            freeReg(temp);
                        } else ss << "  flw " << reg << ", " << val_offset << "(sp)\n";
                        if (offset >= 2048) {
                            std::string temp = allocTempReg(false);
                            ss << "  li " << temp << ", " << offset << '\n';
                            ss << "  add " << temp << ", " << "sp, " << temp << '\n';
                            ss << "  fsw " << reg << ", " << "0(" << temp << ")\n";
                            freeReg(temp);
                        } else ss << "  fsw " << reg << ", " << offset << "(sp)\n"; 
                        freeReg(reg);
                    }
                } else { // 整型参数
                    arg_idx = int_arg_idx[arg];
                    if (arg_idx < 8) {
                        if (offset >= 2048) {
                            std::string reg = allocTempReg(false);
                            ss << "  li " << reg << ", " << offset << '\n';
                            ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                            ss << "  sw a" << arg_idx << ", " << "0(" << reg << ")\n";
                            freeReg(reg);
                        } else ss << "  sw a" << arg_idx << ", " << offset << "(sp)\n";
                    } else {
                        int val_offset = stack_size_ + arg_offset[arg];
                        std::string reg = allocTempReg(false);
                        if (val_offset >= 2048) {
                            std::string temp = allocTempReg(false);
                            ss << "  li " << temp << ", " << val_offset << '\n';
                            ss << "  add " << temp << ", " << "sp, " << temp << '\n';
                            ss << "  lw " << reg << ", " << "0(" << temp << ")\n";
                            freeReg(temp);
                        } else ss << "  lw " << reg << ", " << val_offset << "(sp)\n";
                        if (offset >= 2048) {
                            std::string temp = allocTempReg(false);
                            ss << "  li " << temp << ", " << offset << '\n';
                            ss << "  add " << temp << ", " << "sp, " << temp << '\n';
                            ss << "  sw " << reg << ", " << "0(" << temp << ")\n";
                            freeReg(temp);
                        } else ss << "  sw " << reg << ", " << offset << "(sp)\n"; 
                        freeReg(reg);
                    }
                }
            } else if (auto global = dynamic_cast<const MyIR::GlobalVariable*>(ptr)) {
                std::string addr_reg = allocTempReg(false);
                ss << "  la " << addr_reg << ", " << global->get_name() << "\n";
                std::string val_reg;
                if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(val)) {
                    val_reg = allocTempReg(false);
                    ss << "  li " << val_reg << ", " << const_int->get_value() << "\n";
                    if (is_pointer) {
                        ss << "  sd " << val_reg << ", 0(" << addr_reg << ")\n";
                    } else {
                        ss << "  sw " << val_reg << ", 0(" << addr_reg << ")\n";
                    }
                    freeReg(val_reg);
                } else {
                    val_reg = getReg(val);
                    if (is_pointer) {
                        ss << "  sd " << val_reg << ", 0(" << addr_reg << ")\n";
                    } else if (val->get_type()->get_type_id() == MyIR::TypeID::Float) {
                        ss << "  fsw " << val_reg << ", 0(" << addr_reg << ")\n";
                    } else {
                        ss << "  sw " << val_reg << ", 0(" << addr_reg << ")\n";
                    }
                    freeReg(val_reg);
                }
                freeReg(addr_reg);
            } else if (auto alloca = dynamic_cast<const MyIR::AllocaInst *>(ptr)) {
                int offset = stack_offset_[ptr];
                std::string val_reg;
                if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(val)) {
                    val_reg = allocTempReg(false);
                    ss << "  li " << val_reg << ", " << const_int->get_value() << "\n";
                    if (offset >= 2048) {
                        std::string reg = allocTempReg(false);
                        ss << "  li " << reg << ", " << offset << '\n';
                        ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                        ss << "  sw " << val_reg << ", " << "0(" << reg << ")\n";
                        freeReg(reg);
                    } else ss << "  sw " << val_reg << ", " << offset << "(sp)\n";
                    freeReg(val_reg);
                } else if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(val)) {
                    val_reg = allocTempReg(true);
                    std::string temp = allocTempReg(false);
                    ss << "  la " << temp << ", " << getFloatConstLabel(const_float->get_value()) << "\n";
                    ss << "  flw " << val_reg << ", 0(" << temp << ")\n"; 
                    freeReg(temp);
                    if (offset >= 2048) {
                        std::string reg = allocTempReg(false);
                        ss << "  li " << reg << ", " << offset << '\n';
                        ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                        ss << "  fsw " << val_reg << ", " << "0(" << reg << ")\n";
                        freeReg(reg);
                    } else ss << "  fsw " << val_reg << ", " << offset << "(sp)\n";
                    freeReg(val_reg);
                } else {
                    val_reg = getReg(val);
                    if (is_pointer) {
                        if (offset >= 2048) {
                            std::string reg = allocTempReg(false);
                            ss << "  li " << reg << ", " << offset << '\n';
                            ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                            ss << "  sd " << val_reg << ", " << "0(" << reg << ")\n";
                            freeReg(reg);
                        } else ss << "  sd " << val_reg << ", " << offset << "(sp)\n";
                    } else if (is_float) {
                        if (offset >= 2048) {
                            std::string reg = allocTempReg(true);
                            ss << "  li " << reg << ", " << offset << '\n';
                            ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                            ss << "  fsw " << val_reg << ", " << "0(" << reg << ")\n";
                            freeReg(reg);
                        } else ss << "  fsw " << val_reg << ", " << offset << "(sp)\n";
                    } else {
                        if (offset >= 2048) {
                            std::string reg = allocTempReg(false);
                            ss << "  li " << reg << ", " << offset << '\n';
                            ss << "  add " << reg << ", " << "sp, " << reg << '\n';
                            ss << "  sw " << val_reg << ", " << "0(" << reg << ")\n";
                            freeReg(reg);
                        } else ss << "  sw " << val_reg << ", " << offset << "(sp)\n";
                    }
                    freeReg(val_reg);
                }
            } else {
                std::string val_reg, addr_reg = getReg(ptr);
                if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(val)) {
                    val_reg = allocTempReg(false);
                    ss << "  li " << val_reg << ", " << const_int->get_value() << "\n";
                    ss << "  sw " << val_reg << ", 0(" << addr_reg << ")\n";
                    freeReg(val_reg);
                } else if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(val)) {
                    val_reg = allocTempReg(true);
                    std::string temp = allocTempReg(false);
                    ss << "  la " << temp << ", " << getFloatConstLabel(const_float->get_value()) << '\n';
                    ss << "  flw " << val_reg  << ", 0(" << temp << ")\n";
                    freeReg(temp);
                    ss << "  fsw " << val_reg << ", 0(" << addr_reg << ")\n";
                    freeReg(val_reg);
                } else {
                    val_reg = getReg(val);
                    if (is_pointer) {
                        ss << "  sd " << val_reg << ", 0(" << addr_reg << ")\n";
                    } else if (is_float) {
                        ss << "  fsw " << val_reg << ", 0(" << addr_reg << ")\n";
                    } else {
                        ss << "  sw " << val_reg << ", 0(" << addr_reg << ")\n";
                    }
                    freeReg(val_reg);
                }
                freeReg(addr_reg);
            }
            break;
        }
        case MyIR::Opcode::Ret: {
            auto ret = dynamic_cast<const MyIR::ReturnInst *>(inst);
            if (ret->has_return_value()) {
                auto val = ret->get_operand(0);
                if (auto cint = dynamic_cast<const MyIR::ConstantInt *>(val)) {
                    ss << "  li a0, " << cint->get_value() << "\n";
                } else if (auto cfloat = dynamic_cast<const MyIR::ConstantFloat *>(val)) {
                    std::string temp = allocTempReg(false);
                    std::string label = getFloatConstLabel(cfloat->get_value());
                    ss << "  la " << temp << ", " << label << "\n";
                    ss << "  flw fa0, " << "0(" << temp << ")" << '\n';
                    freeReg(temp);
                } 
                else if (val->get_type()->get_type_id() == MyIR::TypeID::Float) {
                    std::string val_reg = getReg(val);
                    ss << "  fmv.s fa0, " << val_reg << "\n";
                    freeReg(val_reg);
                } else {
                    std::string val_reg = getReg(val);
                    ss << "  mv a0, " << val_reg << "\n";
                    freeReg(val_reg);
                }
            }
            // 不再生成 epilogue指令，只生成跳转到函数结尾
            ss << "  j _" << ret->get_parent()->get_parent()->get_name() << "_exit\n";
            break;
        }
        // 分支指令修正
        case MyIR::Opcode::Br: {
            auto br = dynamic_cast<const MyIR::BranchInst *>(inst);
            if (br->is_conditional()) {
                std::string cond = getReg(br->get_operand(0));
                ss << "  bnez " << cond << ", " << getLabel(br->get_true_dest()) << "\n";
                ss << "  j " << getLabel(br->get_false_dest()) << "\n";
                freeReg(cond);
            } else {
                ss << "  j " << getLabel(br->get_uncond_dest()) << "\n";
            }
            break;
        }

        // 比较指令修正
        case MyIR::Opcode::ICmp: {
            auto cmp = dynamic_cast<const MyIR::CmpInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2;

            // 处理第一个操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(cmp->get_operand(0))) {
                rs1 = allocTempReg(false);
                ss << "  li " << rs1 << ", " << const_int->get_value() << "\n";
            } else {
                rs1 = getReg(cmp->get_operand(0));
            }

            // 根据比较类型生成对应的指令
            switch (cmp->get_predicate()) {
                case MyIR::CmpPredicate::EQ:
                    // 检查第二个操作数是否为立即数
                    if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(cmp->get_operand(1))) {
                        int value = const_int->get_value();
                        if (value >= -2048 && value <= 2047) {
                            // 可以直接使用xori
                            ss << "  xori " << rd << ", " << rs1 << ", " << value << "\n";
                        } else {
                            // 值太大，需要先加载到寄存器
                            rs2 = allocTempReg(false);
                            ss << "  li " << rs2 << ", " << value << "\n";
                            ss << "  xor " << rd << ", " << rs1 << ", " << rs2 << "\n";
                            freeReg(rs2);
                        }
                    } else {
                        rs2 = getReg(cmp->get_operand(1));
                        ss << "  xor " << rd << ", " << rs1 << ", " << rs2 << "\n";
                        freeReg(rs2);
                    }
                    ss << "  seqz " << rd << ", " << rd << "\n";
                    break;
                case MyIR::CmpPredicate::NE:
                    // 检查第二个操作数是否为立即数
                    if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(cmp->get_operand(1))) {
                        int value = const_int->get_value();
                        if (value >= -2048 && value <= 2047) {
                            // 可以直接使用xori
                            ss << "  xori " << rd << ", " << rs1 << ", " << value << "\n";
                        } else {
                            // 值太大，需要先加载到寄存器
                            rs2 = allocTempReg(false);
                            ss << "  li " << rs2 << ", " << value << "\n";
                            ss << "  xor " << rd << ", " << rs1 << ", " << rs2 << "\n";
                            freeReg(rs2);
                        }
                    } else {
                        rs2 = getReg(cmp->get_operand(1));
                        ss << "  xor " << rd << ", " << rs1 << ", " << rs2 << "\n";
                        freeReg(rs2);
                    }
                    ss << "  snez " << rd << ", " << rd << "\n";
                    break;
                case MyIR::CmpPredicate::SLT: {
                    // 检查第二个操作数是否为立即数
                    if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(cmp->get_operand(1))) {
                        int value = const_int->get_value();
                        if (value >= -2048 && value <= 2047) {
                            // 可以直接使用slti
                            ss << "  slti " << rd << ", " << rs1 << ", " << value << "\n";
                        } else {
                            // 值太大，需要先加载到寄存器
                            rs2 = allocTempReg(false);
                            ss << "  li " << rs2 << ", " << value << "\n";
                            ss << "  slt " << rd << ", " << rs1 << ", " << rs2 << "\n";
                            freeReg(rs2);
                        }
                    } else {
                        rs2 = getReg(cmp->get_operand(1));
                        ss << "  slt " << rd << ", " << rs1 << ", " << rs2 << "\n";
                        freeReg(rs2);
                    }

                    break;
                }
                case MyIR::CmpPredicate::SLE:
                    // 小于等于可以通过对大于取反实现
                    if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(cmp->get_operand(1))) {
                        int value = const_int->get_value();
                        if (value >= -2048 && value <= 2047) {
                            // 使用slti实现: a <= b 等价于 a < b + 1
                            ss << "  slti " << rd << ", " << rs1 << ", " << value + 1 << "\n";
                        } else {
                            // 值太大，需要先加载到寄存器
                            rs2 = allocTempReg(false);
                            ss << "  li " << rs2 << ", " << value << "\n";
                            ss << "  slt " << rd << ", " << rs2 << ", " << rs1 << "\n";
                            ss << "  xori " << rd << ", " << rd << ", 1\n";
                            freeReg(rs2);
                        }
                    } else {
                        rs2 = getReg(cmp->get_operand(1));
                        ss << "  slt " << rd << ", " << rs2 << ", " << rs1 << "\n";
                        ss << "  xori " << rd << ", " << rd << ", 1\n";
                        freeReg(rs2);
                    }
                    break;
                case MyIR::CmpPredicate::SGT:
                    // 大于可以通过交换操作数使用slti实现
                    if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(cmp->get_operand(1))) {
                        int value = const_int->get_value();
                        if (value >= -2048 && value <= 2047) {
                            // a > b 等价于 !(a < b + 1)
                            ss << "  slti " << rd << ", " << rs1 << ", " << value + 1 << "\n";
                            ss << "  xori " << rd << ", " << rd << ", 1\n";
                        } else {
                            // 值太大，需要先加载到寄存器
                            rs2 = allocTempReg(false);
                            ss << "  li " << rs2 << ", " << value << "\n";
                            ss << "  slt " << rd << ", " << rs2 << ", " << rs1 << "\n";
                            freeReg(rs2);
                        }
                    } else {
                        rs2 = getReg(cmp->get_operand(1));
                        ss << "  slt " << rd << ", " << rs2 << ", " << rs1 << "\n";
                        freeReg(rs2);
                    }
                    break;
                case MyIR::CmpPredicate::SGE: {
                    // 大于等于可以用小于取反实现
                    if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(cmp->get_operand(1))) {
                        int value = const_int->get_value();
                        if (value >= -2048 && value <= 2047) {
                            ss << "  slti " << rd << ", " << rs1 << ", " << value << "\n";
                            ss << "  xori " << rd << ", " << rd << ", 1\n";
                        } else {
                            rs2 = allocTempReg(false);
                            ss << "  li " << rs2 << ", " << value << "\n";
                            ss << "  slt " << rd << ", " << rs1 << ", " << rs2 << "\n";
                            ss << "  xori " << rd << ", " << rd << ", 1\n";
                            freeReg(rs2);
                        }
                    } else {
                        rs2 = getReg(cmp->get_operand(1));
                        ss << "  slt " << rd << ", " << rs1 << ", " << rs2 << "\n";
                        ss << "  xori " << rd << ", " << rd << ", 1\n";
                        freeReg(rs2);
                    }
                    break;
                }
                default:
                    break;
            }
            // 释放临时寄存器
            // if (dynamic_cast<const MyIR::ConstantInt*>(cmp->get_operand(0))) {
                freeReg(rs1);
            // }
            break;
        }

        // 修改Call指令的处理
        case MyIR::Opcode::Call: {
            auto call = dynamic_cast<const MyIR::CallInst *>(inst);
            
            // 保存当前活跃的临时寄存器
            saveActiveTempRegs(ss);
            
            if (call->get_operand(0)->get_name() == "llvm.memset.p0i8.i64") {

                auto dest = call->get_operand(1);
                auto value = call->get_operand(2);
                auto size = call->get_operand(3);
                
                std::string dest_reg = getReg(dest);
                // 处理目标地址参数
                if (array_base_.count(dest)) {
                    // 如果是数组，计算栈上的基地址
                    // int base_offset = array_base_[dest];
                    // ss << "  addi a0, sp, " << base_offset << "\n";
                    ss << "  mv a0, " << dest_reg << "\n";
                } else {
                    // 其他情况直接移动地址
                    ss << "  mv a0, " << dest_reg << "\n";
                }
                freeReg(dest_reg);
                
                // 处理填充值参数
                if (auto const_val = dynamic_cast<const MyIR::ConstantInt*>(value)) {
                    ss << "  li a1, " << const_val->get_value() << "\n";
                } else {
                    std::string val_reg = getReg(value); 
                    ss << "  mv a1, " << val_reg << "\n";
                    freeReg(val_reg);
                }
                
                // 处理大小参数
                if (auto const_size = dynamic_cast<const MyIR::ConstantInt*>(size)) {
                    ss << "  li a2, " << const_size->get_value() << "\n";
                } else {
                    std::string size_reg = getReg(size); 
                    ss << "  mv a2, " << size_reg << "\n";
                    freeReg(size_reg);
                }
                
                // 调用 memset
                ss << "  call memset\n";
                // 恢复保存的临时寄存器
                restoreActiveTempRegs(ss);
            } else {

                // analyzeCallInstOperands(call);

                // 处理其他函数调用
                size_t int_idx = 0, float_idx = 0;
                std::vector<const MyIR::Value*> stack_args;
                
                // 处理参数
                for (size_t i = 1; i < call->get_num_operands(); ++i) {
                    auto arg = call->get_operand(i);
                    
                    // 检查参数是否为常数
                    if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(arg)) {
                        // int_arg_idx[arg] = int_idx;
                        // 常数直接加载到参数寄存器
                        if (int_idx < 8) {
                            ss << "  li a" << int_idx << ", " << const_int->get_value() << "\n";
                        } else {
                            // 超过8个参数，需要存入栈
                            // std::string temp = allocTempReg();
                            // ss << "  li " << temp << ", " << const_int->get_value() << "\n";
                            stack_args.push_back(arg);
                            // freeReg(temp);
                        }
                        ++int_idx;
                    } else if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(arg)) {
                        // float_arg_idx[arg] = float_idx;
                        // 浮点常数处理
                        if (float_idx < 8) {
                            std::string temp = allocTempReg(false);
                            std::string label = getFloatConstLabel(const_float->get_value());
                            ss << "  la " << temp << ", " << label << "\n";
                            ss << "  flw fa" << float_idx << ", 0(" << temp << ")\n";
                            freeReg(temp);
                        } else {
                            // std::string temp = allocTempReg(true);
                            // ss << "  li.s " << temp << ", " << const_float->get_value() << "\n";
                            stack_args.push_back(arg);
                            // freeReg(temp);
                        }
                        ++float_idx;
                    } else {
                        // 非常数参数的处理
                        std::string arg_reg = getReg(arg);
                        if (arg->get_type()->get_type_id() == MyIR::TypeID::Float) {
                            // float_arg_idx[arg] = float_idx;
                            if (float_idx < 8) {
                                ss << "  fmv.s fa" << float_idx << ", " << arg_reg << "\n";
                            } else {
                                stack_args.push_back(arg);
                            }
                            ++float_idx;
                        } else {
                            // int_arg_idx[arg] = int_idx;
                            if (int_idx < 8) {
                                ss << "  mv a" << int_idx << ", " << arg_reg << "\n";
                            } else {
                                stack_args.push_back(arg);
                            }
                            ++int_idx;
                        }
                        freeReg(arg_reg);
                    }
                }
                
                // 处理栈上参数
                if (!stack_args.empty()) {
                    int sz = stack_args.size() * 8;
                    while (sz > 2048) {
                        ss << "  addi sp, sp, -2048\n";
                        sz -= 2048;
                    }
                    ss << "  addi sp, sp, -" << sz << "\n";
                }
                int offset = 0;
                for (auto it = stack_args.begin(); it != stack_args.end(); ++it) {
                    auto arg = *it; 
                    // arg_offset[arg] = offset;
                    // 检查参数是否为常数
                    std::string temp;
                    if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(arg)) {
                        // 常数直接加载到参数寄存器
                        temp = allocTempReg(false);
                        ss << "  li " << temp << ", " << const_int->get_value() << "\n";
                        if (offset >= 2048) {
                            std::string reg = allocTempReg(false);
                            ss << "  li " << reg << ", " << offset << '\n';
                            ss << "  add " << reg << ", sp, " << reg << '\n';
                            ss << "  sw " << temp << ", 0(" << reg << ")\n";
                            freeReg(reg); 
                        } else ss << "  sw " << temp << ", " << offset << "(sp)\n";
                        freeReg(temp);                        
                    } else if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(arg)) {
                        // 浮点常数处理
                        temp = allocTempReg(true);
                        std::string itmp = allocTempReg(false);
                        std::string label = getFloatConstLabel(const_float->get_value());
                        ss << "  la " << itmp << ", " << label << '\n';
                        ss << "  flw" << temp << ", " << "0(" << itmp << ")\n";
                        if (offset >= 2048) {
                            std::string reg = allocTempReg(false);
                            ss << "  li " << reg << ", " << offset << '\n';
                            ss << "  add " << reg << ", sp, " << reg << '\n';
                            ss << "  fsw " << temp << ", 0(" << reg << ")\n";
                            freeReg(reg); 
                        } else ss << "  fsw " << temp << ", " << offset << "(sp)\n";
                        freeReg(itmp);
                        freeReg(temp);
                    } else {
                        // 非常数参数的处理
                        std::string arg_reg = getReg(arg);
                        if (arg->get_type()->get_type_id() == MyIR::TypeID::Float) {
                            if (offset >= 2048) {
                                std::string reg = allocTempReg(false);
                                ss << "  li " << reg << ", " << offset << '\n';
                                ss << "  add " << reg << ", sp, " << reg << '\n';
                                ss << "  fsw " << arg_reg << ", 0(" << reg << ")\n";
                                freeReg(reg); 
                            } else ss << "  fsw " << arg_reg << ", " << offset << "(sp)\n";
                        } else if (arg->get_type()->get_type_id() == MyIR::TypeID::Pointer) {
                            if (offset >= 2048) {
                                std::string reg = allocTempReg(false);
                                ss << "  li " << reg << ", " << offset << '\n';
                                ss << "  add " << reg << ", sp, " << reg << '\n';
                                ss << "  sd " << arg_reg << ", 0(" << reg << ")\n";
                                freeReg(reg); 
                            } else ss << "  sd " << arg_reg << ", " << offset << "(sp)\n";
                        } else {
                            if (offset >= 2048) {
                                std::string reg = allocTempReg(false);
                                ss << "  li " << reg << ", " << offset << '\n';
                                ss << "  add " << reg << ", sp, " << reg << '\n';
                                ss << "  sw " << arg_reg << ", 0(" << reg << ")\n";
                                freeReg(reg); 
                            } else ss << "  sw " << arg_reg << ", " << offset << "(sp)\n";
                        }
                        freeReg(arg_reg);
                    }
                    offset += 8;
                }
                
                // 调用函数
                ss << "  call " << call->get_operand(0)->get_name() << "\n";
                
                // 恢复栈指针
                if (!stack_args.empty()) {
                    int sz = stack_args.size() * 8;
                    while (sz >= 2048) {
                        ss << "  addi sp, sp, 2047\n";
                        sz -= 2047;
                    }
                    ss << "  addi sp, sp, " << sz << "\n";
                }
                
                // 恢复保存的临时寄存器
                restoreActiveTempRegs(ss);
                
                // 处理返回值
                if (call->get_type()->get_type_id() != MyIR::TypeID::Void) {
                    std::string rd = getReg(inst);
                    if (call->get_type()->get_type_id() == MyIR::TypeID::Float) {
                        ss << "  fmv.s " << rd << ", fa0\n";
                    } else {
                        ss << "  mv " << rd << ", a0\n";
                    }
                }
            }
            break;
        }

        // 浮点算术指令处理
        case MyIR::Opcode::FAdd: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2, ftmp;
            
            // 处理第一个操作数
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(bin->get_operand(0))) {
                ftmp = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << ftmp << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                ftmp = getReg(bin->get_operand(0));
            }
            rs1 = allocTempReg(true);
            ss << "  fcvt.d.s " << rs1 << ", " << ftmp << '\n';
            freeReg(ftmp);
            
            // 处理第二个操作数
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(bin->get_operand(1))) {
                ftmp = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << ftmp << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                ftmp = getReg(bin->get_operand(1));
            }
            rs2 = allocTempReg(true);
            ss << "  fcvt.d.s " << rs2 << ", " << ftmp << '\n';
            freeReg(ftmp);
            
            ftmp = allocTempReg(true);
            ss << "  fadd.d " << ftmp << ", " << rs1 << ", " << rs2 << "\n";
            ss << "  fcvt.s.d " << rd << ", " << ftmp << '\n';
            freeReg(ftmp);
            freeReg(rs1);
            freeReg(rs2);
            break;
        }

        case MyIR::Opcode::FSub: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2, ftmp;
            
            // 处理第一个操作数
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(bin->get_operand(0))) {
                ftmp = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << ftmp << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                ftmp = getReg(bin->get_operand(0));
            }
            rs1 = allocTempReg(true);
            ss << "  fcvt.d.s " << rs1 << ", " << ftmp << '\n';
            freeReg(ftmp);
            
            // 处理第二个操作数
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(bin->get_operand(1))) {
                ftmp = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << ftmp << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                ftmp = getReg(bin->get_operand(1));
            }
            rs2 = allocTempReg(true);
            ss << "  fcvt.d.s " << rs2 << ", " << ftmp << '\n';
            freeReg(ftmp);
            
            ftmp = allocTempReg(true);
            ss << "  fsub.d " << ftmp << ", " << rs1 << ", " << rs2 << "\n";
            ss << "  fcvt.s.d " << rd << ", " << ftmp << '\n';
            freeReg(ftmp);
            freeReg(rs1);
            freeReg(rs2);
            break;
        }

        case MyIR::Opcode::FMul: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2, ftmp;
            
            // 处理第一个操作数
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(bin->get_operand(0))) {
                ftmp = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << ftmp << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                ftmp = getReg(bin->get_operand(0));
            }
            rs1 = allocTempReg(true);
            ss << "  fcvt.d.s " << rs1 << ", " << ftmp << '\n';
            freeReg(ftmp);
            
            // 处理第二个操作数
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(bin->get_operand(1))) {
                ftmp = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << ftmp << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                ftmp = getReg(bin->get_operand(1));
            }
            rs2 = allocTempReg(true);
            ss << "  fcvt.d.s " << rs2 << ", " << ftmp << '\n';
            freeReg(ftmp);
            
            ftmp = allocTempReg(true);
            ss << "  fmul.d " << ftmp << ", " << rs1 << ", " << rs2 << "\n";
            ss << "  fcvt.s.d " << rd << ", " << ftmp << '\n';
            freeReg(ftmp);
            freeReg(rs1);
            freeReg(rs2);
            break;
        }

        case MyIR::Opcode::FDiv: {
            auto bin = dynamic_cast<const MyIR::BinaryInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2, ftmp;
            
            // 处理第一个操作数
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(bin->get_operand(0))) {
                ftmp = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << ftmp << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                ftmp = getReg(bin->get_operand(0));
            }
            rs1 = allocTempReg(true);
            ss << "  fcvt.d.s " << rs1 << ", " << ftmp << '\n';
            freeReg(ftmp);
            
            // 处理第二个操作数
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(bin->get_operand(1))) {
                ftmp = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << ftmp << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                ftmp = getReg(bin->get_operand(1));
            }
            rs2 = allocTempReg(true);
            ss << "  fcvt.d.s " << rs2 << ", " << ftmp << '\n';
            freeReg(ftmp);
            
            ftmp = allocTempReg(true);
            ss << "  fdiv.d " << ftmp << ", " << rs1 << ", " << rs2 << "\n";
            ss << "  fcvt.s.d " << rd << ", " << ftmp << '\n';
            freeReg(ftmp);
            freeReg(rs1);
            freeReg(rs2);
            break;
        }
        // 在 genInstruction 中处理 GetElementPtr 指令
        case MyIR::Opcode::GEP: {
            auto gep = dynamic_cast<const MyIR::GetElementPtrInst*>(inst);
            // 替换原有的数组访问逻辑，直接调用专门的数组访问函数
            genArrayAccess(gep, ss);
            break;
        }
        case MyIR::Opcode::Bitcast: {
            auto cast = dynamic_cast<const MyIR::CastInst *>(inst);
            std::string rd = getReg(inst);
            auto operand = cast->get_operand(0);
            
            // 检查操作数是否为数组
            if (array_base_.count(operand)) {
                // 数组情况：先加载数组基地址到目标寄存器
                int base_offset = array_base_[operand];
                if (base_offset >= 2048) {
                    std::string tmp = allocTempReg(false);
                    ss << "  li " << tmp << ", " << base_offset << '\n';
                    ss << "  add " << rd << ", sp, " << tmp << '\n';
                } else ss << "  addi " << rd << ", sp, " << base_offset << "\n";
            } else {
                // 非数组情况：直接移动寄存器值
                std::string rs = getReg(operand);
                if (inst->get_type()->get_type_id() == MyIR::TypeID::Float) {
                    ss << "  fmv.s " << rd << ", " << rs << "\n";
                } else {
                    ss << "  mv " << rd << ", " << rs << "\n";
                }
                freeReg(rs);
            }
            break;
        }
        case MyIR::Opcode::ZExt: {
            auto cast = dynamic_cast<const MyIR::CastInst *>(inst);
            std::string rd = getReg(inst);
            auto operand = cast->get_operand(0);
            std::string rs;
            
            // 处理源操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(operand)) {
                rs = allocTempReg(false);
                ss << "  li " << rs << ", " << const_int->get_value() << "\n";
            } else {
                rs = getReg(operand);
            }
            
            // 对于布尔值到整数的扩展，直接复制即可
            // RISC-V中，条件指令已经生成了0或1的结果，无需额外处理
            ss << "  mv " << rd << ", " << rs << "\n";
            // 释放临时寄存器
            // if (dynamic_cast<const MyIR::ConstantInt*>(operand)) {
                freeReg(rs);
            // }
            break;
        }
        case MyIR::Opcode::SIToFP: {
            auto cast = dynamic_cast<const MyIR::CastInst *>(inst);
            std::string rd = getReg(inst);
            auto operand = cast->get_operand(0);
            std::string rs;
            
            // 处理源操作数
            if (auto const_int = dynamic_cast<const MyIR::ConstantInt*>(operand)) {
                // 如果是常量整数，先加载到整数寄存器
                rs = allocTempReg(false);
                ss << "  li " << rs << ", " << const_int->get_value() << "\n";
            } else {
                // 否则获取源操作数的寄存器
                rs = getReg(operand);
            }
            
            // 使用RISC-V的fcvt指令进行转换
            // fcvt.s.w - Convert 32-bit Integer to Single-Precision Float
            // fcvt.d.w - Convert 32-bit Integer to Double-Precision Float
            if (inst->get_type()->get_type_id() == MyIR::TypeID::Float) {
                ss << "  fcvt.s.w " << rd << ", " << rs << "\n";
            }
            
            // 释放源操作数寄存器
            freeReg(rs);
            break;
        }
        case MyIR::Opcode::FPToSI: {
            auto cast = dynamic_cast<const MyIR::CastInst *>(inst);
            std::string rd = getReg(inst);
            auto operand = cast->get_operand(0);
            std::string rs;
            
            // 处理源操作数
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(operand)) {
                // 如果是常量浮点数，先加载到浮点寄存器
                rs = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << rs << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                // 否则获取源操作数的寄存器
                rs = getReg(operand);
            }
            
            // 使用RISC-V的fcvt指令进行截断式转换
            // fcvt.w.s rd, rs, rtz  - Convert Single-Precision Float to 32-bit Integer with Round to Zero
            ss << "  fcvt.w.s " << rd << ", " << rs << ", rtz\n";
            
            // 释放源操作数寄存器
            freeReg(rs);
            break;
        }
        case MyIR::Opcode::FCmp: {
            auto cmp = dynamic_cast<const MyIR::CmpInst *>(inst);
            std::string rd = getReg(inst);
            std::string rs1, rs2, ftmp;

            // Handle first operand
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(cmp->get_operand(0))) {
                rs1 = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << rs1 << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                rs1 = getReg(cmp->get_operand(0));
            }
            // rs1 = allocTempReg(true);
            // ss << "  fcvt.d.s " << rs1 << ", " << ftmp << '\n';
            // freeReg(ftmp);

            // Handle second operand
            if (auto const_float = dynamic_cast<const MyIR::ConstantFloat*>(cmp->get_operand(1))) {
                rs2 = allocTempReg(true);
                std::string temp = allocTempReg(false);
                std::string label = getFloatConstLabel(const_float->get_value());
                ss << "  la " << temp << ", " << label << "\n";
                ss << "  flw " << rs2 << ", 0(" << temp << ")\n";
                freeReg(temp);
            } else {
                rs2 = getReg(cmp->get_operand(1));
            }
            // rs2 = allocTempReg(true);
            // ss << "  fcvt.d.s " << rs2 << ", " << ftmp << '\n';
            // freeReg(ftmp);
            
            // ftmp = allocTempReg(true);
            // Generate comparison based on predicate using double precision
            switch (cmp->get_predicate()) {
                case MyIR::CmpPredicate::OEQ:  // Ordered and equal
                    ss << "  feq.s " << rd << ", " << rs1 << ", " << rs2 << "\n";
                    break;

                case MyIR::CmpPredicate::ONE:  // Ordered and not equal
                    ss << "  feq.s " << rd << ", " << rs1 << ", " << rs2 << "\n";
                    ss << "  xori " << rd << ", " << rd << ", 1\n";
                    break;

                case MyIR::CmpPredicate::OGT:  // Ordered and greater than
                    ss << "  flt.s " << rd << ", " << rs2 << ", " << rs1 << "\n";
                    break;

                case MyIR::CmpPredicate::OGE:  // Ordered and greater than or equal
                    ss << "  fle.s " << rd << ", " << rs1 << ", " << rs2 << "\n";
                    ss << "  xori " << rd << ", " << rd << ", 1\n";
                    break;

                case MyIR::CmpPredicate::OLT:  // Ordered and less than
                    ss << "  flt.s " << rd << ", " << rs1 << ", " << rs2 << "\n";
                    break;

                case MyIR::CmpPredicate::OLE:  // Ordered and less than or equal
                    ss << "  fle.s " << rd << ", " << rs1 << ", " << rs2 << "\n";
                    break;

                default:
                    break;
            }
            // ss << "  fcvt.s.d " << rd << ", " << ftmp << '\n';
            // Free registers
            // freeReg(ftmp);
            freeReg(rs1);
            freeReg(rs2);
            break;
        }
        // 其他指令类型可继续扩展
        default:
            break;
    }
}

void AsmGenerator::initRegisterPool() {

    // 初始化整数临时寄存器池
    for (int i = 11; i >= 0; i--) 
        avail_temp_regs_.push_back("s" + std::to_string(i));
    for (int i = 6; i >= 0; i--) {
        avail_temp_regs_.push_back("t" + std::to_string(i));
    }
    // 初始化浮点临时寄存器池
    for (int i = 11; i >= 0; i--) {
        avail_float_regs_.push_back("ft" + std::to_string(i));
    }
}

std::string AsmGenerator::allocTempReg(bool is_float) {
    std::vector<std::string>& pool = is_float ? avail_float_regs_ : avail_temp_regs_;
    
    // 如果还有可用寄存器，直接分配
    if (!pool.empty()) {
        std::string reg = pool.back();
        pool.pop_back();
        used_regs_.insert(reg);
        return reg;
    }
    
    // 没有可用寄存器，需要溢出
    return spillRegister(is_float);
}

std::string AsmGenerator::spillRegister(bool is_float) {
    // std::stringstream &ss = ir_stream_;
    
    // 选择一个要溢出的寄存器
    std::string spill_reg;
    const MyIR::Value* spill_value = nullptr;
    
    for (const auto& pair : reg2value_) {
        std::string reg = pair.first;
        if ((is_float && reg[0] == 'f') || (!is_float && reg[0] == 't')) {
            spill_reg = reg;
            spill_value = pair.second;
            break;
        }
    }
    
    // 将寄存器值存入栈
    if (spill_value != nullptr) {
        int offset = stack_size_;
        stack_size_ += 8;
        ss << "  " << (is_float ? "fsw " : "sw ") << spill_reg << ", " 
           << offset << "(sp)\n";
        
        // 更新映射
        value2reg_.erase(spill_value);
        reg2value_.erase(spill_reg);
        spill_locations_[spill_value] = offset;
    }
    
    return spill_reg;
}

std::string AsmGenerator::getReg(const MyIR::Value* val) {
    // 检查是否已分配寄存器
    auto it = value2reg_.find(val);
    if (it != value2reg_.end()) {
        return it->second;
    }
    
    // 检查是否在栈上    
    auto spill_it = spill_locations_.find(val);
    if (spill_it != spill_locations_.end()) {
        bool is_float = val->get_type()->get_type_id() == MyIR::TypeID::Float;
        std::string reg = allocTempReg(is_float);
        ss << "  " << (is_float ? "flw " : "lw ") << reg << ", " 
                  << spill_it->second << "(sp)\n";
        
        // 更新映射
        value2reg_[val] = reg;
        reg2value_[reg] = val;
        return reg;
    }
    
    // 分配新寄存器
    bool is_float = val->get_type()->get_type_id() == MyIR::TypeID::Float;
    std::string reg = allocTempReg(is_float);
    value2reg_[val] = reg;
    reg2value_[reg] = val;
    return reg;
}

void AsmGenerator::freeReg(const std::string& reg) {
    if (used_regs_.count(reg)) {
        used_regs_.erase(reg);
        if (reg[0] == 'f') {
            avail_float_regs_.push_back(reg);
        } else {
            avail_temp_regs_.push_back(reg);
        }
    }
}   

// 建立活跃临时寄存器与栈帧偏移的映射
std::map<std::string, int> reg2offset;

// 在Call指令处理之前保存活跃的临时寄存器
void AsmGenerator::saveActiveTempRegs(std::stringstream &ss) {
    if (used_regs_.empty()) {
        caller_save_offset_ = 0;
        return;
    }
    
    // 为所有需要保存的寄存器分配栈空间
    int total_size = used_regs_.size() * 8;
    ss << "  addi sp, sp, -" << total_size << "\n";
    
    // 保存每个活跃的临时寄存器
    int offset = 0;
    for (const auto& reg : used_regs_) {
        // 获取寄存器对应的值
        auto value = reg2value_[reg];
        bool is_pointer = value && value->get_type()->get_type_id() == MyIR::TypeID::Pointer;
        bool is_float = value && value->get_type()->get_type_id() == MyIR::TypeID::Float;

        // 根据值类型选择正确的存储指令
        if (is_pointer) {
            ss << "  sd " << reg << ", " << offset << "(sp)\n";
        } else if (is_float) {
            ss << "  fsw " << reg << ", " << offset << "(sp)\n";
        } else {
            ss << "  sw " << reg << ", " << offset << "(sp)\n";
        }
        reg2offset[reg] = offset;
        offset += 8;
    }
    
    caller_save_offset_ = total_size;
}

void AsmGenerator::restoreActiveTempRegs(std::stringstream &ss) {
    if (used_regs_.empty()) {
        if (caller_save_offset_) {
            ss << "  addi sp, sp, " << caller_save_offset_ << "\n";
            caller_save_offset_ = 0;
        }
        return;
    }
    
    // 恢复每个临时寄存器
    for (const auto& reg : used_regs_) {
        // 获取寄存器对应的值
        auto value = reg2value_[reg];
        bool is_pointer = value && value->get_type()->get_type_id() == MyIR::TypeID::Pointer;
        bool is_float = value && value->get_type()->get_type_id() == MyIR::TypeID::Float;

        // 根据值类型选择正确的加载指令
        if (is_pointer) {
            ss << "  ld " << reg << ", " << reg2offset[reg] << "(sp)\n";
        } else if (is_float) {
            ss << "  flw " << reg << ", " << reg2offset[reg] << "(sp)\n";
        } else {
            ss << "  lw " << reg << ", " << reg2offset[reg] << "(sp)\n";
        }
    }
    
    // 恢复栈指针
    ss << "  addi sp, sp, " << caller_save_offset_ << "\n";
    caller_save_offset_ = 0;
}


std::vector<size_t> AsmGenerator::getArrayDimensions(const MyIR::GetElementPtrInst* gep) {
    std::vector<size_t> dimensions;
    // dimensions.push_back(0);
    // 获取基础指针的类型
    auto ptr = gep->get_operand(0);
    auto current_type = ptr->get_type();
    
    // 如果是指针类型，获取其元素类型
    if (auto ptr_type = std::dynamic_pointer_cast<MyIR::PointerType>(current_type)) {
        current_type = ptr_type->get_element_type();
    }
    
    // 递归遍历所有数组类型，收集维度信息
    while (auto array_type = std::dynamic_pointer_cast<MyIR::ArrayType>(current_type)) {
        dimensions.push_back(array_type->get_num_elements());
        current_type = array_type->get_element_type();
    }
    
    return dimensions;
}

void AsmGenerator::genArrayAccess(const MyIR::GetElementPtrInst* gep, std::stringstream& ss) {
    std::string rd = getReg(gep);
    auto base = gep->get_operand(0);
    
    // Get array element type
    auto ptr_type = std::dynamic_pointer_cast<MyIR::PointerType>(base->get_type());
    auto array_type = std::dynamic_pointer_cast<MyIR::ArrayType>(ptr_type->get_element_type());
    // bool is_float_array = array_type->get_element_type()->get_type_id() == MyIR::TypeID::Float;
    // ss << "ok\n";
    // return;
    // 基地址计算
    if (auto global = dynamic_cast<const MyIR::GlobalVariable*>(base)) {
        ss << "  la " << rd << ", " << global->get_name() << "\n";
    } else if (array_base_.count(base)) {
        int base_offset = array_base_[base] + arg_stack_size + caller_save_offset_;
        if (base_offset >= 2048) {
            std::string tmp = allocTempReg(false);
            ss << "  li " << tmp << ", " << base_offset << "\n";
            ss << "  add " << rd << ", sp, " << tmp << "\n";
            freeReg(tmp);
        } else {
            ss << "  addi " << rd << ", sp, " << base_offset << "\n";
        }
    } else {
        std::string base_reg = getReg(base);
        ss << "  mv " << rd << ", " << base_reg << "\n";
        freeReg(base_reg);
    }

    // 获取数组维度和索引
    auto dimensions = getArrayDimensions(gep);
    auto indices = gep->get_indices();
    
    // for (size_t dim : dimensions) ss << dim << ' ';
    // ss << '\n';
    // return;
    // 计算步长
    int stride = 4;  // 基础类型大小
    for (size_t i = 0; i < dimensions.size(); i++) {
        stride *= (int)dimensions[i];
    }

    // 处理每个维度的索引
    for (size_t i = 0; i < indices.size(); ++i) {
        auto idx = indices[i];
        
        // 计算偏移量
        if (auto const_idx = dynamic_cast<const MyIR::ConstantInt*>(idx)) {
            int offset = const_idx->get_value() * stride;
            if (offset >= 2048) {
                std::string tmp = allocTempReg(false);
                ss << "  li " << tmp << ", " << offset << "\n";
                ss << "  add " << rd << ", " << rd << ", " << tmp << "\n";
                freeReg(tmp);
            } else {
                ss << "  addi " << rd << ", " << rd << ", " << offset << "\n";
            }
        } else {
            std::string idx_reg = getReg(idx);
            std::string tmp = allocTempReg(false);
            ss << "  li " << tmp << ", " << stride << "\n";
            ss << "  mul " << tmp << ", " << idx_reg << ", " << tmp << "\n";
            ss << "  add " << rd << ", " << rd << ", " << tmp << "\n";
            freeReg(idx_reg);
            freeReg(tmp);
        }
        // 更新步长
        if (i < dimensions.size()) stride /= (int)dimensions[i];
    }
}