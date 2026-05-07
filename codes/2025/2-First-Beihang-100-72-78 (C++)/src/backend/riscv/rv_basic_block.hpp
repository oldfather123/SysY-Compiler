#ifndef GEECEECEE_RV_BASIC_BLOCK_HPP
#define GEECEECEE_RV_BASIC_BLOCK_HPP

#pragma once
#include <list>
#include <set>
#include <string>

#include "rv_operand.hpp"

namespace backend::riscv {

class RVFunction;     // Forward declaration
class RVInstruction;  // Forward declaration

// 活跃信息结构体
struct BlockLiveInfo {
    std::set<RVReg *> live_use = std::set<RVReg *>();
    std::set<RVReg *> live_def = std::set<RVReg *>();
    std::set<RVReg *> live_in = std::set<RVReg *>();
    std::set<RVReg *> live_out = std::set<RVReg *>();
};

class RVBasicBlock {
public:
    using Ptr = RVBasicBlock *;

    // 指令列表类型定义
    using InstList = std::list<RVInstruction *>;
    using InstIterator = InstList::iterator;
    using InstConstIterator = InstList::const_iterator;

    explicit RVBasicBlock(std::string l);
    ~RVBasicBlock();

    // 指令相关方法
    void add_inst(RVInstruction *inst);
    InstList &get_insts() { return instructions; }
    const InstList &get_insts() const { return instructions; }
    void delete_all_insts();

    // 指令列表操作，维护迭代器引用
    InstIterator insert_inst(InstIterator pos, RVInstruction *inst);
    InstIterator remove_inst(InstIterator pos);
    void move_inst(InstIterator from, InstIterator to);

    // 获取指令在列表中的位置
    InstIterator get_inst_position(RVInstruction *inst);

    // 检查指令是否属于这个基本块
    bool contains_inst(RVInstruction *inst);

    // 前驱和后继块相关方法
    void add_pred(RVBasicBlock *block);
    void add_succ(RVBasicBlock *block);
    void remove_pred(RVBasicBlock *block);
    void remove_succ(RVBasicBlock *block);
    void clear_preds() { preds.clear(); }
    void clear_succs() { succs.clear(); }
    const std::set<RVBasicBlock *> &get_preds() const { return preds; }
    const std::set<RVBasicBlock *> &get_succs() const { return succs; }

    // 获取基本块名称/标签
    const std::string &get_name() const { return name; }
    void set_name(const std::string &label) { name = label; }

    // 创建标签操作数
    RVLabel *get_label() const;

    // 输出方法
    std::string to_string() const;  // label:\n  inst\n  inst\n

    // 活跃信息相关接口
    BlockLiveInfo &get_live_info() { return live_info; }
    const BlockLiveInfo &get_live_info() const { return live_info; }
    void set_live_info(const BlockLiveInfo &info) { live_info = info; }

    // 循环深度相关接口
    int get_loop_depth() const { return loop_depth; }
    void set_loop_depth(int depth) { loop_depth = depth; }

    // 函数对象管理
    void set_function(RVFunction *func);
    RVFunction *get_function() const;

private:
    std::string name;
    InstList instructions = InstList();
    std::set<RVBasicBlock *> preds = std::set<RVBasicBlock *>();  // 前驱块列表
    std::set<RVBasicBlock *> succs = std::set<RVBasicBlock *>();  // 后继块列表
    BlockLiveInfo live_info;
    RVFunction *function = nullptr;  // 所属函数
    int loop_depth = 0;              // 循环深度
};

}  // namespace backend::riscv

#endif
