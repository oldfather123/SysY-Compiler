#pragma once
#ifndef CFG_H
#define CFG_H

#include "BasicBlock.h"
#include<queue>

class CFG
{
private:

public:
	long reCount;//虚拟寄存器的计数器
	BasicBlock* entry;
	vector<BasicBlock*> basicBlocks;//存储这个函数内所有的基本块
	vector<Edge*>edges;
	string name;

	map<string, pair<int, int>>arrayStack;//一个函数的数组栈空间
	int maxStackSize = 0;
	int funcStackSize = 0;

	CFG() { reCount = 0; };

	//为两个块之间添加前驱和后继关系，并在cfg中添加控制流边
	void addEdge(BasicBlock* outBlock, BasicBlock* inBlock) {
		Edge* edge = new Edge(outBlock, inBlock);
		outBlock->postBlock.push_back(inBlock);
		inBlock->preBlock.push_back(outBlock);
		this->edges.push_back(edge);
	}

	void pump() {
		for (BasicBlock* iter : basicBlocks) {
			iter->pump();
		}
		cout << endl << endl << endl;
	}

	//通过visited，标记没有访问的节点
	void labeledNoUseBlock(map<int,int>&visited,BasicBlock* block) {
		for (BasicBlock* b : block->postBlock) {
			if (visited[b->id] == 0) {
				visited[b->id] = 1;
				labeledNoUseBlock(visited, b);
			}		
		}
	}

	//重新添加block到result
	void insertBlock(vector<BasicBlock*>& result, map<BasicBlock*, int>&inDegree,BasicBlock* block) {
		result.push_back(block);
		for (BasicBlock* afterBlock : block->postBlock) {
			inDegree[afterBlock]--;
			if (inDegree[afterBlock] == 0) {
				insertBlock(result, inDegree, afterBlock);
			}
		}
	}

	//拓扑排序，并删除无用块
	void TopoSorting() {
		//先遍历获得no_use不可达块，并且解除其后继的前置关系
		map<int,int> visited;
		visited[entry->id] = 1;
		labeledNoUseBlock(visited, entry);
		for (size_t i = 0;i < visited.size();i++) {
			if (visited[i] == 0) {
				basicBlocks[i]->label = BasicBlock::LabelType::NO_USE;
				for (BasicBlock* block : basicBlocks[i]->postBlock) {
					auto it = find(block->preBlock.begin(), block->preBlock.end(), basicBlocks[i]);
					block->preBlock.erase(it);
				}
			}				
		}
		//拓扑排序
		map<BasicBlock*, int> inDegree;//入度表
		for (BasicBlock* block : basicBlocks) {
			if (block->label != BasicBlock::LabelType::NO_USE) {
				if (block->label == BasicBlock::LabelType::WHILE_CONDITION) {
					inDegree[block] = 1;
				}
				else {
					inDegree[block] = block->preBlock.size();
				}					
			}
		}
		vector<BasicBlock*>result;
		insertBlock(result, inDegree,entry);
		//重新编号
		for (size_t i = 0;i < result.size();i++) {
			result[i]->id = i;
		}
		//修改target
		for (size_t i = 0;i < result.size();i++) {
			if(result[i]->dag->rootNode)
				result[i]->dag->rootNode->targets.clear();
			for (BasicBlock* block : result[i]->postBlock) {
				result[i]->dag->rootNode->targets.push_back(block->id);
			}
		}
		//去掉多余的while_condition(没有preblock)
		for (auto block : result) {
			if (block->label == BasicBlock::LabelType::WHILE_CONDITION) {
				if (block->preBlock.size() == 1)
					block->label = BasicBlock::LabelType::NONE;
			}
		}
		basicBlocks = result;
	}

	void cfg_traversal() {
		//int count = 0;
		/*******************记得改********************************/
		reCount = 0;
		for (auto blockIt = basicBlocks.begin(); blockIt != basicBlocks.end(); blockIt++) {

			for (auto nodeIt = (*blockIt)->dag->nodes.begin(); nodeIt != (*blockIt)->dag->nodes.end(); ++nodeIt) {

				//对结点的处理
				if ((*nodeIt)->nodeType == LOAD) {
					reCount++;
					(*nodeIt)->resultId = reCount;
					//cout << "%" << reCount << " load " << (*nodeIt)->inputs[0]->name << endl;
				}
				else if ((*nodeIt)->nodeType == OP_NODE) {
					reCount++;
					(*nodeIt)->resultId = reCount;
				}
				else if ((*nodeIt)->nodeType == CONSTANT && (*nodeIt)->resultType == f32) {
					reCount++;
					(*nodeIt)->resultId = reCount;
				}
			}
			//count++;
		}
	}

	//获取当前可用虚拟寄存器
	long getCount() {
		reCount++;
		return reCount;
	}
};


	

#endif // !CFG_H


