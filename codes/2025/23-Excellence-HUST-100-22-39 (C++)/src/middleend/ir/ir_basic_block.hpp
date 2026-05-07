#ifndef __IR_BASICBLOCK_HPP__
#define __IR_BASICBLOCK_HPP__

#include <list>
#include <vector>
#include <set>
#include <string>
#include <unordered_set>
#include "ir.hpp"
#include "ir_value.hpp"

class BasicBlock: public Value {
public:
    list<Instruction*> inst_list;   // 基本块内的指令列表
    Function* parent;               // 所属函数
    vector<BasicBlock*> pre_bbs;    // 前驱基本块
    vector<BasicBlock*> succ_bbs;   // 后继基本块
    BasicBlock* idom;               // 立即支配者
    set<BasicBlock*> dom_frontier;  // 支配边界：支配基本块的某个前驱但是不支配该基本块
    set<BasicBlock*> rdoms;         // 逆支配：从该基本块出发必须经过的基本块
    set<BasicBlock*> rdom_frontier; // 逆支配边界
    int dom_level;                  // block编号
    unordered_set<Value*> live_in;  // 活跃变量集合：进入该基本块的活跃变量
    unordered_set<Value*> live_out; // 活跃变量集合：离开该基本块

    /// @brief 构造函数
    /// @param module 所属函数的所属模块（每次运行程序时的唯一）
    /// @param name 基本块名称
    /// @param parent 所属函数
    explicit BasicBlock(Module* module, const string& name, Function* parent);
    void print(ostream& out) override;

    /// @brief 头部插入指令
    /// @param inst 要插入的指令
    /// @return 是否成功
    bool add_inst_front(Instruction* inst);

    /// @brief 尾部插入指令
    /// @param inst 要插入的指令
    /// @return 是否成功
    bool add_inst_back(Instruction* inst);

    /// @brief 在指定指令前插入新指令
    /// @param new_inst 要插入的新指令
    /// @param inst 指定指令
    /// @return 是否成功
    bool add_inst_before_inst(Instruction* new_inst, Instruction* inst);

    /// @brief 在终结指令前插入新指令
    /// @param new_inst 要插入的新指令
    /// @return 是否成功
    bool add_inst_before_terminator(Instruction* new_inst);

    /// @brief 添加前驱基本块
    /// @param bb 前驱基本块
    void add_pre_basic_block(BasicBlock* bb);

    /// @brief 添加后继基本块
    /// @param bb 后继基本块
    void add_succ_basic_block(BasicBlock* bb);

    /// @brief 移除前驱基本块
    /// @param bb 前驱基本块
    void remove_pre_basic_block(BasicBlock* bb);

    /// @brief 移除后继基本块
    /// @param bb 后继基本块
    void remove_succ_basic_block(BasicBlock* bb);

    /// @brief 当前基本块是否被指定基本块支配
    /// @param bb 指定基本块
    bool is_dominated_by(BasicBlock* bb);

    /// @brief 获取当前基本块的最后一条指令
    /// @note 只能是 Ret 或 Br 指令，否则返回空指针
    /// @return 终结指令
    Instruction* get_terminator(); // 获取终结指令

    /// @brief 从基本块中移除指令
    /// @note 只移除指令，不删除指令的 use 关系，因为可能会插入其他基本块
    /// @param inst 要移除的指令
    /// @return 是否成功
    bool remove_inst(Instruction* inst);

    /// @brief 删除指令
    /// @note 删除指令的同时，也会删除指令的 use 关系
    /// @param inst 要删除的指令
    /// @return 是否成功
    bool delete_inst(Instruction* inst);
};

#endif // __IR_BASICBLOCK_HPP__
