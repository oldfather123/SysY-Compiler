#include "reg_alloc.hpp"
#include "riscv_value.hpp"
#include "riscv_basic_block.hpp"
#include "riscv_function.hpp"
#include "riscv_generator.hpp"
#include "register.hpp"
#include "ir_type.hpp"
#include "ir_value.hpp"
#include "ir_instruction.hpp"

// ----------------------------------------  并查集 UnionFind  ----------------------------------------

Value* UnionFind::get_parent(Value* val) {
    if(parent[val] == val) {
        return val;
    } else {
        parent[val] = get_parent(parent[val]); // 路径压缩
        return parent[val];
    }
}

Value* UnionFind::query(Value* val) {
    if(!parent.count(val)) {
        return parent[val] = val; // 初始化父节点为自身
    }
    return get_parent(val);
}

void UnionFind::merge(Value* val1, Value* val2) {
    val1 = query(val1);
    val2 = query(val2);
    if(val1 == val2) {
        return; // 已经是同一集合
    } else {
        parent[val1] = val2; // 将 val1 的父节点设置为 val2
    }
}

// ----------------------------------------  寄存器分配器 RegAlloc  ----------------------------------------

RegAlloc::RegAlloc(): int_reg_id(18), float_reg_id(18), timestamp(0) {
    uf = new UnionFind(); // 初始化并查集
    reg_used.insert(get_reg_op_named("ra")); // ra 寄存器储存函数的返回地址
}

void RegAlloc::set_val_to_rval(Value* val, RiscvValue* rval) {
    val = uf->query(val); // 确保 val 是根节点
    val_to_rval[val] = rval; // 将 val 映射到 RiscvValue
}

void RegAlloc::set_val_to_reg(Value* val, RiscvValue* reg) {
    val = uf->query(val); // 确保 val 是根节点
    if(!reg->is_reg()) {
        cerr << "[ERROR] [RegAlloc::set_val_to_reg] RiscvValue is not a register type.\n";
        exit(1);
    }
    val_to_reg[val] = reg; // 将 val 映射到寄存器 RiscvValue
    reg_to_val[reg] = val; // 反向映射寄存器到 Value
    reg_used.insert(reg); // 将寄存器标记为已使用
}

void RegAlloc::set_val_to_mem(Value* val, RiscvValue* mem) {
    val = uf->query(val); // 确保 val 是根节点
    if(val->type->tid != TypeID::PointerTy && val->type->tid != TypeID::ArrayTy && val->type->tid != TypeID::FloatTy && !val->is_const()) {
        cerr << "[ERROR] [RegAlloc::set_val_to_mem] Value not pointing memory.\n";
        exit(1);
    }
    val_to_mem[val] = mem; // 将 val 映射到内存位置 RiscvValue
    mem_to_val[mem] = val;
}

Value* RegAlloc::get_val_in_reg(RiscvValue* reg) {
    if(!reg_to_val.count(reg)) {
        return nullptr;
    }
    return uf->query(reg_to_val[reg]); // 并查集得到 Value* 的根结点，作为变量
}

RiscvValue* RegAlloc::get_reg_of_val(Value* val) {
    val = uf->query(val);
    if(!val_to_reg.count(val)) {
        return nullptr;
    }
    return val_to_reg[val];
}

Value* RegAlloc::get_val_in_mem(RiscvValue* mem) {
    if(!mem_to_val.count(mem)) {
        return nullptr;
    }
    return uf->query(mem_to_val[mem]);
}

RiscvValue* RegAlloc::get_mem_of_val(Value* val) {
    val = uf->query(val);
    if(!val_to_mem.count(val)) {
        cerr << "[ERROR] [RegAlloc::get_mem_of_val] Value not pointing memory.\n";
        exit(1);
    }
    return val_to_mem[val];
}

bool RegAlloc::update_val_to_reg(Value* val, RiscvValue* reg, RiscvBasicBlock* rbb, RiscvInstruction* rinst) {
    bool need_load = true;
    val = uf->query(val); // 确保 val 是变量根结点，即在 RISCV 视图下变量的唯一标识
    Value* old_val = get_val_in_reg(reg); // 得到给定寄存器对应的变量
    RiscvValue* old_reg = get_reg_of_val(val); // 得到给定变量对应的寄存器
    if(old_val != nullptr && old_val != val) { // 如果给定寄存器对应的变量不是给定变量，则将寄存器中的值写回到内存中
        write_back_reg(reg, rbb, rinst);
        need_load = true;
    } else if(old_val == nullptr) { // 如果寄存器没有对应的变量，则直接加载给定变量到给定寄存器中
    	need_load = true;
    } else { // old_val != nullptr && old_val == val, 即给定寄存器对应的变量就是给定变量，不需要额外的加载
        need_load = false;
    }
    if(old_reg != nullptr && old_reg != reg) { // 如果给定变量对应的寄存器不是给定寄存器，则将给定变量对应的寄存器中的值写回到内存中
        write_back_reg(old_reg, rbb, rinst); // 以节省寄存器资源
    }
    set_val_to_reg(val, reg);
    return need_load; // 返回是否真正将 val 对应的值从内存中加载到了寄存器中
}

void RegAlloc::clear_reg(RiscvValue* reg) {
    Value* val = get_val_in_reg(reg);
    if(val == nullptr) {
        return;
    }
    val = uf->query(val);
    if(reg_to_val.find(reg) != reg_to_val.end()) { // 去除寄存器到 Value* 的映射
        reg_to_val.erase(reg);
    }
    if(reg_find_timestamp.count(reg)) { // 去除寄存器的查找时间戳
        reg_find_timestamp.erase(reg);
    }
    if(val_to_reg.find(val) != val_to_reg.end()) { // 去除 Value* 到寄存器的映射
        val_to_reg.erase(val);
    }
}

RiscvValue* RegAlloc::get_reg_op(Value* val, RiscvBasicBlock* rbb, RiscvInstruction* rinst, bool load, RiscvValue* specified_reg, bool direct) {
    timestamp++; // 更新安全查找时间戳，即又一次查找了寄存器
    val = uf->query(val);
    bool is_global = dynamic_cast<GlobalVariable*>(val) != nullptr;
    bool is_alloca = dynamic_cast<AllocaInst*>(val) != nullptr;
    bool is_ptr = val->type->tid == TypeID::PointerTy;
    // 如果没有寄存器分配给这个变量，则分配一个
    if(specified_reg) { // 如果指定了寄存器
        bool need_load = update_val_to_reg(val, specified_reg, rbb, rinst);
        if(load == true) {
            load = need_load; // 如果原始寄存器对应的 value 即为目标 value，就没有 load 的必要了
        }
    } else if(val_to_reg.find(val) == val_to_reg.end() || is_alloca || val->name.empty()) { //? alloca 指令和常量不安全
        bool found = false;
        RiscvValue* cur = nullptr;
        // 循环使用 x18~x27、f18~f27 寄存器
        while(!found) {
            // cerr << "[Info] Here looking for a register\n";
            if(val->type->tid != TypeID::FloatTy) { // 整型数据，需要整型寄存器
                ++int_reg_id;
                if(int_reg_id > 27) {
                    int_reg_id = 18;
                }
                cur = get_reg_op_with_type_id(RegType::IntReg, int_reg_id);
            } else { // 浮点型数据，需要浮点寄存器
                ++float_reg_id;
                if(float_reg_id > 27) {
                    float_reg_id = 18;
                }
                cur = get_reg_op_with_type_id(RegType::FloatReg, float_reg_id);
            }
            if(!reg_find_timestamp.count(cur) || timestamp - reg_find_timestamp[cur] > SAFE_FIND_LIMIT) {
                val = uf->query(val); // 确保 val 是变量根结点
                Value* old_val = get_val_in_reg(cur); // 得到给定寄存器对应的变量
                RiscvValue* old_reg = get_reg_of_val(val); // 得到给定变量对应的寄存器
                if(old_val && old_val != val) { // 如果给定寄存器对应的变量不是给定变量，则将寄存器中的值写回到内存中
                    write_back_reg(cur, rbb, rinst);
                }
                if(old_reg && old_reg != cur) { // 如果给定变量对应的寄存器不是给定寄存器，则将给定变量对应的寄存器中的值写回到内存中
                    write_back_reg(old_reg, rbb, rinst);
                }
                set_val_to_reg(val, cur);
                found = true;
            }
        }
    } else {
        reg_find_timestamp[val_to_reg[val]] = timestamp;
        return val_to_reg[val];
    }
    // // ! 尽管所有寄存器都被认为是不安全的，但目前在 get_reg_op() 中无法正确地将寄存器写回。
    // // ! 因此，下面的不安全部分目前未被执行。
    // // ! 或许应该考虑使用 writeback() 方法。
    // 当前，所有寄存器都被认为是不安全的，因此寄存器在使用前
    // 必须从内存加载，并在使用后保存到内存中。
    // TODO: 将通过寄存器传递的参数从内存中剔除，此时不需要进行写回
    auto mem_addr = get_mem_op(val, rbb, rinst, 1); // Value's direct memory address
    auto cur_reg = val_to_reg[val];             // Value's current register
    auto load_type = val->type;
    reg_find_timestamp[cur_reg] = timestamp; // Update time stamp
    if(load) {
        if(mem_addr) {
            rbb->add_rinst_before_rinst(new RiscvLoadInst(load_type, cur_reg, mem_addr, rbb), rinst);
        } else if(val->name.empty()) {
            if(auto const_int = dynamic_cast<ConstInt*>(val)) { // 整型常量直接 LI 指令加载到寄存器
                rbb->add_rinst_before_rinst(new RiscvLiInst(cur_reg, const_int->val, rbb), rinst);
            } else if(dynamic_cast<ConstFloat*>(val)) { // 浮点型常量以内存的形式存在，需要寻找其地址
                rbb->add_rinst_before_rinst(new RiscvMoveInst(cur_reg, this->get_mem_op(val), rbb), rinst);
            } else {
                cerr << "[ERROR] [RegAlloc::get_reg_op] Constant Value But Not ConstInt and ConstFloat\n";
                exit(1);
            }
        } else if(is_alloca) {
            rbb->add_rinst_before_rinst(new RiscvBinaryInst(RiscvInstID::ADDI, get_reg_op_named("fp"),
                                new RiscvConst(static_cast<RiscvIntMem*>(val_to_rval[val])->offset), cur_reg, rbb), rinst);
        } else {
            cerr<< "[ERROR] [RegAlloc::get_reg_op] Cannot find memory address for value\n";
            exit(1);
        }
    }
    return cur_reg;
}

RiscvValue* RegAlloc::get_mem_op(Value* val, RiscvBasicBlock* rbb, RiscvInstruction* rinst, bool direct) {
    val = uf->query(val);
    if(!val_to_rval.count(val) && !val->name.empty()) { // 系统中不存在这个变量的映射且这个变量不是常数
        // NOTE: 理论上不应该到这，应该不是寄存器操作数就是内存操作数
        cerr<< "[Warning] [RegAlloc::get_mem_op] Value " << hex << val << " (" << val->name << ")'s memory map not found." << endl;
    }
    bool is_global = dynamic_cast<GlobalVariable*>(val) != nullptr;
    bool is_ptr = val->type->tid == TypeID::PointerTy;
    bool is_alloca = dynamic_cast<AllocaInst*>(val) != nullptr;
    is_global = is_global || dynamic_cast<ConstFloat*>(val) != nullptr;
    if(is_global) { // 如果是全局变量，则将其地址加载到寄存器 t5 中
        if(!rbb) {
            // cerr<< "[Warning] [RegAlloc::get_mem_op] Trying to add global variable addressing instruction, but basic block pointer is null.\n";
            return nullptr;
        }
        rbb->add_rinst_before_rinst(new RiscvLoadAddrInst(get_reg_op_named("t5"), val_to_rval[val]->print(), rbb), rinst);
        return new RiscvIntMem("t5");
    }
    // 如果不是直接加载指针的地址，则说明是间接寻址，使用间接寻址进行操作
    // 忽略 alloca，因为该指令只在 get_reg_op() 中处理
    if(is_ptr && !is_alloca && !direct) {
        if(!rbb) {
            cerr<< "[Warning] [RegAlloc::get_mem_op] Trying to add indirect pointer addressing instruction, but basic block pointer is null.\n";
            return nullptr;
        }
        if(val_to_reg.find(val) != val_to_reg.end()) { // 如果当前已经有寄存器储存的该变量的值，则直接返回该寄存器
            return new RiscvIntMem(val_to_reg[val]->print());
        }
        // 如果没有寄存器储存该变量的值，则需要加载该变量的地址，并储存至寄存器 t4 中
        rbb->add_rinst_before_rinst(new RiscvLoadInst(new Type(TypeID::PointerTy), get_reg_op_named("t4"), val_to_rval[val], rbb), rinst);
        return new RiscvIntMem("t4");
    } else if(direct && is_alloca) { // 无法直接读取 alloca 指令的地址
        return nullptr;
    }
    if(!val_to_rval.count(val)) {
        return nullptr;
    }
    return val_to_rval[val];
}

RiscvValue* RegAlloc::get_specified_reg_op(Value* val, string reg_alias, RiscvBasicBlock* rbb, RiscvInstruction* rinst, bool direct) {
    val = uf->query(val);
    RiscvValue* reg_op = get_reg_op_named(reg_alias);
    return get_reg_op(val, rbb, rinst, 1, reg_op, direct);
}

RiscvInstruction* RegAlloc::write_back_reg(RiscvValue* reg, RiscvBasicBlock* rbb, RiscvInstruction* rinst) {
    Value* value = get_val_in_reg(reg); // 寻找到对应的 Value* 对应的根结点，即代表同一变量的 Value*
    if(!value) {
        return nullptr; // 未找到
    }
    // 清理映射信息
    reg_to_val.erase(reg);
    reg_find_timestamp.erase(reg);
    val_to_reg.erase(value);
    RiscvValue* mem_addr = get_mem_op(value);
    if(!mem_addr) {
        return nullptr; // Maybe an immediate value or dicrect accessing alloca
    }
    auto store_type = value->type;
    auto store_inst = new RiscvStoreInst(value->type, reg, mem_addr, rbb);
    // 插入 store 指令到基本块中
    if(rinst) {
        rbb->add_rinst_before_rinst(store_inst, rinst);
    } else {
        rbb->add_rinst_back(store_inst);
    }
    return store_inst;
}

RiscvInstruction* RegAlloc::write_back_val(Value* val, RiscvBasicBlock* rbb, RiscvInstruction* rinst) {
    auto reg = get_reg_of_val(val);
    return write_back_reg(reg, rbb, rinst);
}

void RegAlloc::write_back_all_regs(RiscvBasicBlock* rbb, RiscvInstruction* rinst) {
    vector<RiscvValue*> regs_to_writeback;
    for(auto [reg, val]: reg_to_val) {
        regs_to_writeback.push_back(reg);
    }
    if(regs_to_writeback.empty()) {
        return; // 没有寄存器需要写回
    }
    for(auto reg: regs_to_writeback) {
        write_back_reg(reg, rbb, rinst);
    }
}