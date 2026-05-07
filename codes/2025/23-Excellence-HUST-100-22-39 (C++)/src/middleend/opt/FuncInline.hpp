#ifndef __FUNC_INLINE_H__
#define __FUNC_INLINE_H__

#include "FuncInfo.hpp"

class FuncInline: public Optimization {
public:
    explicit FuncInline(Module* m): Optimization(m) {}
    void execute() override;

private:
    unordered_map<Function*, FuncInfo> func_info_map;
    unordered_map<Value*, Value*> op_map; // 用于操作数的映射，包括形参到实参、指令到新指令的映射，在多基本块的 callee 中增加基本块到新基本块的映射

    // 收集函数信息
    void collect_func_info();
    // 函数排序：处理嵌套函数的内联时，先内联最内层的函数
    vector<Function*> sort_function();
    // 规范化内联函数：合并所有 ret 语句
    void normalize_inline_func(Function* callee);
    // 函数内联
    void inline_func_with_one_bb(Function* callee, Function* caller); // 单基本块
    void inline_func_with_multiple_bb(Function* callee, Function* caller); // 多基本块
    // 拆分 caller 调用语句前后的指令
    pair<BasicBlock*, BasicBlock*> split_call_inst(CallInst* call_inst, Function* callee, Function* caller);
    // 建立形参到实参的映射
    void build_formal_to_actual_arg_map(CallInst* call_inst, Function* callee);
    // 在 caller 中创建新基本块，同时建立 callee 基本块到 caller 新基本块的映射
    void copy_bb_from_callee(Function* callee, Function* caller, BasicBlock* before_bb);
    // 维护 caller 中新基本块的前驱后继关系
    void map_caller_bb_pre_succ(Function* caller);
    // 移植 callee 指令到 caller 中
    void copy_inst(Function* callee, Function* caller, CallInst* call_inst, BasicBlock* after_bb, vector<PhiInst*>& phi_inst_to_update);
    // 更新 Phi 指令的操作数
    void update_phi_operands(vector<PhiInst*>& phi_inst_to_update);
};

#endif // __FUNC_INLINE_H__