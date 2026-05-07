#pragma once
#ifndef GCFG_H
#define GCFG_H
#include "CFG.h"

//存储所有函数的CFG对象
class GCFG
{
public:
	std::map<std::string, CFG*> controlFlow;//用来记录每个函数的基本块，还有全局基本块

	//求数组栈空间
	void getArrayStack(SymbolTable* symboltable);
};

#endif // !GCFG_H

