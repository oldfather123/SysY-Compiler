#pragma once

#include <list>
#include <iterator>
#include <set>

#include "Value.hh"
#include "Instruction.hh"
#include "Module.hh"
#include "Function.hh"

class Function;
class Instruction;
class Module;

class BasicBlock : public Value {
public:
    static BasicBlock *create(Module *m, const std::string &name, Function *parent);

    //// return parent or null(if none)
    Function *get_parent() { return parent_; }

    Module *get_module();

    //& cfg api begin
    std::list<BasicBlock *>& get_pre_basic_blocks() { return pre_bbs_; }
    std::list<BasicBlock *>& get_succ_basic_blocks() { return succ_bbs_; } 

    void add_pre_basic_block(BasicBlock *bb) { pre_bbs_.push_back(bb);}
    void add_succ_basic_block(BasicBlock *bb) { succ_bbs_.push_back(bb);}

    void remove_pre_basic_block(BasicBlock *bb) { pre_bbs_.remove(bb); }
    void remove_succ_basic_block(BasicBlock *bb) { succ_bbs_.remove(bb); }
    void incoming_reset() { incoming_branch = 0; }
    bool is_incoming_zero() { return incoming_branch == 0; }
    void incoming_add(int num) { incoming_branch += num; }
    void incoming_decrement(){incoming_branch--;}
    int get_incoming_branch() { return incoming_branch; }
    void loop_depth_reset() { loop_depth = 0; }
    int get_loop_depth() { return loop_depth; }
    void loop_depth_add(int num) { loop_depth += num; }
    //& cfg api end

    //& dominate tree api begin
    void set_live_in_int(std::set<Value*> in){ilive_in = in;}
    void set_live_out_int(std::set<Value*> out){ilive_out = out;}
    void set_live_in_float(std::set<Value*> in){flive_in = in;}
    void set_live_out_float(std::set<Value*> out){flive_out = out;}
    std::set<Value*>& get_live_in_int(){return ilive_in;}
    std::set<Value*>& get_live_out_int(){return ilive_out;}
    std::set<Value*>& get_live_in_float(){return flive_in;}
    std::set<Value*>& get_live_out_float(){return flive_out;}
    //& dominate tree api end

    //& dominates frontier api begin
    void set_idom(BasicBlock* bb){idom_ = bb;}
    BasicBlock* get_idom(){return idom_;}
    void add_dom_frontier(BasicBlock* bb){dom_frontier_.insert(bb);}
    void add_rdom_frontier(BasicBlock* bb){rdom_frontier_.insert(bb);}
    void clear_rdom_frontier(){rdom_frontier_.clear();}
    std::set<BasicBlock *> &get_dom_frontier(){return dom_frontier_;}
    std::set<BasicBlock *> &get_rdom_frontier(){return rdom_frontier_;}
    std::set<BasicBlock *> &get_rdoms(){return rdoms_;}
    auto add_rdom(BasicBlock* bb){return rdoms_.insert(bb);}
    void clear_rdom(){rdoms_.clear();}
    //& dominates frontier api end

    //// Returns the terminator instruction if the block is well formed or null
    //// if the block is not well formed.
    const Instruction *get_terminator() const;
    Instruction *get_terminator() {
      return const_cast<Instruction *>(
          static_cast<const BasicBlock *>(this)->get_terminator()
      );
    }

    std::list<Instruction*>::iterator begin() { return instr_list_.begin(); }
    std::list<Instruction*>::iterator end() { return instr_list_.end(); }
    std::list<Instruction*>::iterator get_terminator_itr() {
        auto itr = end();
        itr--;
        return itr;
    }

    void set_instructions_list(std::list<Instruction*> &insts_list) {
        instr_list_.clear();
        instr_list_.assign(insts_list.begin(), insts_list.end());
    }
  
    //// add instruction
    void add_instruction(Instruction *instr);
    void add_instruction(std::list<Instruction *>::iterator instr_pos, Instruction *instr);
    void add_instr_begin(Instruction *instr);

    void delete_instr(Instruction *instr);

    std::list<Instruction *>::iterator find_instruction(Instruction *instr);

    bool empty() { return instr_list_.empty(); }
    int get_num_of_instrs() { return instr_list_.size(); }
    std::list<Instruction *>& get_instructions() { return instr_list_; }

    void erase_from_parent();

    void dom_frontier_reset() {
        dom_frontier_.clear();
    }

    virtual std::string print() override;


private:
    explicit BasicBlock(Module *m, const std::string &name, Function *parent);
    
private:  
    std::list<BasicBlock *> pre_bbs_;
    std::list<BasicBlock *> succ_bbs_;
    std::list<Instruction *> instr_list_;
    BasicBlock* idom_ = nullptr;
    std::set<BasicBlock *> dom_frontier_;   //& dom_tree
    std::set<BasicBlock*> rdom_frontier_;
    std::set<BasicBlock*> rdoms_;
    Function *parent_;
    std::set<Value*> ilive_in;
    std::set<Value*> ilive_out;
    std::set<Value*> flive_in;
    std::set<Value*> flive_out;
    int incoming_branch = 0;
    int loop_depth = 0;
};

