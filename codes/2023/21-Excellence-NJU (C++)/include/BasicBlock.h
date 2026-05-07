//
// Created by q1w2e3r4 on 23-7-9.
//
#include "Type.h"
#include "codegen/MachineBlock.hpp"
#include "ValueRef.h"
#include <set>
#include <vector>
#include <map>
#include <iostream>

#ifndef COMPILER_BASICBLOCK_H
#define COMPILER_BASICBLOCK_H

class Instruction;
class Function;
class DomNode;
class AsmBuilder;
class Loop;

class BasicBlock : public ValueRef{
public:
    std::string label;
    std::vector<BasicBlock *> pred,succ;
    std::vector<Instruction*> local_instr;

    Function * func_belong;
    DomNode * domNode;

    std::set<ValueRef*> live_in;
    std::set<ValueRef*> live_out;

    std::set<ValueRef*> live_use;
    std::set<ValueRef*> live_def;

    Loop* belong_loop = nullptr;
    int loop_depth = 0;

    MachineBlock *mBlock;

    BasicBlock(std::string& label,Function * func);

    void appendCode(Instruction* instruction);

    void insertCodeAtFront(Instruction* instruction);

    void Emit(std::ostream &os);

    std::string get_Ref() override { return label; }
    std::string get_TypeName() override { return "label"; }

    void codegen(AsmBuilder *builder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int framesize,
                 vector<Type*> &arguments,
                 bool is_entry = false);
};


#endif //COMPILER_BASICBLOCK_H
