//#include<iostream>
//#include"SSA.h"
//
//
//int main() {
//	//创建节点
//	DAGNode *node1=new DAGNode();
//	node1->name = "a";
//	node1->nodeType = VALUE_NODE;
//
//	DAGNode *node2 = new DAGNode();
//	node2->name = "b";
//	node2->nodeType = VALUE_NODE;
//
//	DAGNode *node3 = new DAGNode();
//	node3->name = "=";
//	node3->nodeType = STORE;
//	node3->inputs.push_back(node1);
//	node3->inputs.push_back(node2);
//
//	DAGNode* node4 = new DAGNode();
//	node4->name = "a";
//	node4->nodeType = VALUE_NODE;
//
//	DAGNode* node5 = new DAGNode();
//	node5->name = "c";
//	node5->nodeType = VALUE_NODE;
//
//	DAGNode* node6 = new DAGNode();
//	node6->name = "=";
//	node6->nodeType = STORE;
//	node6->inputs.push_back(node4);
//	node6->inputs.push_back(node5);
//
//	DAGNode* node7 = new DAGNode();
//	node7->name = "b";
//	node7->nodeType = VALUE_NODE;
//
//	DAGNode* node8 = new DAGNode();
//	node8->name = "c";
//	node8->nodeType = VALUE_NODE;
//
//	DAGNode* node9 = new DAGNode();
//	node9->name = "=";
//	node9->nodeType = STORE;
//	node9->inputs.push_back(node7);
//	node9->inputs.push_back(node8);
//
//	DAGNode* node10 = new DAGNode();
//	node10->name = "a";
//	node10->nodeType = VALUE_NODE;
//
//	DAGNode* node11 = new DAGNode();
//	node11->name = "b";
//	node11->nodeType = VALUE_NODE;
//
//	DAGNode* node12 = new DAGNode();
//	node12->name = "+";
//	node12->nodeType = OP_NODE;
//	node12->inputs.push_back(node10);
//	node12->inputs.push_back(node11);
//
//	DAGNode* node13 = new DAGNode();
//	node13->name = "c";
//	node13->nodeType = VALUE_NODE;
//
//	DAGNode* node14 = new DAGNode();
//	node14->name = "=";
//	node14->nodeType = STORE;
//	node14->inputs.push_back(node13);
//	node14->inputs.push_back(node12);
//
//	DAGNode* node15 = new DAGNode();
//	node15->name = "c";
//	node15->nodeType = VALUE_NODE;
//
//	DAGNode* node16 = new DAGNode();
//	node16->name = "1";
//	node16->nodeType = CONSTANT;
//
//	DAGNode* node17 = new DAGNode();
//	node17->name = "=";
//	node17->nodeType = STORE;
//	node17->inputs.push_back(node15);
//	node17->inputs.push_back(node16);
//
//	DAG *dag1 = new DAG();
//	dag1->nodes.push_back(node3);
//	dag1->nodes.push_back(node1);
//	dag1->nodes.push_back(node2);
//
//
//	DAG* dag2 = new DAG();
//	dag2->nodes.push_back(node6);
//	dag2->nodes.push_back(node4);
//	dag2->nodes.push_back(node5);
//
//	DAG* dag3 = new DAG();
//	dag3->nodes.push_back(node17);
//	dag3->nodes.push_back(node15);
//	dag3->nodes.push_back(node16);
//	dag3->nodes.push_back(node9);
//	dag3->nodes.push_back(node7);
//	dag3->nodes.push_back(node8);
//
//	DAG* dag4 = new DAG();
//	dag4->nodes.push_back(node14);
//	dag4->nodes.push_back(node13);
//	dag4->nodes.push_back(node12);
//	dag4->nodes.push_back(node11);
//	dag4->nodes.push_back(node10);
//
//
//	BasicBlock* block1 = new BasicBlock(1);
//	block1->dag = dag1;
//	block1->id = 1;
//	BasicBlock* block2 = new BasicBlock(2);
//	block2->dag = dag2;
//	block2->id = 2;
//	BasicBlock* block3 = new BasicBlock(3);
//	block3->dag = dag3;
//	block3->id = 3;
//	BasicBlock* block4 = new BasicBlock(4);
//	block4->dag = dag4;
//	block4->id = 4;
//
//	
//	CFG* cfg1=new CFG(block3); 
//	cfg1->basicBlocks.push_back(block1);
//	cfg1->basicBlocks.push_back(block2);
//	cfg1->basicBlocks.push_back(block4);
//	cfg1->addEdge(block3, block1);
//	cfg1->addEdge(block3, block2);
//	cfg1->addEdge(block2, block4);
//	cfg1->addEdge(block1, block4);
//
//
//	SSA *ssa1=new SSA(cfg1);
//	ssa1->SSA_WORK();
//	
//
//
//
//
//	return 0;
//	
//}
