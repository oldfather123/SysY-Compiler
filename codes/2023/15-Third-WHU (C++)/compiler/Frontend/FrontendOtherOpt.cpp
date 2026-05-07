#include "FrontendOtherOpt.h"
#include "../DFA/LoopOpimization.h"

static map<Symbol*, InitialVal> sym_to_val;

static bool ischanged_branch = false;

//第一个表示能不能移除，第二个表示怎么移除
static pair<bool, bool> tryRemoveBlock(BasicBlock* block)
{
	pair<bool, bool>result;
	if (block->dag->rootNode->relyNodes.size() != 0 || block->dag->rootNode->name != BRANCH_NODE) {
		result.first = false;
		return result;
	}

	DAGNode* root = block->dag->rootNode->inputs[0];
	if (root->inputs.size() != 2) {
		result.first = false;
		return result;
	}
	if (root->inputs[0]->nodeType == DAGNodeType::LOAD && root->inputs[0]->inputs[0]->nodeType == DAGNodeType::VALUE_NODE &&root->inputs[1]->nodeType == DAGNodeType::CONSTANT) {
		if (root->name == GREATER_THAN) {
			if (sym_to_val.find(root->inputs[0]->inputs[0]->symbol) != sym_to_val.end()) {
				if (root->inputs[0]->inputs[0]->symbol->type == i32) {
					if (sym_to_val[root->inputs[0]->inputs[0]->symbol].ival > root->inputs[1]->value.iValue) {
						result.first = true;
						result.second = true;
					}
					else {
						result.first = true;
						result.second = false;
					}
				}
				else {
					if (sym_to_val[root->inputs[0]->inputs[0]->symbol].fval > root->inputs[1]->value.fValue) {
						result.first = true;
						result.second = true;
					}
					else {
						result.first = true;
						result.second = false;
					}
				}
				return result;
			}
		}
		else if (root->name == LESS_THAN) {
			if (sym_to_val.find(root->inputs[0]->inputs[0]->symbol) != sym_to_val.end()) {
				if (root->inputs[0]->inputs[0]->symbol->type == i32) {
					if (sym_to_val[root->inputs[0]->inputs[0]->symbol].ival < root->inputs[1]->value.iValue) {
						result.first = true;
						result.second = true;
					}
					else {
						result.first = true;
						result.second = false;
					}
				}
				else {
					if (sym_to_val[root->inputs[0]->inputs[0]->symbol].fval < root->inputs[1]->value.fValue) {
						result.first = true;
						result.second = true;
					}
					else {
						result.first = true;
						result.second = false;
					}
				}
				return result;
			}
		}
	}
	result.first = false;
	return result;
}

static void travelDAG(DAGNode* node)
{
	pair<bool, bool> result;
	DAGNode* currentNode = node;
	while (1) {

		if (currentNode->nodeType == DAGNodeType::STORE) {
			if (currentNode->inputs[1]->nodeType == DAGNodeType::CONSTANT) {
				InitialVal init;
				init.type = currentNode->inputs[1]->resultType;
				if (init.type == ResultType::i32)
					init.ival = currentNode->inputs[1]->value.iValue;
				else
					init.fval = currentNode->inputs[1]->value.fValue;

				if (sym_to_val.find(currentNode->inputs[0]->symbol) == sym_to_val.end()) {
					sym_to_val[currentNode->inputs[0]->symbol] = init;
				}
			}
		}

		if (currentNode->relyNodes.size() == 0)break;
		else currentNode = currentNode->relyNodes[0];
	}
}

static void tryRemoveBranch(BasicBlock* block)
{
	for (auto tblock : block->preBlock) {
		if (tblock->label != BasicBlock::LabelType::WHILEBODY && tblock->label != BasicBlock::LabelType::IF_END_AND_WHILEBODY) {
			sym_to_val.clear();
			travelDAG(tblock->dag->rootNode);
			pair<bool, bool> result = tryRemoveBlock(block);
			sym_to_val.clear();

			if (!result.first)return;
			if (block->label == BasicBlock::LabelType::WHILE_CONDITION) {
				if (result.second) return;
			}

			block->dag->rootNode->targets.clear();
			block->dag->rootNode->inputs.clear();
			if (result.second) {
				BasicBlock* deleteBlock = block->postBlock[1];
				auto it = find(deleteBlock->preBlock.begin(), deleteBlock->preBlock.end(), block);
				deleteBlock->preBlock.erase(it);
				block->postBlock.pop_back();
				block->dag->rootNode->targets.push_back(block->postBlock[0]->id);
			}
			else {
				BasicBlock* deleteBlock = block->postBlock[0];
				auto it = find(deleteBlock->preBlock.begin(), deleteBlock->preBlock.end(), block);
				deleteBlock->preBlock.erase(it);
				auto it2 = block->postBlock.begin();
				block->postBlock.erase(it2);
				block->dag->rootNode->targets.push_back(block->postBlock[0]->id);
			}
			break;
		}
	}
}

static void visitAllBlock(BasicBlock* block,map<BasicBlock*,int>&visited)
{
	visited[block] = 1;
	if (block->postBlock.size() > 1) {
		tryRemoveBranch(block);
	}

	for (auto tblock : block->postBlock) {
		if (visited[tblock] != 1) {
			visitAllBlock(tblock,visited);
		}
	}
}

void FrontendDecreaseBranch(GCFG* gcfg)
{
	for (auto it : gcfg->controlFlow) {
		map<BasicBlock*, int> visited;
		visitAllBlock(it.second->entry,visited);
	}
}