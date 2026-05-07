#pragma once

#include "PassManager.hpp"
#include "Constant.hpp"
#include "IRBuilder.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "Value.hpp"

#include <vector>
#include <unordered_map>

// 前向声明辅助函数
ConstantFP *cast_constantfp(Value *value);
ConstantInt *cast_constantint(Value *value);


class ConstFolder {
public:
    ConstFolder(Module *m) : module_(m) {}
    
    // 二元运算
    ConstantInt *compute(Instruction::OpID op, ConstantInt *value1, ConstantInt *value2);
    ConstantFP *compute(Instruction::OpID op, ConstantFP *value1, ConstantFP *value2);
    
    ConstantFP *compute(Instruction::OpID op, ConstantInt *value1);  // int -> float
    ConstantInt *compute(Instruction::OpID op, ConstantFP *value1);  // float -> int

private:
    Module *module_;
};

class ConstantPropagation : public Pass {
public:
    ConstantPropagation(Module *m) : Pass(m), 
        builder_(new IRBuilder(nullptr, m)), 
        folder_(new ConstFolder(m)) {}
    
    ~ConstantPropagation() {
        delete builder_;
        delete folder_;
    }
    
    void run() override;

private:
    // 递归清理无用基本块
    void clear_blocks_recs(BasicBlock *start_bb);
    
    // 检查基本块是否是函数入口
    bool is_entry(BasicBlock *bb);
    
    // 成员变量
    IRBuilder *builder_;                    
    ConstFolder *folder_;                   
    std::vector<Instruction *> wait_delete_; 
    std::vector<BasicBlock *> delete_bb_;    
    int propagated_count_{0};               //一个统计器，没啥用
    
    
};