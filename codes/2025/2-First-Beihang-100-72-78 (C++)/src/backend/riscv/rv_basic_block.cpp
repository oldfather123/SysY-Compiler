#include "rv_basic_block.hpp"

#include <utility>

#include "rv_function.hpp"
#include "rv_instruction.hpp"

namespace backend::riscv {

RVBasicBlock::RVBasicBlock(std::string l) : name(std::move(l)), function(nullptr) {}

RVBasicBlock::~RVBasicBlock() {
    delete_all_insts();
    // preds/succs 只存指针，不负责释放
    preds.clear();
    succs.clear();
}

void RVBasicBlock::add_inst(RVInstruction *inst) {
    inst->update_graph_uses();

    // 将指令添加到指令列表
    instructions.push_back(inst);

    // 设置指令的迭代器引用，指向刚添加的位置
    inst->set_inst_iterator(std::prev(instructions.end()));

    // 设置指令的父基本块
    inst->set_parent_block(this);
}

void RVBasicBlock::add_pred(RVBasicBlock *block) { preds.insert(block); }

void RVBasicBlock::add_succ(RVBasicBlock *block) { succs.insert(block); }

void RVBasicBlock::remove_pred(RVBasicBlock *block) { preds.erase(block); }

void RVBasicBlock::remove_succ(RVBasicBlock *block) { succs.erase(block); }

RVLabel *RVBasicBlock::get_label() const { return RVLabel::create(name); }

std::string RVBasicBlock::to_string() const {
    // std::string output;
    std::string output = ".p2align 2\n";

    // 如果有标签，输出标签
    if (!name.empty()) {
        output += name + ":\n";
    }

    // 输出所有指令
    for (auto *inst : instructions) {
        output += "\t" + inst->to_string() + "\n";
    }

    return output;
}

void RVBasicBlock::set_function(RVFunction *func) { function = func; }

RVFunction *RVBasicBlock::get_function() const { return function; }

void RVBasicBlock::delete_all_insts() {
    for (auto *inst : instructions) {
        inst->delete_self();
    }
    instructions.clear();
}

RVBasicBlock::InstIterator RVBasicBlock::insert_inst(RVBasicBlock::InstIterator pos, RVInstruction *inst) {
    inst->update_graph_uses();

    // 在指定位置插入指令
    auto it = instructions.insert(pos, inst);

    // 设置指令的迭代器引用
    inst->set_inst_iterator(it);

    // 设置指令的父基本块
    inst->set_parent_block(this);

    return it;
}

RVBasicBlock::InstIterator RVBasicBlock::remove_inst(RVBasicBlock::InstIterator pos) {
    if (pos == instructions.end()) {
        return pos;
    }

    RVInstruction *inst = *pos;

    inst->delete_graph_uses();

    // 清理指令的迭代器引用
    inst->clear_inst_iterator();

    // 清理指令的父基本块引用
    inst->set_parent_block(nullptr);

    // 从列表中移除指令
    auto next_it = instructions.erase(pos);

    // 删除指令
    delete inst;

    return next_it;
}

void RVBasicBlock::move_inst(RVBasicBlock::InstIterator from, RVBasicBlock::InstIterator to) {
    if (from == to || from == instructions.end()) {
        return;
    }

    RVInstruction *inst = *from;

    // 从原位置移除
    instructions.erase(from);

    // 插入到新位置
    auto new_it = instructions.insert(to, inst);

    // 更新指令的迭代器引用
    inst->set_inst_iterator(new_it);

    // 确保指令的父基本块设置正确
    inst->set_parent_block(this);
}

RVBasicBlock::InstIterator RVBasicBlock::get_inst_position(RVInstruction *inst) {
    // 首先尝试使用指令自己的迭代器引用
    auto it = inst->get_inst_iterator();
    if (it != RVBasicBlock::InstIterator() && it != instructions.end() && *it == inst) {
        return it;
    }

    // 如果指令的迭代器引用无效，则线性搜索
    for (auto it = instructions.begin(); it != instructions.end(); ++it) {
        if (*it == inst) {
            return it;
        }
    }

    return instructions.end();
}

bool RVBasicBlock::contains_inst(RVInstruction *inst) { return get_inst_position(inst) != instructions.end(); }

}  // namespace backend::riscv
