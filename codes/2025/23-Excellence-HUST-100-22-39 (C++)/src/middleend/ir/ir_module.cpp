#include "ir_module.hpp"
#include "ir_type.hpp"
#include "ir_value.hpp"
#include "ir_function.hpp"

Module::Module() {
    // 初始化基本类型
    void_type = new Type(TypeID::VoidTy);
    int1_type = new IntegerType(1);
    int32_type = new IntegerType(32);
    float_type = new Type(TypeID::FloatTy);
    label_type = new Type(TypeID::LabelTy);
}

Module::~Module() {
    delete void_type;
    delete int1_type;
    delete int32_type;
    delete float_type;
    delete label_type;
}

void Module::add_global_variable(GlobalVariable* global_var) {
    global_var_list.push_back(global_var);
}

void Module::add_function(Function* function) {
    func_list.push_back(function);
}

PointerType* Module::get_pointer_type(Type* pointed_type) {
    if(!pointer_type.count(pointed_type)) {
        pointer_type[pointed_type] = new PointerType(pointed_type);
    }
    return pointer_type[pointed_type];
}

ArrayType* Module::get_array_type(Type* element_type, int num_elements) {
    pair<Type*, int> key = make_pair(element_type, num_elements);
    if(!array_type.count(key)) {
        array_type[key] = new ArrayType(element_type, num_elements);
    }
    return array_type[key];
}