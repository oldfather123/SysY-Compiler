#pragma once
#ifndef AST_H
#define AST_H

#include"SymbolTable.h"


//只需要考虑左边的短路
enum ConditionConst {
	FALSE,
	TRUE,
	NONE,
	UNARY_GET_RIGHT,
	IMPLI_GET_RIGHT,
	BINARY_GET_LEFT,
	BINARY_GET_RIGHT,
};

//用于计算常量的返回值
typedef struct {
	std::string type;
	int iVal;
	float fVal;
}ValTypeStruct;

//抽象语法树基类
class ExprAST {
public:
	bool isBranch=false;
	std::shared_ptr<ExprAST> Next;
	std::shared_ptr<ExprAST> TrueExit;
	std::shared_ptr<ExprAST> FalseExit;
	virtual std::string getClassName() { return "1"; };
	virtual ~ExprAST() = default;
	virtual ValType* AnaExpr() { return nullptr; };
	virtual ValTypeStruct Evaluate() { ValTypeStruct val;return val; };
	virtual void analyzeFlow() { };
	virtual ConditionConst conditionAna() { return ConditionConst::NONE; };
	bool operator<(const ExprAST* other) const {
		return this < other;
	}
};


//数组类型
class ArrayType : public Type {
public:
	ValType* ElementType;//基本类型
	std::vector<std::shared_ptr<ExprAST>> exprs;//如果是数组定义，暂存数组每一维的表达式
	std::vector<int> Size;

	ArrayType(ValType* ElementType, std::vector<int> Size)
		: ElementType(ElementType), Size(Size) {
		setTypeID(Array);
	}
	ArrayType(ValType* ElementType, std::vector<std::shared_ptr<ExprAST>> exprs)
		: ElementType(ElementType), exprs(exprs) {
		setTypeID(Array);
	}
	std::vector<std::shared_ptr<ExprAST>> getExprs() {
		return exprs;
	}
	ValType* getElementType() const {
		return ElementType;
	}
	std::vector<int> getSize() const {
		return Size;
	}
	int ParseExprsToSize();
};

//为字符串添加类型
class StringAST :public ExprAST {
public:
	int id;//在vector中存放的id
	std::string str;

	StringAST(int id,std::string str) :id(id) ,str(str) {};
	std::string getClassName() { return "StringAST"; };
	void print();
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
};

//数组填充
class ImplicitValueInit :public ExprAST {
public:
	ValType* type;//填充类型
	ImplicitValueInit(ValType* type) :type(type){};
	std::string getClassName() { return "ImplicitValueInit"; };
	void print();
	//控制流分析
	void analyzeFlow();
};

//数组初始化列表
class InitListAST :public ExprAST {
public:
	ValType* Type;//存当前数组类型，如int[3]
	std::vector<std::shared_ptr<ExprAST>> List;//InitListExpr列表或数值列表

	InitListAST(ValType* Type, std::vector<std::shared_ptr<ExprAST>> List) :Type(Type), List(List) {};
	std::string getClassName() { return "InitList"; };
	void print();
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
};

//函数参数类型
class ParamDeclAST :public ExprAST {
public:
	ValType* Type;
	std::string Name;
	Symbol* symbol;

	ParamDeclAST(ValType* Type, const std::string& Name) :Type(Type), Name(Name) { this->symbol = nullptr; };
	std::string getClassName() { return "ParamDecl"; };
	ValType* getValType() { return Type; };
	std::string getName() { return Name; };
	void print();
	//语义分析
	ValType* AnaExpr();
};

//变量引用类型
class DeclRefAST :public ExprAST {
public:
	std::string Name;
	Symbol* UseDecl;//指向定义的节点，常量的话用于获取其常量值

	DeclRefAST(const std::string& Name, Symbol* UseDecl) :Name(Name), UseDecl(UseDecl) {};
	std::string getClassName() { return "DeclRef"; };
	std::string getName() { return Name; };
	void print();
	//语义分析
	ValType* AnaExpr();
	ValTypeStruct Evaluate();
	//控制流分析
	void analyzeFlow();

	ConditionConst conditionAna();
};

//函数调用表达式
class CallExprAST : public ExprAST {
public:
	std::string Callee;//调用的函数名
	Symbol* UseFunc;//调用的函数指针
	std::vector<std::shared_ptr<ExprAST>> Args;//参数

	CallExprAST(const std::string& Callee, Symbol* UseFunc,
		std::vector<std::shared_ptr<ExprAST>> Args)
		: Callee(Callee), UseFunc(UseFunc), Args(Args) {};
	std::string getClassName() { return "CallExpr"; };
	std::string getName() { return Callee; };
	std::vector<ExprAST*> getArgs() {
		std::vector<ExprAST*> result;
		for (const auto& expr : Args) {
			result.push_back(expr.get());
		}
		return result;
	};
	void print();
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();

	ConditionConst conditionAna();
};

//数组引用
class ArraySubscripAST :public ExprAST {
public:
	ValType* Type;
	std::shared_ptr<ExprAST> ArraySub;
	std::shared_ptr<ExprAST> Addr;
	
	ArraySubscripAST(std::shared_ptr<ExprAST> ArraySub, std::shared_ptr<ExprAST> Addr) :
		ArraySub(ArraySub), Addr(Addr) {
		this->Type = nullptr;
	};
	std::string getClassName() { return "ArraySubscrip"; };
	void print();
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
	
	ValTypeStruct Evaluate();

	ConditionConst conditionAna();
};

//break语句
class BreakAST :public ExprAST {
public:
	std::string getClassName() { return "Break"; };
	void print();
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
};

//continue语句
class ContinueAST :public ExprAST {
public:
	std::string getClassName() { return "Continue"; };
	void print();
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
};

//Return语句
class ReturnExprAST :public ExprAST {

public:
	ValType* Type;
	std::shared_ptr<ExprAST>Stmt;


	ReturnExprAST(ValType* Type,std::shared_ptr<ExprAST>Stmt) :Type(Type),Stmt(Stmt) {};
	std::string getClassName() { return "ReturnExpr"; };
	void print();
	ExprAST* getStmt() { return Stmt.get(); };
	ValType* getType() { return Type; };
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
};

//while循环
class WhileExprAST :public ExprAST {
public:
	std::shared_ptr<ExprAST>Condition;
	std::shared_ptr<ExprAST>Stmt;

	WhileExprAST(std::shared_ptr<ExprAST>Condition, std::shared_ptr<ExprAST>Stmt) :Condition(Condition), Stmt(Stmt) {};
	std::string getClassName() { return "WhileExpr"; };
	void print();
	ExprAST* getCondition() { return Condition.get(); };
	ExprAST* getStmt() { return Stmt.get(); };
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
	ConditionConst conditionAna();
};

//if-else条件
class IfElseExprAST :public ExprAST {

public:
	std::shared_ptr<ExprAST> Condition;//判断条件表达式
	std::shared_ptr<ExprAST> Stmts;//语句块
	std::shared_ptr<ExprAST> ElseStmt;//else语句


	IfElseExprAST(std::shared_ptr<ExprAST> Condition, std::shared_ptr<ExprAST> Stmts, std::shared_ptr<ExprAST> ElseStmt) :
		Condition(Condition), Stmts(Stmts), ElseStmt(ElseStmt) {};
	std::string getClassName() { return "IfElseExpr"; };
	void print();
	ExprAST* getCondition() { return Condition.get(); };
	ExprAST* getStmts() { return Stmts.get(); };
	ExprAST* getElseStmt() { return ElseStmt.get(); };
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
	ConditionConst conditionAna();
};

//浮点数
class FloatLiteralAST : public ExprAST {

public:
	ValType* Type;
	float Val;

	FloatLiteralAST(ValType* Type, float Val) :Type(Type), Val(Val) {};
	std::string getClassName() { return "FloatLiteral"; };
	ValType* getType() { return this->Type; };
	float getVal() { return this->Val; };
	void print();
	//语义分析
	ValType* AnaExpr();
	ValTypeStruct Evaluate();
	//控制流分析
	void analyzeFlow();

	ConditionConst conditionAna();
};

//整型
class IntegerLiteralAST : public ExprAST {

public:
	ValType* Type;
	int Val;

	IntegerLiteralAST(ValType* Type, int Val) : Type(Type), Val(Val) {};
	std::string getClassName() { return "IntegerLiteral"; };
	ValType* getType() { return this->Type; };
	int getVal() { return this->Val; };
	void print();
	//语义分析
	ValType* AnaExpr();
	ValTypeStruct Evaluate();
	//控制流分析
	void analyzeFlow();

	ConditionConst conditionAna();
};



//一元运算符（）
class UnaryExprAST : public ExprAST {
public:
	ValType* Type;//类型int float bool
	int Op;//运算符
	std::shared_ptr<ExprAST> RHS;//右运算数


	UnaryExprAST(ValType* type, int Op,
		std::shared_ptr<ExprAST> RHS)
		: Type(type), Op(Op), RHS(RHS) {};
	std::string getClassName() { return "UnaryExpr"; };
	int getOp() { return this->Op; };
	ValType* getType() { return this->Type; };
	ExprAST* getRHS() { return this->RHS.get(); };
	void print();
	//语义分析
	ValType* AnaExpr();
	ValTypeStruct Evaluate();
	//控制流分析
	void analyzeFlow();
	ConditionConst conditionAna();
};


//二元表达式，包括变量声明、运算表达式等等
class BinaryExprAST : public ExprAST {
public:
	ValType* Type;//类型int float bool
	int Op;//运算符
	std::shared_ptr<ExprAST> LHS, RHS;//左右运算数


	BinaryExprAST(ValType* type, int Op, std::shared_ptr<ExprAST> LHS,
		std::shared_ptr<ExprAST> RHS)
		: Type(type), Op(Op), LHS(LHS), RHS(RHS) {};
	std::string getClassName() { return "BinaryExpr"; };
	int getOp() { return this->Op; };
	ValType* getType() { return this->Type; };
	ExprAST* getLHS() { return this->LHS.get(); };
	ExprAST* getRHS() { return this->RHS.get(); };
	void print();
	//语义分析
	ValType* AnaExpr();
	ValType* JudgeType(ValType* otherType, ValType* resultType);
	ValTypeStruct Evaluate();
	//控制流分析
	void analyzeFlow();
	ConditionConst conditionAna();
};

//函数体内的声明语句，一句内可能包含多个声明
class DeclStmtAST :public ExprAST {

public:
	std::vector<std::shared_ptr<ExprAST>> ValDecls;
	

	DeclStmtAST(std::vector<std::shared_ptr<ExprAST>> ValDecls) :ValDecls(ValDecls){};
	std::string getClassName() { return "DeclStmt"; };
	std::vector<ExprAST*> getValDecls() {
		std::vector<ExprAST*> result;
		for (const auto& expr : ValDecls) {
			result.push_back(expr.get());
		}
		return result;
	};
	void print();
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
};


//函数体语句块
class CompoundStmtAST :public ExprAST {

public:
	std::vector<std::shared_ptr<ExprAST>> Stmts;//存放函数体内的各种语句，如Decl，If......
	Scope* scope;

	CompoundStmtAST(Scope* scope,std::vector<std::shared_ptr<ExprAST>> Stmts) :scope(scope),Stmts(Stmts){};
	std::string getClassName() { return "CompoundStmt"; };
	std::vector<ExprAST*> getStmts() {
		std::vector<ExprAST*> result;
		for (const auto& expr : Stmts) {
			result.push_back(expr.get());
		}
		return result;
	};
	void print();
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
	ConditionConst conditionAna();
};


//隐式类型转换：当发现需要类型转换后，ValDecl的CastOrVal指向这个节点
class ImplCastExprAST :public ExprAST {
public:
	ValType* FromType;//原始类型
	ValType* ToType;//要转换成的类型
	std::shared_ptr<ExprAST> Val;//float或int或 表达式(二元 or 一元) 或变量引用（数组）

	//生成dag用变量
	std::string CastString;


	ImplCastExprAST(ValType* FType,ValType* Type, std::shared_ptr<ExprAST> Val) :FromType(FType),ToType(Type), Val(Val){};
	ImplCastExprAST(ValType* FType, ValType* Type, std::shared_ptr<ExprAST> Val, std::string& CastString) :FromType(FType), ToType(Type), Val(Val), CastString(CastString){};
	std::string getClassName() { return "ImplCastExpr"; };
	void print();
	ExprAST* getVal() { return Val.get(); };
	ValType* getFromType() { return this->FromType; };
	ValType* getToType() { return this->ToType; };
	//语义分析
	ValTypeStruct Evaluate();
	//控制流分析
	void analyzeFlow();

	ConditionConst conditionAna();
};


//函数，包括声明和函数体
class FunctionAST :public ExprAST {
public:
	FuncType* Type;//函数类型，包含返回值和函数参数的类型
	std::string Name;//函数名
	std::vector<std::shared_ptr<ExprAST>> Params;//参数，ParamAST
	std::shared_ptr<ExprAST> CompoundStmt;//函数体
	Symbol* symbol;//函数在符号表中的位置


	FunctionAST(FuncType* Type, const std::string& Name, std::vector<std::shared_ptr<ExprAST>> Params,
		std::shared_ptr<ExprAST> CompoundStmt)
		: Type(Type), Name(Name), Params(Params),CompoundStmt(CompoundStmt) {};
	std::string getClassName() { return "Function"; };
	FuncType* getFunctype() { return Type; };
	std::string getName() { return Name; };
	std::vector<ExprAST*> getParams() {
		std::vector<ExprAST*> result;
		for (const auto& expr : Params) {
			result.push_back(expr.get());
		}
		return result;
	};
	ExprAST* getCompoundStmt() {
		return CompoundStmt.get();
	}
	void print();

	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
	ConditionConst conditionAna();
};


//变量声明语句
class ValDeclAST :public ExprAST {
public:
	ValType* valType;//类型
	std::string Name;//变量名
	std::shared_ptr<ExprAST> Val;//隐式类型转换或直接赋值（无需转换），还包括数组的initlist
	Symbol* symbol;//声明的变量在符号表中的位置
	
	ValDeclAST(ValType* valType, const std::string& Name,std::shared_ptr<ExprAST> Val) :
		valType(valType), Name(Name), Val(Val){};
	std::string getClassName() { return "ValDecl"; };
	ValType* getType() { return valType; };
	std::string getName() { return Name; };
	ExprAST* getCastOrVal() {return Val.get();};
	void print();
	//语义分析
	ValType* AnaExpr();
	int getIElement(void* array, const std::vector<int>& indices);
	float getFElement(void* array, const std::vector<int>& indices);
	//控制流分析
	void analyzeFlow();
};


//编译单元，树的根结点
class CompUnitAST : public ExprAST {
public:
	std::vector<std::shared_ptr<ExprAST>> TopExpr;//包括顶层变量声明ValDeclAST和函数定义FunctionAST


	CompUnitAST(std::vector<std::shared_ptr<ExprAST>> TopExpr) :TopExpr(TopExpr) {};
	std::vector<ExprAST*> getTopExpr() {
		std::vector<ExprAST*> result;
		for (const auto& expr : TopExpr) {
			result.push_back(expr.get());
		}
		return result;
	};//***********************std::move的问题：解决方法，用expr.get()返回一系列原始指针而非返回unique_ptr
	std::string getClassName() { return "CompUnit"; };
	void print();
	//语义分析
	ValType* AnaExpr();
	//控制流分析
	void analyzeFlow();
	ConditionConst conditionAna();
};



//AST管理类
class ASTContext {

public:
	std::shared_ptr<ExprAST> CompileUnit;

	ASTContext() {}
	void setCompileUnit(std::shared_ptr<ExprAST> CU) {
		CompileUnit = CU;
	}
	ExprAST* getCompileUnit() {
		return CompileUnit.get();
	};

	//自上而下先根遍历，打印语法树
	void visit(ExprAST* Expr, int level) ;
	void visitCompUnit(ExprAST* Expr, int level);
	void visitValDecl(ExprAST* Expr, int level) ;
	void visitBinaryExpr(ExprAST* Expr, int level);
	void visitUnaryExpr(ExprAST* Expr, int level);
	void visitIntegerExpr(ExprAST* Expr, int level);
	void visitFloatExpr(ExprAST* Expr, int level);
	void visitFunctionExpr(ExprAST* Expr, int level);
	void visitParamDeclExpr(ExprAST* Expr, int level);
	void visitDeclRefExpr(ExprAST* Expr, int level);
	void visitDeclStmtExpr(ExprAST* Expr, int level);
	void visitCompoundStmtExpr(ExprAST* Expr, int level);
	void visitReturnStmtExpr(ExprAST* Expr, int level);
	void visitIfElseExpr(ExprAST* Expr, int level);
	void visitWhileExpr(ExprAST* Expr, int level);
	void visitBreak(ExprAST* Expr, int level);
	void visitContinue(ExprAST* Expr, int level);
	void visitCall(ExprAST* Expr, int level);
	void visitImplicCast(ExprAST* Expr, int level);
	void visitInitList(ExprAST* Expr, int level);
	void visitImplicitValueInit(ExprAST* Expr, int level);
	void visitArraySubscrip(ExprAST* Expr, int level);
	void visitStringAST(ExprAST* Expr, int level);
};

#endif // !AST_H




