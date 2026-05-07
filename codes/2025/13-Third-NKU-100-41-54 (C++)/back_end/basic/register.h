#ifndef REGISTER_H
#define REGISTER_H
#include "../../include/Instruction.h"
#include<string>
#include<vector>
#include"riscv_def.h"
//注：参考原框架的MachineDataType和Register类，修改空间不大，暂未修改
typedef unsigned __int128 Uint128;
typedef unsigned long long Uint64;
typedef unsigned int Uint32;
//定义基础的数据类型，包括int&float,32&64&128
struct MachineDataType {
    enum { INT, FLOAT };
    enum { B32, B64, B128 };
    unsigned data_type;
    unsigned data_length;
    MachineDataType() {}
    MachineDataType(const MachineDataType &other) {
        this->data_type = other.data_type;
        this->data_length = other.data_length;
    }
    MachineDataType operator=(const MachineDataType &other) {
        this->data_type = other.data_type;
        this->data_length = other.data_length;
        return *this;
    }
    MachineDataType(unsigned data_type, unsigned data_length) : data_type(data_type), data_length(data_length) {}
    bool operator==(const MachineDataType &other) const {
        return this->data_type == other.data_type && this->data_length == other.data_length;
    }
    int getDataWidth() {
        switch (data_length) {
        case B32:
            return 4;
        case B64:
            return 8;
        case B128:
            return 16;
        }
        return 0;
    }
    std::string toString() {
        std::string ret;
        if (data_type == INT)
            ret += 'i';
        if (data_type == FLOAT)
            ret += 'f';
        if (data_length == B32)
            ret += "32";
        if (data_length == B64)
            ret += "64";
        if (data_length == B128)
            ret += "128";
        return ret;
    }
};
extern MachineDataType INT32, INT64, INT128, FLOAT_32, FLOAT64, FLOAT128;

//定义数据类型、对应的寄存器编号、是否为虚拟寄存器（是否需要溢出）
struct Register {
public:
    int reg_no;         // 寄存器编号
    bool is_virtual;    // 是否为虚拟寄存器
    MachineDataType type;

    Register() : is_virtual(false) {}
    Register(bool is_virtual, int reg_no, MachineDataType type, bool save = false)
        : is_virtual(is_virtual), reg_no(reg_no), type(type) {}
    int getDataWidth() { return type.getDataWidth(); }
    Register(const Register &other) {
        this->is_virtual = other.is_virtual;
        this->reg_no = other.reg_no;
        this->type = other.type;
    }
    Register operator=(const Register &other) {
        this->is_virtual = other.is_virtual;
        this->reg_no = other.reg_no;
        this->type = other.type;
        return *this;
    }
    bool operator<(Register other) const {
        if (is_virtual != other.is_virtual)
            return is_virtual < other.is_virtual;
        if (reg_no != other.reg_no)
            return reg_no < other.reg_no;
        if (type.data_type != other.type.data_type)
            return type.data_type < other.type.data_type;
        if (type.data_length != other.type.data_length)
            return type.data_length < other.type.data_length;
        return false;
    }
    bool operator==(Register other) const {
        return reg_no == other.reg_no && is_virtual == other.is_virtual && type.data_type == other.type.data_type &&
                type.data_length == other.type.data_length;
    }
};
static inline MachineDataType getRVRegType(int reg_no) {
    if (reg_no >= RISCV_x0 && reg_no <= RISCV_x31) {
        return INT64;
    }
    if (reg_no >= RISCV_f0 && reg_no <= RISCV_f31) {
        return FLOAT64;
    }
    ERROR("Unknown reg_no %d", reg_no);
}

static inline Register GetPhysicalReg(int reg_no) { return Register(false, reg_no, getRVRegType(reg_no)); }

#pragma GCC diagnostic ignored "-Wwritable-strings"
#pragma GCC diagnostic ignored "-Wc99-designator"
struct RiscV64RegisterInfo {
    char *name;
};
extern std::unordered_map<int, RiscV64RegisterInfo> RiscV64Registers;
extern Register RISCVregs[];
#endif