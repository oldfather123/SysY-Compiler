#pragma warning(disable : 4996)
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Frontend/Parser.h"
#include "Frontend/SemanticAnalyzer.h"
#include "Frontend/ConditionAna.h"
#include "Frontend/CFA.h"
#include "Frontend/InlineOpt.h"
#include "Frontend/FrontendOtherOpt.h"
#include "SSA/GSSA.h"
#include "DFA/CSE.h"
#include "DFA/DFA.h"
#include "DFA/InterferenceGraph.h"
#include "RISC_V/Linearize.h"
#include "RISC_V/InstructionSelection.h"
#include "RISC_V/RegisterAllocate.h"
#include "RISC_V/InstructionGenerate.h"
#include "RISC_V/Generate.h"
#include "DFA/LoopOpimization.h"


int main(int argc, const char* argv[])
{
	//是否开启优化
	int Opt = 0;
	if (argc == 6) {
		Opt = 1;
	}

	//词法源文件
	FILE* lsource = fopen(argv[4], "r");
	//FILE* lsource = fopen("Frontend/lsource.sy", "r");

	//管理所有字符
	std::vector<std::string> StrVector;

	if (lsource == NULL) {
		//printf("Can't open the file");
		return -1;
	}
	
	//ctx用来管理AST所有的属性
	ASTContext* ctx = new ASTContext();
	//符号表存储符号
	SymbolTable* symbolTable = new SymbolTable();
	std::shared_ptr<ExprAST>CU= MainLoop(lsource,ctx,symbolTable, StrVector);
	if (!CU) {
		//printf("not visit\n");
		//system("pause");
		return 0;
	}
	ctx->setCompileUnit(CU);
	//ctx->visit(ctx->getCompileUnit(), -1);

	fclose(lsource);

	if (SemanticAnalyze(ctx) == -1) {
		//printf("error SemanticAnalyze\n");
		//return 0;
	}

	//printf("\n");
	//ctx->visit(ctx->getCompileUnit(), -1);

	ConditionAna(ctx);

	//printf("\n");
	//ctx->visit(ctx->getCompileUnit(), -1);

	//前端控制流分析
	GCFG * gcfg=analyzeControlFlow(ctx->CompileUnit);
	
	//函数内联
	map<string, vector<string>> useGlobals = inlineOptimization(symbolTable, gcfg, CU,Opt);


//###########################################################################
//消除显而易见的假跳转
	//FrontendDecreaseBranch(gcfg);

	//前端优化
	ExpManager* expManager = new ExpManager();
	expManager->calculate_constantExp(gcfg);
	expManager->elimanate_UnaryExp(gcfg);
	expManager->elimanate_UnaryExp(gcfg);
	
	//循环分析
	LoopOptManager* loopManager = new LoopOptManager();
	//循环展开
	//loopManager->LoopUnrolling(gcfg, symbolTable);

	//SSA生成
	GSSA* gssa = new GSSA(gcfg, symbolTable);
	//gssa->GSSA_PUMP();
	gssa->GSSA_WORK();

	////基本块排序
	//for (auto it : gcfg->controlFlow) {
	//	it.second->TopoSorting();
	//}

	//数据流优化
	DFA* dfa = new DFA(gcfg, symbolTable);
	dfa->pass(useGlobals, Opt);
	
	//loopManager->LICM_OPT(gcfg, dfa,symbolTable);

	//获取循环树
	loopManager->getLoopTree(gcfg);
	//给每一个块标循环层级
	loopManager->getLoopLevel(gcfg);


	//循环不变式外提
	//LICM_OPT(dfa);

	
	//
	gcfg->getArrayStack(symbolTable);
	//gcfg->getLoopLevel();
	//指令选择
	instructionSelection(gcfg, symbolTable);
	
	dfa->mir_pass(useGlobals, Opt);
	//SSA重建
	gssa->GSSA_DESTROY();

	//超过的形参放在栈上，排序，保护寄存器提前溢出
	dfa->dfa_topo();

	gssa->GSSA_TOPOSORTING();
	//寄存器分配
	graphColoringRegisterAllocation(gcfg, dfa, symbolTable);

	//线性化和窥孔
	Linear* linear = new Linear(gcfg, dfa);

	//instructionGenerate(gcfg, dfa, linear, "test.txt");
	instructionGenerate(gcfg, dfa, linear, argv[3]);

	////添加字符串
	//for(int i = 0; i < StrVector.size(); i++) {
	//	addglobalstring("S" + to_string(i), StrVector[i]);
	//}
	//添加全局变量
	globalVar(symbolTable);

	//system("pause");
	return 0;
}

