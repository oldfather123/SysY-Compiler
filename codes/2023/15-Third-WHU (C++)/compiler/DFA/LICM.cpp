#include"LoopOpimization.h"
#include<unordered_map>

static bitset<SIZE> result;

//记录已经被处理过的stmt，就不再使用新的节点:如果是兄弟节点使用的话，那么
static unordered_map<string, long>hasMovedStmtCurrent;

static LoopTreeNode* currentNode;

//判断在这一轮中有没有进行不变式外提
static bool isChanged = false;

//记录一个节点是不是被拷贝了，如果拷贝了，就返回拷贝后的节点，没有的话返回nullptr
static map<DAGNode*, DAGNode*> hasCopiedNode_LICM;
static DAGNode* judgeIfCopied_LICM(DAGNode* node) {
	auto it = hasCopiedNode_LICM.find(node);
	if (it != hasCopiedNode_LICM.end()) {
		return it->second;
	}
	return nullptr;
}

//拷贝一段dagnode
//mode 0 为block拷贝时，mode 1 为afterblock拷贝时
static DAGNode* copyDagnode_LICM(DAGNode* copyRoot,CFG* cfg)
{
	DAGNode* newNode = judgeIfCopied_LICM(copyRoot);
	if (!newNode) {
		//没有拷贝过，拷贝本节点
		if (copyRoot->symbol) {
			newNode = new DAGNode(copyRoot, copyRoot->symbol);
		}
		else {
			newNode = new DAGNode(copyRoot, nullptr);
		}
		if (newNode->nodeType == DAGNodeType::OP_NODE) {
			newNode->resultId = cfg->getCount();
		}
		//加入已经拷贝过的map
		hasCopiedNode_LICM[copyRoot] = newNode;
	}
	else {
		return newNode;
	}

	//拷贝输入节点
	if (copyRoot->inputs.size() != 0) {
		for (size_t i = 0;i < copyRoot->inputs.size();i++) {
			DAGNode* returnNode = copyDagnode_LICM(copyRoot->inputs[i],cfg);
			newNode->inputs.push_back(returnNode);
			returnNode->outputs.push_back(newNode);
		}
	}

	return newNode;
}

//将op_node转换为寄存器节点
//void transform_to_regnode(DAG* dag, DAGNode* node, long id) {
//	node->nodeType = REG_NODE;
//	node->resultId = id;
//
//	vector<DAGNode*> tempInputs = node->inputs;
//	for (int i = 0; i < tempInputs.size(); i++) {
//		dag->remove_edge(node, tempInputs[i]);//移除边
//		dag->remove_node(tempInputs[i]);//如果没有与当前常量值相连边的话，删除该常量节点
//	}
//}

//遍历直到找到不变式，拷贝，然后外提
static void findAndMoveDag(DAG* dag,const unordered_map<string,int>& difference, DAGNode* node, BasicBlock* insertBlock,DataFlowAnalysis* dfa)
{
	if (node->nodeType == DAGNodeType::OP_NODE) {
		long id = node->resultId;

		if (dfa->reg_exp.find(id) != dfa->reg_exp.end()) {
			string str = dfa->reg_exp[id];

			if (difference.find(str) != difference.end()) {

				//本循环内没有处理过
				if (hasMovedStmtCurrent.find(str) == hasMovedStmtCurrent.end()) {
					hasCopiedNode_LICM.clear();
					DAGNode* returnNode = copyDagnode_LICM(node, dfa->cfg);
					returnNode->relyNodes = insertBlock->dag->rootNode->relyNodes;
					insertBlock->dag->rootNode->relyNodes.clear();
					insertBlock->dag->rootNode->relyNodes.push_back(returnNode);

					//transform_to_regnode(dag, node, returnNode->resultId);
					hasMovedStmtCurrent[str] = 1;
					//hasMovedStmtGlobal[str] = make_pair(returnNode->resultId, currentNode);
				}
				//else {
				//	transform_to_regnode(dag, node, hasMovedStmtCurrent[str]);
				//}
				
			}
		}
		
	}

	if (node->relyNodes.size() != 0) {
		for (DAGNode* relyNode : node->relyNodes) {
			findAndMoveDag(dag,difference, relyNode, insertBlock, dfa);
		}
	}

	if (node->inputs.size() != 0) {
		for (DAGNode* inputNode : node->inputs) {
			findAndMoveDag(dag,difference, inputNode, insertBlock, dfa);
		}
	}
}

static void travel_basicblock(BasicBlock* block, BasicBlock* insertBlock,BasicBlock* endBlock,map<BasicBlock*, int>& visited, const unordered_map<string,int>& diffenrence, DataFlowAnalysis* dfa)
{
	visited[block] = 1;
	findAndMoveDag(block->dag,diffenrence, block->dag->rootNode, insertBlock,dfa);

	for (auto tblock : block->postBlock) {
		if (visited[tblock] != 1 && tblock != endBlock) {
			travel_basicblock(tblock, insertBlock, endBlock, visited, diffenrence, dfa);
		}
	}

}

//遍历直到找到不变式，拷贝，然后外提
static void findAndMoveInst(const unordered_map<string,int>& diffenrence,BasicBlock* beforeBlock,BasicBlock* conditionBlock, DataFlowAnalysis* dfa)
{
	map<BasicBlock*, int>visited;
	travel_basicblock(conditionBlock, beforeBlock,conditionBlock->True_False_Exit.second, visited, diffenrence, dfa);
}

//重新恢复dag的nodes成员
static void reBuildNodes_LICM(DAG* dag, DAGNode* node)
{
	if (!node)return;
	if (node->relyNodes.size() != 0) {
		for (size_t i = 0;i < node->relyNodes.size();i++) {
			reBuildNodes_LICM(dag, node->relyNodes[i]);
		}
	}

	if (node->inputs.size() != 0) {
		for (size_t i = 0;i < node->inputs.size();i++) {
			reBuildNodes_LICM(dag, node->inputs[i]);
		}
	}

	for (size_t i = 0;i < dag->nodes.size();i++) {
		if (dag->nodes[i] == node) return;
	}
	dag->nodes.push_back(node);
}

//去掉所有循环体里面的kill表达式
//static void LICM_Remove_Kill(BasicBlock* block, BasicBlock* endblock,map<BasicBlock*,int>&visited)
//{
//	visited[block] = 1;
//	result &= (~block->dataInfo->AvaKills);
//
//	for (auto tblock : block->postBlock) {
//		if (visited[tblock] != 1 && tblock != endblock) {
//			LICM_Remove_Kill(tblock, endblock, visited);
//		}
//	}
//}
static vector<BasicBlock*> kill_result;
static void LICM_Find_kill(BasicBlock* block, BasicBlock* endblock,map<BasicBlock*, int>& visited)
{
	visited[block] = 1;
	kill_result.push_back(block);

	for (auto tblock : block->postBlock) {
		if (visited[tblock] != 1 && tblock != endblock) {
			LICM_Find_kill(tblock, endblock, visited);
		}
	}
}


//根据每一个结尾块提出不变式
static void LICM_Insert(BasicBlock* beforeBlock, BasicBlock* conditionBlock,DataFlowAnalysis* dfa) {
	AvaExp* avaExp = new AvaExp();
	//设成全1，然后方便后面求交集
	result.set();
	for (BasicBlock* toMoveBlock : conditionBlock->preBlock) {
		if (toMoveBlock != beforeBlock) {
			result &= toMoveBlock->dataInfo->AvaOutSet;	
		}
	}
	//把从直接前继的块的产生去掉
	result &= (~beforeBlock->dataInfo->AvaOutSet);
	//result = (result & (~conditionBlock->dataInfo->AvaKills)) & (~conditionBlock->dataInfo->AvaGens);
	//去掉所有循环体里面的kill表达式
	kill_result.clear();
	map<BasicBlock*, int>visited;
	LICM_Find_kill(conditionBlock, conditionBlock->True_False_Exit.second, visited);
	for (auto kill : kill_result) {
		result &= (~kill->dataInfo->AvaKills);
	}

	kill_result.clear();

	vector<string> result_string = beforeBlock->dataInfo->bitSet_to_vector(result, dfa->ava_exp_vec);
	unordered_map<string, int> result_map;
	for (auto res : result_string) {
		result_map[res] = 1;
	}

	findAndMoveInst(result_map, beforeBlock, conditionBlock, dfa);
	beforeBlock->dag->nodes.clear();
	reBuildNodes_LICM(beforeBlock->dag, beforeBlock->dag->rootNode);
}

//找到最深的没有被处理的循环进行处理
static void LCIM_PROC(BasicBlock* conditionBlock, DataFlowAnalysis* dfa) 
{
	//找到那个不是body的块，开始进行不变式分析
	for (BasicBlock* block : conditionBlock->preBlock) {
		if (block->label != BasicBlock::LabelType::WHILEBODY && block->label != BasicBlock::LabelType::IF_END_AND_WHILEBODY) {			
			LICM_Insert(block, conditionBlock,dfa);
			break;
		}
	}
}

void LoopOptManager::LICM_OPT(GCFG*gcfg,DFA* dfa,SymbolTable* table)
{
	this->getLoopTree(gcfg);
	for (auto it : dfa->cfg_dfa) {	
		this->getDFSLoopTrees(it.first->name);
		for (auto loop : this->DFSLoopTrees) {
			currentNode = loop;
			hasMovedStmtCurrent.clear();
			it.second->availableExpressionsAnalysis(table,0);
			LCIM_PROC(loop->block, it.second);
		}
	}
}