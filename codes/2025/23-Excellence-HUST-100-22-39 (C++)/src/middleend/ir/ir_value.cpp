#include <sstream>
#include <cstdint>
#include "ir_type.hpp"
#include "ir_value.hpp"
#include "ir_instruction.hpp"
#include "ir_module.hpp"

Use::Use(Value* val, int index)
    : val(val), index(index) {}

Value::Value(Type* type, const string& name)
    : type(type), name(name) {}

bool Value::is_const() {
    return name.empty();
}

list<Use>::iterator Value::add_use(Value* val, int index) {
    use_list.emplace_back(val, index);
    return --use_list.end();
}

void Value::remove_use(list<Use>::iterator it) {
    use_list.erase(it);
}

void Value::replace_use_with(Value* new_val) {
    for(auto use: use_list) {
        auto inst = dynamic_cast<Instruction*>(use.val);
        inst->set_op(use.index, new_val);
    }
}

ValUndef::ValUndef(Type* type)
    : Value(type, "undef") {};

Const::Const(Type* type)
    : Value(type, "") {}

ConstInt::ConstInt(IntegerType* type, int val)
    : Const(type), val(val) {}

ConstFloat::ConstFloat(Type* type, float val)
    : Const(type), val(val) {}

ConstArray::ConstArray(ArrayType* type, const vector<Const*>& const_array)
    : Const(type) {
    this->const_array.assign(const_array.begin(), const_array.end());
}

ConstZero::ConstZero(Type* type)
    : Const(type) {}

GlobalVariable::GlobalVariable(string& name, Module* module, Type* type, bool is_const, Const* init_val)
    : Value(module->get_pointer_type(type), name), is_const(is_const), init_val(init_val) {
    module->add_global_variable(this);
}

void ConstFloat::print(ostream& out) {
    out << "0x" << hex << *reinterpret_cast<uint32_t*>(&val);
}