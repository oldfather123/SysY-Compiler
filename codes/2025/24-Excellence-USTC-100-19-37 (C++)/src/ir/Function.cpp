#include "Function.hpp"
#include "Module.hpp"
// #define DEBUG
#ifdef DEBUG
    #define _DEBUG_(string)                                                         \
        std::cout << "[SysYBuilder] DEBUG: " << string << std::endl << std::flush;
#else
    #define _DEBUG_(string)
#endif

Function::Function(FunctionType *ty, const std::string &name, Module *parent)
    : Value(ty, name), parent_(parent), seq_cnt_(0) {
    // num_args_ = ty->getNumParams();
    parent->add_function(this);
    // build args
    for (unsigned i = 0; i < get_num_of_args(); i++) {
        arguments_.emplace_back(ty->get_param_type(i), "", this, i);
    }
}
Function *Function::create(FunctionType *ty, const std::string &name,
                           Module *parent) {
    return new Function(ty, name, parent);
}

FunctionType *Function::get_function_type() const {
    return static_cast<FunctionType *>(get_type());
}

Type *Function::get_return_type() const {
    return get_function_type()->get_return_type();
}

unsigned Function::get_num_of_args() const {
    return get_function_type()->get_num_of_args();
}

unsigned Function::get_num_basic_blocks() const { return basic_blocks_.size(); }

Module *Function::get_parent() const { return parent_; }

void Function::remove(BasicBlock *bb) {
    basic_blocks_.remove(bb);
    for (auto pre : bb->get_pre_basic_blocks()) {
        pre->remove_succ_basic_block(bb);
    }
    for (auto succ : bb->get_succ_basic_blocks()) {
        succ->remove_pre_basic_block(bb);
    }
}

void Function::add_basic_block(BasicBlock *bb) { basic_blocks_.push_back(bb); }
