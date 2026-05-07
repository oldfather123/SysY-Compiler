#include "GlobalVariable.hpp"
#include "Module.hpp"

GlobalVariable::GlobalVariable(std::string name, Module *m, Type *ty,
                               bool is_const, Constant *init)
    : User(ty, name), is_const_(is_const), init_val_(init) {
    m->add_global_variable(this);
    if (init) {
        this->add_operand(init);
    }
} // global操作数为initval

GlobalVariable *GlobalVariable::create(std::string name, Module *m, Type *ty,
                                       bool is_const,
                                       Constant *init = nullptr) {
    return new GlobalVariable(name, m, PointerType::get(ty), is_const, init);
}