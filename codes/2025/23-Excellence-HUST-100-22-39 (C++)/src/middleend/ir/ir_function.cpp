#include <algorithm>
#include <unordered_set>
#include "ir_function.hpp"
#include "ir_type.hpp"
#include "ir_value.hpp"
#include "ir_module.hpp"
#include "ir_basic_block.hpp"
#include "ir_instruction.hpp"

Argument::Argument(Type* type, const string& name, Function* parent, int index)
    : Value(type, name), parent(parent), index(index) {}

Function::Function(FunctionType* type, const string& name, Module* parent)
    : Value(type, name), parent(parent), ssa_id(0) {
    parent->add_function(this);
    for(int i = 0; i < type->param_types.size(); ++i) {
        args.push_back(new Argument(type->param_types[i], "", this, i));
    }
}

void Function::add_bb(BasicBlock* bb) {
    bb_list.push_back(bb);
}

void Function::remove_bb(BasicBlock* bb) {
    // 先remove所有该基本块到末尾，再将末尾的元素删除
    bb_list.erase(remove(bb_list.begin(), bb_list.end(), bb), bb_list.end());
    // 处理该基本块的前驱和后继的关系
    for(auto pre: bb->pre_bbs) {
        pre->remove_succ_basic_block(bb);
    }
    for(auto succ: bb->succ_bbs) {
        succ->remove_pre_basic_block(bb);
    }
}

BasicBlock* Function::get_entry_bb() {
    if(bb_list.empty()) {
        return nullptr; // 如果没有基本块，返回空
    }
    for(auto bb: bb_list) {
        if(bb->name == "label_entry") {
            return bb; // 找到入口基本块
        }
    }
    return bb_list.front(); // 如果没有找到名为 "label_entry" 的基本块，返回第一个基本块
}

BasicBlock* Function::get_ret_bb() {
    for(auto bb: bb_list) {
        auto terminator = bb->get_terminator();
        if(terminator && terminator->iid == IRInstID::Ret) {
            return bb;
        }
    }
    return nullptr; // 如果没有找到返回空
}

Type* Function::get_ret_type() {
    return static_cast<FunctionType*>(type)->ret_type;
}

void Function::set_ssa_id() {
    unordered_set<Value*> named_vals;
    for(auto& arg: args) {
        if(!named_vals.count(arg)) {
            if(arg->name.empty()) {
                arg->name = "arg_" + to_string(ssa_id++);
                named_vals.insert(arg);
            }
        }
    }
    for(auto& bb: bb_list) {
        if(!named_vals.count(bb) && bb->name.substr(0, 6) != "label_") {
            bb->name = "label_" + to_string(ssa_id++);
            named_vals.insert(bb);
        }
        for(auto& inst: bb->inst_list) {
            if(!named_vals.count(inst)) {
                if(inst->name.empty() && inst->type->tid != TypeID::VoidTy) {
                    inst->name = "v" + to_string(ssa_id++);
                    named_vals.insert(inst);
                }
            }
        }
    }
}