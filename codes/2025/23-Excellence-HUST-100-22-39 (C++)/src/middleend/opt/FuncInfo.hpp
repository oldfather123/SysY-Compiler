#ifndef __FUNC_INFO_H__
#define __FUNC_INFO_H__

#include <unordered_map>
#include <set>
#include <unordered_set>
#include "opt.hpp"

struct FuncInfo {
    Function* func;
    int param_count;
    int bb_count;
    int inst_count;
    bool has_return;
    bool is_recursive;
    bool has_global_access; // 是否有全局变量的读写操作
    vector<CallInst*> call_insts;
    set<Function*> callee_funcs; // 被调用的函数列表
    set<Function*> caller_funcs; // 调用该函数的函数列表

    // 额外信息，等待构造出所有函数的信息之后再设置
    bool is_pure;
    bool is_inlineable; // 是否适合内联

    FuncInfo() = default;
    FuncInfo(Function* func);
};

bool is_pure_func(Function* func);

void show_func_info(const FuncInfo& info);
void show_func_info_map(const unordered_map<Function*, FuncInfo>& func_info_map);

#endif // __FUNC_INFO_H__