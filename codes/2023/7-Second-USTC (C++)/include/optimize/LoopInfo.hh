#pragma once

#include "Pass.hh"
#include "Module.hh"
#include "LoopSearch.hh"
#include "utils.hh"

#include <unordered_map>
#include <unordered_set>

// using IvType = Loop::InductionVar::IndVarType;

class Loop
{
public:
    Loop(BBLoop &loop, Module *m)
        : bbSet(loop), parentLoop(nullptr), base(nullptr), exit(nullptr), m_(m)
    {}
    BasicBlock * getBase(){ return base;}

    struct InductionVar
    {
        Instruction *inst;
        // 归纳变量类型：基本归纳变量、依赖归纳变量
        enum class IndVarType { basic, dependent} type;
        enum class Direction { increase, decrease, unknown} direction;
        enum class UpdateOp { add, sub} updateOp;
        
        Value *initVal;
        Value *finalVal;
        Value *stepVal;

        // 当该归纳变量为依赖归纳变量时，basis表示其基，coef表示系数，cons标识常数
        // 即 iv = coef*basis + cons
        InductionVar *basis; 
        Value *coef;
        Value *cons;

        std::string print_direction() 
        {
            switch(direction) 
            {
                case Direction::increase: return "increase"; break;
                case Direction::decrease: return "decrease"; break;
                case Direction::unknown: return "unknown"; break;
                default: return ""; break; 
            }
        }
        std::string print_final_val()
        {
            if(finalVal == nullptr)
                return "未知";
            else
                return finalVal->print();
        }

        bool is_dependent_iv() { return type == IndVarType::dependent; }
        InductionVar(Instruction * i, IndVarType t) 
            : inst(i), type(t) {}
    };
    
    // 目前只处理只有一个基础归纳变量的循环
    InductionVar* get_basic_iduction_var();
    // 获取更新IndVar的指令，即 i = i+1 这样的指令
    Instruction *get_update_inst(Instruction *inst);
    Instruction *get_update_inst(Value *var){return get_update_inst(dynamic_cast<Instruction*>(var));}
    void set_loop_base(BasicBlock *base_){ base = base_;}
    BasicBlock *get_loop_base() { return base; }
    BasicBlock *get_preheader() { return preHeader; }
    BasicBlock *get_end_block() { return endBlock; }
    Instruction *get_terminal_of_base();
    std::unordered_set<InductionVar *>& get_ind_vars() { return indVars;};
    void find_break();
    void find_preheader_and_end();
    void find_basic_iduction_var();
    void find_dependent_iduction_var();
    bool has_break() { return hasBreak; }
    bool contains(BasicBlock *bb) {return bbSet.find(bb) != bbSet.end();}
    bool contains(Instruction *inst) {return contains(inst->get_parent());}
    bool contains(Value *val) 
    {
        auto inst = dynamic_cast<Instruction*>(val);
        auto block = dynamic_cast<BasicBlock*>(val);
        if(block)
            return contains(block);
        else if(!inst) 
            return false;
        else 
            return contains(inst);
    }
    
    // 在base块的phi指令之后插入指令
    void insert_at_begin(Instruction *inst) 
    {
        for (auto iter = base->get_instructions().begin(); iter != base->get_instructions().end(); iter++) 
        {
            if (!(*iter)->is_phi()) 
            {
                base->get_instructions().insert(iter, inst);
                inst->set_parent(base);
                return;
            }
        }
    }
    void insert_at_begin(Value *value)
    {
        auto inst = dynamic_cast<Instruction *>(value);
        if(!inst)
            return;
        else
            insert_at_begin(inst);
    }

    void debug_print_basic_iv();
private:
    bool hasBreak = false;
    bool hasContinue = false;
    BBLoop bbSet;
    Loop *parentLoop;
    BasicBlock *base;
    BasicBlock *exit;
    BasicBlock *endBlock;
    BasicBlock *preHeader;
    BasicBlock *latch;
    Module *m_;

    std::unordered_set<BasicBlock *> latchBlocks;
    std::unordered_set<InductionVar *> indVars;
    std::unordered_map<Instruction *, InductionVar *> inst2Iv;

    bool is_two_prev_phi(Instruction *inst) { return (inst->is_phi() && inst->get_num_operands() == 4);}
    bool check_phis_prev(Instruction *inst);
    bool check_update_inst_type(Instruction *inst);
    bool is_loop_invariant(Instruction *inst);
    bool is_loop_invariant(Value *value){return is_loop_invariant(dynamic_cast<Instruction *>(value));}
    bool ops_is_invariant(Instruction *inst);
    bool is_update_inst(Instruction *iv, Instruction *inst);
    InductionVar* iv_exists(Instruction *inst);
    InductionVar* iv_exists(Value *val);
    void find_latch();
    Instruction *get_terminal_of_bb(BasicBlock *bb);
    bool bound_inst_is_br(Instruction *inst);
    bool check_bound_inst_and_set_ind_val(Instruction *inst, InductionVar *newIndVal);
    bool check_bound_inst(Instruction *inst);
    void set_final_val(Value *boundVal, InductionVar *newIndVal);
    void set_direction(InductionVar *newIndVal);
    void set_step_val(InductionVar *newIndVal);
    void set_init_val(InductionVar *newIndVal);
    void set_update_op(InductionVar *newIndVal);
    bool is_break(Instruction *inst);
};


class LoopInfo : public Pass
{
public:
    LoopInfo(Module *module, bool print_ir=false) : Pass(module, print_ir) {}
    void execute() final;
    const std::string get_name() const override {return name;}

    std::vector<Loop *> allLoops; 
    

private:
    std::string name = "LoopInfo";
    
};

