#pragma once
#ifndef ST_H
#define ST_H

#include "Type.h"

enum ResultType { i32, f32, iarray,farray,ipoint,fpoint, VOID ,stringT};

//常量的时候选一个进行计算，填入到符号表
typedef struct ConstValue {
	int iVal;
	float fVal;
	std::map<int, int> imap;
	std::map<int, float>fmap;
}ConstValue;

//全局数组传偏移量
typedef struct GlobalValue {
	int iVal;
	float fVal;
	std::map<int, int> imap;
	std::map<int, float>fmap;
}GlobalValue;

enum SymbolType {
	FUNCTION,
	VAL
};

class Scope;

class Symbol {
public:
	Symbol() {};
	Symbol(ValType* valtype, Scope* currentscope) : valtype(valtype), symbolType(SymbolType::VAL), currentscope(currentscope) {}
	Symbol(FuncType* functype, Scope* currentscope) :functype(functype), symbolType(SymbolType::FUNCTION), currentscope(currentscope) {}
	Symbol(Symbol* symbol) {
		this->currentscope = symbol->currentscope;
		this->type = symbol->type;
		this->isConst = symbol->isConst;
		this->dimensions = symbol->dimensions;
		this->symbolType = symbol->symbolType;
		this->returnType = symbol->returnType;
		this->constValue = symbol->constValue;
		this->globalValue = symbol->globalValue;
		this->ParamSymbols = symbol->ParamSymbols;
		this->offsetVec = symbol->offsetVec;
	}

	//在语法分析的时候进行简单的解析
	std::string name;
	ValType* valtype;//存储变量未解析之前的类型
	FuncType* functype;
	SymbolType symbolType;//符号的类型：函数、变量

	Scope* currentscope;//存放当前标识符的作用域

	//在语义分析的时候进行抽象，将symbol转换成单纯的类型，摆脱抽象语法树       
	//函数除了知道返回值类型，还需要知道参数是谁
	ResultType type;//"i32,f32,iarray,farray,void" 
	bool isConst = false;
	std::vector<int> dimensions;//数组维度/指针维度
	std::vector<int> offsetVec;//每一维取地址产生的偏移量
	ConstValue constValue;//常量：如果既是常量又是全局量，优先是一个常量
	GlobalValue globalValue;//全局量
	ResultType returnType;//如果是函数的话就是返回值类型"i32,f32,void"
	std::vector<Symbol*>ParamSymbols;//如果是函数，存储函数的参数

	bool isUsed = false;//判断一个变量是不是被用到，数组求栈空间
};
class Scope {
public:
	Scope() : Parent(nullptr) {}

	void AddSymbol(std::string name, Symbol* Sym) { Symbols[name] = Sym;Sym->name = name; }

	Symbol* Lookup(const std::string& Name) {
		auto iter = Symbols.find(Name);
		if (iter != Symbols.end()) {
			return iter->second;
		}
		else {
			return nullptr;
		}	
	}

	void setParent(Scope* P) { Parent = P; }
	Scope* getParent() const { return Parent; }

	std::map < std::string, Symbol*> Symbols;
	Scope* Parent;
	std::vector<Scope*> childs;
};
//把symbol和scope挪到ast.h中

class SymbolTable {
public:
	SymbolTable() : GlobalScope(new Scope) {}

	void AddSymbol(std::string name,Symbol* Sym) { Sym->currentscope->AddSymbol(name,Sym); }
	void SSA_AddSymbol(std::string name, Symbol* Sym) { Sym->currentscope->AddSymbol(name, Sym); }
	void Inline_AddSymbol(std::string funcname, Symbol* Sym) { 
		Scope* scope = getFuncScope(funcname);
		scope->AddSymbol(Sym->name, Sym); 
	}

	Symbol* Lookup(const std::string& Name) {
		Scope* S = CurrentScope;
		while (S != nullptr) {
			Symbol* Sym = S->Lookup(Name);
			if (Sym != nullptr)
				return Sym;
			S = S->getParent();
		}
		return nullptr;
	}

	Symbol* LookupFunc(const std::string& Name) {
		Scope* S = GlobalScope;
		while (S != nullptr) {
			Symbol* Sym = S->Lookup(Name);
			if (Sym != nullptr&&Sym->symbolType==FUNCTION)
				return Sym;
			S = S->getParent();
		}
		return nullptr;
	}

	bool IsInCurrentScope(const std::string& Name) {
		Scope* S = CurrentScope;
		if (S != nullptr) {
			Symbol* Sym = S->Lookup(Name);
			if (Sym != nullptr)
				return true;
		}
		return false;
	}

	void EnterScope() {
		Scope* NewScope = new Scope;
		NewScope->setParent(CurrentScope);
		CurrentScope->childs.push_back(NewScope);
		CurrentScope = NewScope;
	}

	Scope* getCurrentScope() { return CurrentScope; };

	void ExitScope() { CurrentScope = CurrentScope->getParent(); }

	//获取所有全局变量
	std::vector<Symbol*> getGlobalSymbol()
	{
		std::vector<Symbol*> result;
		for (auto it = GlobalScope->Symbols.begin(); it != GlobalScope->Symbols.end(); it++) {
			if (it->second->symbolType == SymbolType::VAL) {
				result.push_back(it->second);
			}
		}
		return result;
	}

	//获取一个全局符号
	Symbol*  getGlobalSymbol(std::string name) {
		return GlobalScope->Symbols[name];
	}

	Scope* getGlobalScope() {
		return this->GlobalScope;
	}

	void addFuncScope(std::string name, Scope* scope) {
		funcScope[name] = scope;
	}

	//获取函数的第一个作用域
	Scope* getFuncScope(std::string name) {
		return funcScope[name];
	}

	//判断是不是全局变量
	bool isGlobalSymbol(Symbol* symbol) {
		return symbol->currentscope == GlobalScope;
	}

	//获取函数参数
	std::vector<Symbol*> getFuncSymbols(std::string name) {
		Symbol* symbol = GlobalScope->Symbols[name];
		return symbol->ParamSymbols;
	}

	bool isGlobal(Symbol* sym) {
		return sym->currentscope == GlobalScope;
	}

	bool isAfatherB(Scope* A_SymScope,Scope* B_NextBlockScope) {
		Scope* current = B_NextBlockScope;
		while (current != GlobalScope) {
			if (current == A_SymScope) {
				return true;
			}
			current = current->Parent;
		}
		return false;
	}

	Scope* GlobalScope;
	std::map<std::string, Scope*> funcScope;
	Scope* CurrentScope = GlobalScope;
};


#endif