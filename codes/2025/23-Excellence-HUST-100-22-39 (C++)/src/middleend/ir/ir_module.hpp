#ifndef __IR_MODULE_HPP__
#define __IR_MODULE_HPP__

#include <vector>
#include <map>
#include <string>
#include "ir.hpp"

class Module {
public:
    vector<GlobalVariable*> global_var_list;    // 全局变量列表
    vector<Function*> func_list;                // 函数列表
    Type* void_type;            // void
    IntegerType* int1_type;     // i1
    IntegerType* int32_type;    // i32
    Type* float_type;           // float
    Type* label_type;           // label
    map<Type*, PointerType*> pointer_type;        // 缓存的指针类型
    map<pair<Type*, int>, ArrayType*> array_type; // 缓存的数组类型

    /// @brief 构造函数
    /// @note 初始化基本类型：void_type、int1_type、int32_type、float_type、label_type
    explicit Module();
    void print(ostream& out);

    /// @brief 析构函数
    /// @note 释放基础类型
    ~Module();

    /// @brief 添加全局变量
    /// @param global_var 待添加的全局变量
    void add_global_variable(GlobalVariable* global_var);

    /// @brief 添加函数
    /// @param function 待添加的函数
    void add_function(Function* function);

    /// @brief 获取指针类型
    /// @param pointed_type 指向的类型
    /// @return 指针类型
    PointerType* get_pointer_type(Type* pointed_type);

    /// @brief 获取数组类型
    /// @param element_type 数组元素类型
    /// @param num_elements 数组元素个数
    /// @return 数组类型
    ArrayType* get_array_type(Type* element_type, int num_elements);
};

#endif // __IR_MODULE_HPP__
