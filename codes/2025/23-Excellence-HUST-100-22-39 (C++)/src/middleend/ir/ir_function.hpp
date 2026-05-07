#ifndef __IR_FUNCTION_HPP__
#define __IR_FUNCTION_HPP__

#include <string>
#include "ir.hpp"
#include "ir_value.hpp"

class Argument: public Value {
public:
    Function* parent; // 所属函数
    int index;        // 参数在函数参数列表中的索引

    /// @brief 构造函数
    /// @param type 参数类型
    /// @param name 参数名称
    /// @param parent 所属函数
    /// @param index 参数在函数参数列表中的索引
    explicit Argument(Type* type, const string& name = "", Function* parent = nullptr, int index = 0);
    void print(ostream& out) override;
};

class Function: public Value {
public:
    Module* parent;                 // 所属模块
    vector<Argument*> args;         // 函数参数列表
    vector<BasicBlock*> bb_list;    // 基本块列表
    int ssa_id;

    /// @brief 构造函数
    /// @note 储存函数类型 type、函数名称 name 和所属模块 parent
    /// @param type 函数类型
    /// @param name 函数名称
    /// @param parent 所属模块
    Function(FunctionType* type, const string& name, Module* parent);

    /// @brief 添加基本块到该函数的基本块列表
    /// @param bb 待添加的基本块
    void add_bb(BasicBlock* bb);

    /// @brief 从该函数的基本块列表中移除基本块
    /// @param bb 待移除的基本块
    void remove_bb(BasicBlock* bb);

    /// @brief 获取入口基本块
    /// @return 入口基本块
    BasicBlock* get_entry_bb();

    /// @brief 获取返回基本块
    /// @return 返回基本块
    BasicBlock* get_ret_bb();

    /// @brief 获取返回值类型
    /// @return 函数的返回值类型
    Type* get_ret_type();

    // print IR for debug
    void set_ssa_id();
    void print(ostream& out) override;
};

#endif // __IR_FUNCTION_HPP__
