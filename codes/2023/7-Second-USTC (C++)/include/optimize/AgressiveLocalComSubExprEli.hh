#pragma once

#include "Pass.hh"
#include <map>
#include <set>
#include <tuple>
#include "Instruction.hh"
#include "LocalComSubExprEli.hh"

class AgressiveLocalComSubExprEli : public Pass{
private:
    Module *module_;
    std::string name = "AgressiveLocalComSubExprEli";
    std::map<BasicBlock*, std::map<std::tuple<Value*, Value*, Instruction::OpID>, Value*>> AEB;
    std::map<BasicBlock* ,std::map<std::tuple<Value*, int, Instruction::OpID>, Value*>> AEB_riconst;
    std::map<BasicBlock* ,std::map<std::tuple<int, Value*, Instruction::OpID>, Value*>> AEB_liconst;
    std::map<BasicBlock* ,std::map<std::tuple<Value*, float, Instruction::OpID>, Value*>> AEB_rfconst;
    std::map<BasicBlock* ,std::map<std::tuple<float, Value*, Instruction::OpID>, Value*>> AEB_lfconst;
    std::map<BasicBlock* ,std::map<std::tuple<Value *,Value*>, Value *>> gep_2op;
    std::map<BasicBlock* ,std::map<std::tuple<Value *,int>, Value *>> gep_2op_const;
    std::map<BasicBlock* ,std::map<std::tuple<Value *,Value*>, Value *>> gep_3op;
    std::map<BasicBlock* ,std::map<std::tuple<Value *,int>, Value *>> gep_3op_const;
    std::map<BasicBlock* ,std::map<Value *, Value*>> load_val;
    std::map<BasicBlock* ,std::map<std::tuple<Value *,int, Instruction::OpID>, Value *>> load_off_val_const;
    std::map<BasicBlock* ,std::map<std::tuple<Value *,Value*, Instruction::OpID>, Value *>> load_off_val;
    std::map<BasicBlock* ,std::map<std::pair<std::string, Arglist>, Value *>> call_val;

    std::set<BasicBlock *> visited;

    Function *func_;
    std::set<std::string> eliminatable_call;

public:
    explicit AgressiveLocalComSubExprEli(Module *module, bool print_ir=false) : Pass(module, print_ir) { module_ = module; }
    void execute() final;
    const std::string get_name() const override {return name;}
    ~AgressiveLocalComSubExprEli(){};

    void delete_expr_local(BasicBlock* bb);
    void get_eliminatable_call();
    bool is_call_eliminatable(std::string func_id);
    Arglist *get_call_args(CallInst *instr);
};