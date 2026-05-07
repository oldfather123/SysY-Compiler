#pragma once

#include "Type.hpp"
#include "User.hpp"
#include "ilist.hpp"
#include "Instruction.hpp"
#include <cstdint>
#include "Module.hpp"
#include <unordered_map>
#include <vector>

using entry_ret=std::tuple<BasicBlock*,BasicBlock*,Value*>;

Instruction *clone_inst(Instruction *inst, BasicBlock *bb,std::unordered_map<Value*, Value*>map);
entry_ret clone_func(Module *m,Function *origin_func,Function *target_func,std::unordered_map<Value *, Value *> &map);
void replace_operand(Instruction *inst, unsigned i, Value *new_operand);
void split_basic_block_into_three(BasicBlock *orig_bb, Module *m);
BasicBlock *insert_before_block(BasicBlock *orig_bb, Module *m);
void move_instrs(BasicBlock *insert_block,BasicBlock *inst_block,
    std::vector<Instruction *> to_move,Instruction *insert_before_inst);