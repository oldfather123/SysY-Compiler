#ifndef IRGEN_H
#define IRGEN_H

#include "../include/ast.h"
#include "../include/cfg.h"
#include "../include/symbol.h"
#include "../include/type.h"
#include <assert.h>
#include <map>
#include <stack>
#include <vector>
#include <memory>

typedef std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> rParamsVec;

class IRgenTable {
public:

    SymbolRegTable symbol_table;
	SymbolDimTable symboldim_table;

	// 实参操作数存储, 同时用栈存储形参和实参方便比对，进行类型转换
	std::stack<rParamsVec> FuncRParamsStack;
	std::stack<std::vector<FuncFParam>> FuncFParamsStack;

    IRgenTable() {}
};

#endif