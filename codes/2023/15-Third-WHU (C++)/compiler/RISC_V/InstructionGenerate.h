#pragma once
#ifndef Instruction_Generate_H
#define Instruction_Generate_H

#include "../IR/DAG.h"
#include "../DFA/DFA.h"
#include "Linearize.h"

//÷∏¡Ó…˙≥…
//void instructionGenerate(string functionname, SymbolTable* GlobalSymbolTable, vector<DAGNode*>& linearDAG, map<Symbol*, int>& m, int n);

//void instructionToAssemblerCode(string functionname, DAGNode* dag, std::map<Symbol*, int>& m);

//extern bool round_float;

void instructionGenerate(GCFG* gcfg, DFA* dfa, Linear* linear, string targetname);

#endif