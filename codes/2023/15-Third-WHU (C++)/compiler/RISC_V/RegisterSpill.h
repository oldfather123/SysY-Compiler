#pragma
#ifndef RegisterSpill_H
#define RegisterSpill_H
#include "../DFA/InterferenceGraph.h"
#include "../DFA/DataFlowAnalysis.h"
#include <climits>
#include <cmath>

//计算cost函数
InterNode* spill_cost(DataFlowAnalysis*& dfa, vector<InterNode*>& stack);
string spill_protect_cost(DataFlowAnalysis* &dfa, vector<string> protecct_nodes);


//替换掉（被溢出的合并节点）在拆分前的所有节点（寄存器节点、变量名节点）
//load a,reg变成load地址
//stroe a变成store地址
void spill_replaceIR(string merged_node, CFG* cfg, int i_or_f);


InterNode* reg_spill(DataFlowAnalysis* dfa, vector<InterNode*>& stack, int i_or_f);

void pre_tra(DataFlowAnalysis*& dfa, InterferenceGraph*& graph, int i_or_f);
//溢出保护寄存器
void choose_protect(DataFlowAnalysis*& dfa, int reg_num, set<InterNode*> prot_set, int i_or_f, set<string>& spilled);
//提前溢出保护寄存器
void pre_spill_protect(DataFlowAnalysis*& dfa, IGManager*& ig);
#endif