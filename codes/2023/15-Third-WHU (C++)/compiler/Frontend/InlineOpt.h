#pragma once
#ifndef INLINE_H
#define INLINE_H
#include"../IR/GCFG.h"
#include"AST.h"


DAGNode* getNewValNode(string name, Symbol* symbol);
map<string, vector<string>> inlineOptimization(SymbolTable* table, GCFG* gcfg, std::shared_ptr<ExprAST>CU,int Opt);
//删除没有用到的函数
void removeNoUseFunc(GCFG* gcfg);

void removeNoUseFunc(GCFG* gcfg);


#endif // !INLINE_H

