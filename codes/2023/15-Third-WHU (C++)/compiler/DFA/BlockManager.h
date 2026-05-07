#pragma once
#include "../IR/BasicBlock.h"
#include<set>

class BlockManager {
public:
	set<DAGNode*> nodes;
	//移除不可达节点
	void remove_unreachableNodes(BasicBlock* block) {
		int n = block->dag->nodes.size();
		if (n <= 0)
			return;
		nodes.clear();//初始化
		dfs_remove(block->dag->rootNode);
		block->dag->nodes.assign(nodes.begin(), nodes.end());
	}

	void dfs_remove(DAGNode* root) {
		nodes.insert(root);
		for (int i = 0; i < root->inputs.size(); i++) {
			if (nodes.find(root->inputs[i]) == nodes.end())
				dfs_remove(root->inputs[i]);
		}
		for (int j = 0; j < root->relyNodes.size(); j++) {
			if (nodes.find(root->relyNodes[j]) == nodes.end())
				dfs_remove(root->relyNodes[j]);
		}
	}
};