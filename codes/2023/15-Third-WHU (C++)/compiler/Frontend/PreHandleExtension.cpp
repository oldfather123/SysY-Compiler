#include "PreHandleExtension.h"

SymbolTable* table;

void preHandleExtension(SymbolTable* symbolTable) 
{
	table = symbolTable;
	symbolTable->AddSymbol(GET_INT,InitGetInt());
	symbolTable->AddSymbol(GET_CH, InitGetCh());
	symbolTable->AddSymbol(GET_FLOAT, InitGetFloat());
	symbolTable->AddSymbol(GET_ARRAY, InitGetArray());
	symbolTable->AddSymbol(GET_F_ARRAY, InitGetFArray());
	symbolTable->AddSymbol(PUT_INT, InitPutInt());
	symbolTable->AddSymbol(PUT_CHAR, InitPutChar());
	symbolTable->AddSymbol(PUT_FLOAT, InitPutFloat());
	symbolTable->AddSymbol(PUT_ARRAY, InitPutArray());
	symbolTable->AddSymbol(PUT_F_ARRAY, InitPutFArray());
	symbolTable->AddSymbol(PUT_F, InitPutF());
	symbolTable->AddSymbol(START_TIME, InitStartTime());
	symbolTable->AddSymbol(STOP_TIME, InitStopTime());
}

//1.int getint(){}
Symbol* InitGetInt()
{
	FuncType* functype = new FuncType();
	ValType* returnType = new ValType();
	returnType->setType(new IntType());
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::i32;
	return symbol;
}

//2.int getch(){}
Symbol* InitGetCh()
{
	FuncType* functype = new FuncType();
	ValType* returnType = new ValType();
	returnType->setType(new IntType());
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::i32;
	return symbol;
}

//3.float getfloat(){}
Symbol* InitGetFloat()
{
	FuncType* functype = new FuncType();
	ValType* returnType = new ValType();
	returnType->setType(new FloatType());
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::f32;
	return symbol;
}

//4.int getarray(int[]){}
Symbol* InitGetArray()
{
	FuncType* functype = new FuncType();

	std::vector<std::shared_ptr<ExprAST>> exprs;
	ValType* baseType = new ValType();
	baseType->setType(new IntType());

	ValType* inttype = new ValType();
	inttype->setConst();
	inttype->setType(new IntType());
	exprs.push_back(std::make_shared<IntegerLiteralAST>(inttype, 0));

	ArrayType* pointType = new ArrayType(baseType,exprs);
	ValType* valType = new ValType();
	valType->setType(pointType);

	std::vector<ValType*> Params;
	Params.push_back(valType);

	ValType* returnType = new ValType();
	returnType->setType(new IntType());

	functype->setParamTypes(Params);
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::i32;
	return symbol;
}

//5.int getfarray(float[]){}
Symbol* InitGetFArray()
{
	FuncType* functype = new FuncType();

	std::vector<std::shared_ptr<ExprAST>> exprs;
	ValType* baseType = new ValType();
	baseType->setType(new FloatType());

	ValType* inttype = new ValType();
	inttype->setConst();
	inttype->setType(new IntType());
	exprs.push_back(std::make_shared<IntegerLiteralAST>(inttype, 0));

	ArrayType* pointType = new ArrayType(baseType, exprs);
	ValType* valType = new ValType();
	valType->setType(pointType);

	std::vector<ValType*> Params;
	Params.push_back(valType);

	ValType* returnType = new ValType();
	returnType->setType(new IntType());

	functype->setParamTypes(Params);
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::i32;
	return symbol;
}

//6.void putint(int){}
Symbol* InitPutInt()
{
	FuncType* functype = new FuncType();

	ValType* valType = new ValType();
	valType->setType(new IntType());

	std::vector<ValType*> Params;
	Params.push_back(valType);

	ValType* returnType = new ValType();
	returnType->setType(new VoidType());

	functype->setParamTypes(Params);
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::VOID;
	return symbol;
}

//7.void putch(int){}
Symbol* InitPutChar()
{
	FuncType* functype = new FuncType();

	ValType* valType = new ValType();
	valType->setType(new IntType());

	std::vector<ValType*> Params;
	Params.push_back(valType);

	ValType* returnType = new ValType();
	returnType->setType(new VoidType());

	functype->setParamTypes(Params);
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::VOID;
	return symbol;
}

//8.void putfloat(float){}
Symbol* InitPutFloat()
{
	FuncType* functype = new FuncType();

	ValType* valType = new ValType();
	valType->setType(new FloatType());

	std::vector<ValType*> Params;
	Params.push_back(valType);

	ValType* returnType = new ValType();
	returnType->setType(new VoidType());

	functype->setParamTypes(Params);
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::VOID;
	return symbol;
}

//9.void putarray(int,int[]){}
Symbol* InitPutArray()
{
	FuncType* functype = new FuncType();

	ValType* valType1 = new ValType();
	valType1->setType(new IntType());

	std::vector<std::shared_ptr<ExprAST>> exprs;
	ValType* baseType = new ValType();
	baseType->setType(new IntType());

	ValType* inttype = new ValType();
	inttype->setConst();
	inttype->setType(new IntType());
	exprs.push_back(std::make_shared<IntegerLiteralAST>(inttype, 0));

	ArrayType* pointType = new ArrayType(baseType, exprs);
	ValType* valType2 = new ValType();
	valType2->setType(pointType);

	std::vector<ValType*> Params;
	Params.push_back(valType1);
	Params.push_back(valType2);

	ValType* returnType = new ValType();
	returnType->setType(new VoidType());

	functype->setParamTypes(Params);
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::VOID;
	return symbol;
}

//10.void putfarray(int,float[]){}
Symbol* InitPutFArray()
{
	FuncType* functype = new FuncType();

	ValType* valType1 = new ValType();
	valType1->setType(new IntType());

	std::vector<std::shared_ptr<ExprAST>> exprs;
	ValType* baseType = new ValType();
	baseType->setType(new FloatType());

	ValType* inttype = new ValType();
	inttype->setConst();
	inttype->setType(new IntType());
	exprs.push_back(std::make_shared<IntegerLiteralAST>(inttype, 0));

	ArrayType* pointType = new ArrayType(baseType, exprs);
	ValType* valType2 = new ValType();
	valType2->setType(pointType);

	std::vector<ValType*> Params;
	Params.push_back(valType1);
	Params.push_back(valType2);

	ValType* returnType = new ValType();
	returnType->setType(new VoidType());

	functype->setParamTypes(Params);
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::VOID;
	return symbol;
}

//11做不完全，只有返回值，特殊情况特殊判断
//11.void putf("...",...){}
Symbol* InitPutF()
{
	FuncType* functype = new FuncType();
	ValType* returnType = new ValType();
	returnType->setType(new VoidType());
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::VOID;
	return symbol;
}

//12.void starttime(){}
Symbol* InitStartTime()
{
	FuncType* functype = new FuncType();
	ValType* returnType = new ValType();
	returnType->setType(new VoidType());
	functype->setReturnType(returnType);
	Symbol* symbol = new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::VOID;
	return symbol;
}

//12.void stoptime(){}
Symbol* InitStopTime()
{
	FuncType* functype = new FuncType();
	ValType* returnType = new ValType();
	returnType->setType(new VoidType());
	functype->setReturnType(returnType);
	Symbol* symbol= new Symbol(functype, table->getCurrentScope());
	symbol->returnType = ResultType::VOID;
	return symbol;
}



