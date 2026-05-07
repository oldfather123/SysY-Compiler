#pragma once

#include "Pass.hh"
#include "Module.hh"

class DeadCodeEliWithBr : public Pass
{
public:
    DeadCodeEliWithBr(Module *module, bool print_ir=false) : Pass(module, print_ir) {}
    void execute() final;
    const std::string get_name() const override {return name;}
    
private:
    Function *func_;
    Function *main_func;
    const std::string name = "DeadCodeEliWithBr";
    bool is_critical_inst(Instruction *inst);
    void mark();
    void sweep();
    void find_func();
    void delete_unused_func();
    void delete_unreachable_bb();
    void delete_unused_control();
    BasicBlock * get_irdom(BasicBlock *bb);
    std::map<Instruction *, bool> inst_mark;    //& 存储Instruction是否被标记
    std::map<BasicBlock *, bool> bb_mark;       //& 存储block是否被标记
    std::map<Function *, bool> func_mark;       //& 1. 用来存储被引用到的函数 2.被引用的函数有没有用
    std::list<Function *> func_list;            //& 被调用的函数列表

};
