#pragma once
#include <string>
#include <vector>

namespace LinearIR {
	enum ValType {
		Int,
		Float,
		IntConst,
		FloatConst,
		Void
	};

	//中间代码类型
	enum IRType {
		Label,					//标签
		Experssion,				//表达式
		FuncDef,				//函数定义开始
		FuncEnd,				//函数定义结束
		FuncCall,				//函数调用（暂未想好如何表示传入参数）
		Return,					//函数返回
		ValReturn,				//带参数的函数返回（暂未想好如何表示返回值）
		//变量访存放在表达式类型中
		//ValLoad,				//变量访问
		//ValStore,				//变量存储
		Arrload,				//数组访问
		ArrStore,				//数组存储
		GlobalVarDef,			//全局变量定义
		GlobalArrDef,			//全局数组定义
		JAL,					//直接跳转
		BEQ,					//条件跳转
	};

	////符号中间代码作用域
	//enum IRScope {
	//	Unused,					//未使用
	//	Global,					//全局符号
	//	Temp,
	//	Local,
	//};

	//中间代码操作符
	enum IROper {
		Null,					//没有操作符
		Add,					//	+
		Sub,					//	-
		Mul,					//	*
		Div,					//	/
		Mod,					//	%
		Equal,					//	==
		NEqual,					//	!=
		Greater,				//	>
		Less,					//	<
		GreaterAndEqual,		//	>=
		LessAndEqual,			//	<=
		Not,					//	!
		And,					//	&&
		Or,						//	||
		Load,					//	访问
		Store,					//	存储
		Assign					//	=
	};

	//表达式类型虚拟寄存器编号数组含义
	//操作数1，操作数2，目标
	const int rd = 0;
	const int op1 = 1;
	const int op2 = 2;

	struct IRExperssion {
		IROper oper;							//表达式运算符
		//如果表达式为带常数的操作，将该寄存器编号改为-1
		//如果两者都是常数，希望上层可以做处理，将所得值直接进行替换，修正为一个常数
		ValType optype;							//操作数类型
		std::vector < long > reCount[3];		//虚拟寄存器编号数组
	};

	//中间代码单元
	struct IRUnit {
		IRType type;					//中间代码类型
		std::string label;				//若为标签类型的标签名称,跳转类型、函数调用的目标
		IRExperssion expe;				//表达式类型的表达式,条件跳转也使用
		std::vector<long> parameters;	//目前设想的传递函数参数方式，用虚拟寄存器编号数组
	};

	class IR {
	public:
		std::vector<IRUnit> IR;
	};

}
