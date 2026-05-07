#ifndef RISCV64_ISEL_H
#define RISCV64_ISEL_H

#include "RISCv64LLIR.h"

// Forward declarations
namespace sysy {
    class GlobalValue;
    class Value;
}

extern int DEBUG;
extern int DEEPDEBUG;
extern int optLevel;

namespace sysy {

class RISCv64ISel {
public:
    RISCv64ISel();
    // 模块主入口：将一个高层IR函数转换为底层LLIR函数
    std::unique_ptr<MachineFunction> runOnFunction(Function* func);

    // 公开接口，以便后续模块（如RegAlloc）可以查询或创建vreg
    unsigned getVReg(Value* val);
    unsigned getNewVReg(Type* type);
    unsigned getVRegCounter() const; 
    // 获取 vreg_map 的公共接口
    const std::map<Value*, unsigned>& getVRegMap() const { return vreg_map; }
    const std::map<unsigned, Value*>& getVRegValueMap() const { return vreg_to_value_map; }
    const std::map<unsigned, Type*>& getVRegTypeMap() const { return vreg_type_map; }
private:
    // DAG节点定义，作为ISel的内部实现细节
    struct DAGNode; 
    
    // 指令选择主流程
    void select();
    // 为单个基本块生成指令
    void selectBasicBlock(BasicBlock* bb);
    // 核心函数：为DAG节点选择并生成MachineInstr
    void selectNode(DAGNode* node);

    // DAG 构建相关函数 (从原RISCv64Backend迁移)
    std::vector<std::unique_ptr<DAGNode>> build_dag(BasicBlock* bb);
    DAGNode* get_operand_node(Value* val_ir, std::map<Value*, DAGNode*>&, std::vector<std::unique_ptr<DAGNode>>&);
    DAGNode* create_node(int kind, Value* val, std::map<Value*, DAGNode*>&, std::vector<std::unique_ptr<DAGNode>>&);
    // 用于计算类型大小的辅助函数
    unsigned getTypeSizeInBytes(Type* type);
    
    // 打印DAG图以供调试
    void print_dag(const std::vector<std::unique_ptr<DAGNode>>& dag, const std::string& bb_name);
    
    // 状态
    Function* F; // 当前处理的高层IR函数
    std::unique_ptr<MachineFunction> MFunc; // 正在构建的底层LLIR函数
    MachineBasicBlock* CurMBB; // 当前正在处理的机器基本块

    // 映射关系
    std::map<Value*, unsigned> vreg_map;
    std::map<unsigned, Value*> vreg_to_value_map;
    std::map<unsigned, Type*> vreg_type_map;
    std::map<const BasicBlock*, MachineBasicBlock*> bb_map;

    unsigned vreg_counter;
    int local_label_counter;
};

} // namespace sysy

#endif // RISCV64_ISEL_H