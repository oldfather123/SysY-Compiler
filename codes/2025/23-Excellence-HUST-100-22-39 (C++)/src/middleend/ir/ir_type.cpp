#include "ir_type.hpp"

Type::Type(TypeID tid)
    : tid(tid) {}

IntegerType::IntegerType(int num_bits)
    : Type(TypeID::IntegerTy), num_bits(num_bits) {}

ArrayType::ArrayType(Type* element_type, int num_elements)
    : Type(TypeID::ArrayTy), element_type(element_type), num_elements(num_elements) {}

PointerType::PointerType(Type* pointed_type)
    : Type(TypeID::PointerTy), pointed_type(pointed_type) {}

FunctionType::FunctionType(Type* ret_type, vector<Type*> param_types)
    : Type(TypeID::FunctionTy), ret_type(ret_type) {
    for(auto param_type: param_types) {
        if(!param_type) {
            cerr << "[ERROR] FunctionType: parameter type cannot be null!" << endl;
            exit(1);
        }
        this->param_types.push_back(param_type);
    }
}