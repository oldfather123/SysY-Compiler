#ifndef INST_HELP_H
#define INST_HELP_H
#include<string>
#include"../basic/register.h"
#include <memory>
#include <stdexcept>
//注：参考原框架的Label,RiscVLabel,MachineBaseOperand,MachineRegister,为了方便后端代码调试，将原框架代码附在之后
//Label及RISCVLabel改动后使用上的差距
//1.成员变量的访问上：原代码：if (label.jmp_label_id == ...)；修改后的代码：if (label.get_jmp_id() == ...) 
//2.类型检查上：原代码：if (label.is_data_address) { ... }；修改后的代码：if (label.is_data_symbol()) { ... }
//MachineBaseOperand及其继承类暂未改动
struct MachineBaseOperand {
    MachineDataType type;
    enum { REG, IMMI, IMMF, IMMD };
    int op_type;
    MachineBaseOperand(int op_type) : op_type(op_type) {}
    virtual std::string toString() = 0;
};
struct MachineRegister : public MachineBaseOperand {
    Register reg;
    MachineRegister(Register reg) : MachineBaseOperand(MachineBaseOperand::REG), reg(reg) {}
    std::string toString() {
        if (reg.is_virtual)
            return "%" + std::to_string(reg.reg_no);
        else
            return "phy_" + std::to_string(reg.reg_no);
    }
};

struct MachineImmediateInt : public MachineBaseOperand {
    int imm32;
    MachineImmediateInt(int imm32) : MachineBaseOperand(MachineBaseOperand::IMMI), imm32(imm32) {}
    std::string toString() { return std::to_string(imm32); }
};
struct MachineImmediateFloat : public MachineBaseOperand {
    float fimm32;
    MachineImmediateFloat(float fimm32) : MachineBaseOperand(MachineBaseOperand::IMMF), fimm32(fimm32) {}
    std::string toString() { return std::to_string(fimm32); }
};
struct MachineImmediateDouble : public MachineBaseOperand {
    double dimm64;
    MachineImmediateDouble(double dimm64) : MachineBaseOperand(MachineBaseOperand::IMMD), dimm64(dimm64) {}
    std::string toString() { return std::to_string(dimm64); }
};

// 新增：标签类型枚举
enum class LabelType {
    Jump,       // 跳转标签（jmp_label_id）
    Memory,     // 内存标签（mem_label_id）
    Print,      // 打印标签（print_label_id）
    DataSymbol  // 数据符号标签（name + is_hi）
};

// 基类 Label，定义通用接口
class Label {
public:
    LabelType type;
    int seq_label_id;

    explicit Label(LabelType type, int seq = 0) 
        : type(type), seq_label_id(seq) {}

    virtual ~Label() = default;

    // 类型安全的访问方法（防止错误访问成员）
    virtual int get_jmp_id() const { 
        throw std::runtime_error("Not a jump label"); 
    }
    virtual int get_mem_id() const { 
        throw std::runtime_error("Not a memory label"); 
    }
    virtual int get_local_label_id() const {
        throw std::runtime_error("Not a local label");
    }
    virtual std::string get_data_name() const { 
        throw std::runtime_error("Not a data symbol"); 
    }
    virtual bool is_hi() const { 
        throw std::runtime_error("Not a data symbol"); 
    }

    // 深拷贝支持
    virtual std::unique_ptr<Label> clone() const = 0;
};

// 跳转标签（原 Label 的跳转分支）
class JumpLabel : public Label {
public:
    int jmp_label_id;

    JumpLabel(int jmp, int seq = 0) 
        : Label(LabelType::Jump, seq), jmp_label_id(jmp) {}

    int get_jmp_id() const override { return jmp_label_id; }

    std::unique_ptr<Label> clone() const override {
        return std::make_unique<JumpLabel>(*this);
    }
};

// 内存标签（原 Label 的内存分支）
class MemoryLabel : public Label {
public:
    int mem_label_id;

    MemoryLabel(int mem, int seq = 0) 
        : Label(LabelType::Memory, seq), mem_label_id(mem) {}

    int get_mem_id() const override { return mem_label_id; }

    std::unique_ptr<Label> clone() const override {
        return std::make_unique<MemoryLabel>(*this);
    }
};

// 数据符号标签（原 RiscVLabel 的数据分支）
class DataSymbolLabel : public Label {
public:
    std::string name;
    bool hi_part;  // 是否表示高位地址（%hi）

    DataSymbolLabel(const std::string& name, bool is_hi, int seq = 0) 
        : Label(LabelType::DataSymbol, seq), name(name), hi_part(is_hi) {}

    std::string get_data_name() const override { return name; }
    bool is_hi() const override { return hi_part; }

    std::unique_ptr<Label> clone() const override {
        return std::make_unique<DataSymbolLabel>(*this);
    }
};

class LocalLabel : public Label {
public:
    int local_id;

    LocalLabel(int id)
        : Label(LabelType::DataSymbol), local_id(id) {} // Note: Using DataSymbol for now

    int get_local_label_id() const override { return local_id; }
    std::string get_data_name() const override { return ""; } // Return empty string for local labels
    bool is_hi() const override { return false; } // Not applicable for local labels


    std::unique_ptr<Label> clone() const override {
        return std::make_unique<LocalLabel>(*this);
    }
};

class RiscVLabel {
private:
    std::unique_ptr<Label> label;

public:
    RiscVLabel() : label(std::make_unique<JumpLabel>(0, 0)) {}
    // 构造函数委托给具体 Label 类型
    RiscVLabel(int jmp, int seq) 
        : label(std::make_unique<JumpLabel>(jmp, seq)) {}

    RiscVLabel(const std::string& name, bool is_hi) 
        : label(std::make_unique<DataSymbolLabel>(name, is_hi)) {}

    RiscVLabel(int local_id)
        : label(std::make_unique<LocalLabel>(local_id)) {}

    // 拷贝构造函数和赋值运算符
    RiscVLabel(const RiscVLabel& other) 
        : label(other.label ? other.label->clone() : nullptr) {}

    RiscVLabel& operator=(const RiscVLabel& other) {
        if (this != &other) {
            label = other.label ? other.label->clone() : nullptr;
        }
        return *this;
    }

    // 类型检查方法
    bool is_jump() const { 
        return label && label->type == LabelType::Jump; 
    }
    bool is_data_symbol() const { 
        return label && label->type == LabelType::DataSymbol; 
    }

    bool is_local_label() const {
        return label && dynamic_cast<LocalLabel*>(label.get()) != nullptr;
    }

    // 访问成员（类型安全）
    int get_jmp_id() const {
        if (!is_jump()) throw std::runtime_error("Not a jump label");
        return static_cast<JumpLabel*>(label.get())->get_jmp_id();
    }

    std::string get_data_name() const {
        if (!is_data_symbol()) throw std::runtime_error("Not a data symbol");
        return static_cast<DataSymbolLabel*>(label.get())->get_data_name();
    }

    int get_local_label_id() const {
        if(!is_local_label()) throw std::runtime_error("Not a local label");
        return static_cast<LocalLabel*>(label.get())->get_local_label_id();
    }

    bool is_hi() const {
        if (!is_data_symbol()) throw std::runtime_error("Not a data symbol");
        return static_cast<DataSymbolLabel*>(label.get())->is_hi();
    }

    // 比较运算符
    bool operator==(const RiscVLabel& other) const {
        if (!label || !other.label) return false;
        if (label->type != other.label->type) return false;

        switch (label->type) {
            case LabelType::Jump:
                return get_jmp_id() == other.get_jmp_id();
            case LabelType::DataSymbol:
                if (is_local_label() && other.is_local_label()) {
                    return get_local_label_id() == other.get_local_label_id();
                }
                if (is_data_symbol() && other.is_data_symbol() && !is_local_label() && !other.is_local_label()){
                    return get_data_name() == other.get_data_name() 
                        && is_hi() == other.is_hi();
                }
                return false;
            default:
                return false; // 其他类型暂不处理
        }
    }
};

#endif
