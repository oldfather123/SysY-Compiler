#include "register.hpp"
#include "riscv_value.hpp"

vector<Register*> regs;
vector<RiscvValue*> reg_ops;
vector<RiscvValue*> reg_protected;

Register::Register(RegType reg_type, int reg_id)
    : reg_type(reg_type), reg_id(reg_id) {
    if(reg_type == RegType::FloatReg) { // 浮点寄存器，编号 f0 ~ f31，别名见下
        if(reg_id >= 0 && reg_id <= 7) {                       // ft0 ~ ft7，浮点临时数据存储
            alias = "ft" + to_string(reg_id);
        } else if(reg_id <= 9) {                // fs0 ~ fs1，浮点保存数据
            alias = "fs" + to_string(reg_id - 8);
        } else if(reg_id <= 17) {               // fa0 ~ fa7，浮点函数参数和返回值
            alias = "fa" + to_string(reg_id - 10);
        } else if(reg_id <= 27) {               // fs2 ~ fs11，浮点保存数据
            alias = "fs" + to_string(reg_id - 18 + 2);
        } else if(reg_id <= 31) {               // ft8 ~ ft11，浮点临时数据存储
            alias = "ft" + to_string(reg_id - 28 + 8);
        } else {
            cerr << "[ERROR] [Regester::Register] Invalid float register id: " << reg_id << "\n";
            exit(1);
        }
    } else {
        switch(reg_id) { // 整型寄存器，编号 x0 ~ x31，别名见下
            case 0:                                     // x0，零寄存器，恒为常量 0
                alias = "zero";
                break;
            case 1:                                     // ra，返回地址寄存器
                alias = "ra";
                break;
            case 2:                                     // sp，栈指针
                alias = "sp";
                break;
            case 3:                                     // gp，全局指针
                alias = "gp";
                break;
            case 4:                                     // tp，线程指针
                alias = "tp";
                break;
            case 5:
            case 6:
            case 7:                                     // t0 ~ t2，临时寄存器
                alias = "t" + to_string(reg_id - 5);
                break;
            case 8:                                     // fp/s0，帧指针
                alias = "fp"; // another name: s0
                break;
            case 9:                                     // s1，保存寄存器
                alias = "s1";
                break;
            default: {
                if(reg_id >= 10 && reg_id <= 17) {      // a0 ~ a7，函数参数和返回值
                    alias = "a" + to_string(reg_id - 10);
                } else if(reg_id >= 18 && reg_id <= 27) {      // s2 ~ s11，保存寄存器
                    alias = "s" + to_string(reg_id - 16);
                } else if(reg_id >= 28 && reg_id <= 31) {      // t3 ~ t6，临时寄存器
                    alias = "t" + to_string(reg_id - 25);
                } else {
                    cerr << "[ERROR] [Regester::Register] Invalid integer register id: " << reg_id << "\n";
                    exit(1);
                }
                break;
            }
        }
    }
}

Register* get_reg_named(const string& alias) {
    for(auto& reg: regs) {
        if(reg->alias == alias) {
            return reg;
        }
    }
    cerr << "[ERROR] [get_reg_named] No register found with alias: " << alias << "\n";
    exit(1);
}

RiscvValue* get_reg_op_named(const string& alias) {
    for(auto& reg_op: reg_ops) {
        if(reg_op->print() == alias) {
            return reg_op;
        }
    }
    cerr << "[ERROR] [get_reg_op_named] No register found with alias: " << alias << "\n";
    exit(1);
}

RiscvValue* get_reg_op_with_type_id(RegType reg_type, int reg_id) {
    for(auto reg_op: reg_ops) {
        if(auto int_reg = dynamic_cast<RiscvIntReg*>(reg_op)) {
            if(int_reg->reg->reg_type == reg_type && int_reg->reg->reg_id == reg_id) {
                return reg_op;
            }
        } else if(auto float_reg = dynamic_cast<RiscvFloatReg*>(reg_op)) {
            if(float_reg->reg->reg_type == reg_type && float_reg->reg->reg_id == reg_id) {
                return reg_op;
            }
        }
    }
    cerr << "[ERROR] [get_reg_op_with_type_id] No register found with type: " << static_cast<int>(reg_type) << " and id: " << reg_id << "\n";
    exit(1);
}