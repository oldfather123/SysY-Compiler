#include <unordered_set>
#include "riscv_function.hpp"
#include "riscv_basic_block.hpp"
#include "reg_alloc.hpp"

unordered_set<string> lib_funcs = {
    "getint", "getch", "getfloat", "getarray", "getfarray",
    "putint", "putch", "putfloat", "putarray", "putfarray",
    "_sysy_starttime", "_sysy_stoptime",
    "__aeabi_memclr4", "llvm.memset.p0.i32"
};

RiscvFunction::RiscvFunction(const string& name)
    : RiscvValue(RiscvTypeID::RFunctionTy, name), base(-VARIABLE_ALIGN_BYTE) {
    reg_alloc= new RegAlloc(); // 初始化寄存器池
}

void RiscvFunction::add_arg(RiscvValue* arg) {
    if(arg == nullptr) {
        cerr << "[Error] Attempted to add a null argument to function " << name << ".\n";
        exit(1);
    }
    if(!args_offset.count(arg)) {
        args_offset[arg] = base; // 新增参数映射
        base -= VARIABLE_ALIGN_BYTE; // 更新栈帧基地址
    } else {
        cerr << "[Error] Argument already exists in function " << name << ".\n";
        exit(1);
    }
}

void RiscvFunction::add_temp_var(RiscvValue* val) {
    if(!val) {
        cerr << "[Error] Attempted to add a null temporary variable to function " << name << ".\n";
        exit(1);
    }
    add_arg(val); // 在栈上新增临时变量映射
}

void RiscvFunction::store_array(int byte_size) {
    if(byte_size <= 0) {
        cerr << "[Error] Invalid element number for array storage in function " << name << ".\n";
        exit(1);
    }
    // 进行 8 字节对齐存储
    if(byte_size & 7) { // 如果低三位有 1，说明不是 8 的整数倍
        byte_size += 8 - (byte_size & 7); // 补齐到下一个 8 的整数倍
    }
    base -= byte_size; // 更新栈帧基地址
}

void RiscvFunction::add_rbb(RiscvBasicBlock* rbb) {
    if(!rbb) {
        cerr << "[Error] Attempted to add a null basic block to function " << name << ".\n";
        exit(1);
    }
    rbb_list.push_back(rbb); // 添加基本块到函数的基本块列表
    rbb->parent = this; // 设置基本块的父函数
}

bool RiscvFunction::is_libfunc() {
    return lib_funcs.count(name);
}