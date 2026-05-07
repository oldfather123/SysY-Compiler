#include "riscv.hpp"
#include "ir_type.hpp"

int calculate_type_size(Type* type) {
    int total_size = 1; // 元素个数初始化为 1
    while(auto array_type = dynamic_cast<ArrayType*>(type)) {
        total_size *= array_type->num_elements; // 计算数组元素个数
        type = array_type->element_type; // 更新类型为数组元素类型
    }
    if(type->tid == TypeID::IntegerTy || type->tid == TypeID::FloatTy) {
        total_size *= 4; // 整型或浮点型占用 4 字节
    } else if(type->tid == TypeID::PointerTy) {
        total_size *= 8; // 指针类型占用 8 字节
    } else {
        cerr << "[ERROR] Unsupported type for size calculation!\n";
        exit(1);
    }
    return total_size; // 返回总大小
}

Type* get_element_type(Type* type) {
    while(auto pointer_type = dynamic_cast<PointerType*>(type)) {
        type = pointer_type->pointed_type; // 获取指针指向的类型
    }
    while(auto array_type = dynamic_cast<ArrayType*>(type)) {
        type = array_type->element_type; // 获取数组元素类型
    }
    return type; // 返回最终的数值类型
}