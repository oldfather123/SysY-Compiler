#ifndef StackFrameExtension_H
#define StackFrameExtension_H
#include "DataFlowAnalysis.h"
#define ARGUMENT_SPILL "%argument_spill"
//计算支配点集
void calcu_dom(CFG* cfg, map<BasicBlock*, set<BasicBlock*>>& doms);

//计算当前基本块的前置块的支配集的交集
set<BasicBlock*> calcu_dom_inters(vector<BasicBlock*> pres, map<BasicBlock*, set<BasicBlock*>> doms);
//处理一个函数多余的形参
void stack_frame_extension(DataFlowAnalysis* dfa, SymbolTable* sym);

//在一个vector中找到它
int find_inVec(vector<string> live_vec, string para);

#endif