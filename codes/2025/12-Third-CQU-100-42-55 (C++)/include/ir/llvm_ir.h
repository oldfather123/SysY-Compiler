#ifndef LLVM_IR_H
#define LLVM_IR_H

#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <iostream>
#include <set>

namespace llvm_ir {
class Instruction;  // 前向声明
class BasicBlock;
class Function; 
class PhiInst;
class Value;

// ================= Helper Functions =================
// 普通指令克隆
// valueMap: 值映射，用于处理指令中的值，如果值在valueMap中，则使用valueMap中的值，否则使用原始值
std::unique_ptr<Instruction> cloneInstruction(const Instruction* orig, std::map<Value*, Value*>& valueMap, const std::string& namePrefix);

// PHI指令克隆，不包含incoming_values
std::unique_ptr<PhiInst> clonePhi(const PhiInst* orig, const std::string& namePrefix);


// ================= Type&Value =================
// LLVM IR基本类型
enum class Type {
    Void,
    I1,       // bool
    I8,       // char
    I32,      // int
    I64,      // long 目前没实现
    Float,    // float
    Double,   // double 目前没实现
    Ptr,      // pointer
    Array,    // array
    Function  // function type
};

// LLVM IR值的基类
class Value {
public:
    std::string name;
    Type type;

    Value(const std::string& n, Type t) : name(n), type(t) {}
    virtual ~Value() = default;
    virtual std::string toString() const { return getTypeName() + " " + name; }

    std::string getTypeName() const;
    // --- 新增: SSA 用途追踪 ---
    void addUse(Instruction* user);
    void removeUse(Instruction* user);  // 在 ir.cpp 中实现
    std::vector<Instruction*>& getUses();
    const std::vector<Instruction*>& getUses() const;
    
    // 返回当前对象的Value*指针
    virtual Value* get_value_ptr() { return this; }

private:
    std::vector<Instruction*> uses;
};

// 常量值
class Constant : public Value {
public:
    Constant(const std::string& n, Type t) : Value(n, t) {}
    virtual std::string toString() const override = 0;
};

// 整数常量
class ConstantInt : public Constant {
public:
    int32_t value;

    ConstantInt(int32_t val, Type t = Type::I32) : Constant(std::to_string(val), t), value(val) {}

    std::string toString() const override { return getTypeName() + " " + std::to_string(value); }
};

// 浮点常量
class ConstantFloat : public Constant {
public:
    float value;

    // 单参数构造函数，默认使用 Type::Float
    ConstantFloat(float val) : Constant("", Type::Float), value(val) {
        // name 在 toString() 中动态生成，所以这里可以是空的
    }

    // 构造函数保持不变，它接收一个 double 和目标类型
    ConstantFloat(float val, Type t) : Constant("", t), value(val) {
        // name 在 toString() 中动态生成，所以这里可以是空的
    }

    std::string toString() const override { return getTypeName() + " " + formatFloatAsHex(); }

private:
    // 根据类型将浮点数格式化为十六进制
    std::string formatFloatAsHex() const {
        std::ostringstream oss;
        oss << "0x" << std::hex << std::uppercase << std::setfill('0');

        if (this->type == Type::Float) {
            // 转换为单精度浮点数
            float float_val = static_cast<float>(this->value);

            // 将单精度值扩展为双精度表示
            double double_val = static_cast<double>(float_val);

            // 获取64位位表示
            uint64_t long_val;
            std::memcpy(&long_val, &double_val, sizeof(long_val));

            // 输出16位十六进制（64位）
            oss << std::setw(16) << long_val;

        } else if (this->type == Type::Double) {
            uint64_t long_val;
            std::memcpy(&long_val, &this->value, sizeof(long_val));
            oss << std::setw(16) << long_val;
        }

        return oss.str();
    }
};

// 全局变量
class GlobalVariable : public Value {
public:
    Value* initializer;
    bool isConstant;
    int arraySize;     // 数组大小（0表示非数组）
    Type elementType;  // 数组元素类型
    // 用于常量数组的元素级初始化（当 arraySize > 0 时使用）
    std::vector<int32_t> intInitVals;   // elementType == I32
    std::vector<float>   floatInitVals; // elementType == Float

    GlobalVariable(const std::string& name, Type type, Value* init = nullptr, bool constant = false) : Value(name, type), initializer(init), isConstant(constant), arraySize(0), elementType(Type::Void) {}

    // 数组类型构造函数
    GlobalVariable(const std::string& name, Type arrayType, Value* init, bool constant, int size, Type elemType) : Value(name, arrayType), initializer(init), isConstant(constant), arraySize(size), elementType(elemType) {}

    std::string toString() const override;
};


// ================= BasicBlock&Function&Module =================
class BasicBlock {
public:
    std::string label;
    std::vector<std::string> comments;                                     // 注释
    std::list<std::unique_ptr<Instruction>> instructions;    // 改用 list
    std::vector<std::unique_ptr<PhiInst>> phi_instructions;  // 分开存储 PHI 指令
    Function* parent;                                        // 指向父函数
    std::vector<BasicBlock*> predecessors;                   // 前驱块
    std::vector<BasicBlock*> successors;                     // 后继块

    BasicBlock(const std::string& name, Function* p = nullptr, std::vector<std::string> comments = {}) : label(name), comments(comments), parent(p) {}

    void addInstruction(std::unique_ptr<Instruction> inst);  // 在instructions中Push_back，并设置parent
    void insertInstructionAt(std::unique_ptr<Instruction> inst, size_t position); // 在指定位置插入指令，并设置parent
    void addPhi(std::unique_ptr<PhiInst> phi);               // 在phi_instructions中Push_back，并设置parent
    void insertPhiAt(std::unique_ptr<PhiInst> phi, size_t position); // 在指定位置插入phi指令，并设置parent
    // 替换指令，只能用于同类型指令！！！！
    void replaceInstructionWith(Instruction* oldinst, std::unique_ptr<Instruction> newinst); // 用newinst替换oldinst，设置parent，维护use关系
    Instruction* getTerminator() const;                      // 获取块的终止指令 (br, ret), 如果块中没有终止指令，返回nullptr
    std::string toString() const;
    std::string getName() const { return label; }
};

// 函数属性枚举
enum class FunctionAttribute {
    NoUnwind,  // Function does not throw exceptions
    ReadNone,  // Function does not access memory
    ReadOnly,  // Function only reads from memory
};

// 函数
class Function {
public:
    std::string name;
    Type returnType;
    std::vector<Value*> parameters;
    std::vector<std::unique_ptr<BasicBlock>> basicBlocks;
    std::set<FunctionAttribute> attributes; // 函数属性
    
    Function(const std::string& n, Type retType) : name(n), returnType(retType) {}
    
    void addBasicBlock(std::unique_ptr<BasicBlock> bb); // 在basicBlocks中Push_back，并设置parent
    void deleteBasicBlock(BasicBlock* bb); // 删除基本块，并维护CFG和PHI
    void insertBasicBlockAt(std::unique_ptr<BasicBlock> bb, size_t position); // 在指定位置插入基本块

    BasicBlock* getEntryBlock() const;
    BasicBlock* getBlockByName(const std::string& name);
    bool isDeclaration() const { return basicBlocks.empty(); }
    std::string toString() const;
};

// 模块
class Module {
public:
    std::string name;
    std::vector<std::unique_ptr<Function>> functions;
    std::vector<std::unique_ptr<GlobalVariable>> globalVariables;

    Module(const std::string& n) : name(n) {}

    void addFunction(std::unique_ptr<Function> func) { functions.push_back(std::move(func)); }

    void addGlobalVariable(std::unique_ptr<GlobalVariable> gvar) { globalVariables.push_back(std::move(gvar)); }

    std::string toString() const;
};


// ================= Instruction =================
// 指令操作码
enum class Opcode {
    // 终止指令
    Ret,
    Br,
    Switch,

    // 算术指令
    Add,
    FAdd,
    Sub,
    FSub,
    Mul,
    FMul,
    UDiv,
    SDiv,
    FDiv,
    URem,
    SRem,
    FRem,

    // 位运算指令
    Shl,
    LShr,
    AShr,
    And,
    Or,
    Xor,

    // 内存指令
    Alloca,
    Load,
    Store,
    GetElementPtr,

    // 类型转换指令
    Trunc,
    ZExt,
    SExt,
    FPToUI,
    FPToSI,
    UIToFP,
    SIToFP,
    FPTrunc,
    FPExt,
    PtrToInt,
    IntToPtr,
    BitCast,

    // 比较指令
    ICmp,
    FCmp,

    // 其他指令
    PHI,
    Call,
    Select,

    Move
};

// 比较条件
enum class ICmpCond {
    EQ,   // equal
    NE,   // not equal
    UGT,  // unsigned greater than
    UGE,  // unsigned greater or equal
    ULT,  // unsigned less than
    ULE,  // unsigned less or equal
    SGT,  // signed greater than
    SGE,  // signed greater or equal
    SLT,  // signed less than
    SLE   // signed less or equal
};

enum class FCmpCond {
    FALSE,  // always false
    OEQ,    // ordered and equal
    OGT,    // ordered and greater than
    OGE,    // ordered and greater than or equal
    OLT,    // ordered and less than
    OLE,    // ordered and less than or equal
    ONE,    // ordered and not equal
    ORD,    // ordered (no nans)
    UEQ,    // unordered or equal
    UGT,    // unordered or greater than
    UGE,    // unordered or greater than or equal
    ULT,    // unordered or less than
    ULE,    // unordered or less than or equal
    UNE,    // unordered or not equal
    UNO,    // unordered (either nans)
    TRUE    // always true
};

// LLVM IR指令基类
class Instruction : public Value {
public:
    Opcode opcode;                 // 指令类型
    std::vector<Value*> operands;  // 操作数
    BasicBlock* parent;            // 指向父基本块

    Instruction(Opcode op, const std::string& name, Type type) : Value(name, type), opcode(op), parent(nullptr) {}

    virtual std::string toString() const override = 0;
    std::string getOpcodeName() const;
    // ---替换用途/移除 ---
    void replaceAllUsesWith(Value* newValue);  // 为newValue添加所有use，用newValue替换this在所有use中的引用，并清空当前指令的use
    void removeFromParent();                   // 在parent的instructions/phi_instructions中删除当前指令, 并移除对所有操作数的use关系
    void replaceOperand(Value* oldValue, Value* newValue); // 在指令的operands中将oldValue替换为newValue，并更新use关系

protected:
    // 获取操作数的正确值表示
    std::string getOperandValue(Value* operand) const {
        if (!operand) {
            std::cerr << "[ERROR] getOperandValue: operand is nullptr" << std::endl;
            return "<nullptr>";
        }

        if (operand->name.empty()) {
            // 如果name为空，说明是常量，使用toString()获取完整表示
            std::string fullStr = operand->toString();
            // 从toString()结果中提取值部分（去掉类型名）
            size_t spacePos = fullStr.find(' ');
            if (spacePos != std::string::npos) {
                return fullStr.substr(spacePos + 1);
            }
            return fullStr;
        } else {
            std::string nameStr = operand->name;
            return nameStr;
        }
    }
};

// 算术指令
class BinaryOperator : public Instruction {
public:
    BinaryOperator(Opcode op, Value* lhs, Value* rhs, const std::string& name) : Instruction(op, name, lhs->type) {
        operands.push_back(lhs);
        operands.push_back(rhs);
        if (lhs)
            lhs->addUse(this);
        if (rhs)
            rhs->addUse(this);
    }

    std::string toString() const override { return name + " = " + getOpcodeName() + " " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + ", " + getOperandValue(operands[1]); }
};

// 比较指令
std::string getICmpConditionName(ICmpCond condition);
class ICmpInst : public Instruction {
public:
    ICmpCond condition;

    ICmpInst(ICmpCond cond, Value* lhs, Value* rhs, const std::string& name) : Instruction(Opcode::ICmp, name, Type::I1), condition(cond) {
        operands.push_back(lhs);
        operands.push_back(rhs);
        if (lhs)
            lhs->addUse(this);
        if (rhs)
            rhs->addUse(this);
    }

    std::string toString() const override;
};

class FCmpInst : public Instruction {
public:
    FCmpCond condition;

    FCmpInst(FCmpCond cond, Value* lhs, Value* rhs, const std::string& name) : Instruction(Opcode::FCmp, name, Type::I1), condition(cond) {
        operands.push_back(lhs);
        operands.push_back(rhs);
        if (lhs)
            lhs->addUse(this);
        if (rhs)
            rhs->addUse(this);
    }

    std::string toString() const override;
};

// 内存分配指令
class AllocaInst : public Instruction {
public:
    Type allocatedType;
    int arraySize;  // 0表示单个元素，>0表示数组

    AllocaInst(Type type, const std::string& name) : Instruction(Opcode::Alloca, name, Type::Ptr), allocatedType(type), arraySize(0) {}

    AllocaInst(Type type, int size, const std::string& name) : Instruction(Opcode::Alloca, name, Type::Ptr), allocatedType(type), arraySize(size) {}

    std::string toString() const override;
    std::string getName() const { return name; }
};

// Load指令
class LoadInst : public Instruction {
public:
    LoadInst(Value* ptr, const std::string& name, Type loadType) : Instruction(Opcode::Load, name, loadType) {
        operands.push_back(ptr);
        if (ptr)
            ptr->addUse(this);
    }

    Value* getPointerOperand() const { return operands[0]; }

    std::string toString() const override { return name + " = load " + getTypeName() + ", " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]); }
};

// Store指令
class StoreInst : public Instruction {
public:
    StoreInst(Value* val, Value* ptr) : Instruction(Opcode::Store, "", Type::Void) {
        operands.push_back(val);
        operands.push_back(ptr);
        if (val)
            val->addUse(this);
        if (ptr)
            ptr->addUse(this);
    }

    Value* getValueOperand() const { return operands[0]; }
    Value* getPointerOperand() const { return operands[1]; }

    std::string toString() const override { return "store " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + ", " + operands[1]->getTypeName() + " " + getOperandValue(operands[1]); }
};

// ZExt指令 (零扩展)
class ZExtInst : public Instruction {
public:
    Type targetType;

    ZExtInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::ZExt, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = zext " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        case Type::Void: return "void";
        default: return "i32";
        }
    }
};

// Trunc指令 (截断)
class TruncInst : public Instruction {
public:
    Type targetType;

    TruncInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::Trunc, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = trunc " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "i32";
        }
    }
};

// SExt指令 (符号扩展)
class SExtInst : public Instruction {
public:
    Type targetType;

    SExtInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::SExt, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = sext " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "i32";
        }
    }
};

// SIToFP指令 (有符号整数转浮点)
class SIToFPInst : public Instruction {
public:
    Type targetType;

    SIToFPInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::SIToFP, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = sitofp " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "void";
        }
    }
};

// FPToSI指令 (浮点转有符号整数)
class FPToSIInst : public Instruction {
public:
    Type targetType;

    FPToSIInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::FPToSI, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = fptosi " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "void";
        }
    }
};

// FPToUI指令 (浮点转无符号整数)
class FPToUIInst : public Instruction {
public:
    Type targetType;

    FPToUIInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::FPToUI, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = fptoui " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "i32";
        }
    }
};

// UIToFP指令 (无符号整数转浮点)
class UIToFPInst : public Instruction {
public:
    Type targetType;

    UIToFPInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::UIToFP, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = uitofp " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "i32";
        }
    }
};

// FPTrunc指令 (浮点截断)
class FPTruncInst : public Instruction {
public:
    Type targetType;

    FPTruncInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::FPTrunc, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = fptrunc " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "i32";
        }
    }
};

// FPExt指令 (浮点扩展)
class FPExtInst : public Instruction {
public:
    Type targetType;

    FPExtInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::FPExt, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = fpext " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "i32";
        }
    }
};

// PtrToInt指令 (指针转整数)
class PtrToIntInst : public Instruction {
public:
    Type targetType;

    PtrToIntInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::PtrToInt, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = ptrtoint " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "i32";
        }
    }
};

// IntToPtr指令 (整数转指针)
class IntToPtrInst : public Instruction {
public:
    Type targetType;

    IntToPtrInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::IntToPtr, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = inttoptr " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "i32";
        }
    }
};

// BitCast指令 (位转换)
class BitCastInst : public Instruction {
public:
    Type targetType;

    BitCastInst(Value* val, Type toType, const std::string& name) : Instruction(Opcode::BitCast, name, toType), targetType(toType) {
        operands.push_back(val);
        if (val)
            val->addUse(this);
    }

    std::string toString() const override { return name + " = bitcast " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + " to " + getTypeNameForType(targetType); }

private:
    std::string getTypeNameForType(Type t) const {
        switch (t) {
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        default: return "i32";
        }
    }
};

// Select指令 (条件选择)
class SelectInst : public Instruction {
public:
    SelectInst(Value* cond, Value* trueVal, Value* falseVal, const std::string& name) : Instruction(Opcode::Select, name, trueVal->type) {
        operands.push_back(cond);
        operands.push_back(trueVal);
        operands.push_back(falseVal);
        if (cond)
            cond->addUse(this);
        if (trueVal)
            trueVal->addUse(this);
        if (falseVal)
            falseVal->addUse(this);
    }

    std::string toString() const override { return name + " = select " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + ", " + operands[1]->getTypeName() + " " + getOperandValue(operands[1]) + ", " + operands[2]->getTypeName() + " " + getOperandValue(operands[2]); }
};

// GetElementPtr指令
class GetElementPtrInst : public Instruction {
public:
    Type elementType;
    int arraySize;  // 数组大小，0表示未知或非数组

    GetElementPtrInst(Value* ptr, Value* index, const std::string& name) : Instruction(Opcode::GetElementPtr, name, Type::Ptr), elementType(Type::I32), arraySize(0) {
        operands.push_back(ptr);
        operands.push_back(index);
        if (ptr)
            ptr->addUse(this);
        if (index)
            index->addUse(this);
    }

    GetElementPtrInst(Value* ptr, Value* index, const std::string& name, Type elemType, int arrSize) : Instruction(Opcode::GetElementPtr, name, Type::Ptr), elementType(elemType), arraySize(arrSize) {
        operands.push_back(ptr);
        operands.push_back(index);
        if (ptr)
            ptr->addUse(this);
        if (index)
            index->addUse(this);
    }

    std::string toString() const override {
        std::string elemTypeStr = "i32";  // 默认
        switch (elementType) {
        case Type::I32: elemTypeStr = "i32"; break;
        case Type::Float: elemTypeStr = "float"; break;
        case Type::I8: elemTypeStr = "i8"; break;
        case Type::I64: elemTypeStr = "i64"; break;
        default: elemTypeStr = "i32"; break;
        }

        if (arraySize > 0) {
            // 正确的数组GEP格式：%result = getelementptr inbounds [n x type], ptr %array, i32 0, i32 %index
            return name + " = getelementptr inbounds [" + std::to_string(arraySize) + " x " + elemTypeStr + "], " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + ", i32 0, " + operands[1]->getTypeName() + " " + getOperandValue(operands[1]);
        } else {
            // 简化的GEP格式
            return name + " = getelementptr inbounds " + elemTypeStr + ", " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + ", " + operands[1]->getTypeName() + " " + getOperandValue(operands[1]);
        }
    }
};

// 函数调用指令
class CallInst : public Instruction {
public:
    std::string functionName;

    CallInst(const std::string& funcName, const std::vector<Value*>& args, const std::string& name, Type retType) : Instruction(Opcode::Call, name, retType), functionName(funcName) {
        operands = args;
        for (auto arg : args) {
            if (arg)
                arg->addUse(this);
        }
    }

    std::string toString() const override;
};

// Return指令
class ReturnInst : public Instruction {
public:
    ReturnInst(Value* val = nullptr) : Instruction(Opcode::Ret, "", Type::Void) {
        if (val) {
            operands.push_back(val);
            val->addUse(this);
        }
    }

    std::string toString() const override {
        if (operands.empty()) {
            return "ret void";
        } else {
            return "ret " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]);
        }
    }
};

// 分支指令
class BranchInst : public Instruction {
public:
    std::string trueLabel;
    std::string falseLabel;

    // 无条件分支
    BranchInst(const std::string& label) : Instruction(Opcode::Br, "", Type::Void), trueLabel(label) {}

    // 条件分支
    BranchInst(Value* cond, const std::string& trueL, const std::string& falseL) : Instruction(Opcode::Br, "", Type::Void), trueLabel(trueL), falseLabel(falseL) {
        operands.push_back(cond);
        if (cond)
            cond->addUse(this);
    }

    std::string toString() const override;
    std::string getTrueLabel() const { return trueLabel; }
    std::string getFalseLabel() const { return falseLabel; }
    bool isConditional() const { return !trueLabel.empty() && !falseLabel.empty(); }
};

// Undefined Value (用于SSA重命名)
class UndefValue : public Value {
public:
    UndefValue(Type t) : Value("undef", t) {}
    std::string toString() const override { return getTypeName() + " undef"; }
};

// PHI Node 指令
class PhiInst : public Instruction {
public:
    PhiInst(Type type, const std::string& name) : Instruction(Opcode::PHI, name, type) {}
    void addIncoming(Value* val, BasicBlock* from_block) {
        incoming_values.push_back({val, from_block});
        if(val) val->addUse(this);
    }
    void clearIncoming() {
        incoming_values.clear();
    }
    std::string toString() const override;
    void replaceOperand(Value* oldValue, Value* newValue);
    std::vector<std::pair<Value*, BasicBlock*>> incoming_values;
    std::string getName() const { return name; }
};

// 添加Move指令类
class MoveInst : public Instruction {
    Value* temp_verg_value = nullptr;
public:
    MoveInst(Value* src, const std::string& name, Type type, Value* temp_verg_value) 
        : Instruction(Opcode::Move, name, type) {
        operands.push_back(src);
        if (src) src->addUse(this);
        this->temp_verg_value = temp_verg_value;
    }

    Value* get_value_ptr() override { 
        return temp_verg_value ? temp_verg_value : Instruction::get_value_ptr(); 
    }

    std::string toString() const override {
        return name + " = move " + getOperandValue(operands[0]);
    }
};

// LLVM IR构建器
class IRBuilder {
private:
    BasicBlock* currentBlock;
    int tempCounter;

public:
    IRBuilder() : currentBlock(nullptr), tempCounter(0) {}

    void setInsertPoint(BasicBlock* bb) { currentBlock = bb; }

    std::string getNextTempName() { return "%t" + std::to_string(tempCounter++); }

    // 创建算术指令
    BinaryOperator* createAdd(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createFAdd(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createSub(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createFSub(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createMul(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createFMul(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createSDiv(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createFDiv(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createSRem(Value* lhs, Value* rhs, const std::string& name = "");

    // 创建逻辑指令
    BinaryOperator* createAnd(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createOr(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createXor(Value* lhs, Value* rhs, const std::string& name = "");

    // 创建比较指令
    ICmpInst* createICmp(ICmpCond cond, Value* lhs, Value* rhs, const std::string& name = "");
    FCmpInst* createFCmp(FCmpCond cond, Value* lhs, Value* rhs, const std::string& name = "");

    // 创建位运算指令
    BinaryOperator* createShl(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createLShr(Value* lhs, Value* rhs, const std::string& name = "");
    BinaryOperator* createAShr(Value* lhs, Value* rhs, const std::string& name = "");

    // 创建类型转换指令
    TruncInst* createTrunc(Value* val, Type toType, const std::string& name = "");
    ZExtInst* createZExt(Value* val, Type toType, const std::string& name = "");
    SExtInst* createSExt(Value* val, Type toType, const std::string& name = "");
    FPToUIInst* createFPToUI(Value* val, Type toType, const std::string& name = "");
    SIToFPInst* createSIToFP(Value* val, Type toType, const std::string& name = "");
    FPToSIInst* createFPToSI(Value* val, Type toType, const std::string& name = "");
    UIToFPInst* createUIToFP(Value* val, Type toType, const std::string& name = "");
    FPTruncInst* createFPTrunc(Value* val, Type toType, const std::string& name = "");
    FPExtInst* createFPExt(Value* val, Type toType, const std::string& name = "");
    PtrToIntInst* createPtrToInt(Value* val, Type toType, const std::string& name = "");
    IntToPtrInst* createIntToPtr(Value* val, Type toType, const std::string& name = "");
    BitCastInst* createBitCast(Value* val, Type toType, const std::string& name = "");

    // 创建其他指令
    SelectInst* createSelect(Value* cond, Value* trueVal, Value* falseVal, const std::string& name = "");
    PhiInst* createPHI(Type type, const std::string& name = "");

    // 创建内存指令
    AllocaInst* createAlloca(Type type, const std::string& name = "");
    AllocaInst* createAlloca(Type type, int arraySize, const std::string& name = "");
    LoadInst* createLoad(Value* ptr, const std::string& name = "", Type loadType = Type::I32);
    StoreInst* createStore(Value* val, Value* ptr);
    GetElementPtrInst* createGEP(Value* ptr, Value* index, const std::string& name = "");
    GetElementPtrInst* createGEP(Value* ptr, Value* index, const std::string& name, Type elemType, int arraySize);

    // 创建控制流指令
    CallInst* createCall(const std::string& funcName, const std::vector<Value*>& args, const std::string& name = "", Type retType = Type::Void);
    ReturnInst* createRet(Value* val = nullptr);
    BranchInst* createBr(const std::string& label);
    BranchInst* createCondBr(Value* cond, const std::string& trueLabel, const std::string& falseLabel);

private:
    void insertInstruction(std::unique_ptr<Instruction> inst);
};

} // namespace llvm_ir

#endif
