#include "InlineOpt.h"
#include <algorithm>
#include <unordered_map>

#define MAX_BLOCK 2
#define LIB_FUNC_BLOCKNUM 100

#define MAX_INLINE_TIMES 1

//工具函数开始******************

//判断是不是函数调用
static std::vector<std::string> CommonOPNameVec = { PLUS ,SUBTRACT,MULTIPLY,DIVIDE,GREATER_THAN,GREATER_EQUAL ,LESS_THAN ,LESS_EQUAL,EQUAL,NOT_EQUAL ,
									LOGICAL_NOT,LOGICAL_AND,LOGICAL_OR,REMAINDER,RETURN_NODE,BRANCH_NODE,ARRAY_INIT_NODE,IMPLICIT_INIT_NODE,ARRAYSUB_NODE,INT_TO_FLOAT ,FLOAT_TO_INT,ARRAY_TO_POINT};
static bool isCalleeNode(std::string name) {
	for (std::string opName : CommonOPNameVec) {
		if (name == opName)
			return false;
	}
	return true;
}

//copy了除了inputs/outputs/reusltId的成员
DAGNode* copyNewDAGNode(DAGNode* node) {
	DAGNode* newNode = new DAGNode(" ", DAGNodeType::CONSTANT);
	newNode->name = node->name;
	newNode->nodeType = node->nodeType;
	newNode->inst = node->inst;
	newNode->isConst = node->isConst;
	newNode->offset = node->offset;
	newNode->parameter = node->parameter;
	newNode->symbol = node->symbol;
	newNode->targets = node->targets;
	newNode->value = node->value;
	return newNode;
}


//工具函数结束******************

static GCFG* gcfg;
static SymbolTable* symbolTable;

typedef struct FuncNode {
	string name;
	map<string, int> callTimes;
	map<string, int> useGlobalVariable;
	int blockNums;
	bool canInline;
	bool isLibFunc = false;
	FuncNode(string name) :name(name) ,blockNums(0),canInline(false) {};
}FuncNode;

static vector<FuncNode*> FuncInfo;
static vector<int> isVisit;

static void initLibFuncInfo()
{
	FuncNode* node_gi = new FuncNode(GET_INT);
	node_gi->isLibFunc = true;
	FuncInfo.push_back(node_gi);

	FuncNode* node_ch = new FuncNode(GET_CH);
	node_ch->isLibFunc = true;
	FuncInfo.push_back(node_ch);

	FuncNode* node_f = new FuncNode(GET_FLOAT);
	node_f->isLibFunc = true;
	FuncInfo.push_back(node_f);

	FuncNode* node_a = new FuncNode(GET_ARRAY);
	node_a->isLibFunc = true;
	FuncInfo.push_back(node_a);

	FuncNode* node_fa = new FuncNode(GET_F_ARRAY);
	node_fa->isLibFunc = true;
	FuncInfo.push_back(node_fa);

	FuncNode* node_pi = new FuncNode(PUT_INT);
	node_pi->isLibFunc = true;
	FuncInfo.push_back(node_pi);

	FuncNode* node_pc = new FuncNode(PUT_CHAR);
	node_pc->isLibFunc = true;
	FuncInfo.push_back(node_pc);

	FuncNode* node_pf = new FuncNode(PUT_FLOAT);
	node_pf->isLibFunc = true;
	FuncInfo.push_back(node_pf);

	FuncNode* node_PA = new FuncNode(PUT_ARRAY);
	node_PA->isLibFunc = true;
	FuncInfo.push_back(node_PA);

	FuncNode* node_pfa = new FuncNode(PUT_F_ARRAY);
	node_pfa->isLibFunc = true;
	FuncInfo.push_back(node_pfa);

	FuncNode* node_pfo = new FuncNode(PUT_F);
	node_pfo->isLibFunc = true;
	FuncInfo.push_back(node_pfo);

	FuncNode* node_st = new FuncNode(START_TIME);
	node_st->isLibFunc = true;
	FuncInfo.push_back(node_st);

	FuncNode* node_sto = new FuncNode(STOP_TIME);
	node_sto->isLibFunc = true;
	FuncInfo.push_back(node_sto);
}

//从FuncInfo中找到对应函数info
static FuncNode* getFunInfo(string name) {
	for (FuncNode* node : FuncInfo) {
		if (node->name == name)
			return node;
	}
	return nullptr;
}

//检查dag中的函数调用，和全局变量使用（如果调用了函数，还要加入其的全局变量）
void checkDag(DAG* dag, FuncNode* funcnode)
{
	for (DAGNode* dagnode : dag->nodes) {
		if (dagnode->nodeType == DAGNodeType::OP_NODE && isCalleeNode(dagnode->name)) {
			funcnode->callTimes[dagnode->name]++;
			FuncNode* func = getFunInfo(dagnode->name);
			for (auto it : func->useGlobalVariable) {
				funcnode->useGlobalVariable[it.first] += it.second;
			}
		}		
		if (dagnode->nodeType == DAGNodeType::STORE && dagnode->inputs[0]->nodeType == DAGNodeType::VALUE_NODE && symbolTable->isGlobalSymbol(dagnode->inputs[0]->symbol))
			funcnode->useGlobalVariable[dagnode->inputs[0]->name] = 1;
	}
}

//递归访问每一个基本块
void checkBlock(BasicBlock* block,FuncNode* funcnode)
{
	checkDag(block->dag,funcnode);
	if (block->postBlock.size() != 0) {
		for (BasicBlock* blockToVisit : block->postBlock) {
			//如果还没有访问过，那么就访问
			if (!isVisit[blockToVisit->id]) {
				isVisit[blockToVisit->id] = 1;
				checkBlock(blockToVisit,funcnode);
			}
		}
	}
}

//检查一个函数的调用关系
void checkUseChain(string name,CFG* cfg)
{
	//遍历所有的基本块以及dagnode，找到函数调用的总次数，算出各个函数的引用次数
	//暂定规则：短函数可以被内联；多次被调用的函数可以考虑内联
	FuncNode* funcnode = new FuncNode(name);
	FuncInfo.push_back(funcnode);
	isVisit.clear();
	isVisit.resize(cfg->basicBlocks.size(), 0);
	checkBlock(cfg->entry,funcnode);
	//遍历结束以后信息存在map中
	funcnode->blockNums = cfg->basicBlocks.size();
}


//存一条语句中使用的全局变量
static map<string,int> useGlobalVariable;

//统计在一个函数中其他函数已经被插入的次数
static map<string, int> calleeTimes;

static map<Symbol*, Symbol*> origin_Stmbol_To_NewSymbol_Map;
static map<Scope*, Scope*>orgin_Scope_To_NewScope_Map;

//由于内联是循环完成的，当一次遍历中不再出现内联，那么就停止
static bool iaChangedInThisTerm = false;

//*******************开始改变控制流

//记录一个节点是不是被拷贝了，如果拷贝了，就返回拷贝后的节点，没有的话返回nullptr
static map<DAGNode*, DAGNode*> hasCopiedNode;
static DAGNode* judgeIfCopied(DAGNode* node) {
	auto it = hasCopiedNode.find(node);
	if (it != hasCopiedNode.end()) {
		return it->second;
	}
	return nullptr;
}

//拷贝一段dagnode
//mode 0 为block拷贝时，mode 1 为afterblock拷贝时
static DAGNode* copyDagnode(DAGNode* copyRoot,int mode)
{
	if (!copyRoot)return nullptr;
	DAGNode* newNode = judgeIfCopied(copyRoot);
	if (!newNode) {
		//没有拷贝过，拷贝本节点
		if (copyRoot->symbol) {
			if (mode == 0) {
				//还要处理全局变量
				if (symbolTable->isGlobalSymbol(copyRoot->symbol)) {
					newNode = new DAGNode(copyRoot, copyRoot->symbol);
				}
				else {
					newNode = new DAGNode(copyRoot, origin_Stmbol_To_NewSymbol_Map[copyRoot->symbol]);
				}				
			}
			else {
				newNode = new DAGNode(copyRoot, copyRoot->symbol);
			}		
		}
		else {
			newNode = new DAGNode(copyRoot, nullptr);
		}
		//加入已经拷贝过的map
		hasCopiedNode[copyRoot] = newNode;
	}
	else {
		return newNode;
	}
	
	//拷贝依赖节点
	if (copyRoot->relyNodes.size() != 0) {
		for (size_t i = 0;i < copyRoot->relyNodes.size();i++) {
			DAGNode* returnNode = copyDagnode(copyRoot->relyNodes[i],mode);
			newNode->relyNodes.push_back(returnNode);
		}
	}

	//拷贝输入节点
	if (copyRoot->inputs.size() != 0) {
		for (size_t i = 0;i < copyRoot->inputs.size();i++) {
			DAGNode* returnNode = copyDagnode(copyRoot->inputs[i],mode);
			newNode->inputs.push_back(returnNode);
			returnNode->outputs.push_back(newNode);
		}
	}

	return newNode;
}

//深度优先递归，复制符号表，除了函数参数
void insertSymbolsDeep(Scope* scope,string& calleeName)
{
	for (auto it : scope->Symbols) {
		Symbol* copySymbol = new Symbol(it.second);
		//改名字，根据调用次数
		string name = it.second->name;
		name += "@";
		name += calleeName;
		name += to_string(calleeTimes[calleeName]);
		copySymbol->name = name;
		copySymbol->currentscope = symbolTable->CurrentScope;

		symbolTable->AddSymbol(name, copySymbol);
		//将复制的符号对应起来，复制dagnode的时候可以查找
		origin_Stmbol_To_NewSymbol_Map[it.second] = copySymbol;		
	}
	for (size_t i = 0;i < scope->childs.size();i++) {
		symbolTable->EnterScope();
		orgin_Scope_To_NewScope_Map[scope->childs[i]] = symbolTable->CurrentScope;
		insertSymbolsDeep(scope->childs[i], calleeName);
		symbolTable->ExitScope();
	}
}

//插入符号表
void copySymbols(Scope* insertTargetScope,string calleeName)
{
	Scope* calleeScope = symbolTable->getFuncScope(calleeName);

	symbolTable->CurrentScope = insertTargetScope;
	symbolTable->EnterScope();
	orgin_Scope_To_NewScope_Map[calleeScope] = symbolTable->CurrentScope;
	insertSymbolsDeep(calleeScope,calleeName);
}


//记录一个基本块是不是被拷贝了，如果拷贝了，就返回拷贝后的块，没有的话返回nullptr
map<BasicBlock*, BasicBlock*> hasCopiedBlock;
BasicBlock* judgeIfCopied(BasicBlock* block) {
	auto it = hasCopiedBlock.find(block);
	if (it != hasCopiedBlock.end()) {
		return it->second;
	}
	return nullptr;
}

//插入的cfg的结尾块
static vector<BasicBlock*> endBlocks;

//重新恢复dag的nodes成员
void reBuildNodes(DAG* dag,DAGNode* node)
{
	if (!node)return;
	if (node->relyNodes.size() != 0) {
		for (size_t i = 0;i < node->relyNodes.size();i++) {
			reBuildNodes(dag,node->relyNodes[i]);
		}
	}

	if (node->inputs.size() != 0) {
		for (size_t i = 0;i < node->inputs.size();i++) {
			reBuildNodes(dag, node->inputs[i]);
		}
	}

	for (size_t i = 0;i < dag->nodes.size();i++) {
		if (dag->nodes[i] == node) return;
	}
	dag->nodes.push_back(node);
}

//递归拷贝基本块
BasicBlock* insertTargetBlock(CFG* sourcecfg, CFG* targetcfg,BasicBlock* block,Symbol* returnSymbol)
{
	BasicBlock* newBlock = judgeIfCopied(block);
	if (!newBlock) {
		newBlock = new BasicBlock(block,orgin_Scope_To_NewScope_Map[block->scope]);

		hasCopiedNode.clear();
		newBlock->dag->rootNode = copyDagnode(block->dag->rootNode,0);	
		//将新的基本块插入cfg，并且重新编号
		sourcecfg->basicBlocks.push_back(newBlock);
		newBlock->id = sourcecfg->basicBlocks.size() - 1;
		if (block->postBlock.size() == 0) {
			if (newBlock->dag->rootNode && newBlock->dag->rootNode->name == RETURN_NODE) {
				//修改return节点
				if (newBlock->dag->rootNode->inputs.size() != 0) {
					DAGNode* newValNode = getNewValNode(returnSymbol->name, returnSymbol);
					DAGNode* storeNode = new DAGNode(" ", DAGNodeType::STORE);
					storeNode->inputs.push_back(newValNode);
					newValNode->outputs.push_back(storeNode);
					storeNode->inputs.push_back(newBlock->dag->rootNode->inputs[0]);
					newBlock->dag->rootNode->inputs[0]->outputs.push_back(storeNode);
					newBlock->dag->remove_edge(newBlock->dag->rootNode, newBlock->dag->rootNode->inputs[0]);

					storeNode->relyNodes = newBlock->dag->rootNode->relyNodes;
					newBlock->dag->rootNode = storeNode;
				}
				else {
					if (newBlock->dag->rootNode->relyNodes.size() != 0) {
						newBlock->dag->rootNode = newBlock->dag->rootNode->relyNodes[0];
					}
					else {
						newBlock->dag->rootNode = nullptr;
					}
				}
			}
			//作为结尾节点
			endBlocks.push_back(newBlock);
			hasCopiedBlock[block] = newBlock;
			//恢复nodes
			newBlock->dag->nodes.clear();
			reBuildNodes(newBlock->dag, newBlock->dag->rootNode);
			//没有后继，可以直接返回
			return newBlock;
		}
		newBlock->dag->nodes.clear();
		reBuildNodes(newBlock->dag, newBlock->dag->rootNode);
		hasCopiedBlock[block] = newBlock;
	}
	else {
		return newBlock;
	}

	if (block->postBlock.size() != 0) {
		for (BasicBlock* subBlock : block->postBlock) {
			BasicBlock* returnBlock = insertTargetBlock(sourcecfg, targetcfg, subBlock, returnSymbol);
			newBlock->postBlock.push_back(returnBlock);
			returnBlock->preBlock.push_back(newBlock);
			newBlock->dag->rootNode->targets.push_back(returnBlock->id);
		}
	}

	return newBlock;
}

BasicBlock* insertTargetCFG(CFG* sourcecfg, CFG* targetcfg, string funcname,Symbol* returnSymbol)
{
	return insertTargetBlock(sourcecfg, targetcfg, targetcfg->entry,returnSymbol);

}

//将返回语句改为store，新建变量
DAGNode* getNewValNode(string name, Symbol* symbol)
{
	DAGNode* node = new DAGNode(name, DAGNodeType::VALUE_NODE);
	node->symbol = symbol;
	node->resultType = symbol->type;
	return node;
}

//在插入cfg之前添加参数
void transParams(BasicBlock* paramBlock, DAGNode* callee)
{
	//1.先找到函数的参数
	vector<Symbol*> params = symbolTable->getFuncSymbols(callee->name);
	//2.根据函数的输入参数，找到新的符号进行插入
	for (int i = 0;i < callee->inputs.size();i++) {
		Symbol* newSymbol = origin_Stmbol_To_NewSymbol_Map[params[i]];
		DAGNode* valnode = getNewValNode(newSymbol->name, newSymbol);
		DAGNode* storeNode = new DAGNode(" ", DAGNodeType::STORE);

		storeNode->inputs.push_back(valnode);
		valnode->outputs.push_back(storeNode);

		storeNode->inputs.push_back(callee->inputs[i]);
		auto it = find(callee->inputs[i]->outputs.begin(), callee->inputs[i]->outputs.end(), callee);
		if (it != callee->inputs[i]->outputs.end())
			callee->inputs[i]->outputs.erase(it);
		callee->inputs[i]->outputs.push_back(storeNode);

		if (!paramBlock->dag->rootNode)
			paramBlock->dag->rootNode = storeNode;
		else {
			storeNode->relyNodes.push_back(paramBlock->dag->rootNode);
			paramBlock->dag->rootNode = storeNode;
		}
	}
}

//插入cfg的入口函数
void insertCFG(CFG* sourceCFG,BasicBlock* sourceBlock,DAGNode* rootNode,DAGNode* callee,string targetName,DAGNode* preRely)
{
	
	int id = sourceCFG->basicBlocks.size() - 1;
	//先拆分基本块
	//1.创建新的基本块
	BasicBlock* afterBlock = new BasicBlock(++id,sourceBlock->scope);
	sourceCFG->basicBlocks.push_back(afterBlock);
	//2.拆分基本块，保留sourceblock的跳转到afterblock
	afterBlock->postBlock = sourceBlock->postBlock;
	afterBlock->dag->rootNode = sourceBlock->dag->rootNode;

	//*****************修改原先后继的preblock为现在的afterblock**********
	for (auto tblock : afterBlock->postBlock) {
		auto it = find(tblock->preBlock.begin(), tblock->preBlock.end(), sourceBlock);
		(*it) = afterBlock;
	}

	//针对特殊基本块进行处理
	if (sourceBlock->label == BasicBlock::LabelType::WHILEBODY||sourceBlock->label == BasicBlock::LabelType::IF_END_AND_WHILEBODY) {
		afterBlock->label = sourceBlock->label;
		sourceBlock->label = BasicBlock::LabelType::NONE;
		
		/*BasicBlock* conditionBlock = afterBlock->postBlock[0];
		auto it = find(conditionBlock->preBlock.begin(), conditionBlock->preBlock.end(), sourceBlock);
		conditionBlock->preBlock.erase(it);
		conditionBlock->preBlock.push_back(afterBlock);*/
	}
	else if (sourceBlock->label == BasicBlock::LabelType::IF_END) {
		afterBlock->label = sourceBlock->label;
		sourceBlock->label = BasicBlock::LabelType::NONE;
	}

	if (rootNode->relyNodes.size() != 0) {
		sourceBlock->dag->rootNode = rootNode->relyNodes[0];
		rootNode->relyNodes.clear();
		if (rootNode == callee) {
			preRely->relyNodes.clear();
		}
		hasCopiedNode.clear();
		sourceBlock->dag->rootNode = copyDagnode(sourceBlock->dag->rootNode, 1);
	}
	else {
		if (rootNode == callee) {
			preRely->relyNodes.clear();
		}
		sourceBlock->dag->rootNode = nullptr;
	}

	//3.拷贝符号表
	copySymbols(sourceBlock->scope, targetName);

	//4.在ParamsBlock中加入传参
	BasicBlock* paramBlock = new BasicBlock(++id, orgin_Scope_To_NewScope_Map[symbolTable->getFuncScope(targetName)]);
	sourceCFG->basicBlocks.push_back(paramBlock);
	transParams(paramBlock, callee);

	//5.新建返回变量
	//如果被插入函数没有返回值节点的符号的话，添加之
	Scope* scope = sourceBlock->scope;
	//获取名字
	string name = callee->name + "@";
	name += std::to_string(calleeTimes[callee->name]);
	calleeTimes[callee->name]++;
	//将返回语句改为这个symbol的store语句,这个变量的符号在函数的第一个scope
	Symbol* returnSymbol;
	auto it = scope->Symbols.find(name);
	if (it != scope->Symbols.end()) {
		returnSymbol = it->second;
	}
	else {
		Symbol* funcSymbol = symbolTable->getGlobalSymbol(callee->name);
		returnSymbol = new Symbol();
		returnSymbol->name = name;
		returnSymbol->type = funcSymbol->returnType;
		returnSymbol->currentscope = scope;
		//如果是void就不加入符号表了，没有必要
		if(returnSymbol->returnType != ResultType::VOID)
			scope->AddSymbol(name, returnSymbol);
	}

	//保存callee的名字
	string calleeName = callee->name;

	if (rootNode != callee) {
		//先修改callee节点，查找其在其输出节点inputs中的位置
		DAGNode* beforeCallee = callee->outputs[0];//现在只考虑有上下符号的情况**********
		auto callIT = find(beforeCallee->inputs.begin(), beforeCallee->inputs.end(), callee);
		callee = getNewValNode(returnSymbol->name, returnSymbol);
		DAGNode* loadNode = new DAGNode(" ", DAGNodeType::LOAD);
		loadNode->resultType = callee->resultType;
		loadNode->inputs.push_back(callee);
		callee->outputs.push_back(loadNode);
		(*callIT) = loadNode;
	}
	
	hasCopiedNode.clear();
	afterBlock->dag->rootNode = copyDagnode(afterBlock->dag->rootNode, 1);
	for (BasicBlock* block : afterBlock->postBlock) {
		afterBlock->dag->rootNode->targets.push_back(block->id);
	}


	//6.插入callee的控制流，添加其内部跳转
	BasicBlock* returnBlock = insertTargetCFG(sourceCFG, gcfg->controlFlow[calleeName], calleeName, returnSymbol);
	//7.添加插入前后跳转
	//(1)从source块到条件块
	DAGNode* toParamBlock = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
	toParamBlock->targets.push_back(paramBlock->id);
	if (!sourceBlock->dag->rootNode) {
		sourceBlock->dag->rootNode = toParamBlock;
	}
	else {
		toParamBlock->relyNodes.push_back(sourceBlock->dag->rootNode);
		sourceBlock->dag->rootNode = toParamBlock;
	}
	sourceBlock->postBlock.clear();
	sourceBlock->postBlock.push_back(paramBlock);
	paramBlock->preBlock.push_back(sourceBlock);

	DAGNode* branchNode = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
	branchNode->targets.push_back(returnBlock->id);
	if (!paramBlock->dag->rootNode) {
		paramBlock->dag->rootNode = branchNode;
	}
	else {
		branchNode->relyNodes.push_back(paramBlock->dag->rootNode);
		paramBlock->dag->rootNode = branchNode;
	}
	paramBlock->postBlock.push_back(returnBlock);
	returnBlock->preBlock.push_back(paramBlock);

	for (BasicBlock* basicBlock : endBlocks) {
		//1.添加前后继块
		basicBlock->postBlock.push_back(afterBlock);
		afterBlock->preBlock.push_back(basicBlock);

		//2.添加branch节点
		if (!basicBlock->dag->rootNode) {
			DAGNode* branchNode = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
			branchNode->targets.push_back(afterBlock->id);
			basicBlock->dag->rootNode = branchNode;
		}
		else {
			DAGNode* branchNode = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
			branchNode->targets.push_back(afterBlock->id);
			branchNode->relyNodes.push_back(basicBlock->dag->rootNode);
			basicBlock->dag->rootNode = branchNode;
		}
		basicBlock->dag->nodes.clear();
		reBuildNodes(basicBlock->dag, basicBlock->dag->rootNode);
	}

	//8.维护nodes
	sourceBlock->dag->nodes.clear();
	reBuildNodes(sourceBlock->dag, sourceBlock->dag->rootNode);

	paramBlock->dag->nodes.clear();
	reBuildNodes(paramBlock->dag, paramBlock->dag->rootNode);

	afterBlock->dag->nodes.clear();
	reBuildNodes(afterBlock->dag, afterBlock->dag->rootNode);

	//9.维护condition的真出口和假出口：如果有while(1) 的 情况 会出现 nullptr
	int startId = paramBlock->id + 1;
	for (int i = startId;i < sourceCFG->basicBlocks.size();i++) {
		if (sourceCFG->basicBlocks[i]->label == BasicBlock::LabelType::WHILE_CONDITION) {
			BasicBlock* block = sourceCFG->basicBlocks[i];
			block->True_False_Exit.first = hasCopiedBlock[block->True_False_Exit.first];
			block->True_False_Exit.second = hasCopiedBlock[block->True_False_Exit.second];
		}
	}
}

//*******************结束改变控制流


//*****************开始查找插入点

//访问每一个语句的非开头结点
DAGNode* travelCommonNode(DAGNode* dagnode)
{
	if (dagnode->inputs.size() != 0) {
		for (int i = 0;i < dagnode->inputs.size();i++) {
			DAGNode* returnNode = travelCommonNode(dagnode->inputs[i]);
			if (returnNode)
				return returnNode;
		}
	}
	//如果是函数调用节点，判断能不能内联
	if (dagnode->nodeType == DAGNodeType::OP_NODE && isCalleeNode(dagnode->name)) {
		FuncNode* func = getFunInfo(dagnode->name);
		if(func->canInline) {
			//判断在此之前使用到的全局变量和函数调用中使用到的有没有冲突
			for (auto it : useGlobalVariable) {
				if (it.second != 0 && getFunInfo(dagnode->name)->useGlobalVariable[it.first] != 0) {
					return nullptr;
				}
			}
			if (calleeTimes[func->name] >= MAX_INLINE_TIMES) {
				return nullptr;
			}
			return dagnode;
		}
		return nullptr;
	}
	//记录用到的全局变量
	if (dagnode->nodeType == DAGNodeType::VALUE_NODE) {
		if(symbolTable->isGlobalSymbol(dagnode->symbol))
			useGlobalVariable[dagnode->name]++;
	}
	return nullptr;
}

//访问每一条语句的根结点
DAGNode* travelRoot(CFG* cfg, BasicBlock* block, DAGNode* dagnode, FuncNode* node)
{
	if (!dagnode)
		return nullptr;
	if (dagnode->relyNodes.size() != 0) {
		for (int i = 0;i < dagnode->relyNodes.size();i++) {
			if (iaChangedInThisTerm) return nullptr;
			useGlobalVariable.clear();
			DAGNode* returnNode = travelRoot(cfg, block, dagnode->relyNodes[i], node);
			if (returnNode) {
				insertCFG(cfg, block, returnNode, returnNode, returnNode->name,dagnode);
				iaChangedInThisTerm = true;
				return nullptr;
			}
			useGlobalVariable.clear();
		}
	}
	if (dagnode->inputs.size() != 0) {
		for (int i = 0;i < dagnode->inputs.size();i++) {
			if (iaChangedInThisTerm) return nullptr;
			DAGNode* returnNode = travelCommonNode(dagnode->inputs[i]);
			if (returnNode) {
				insertCFG(cfg, block, dagnode,returnNode, returnNode->name,nullptr);
				iaChangedInThisTerm = true;
				return nullptr;
			}
		}
	}
	if (dagnode->nodeType == DAGNodeType::OP_NODE && isCalleeNode(dagnode->name)) {
		auto func = getFunInfo(dagnode->name);
		if (func->canInline) {
			if (calleeTimes[func->name] >= MAX_INLINE_TIMES) {
				return nullptr;
			}
			return dagnode;
		}
			
	}
	return nullptr;
}

void travelBasicBlock(CFG* cfg, BasicBlock* block, FuncNode* node)
{
	vector<BasicBlock*> postblocks = block->postBlock;
	if (!block->isCondition) {
		travelRoot(cfg, block, block->dag->rootNode, node);
	}
	//这一遍已经内联过了，不再进行内联
	if (iaChangedInThisTerm)return;
	if (postblocks.size() != 0) {
		for (BasicBlock* blockToVisit : postblocks) {
			//如果还没有访问过，那么就访问
			if (!isVisit[blockToVisit->id]) {
				isVisit[blockToVisit->id] = 1;
				travelBasicBlock(cfg, blockToVisit, node);
			}
		}
	}
}

//对一个函数进行内联，循环进行，直到不再改变
void StartInline(CFG* cfg, FuncNode* node)
{
	do{
		iaChangedInThisTerm = false;
		//还原map		
		origin_Stmbol_To_NewSymbol_Map.clear();
		orgin_Scope_To_NewScope_Map.clear();
		hasCopiedBlock.clear();
		endBlocks.clear();
		isVisit.resize(cfg->basicBlocks.size(), 0);
		fill(isVisit.begin(), isVisit.end(), 0);
		isVisit[0] = 1;
		travelBasicBlock(cfg, cfg->entry, node);
	} while (iaChangedInThisTerm);	
}

//****************************结束查找插入点

static map<string, int> libfuncs = { {GET_INT,1},{GET_CH,1},{GET_FLOAT,1},{GET_ARRAY,1},{GET_F_ARRAY,1},{PUT_INT,1},
{PUT_CHAR,1},{PUT_FLOAT,1},{PUT_ARRAY,1},{PUT_F_ARRAY,1},{PUT_F,1},{START_TIME,1},{STOP_TIME,1} };

//入口函数
map<string,vector<string>> inlineOptimization(SymbolTable* table, GCFG* p_gcfg, std::shared_ptr<ExprAST>CU,int Opt)
{
	initLibFuncInfo();
	gcfg = p_gcfg;
	symbolTable = table;

	//先求出每一个函数的依赖关系
	std::shared_ptr<CompUnitAST> cu = std::dynamic_pointer_cast<CompUnitAST>(CU);
	for (std::shared_ptr<ExprAST> FuncOrVar : cu->TopExpr) {
		if (FuncOrVar->getClassName() == "Function") {
			std::shared_ptr<FunctionAST> func = std::dynamic_pointer_cast<FunctionAST>(FuncOrVar);
			checkUseChain(func->Name,gcfg->controlFlow[func->Name]);
		}
	}

	//遍历每一个funucnode，判断其是否可以进行内联
	for (FuncNode* funcToJudge : FuncInfo) {
		if (funcToJudge->isLibFunc) {
			continue;
		}
		if (funcToJudge->callTimes.size() == 0) {
			if (funcToJudge->blockNums <= MAX_BLOCK) {
				funcToJudge->canInline = true;
			}
			if (gcfg->controlFlow[funcToJudge->name]->entry->dag->nodes.size() > 10000 || table->getFuncScope(funcToJudge->name)->Symbols.size()>10000) {
				funcToJudge->canInline = false;
			}
			vector<Symbol*> symbols = table->getFuncSymbols(funcToJudge->name);
			if (symbols.size() != 0) {
				int size = 0;//记录数组个数
				for (auto symbol : symbols) {
					if (symbol->type == ipoint || symbol->type == fpoint) {
						size++;
					}
				}
				if (size > 8) {
					funcToJudge->canInline = false;
				}
			}
		}
		else {
			if (funcToJudge->callTimes.find(funcToJudge->name) != funcToJudge->callTimes.end()) {
				funcToJudge->canInline = false;
				continue;
			}
			if (gcfg->controlFlow[funcToJudge->name]->entry->dag->nodes.size() > 10000 || table->getFuncScope(funcToJudge->name)->Symbols.size() > 10000) {
				funcToJudge->canInline = false;
				continue;
			}
			vector<Symbol*> symbols = table->getFuncSymbols(funcToJudge->name);
			if (symbols.size() != 0) {
				int size = 0;//记录数组个数
				for (auto symbol : symbols) {
					if (symbol->type == ipoint || symbol->type == fpoint) {
						size++;
					}
				}
				if (size > 8) {
					funcToJudge->canInline = false;
					continue;
				}
			}
			for (auto it : funcToJudge->callTimes) {
				FuncNode* node = getFunInfo(it.first);
				if (node) {
					if (node->canInline) {
						funcToJudge->blockNums += node->blockNums * it.second;
					}
					funcToJudge->canInline = false;
					continue;
				}		
			}
			if (funcToJudge->blockNums <= MAX_BLOCK) {
				funcToJudge->canInline = true;
			}
		}
	}
	
	if(Opt){
		//开始内联
		for (FuncNode* node : FuncInfo) {
			if (libfuncs[node->name] == 1)continue;
			calleeTimes.clear();
			isVisit.clear();
			StartInline(gcfg->controlFlow[node->name], node);
		}
	}
	removeNoUseFunc(gcfg);

	map<string, vector<string>> useG_Var;
	for (auto func : FuncInfo) {
		vector<string> globals;
		for (auto it: func->useGlobalVariable) {
			if (it.second != 0) {
				globals.push_back(it.first);
			}
		}
		useG_Var[func->name] = globals;
	}

	return useG_Var;
}

static void travelDAG_delete(DAGNode* node, queue<string>& funcs, const unordered_map<string, int>& usedFunc)
{
	if (!node)return;
	for (auto rely : node->relyNodes) {
		travelDAG_delete(rely, funcs, usedFunc);
	}

	for (auto input : node->inputs) {
		travelDAG_delete(input, funcs, usedFunc);
	}

	if (node->nodeType == DAGNodeType::OP_NODE) {
		if (usedFunc.find(node->name)!=usedFunc.end()) {
			funcs.push(node->name);
		}
	}
}

static void travelBlocks_delete(BasicBlock* block, map<int, int>& visited, queue<string>& funcs,const unordered_map<string,int>&usedFunc)
{
	visited[block->id] = 1;
	travelDAG_delete(block->dag->rootNode, funcs, usedFunc);

	for (auto tblock : block->postBlock) {
		if (visited[tblock->id] != 1) {
			travelBlocks_delete(tblock, visited, funcs, usedFunc);
		}
	}
}

//删除没有用到的函数
void removeNoUseFunc(GCFG* gcfg)
{
	map<std::string, CFG*> newControllFlow;
	queue<string> funcs;
	funcs.push("main");

	unordered_map<string, int> usedFunc;
	for (auto func : FuncInfo) {
		if (!func->isLibFunc) {
			usedFunc[func->name] = 1;
		}
	}

	while (!funcs.empty()) {
		string toVisit = funcs.front();
		funcs.pop();
		if (newControllFlow.find(toVisit) == newControllFlow.end()) {
			newControllFlow[toVisit] = gcfg->controlFlow[toVisit];
			map<int, int>visited;
			travelBlocks_delete(gcfg->controlFlow[toVisit]->entry, visited, funcs,usedFunc);
		}
	}

	gcfg->controlFlow = newControllFlow;
}
