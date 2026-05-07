#pragma once

#include "Module.hh"
#include "User.hh"
#include "Constant.hh"

class GlobalVariable : public User {
public:
    virtual ~GlobalVariable() = default;

    static GlobalVariable *create(std::string name, Module *m, Type *ty, bool is_const, Constant *init);

    Constant *get_init() { return init_val_; }
    bool is_const() { return is_const_; }

    bool is_zero_initializer() { return dynamic_cast<ConstantZero*>(init_val_) != nullptr; }
    
    std::string print();

private:
    GlobalVariable(std::string name, Module *m, Type *ty, bool is_const, Constant *init = nullptr);

private:
    bool is_const_;
    Constant *init_val_;
};


