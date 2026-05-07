#pragma once

#include "PassManager.hpp"
#include "Dominators.hpp"
#include "Instruction.hpp"
#include "BasicBlock.hpp"
#include "Value.hpp"

#include <map>
#include <memory>
#include <vector>
#include <set>

class Mem2Reg : public Pass {
public:
    explicit Mem2Reg(Module *m) : Pass(m) {}
    ~Mem2Reg() override = default;

   
    void run() override;

private:
    // 将所有 alloca 指令移动到入口块
    void loop_alloc_inv_hoist();
    
    // 在必要位置插入 phi 指令
    void generate_phi();
    void complete_phi_nodes();
    void remove_dead_allocas();

    // 对基本块进行 SSA 重命名
    void rename(BasicBlock *bb);
    
    // 删除多余的 PHI 节点
    void remove_trivial_phi();

    // 判断是否为可提升的指针（非全局，也非 GEP）
    static bool is_global_variable(Value *v);
    static bool is_gep_instr(Value *v);
    static bool is_valid_ptr(Value *v);

    Function                         *func_{nullptr};
    std::unique_ptr<Dominators>       dominators_;

    // 参考代码中使用的成员变量名
    // 每个变量的"定义栈"，用于重命名阶段
    std::map<Value *, std::vector<Value *>> var_val_stack_;

    // 每个插入的 phi 指令对应的原始 alloca 地址
    std::map<Value *, Value *>              phi_map;
};