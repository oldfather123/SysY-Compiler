#ifndef __CONSTANT_ANALYSIS_H__
#define __CONSTANT_ANALYSIS_H__

#include "opt.hpp"

class ConstantAnalysis: public Optimization {
private:
    // 计算整型二元运算式
    ConstInt* cal_int(IRInstID op, ConstInt* v1, ConstInt* v2);
    // 计算浮点型二元运算式
    ConstFloat* cal_float(IRInstID op, ConstFloat* v1, ConstFloat* v2);
    // 计算整型比较
    ConstInt* cal_icmp(ICmpOp op, ConstInt* v1, ConstInt* v2);
    // 计算浮点比较
    ConstInt* cal_fcmp(FCmpOp op, ConstFloat* v1, ConstFloat* v2);
    // 常量折叠、常量传播
    bool const_fold_prop(Function* func);
	// 链接常量条件分支
    void link_cond_branch(BasicBlock* bb, BasicBlock* target_bb);
    // 稀疏条件常量传播（Sparse Conditional Constant Propagation, SCCP）
    bool sparse_cond_const_prop(Function* func);
public:    
    explicit ConstantAnalysis(Module* m): Optimization(m){}
    void execute() override;
};

#endif // __CONSTANT_ANALYSIS_H__