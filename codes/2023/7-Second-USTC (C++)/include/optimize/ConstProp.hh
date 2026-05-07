#pragma once

#include "Module.hh"
#include "Pass.hh"
#include "Constant.hh"
#include "logging.hh"
#include <vector>
#include <map>
#include <set>

class ConstProp : public Pass
{
private:
    Module *module_;
    std::string name = "ConstProp";

public:
    explicit ConstProp(Module* module, bool print_ir=false): Pass(module, print_ir){module_ = module;}
    ~ConstProp(){};
    void execute() final;
    const std::string get_name() const override {return name;}
    ConstantInt *resCalc(Instruction::OpID op, ConstantInt *value1, ConstantInt *value2);
    ConstantFP *resCalc(Instruction::OpID op, ConstantFP *value1, ConstantFP *value2);
    ConstantInt *resCalc(CmpOp cmpOp, ConstantInt *value1, ConstantInt *value2);
    ConstantInt *resCalc(CmpOp cmpOp, ConstantFP *value1, ConstantFP *value2);
    static ConstantInt *cast_to_const_int(Value *value);
    static ConstantFP *cast_to_const_float(Value *value);
    void ConstFold(BasicBlock *block);
    void DeadBlockEli(Function *fun);
    void BlockWalk(Function *fun,BasicBlock *block);
    void RemoveSinglePhi(Function *fun);

    //数组常量传播
    // std::map<std::pair<Value*, Value*>, Value*> array_val_map;
    std::set<BasicBlock *> deadBlock;
    //全局变量常量传播
    std::set<Value*> unchangedGlobalVal;
    void collect_unchanged_global_val();
};
