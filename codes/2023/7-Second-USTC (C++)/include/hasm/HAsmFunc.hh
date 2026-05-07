#pragma once

#include <string>

#include "Function.hh"

#include "HAsmModule.hh"
#include "HAsmBlock.hh"
#include "HAsmLoc.hh"

class HAsmModule;
class HAsmBlock;

class HAsmFunc {
public:
    explicit HAsmFunc(HAsmModule *parent, Function *f): parent_(parent), f_(f) {}
    void set_func_header(std::string header) { func_header_ = header; }

    Function *get_function() { return f_; }

    HAsmBlock *create_bb(BasicBlock *bb, Label *label);

    std::string get_asm_code();
    std::string print();

private:
    std::string func_header_;
    std::list<HAsmBlock*> blocks_list_;
    HAsmModule *parent_;
    Function *f_;
};