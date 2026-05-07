#ifndef __OPT_HPP__
#define __OPT_HPP__

#include <unordered_set>
#include "ir.hpp"
#include "ir_type.hpp"
#include "ir_value.hpp"
#include "ir_instruction.hpp"
#include "ir_basic_block.hpp"
#include "ir_function.hpp"
#include "ir_module.hpp"

class Optimization {
public:
    Module* m;
    explicit Optimization(Module* m): m(m) {}
    virtual void execute() = 0;
};

/// @brief 删除前驱后继关系时，通过删除后继块中可能存在的 Phi 指令的操作数对，来达到化简 Phi 指令甚至消除 Phi 指令的目的
/// @param bb 当前基本块
/// @param succ_bb 后继基本块
void remove_phi_in_succ(BasicBlock* bb, BasicBlock* succ_bb);

/// @brief 深度优先搜索遍历基本块，标记可达的基本块
/// @param bb 当前基本块
/// @param visited 已访问的基本块集合
void dfs_cfg(BasicBlock* bb, unordered_set<BasicBlock*>& visited);

/// @brief 删除不可达的基本块
/// @param func 待检查的函数
void delete_unreachable_blocks(Function* func);

#endif // __OPT_HPP__