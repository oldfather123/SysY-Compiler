#include "LoopOpimization.h"

//unroll次数4，小于8块的全部展开
#define MAX_LOOP_UNROLL_TIMES 0


//判断每次循环是否进行了展开
static bool hasUnrolling = false;

////////////////////////////////////////////////////////////////////////////

//循环树线性化的递归函数
static void insertTreeNodes(LoopTreeNode* node,vector<LoopTreeNode*>& vec)
{
	for (LoopTreeNode* child : node->childs) {
		insertTreeNodes(child,vec);
	}
	vec.push_back(node);
}

//一个函数的循环树线性化(不加入根结点)
void LoopOptManager::getDFSLoopTrees(const string& name)
{
	this->DFSLoopTrees.clear();
	LoopTreeNode* node = LoopTrees[name];
	for (LoopTreeNode* child : node->childs) {
		insertTreeNodes(child,this->DFSLoopTrees);
	}
}

/////////////////////////////////////////////////////////////////////////////

//求每一个函数的循环树
static void getLoopTreeDeep(BasicBlock* block,LoopTreeNode* root,map<int,int> & visited)
{
	visited[block->id] = 1;
	if (block->label == BasicBlock::LabelType::NO_USE)return;
	if (block->label == BasicBlock::LabelType::WHILE_CONDITION) {
		LoopTreeNode* node = new LoopTreeNode(block, root);
		//必须先检查假出口，再看循环体（真出口），否则碰到break就完蛋了
		if (visited[block->True_False_Exit.second->id] != 1)
			getLoopTreeDeep(block->True_False_Exit.second, root, visited);
		if (visited[block->postBlock[0]->id] != 1)
			getLoopTreeDeep(block->postBlock[0], node, visited);
	}

	for (BasicBlock* toBlock : block->postBlock) {
		if (visited[toBlock->id] != 1) {
			getLoopTreeDeep(toBlock, root, visited);
		}
	}
}

void LoopOptManager::getLoopTree(GCFG* gcfg)
{
	//求循环树
	for (auto it : gcfg->controlFlow) {
		map<int, int> visited;
		LoopTreeNode* root = new LoopTreeNode(nullptr, nullptr);
		getLoopTreeDeep(it.second->entry, root, visited);
		this->LoopTrees[it.first] = root;
	}
}

///////////////////////////////////////////////////////////////////////////////////


static void getCurrentLevel(int current, map<BasicBlock*,int>& visited, BasicBlock* block,BasicBlock* endBlock)
{
	visited[block] = 1;
	if (block->label == BasicBlock::LabelType::NO_USE)return;
	block->LoopLevel = current;

	for (auto tblock : block->postBlock) {
		if (visited[tblock] == 1)continue;
		if (tblock == endBlock)continue;
		getCurrentLevel(current, visited, tblock,endBlock);
	}
}

static void travelAllChilds(int level , LoopTreeNode* node)
{
	map<BasicBlock*, int>visited;
	getCurrentLevel(level, visited, node->block, node->block->True_False_Exit.second);

	for (auto child : node->childs) {
		travelAllChilds(level + 1, child);
	}
}

//获取循环层级
void LoopOptManager::getLoopLevel(GCFG* gcfg)
{
	for (auto it : gcfg->controlFlow) {
		LoopTreeNode* root = this->LoopTrees[it.first];
		for (LoopTreeNode* node : root->childs) {
			travelAllChilds(1, node);
		}
	}
}

////////////////////////////////////

//判断一个条件中变量的数目
//两种变量：ValNode RegNode
static void JudgeVarNums(vector<DAGNode*>& vars,DAGNode* node,int& relationTime)
{
	if (node->nodeType == DAGNodeType::VALUE_NODE){
		vars.push_back(node);
	}

	for (DAGNode* input : node->inputs) {
		JudgeVarNums(vars, input,relationTime);
	}

	//没有rely
	if (node->relyNodes.size() != 0) {
		vars.resize(2, 0);
	}
	
	//没有reg_node
	if (node->nodeType==DAGNodeType::REG_NODE) {
		vars.resize(2, 0);
	}
	//没有数组
	if (node->resultType == ResultType::ipoint || node->resultType == ResultType::fpoint) {
		vars.resize(2, 0);
	}
	//没有函数调用
	if (node->inst == CALL) {
		vars.resize(2, 0);
	}
	//没有奇怪的运算符
	if (node->inst == INSTRUCTIONS::REM) {
		vars.resize(2, 0);
	}
}

//判断是否没有break和continue
static bool JudgeHasBreakOrContinue(BasicBlock* block)
{
	//有continue
	if (block->preBlock.size() != 2)return true;

	//是否有break
	for (auto tBlock :block->True_False_Exit.second->preBlock) {
		if (!tBlock->isCondition)return true;
	}
	return false;
}
//***********判断是不是只有一个分支（暂时的）***************
static bool JudgeHasOnlyBranch(BasicBlock* block)
{
	BasicBlock* current = block;
	while (current->label != BasicBlock::LabelType::WHILEBODY && current->label != BasicBlock::LabelType::IF_END_AND_WHILEBODY) {
		if (current->postBlock.size() != 1)return false;
		if (current->label == BasicBlock::LabelType::WHILE_CONDITION)return false;
		current = current->postBlock[0];
	}
	return true;
}

//递归遍历每一个基本块，检查一个变量有没有发生改变
static bool visitedBlockJudgeDFS(BasicBlock* block,Symbol* sym , map<BasicBlock*,int>&visited,BasicBlock* endBlock)
{
	return false;
}

//判断一个元素在循环中有没有被改变(条件块只有一个)
static bool JudgeIfVarChangedInLoop(LoopTreeNode* loop,Symbol* sym,map<BasicBlock*,int>&visited)
{
	return false;
}

//判断步长DFS
static LoopStep JudgeStepDeep(LoopTreeNode* loop, DAGNode* node,DAGNode* travelNode)
{
	LoopStep currentStep;
	currentStep.type = node->resultType;

	if (travelNode->relyNodes.size() != 0) {
		LoopStep returnStep = JudgeStepDeep(loop, node, travelNode->relyNodes[0]);
		if (returnStep.type == ResultType::VOID)return returnStep;
		currentStep.fstep += returnStep.fstep;
		currentStep.istep += returnStep.istep;
	}
		

	if (travelNode->nodeType == DAGNodeType::STORE) {
		if (travelNode->inputs[0]->nodeType == DAGNodeType::VALUE_NODE && travelNode->inputs[0]->symbol == node->symbol) {
			if (travelNode->inputs[1]->name == SUBTRACT || travelNode->inputs[1]->name == PLUS) {
				DAGNode* calNode = travelNode->inputs[1];
				//***************暂时只考虑一边直接是变量另一边直接是常数的情况
				if (calNode->inputs[0]->nodeType == DAGNodeType::LOAD && calNode->inputs[0]->inputs[0]->symbol == node->symbol) {
					if (calNode->inputs[1]->nodeType == DAGNodeType::CONSTANT) {
						if (calNode->inputs[1]->resultType == ResultType::i32) {
							currentStep.istep += calNode->inputs[1]->value.iValue;
						}
						else {
							currentStep.fstep += calNode->inputs[1]->value.fValue;
						}	
					}
				}
				else if (calNode->inputs[1]->nodeType == DAGNodeType::VALUE_NODE && calNode->inputs[1]->symbol == node->symbol) {
					if (calNode->inputs[0]->nodeType == DAGNodeType::CONSTANT) {
						if (calNode->inputs[0]->resultType == ResultType::i32) {
							currentStep.istep += calNode->inputs[0]->value.iValue;
						}
						else {
							currentStep.fstep += calNode->inputs[0]->value.fValue;
						}
					}
				}
			}
			else {
				currentStep.type = ResultType::VOID;
			}
		}
	}

	return currentStep;
}

//判断步长（循环里面套循环的时候，除非里面没有对该循环变量的赋值，否则步长先认为不确定）********暂时只会对里面没有循环的进行分析*********
static void JudgeStep(LoopTreeNode* loop,DAGNode* node)
{
	BasicBlock* current = loop->block->True_False_Exit.first;
	LoopStep result;
	while (1) {
		LoopStep temp = JudgeStepDeep(loop, node, current->dag->rootNode);
		if (temp.type == ResultType::VOID) {
			loop->step = temp;
			return;
		}
		result.type = temp.type;
		result.istep += temp.istep;
		result.fstep += temp.fstep;
		
		if (current->label == BasicBlock::LabelType::WHILEBODY || current->label == BasicBlock::LabelType::IF_END_AND_WHILEBODY) {
			break;
		}
		current = current->postBlock[0];
	}
	loop->step = result;
}

//确定边界
static void JudgeBoarder(LoopTreeNode* loop, DAGNode* node)
{
	BasicBlock* condition = loop->block;
	DAGNode* conditionNode = condition->dag->rootNode->inputs[0];
	if (conditionNode->name == LESS_THAN) {	
		if (conditionNode->inputs[0]->nodeType == DAGNodeType::LOAD && conditionNode->inputs[0]->inputs[0]->symbol == node->symbol) {
			loop->boarderVal.type = node->resultType;
			loop->conditionType = ConditionType::LESS;
			if (conditionNode->inputs[1]->nodeType == DAGNodeType::CONSTANT) {
				if (node->resultType == i32) {
					loop->boarderVal.ival = conditionNode->inputs[1]->value.iValue;
				}
				else {
					loop->boarderVal.fval = conditionNode->inputs[1]->value.fValue;
				}
				return;
			}
		}
	}
	loop->boarderVal.type == ResultType::VOID;
}

//确定初始值
static void JudgeInitialValue(LoopTreeNode* loop,DAGNode* dagnode,DAGNode* node)
{
	DAGNode* current = dagnode;
	while (1) {
		if (current->nodeType == DAGNodeType::STORE) {
			if (current->inputs[0]->nodeType == DAGNodeType::VALUE_NODE && current->inputs[0]->symbol == node->symbol) {
				if (current->inputs[1]->nodeType == DAGNodeType::CONSTANT) {
					loop->initialVal.type = node->resultType;
					if (node->resultType == i32) {
						loop->initialVal.ival = current->inputs[1]->value.iValue;
					}
					else {
						loop->initialVal.fval = current->inputs[1]->value.fValue;
					}
					return;
				}
				else {
					loop->initialVal.type = VOID;
					return;
				}
			}
		}
		if (current->relyNodes.size() == 0)break;
		else current = current->relyNodes[0];
	} 
	loop->initialVal.type = VOID;
	return;
}

//判断一个循环是否能展开
static void JudgeIfCanUnroll(LoopTreeNode* loop) {
	BasicBlock* condition = loop -> block;
	//while(1) 或 while(0 --- 加其他条件)不展开
	if (condition->postBlock.size() == 1)return;
	//条件只有一个基本块
	else if (condition->postBlock[0] == condition->True_False_Exit.first && condition->postBlock[1] == condition->True_False_Exit.second) {
		vector<DAGNode*> vars;
		int relationTime = 0;
		JudgeVarNums(vars,condition->dag->rootNode, relationTime);
		//1.条件里只能有一个变量（并且只能用一次）
		if (vars.size() != 1)return;
		//2.没有continue和break
		if (JudgeHasBreakOrContinue(condition)) return;
		//3.****************暂时的，循环只有一个分支***************
		if (!JudgeHasOnlyBranch(condition->True_False_Exit.first))return;
		//4.确定步长
		JudgeStep(loop, vars[0]);
		if (loop->step.type == ResultType::VOID)return;
		//5.确定边界
		JudgeBoarder(loop, vars[0]);
		if (loop->boarderVal.type == ResultType::VOID)return;
		//6.确定初值
		for (auto block : loop->block->preBlock) {
			if (block->label != BasicBlock::LabelType::WHILEBODY && block->label != BasicBlock::LabelType::IF_END_AND_WHILEBODY) {
				JudgeInitialValue(loop, block->dag->rootNode, vars[0]);
				break;
			}
		}
		if (loop->initialVal.type == ResultType::VOID)return;
		//7.确定循环次数
		if (loop->conditionType == ConditionType::LESS) {
			if (loop->boarderVal.type == ResultType::i32) {
				loop->times = (loop->boarderVal.ival - loop->initialVal.ival) / loop->step.istep;
			}
			else return;
		}
		loop->changableVar = vars[0]->symbol;
		//8.可以展开
		loop->canUnroll = true;
	}
}

//记录一个节点是不是被拷贝了，如果拷贝了，就返回拷贝后的节点，没有的话返回nullptr
static map<DAGNode*, DAGNode*> hasCopiedNode_loop;
static DAGNode* judgeIfCopied(DAGNode* node) {
	auto it = hasCopiedNode_loop.find(node);
	if (it != hasCopiedNode_loop.end()) {
		return it->second;
	}
	return nullptr;
}

//拷贝一段dagnode
//mode 0 为block拷贝时，mode 1 为afterblock拷贝时
static DAGNode* copyDagnode_loop(DAGNode* copyRoot)
{
	if (!copyRoot)return nullptr;
	DAGNode* newNode = judgeIfCopied(copyRoot);
	if (!newNode) {
		//没有拷贝过，拷贝本节点
		if (copyRoot->symbol) {
			newNode = new DAGNode(copyRoot, copyRoot->symbol);
		}
		else {
			newNode = new DAGNode(copyRoot, nullptr);
		}
		//加入已经拷贝过的map
		hasCopiedNode_loop[copyRoot] = newNode;
	}
	else {
		return newNode;
	}

	//拷贝依赖节点
	if (copyRoot->relyNodes.size() != 0) {
		for (size_t i = 0;i < copyRoot->relyNodes.size();i++) {
			DAGNode* returnNode = copyDagnode_loop(copyRoot->relyNodes[i]);
			if(returnNode)
				newNode->relyNodes.push_back(returnNode);
		}
	}

	//拷贝输入节点
	if (copyRoot->inputs.size() != 0) {
		for (size_t i = 0;i < copyRoot->inputs.size();i++) {
			DAGNode* returnNode = copyDagnode_loop(copyRoot->inputs[i]);
			newNode->inputs.push_back(returnNode);
			returnNode->outputs.push_back(newNode);
		}
	}

	return newNode;
}

//记录一个基本块是不是被拷贝了，如果拷贝了，就返回拷贝后的块，没有的话返回nullptr
static map<BasicBlock*, BasicBlock*> hasCopiedBlock_LOOP;
static BasicBlock* judgeIfCopied_LOOP(BasicBlock* block) {
	auto it = hasCopiedBlock_LOOP.find(block);
	if (it != hasCopiedBlock_LOOP.end()) {
		return it->second;
	}
	return nullptr;
}

//重新恢复dag的nodes成员
static void reBuildNodes_loop(DAG* dag, DAGNode* node)
{
	if (!node)return;
	if (node->relyNodes.size() != 0) {
		for (size_t i = 0;i < node->relyNodes.size();i++) {
			reBuildNodes_loop(dag, node->relyNodes[i]);
		}
	}

	if (node->inputs.size() != 0) {
		for (size_t i = 0;i < node->inputs.size();i++) {
			reBuildNodes_loop(dag, node->inputs[i]);
		}
	}

	for (size_t i = 0;i < dag->nodes.size();i++) {
		if (dag->nodes[i] == node) return;
	}
	dag->nodes.push_back(node);
}

static BasicBlock* endBlock;

//递归拷贝基本块(currrentval是当前循环的值)
static BasicBlock* insertTargetBlock_LOOP(CFG* sourcecfg, BasicBlock* block)
{
	BasicBlock* newBlock = judgeIfCopied_LOOP(block);
	if (!newBlock) {
		newBlock = new BasicBlock(block, block->scope);

		hasCopiedNode_loop.clear();
		newBlock->dag->rootNode = copyDagnode_loop(block->dag->rootNode);
		//将新的基本块插入cfg，并且重新编号
		sourcecfg->basicBlocks.push_back(newBlock);
		newBlock->id = sourcecfg->basicBlocks.size() - 1;
		newBlock->dag->nodes.clear();
		reBuildNodes_loop(newBlock->dag, newBlock->dag->rootNode);
		hasCopiedBlock_LOOP[block] = newBlock;
		if (block->label == BasicBlock::LabelType::WHILEBODY || block->label == BasicBlock::LabelType::IF_END_AND_WHILEBODY) {
			newBlock->label = BasicBlock::LabelType::NONE;
			endBlock = newBlock;
			return newBlock;
		}
	}
	else {
		return newBlock;
	}

	if (block->postBlock.size() != 0 ) {
		for (BasicBlock* subBlock : block->postBlock) {
			BasicBlock* returnBlock = insertTargetBlock_LOOP(sourcecfg,  subBlock);
			newBlock->postBlock.push_back(returnBlock);
			returnBlock->preBlock.push_back(newBlock);
			newBlock->dag->rootNode->targets.push_back(returnBlock->id);
		}
	}

	return newBlock;
}

//把i替换成常量
void ExamineSymbol(DAGNode* node, Symbol* sym, int currentVal)
{
	if (node->nodeType == DAGNodeType::LOAD && node->inputs[0]->symbol == sym) {
		node->nodeType = DAGNodeType::CONSTANT;
		node->resultType == ResultType::i32;
		node->value.iValue = currentVal;
		node->inputs.clear();
	}

	for (auto tnode : node->inputs) {
		ExamineSymbol(tnode, sym, currentVal);
	}
}

//合并
void MergeAfterLoopBlocks(BasicBlock* beginBlock, BasicBlock* afterBlock,Symbol* sym,int currentVal)
{
	BasicBlock* currentBlock = afterBlock;
	while (1) {
		DAGNode* currentNode = currentBlock->dag->rootNode;
		DAGNode* beforeNode = currentNode;
		while (1) {
			if (currentNode->nodeType == DAGNodeType::STORE) {
				//跳过对i的赋值
				if (currentNode->inputs[0]->symbol == sym) {
					beforeNode->relyNodes.clear();
					beforeNode->relyNodes= currentNode->relyNodes;
					currentNode = beforeNode;
				}
			}

			//替换对i的使用
			ExamineSymbol(currentNode, sym, currentVal);

			if (currentNode->relyNodes.size() == 0) {
				if (currentBlock->preBlock[0] != beginBlock) {
					auto tempBlock = currentBlock->preBlock[0];
					//如果tempBlock只有一个跳转语句
					while (tempBlock->dag->rootNode->relyNodes.size() == 0 && tempBlock->preBlock[0] != beginBlock) {
						tempBlock = tempBlock->preBlock[0];
					}

					if (tempBlock->preBlock[0] != beginBlock) {
						beforeNode = currentNode;
						currentNode->relyNodes.push_back(tempBlock->dag->rootNode->relyNodes[0]);
						currentNode = currentNode->relyNodes[0];
					}		

					currentBlock->preBlock = tempBlock->preBlock;
				}

				
				if (currentBlock->preBlock[0] == beginBlock)break;
			}
			else {
				if (currentNode == beforeNode) {
					currentNode = currentNode->relyNodes[0];
				}
				else {
					beforeNode = currentNode;
					currentNode = currentNode->relyNodes[0];				
				}
			}
		}
		
		beginBlock->postBlock.clear();
		beginBlock->postBlock.push_back(currentBlock);

		currentBlock->preBlock.clear();
		currentBlock->preBlock.push_back(beginBlock);
		
		beginBlock->dag->rootNode->targets.clear();
		beginBlock->dag->rootNode->targets.push_back(currentBlock->id);

		break;
	}
}

//开始循环展开，假设全部展开**********
static void StartLoopUnrollingAll(CFG* sourceCFG,LoopTreeNode* loop)
{
	BasicBlock* beforeBlock = new BasicBlock(1,nullptr);
	for (auto tblock : loop->block->preBlock) {
		if (tblock->label != BasicBlock::LabelType::WHILEBODY && tblock->label != BasicBlock::LabelType::IF_END_AND_WHILEBODY)
			beforeBlock = tblock;
	}
	//1.清除原循环出口和前继块的跳转
	BasicBlock* falseBlock = loop->block->True_False_Exit.second;
	falseBlock->preBlock.clear();

	auto it = find(loop->block->preBlock.begin(), loop->block->preBlock.end(), beforeBlock);
	loop->block->preBlock.erase(it);

	beforeBlock->postBlock.clear();
	beforeBlock->dag->rootNode->targets.clear();

	//先全部当做递增来处理
	for (int i = loop->initialVal.ival;i < loop->initialVal.ival + loop->times;i++) {
		hasCopiedBlock_LOOP.clear();
		BasicBlock* block = insertTargetBlock_LOOP(sourceCFG,loop->block->True_False_Exit.first);
		//2.连接前中
		beforeBlock->postBlock.push_back(block);
		block->preBlock.push_back(beforeBlock);
		beforeBlock->dag->rootNode->targets.push_back(block->id);

		//4.既然全部展开了，说明可以合并所有的基本块
		MergeAfterLoopBlocks(beforeBlock, endBlock, loop->changableVar, (i - loop->initialVal.ival) * loop->step.istep);

		beforeBlock = endBlock;
	}
	//3.连接中后，既然是全部展开，那么就可以直接跳过这个循环了
	endBlock->postBlock.push_back(falseBlock);
	endBlock->dag->rootNode->targets.push_back(falseBlock->id);
	falseBlock->preBlock.push_back(endBlock);

	
}

//循环展开入口函数
void LoopOptManager::LoopUnrolling(GCFG* gcfg,SymbolTable* symbolTable)
{	
	this->getLoopTree(gcfg);

	//判断一个循环能否展开：遍历所有条件块，1.只有一个变量 2.步长确定 3.只有一个条件语句块（暂时）4.没有continue和break
	//每个函数进行循环展开
	//(如果以后展开之后有问题，比如部分展开，可以重新生成循环树)
	for (auto it : gcfg->controlFlow) {
		do {
			getDFSLoopTrees(it.first);
			for (auto loop : DFSLoopTrees) {
				JudgeIfCanUnroll(loop);
				if (loop->canUnroll) {
					//全部展开
					if (loop->times < MAX_LOOP_UNROLL_TIMES && loop->times>0) {
						if (loop->childTimes <= 0) {
							StartLoopUnrollingAll(it.second, loop);
							auto it = find(loop->parent->childs.begin(), loop->parent->childs.end(), loop);
							loop->parent->childs.erase(it);
							loop->parent->childTimes = loop->childTimes * loop->times;
						}					
					}				
				}
			}
		} while (hasUnrolling);
	}
	
}