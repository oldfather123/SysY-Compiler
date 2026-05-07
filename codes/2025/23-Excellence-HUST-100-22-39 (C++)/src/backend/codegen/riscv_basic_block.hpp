#ifndef __RISCV_BASIC_BLOCK_HPP__
#define __RISCV_BASIC_BLOCK_HPP__

#include <list>
#include "riscv.hpp"
#include "riscv_value.hpp"
#include "riscv_instruction.hpp"

class RiscvBasicBlock: public RiscvValue {
public:
    int rbid;                            // 基本块编号
    RiscvFunction* parent;              // 所属函数
    list<RiscvInstruction*> rinst_list;  // 基本块内的指令

    /// @brief 构造函数
    /// @param name 基本块名称
    /// @param parent 基本块所属函数
    /// @param rbid 基本块标号
    RiscvBasicBlock(string name, int rbid, RiscvFunction* parent = nullptr);
    string print() override;

    /// @brief 在基本块头部添加指令
    /// @param rinst 被添加的指令
    void add_rinst_front(RiscvInstruction* rinst);

    /// @brief 在基本块尾部添加指令
    /// @param rinst 被添加的指令
    void add_rinst_back(RiscvInstruction* rinst);

    /// @brief 在指定指令之前添加新指令
    /// @param new_rinst 被添加的指令
    /// @param rinst 指定指令
    void add_rinst_before_rinst(RiscvInstruction* new_rinst, RiscvInstruction* rinst);

    /// @brief 在指定指令之后添加新指令
    /// @param new_rinst 被添加的指令
    /// @param rinst 指定指令
    void add_rinst_after_rinst(RiscvInstruction* new_rinst, RiscvInstruction* rinst);

    /// @brief 在基本块的终结指令之前添加新指令
    /// @param rinst 被添加的指令
    void add_rinst_before_terminator(RiscvInstruction* rinst);

    /// @brief 删除指令
    /// @param rinst 被删除的指令
    void delete_rinst(RiscvInstruction* rinst);

    /// @brief 获取特定次序的指令
    /// @note 为 std::list 结构提供的类似 vector 随机下标访问的接口
    /// @param index 指令的索引
    RiscvInstruction* get_rinst_by_index(int index);
};

#endif // __RISCV_BASIC_BLOCK_HPP__