#ifndef __IR_VALUE_HPP__
#define __IR_VALUE_HPP__

#include <list>
#include <vector>
#include "ir.hpp"

class Use {
public:
    Value* val; // 该 Use 的所属者 Value
    int index;  // 在 Value 的 operands 中的索引

    /// @brief 构造函数
    /// @param val 该 Use 的所属者 Value
    /// @param index 在 Value 的 operands 中的索引
    Use(Value* val, int index);
};

/// @brief IR 数据基类
/// @note 包含数据类型 type、名称 name 和所有 Use 列表 use_list
class Value {
public:
    Type* type;         // Value 的数据类型
    string name;        // Value 的名称
    list<Use> use_list; // 该 Value 的所有 Use 列表

    /// @brief 构造函数
    /// @param type 数据类型
    /// @param name 数据名称
    explicit Value(Type* type, const string& name);
    virtual ~Value() = default;
    virtual void print(ostream& out) = 0;

    /// @brief 是否为常量
    /// @note 如果 name 为空，则认为是常量
    bool is_const();

    /// @brief 添加对该 Value 的 Use
    /// @param val 使用当前 Value 的 Instruction
    /// @param index 当前 Value 在该 Instruction 的操作数列表中的索引
    /// @return 指向新添加 Use 的迭代器
    list<Use>::iterator add_use(Value* val, int index);

    /// @brief 移除迭代器对应的 Use
    /// @param it 指向要移除的 Use 的迭代器
    void remove_use(list<Use>::iterator it);

    /// @brief 替换对该 Value 的引用为新 Value
    /// @param new_val 新的 Value
    void replace_use_with(Value* new_val);
};

/// @brief 未定义 Value
/// @note 用于 Phi 指令中尚未定义的值，名称为 undef
class ValUndef: public Value {
public:
    /// @brief 构造函数
    /// @param type 数据类型
    explicit ValUndef(Type* type);
    void print(ostream& out) override;
};

/// @brief 常量基类
/// @note 常量名称为空
class Const: public Value {
public:
    /// @brief 构造函数
    /// @param type 数据类型
    /// @note 常量的名称为空字符串
    Const(Type* type);
};

/// @brief 整数常量
/// @note 储存具体整数值 val
class ConstInt: public Const {
public:
    int val; // 整数值

    /// @brief 构造函数
    /// @param type 整数类型
    /// @param val 整数值
    ConstInt(IntegerType* type, int val);
    void print(ostream& out) override;
};

/// @brief 浮点常量
/// @note 储存具体浮点值 val
class ConstFloat: public Const {
public:
    float val; // 浮点数值

    /// @brief 构造函数
    /// @param type 浮点类型
    /// @param val 浮点数值
    ConstFloat(Type* type, float val);

    /// @brief 打印浮点常量
    /// @return 浮点常量的字符串表示
    void print(ostream& out) override;
};

/// @brief 数组常量
/// @note 储存数组元素列表 const_array
class ConstArray: public Const {
public:
    vector<Const*> const_array; // 数组元素列表

    /// @brief 构造函数
    /// @param type 数组类型
    /// @param const_array 数组元素列表
    ConstArray(ArrayType* type, const vector<Const*>& const_array);
    void print(ostream& out) override;
};

/// @brief 零常量
/// @note 用于初始化全局变量和局部变量的默认值
class ConstZero: public Const {
public:
    /// @brief 构造函数
    /// @param type 数据类型
    /// @note 零常量的值为 0
    explicit ConstZero(Type* type);
    void print(ostream& out) override;
};

/// @brief 全局变量
/// @note [1] 包含是否为常量标志 is_const 和初始化值 init_val
/// @note [2] 全局变量的类型为指针类型，指向其实际类型
class GlobalVariable: public Value {
public:
    bool is_const;      // 是否为常量
    Const* init_val;    // 初始化值，可能为 nullptr

    /// @brief 构造函数
    /// @param name 全局变量名称
    /// @param module 所属模块
    /// @param type 全局变量类型
    /// @param is_const 是否为常量
    /// @param init_val 初始化值，默认为 nullptr
    /// @note 全局变量的类型为指针类型，指向其实际类型
    GlobalVariable(string& name, Module* module, Type* type, bool is_const, Const* init_val = nullptr);
    void print(ostream& out) override;
};

#endif // __IR_VALUE_HPP__