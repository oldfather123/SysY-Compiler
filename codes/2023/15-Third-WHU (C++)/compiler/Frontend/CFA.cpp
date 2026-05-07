#include"CFA.h"
#include<stack>
#include <map>

static std::string CurrentFunc;

static Scope* CurrencScope;

int id = -1;//解析新的函数，初始化基本块的id

GCFG* gcfg;

std::vector<DAGNode*> DAGNodeList;//用于DAG生成

std::stack<int> GlobalWhileId;//栈顶记录最近出现的while

std::map<int, std::vector<int>>BreakId;
std::map<int, std::vector<int>>ContinueId;



std::map<std::string, CFG*> controlFlow;//用来记录每个函数的基本块
DAGNode* returnNode;//用来存储每一级节点的返回的DAGNode

struct PtrComparer {
	bool operator()(const std::shared_ptr<ExprAST>& ptr1, const std::shared_ptr<ExprAST>& ptr2) const {
		return ptr1.get() < ptr2.get();
	}
};

//处理条件语句
std::map <ExprAST*, int> conditionMap;
//键值是真/假出口，正常来说只会有两种，所以先添加一个，然后删掉，再遍历取第二个
std::map<ExprAST*, std::vector<int>> lackOfTrueExit;
std::map<ExprAST*, std::vector<int>> lackOfFalseExit;

std::stack<std::map<ExprAST*, std::vector<int>>>lackOfTrueExitStack;
std::stack<std::map<ExprAST*, std::vector<int>>>lackOfFalseExitStack;


typedef struct ConditionBE {
	int begin = 0;
	int end = 0;
}ConditionBE;

bool JudgeParams(vector<DAGNode*> params, vector<DAGNode*> ListParams) {
	if (params.size() != ListParams.size())
		return false;
	for (size_t i = 0; i < params.size(); i++) {
		if (params[i] != ListParams[i])
			return false;
	}
	return true;
}

//生成所有的条件基本块，之后再通过map查找一一对应
void ConditionBlockInit(std::shared_ptr<ExprAST> expr)
{
	if (expr->getClassName() == "BinaryExpr") {
		std::shared_ptr<BinaryExprAST> b = std::dynamic_pointer_cast<BinaryExprAST>(expr);
		if (b->Op == 257 || b->Op == 258) {
			ConditionBlockInit(b->LHS);
			ConditionBlockInit(b->RHS);
		}
		else {
			DAGNodeList.clear();
			BasicBlock* conditionBlock = new BasicBlock(++id,CurrencScope);
			conditionBlock->isCondition = true;
			controlFlow[CurrentFunc]->basicBlocks.push_back(conditionBlock);
			conditionMap[expr.get()] = id;
			expr->analyzeFlow();
			conditionBlock->dag->rootNode = returnNode;
		}
	}
	else {
		DAGNodeList.clear();
		BasicBlock* conditionBlock = new BasicBlock(++id, CurrencScope);
		conditionBlock->isCondition = true;
		controlFlow[CurrentFunc]->basicBlocks.push_back(conditionBlock);
		conditionMap[expr.get()] = id;
		expr->analyzeFlow();
		conditionBlock->dag->rootNode = returnNode;
	}
}

//生成所有的条件基本块，之后再通过map查找一一对应
ConditionBE ConditionBlockConnect(std::shared_ptr<ExprAST> expr)
{
	if (expr->getClassName() == "BinaryExpr") {
		std::shared_ptr<BinaryExprAST> b = std::dynamic_pointer_cast<BinaryExprAST>(expr);
		if (b->Op == 257 || b->Op == 258) {
			ConditionBE be1 = ConditionBlockConnect(b->LHS);
			ConditionBE be2 = ConditionBlockConnect(b->RHS);
			ConditionBE result;
			result.begin = be1.begin;
			result.end = be2.end;
			return result;
		}
	}
	//当前非|| && 二元运算符对应的块
	auto it = conditionMap.find(expr.get());
	int currentId = it->second;

	DAGNode* branchNode = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
	branchNode->inputs.push_back(controlFlow[CurrentFunc]->basicBlocks[currentId]->dag->rootNode);
	controlFlow[CurrentFunc]->basicBlocks[currentId]->dag->rootNode->outputs.push_back(branchNode);
	controlFlow[CurrentFunc]->basicBlocks[currentId]->dag->rootNode = branchNode;
	controlFlow[CurrentFunc]->basicBlocks[currentId]->dag->nodes.push_back(branchNode);

	//把后继块的两个位置先声明出来
	controlFlow[CurrentFunc]->basicBlocks[currentId]->postBlock.resize(2, nullptr);

	auto trueit = conditionMap.find(expr->TrueExit.get());
	if (trueit != conditionMap.end()) {
		int trueId = trueit->second;
		branchNode->targets.push_back(trueId);
		controlFlow[CurrentFunc]->basicBlocks[currentId]->postBlock[0] = controlFlow[CurrentFunc]->basicBlocks[trueId];
		controlFlow[CurrentFunc]->basicBlocks[trueId]->preBlock.push_back(controlFlow[CurrentFunc]->basicBlocks[currentId]);
	}
	else {
		branchNode->targets.push_back(0);
		lackOfTrueExit[expr->TrueExit.get()].push_back(currentId);
	}

	auto falseit = conditionMap.find(expr->FalseExit.get());
	if (falseit != conditionMap.end()) {
		int falseId = falseit->second;
		branchNode->targets.push_back(falseId);
		controlFlow[CurrentFunc]->basicBlocks[currentId]->postBlock[1] = controlFlow[CurrentFunc]->basicBlocks[falseId];
		controlFlow[CurrentFunc]->basicBlocks[falseId]->preBlock.push_back(controlFlow[CurrentFunc]->basicBlocks[currentId]);
	}
	else {
		branchNode->targets.push_back(0);
		lackOfFalseExit[expr->FalseExit.get()].push_back(currentId);
	}

	ConditionBE result;
	result.begin = currentId;
	result.end = currentId;
	return result;
}

void addExit(int connectId ,std::shared_ptr<ExprAST> expr)
{
	lackOfTrueExit = lackOfTrueExitStack.top();
	lackOfTrueExitStack.pop();

	lackOfFalseExit = lackOfFalseExitStack.top();
	lackOfFalseExit = lackOfFalseExitStack.top();
	lackOfFalseExitStack.pop();


	auto it = lackOfTrueExit.find(expr.get());
	if (it != lackOfTrueExit.end()) {
		std::vector<int> ids = it->second;
		for (size_t i = 0; i < ids.size(); i++) {
			controlFlow[CurrentFunc]->basicBlocks[ids[i]]->dag->rootNode->targets[0] = connectId;
			controlFlow[CurrentFunc]->basicBlocks[ids[i]]->postBlock[0] = controlFlow[CurrentFunc]->basicBlocks[connectId];
			controlFlow[CurrentFunc]->basicBlocks[connectId]->preBlock.push_back(controlFlow[CurrentFunc]->basicBlocks[ids[i]]);
		}
	}

	it = lackOfFalseExit.find(expr.get());
	if (it != lackOfFalseExit.end()) {
		std::vector<int> ids = it->second;
		for (size_t i = 0; i < ids.size(); i++) {
			controlFlow[CurrentFunc]->basicBlocks[ids[i]]->dag->rootNode->targets[1] = connectId;
			controlFlow[CurrentFunc]->basicBlocks[ids[i]]->postBlock[1] = controlFlow[CurrentFunc]->basicBlocks[connectId];
			controlFlow[CurrentFunc]->basicBlocks[connectId]->preBlock.push_back(controlFlow[CurrentFunc]->basicBlocks[ids[i]]);
		}
	}

	lackOfTrueExit.erase(expr.get());
	lackOfFalseExit.erase(expr.get());

	lackOfTrueExitStack.push(lackOfTrueExit);
	lackOfFalseExitStack.push(lackOfFalseExit);

	lackOfTrueExit.clear();
	lackOfFalseExit.clear();

}

void add2Exit()
{
	//按道理来说，做完1以后，map里只会剩下一个键值对
	lackOfTrueExit = lackOfTrueExitStack.top();
	lackOfTrueExitStack.pop();

	lackOfFalseExit = lackOfFalseExitStack.top();
	lackOfFalseExitStack.pop();

	auto it = lackOfTrueExit.begin();
	if (it != lackOfTrueExit.end()) {
		vector<int> ids = it->second;
		for (size_t i = 0; i < ids.size(); i++) {
			controlFlow[CurrentFunc]->basicBlocks[ids[i]]->dag->rootNode->targets[0] = id;
			controlFlow[CurrentFunc]->basicBlocks[ids[i]]->postBlock[0] = controlFlow[CurrentFunc]->basicBlocks[id];
			controlFlow[CurrentFunc]->basicBlocks[id]->preBlock.push_back(controlFlow[CurrentFunc]->basicBlocks[ids[i]]);
		}
	}



	it = lackOfFalseExit.begin();
	if (it != lackOfFalseExit.end()) {
		vector<int> ids = it->second;
		for (size_t i = 0; i < ids.size(); i++) {
			controlFlow[CurrentFunc]->basicBlocks[ids[i]]->dag->rootNode->targets[1] = id;
			controlFlow[CurrentFunc]->basicBlocks[ids[i]]->postBlock[1] = controlFlow[CurrentFunc]->basicBlocks[id];
			controlFlow[CurrentFunc]->basicBlocks[id]->preBlock.push_back(controlFlow[CurrentFunc]->basicBlocks[ids[i]]);
		}
	}


	lackOfTrueExit.clear();
	lackOfFalseExit.clear();
}

//将数组降维，改为偏移量
typedef struct ArrayResult {
	int dim;//当前所在维度
	DAGNode* variable;
	DAGNode* sub;
}ArrayResult;

DAGNode* findIntConstant(int val)
{
	for (size_t i = 0; i < DAGNodeList.size(); i++) {
		if (DAGNodeList[i]->nodeType == DAGNodeType::CONSTANT && DAGNodeList[i]->resultType == ResultType::i32 && DAGNodeList[i]->value.iValue == val)
			return DAGNodeList[i];
	}
	return nullptr;
}

DAGNode* findBinary(std::string name, DAGNode* left, DAGNode* right)
{
	if (DAGNodeList.size() != 0) {
		for (int i = 0; i < DAGNodeList.size(); i++) {
			if (DAGNodeList[i]->nodeType == DAGNodeType::OP_NODE && DAGNodeList[i]->name == name && DAGNodeList[i]->inputs.size() == 2 &&
				((DAGNodeList[i]->inputs[0] == left && DAGNodeList[i]->inputs[1] == right) || (DAGNodeList[i]->inputs[1] == left && DAGNodeList[i]->inputs[0] == right))) {
				return DAGNodeList[i];
			}
		}
	}
	return nullptr;
}


ArrayResult SubArrayAna(ExprAST* astNode)
{
	ArraySubscripAST* subAST = dynamic_cast<ArraySubscripAST*>(astNode);
	if (subAST->Addr->getClassName() != "DeclRef") {
		ArrayResult result = SubArrayAna(subAST->Addr.get());
		subAST->ArraySub->analyzeFlow();
		DAGNode* subNode = returnNode;

		//新建被乘数
		DAGNode* constNode = findIntConstant(result.variable->symbol->offsetVec[result.dim]);
		if (!constNode) {
			constNode = new DAGNode(" ", DAGNodeType::CONSTANT);
			constNode->isConst = true;
			constNode->resultType = ResultType::i32;
			constNode->value.iValue = result.variable->symbol->offsetVec[result.dim];
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(constNode);
		}

		//新建乘号
		DAGNode* mulNode = findBinary(MULTIPLY, subNode, constNode);
		if (!mulNode) {
			mulNode = new DAGNode(MULTIPLY, DAGNodeType::OP_NODE);
			mulNode->resultType = ResultType::i32;
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(mulNode, subNode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(mulNode, constNode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(mulNode);
			if (subNode->isConst)
				mulNode->isConst = true;
		}

		//新建加号
		DAGNode* addNode = findBinary(PLUS, result.sub, mulNode);
		if (!addNode) {
			addNode = new DAGNode(PLUS, DAGNodeType::OP_NODE);
			addNode->resultType = ResultType::i32;
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(addNode, result.sub);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(addNode, mulNode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(addNode);
			if (result.sub->isConst && mulNode->isConst)
				addNode->isConst = true;
		}

		result.dim++;
		result.sub = addNode;
		return result;
	}
	else {
		ArrayResult result;
		result.dim = 0;

		subAST->Addr->analyzeFlow();
		//获取变量节点，去掉load
		DAGNode* variable = returnNode->inputs[0];
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->remove_edge(returnNode, variable);
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->remove_node(returnNode);

		result.variable = variable;

		subAST->ArraySub->analyzeFlow();
		DAGNode* subNode = returnNode;

		//新建被乘数
		DAGNode* constNode = findIntConstant(result.variable->symbol->offsetVec[result.dim]);
		if (!constNode) {
			constNode = new DAGNode(" ", DAGNodeType::CONSTANT);
			constNode->isConst = true;
			constNode->resultType = ResultType::i32;
			constNode->value.iValue = result.variable->symbol->offsetVec[result.dim];
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(constNode);
		}

		//新建乘号
		DAGNode* mulNode = findBinary(MULTIPLY, subNode, constNode);
		if (!mulNode) {
			mulNode = new DAGNode(MULTIPLY, DAGNodeType::OP_NODE);
			mulNode->resultType = ResultType::i32;
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(mulNode, subNode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(mulNode, constNode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(mulNode);
			if (subNode->isConst)
				mulNode->isConst = true;
		}

		result.sub = mulNode;
		result.dim++;
		return result;
	}
}

void StringAST::analyzeFlow()
{
	DAGNode* dagnode = new DAGNode(" ", DAGNodeType::CONSTANT);
	dagnode->resultType = ResultType::stringT;
	dagnode->value.iValue = this->id;
	dagnode->isConst = true;
	DAGNodeList.push_back(dagnode);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
	returnNode = dagnode;
	return;
}

void ArraySubscripAST::analyzeFlow()
{
	if (this->Type->isConstQualified()) {
		ValTypeStruct val = this->Evaluate();
		if (val.type == "int") {
			if (DAGNodeList.size() != 0) {
				for (int i = 0; i < DAGNodeList.size(); i++) {
					if (DAGNodeList[i]->nodeType == DAGNodeType::CONSTANT && DAGNodeList[i]->resultType == ResultType::i32 && DAGNodeList[i]->value.iValue == val.iVal) {
						returnNode = DAGNodeList[i];
						return;
					}
				}
			}
			DAGNode* dagnode = new DAGNode(" ", DAGNodeType::CONSTANT);
			dagnode->resultType = ResultType::i32;
			dagnode->value.iValue = val.iVal;
			dagnode->isConst = true;
			DAGNodeList.push_back(dagnode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
			returnNode = dagnode;
			return;
		}
		else {
			if (DAGNodeList.size() != 0) {
				for (int i = 0; i < DAGNodeList.size(); i++) {
					if (DAGNodeList[i]->nodeType == DAGNodeType::CONSTANT && DAGNodeList[i]->resultType == ResultType::f32 && DAGNodeList[i]->value.fValue == val.fVal) {
						returnNode = DAGNodeList[i];
						return;
					}
				}
			}
			DAGNode* dagnode = new DAGNode(" ", DAGNodeType::CONSTANT);
			dagnode->resultType = ResultType::f32;
			dagnode->value.fValue = val.fVal;
			dagnode->isConst = true;
			DAGNodeList.push_back(dagnode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
			returnNode = dagnode;
			return;
		}
	}

	ArrayResult result = SubArrayAna(this);

	if (DAGNodeList.size() != 0) {
		for (int i = 0; i < DAGNodeList.size(); i++) {
			if (DAGNodeList[i]->nodeType == DAGNodeType::OP_NODE && DAGNodeList[i]->name == ARRAYSUB_NODE && DAGNodeList[i]->inputs[0] == result.variable && DAGNodeList[i]->inputs[1] == result.sub) {
				returnNode = DAGNodeList[i];
				return;
			}
		}
	}

	DAGNode* subNode = new DAGNode(ARRAYSUB_NODE, DAGNodeType::OP_NODE);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(subNode, result.variable);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(subNode, result.sub);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(subNode);
	//维数相等，变成一个数
	if (result.dim == result.variable->symbol->dimensions.size()) {
		if (result.variable->symbol->type == ResultType::iarray || result.variable->symbol->type == ResultType::ipoint)
			subNode->resultType = ResultType::ipoint;
		else if (result.variable->symbol->type == ResultType::farray || result.variable->symbol->type == ResultType::fpoint)
			subNode->resultType = ResultType::fpoint;

		//再新建load节点
		DAGNode* loadnode = new DAGNode(" ", DAGNodeType::LOAD);
		if (subNode->resultType == ResultType::ipoint)
			loadnode->resultType = ResultType::i32;
		else if (subNode->resultType == ResultType::fpoint)
			loadnode->resultType = ResultType::f32;
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(loadnode, subNode);
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(loadnode);//在dag的所有节点中添加该节点

		returnNode = loadnode;
		return;
	}
	else {
		if (result.variable->symbol->type == ResultType::iarray)
			subNode->resultType = ResultType::ipoint;
		else if (result.variable->symbol->type == ResultType::farray)
			subNode->resultType = ResultType::fpoint;
		returnNode = subNode;
		return;
	}
}

vector<int> dim_vec;
vector<int> offsetVec;
vector<int> init_val_offset;
void getArrayInitMap(ExprAST* expr, DAGNode* initNode)
{
	int offset = 0;
	InitListAST* initAST = dynamic_cast<InitListAST*>(expr);
	for (int i = 0;i < initAST->List.size();i++) {
		if (initAST->List[i]->getClassName() == "InitList") {
			dim_vec.push_back(offset);
			getArrayInitMap((initAST->List[i]).get(), initNode);
			dim_vec.pop_back();
			offset++;
		}
		else if (initAST->List[i]->getClassName() == "ImplicitValueInit") {
			return;
		}
		else {
			dim_vec.push_back(offset);
			initAST->List[i]->analyzeFlow();
			DAGNode* inputNode = returnNode;

			controlFlow[CurrentFunc]->basicBlocks[id]->dag->add_edge(initNode, inputNode);
			int sum = 0;
			for (int i = 0;i < offsetVec.size();i++) {
				sum += dim_vec[i] * offsetVec[i];
			}
			init_val_offset.push_back(sum);

			dim_vec.pop_back();
			offset++;
		}
	}
}

void InitListAST::analyzeFlow()
{
	vector<int>size = ((ArrayType*)(this->Type->getBaseType()))->Size;
	//计算偏移相乘数组
	offsetVec.clear();
	for (size_t i = size.size();i > 0;i--) {
		if (i == size.size()) {
			offsetVec.push_back(1);
		}
		else {
			int mul = 1;
			for (int j = i;j < size.size();j++) {
				mul *= size[j];
			}
			offsetVec.push_back(mul);
		}
	}
	std::reverse(offsetVec.begin(), offsetVec.end());

	DAGNode* dagnode = new DAGNode(ARRAY_INIT_NODE, DAGNodeType::OP_NODE);
	getArrayInitMap(this, dagnode);
	dagnode->offset = init_val_offset;
	init_val_offset.clear();

	if (((ArrayType*)this->Type->getBaseType())->ElementType->getBaseType()->getTypeID() == Type::TypeID::Int) {
		dagnode->resultType = ResultType::iarray;
	}
	else {
		dagnode->resultType = ResultType::farray;
	}

	if (this->Type->isConstQualified()) {
		dagnode->isConst = true;
	}
	DAGNodeList.push_back(dagnode);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
	returnNode = dagnode;
	return;
}

void ImplicitValueInit::analyzeFlow()
{
	if (DAGNodeList.size() != 0) {
		for (int i = 0; i < DAGNodeList.size(); i++) {
			if (DAGNodeList[i]->nodeType == DAGNodeType::OP_NODE && DAGNodeList[i]->name == IMPLICIT_INIT_NODE) {
				returnNode = DAGNodeList[i];
				return;
			}
		}
	}

	DAGNode* dagnode = new DAGNode(IMPLICIT_INIT_NODE, DAGNodeType::OP_NODE);
	if (this->type->getBaseType()->getTypeID() == Type::TypeID::Int) {
		dagnode->resultType = ResultType::i32;
	}
	else if (this->type->getBaseType()->getTypeID() == Type::TypeID::Float) {
		dagnode->resultType = ResultType::i32;
	}
	else if (((ArrayType*)this->type->getBaseType())->ElementType->getBaseType()->getTypeID() == Type::TypeID::Int) {
		dagnode->resultType = ResultType::iarray;
	}
	else {
		dagnode->resultType = ResultType::farray;
	}
	if (this->type->isConstQualified()) {
		dagnode->isConst = true;
	}
	DAGNodeList.push_back(dagnode);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
	returnNode = dagnode;
	return;
}

void DeclRefAST::analyzeFlow()
{
	//常量直接转换为数值处理，常量数组的话通过arraysub处理
	if (this->UseDecl->isConst && !(this->UseDecl->type == ResultType::farray || this->UseDecl->type == ResultType::iarray)) {
		if (this->UseDecl->type == ResultType::i32) {
			if (DAGNodeList.size() != 0) {
				for (int i = 0; i < DAGNodeList.size(); i++) {
					if (DAGNodeList[i]->nodeType == DAGNodeType::CONSTANT && DAGNodeList[i]->resultType == ResultType::i32 && DAGNodeList[i]->value.iValue == this->UseDecl->constValue.iVal) {
						returnNode = DAGNodeList[i];
						return;
					}
				}
			}
			DAGNode* dagnode = new DAGNode(" ", DAGNodeType::CONSTANT);
			dagnode->resultType = ResultType::i32;
			dagnode->value.iValue = this->UseDecl->constValue.iVal;
			dagnode->isConst = true;
			DAGNodeList.push_back(dagnode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
			returnNode = dagnode;
			return;
		}
		else if (this->UseDecl->type == ResultType::f32) {
			if (DAGNodeList.size() != 0) {
				for (int i = 0; i < DAGNodeList.size(); i++) {
					if (DAGNodeList[i]->nodeType == DAGNodeType::CONSTANT && DAGNodeList[i]->resultType == ResultType::f32 && DAGNodeList[i]->value.fValue == this->UseDecl->constValue.fVal) {
						returnNode = DAGNodeList[i];
						return;
					}
				}
			}
			DAGNode* dagnode = new DAGNode(" ", DAGNodeType::CONSTANT);
			dagnode->resultType = ResultType::f32;
			dagnode->isConst = true;
			dagnode->value.fValue = this->UseDecl->constValue.fVal;
			DAGNodeList.push_back(dagnode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
			returnNode = dagnode;
			return;
		}
	}

	bool Flag = false;//
	DAGNode* dagnode = new DAGNode(" ", DAGNodeType::VALUE_NODE);
	//如果不是常量，那么变成变量节点，不能加上load，得到dagnode变量节点
	if (DAGNodeList.size() != 0) {
		for (int i = 0; i < DAGNodeList.size(); i++) {
			if (DAGNodeList[i]->nodeType == DAGNodeType::VALUE_NODE && DAGNodeList[i]->name == this->Name && DAGNodeList[i]->symbol == this->UseDecl) {
				dagnode = DAGNodeList[i];
				Flag = true;
			}
		}
	}
	if (!Flag) {
		dagnode = new DAGNode(" ", DAGNodeType::VALUE_NODE);
		//先新建一个value_node
		dagnode->resultType = this->UseDecl->type;
		dagnode->name = this->Name;
		dagnode->symbol = this->UseDecl;
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
		DAGNodeList.push_back(dagnode);
	}

	//再新建load节点
	DAGNode* loadnode = new DAGNode(" ", DAGNodeType::LOAD);
	loadnode->resultType = this->UseDecl->type;
	loadnode->inputs.push_back(dagnode);
	dagnode->outputs.push_back(loadnode);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(loadnode);//在dag的所有节点中添加该节点

	returnNode = loadnode;
	return;

}

void FloatLiteralAST::analyzeFlow()
{
	if (DAGNodeList.size() != 0) {
		for (int i = 0; i < DAGNodeList.size(); i++) {
			if (DAGNodeList[i]->nodeType == DAGNodeType::CONSTANT && DAGNodeList[i]->resultType == ResultType::f32 && DAGNodeList[i]->value.fValue == this->Val) {
				returnNode = DAGNodeList[i];
				return;
			}
		}
	}
	DAGNode* dagnode = new DAGNode(" ", DAGNodeType::CONSTANT);
	dagnode->resultType = ResultType::f32;
	dagnode->value.fValue = this->Val;
	dagnode->isConst = true;
	DAGNodeList.push_back(dagnode);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
	returnNode = dagnode;
	return;
}

void IntegerLiteralAST::analyzeFlow()
{
	if (DAGNodeList.size() != 0) {
		for (int i = 0; i < DAGNodeList.size(); i++) {
			if (DAGNodeList[i]->nodeType == DAGNodeType::CONSTANT && DAGNodeList[i]->resultType == ResultType::i32 && DAGNodeList[i]->value.iValue == this->Val) {
				returnNode = DAGNodeList[i];
				return;
			}
		}
	}
	DAGNode* dagnode = new DAGNode(" ", DAGNodeType::CONSTANT);
	dagnode->resultType = ResultType::i32;
	dagnode->value.iValue = this->Val;
	dagnode->isConst = true;
	DAGNodeList.push_back(dagnode);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点
	returnNode = dagnode;
	return;
}

void UnaryExprAST::analyzeFlow()
{
	this->RHS->analyzeFlow();
	DAGNode* rightNode = returnNode;
	if (DAGNodeList.size() != 0) {
		for (int i = 0; i < DAGNodeList.size(); i++) {
			if (DAGNodeList[i]->nodeType == DAGNodeType::OP_NODE && DAGNodeList[i]->name == std::to_string(this->Op) && DAGNodeList[i]->inputs.size() == 1
				&& DAGNodeList[i]->inputs[0] == rightNode) {
				returnNode = DAGNodeList[i];
				return;
			}
		}
	}
	DAGNode* dagnode = new DAGNode(" ", DAGNodeType::OP_NODE);
	dagnode->resultType = rightNode->resultType;
	dagnode->name = std::to_string(this->Op);
	dagnode->inputs.push_back(rightNode);
	rightNode->outputs.push_back(dagnode);

	DAGNodeList.push_back(dagnode);
	if (rightNode->isConst)
		dagnode->isConst = true;
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点

	returnNode = dagnode;
	return;
}

void BinaryExprAST::analyzeFlow()
{
	//等号要改为store节点，需要另外处理
	if (this->Op != '=') {
		this->LHS->analyzeFlow();
		DAGNode* leftNode = returnNode;
		this->RHS->analyzeFlow();
		DAGNode* rightNode = returnNode;
		if (DAGNodeList.size() != 0) {
			for (int i = 0; i < DAGNodeList.size(); i++) {
				if (DAGNodeList[i]->nodeType == DAGNodeType::OP_NODE && DAGNodeList[i]->name == std::to_string(this->Op) && DAGNodeList[i]->inputs.size() == 2 &&
					((DAGNodeList[i]->inputs[0] == leftNode && DAGNodeList[i]->inputs[1] == rightNode) || (DAGNodeList[i]->inputs[1] == leftNode && DAGNodeList[i]->inputs[0] == rightNode))) {
					returnNode = DAGNodeList[i];
					return;
				}
			}
		}
		DAGNode* dagnode = new DAGNode(" ", DAGNodeType::OP_NODE);
		if (this->Type->getBaseType()->getTypeID() == Type::TypeID::Int) {
			dagnode->resultType = ResultType::i32;
		}
		else {
			dagnode->resultType = ResultType::f32;
		}

		dagnode->name = std::to_string(this->Op);//注意op的类型是int
		dagnode->inputs.push_back(leftNode);
		dagnode->inputs.push_back(rightNode);
		leftNode->outputs.push_back(dagnode);
		rightNode->outputs.push_back(dagnode);

		DAGNodeList.push_back(dagnode);
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点


		returnNode = dagnode;
		return;
	}
	else {
		this->RHS->analyzeFlow();
		DAGNode* rightNode = returnNode;
		//左节点不需要进行load，所以分析完取load的input
		this->LHS->analyzeFlow();
		DAGNode* leftNode = returnNode->inputs[0];
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->remove_edge(returnNode, leftNode);
		delete returnNode;
		returnNode = nullptr;
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.pop_back();

		std::vector<DAGNode*> vec = controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes;//************
		//等号不可能有重复的，所以加上store节点
		DAGNode* dagnode = new DAGNode(" ", DAGNodeType::STORE);
		dagnode->inputs.push_back(leftNode);
		dagnode->inputs.push_back(rightNode);
		leftNode->outputs.push_back(dagnode);
		rightNode->outputs.push_back(dagnode);

		if (rightNode->isConst && leftNode->isConst)
			dagnode->isConst = true;

		controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点

		returnNode = dagnode;
		return;
	}

}

void CallExprAST::analyzeFlow()
{
	//先分析所有参数
	vector<DAGNode*>params;
	for (size_t i = 0; i < this->Args.size(); i++) {
		this->Args[i]->analyzeFlow();
		//对于load数组或指针的错误情况做特殊处理
		if (returnNode->nodeType == DAGNodeType::LOAD && (returnNode->resultType == ResultType::iarray || returnNode->resultType == ResultType::farray ||
			returnNode->resultType == ResultType::ipoint || returnNode->resultType == ResultType::fpoint)) {
			auto val = returnNode->inputs[0];
			auto it = find(val->outputs.begin(), val->outputs.end(), returnNode);
			val->outputs.erase(it);
			returnNode = val;
		}
		params.push_back(returnNode);
	}
	DAGNode* callNode = new DAGNode(this->Callee, DAGNodeType::OP_NODE);
	callNode->resultType = this->UseFunc->returnType;
	callNode->inputs = params;
	for (DAGNode* dagnode : params) {
		dagnode->outputs.push_back(callNode);
	}
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(callNode);//在dag的所有节点中添加该节点
	DAGNodeList.push_back(callNode);
	returnNode = callNode;
}

void ImplCastExprAST::analyzeFlow()
{
	this->Val->analyzeFlow();
	DAGNode* valNode = returnNode;
	if (DAGNodeList.size() != 0) {
		for (int i = 0; i < DAGNodeList.size(); i++) {
			if (DAGNodeList[i]->nodeType == DAGNodeType::OP_NODE && DAGNodeList[i]->name == this->CastString && DAGNodeList[i]->inputs[0] == valNode) {
				returnNode = DAGNodeList[i];
				return;
			}
		}
	}
	DAGNode* dagnode = new DAGNode(" ", DAGNodeType::OP_NODE);
	dagnode->name = this->CastString;
	if (dagnode->name == FLOAT_TO_INT)
		dagnode->resultType = ResultType::i32;
	else if (dagnode->name == INT_TO_FLOAT)
		dagnode->resultType = ResultType::f32;
	if (valNode->isConst) {
		dagnode->isConst = true;
	}
	dagnode->inputs.push_back(valNode);
	valNode->outputs.push_back(dagnode);
	DAGNodeList.push_back(dagnode);
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点

	returnNode = dagnode;
	return;
}




void ValDeclAST::analyzeFlow()
{
	if (this->Val && (!this->symbol->isConst || this->symbol->type == iarray || this->symbol->type == farray)) {
		//得到初始化表达式节点
		this->Val->analyzeFlow();
		DAGNode* rightdagNode = returnNode;
		//将左值变成变量节点
		DAGNode* leftdagNode = new DAGNode(" ", DAGNodeType::VALUE_NODE);
		leftdagNode->resultType = this->symbol->type;
		leftdagNode->name = this->Name;
		leftdagNode->symbol = this->symbol;
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(leftdagNode);//在dag的所有节点中添加该节点
		DAGNodeList.push_back(leftdagNode);
		//最后得到赋值节点
		DAGNode* dagnode = new DAGNode(" ", DAGNodeType::STORE);
		dagnode->inputs.push_back(leftdagNode);
		dagnode->inputs.push_back(rightdagNode);
		leftdagNode->outputs.push_back(dagnode);
		rightdagNode->outputs.push_back(dagnode);

		controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(dagnode);//在dag的所有节点中添加该节点

		returnNode = dagnode;
		return;
	}
	returnNode = nullptr;
}

void DeclStmtAST::analyzeFlow()
{
	for (int i = 0; i < this->ValDecls.size(); i++) {
		ValDecls[i]->analyzeFlow();
		if (returnNode) {
			DAGNode* dagnode = returnNode;
			if (controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode) {
				dagnode->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode);
				controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = dagnode;
			}
			else {
				controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = dagnode;
			}
		}
	}
	returnNode = nullptr;
}

void ContinueAST::analyzeFlow()
{
	//类似break语句，只不过是回到while语句的条件
	ContinueId[GlobalWhileId.top()].push_back(id);
	controlFlow[CurrentFunc]->basicBlocks[id]->label = BasicBlock::LabelType::WHILEBODY;

	DAGNodeList.clear();
	//断开基本块
	BasicBlock* afterBlock = new BasicBlock(++id,nullptr);
	controlFlow[CurrentFunc]->basicBlocks.push_back(afterBlock);

	returnNode = nullptr;
}

void BreakAST::analyzeFlow()
{
	//break语句需要跳出到上一个while语句的下一个基本块
	//与return不同，我们需要记下当前基本块的编号
	BreakId[GlobalWhileId.top()].push_back(id);

	DAGNodeList.clear();
	//断开基本块
	BasicBlock* afterBlock = new BasicBlock(++id,nullptr);
	controlFlow[CurrentFunc]->basicBlocks.push_back(afterBlock);

	returnNode = nullptr;
}

void ReturnExprAST::analyzeFlow()
{
	//我们新建一个基本块，并断开其与当前基本块的连接
	DAGNode* returnN = new DAGNode(" ", DAGNodeType::OP_NODE);
	returnN->name = RETURN_NODE;

	//如果有表达式。那么分析
	if (this->Stmt) {
		this->Stmt->analyzeFlow();
		DAGNode* valnode = returnNode;
		returnN->inputs.push_back(valnode);
		valnode->outputs.push_back(returnN);
		returnN->resultType = valnode->resultType;
	}
	else {
		returnN->resultType = ResultType::VOID;
	}

	if (controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode) {
		returnN->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode);
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = returnN;
	}
	else {
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = returnN;
	}

	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(returnN);//在dag的所有节点中添加该返回节点
	returnNode = nullptr;

	DAGNodeList.clear();
	BasicBlock* afterBlock = new BasicBlock(++id,nullptr);
	controlFlow[CurrentFunc]->basicBlocks.push_back(afterBlock);
	afterBlock->label = BasicBlock::LabelType::NO_USE;
}

//进入到函数体或if/while语句的内部，先新建基本块
void CompoundStmtAST::analyzeFlow()
{ 
	//*********
	if (id != -1) {
		DAGNode* node = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
		if (controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode) {
			node->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = node;
		}
		else {
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = node;
		}
		node->targets.push_back(id + 1);
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(node);
	}
	
	//*********
	DAGNodeList.clear();
	BasicBlock* block = new BasicBlock(++id,this->scope);
	controlFlow[CurrentFunc]->basicBlocks.push_back(block);

	if (id != 0) {
		controlFlow[CurrentFunc]->addEdge(controlFlow[CurrentFunc]->basicBlocks[id - 1], block);
	}

	for (int i = 0; i < this->Stmts.size(); i++) {

		//每一次分析语句前，改变currentscope
		CurrencScope = this->scope;


		this->Stmts[i]->analyzeFlow();
		DAGNode* dagNode = returnNode;
		//如果需要添加的话，像DeclStmt就不需要在这里添加
		if (dagNode) {
			if (controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode) {
				dagNode->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode);
				controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = dagNode;
			}
			else {
				controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = dagNode;
			}
		}
	}
	returnNode = nullptr;
}

void WhileExprAST::analyzeFlow()
{
	//暂时保存当前的作用域
	Scope* tempCurrentScope = CurrencScope;


	//while语句的条件需要单独创建基本块来实现
	//新建分支语句节点，连接条件语句
	DAGNode* beforeWhileBranch = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
	//添加依赖关系，修改初始节点
	if (controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode) {
		beforeWhileBranch->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode);
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = beforeWhileBranch;
	}
	else {
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = beforeWhileBranch;
	}
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(beforeWhileBranch);

	int beforeWhileId = id;

	ConditionBlockInit(this->Condition);
	ConditionBE be = ConditionBlockConnect(this->Condition);
	conditionMap.clear();

	lackOfTrueExitStack.push(lackOfTrueExit);
	lackOfFalseExitStack.push(lackOfFalseExit);

	lackOfTrueExit.clear();
	lackOfFalseExit.clear();

	//1.先为上一个块的无条件跳转添加目标
	//2.为没有真假出口的补足
	//3.第一个块打上标签
	//4.whilebody结束和continue结束
	beforeWhileBranch->targets.push_back(be.begin);
	controlFlow[CurrentFunc]->addEdge(controlFlow[CurrentFunc]->basicBlocks[beforeWhileId], controlFlow[CurrentFunc]->basicBlocks[be.begin]);

	//存储条件语句的第一个块
	int FirstConditionId = be.begin;
	GlobalWhileId.push(FirstConditionId);

	//加条件标签
	controlFlow[CurrentFunc]->basicBlocks[FirstConditionId]->label = BasicBlock::LabelType::WHILE_CONDITION;

	//在whilebody之前，condition之后新建基本块
	DAGNodeList.clear();
	BasicBlock* beforeWhileAfterCon_Block = new BasicBlock(++id,tempCurrentScope);
	controlFlow[CurrentFunc]->basicBlocks.push_back(beforeWhileAfterCon_Block);

	//为没有填充的条件添加出口到新的基本块
	addExit(id,this->Stmt);

	//分析whilebody
	this->Stmt->analyzeFlow();

	//body结束回到第一个条件
	controlFlow[CurrentFunc]->addEdge(controlFlow[CurrentFunc]->basicBlocks[id], controlFlow[CurrentFunc]->basicBlocks[FirstConditionId]);

	//为whilebody的最后一个块添加label和无条件跳转
	if (controlFlow[CurrentFunc]->basicBlocks[id]->label == BasicBlock::LabelType::IF_END) {
		controlFlow[CurrentFunc]->basicBlocks[id]->label = BasicBlock::LabelType::IF_END_AND_WHILEBODY;
	}
	else {
		controlFlow[CurrentFunc]->basicBlocks[id]->label = BasicBlock::LabelType::WHILEBODY;
	}
	DAGNode* whileBodyBranch = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
	if (controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode) {
		whileBodyBranch->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode);
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = whileBodyBranch;
	}
	else {
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = whileBodyBranch;
	}
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(whileBodyBranch);
	whileBodyBranch->targets.push_back(FirstConditionId);

	//while语句后的基本块
	DAGNodeList.clear();
	BasicBlock* afterBlock = new BasicBlock(++id, tempCurrentScope);
	controlFlow[CurrentFunc]->basicBlocks.push_back(afterBlock);

	//在这里做条件恒真的判断
	if (this->Condition->getClassName() == "IntegerLiteral") {
		controlFlow[CurrentFunc]->basicBlocks[FirstConditionId]->dag->rootNode->targets.pop_back();
		controlFlow[CurrentFunc]->basicBlocks[FirstConditionId]->dag->rootNode->inputs.clear();
		controlFlow[CurrentFunc]->basicBlocks[FirstConditionId]->postBlock.pop_back();
		controlFlow[CurrentFunc]->basicBlocks[FirstConditionId]->dag->nodes.erase(controlFlow[CurrentFunc]->basicBlocks[FirstConditionId]->dag->nodes.begin());

		lackOfFalseExitStack.pop();
		lackOfTrueExitStack.pop();
	}
	else {
		add2Exit();
	}

	//如果有break语句，将存储的基本块连接到while后的基本块
	auto iter = BreakId.find(FirstConditionId);
	if (iter != BreakId.end())
	{
		std::vector<int>BreakIds = iter->second;
		if (BreakIds.size() != 0)
		{
			for (int i = 0; i < BreakIds.size(); i++) {
				DAGNode* breakBranch = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
				if (controlFlow[CurrentFunc]->basicBlocks[BreakIds[i]]->dag->rootNode) {
					breakBranch->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[BreakIds[i]]->dag->rootNode);
					controlFlow[CurrentFunc]->basicBlocks[BreakIds[i]]->dag->rootNode = breakBranch;
				}
				else {
					controlFlow[CurrentFunc]->basicBlocks[BreakIds[i]]->dag->rootNode = breakBranch;
				}
				controlFlow[CurrentFunc]->basicBlocks[BreakIds[i]]->dag->nodes.push_back(breakBranch);
				breakBranch->targets.push_back(id);
				controlFlow[CurrentFunc]->addEdge(controlFlow[CurrentFunc]->basicBlocks[BreakIds[i]], afterBlock);
			}
		}
		BreakIds.clear();
	}

	//如果有cotinue语句，将存储的基本块连接到while的最后一条语句，记得whilebody
	iter = ContinueId.find(FirstConditionId);
	if (iter != ContinueId.end())
	{
		std::vector<int>ContinueIds = iter->second;
		if (ContinueIds.size() != 0)
		{
			for (int i = 0; i < ContinueIds.size(); i++) {
				DAGNode* continueBranch = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
				if (controlFlow[CurrentFunc]->basicBlocks[ContinueIds[i]]->dag->rootNode) {
					continueBranch->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[ContinueIds[i]]->dag->rootNode);
					controlFlow[CurrentFunc]->basicBlocks[ContinueIds[i]]->dag->rootNode = continueBranch;
				}
				else {
					controlFlow[CurrentFunc]->basicBlocks[ContinueIds[i]]->dag->rootNode = continueBranch;
				}
				controlFlow[CurrentFunc]->basicBlocks[ContinueIds[i]]->dag->nodes.push_back(continueBranch);
				continueBranch->targets.push_back(FirstConditionId);
				controlFlow[CurrentFunc]->addEdge(controlFlow[CurrentFunc]->basicBlocks[ContinueIds[i]], controlFlow[CurrentFunc]->basicBlocks[FirstConditionId]);
			}
		}
		ContinueIds.clear();
	}

	//给条件添加真出口和假出口
	controlFlow[CurrentFunc]->basicBlocks[FirstConditionId]->True_False_Exit = std::make_pair(beforeWhileAfterCon_Block, afterBlock);

	GlobalWhileId.pop();

	returnNode = nullptr;
}

void IfElseExprAST::analyzeFlow()
{
	Scope* tempScope = CurrencScope;


	//while语句的条件需要单独创建基本块来实现
	//新建分支语句节点，连接条件语句
	DAGNode* beforeIfBranch = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
	//添加依赖关系，修改初始节点
	if (controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode) {
		beforeIfBranch->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode);
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = beforeIfBranch;
	}
	else {
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = beforeIfBranch;
	}
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(beforeIfBranch);

	int beforeIfId = id;

	//条件分析
	ConditionBlockInit(this->Condition);
	ConditionBE be = ConditionBlockConnect(this->Condition);
	conditionMap.clear();

	lackOfFalseExitStack.push(lackOfFalseExit);
	lackOfTrueExitStack.push(lackOfTrueExit);

	lackOfTrueExit.clear();
	lackOfFalseExit.clear();

	//1.先为上一个块的无条件跳转添加目标
	//2.为没有真假出口的补足
	//3.第一个块打上标签
	//4.whilebody结束和continue结束
	beforeIfBranch->targets.push_back(be.begin);
	controlFlow[CurrentFunc]->addEdge(controlFlow[CurrentFunc]->basicBlocks[beforeIfId], controlFlow[CurrentFunc]->basicBlocks[be.begin]);

	controlFlow[CurrentFunc]->basicBlocks[be.begin]->label = BasicBlock::LabelType::IF_BEGIN;

	//存储条件语句的第一个块
	int FirstConditionId = be.begin;

	//为ifbody之前，condition之后新建基本块，在创建新的基本块之前clear
	DAGNodeList.clear();
	BasicBlock* beforeIf_AfterCon_Block = new BasicBlock(++id, tempScope);
	controlFlow[CurrentFunc]->basicBlocks.push_back(beforeIf_AfterCon_Block);

	int connectId = id;

	//补足真出口
	addExit(connectId,this->Stmts);

	//分析body
	this->Stmts->analyzeFlow();

	//常规动作，为ifbody添加无条件跳转节点
	DAGNode* branchNodeIf = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
	if (controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode) {
		branchNodeIf->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode);
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = branchNodeIf;
	}
	else {
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = branchNodeIf;
	}
	controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(branchNodeIf);

	//要注意：下面要添加的并不是ifblock，因为在搜索中早就生成了更多的block
	//如果有else的话就新建基本块存else
	if (this->ElseStmt) {
		//存储当前if的最后一个基本块的id，先存储，否则丢失，为if的最后一个语句块添加branch节点
		int ifbodyId = id;

		DAGNodeList.clear();
		BasicBlock* elseBlock = new BasicBlock(++id, tempScope);
		controlFlow[CurrentFunc]->basicBlocks.push_back(elseBlock);

		add2Exit();

		this->ElseStmt->analyzeFlow();

		//常规动作，添加无条件跳转节点
		DAGNode* branchNodeElse = new DAGNode(BRANCH_NODE, DAGNodeType::OP_NODE);
		if (controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode) {
			branchNodeElse->relyNodes.push_back(controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode);
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = branchNodeElse;
		}
		else {
			controlFlow[CurrentFunc]->basicBlocks[id]->dag->rootNode = branchNodeElse;
		}
		controlFlow[CurrentFunc]->basicBlocks[id]->dag->nodes.push_back(branchNodeElse);

		DAGNodeList.clear();


		//先新建一个基本块，为if和else的后继，可能为空
		BasicBlock* afterblock = new BasicBlock(++id, tempScope);
		afterblock->label = BasicBlock::LabelType::IF_END;
		//************为ifbody和elsebody添加无条件跳转***************
		branchNodeIf->targets.push_back(id);
		branchNodeElse->targets.push_back(id);

		controlFlow[CurrentFunc]->addEdge(controlFlow[CurrentFunc]->basicBlocks[ifbodyId], afterblock);//连接ifbody
		controlFlow[CurrentFunc]->addEdge(controlFlow[CurrentFunc]->basicBlocks[id - 1], afterblock);//连接elsebody
		controlFlow[CurrentFunc]->basicBlocks.push_back(afterblock);
	}
	else {
		DAGNodeList.clear();
		//新块和ifbody以及if之前的块连接
		BasicBlock* afterblock = new BasicBlock(++id, tempScope);
		afterblock->label = BasicBlock::LabelType::IF_END;
		controlFlow[CurrentFunc]->basicBlocks.push_back(afterblock);

		add2Exit();
		//为ifbody添加无条件跳转
		branchNodeIf->targets.push_back(id);
		controlFlow[CurrentFunc]->addEdge(controlFlow[CurrentFunc]->basicBlocks[id - 1], afterblock);//连接ifbody		
	}
	returnNode = nullptr;
}

void FunctionAST::analyzeFlow()
{
	id = -1;

	CurrentFunc = this->Name;

	//新建初始基本块，赋予基本属性id，加入
	CFG* cfg = new CFG();
	//cfg->basicBlocks.push_back(new BasicBlock(++id));
	//cfg->entry = cfg->basicBlocks[0];
	cfg->name = CurrentFunc;
	controlFlow[CurrentFunc]=cfg;
	this->CompoundStmt->analyzeFlow();

	cfg->entry = cfg->basicBlocks[0];

	//退出函数时复原id
	id = -1;
}

void CompUnitAST::analyzeFlow()
{
	for (size_t i = 0; i < this->TopExpr.size(); i++) {
		if (TopExpr[i]->getClassName() == "Function") {
			BreakId.clear();
			ContinueId.clear();
			TopExpr[i]->analyzeFlow();
			DAGNodeList.clear();
		}
	}
}

GCFG* analyzeControlFlow(std::shared_ptr<ExprAST>ast) {
	gcfg = new GCFG();
	ast->analyzeFlow();
	gcfg->controlFlow = controlFlow;
	return gcfg;
}
