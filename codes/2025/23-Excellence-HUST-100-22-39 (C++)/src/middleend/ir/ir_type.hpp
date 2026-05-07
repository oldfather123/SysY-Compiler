#ifndef __IR_TYPE_HPP__
#define __IR_TYPE_HPP__

#include <string>
#include <vector>
#include "ir.hpp"

// SysY2022 语言数据类型：空类型、整型、浮点型、数组类型、指针类型、标签类型（基本块）、函数类型
// 由于在中间代码中，基本块和函数也可以作为指令的操作数，所以也定义为一种数据类型
enum TypeID { VoidTy, IntegerTy, FloatTy, ArrayTy, PointerTy, LabelTy, FunctionTy};

/// @brief IR 数据类型基类
/// @note 储存数据类型 tid
class Type {
public:
    TypeID tid;
    explicit Type(TypeID id);
    virtual ~Type() = default;
    void print(ostream& out);
};

/// @brief 整型
/// @note 区分位数，储存 num_bits，分为 i32 和 i1
class IntegerType: public Type {
public:
    int num_bits; // 位数
    explicit IntegerType(int num_bits);
};

/// @brief 数组类型
/// @note 储存元素类型 element_type 和元素个数 num_elements
class ArrayType: public Type {
public:
    Type* element_type; // 元素类型
    int num_elements;   // 元素个数
    explicit ArrayType(Type* element_type, int num_elements);
};

/// @brief 指针类型
/// @note 储存指针指向的类型 pointed_type
class PointerType: public Type {
public:
    Type* pointed_type; // 指针指向的类型
    explicit PointerType(Type* pointed_type);
};

/// @brief 函数类型
/// @note 储存函数返回类型 ret_type 和参数类型数组 param_types
class FunctionType: public Type {
public:
    Type* ret_type;             // 函数返回类型
    vector<Type*> param_types;  // 函数参数类型数组
    explicit FunctionType(Type* ret_type, vector<Type*> param_types);
};

#endif // __IR_TYPE_HPP__