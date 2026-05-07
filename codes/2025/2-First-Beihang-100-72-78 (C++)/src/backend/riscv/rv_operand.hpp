#ifndef GEECEECEE_RV_OPERAND_HPP
#define GEECEECEE_RV_OPERAND_HPP

#pragma once
#include <set>
#include <string>
#include <utility>

namespace backend::riscv {

class RVInstruction;  // 前向声明
class RVFunction;     // Forward declaration

class RVOperand {
public:
    enum class Kind { REG, IMM, LABEL, STK, HI, LO, STACK_FIXER, VIRTUAL_REG };
    virtual Kind get_kind() const = 0;
    virtual std::string to_string() const = 0;
    virtual ~RVOperand();

    // 判断是否为预着色寄存器（默认非寄存器返回false）
    virtual bool is_precolored() const { return false; }

    // Def-Use chain management
    virtual void add_graph_use(RVInstruction *inst);
    virtual void remove_graph_use(RVInstruction *inst);
    virtual const std::set<RVInstruction *> &get_graph_uses() const;
    virtual void clear_uses() { graph_uses.clear(); }

protected:
    std::set<RVInstruction *> graph_uses;
};

class RVReg : public RVOperand {
public:
    // 保持接口与RVOperand一致，便于多态使用
    Kind get_kind() const override = 0;
    virtual std::string to_string() const override = 0;
    virtual bool is_precolored() const override { return false; }
    virtual ~RVReg() override = default;
};

/* -------- Register -------- */
class RVPhyReg : public RVReg {
public:
    enum class Type { INT, FLOAT };

    static RVPhyReg *create(int phys_id, const std::string &name = "");
    static RVPhyReg *create(int phys_id, const std::string &name, Type type);
    Kind get_kind() const override { return Kind::REG; }
    int get_phys_id() const { return _phys_id; }
    std::string to_string() const override;

    // 判断是否为预着色寄存器（物理寄存器）
    bool is_precolored() const override { return true; }

    // 获取寄存器类型（整数或浮点）
    virtual Type get_type() const { return _type; }

    // Enhanced Def-Use chain management for registers
    void add_graph_use(RVInstruction *inst) override;
    void remove_graph_use(RVInstruction *inst) override;
    virtual ~RVPhyReg() override;

private:
    RVPhyReg(int phys_id, std::string name, Type type = Type::INT)
        : _phys_id(phys_id), _name(std::move(name)), _type(type) {}
    int _phys_id = 0;
    std::string _name;  // e.g. fa0, ra
    Type _type;         // 寄存器类型
};

/* -------- Virtual Register -------- */
class RVVirReg : public RVReg {
public:
    enum class RegType { INT_TYPE, FLOAT_TYPE };

    static RVVirReg *create(int index, RegType reg_type, RVFunction *function);
    Kind get_kind() const override { return Kind::VIRTUAL_REG; }
    std::string to_string() const override { return _name; }

    int get_index() const { return _index; }
    RegType get_reg_type() const { return _reg_type; }
    RVFunction *get_function() const { return _function; }

    // 虚拟寄存器不是预着色的
    bool is_precolored() const override { return false; }

    // 获取寄存器类型（整数或浮点）
    RVPhyReg::Type get_type() const {
        return (_reg_type == RegType::INT_TYPE) ? RVPhyReg::Type::INT : RVPhyReg::Type::FLOAT;
    }
    virtual ~RVVirReg() override;

private:
    RVVirReg(int index, RegType reg_type, RVFunction *function);
    int _index;
    RegType _reg_type;
    std::string _name;
    RVFunction *_function;  // 裸指针
};

/* -------- Immediate -------- */
class RVImmediate : public RVOperand {
public:
    static RVImmediate *create(long long val);
    Kind get_kind() const override { return Kind::IMM; }
    long long value() const { return val; }
    std::string to_string() const override;
    virtual ~RVImmediate() override;

private:
    RVImmediate(long long val);
    long long val;
};

/* -------- Label / Symbol -------- */
class RVLabel : public RVOperand {
public:
    static RVLabel *create(std::string name);
    Kind get_kind() const override { return Kind::LABEL; }
    std::string to_string() const override { return _name; }
    const std::string &name() const { return _name; }
    virtual ~RVLabel() override;

private:
    RVLabel(std::string name) : _name(std::move(name)) {}
    std::string _name;
};

/* -------- Stack Location -------- */
class RVStk : public RVOperand {
public:
    static RVStk *create(int offset);
    Kind get_kind() const override { return Kind::STK; }
    std::string to_string() const override;
    int get_offset() const { return _offset; }
    virtual ~RVStk() override;

private:
    RVStk(int offset) : _offset(offset) {}
    int _offset;
};

/* -------- Stack Fixer -------- */
class RVStackFixer : public RVOperand {
public:
    static RVStackFixer *create(RVFunction *function, int extra_offset);
    Kind get_kind() const override { return Kind::STACK_FIXER; }
    std::string to_string() const override;
    int get_extra_offset() const { return _extra_offset; }
    int get_aligned_offset() const;
    RVFunction *get_function() const { return _function; }
    virtual ~RVStackFixer() override;

private:
    RVStackFixer(RVFunction *function, int extra_offset);
    RVFunction *_function;  // 裸指针
    int _extra_offset;
};

/* -------- Symbol Address Parts -------- */
class RVHi : public RVOperand {
public:
    static RVHi *create(RVLabel *label);
    Kind get_kind() const override { return Kind::HI; }
    std::string to_string() const override;
    RVLabel *get_label() const;
    virtual ~RVHi() override;

private:
    RVHi(RVLabel *label);
    RVLabel *_label;
};

class RVLo : public RVOperand {
public:
    static RVLo *create(RVLabel *label);
    Kind get_kind() const override { return Kind::LO; }
    std::string to_string() const override;
    RVLabel *get_label() const;
    virtual ~RVLo() override;

private:
    RVLo(RVLabel *label);
    RVLabel *_label;
};

}  // namespace backend::riscv

#endif
