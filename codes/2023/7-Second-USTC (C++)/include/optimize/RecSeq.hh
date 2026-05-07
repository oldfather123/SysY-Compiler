#pragma once

#include "Module.hh"
#include "Pass.hh"
#include "Constant.hh"
#include "logging.hh"
#include "IRBuilder.hh"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <map>
#include <set>

class RecurFun
{
public:
    Function *func;
    std::vector<Value *> params;
    Value *recParam = nullptr;
    Value *slaveParam = nullptr;
    int recParamIdx;
    int slaveParamIdx;
    Value *endVal;
    Instruction *updateInst;
    CmpInst *condInst;
    BranchInst *brInst;
    BasicBlock *bodyEntry;
    BasicBlock *bodyEntryPre;
    BasicBlock *retBlock;
    std::vector<Instruction *> bodyInsts;
    std::vector<Instruction *> recInsts;

    RecurFun(){};
};

class RecSeq : public Pass
{
private:
    Module *module_;
    std::string name = "RecSeq";

public:
    explicit RecSeq(Module* module, bool print_ir=false): Pass(module, print_ir){module_ = module;}
    ~RecSeq(){};
    void execute() final;
    const std::string get_name() const override {return name;}

    bool findRecFun();
    Instruction *locate_rec_fun_call();
    void prepare();
    void addRecord();

    Instruction *recFunCall;
    std::vector<RecurFun*> recurFuns;
    RecurFun *recFun;
    CallInst *recCallInst;

    Value *slaveInitVal;
    int i_slaveInitVal = 0;
    float f_slaveInitVal = 0;


};
