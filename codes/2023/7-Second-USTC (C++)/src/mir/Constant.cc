#include <iostream>
#include <memory>
#include <sstream>

#include "Constant.hh"
#include "Module.hh"

//& ConstantInt
ConstantInt *ConstantInt::get(int val, Module *m) {
    if (m->cached_int.find(std::make_pair(val, m)) != m->cached_int.end())
        return m->cached_int[std::make_pair(val, m)].get();
    return (m->cached_int[std::make_pair(val, m)] =
                std::unique_ptr<ConstantInt>(new ConstantInt(Type::get_int32_type(m), val)))
        .get();
}

ConstantInt *ConstantInt::get(bool val, Module *m) {
    if (m->cached_bool.find(std::make_pair(val, m)) != m->cached_bool.end())
        return m->cached_bool[std::make_pair(val, m)].get();
    return (m->cached_bool[std::make_pair(val, m)] =
                std::unique_ptr<ConstantInt>(new ConstantInt(Type::get_int1_type(m), val ? 1 : 0)))
        .get();
}


std::string ConstantInt::print() {
    std::string const_ir;
    Type *ty = this->get_type();
    if (ty->is_integer_type() && static_cast<IntegerType *>(ty)->get_num_bits() == 1) {
        const_ir += (this->get_value() == 0) ? "false" : "true";  //&  int1
    } else {
        const_ir += std::to_string(this->get_value());  //& int32
    }
    return const_ir;
}

//& ConstantFP
ConstantFP *ConstantFP::get(float val, Module *m) {
    if (m->cached_float.find(std::make_pair(val, m)) != m->cached_float.end())
        return m->cached_float[std::make_pair(val, m)].get();
    return (m->cached_float[std::make_pair(val, m)] =
                std::unique_ptr<ConstantFP>(new ConstantFP(Type::get_float_type(m), val)))
        .get();
}

std::string ConstantFP::print() {
    std::stringstream fp_ir_ss;
    std::string fp_ir;
    double val = this->get_value();
    fp_ir_ss << "0x" << std::hex << *(uint64_t *)&val << std::endl;
    fp_ir_ss >> fp_ir;
    return fp_ir;
}

//& ConstantZero
ConstantZero *ConstantZero::get(Type *ty, Module *m) {
    if (not m->cached_zero[ty])
        m->cached_zero[ty] = std::unique_ptr<ConstantZero>(new ConstantZero(ty));
    return m->cached_zero[ty].get();
}

std::string ConstantZero::print() { 
    return "zeroinitializer"; 
}

//& ConstantArray
ConstantArray::ConstantArray(ArrayType *ty, const std::vector<Constant *> &val) : Constant(ty, "", val.size()) {
    for (int i = 0; i < val.size(); i++)
        set_operand(i, val[i]);
    this->const_array_.assign(val.begin(), val.end());
}

ConstantArray *ConstantArray::get(ArrayType *ty, const std::vector<Constant *> &val) {
    return new ConstantArray(ty, val);
}

std::string ConstantArray::print() {
    std::string const_ir;
    const_ir += "[";
    const_ir += this->get_type()->get_array_element_type()->print();
    const_ir += " ";
    const_ir += get_element_value(0)->print();
    for ( int i = 1 ; i < this->get_size_of_array() ; i++ ){
        const_ir += ", ";
        const_ir += this->get_type()->get_array_element_type()->print();
        const_ir += " ";
        const_ir += get_element_value(i)->print();
    }
    const_ir += "]";
    return const_ir;
}
