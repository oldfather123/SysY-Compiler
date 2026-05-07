#include <sstream>
#include <cstdint>
#include "riscv_generator.hpp"
#include "riscv_function.hpp"
#include "riscv_basic_block.hpp"
#include "riscv_instruction.hpp"
#include "register.hpp"
#include "reg_alloc.hpp"
#include "ir_type.hpp"
#include "ir_value.hpp"
#include "ir_instruction.hpp"
#include "ir_basic_block.hpp"
#include "ir_function.hpp"
#include "ir_module.hpp"

string RiscvConst::print() {
    if(rtid == RiscvTypeID::RIntImmTy) {
        return to_string(int_val);
    } else if(rtid == RiscvTypeID::RFloatImmTy) {
        return to_string(float_val);
    } else {
        cerr << "[ERROR] Invalid Riscv Constant Type!\n";
        exit(1);
    }
}

string RiscvIntReg::print() {
    if(reg) {
        if(reg->reg_type == RegType::IntReg) {
            return reg->alias;
        } else {
            cerr << "[ERROR] RiscvIntReg has non-integer register!\n";
            exit(1);
        }
    } else {
        cerr << "[ERROR] RiscvIntReg has no valid register!\n";
        exit(1);
    }
}

string RiscvFloatReg::print() {
    if(reg) {
        if(reg->reg_type == RegType::FloatReg) {
            return reg->alias;
        } else {
            cerr << "[ERROR] RiscvFloatReg has non-float register!\n";
            exit(1);
        }
    } else {
        cerr << "[ERROR] RiscvFloatReg has no valid register!\n";
        exit(1);
    }
}

string RiscvIntMem::print() {
    string result = "";
    if(base) { // 如果有寄存器，则直接使用寄存器别名
        result += "(" + base->alias + ")";
    } else {
        if(is_global) { // 全局变量直接返回其名
            return base_name;
        } else { // 否则间接寻址
            result += "(" + base_name + ")";
        }
    }
    if(offset) { // 拼接偏移量
        result = to_string(offset) + result;
    }
    return result;
}

string RiscvFloatMem::print() {
    string result = "";
    if(base) { // 如果有寄存器，则直接使用寄存器别名
        result += "(" + base->alias + ")";
    } else {
        if(is_global) { // 全局变量直接返回其名
            return base_name;
        } else { // 否则间接寻址
            result += "(" + base_name + ")";
        }
    }
    if(offset) { // 拼接偏移量
        result = to_string(offset) + result;
    }
    return result;
}

string RiscvGlobalVariable::print() {
    return print(true, nullptr);
}

string RiscvGlobalVariable::print(bool print_name, Const* init_val) {
    string result = "";
    // 如果在调用的第一层，初始化 init_val
    if(print_name) {
        result += this->name + ":\n";
        init_val = this->init_val;
    }
    if(init_val == nullptr)
        return "\t.zero\t" + to_string(this->element_num * 4) + "\n";;
    // 如果无初始值，或初始值为0（IR中有ConstZero类），则直接用zero命令
    if(dynamic_cast<ConstZero*>(init_val)) {
        result += "\t.zero\t" + to_string(calculate_type_size(init_val->type)) + "\n";
        return result;
    }
    // 下面是非零的处理
    // 整型
    if(init_val->type->tid == TypeID::IntegerTy) {
        result += "\t.word\t" + to_string(dynamic_cast<ConstInt*>(init_val)->val) + "\n";
        return result;
    } else if(init_val->type->tid == TypeID::FloatTy) { // 浮点
        stringstream ss;
        ss << "0x" << hex << static_cast<ConstFloat*>(init_val)->val;
        string val_str = ss.str();
        while(val_str.length() < 10) {
            val_str += "0";
        }
        result += "\t.word\t" + val_str.substr(0, 10) + "\n";
        return result;
    } else if(init_val->type->tid == TypeID::ArrayTy) { // 数组
        ConstArray* const_arr = dynamic_cast<ConstArray*>(init_val);
        if(const_arr == nullptr) {
            cerr << "[ERROR] RiscvGlobalVariable::print() init_value is not ConstArray." << endl;
            exit(1);
        }
        // PS: 由于初始化数组时，对未显式初始化的元素均使用 0 进行初始化，故此时不会存在类型大小与元素数量不匹配的情况，即不会在全局数组末尾产生 .zero <number> 的情况。
        for(auto elements: const_arr->const_array) {
            result += print(false, elements);
        }
        return result;  
    } else {
        cerr << "[ERROR] Unknown RiscvGlobalVariable::print() init_value type.\n";
        exit(1);
    }
}

// ADD  a0, a1, a2
string RiscvBinaryInst::print() {
    string result = "\t\t" + inst_to_str[riid];
    if(is_word && (riid == ADDI || riid == ADD || riid == MUL || riid == REM || riid == DIV)) {
        result += "W"; // 32 位指令
    }
    result += "\t";
    result += ret_val->print();
    result += ", ";
    result += operands[0]->print();
    result += ", ";
    result += operands[1]->print();
    result += "\n";
    // cout << result;
    return result;
}

string RiscvMoveInst::print() {
    string result = "\t\t";
    if (operands[1]->rtid == RiscvTypeID::RIntRegTy) {
        result += "MV\t";
    } else {
        result += "FMV.S\t";
    }
    result += operands[0]->print() + ", " + operands[1]->print() + "\n";
    // cout << result;
    return result;
}

string RiscvLiInst::print() {
    string result = "\t\tLI\t" + operands[0]->print() + ", " + operands[1]->print() + "\n";
    // cout << result;
    return result;
}

string RiscvPushInst::print() {
    cerr << "[Info] Here push print\n";
    string result = "";
    int offset = this->offset;
    for(auto op: operands) {
        offset -= VARIABLE_ALIGN_BYTE;
        result += "\t\tSD\t" + op->print() + ", " + to_string(offset) + "(sp)\n";
    }
    // cout << result;
    return result;
}

string RiscvPopInst::print() {
    cerr << "[Info] Here pop print\n";
    string result = "";
    int offset = this->offset;
    for(auto op: operands) {
        offset -= VARIABLE_ALIGN_BYTE;
        result += "\t\tLD\t" + op->print() + ", " + to_string(offset) + "(sp)\n";
    }
    result += "\t\tADDI\tsp, " + to_string(-offset) + "\n"; // 恢复栈指针
    // cout << result;
    return result;
}

string RiscvCallInst::print() {
    string result = "\t\tCALL\t" + static_cast<RiscvFunction*>(operands[0])->name + "\n";
    // cout << result;
    return result;
}

string RiscvReturnInst::print() {
    // cout << "\t\tRET\n";
    return "\t\tRET\n";
}

string RiscvLoadInst::print() {
    if(operands[0] == nullptr || operands[1] == nullptr) {
        // cout << "";
        return "";
    }
    string result = "\t\t";
    auto mem_addr = static_cast<RiscvIntMem*>(operands[1]);
    bool overflow = mem_addr->offset < -2048 || mem_addr->offset > 2047; // 检查立即数是否在 -2048 到 2047 范围内
    if(type->tid == TypeID::IntegerTy) {
        result += "LW\t";
    } else if(type->tid == TypeID::FloatTy) {
        result += "FLW\t";
    } else if(type->tid == TypeID::PointerTy) {
        result += "LD\t"; // 假设指针类型也是整型加载
    } else {
        cerr << "[Error] [RiscvLoadInst::print] Invalid load type: " << type->tid << "\n";
        exit(1);
    }
    result += operands[0]->print() + ", ";
    result += operands[1]->print();
    result += "\n";
    // cout << result;
    return result;
}

string RiscvStoreInst::print() {
    string result = "\t\t";
    if(type->tid == TypeID::IntegerTy) {
        result += "SW\t";
    } else if(type->tid == TypeID::FloatTy) {
        result += "FSW\t";
    } else if(type->tid == TypeID::PointerTy) {
        result += "SD\t"; // 假设指针类型也是整型存储
    } else {
        cerr << "[Error] [RiscvStoreInst::print] Invalid store type: " << type->tid << "\n";
        exit(1);
    }
    result += operands[0]->print() + ", ";
    result += operands[1]->print();
    result += "\n";
    // cout << result;
    return result;
}

string RiscvICmpInst::print() {
    string result = "\t\t";
    result += icmp_to_str[icmp_op] + "\t"; // 使用 RISC-V 指令名称
    result += operands[0]->print() + ", "; // 第一个操作数
    result += operands[1]->print() + ", "; // 第二个操作数
    result += static_cast<RiscvBasicBlock*>(operands[2])->name + "\n"; // 真分支基本块
    auto flase_bb = static_cast<RiscvBasicBlock*>(operands[3]);
    if(flase_bb) {
        result += "\t\tJ\t" + flase_bb->name + "\n"; // 如果有假分支基本块，添加跳转指令
    }
    // cout << result;
    return result;
}

string RiscvICmpSetInst::print() {
    string result = "\t\t";
    bool eq_or_ne = (icmp_op == ICmpOp::EQ || icmp_op == ICmpOp::NE);
    if(eq_or_ne) {
        result += "SUB\tt6, " + operands[0]->print() + ", " + operands[1]->print() + "\n\t\t"; // 使用 SUB 指令计算差值
    }
    result += icmp_set_to_str[icmp_op] + "\t"; // 使用 RISC-V 指令名称
    result += ret_val->print() + ", "; // 结果寄存器
    if(eq_or_ne) {
        result += "t6\n"; // 如果是 EQ 或 NE，使用 t6 寄存器
    } else {
        result += operands[0]->print() + ", " + operands[1]->print() + "\n"; // 其他情况直接使用操作数
    }
    // cout << result;
    return result;
}

string RiscvFCmpInst::print() {
    string result = "\t\t" + fcmp_to_str[fcmp_op] + "\t"; // 使用 RISC-V 浮点比较指令
    result += ret_val->print() + ", "; // 结果寄存器
    result += operands[0]->print() + ", "; // 第一个操作数
    result += operands[1]->print() + "\n"; // 第二个操作数
    // cout << result;
    return result;
}

string RiscvFpToSiInst::print() {
    string result = "\t\tFCVT.W.S\t"; // 使用 RISC-V 浮点到整型转换指令
    result += operands[1]->print() + ", "; // 目标寄存器
    result += operands[0]->print() + ", "; // 源寄存器
    result += "rtz\n";
    // cout << result;
    return result;
}

string RiscvSiToFpInst::print() {
    string result = "\t\tFCVT.S.W\t"; // 使用 RISC-V 整型到浮点转换指令
    result += operands[1]->print() + ", "; // 目标寄存器
    result += operands[0]->print() + "\n"; // 源寄存器
    // cout << result;
    return result;
}

string RiscvLoadAddrInst::print() {
    string result = "\t\tLA\t" + operands[0]->print() + ", " + name + "\n"; // 使用 LA 指令加载地址
    // cout << result;
    return result;
}

string RiscvBranchInst::print() {
    string result = "\t\t";
    if(operands[0]) {
        result += "BGTZ\t";
        result += operands[0]->print() + ", "; // 条件寄存器
        result += static_cast<RiscvBasicBlock*>(operands[1])->name + "\n\t\t"; // 真分支基本块
    }
    result += "J\t" + static_cast<RiscvBasicBlock*>(operands[2])->name + "\n"; // 跳转到假分支基本块
    // cout << result;
    return result;
}

string RiscvJmpInst::print() {
    string result = "\t\tJ\t" + static_cast<RiscvBasicBlock*>(operands[0])->name + "\n"; // 无条件跳转到目标基本块
    // cout << result;
    return result;
}

string RiscvBasicBlock::print() {
    string result = name + ":\n";
    for(auto inst: rinst_list) {
        result += inst->print();
    }
    return result;
}

string RiscvFunction::print() {
    string result = ".global " + this->name + "\n" + this->name + ":\n"; // 函数标号打印
    for(auto rbb: rbb_list) {
        result += rbb->print();
    }
    return result;
}

void RiscvGenerator::generate_data_section(ostream& out) {
    out << ".section .data\n";
    // 全局变量
    for(auto& global_var: m->global_var_list) {
        auto type = static_cast<PointerType*>(global_var->type)->pointed_type;
        RiscvGlobalVariable* rglobal_var = nullptr;
        switch(type->tid) {
            case TypeID::IntegerTy: {
                rglobal_var = new RiscvGlobalVariable(RiscvTypeID::RIntImmTy, global_var->name, global_var->is_const, global_var->init_val);
                break;
            }
            case TypeID::FloatTy: {
                rglobal_var = new RiscvGlobalVariable(RiscvTypeID::RFloatImmTy, global_var->name, global_var->is_const, global_var->init_val);
                break;
            }
            case TypeID::ArrayTy: {
                auto element_type = static_cast<ArrayType*>(type)->element_type;
                while(element_type->tid == TypeID::ArrayTy) {
                    element_type = static_cast<ArrayType*>(element_type)->element_type;
                }
                if(element_type->tid == TypeID::IntegerTy) {
                    rglobal_var = new RiscvGlobalVariable(RiscvTypeID::RIntImmTy, global_var->name, global_var->is_const, global_var->init_val, calculate_type_size(type) / 4);
                } else if(element_type->tid == TypeID::FloatTy) {
                    rglobal_var = new RiscvGlobalVariable(RiscvTypeID::RFloatImmTy, global_var->name, global_var->is_const, global_var->init_val, calculate_type_size(type) / 4);
                } else {
                    cerr << "[Error] [RiscvGenerator::generate] Unsupported array element type in global variable: " << global_var->name << endl;
                    exit(1);
                }
                break;
            }
            default: {
                cerr << "[Error] [RiscvGenerator::generate] Unsupported global variable type: ";
                global_var->print(cerr);
                cerr << "\n";
                exit(1);
            }
        }
        if(rglobal_var) {
            out << rglobal_var->print();
        } else {
            cerr << "[Error] [RiscvGenerator::generate] Failed to create RiscvGlobalVariable for " << global_var->name << endl;
            exit(1);
        }
    }
    // 浮点常量
    // 因为 RISCV 中没有没有类似于整型的 li a0 3 这种将浮点立即数加载到寄存器的指令，所以需要将浮点常量放在数据段中
    int float_const_count = 0;
    for(auto func: m->func_list) {
        auto rfunc = get_rfunc_from_func(func); // 创建 RiscvFunction，在后续代码段中使用
        if(!rfunc) {
            cerr << "[Error] [RiscvGenerator::generate] Failed to get RiscvFunction for " << func->name << endl;
            exit(1);
        }
        for(auto& bb: func->bb_list) {
            for(auto& inst: bb->inst_list) {
                for(auto& op: inst->operands) {
                    if(auto float_const = dynamic_cast<ConstFloat*>(op)) {
                        string cur_float_name = "float_const_" + to_string(float_const_count++);
                        out << cur_float_name << ":\n" << "\t.word\t" << "0x" << hex << *reinterpret_cast<uint32_t*>(&float_const->val) << "\n"; // 只取前10位
                        auto mem = new RiscvFloatMem(cur_float_name, 0, true);
                        rfunc->reg_alloc->set_val_to_mem(float_const, mem);
                        rfunc->reg_alloc->set_val_to_rval(float_const, mem);
                    }
                }
            }
        }
    }
}

void RiscvGenerator::generate_code_section(ostream& out) {
    out << ".section .text\n";
    for(auto func: m->func_list) {
        auto rfunc = get_rfunc_from_func(func);
        if(!rfunc) {
            cerr << "[Error] [RiscvGenerator::generate] Failed to get RiscvFunction for " << func->name << endl;
            exit(1);
        }
        // 如果是库函数，则创建对应的 RiscvFunction
        if(rfunc->is_libfunc()) {
            auto lib_func = create_lib_func(func);
            if(lib_func) {
                out << lib_func->print();
            }
            continue;
        }
        // 首先检查所有的alloca指令，加入一个基本块进行寄存器保护以及栈空间分配
        allocated_val.clear();
        int_param_count = 0;
        float_param_count = 0;
        param_offset = 0; // 参数偏移量
        rfunc->base = 0;
        // 关联函数参数、寄存器与内存
        for(Value* arg: func->args) {
            store_on_stack(arg, rfunc);
        }
        // Phi Zext BitCast 指令的合流
        for(BasicBlock* bb: func->bb_list) {
            for(Instruction* inst: bb->inst_list) {
                if(inst->iid == IRInstID::Phi) {
                    // NOTE: 对于任何指令，只需要在栈中保存一份位置即可，Phi 指令同样是的
                    store_on_stack(static_cast<Value*>(inst), rfunc);
                    // TODO: 这里存在问题
                    // for(int i = 0; i < inst->operands.size(); i += 2) {
                    //     auto op = inst->operands[i];
                    //     if(!dynamic_cast<ValUndef*>(op) && !dynamic_cast<Const*>(op)) {
                    //         rfunc->reg_alloc->uf->merge(static_cast<Value*>(op), static_cast<Value*>(inst)); // 将 Phi 指令的操作数连接到指令本身
                    //     }
                    // }
                } else if(inst->iid == IRInstID::ZExt) { // 将指令的操作数连接到指令本身
                    rfunc->reg_alloc->uf->merge(inst->operands[0], static_cast<Value*>(inst));
                } else if(inst->iid == IRInstID::BitCast) { // 将指令本身连接到指令的操作数
                    rfunc->reg_alloc->uf->merge(static_cast<Value*>(inst), inst->operands[0]);
                }
            }
        }
        // 处理所有的'非' Phi Zext Alloca 指令
        for(BasicBlock* bb: func->bb_list) {
            for(Instruction* inst: bb->inst_list) {
                // 获取所有非 Phi、ZExt、Alloca 指令，将其自身及操作数均压入栈中
                if(inst->iid != IRInstID::Phi && inst->iid != IRInstID::ZExt && inst->iid != IRInstID::Alloca) { // 非 Phi、ZExt、BitCast、Alloca 指令
                    store_on_stack(static_cast<Value*>(inst), rfunc); // 将指令本身压入栈
                    for(auto op: inst->operands) { // inst 的所有操作数
                        store_on_stack(static_cast<Value*>(op), rfunc); // 将操作数压入栈
                    }
                }
            }
        }
        // 处理所有的 Alloca 指令：分配指针，并且将指针地址也同步保存
        for(BasicBlock* bb: func->bb_list) {
            for(Instruction* inst: bb->inst_list) {
                if(inst->iid == IRInstID::Alloca) { // Alloca 指令
                    auto alloca_inst = static_cast<AllocaInst*>(inst);
                    int cur_type_size = calculate_type_size(alloca_inst->alloca_type);
                    rfunc->store_array(cur_type_size);
                    int cur_sp = rfunc->base;
                    RiscvValue* ptr_pos = new RiscvIntMem(get_reg_named("fp"), cur_sp);
                    rfunc->reg_alloc->set_val_to_rval(static_cast<Value*>(inst), ptr_pos);
                    rfunc->reg_alloc->set_val_to_mem(static_cast<Value*>(inst), ptr_pos);
                }
            }
        }
        // 添加初始化基本块（函数序言）
        RiscvBasicBlock* prologue_bb = get_rbb_from_bb();
        rfunc->add_rbb(prologue_bb);
        // 翻译语句并计算被使用的寄存器
        for(auto bb: func->bb_list) {
            rfunc->add_rbb(transfer_rbb_from_bb(bb, rfunc));
        }
        /* NOTE: Phi 指令的本质是一个选择操作，在翻译 Phi 指令之前，必须确保所有的前驱基本块已经被翻译，
            这样才能确定每个操作数的寄存器或者内存位置，从而确定每个操作数的来源 */
        for(auto bb: func->bb_list) {
            translate_phi_inst(rfunc->reg_alloc, bb);
        }  
        // 保护寄存器
        rfunc->base -= VARIABLE_ALIGN_BYTE;
        int fp_sp = rfunc->base; // 为保护 fp 分配空间
        auto& reg_to_save = reg_protected;
        auto reg_used = rfunc->reg_alloc->reg_used;
        for(auto reg: reg_to_save) {
            if(reg_used.find(reg) != reg_used.end()) {
                rfunc->base -= VARIABLE_ALIGN_BYTE;
                if(reg->rtid == RiscvTypeID::RIntRegTy) {
                    prologue_bb->add_rinst_back(new RiscvStoreInst(new Type(TypeID::PointerTy), reg,
                        new RiscvIntMem(get_reg_named("fp"), rfunc->base), prologue_bb));
                } else {
                    prologue_bb->add_rinst_back(new RiscvStoreInst(new Type(TypeID::FloatTy), reg,
                        new RiscvIntMem(get_reg_named("fp"), rfunc->base), prologue_bb));
                }
            }
        }
        // 分配整体的栈空间，并设置s0为原sp
        prologue_bb->add_rinst_front(new RiscvBinaryInst(RiscvInstID::ADDI, get_reg_op_named("sp"), new RiscvConst(-rfunc->base), get_reg_op_named("fp"), prologue_bb)); // 3: fp <- t0
        prologue_bb->add_rinst_front(new RiscvStoreInst(new Type(TypeID::PointerTy), get_reg_op_named("fp"), new RiscvIntMem(get_reg_named("sp"), fp_sp - rfunc->base), prologue_bb)); // 2: 保护 fp
        prologue_bb->add_rinst_front(new RiscvBinaryInst(RiscvInstID::ADDI, get_reg_op_named("sp"), new RiscvConst(rfunc->base), get_reg_op_named("sp"), prologue_bb)); // 1: 分配栈帧
        // 扫描所有的返回语句并插入寄存器还原等相关内容
        for(auto rbb: rfunc->rbb_list) {
            for(auto rinst: rbb->rinst_list) {
                if(rinst->riid == RiscvInstID::RET) {
                    init_ret_inst(rfunc->reg_alloc, rinst, rbb, rfunc);
                    break;
                }
            }
        }
        inst_emit(rfunc);
        optimize(rfunc);
        out << rfunc->print();
    }
}

void RiscvGenerator::generate(ostream& out) {
    out << ".align 2\n";
    // 数据段 .data
    generate_data_section(out);
    // 代码段 .text  
    generate_code_section(out);
}