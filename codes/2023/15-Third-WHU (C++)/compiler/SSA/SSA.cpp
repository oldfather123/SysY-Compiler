#include "SSA.h"
#include <algorithm>






//将node节点name并集加入vector
void nameset_oradd(Symbol* s, vector<Symbol*>& vec) {
	if(find(vec.begin(),vec.end(),s)==vec.end())
		vec.push_back(s);
}



//在block类型的vector并集加入加一个新的block
void blocks_orand(vector<BasicBlock*>& blocks, BasicBlock* newblock) {
	if (find(blocks.begin(), blocks.end(), newblock) == blocks.end()) {
		blocks.push_back(newblock);
	}
}



//对runner节点查找在basicBlocks向量的下标
int SSA::find_basicblocks_index(BasicBlock*& runner) {
	for (int n = 0; n < basicBlocks.size(); n++) {
		if (runner == basicBlocks[n])
			return n;
	}
	std::cout << "find df index error";
	return -1;
}



//找到不可达的块
void SSA::find_NO_USE(BasicBlock* ava) {

	//避免环
	if (usetag[find_basicblocks_index(ava)] == 1)
	{
		return;
	}



	//特判：void函数以if-else的空块结尾，加入return节点
	if (ava->dag->nodes.empty() && ava->postBlock.empty()) {
		DAGNode* dagnode = new DAGNode(RETURN_NODE, DAGNodeType::OP_NODE);
		ava->dag->rootNode = dagnode;
		ava->dag->nodes.push_back(dagnode);
	}

	usetag[find_basicblocks_index(ava)] = 1;
	for (BasicBlock* post_iter : ava->postBlock) {
		find_NO_USE(post_iter);
	}
}

void SSA::del_NO_USE() {

	for (int b_index = 0; b_index < basicBlocks.size(); b_index++) {
		if (usetag[b_index] == 0) {
			basicBlocks[b_index]->label = BasicBlock::LabelType::NO_USE;
		}
	}

	for (int b_index = 0; b_index < basicBlocks.size(); b_index++) {
		if (basicBlocks[b_index]->label == BasicBlock::LabelType::NO_USE) {
			BasicBlock* temp_nouse = basicBlocks[b_index];
			//带安全的删除
			for (BasicBlock* pre_iter : temp_nouse->preBlock) {
				vector<BasicBlock*>::iterator it = find(pre_iter->postBlock.begin(), pre_iter->postBlock.end(), temp_nouse);
				if (it != pre_iter->postBlock.end()) {
					pre_iter->postBlock.erase(it);
				}
			}
			for (auto post_iter : basicBlocks[b_index]->postBlock) {
				vector<BasicBlock*>::iterator it = find(post_iter->preBlock.begin(), post_iter->preBlock.end(), temp_nouse);
				if (it != post_iter->preBlock.end()) {
					post_iter->preBlock.erase(it);
				}
			}
			//删除当前位置并保持指针
			basicBlocks.erase(basicBlocks.begin() + b_index);
			b_index = b_index - 1;
		}
	}
}





void SSA::toposorting() {

	//清除原数据
	usetag.clear();
	usetag = vector<int>(basicBlocks.size());
	find_NO_USE(entry);
	del_NO_USE();
	//

	//重新计算依赖关系
	de_while_preedge();




	//排序
	usetag.clear();
	usetag = vector<int>(basicBlocks.size());
	vector<BasicBlock*> topo_result;
	topotravel(cfg->entry, topo_result);

	add_while_preedge();

	basicBlocks.clear();
	basicBlocks = topo_result;

}

void SSA::topotravel(BasicBlock* b_iter,vector<BasicBlock*>& topo_result) {
	//避免环
	if (usetag[find_basicblocks_index(b_iter)] == 1) return;

	//插入结果
	topo_result.push_back(b_iter);
	usetag[find_basicblocks_index(b_iter)] = 1;

	//删除lalse的whilecondition标签
	if (b_iter->label == BasicBlock::WHILE_CONDITION) {
		if (b_iter->postBlock.size() == 1) {
			//false就只有一个入口了
			b_iter->label = BasicBlock::NONE;
		}
	}

	//读后继
	for (BasicBlock* next_b : b_iter->postBlock) {
		//后继的pre全部被遍历则可以进
		int do_next = 1;
		for (BasicBlock* temp_pre : next_b->preBlock) {
			if (usetag[find_basicblocks_index(temp_pre)] == 0) {
				do_next = 0;
				break;
			}
		}
		if (do_next) {
			topotravel(next_b, topo_result);
		}
	}

	//无条件进domtree子节点，解决循环问题
	//for (BasicBlock* next_domtreeson : Domtree_sons[b_iter]) {
	//	topotravel(next_domtreeson, topo_result);
	//}

}


//dom求交,返回在第三个参数
//void SSA::and_dom(vector<BasicBlock*>& dom1, vector<BasicBlock*>& dom2, vector<BasicBlock*>& res) {
//	vector<BasicBlock*> temp;
//	for (auto iter1 : dom1)
//	{
//		for (auto iter2 : dom2)
//		{
//			if (iter1 == iter2) {
//				temp.push_back(iter1);
//				break;
//			}
//		}
//
//	}
//
//	res.clear();
//	for (auto iter : temp) {
//		res.push_back(iter);
//	}
//
//}




//计算一个节点的支配者树后继节点
//void SSA::cal_domtree(BasicBlock* b, vector<BasicBlock*>& s) {
//	for (auto iter : basicBlocks) {
//		if (cfg_idom[find_basicblocks_index(iter)] == b) {
//			s.push_back(iter);
//		}
//	}
//}




/*
//对index节点的dom，加入basicblock节点，取并集
void addDom(int index, BasicBlock* basicblock) {
	for (int n = 0; n < cfg_dom[index].size(); n++) {
		if (cfg_dom[index][n] == basicblock)
			return;
	}
	cfg_dom[index].push_back(basicblock);
}
*/





//对index节点的df，加入basicblock节点，取并集
//void SSA::addDF(int& index, BasicBlock*& basicblock) {
//	for (int n = 0; n < DF[index].size(); n++) {
//		if (DF[index][n] == basicblock)
//			return;
//	}
//	DF[index].push_back(basicblock);
//}





//计算支配集
void SSA::cal_Dom() {
	//初始化dom
	for (BasicBlock* b_iter : basicBlocks) {
		ccfg_dom[b_iter].insert(b_iter);
	}



	//迭代
	
		//entry为空
		//for (int i = 1; i < basicBlocks.size(); i++)
	for (int i = 0; i < basicBlocks.size(); i++)
	{
		vector<BasicBlock*> prenodes = basicBlocks[i]->preBlock;
		set<BasicBlock*> temp;
		if (!prenodes.empty())
		{
			//
			temp = ccfg_dom[prenodes[0]];
			for (BasicBlock* pre_iter : prenodes) {
				set<BasicBlock*> temp_section;
				set_intersection(ccfg_dom[pre_iter].begin(), ccfg_dom[pre_iter].end(), temp.begin(), temp.end(), inserter(temp_section,temp_section.begin()));
				temp = temp_section;
			}
		}

		//加上自己
		temp.insert(basicBlocks[i]);

		ccfg_dom[basicBlocks[i]] = temp;

		//必然一遍收敛


	}
}


//计算直接支配集
void SSA::cal_IDom() {


	//entry为空
	//for (int i = 1; i < basicBlocks.size(); i++) {
	for (int i = 0; i < basicBlocks.size(); i++) {
		bool find_idom = false;
		vector<BasicBlock*> temp_now;
		vector<BasicBlock*> temp_pre;
		temp_pre.push_back(basicBlocks[i]);
		//按高度向上迭代
		while (!find_idom) {
			if (temp_pre.empty()) {
				//std::cout << "idom iteration error" << endl<<endl;
				cfg_idom[i] = NULL;
				break;
			}
			temp_now.assign(temp_pre.begin(), temp_pre.end());
			temp_pre.clear();
			for (auto iter : temp_now) {
				vector<BasicBlock*> temp_prenodes = iter->preBlock;
				temp_pre.insert(temp_pre.end(), temp_prenodes.begin(), temp_prenodes.end());
			}
			for (auto iter : ccfg_dom[basicBlocks[i]]) {
				if (find(temp_pre.begin(), temp_pre.end(), iter) != temp_pre.end()) {
					find_idom = true;
					cfg_idom[i] = iter;
					//支配者树
					Domtree_sons[iter].push_back(basicBlocks[i]);
					break;
				}
			}
		}


	}

}






//计算支配边界
void SSA::cal_DF() {


	//初始化，将头节点加入DF
	//for (int it = 0; it < basicBlocks.size(); it++)
	//{
	//	//cfg->addDF(it, entry);
	//}


	//计算支配边界
	for (int b_index = 0; b_index < basicBlocks.size(); b_index++)
	{
		BasicBlock* tb = basicBlocks[b_index];
		vector<BasicBlock*> preblocks = tb->preBlock;
		if (preblocks.size() > 1) {//multi pre
			for (int pre_index = 0; pre_index < preblocks.size(); pre_index++)
			{
				BasicBlock* runner = preblocks[pre_index];
				while (runner != cfg_idom[b_index]) {
					int df_index = find_basicblocks_index(runner);
					ccfg_DF[runner].insert(tb);
					runner = cfg_idom[df_index];
				}
			}
		}
	}




}





void SSA::detect_var(BasicBlock* b,vector<Symbol*>& varkill,DAGNode* iter) {
		
		//使用的变量
		if (iter->nodeType == LOAD) {
			DAGNode* varnode;
			if (iter->inputs[0]->name == ARRAYSUB_NODE) {
				//varnode=iter->inputs[0]->inputs[0];//[]的第一个子节点：数组名
				detect_var(b,varkill, iter->inputs[0]->inputs[1]);//[]的第二个子节点：偏移计算量
				return;
			}//数组
			else {
				varnode = iter->inputs[0]; //变量



				if (varnode->symbol->currentscope == symboltable->getGlobalScope()) {
					//忽略全局变量
				}
				else if (find(varkill.begin(), varkill.end(), varnode->symbol) == varkill.end()) {
					nameset_oradd(varnode->symbol, globalvar);
				}
			}
		}
		//定义的变量
		else if (iter->nodeType == STORE) {

			//计算右值
			if(iter->name!="store_para")
				detect_var(b, varkill, iter->inputs[1]);


			//左值
			DAGNode* varnode;
			if (iter->inputs[0]->name == ARRAYSUB_NODE) { 
				//varnode = iter->inputs[0]->inputs[0]; //要被store的变量名
				detect_var(b,varkill,iter->inputs[0]->inputs[1]);//偏移计算表达式
			}//数组初始化和变量一样，数组元素赋值不同
			else {
				varnode = iter->inputs[0]; //变量

				if (varnode->symbol->currentscope == symboltable->getGlobalScope()) {
					//忽略全局变量
				}
				else {
					nameset_oradd(varnode->symbol, varkill);
					//如果还没初始化.[]会默认插入新的键值对
					blocks_orand(var_blocks[varnode->symbol], b);
				}
			}
		}
		else if (iter->nodeType == CONSTANT) {

		}
		else if (iter->nodeType == OP_NODE) {
			for (DAGNode* input_iter : iter->inputs) {
				detect_var(b, varkill, input_iter);//[]已经在上一层被特判
			}
		}
	
}









void SSA::place_phi() {


	//寻找全局名字，忽略全局变量
	//for (int block_index = 1; block_index < basicBlocks.size(); block_index++) {
	for (int block_index = 0; block_index < basicBlocks.size(); block_index++) {
		vector<Symbol*> varkill;
		BasicBlock* b = basicBlocks[block_index];
		vector<DAGNode*> nextsen;
		//找到第一条语句节点
		DAGNode* firstnode = b->dag->rootNode;
		nextsen.insert(nextsen.begin(), firstnode);
		while (!firstnode->relyNodes.empty()) {
			firstnode = firstnode->relyNodes[0];
			nextsen.insert(nextsen.begin(), firstnode);
		}
		//拓扑遍历,初始化是按语句顺序记录,单边依赖
		//忽略全局变量
		for (int sen_index = 0; sen_index < nextsen.size(); sen_index++) {
			detect_var(b,varkill, nextsen[sen_index]);
		}
	}



	//重写代码
	for (Symbol* name_iter : globalvar) {
		vector<BasicBlock*> worklist{ var_blocks[name_iter].begin(),var_blocks[name_iter].end() };
		
		for (int list_index = 0; list_index < worklist.size();list_index++) {
			BasicBlock* name_block_iter = worklist[list_index];
			for (BasicBlock* df_block_iter : ccfg_DF[name_block_iter]) {
				//不把变量传到生命周期之外
				if (symboltable->isAfatherB(name_iter->currentscope, df_block_iter->scope)) {
					//还没有变量的名字
					if (find(phirec[df_block_iter].begin(), phirec[df_block_iter].end(), name_iter) == phirec[df_block_iter].end()) {
						//在支配边界基本块df中插入name_iter变量的%PHI

						phirec[df_block_iter].push_back(name_iter);
						blocks_orand(worklist, df_block_iter);
					}
				}
			}
		}


	}


}









//重命名带%的变量
void set_newname_node(DAGNode* node, string freshname) {
	
	//找到新名字和符号表
	Symbol* tempsymbol = node->symbol->currentscope->Lookup(freshname);
	//set
	node->name = freshname;
	node->symbol = tempsymbol;
}



//拷贝复制s新建symbol，并更名为name
Symbol* new_name_symbol(string name, Symbol* s) {
	Symbol* temp = new Symbol(s);
	s->currentscope->AddSymbol(name, temp);
	return temp;
}








void SSA::rename() {

	block_rename(entry);
}




Symbol* SSA::newname(Symbol* name) {


	//迭代符号栈
	string namestr = name->name;
	int i = var_count[namestr];
	//使得函数内部下标从1开始
	if (i == 0) {
		i = 1;
		var_count[namestr] += 1;
	}
	var_count[namestr] += 1;
	var_stack[name].push_back(i);


	//在加到符号表时分配了空间
	string temp_name = name->name + "%" + to_string(i);
	Symbol* newsymbol =new_name_symbol(temp_name, name);


	return newsymbol;
}



Symbol* SSA::new_copyname(Symbol* name) {


	//迭代符号栈
	string namestr = name->name;
	int i = copy_count[namestr];
	//使得函数内部下标从1开始
	if (i == 0) {
		i = 1;
		copy_count[namestr] += 1;
	}
	copy_count[namestr] += 1;
	//var_stack[name].push_back(i);


	//在加到符号表时分配了空间
	string temp_name = name->name + "$" + to_string(i);
	Symbol* newsymbol = new_name_symbol(temp_name, name);


	return newsymbol;
}



//查找dag内是否已存在该symbol的变量节点，有则返回，无则nullptr
DAGNode* find_valnode(vector<DAGNode*>& nodes, Symbol* s) {
	DAGNode* ans = nullptr;
	for (DAGNode* iter : nodes) {
		if (iter->symbol == s)
			ans = iter;
	}
	return ans;
}

DAGNode* find_regnode(vector<DAGNode*>& nodes, long regid) {
	DAGNode* ans = nullptr;
	for (DAGNode* iter : nodes) {
		if (iter->resultId == regid)
			ans = iter;
	}
	return ans;
}





void SSA::var_rename(BasicBlock* b,DAGNode* node) {
	//总是与var_stack栈顶比较
	//忽略全局变量
	//要考虑数组/函数调用的inputs
	if (node->nodeType == LOAD) {
		DAGNode* temp_varnode;
		DAGNode* prenode;
		//数组
		if (node->inputs[0]->name == ARRAYSUB_NODE) {
			temp_varnode = node->inputs[0]->inputs[0]; //记录数组名
			prenode = node->inputs[0];
			var_rename(b,node->inputs[0]->inputs[1]);//先处理右值
			//处理数组名作用域的重命名
			if (temp_varnode->symbol->currentscope == symboltable->getGlobalScope()) {
				//忽略全局变量
			}
			else if (temp_varnode->name.find("%") == std::string::npos) {

				if (var_stack.find(temp_varnode->symbol) == var_stack.end()) {
					//sub之前没有init
					Symbol* tempsymbol = newname(temp_varnode->symbol);
					set_newname_node(temp_varnode, tempsymbol->name);
				}
				else {
					set_newname_node(temp_varnode, temp_varnode->name + "%" + to_string(var_stack[temp_varnode->symbol][0]));
				}
			}

		}
		else {
			temp_varnode = node->inputs[0];
			prenode = node;
			//变量

			if (temp_varnode->symbol->currentscope == symboltable->getGlobalScope()) {
				//忽略全局变量
			}
			else if (temp_varnode->symbol->type==iarray || temp_varnode->symbol->type==farray  || temp_varnode->symbol->type==ipoint  || temp_varnode->symbol->type==fpoint) {
				//处理数组作用域的重命名
				if (temp_varnode->name.find("%") == std::string::npos) {
					if (var_stack.find(temp_varnode->symbol) == var_stack.end()) {
						//sub之前没有init
						Symbol* tempsymbol = newname(temp_varnode->symbol);
						set_newname_node(temp_varnode, tempsymbol->name);
					}
					else {
						set_newname_node(temp_varnode, temp_varnode->name + "%" + to_string(var_stack[temp_varnode->symbol][0]));
					}
				}
			}
			//还没有被重命名过
			else if (temp_varnode->name.find("%") == std::string::npos) {
				set_newname_node(temp_varnode, temp_varnode->name + "%" + to_string(var_stack[temp_varnode->symbol].back()));
			}
			else {
				//已经被重命名过
				string temp_varname = temp_varnode->name;
				string varorigin_name = temp_varname.substr(0, temp_varname.find("%"));
				string varindex_now = temp_varname.substr(temp_varname.find("%") + 1);
				Symbol* varorigin_symbol = temp_varnode->symbol->currentscope->Lookup(varorigin_name);
				//与栈顶比较是不是一致,不一致则需要连接到最新的节点上（不是删除或者修改，是连接到最新的节点）
				if (to_string(var_stack[varorigin_symbol].back()) != varindex_now) {
					Symbol* varnewest_symbol = varorigin_symbol->currentscope->Lookup(varorigin_name + "%" + to_string(var_stack[varorigin_symbol].back()));
					if (varnewest_symbol == nullptr)
						cout << " newest-version usevar rename error!" << endl;
					DAGNode* newest_varnode = find_valnode(b->dag->nodes, varnewest_symbol);
					//修改依赖关系
					//node->inputs.pop_back();
					//node->inputs.push_back(newest_varnode);
					prenode->inputs[0] = newest_varnode;
					newest_varnode->outputs.push_back(prenode);
					//使用变量，如果不是在最初始变量节点上修改成重命名变量，那么就没有指向边
					//if (find(temp_varnode->outputs.begin(), temp_varnode->outputs.end(), node) != temp_varnode->outputs.end()) {
					temp_varnode->outputs.erase(find(temp_varnode->outputs.begin(), temp_varnode->outputs.end(), prenode));
					//}
					//temp_varnode->outputs.erase(find(temp_varnode->outputs.begin(), temp_varnode->outputs.end(), node));
				}
			}
		}

	}
	else if (node->nodeType == STORE) {
		//store节点应该先对右值遍历
		//读取load节点
		//右值
		if (node->name != "store_para") {//形参和全局的初始化
			var_rename(b, node->inputs[1]);
		}
		//左值
		DAGNode* temp_varnode;//可能是提前被其他地方重命名的节点
		DAGNode* prenode;
		if (node->name!="store_para" && node->inputs[1]->name == ARRAY_INIT_NODE) {
			temp_varnode = node->inputs[0];//记录数组名
			prenode = node;
			if (temp_varnode->symbol->currentscope == symboltable->getGlobalScope()) {
				//忽略全局变量
			}
			else if (temp_varnode->name.find("%") == std::string::npos) {
				//未重命名
				Symbol* tempsymbol = newname(temp_varnode->symbol);//新建数组名
				set_newname_node(temp_varnode, tempsymbol->name);
			}
		}
		else if (node->inputs[0]->name == ARRAYSUB_NODE) {
			temp_varnode = node->inputs[0]->inputs[0]; //记录数组名
			prenode = node->inputs[0];
			var_rename(b, node->inputs[0]->inputs[1]);//先处理右值
			if (temp_varnode->symbol->currentscope == symboltable->getGlobalScope()) {
				//忽略全局变量
			}
			else if (temp_varnode->name.find("%") == std::string::npos) {
				if (var_stack.find(temp_varnode->symbol) == var_stack.end()) {
					//sub之前没有init
					Symbol* tempsymbol = newname(temp_varnode->symbol);
					set_newname_node(temp_varnode, tempsymbol->name);
				}
				else {
					set_newname_node(temp_varnode, temp_varnode->name + "%" + to_string(var_stack[temp_varnode->symbol][0]));
				}
			}
		}//数组
		else {
			temp_varnode = node->inputs[0];
			prenode = node;
			//变量


			if (temp_varnode->symbol->currentscope == symboltable->getGlobalScope()) {
				//忽略全局
			}
			else if (temp_varnode->symbol->type == iarray || temp_varnode->symbol->type == farray  || temp_varnode->symbol->type == ipoint || temp_varnode->symbol->type == fpoint) {
				//数组名作用域的重命名
				if (temp_varnode->name.find("%") != std::string::npos) {
					set_newname_node(temp_varnode, temp_varnode->name + "%"+to_string(var_stack[temp_varnode->symbol][0]));
				}
				else {
					Symbol* tempsymbol = newname(temp_varnode->symbol);//新建数组名
					set_newname_node(temp_varnode, tempsymbol->name);
				}
			}
			//else if (temp_varnode->name.find("%") == std::string::npos) {//还未被其他共同使用这个节点的语句重命名
			//仅仅是数组名
			else if (!temp_varnode->symbol->dimensions.empty()) {
				//defvar[b].push_back(temp_varnode->symbol);
				Symbol* tempsymbol = newname(temp_varnode->symbol);//新建了变量
				set_newname_node(temp_varnode, tempsymbol->name);
			}
			else {//已经被重命名，需要拆出名字来进行重命名（没有重命名也可以认为是空的重命名）
				string temp_varname = temp_varnode->name;
				string varorigin_name = temp_varname.substr(0, temp_varname.find("%"));
				Symbol* varorigin_symbol = temp_varnode->symbol->currentscope->Lookup(varorigin_name);
				defvar[b].push_back(varorigin_symbol);
				//新建节点(name,nodetype)
				Symbol* newest_symbol = newname(varorigin_symbol);
				string newest_name = newest_symbol->name;
				DAGNode* newest_varnode = new DAGNode(newest_name, VALUE_NODE);
				//设置symbol和resulttype
				newest_varnode->resultType = newest_symbol->type;
				newest_varnode->symbol = newest_symbol;
				//修改依赖关系
				//node->inputs.pop_back();
				prenode->inputs[0] = newest_varnode;
				newest_varnode->outputs.push_back(prenode);
				//之前重命名的变量需要切断与现store的联系
				//与上一个节点的联系
				temp_varnode->outputs.erase(find(temp_varnode->outputs.begin(), temp_varnode->outputs.end(), prenode));
				//插入nodes
				b->dag->nodes.push_back(newest_varnode);
			}
		}
	}
	else if(node->nodeType==CONSTANT) {
		
	}
	//数组名作函数调用不被load
	else if (node->nodeType == VALUE_NODE) {
		DAGNode* temp_varnode = node;

		if (temp_varnode->symbol->currentscope == symboltable->getGlobalScope()) {
			//忽略全局变量
		}
		else if (temp_varnode->symbol->type == iarray || temp_varnode->symbol->type == farray  || temp_varnode->symbol->type == ipoint || temp_varnode->symbol->type == fpoint) {
			//处理数组作用域的重命名
			if (temp_varnode->name.find("%") == std::string::npos) {
				if (var_stack.find(temp_varnode->symbol) == var_stack.end()) {
					//sub之前没有init
					Symbol* tempsymbol = newname(temp_varnode->symbol);
					set_newname_node(temp_varnode, tempsymbol->name);
				}
				else {
					set_newname_node(temp_varnode, temp_varnode->name + "%" + to_string(var_stack[temp_varnode->symbol][0]));
				}
			}
		}
	}
	else {//OP_NODE
		for (DAGNode* iter : node->inputs) {
			var_rename(b,iter);
		}
	}

}



void SSA::block_rename(BasicBlock* b) {


	//对每个%PHI函数的目标变量重命名
	if (phirec.count(b)) {
		for (Symbol* name_iter : phirec[b]) {
			phi_para[b][name_iter].name = newname(name_iter);
		}
	}



	//当出现空块
	//if (!b->dag->nodes.empty()) {
	//}

		vector<DAGNode*> nextsen;
		//找到第一条语句节点
		DAGNode* firstnode = b->dag->rootNode;
		nextsen.insert(nextsen.begin(), firstnode);
		while (!firstnode->relyNodes.empty()) {
			firstnode = firstnode->relyNodes[0];
			nextsen.insert(nextsen.begin(), firstnode);
		}
		//拓扑遍历,初始化是按语句顺序记录,单边依赖
		//忽略全局变量
		for (int sen_index = 0; sen_index < nextsen.size(); sen_index++) {
			var_rename(b, nextsen[sen_index]);
		}





		//填后继块的%PHI的参数
		//是否在这里做SSA的摧毁？？？？？
		for (BasicBlock* iter : b->postBlock) {
			if (phirec.find(iter) == phirec.end())
				continue;//避免新建phirec的空键值对
			for (Symbol* name_iter : phirec[iter]) {
				//特判循环的回填
				if (var_stack[name_iter].empty()) {
					//iter块的某个%PHI函数没有可填入的参数，可能与while多出口到这里有关
					//cout << "warning: protential bug in while_block rename" << endl<<endl;
					continue;
				}
				else {
					//对后续块%PHI函数存入当前块的最新的变量
					//可能存在局部作用域覆盖的问题，由不同的name_iter区分
					//超出作用域的问题由数据流分析消除无用代码
					Symbol* tempsymbol = name_iter->currentscope->Lookup(name_iter->name + "%" + to_string(var_stack[name_iter].back()));
					if (tempsymbol != nullptr) {
						//不重复插入相同para
						//if (find(phi_para[iter][name_iter].para.begin(), phi_para[iter][name_iter].para.end(), tempsymbol) == phi_para[iter][name_iter].para.end()) {
							phi_para[iter][name_iter].para.push_back(tempsymbol);
						//}



						//并且把当前块存入后续块的前驱块vector中，与para的index对齐
						phi_para[iter][name_iter].branch.push_back(b);
						//后续块的%PHI的新名还没有new，先存为空，但是这个新名字是要被当前块的名字赋值的
						phi_des[b][tempsymbol] = nullptr;//存入分支里的变量名等待被索引
					}
				}

			}
		}

		//支配者树，是idom的逆关系
		vector<BasicBlock*> domtree_seccussor=Domtree_sons[b];
		for (auto iter : domtree_seccussor) {
			block_rename(iter);
		}
	






	//pop出%PHI的rename
	if (phirec.find(b) != phirec.end()) {
		for (Symbol* name_iter : phirec[b]) {
			var_stack[name_iter].pop_back();
			//phi_para[b][name_iter].count--;		
		}
	}


	//pop出def的rename
	if (defvar.find(b) != defvar.end()) {
		for (Symbol* name_iter : defvar[b]) {
			var_stack[name_iter].pop_back();
			//phi_para[b][name_iter].count--;
		}
	}

	
}








//本来应该先插入未重命名的%PHI函数节点，再重命名
//实现：先用phirec当作赋值节点存下des，再用phi_para存下src
void SSA::carryout_phi() {


	
	//块前插入节点,空块已删除
	for (int b_index = 0; b_index < basicBlocks.size(); b_index++) {//-1是最后的空块

		BasicBlock* tempblock = basicBlocks[b_index];
		DAG* tempdag = tempblock->dag;
		vector<DAGNode*>& temp_dagnodes = tempdag->nodes;//用来查找指向同一节点
		//找到第一条语句节点
		DAGNode* firstnode = tempdag->rootNode;
		while (!firstnode->relyNodes.empty()) {
			firstnode = firstnode->relyNodes[0];
		}

		//插入所有%PHI函数节点
		if (phirec.count(tempblock)) {
			vector<Symbol*>& temp_phirec = phirec[tempblock];
			map<Symbol*, phi_info>& temp_phipara = phi_para[tempblock];
			int temp_phinum = temp_phirec.size();
			vector<DAGNode*> lastrely;
			//插入当前%PHI函数节点
			for (int phi_index = 0; phi_index < temp_phinum; phi_index++) {

				Symbol* origin_symbol = temp_phirec[phi_index];
				Symbol* dessymbol = temp_phipara[origin_symbol].name;
				vector<Symbol*>& srcsymbolvec = temp_phipara[origin_symbol].para;
				//就应该一个一个插入，而不是一组
				//vector<DAGNode*> nodes_to_insert;


				//先处理src和load
				vector<DAGNode*> loadnodevec;
				for (int src_index = 0; src_index < srcsymbolvec.size(); src_index++) {
					//symbolnode
					Symbol* tempsrcsymbol = srcsymbolvec[src_index];
					DAGNode* srcnode = find_valnode(temp_dagnodes, tempsrcsymbol);
					if (srcnode == nullptr) {
						srcnode = new DAGNode(tempsrcsymbol->name, VALUE_NODE);
						srcnode->symbol = tempsrcsymbol;
						srcnode->resultType = tempsrcsymbol->type;
						temp_dagnodes.insert(temp_dagnodes.begin(),srcnode);
					}


					//loadnode
					DAGNode* loadnode = new DAGNode(" ", LOAD);
					loadnode->resultType = srcnode->resultType;

					//inputs和outputs
					loadnode->inputs.push_back(srcnode);
					srcnode->outputs.push_back(loadnode);

					//插入load节点
					temp_dagnodes.insert(temp_dagnodes.begin(), loadnode);

					//记录loadnode
					loadnodevec.push_back(loadnode);
				}




				//处理des和store
				//新建节点，初始化name和nodetype
				DAGNode* phinode = new DAGNode("%PHI", OP_NODE);
				DAGNode* storenode = new DAGNode(" ", STORE);
				DAGNode* desnode = find_valnode(temp_dagnodes, dessymbol);
				if (desnode == nullptr) {
					desnode = new DAGNode(dessymbol->name, VALUE_NODE);
					//symbol
					desnode->symbol = dessymbol;
					//resulttype
					desnode->resultType = dessymbol->type;
					//insert
					temp_dagnodes.insert(temp_dagnodes.begin(), desnode);
				}

				//resulttype
				storenode->resultType = desnode->resultType;
				phinode->resultType = loadnodevec[0]->resultType;


				//inputs和outputs
				storenode->inputs.push_back(desnode);
				storenode->inputs.push_back(phinode);
				phinode->inputs.insert(phinode->inputs.end(), loadnodevec.begin(), loadnodevec.end());
				for (DAGNode* load_iter : loadnodevec) {
					load_iter->outputs.push_back(phinode);
				}
				desnode->outputs.push_back(storenode);
				phinode->outputs.push_back(storenode);

				//insert
				temp_dagnodes.insert(temp_dagnodes.begin(),phinode);
				temp_dagnodes.insert(temp_dagnodes.begin(), storenode);

				//relynodes
				storenode->relyNodes = lastrely;//拷贝赋值
				lastrely.clear();
				lastrely.push_back(storenode);
			}
		
		
			//接到dag的rely当中
			firstnode->relyNodes = lastrely;
		
		}
		}
		
	





}







void SSA::preprocess() {
	//获取函数列表,函数符号表（已在函数作用域）
	Scope* globalscope = symboltable->getGlobalScope();
	vector<Symbol*> paravec =globalscope->Symbols[cfgname]->ParamSymbols;
	Scope* funcscope = symboltable->getFuncScope(cfgname);
	//头部插入新块，并加入跳转节点作为root
	parablock = new BasicBlock(basicBlocks.size(),nullptr);
	basicBlocks.push_back(parablock);
	cfg->addEdge(parablock, entry);
	DAGNode* branchnode = new DAGNode(BRANCH_NODE,OP_NODE);
	branchnode->targets.push_back(entry->id);
	parablock->dag->rootNode = branchnode;
	parablock->dag->nodes.push_back(branchnode);
	entry = parablock;
	cfg->entry = parablock;


	DAGNode* firstnode = branchnode;
	//插入函数参数
	for (int para_index = 0; para_index < paravec.size(); para_index++) {
		Symbol* temp_parasymbol = paravec[para_index];
		//name\type
		DAGNode* storenode = new DAGNode("store_para", STORE);
		DAGNode* varnode = new DAGNode(temp_parasymbol->name, VALUE_NODE);
		//symbol
		varnode->symbol = temp_parasymbol;
		varnode->resultType = temp_parasymbol->type;
		//const
		//inputs/outputs
		storenode->inputs.push_back(varnode);
		varnode->outputs.push_back(storenode);
		//rely
		firstnode->relyNodes.push_back(storenode);
		firstnode = storenode;
		//nodes
		parablock->dag->nodes.push_back(storenode);
		parablock->dag->nodes.push_back(varnode);
	}




		/*
				//拷贝全局变量表，并替换cfg中的symbol
		Scope* cfg_global = new Scope();
		cfg_global->setParent(funcscope);
		vector<Symbol*> temp_globalvar;
		for (pair<string, Symbol*> iter : symboltable->getGlobalScope()->Symbols) {
			if (iter.second->symbolType == SymbolType::VAL) {
				Symbol* tempsymbol = new Symbol(iter.second);
				tempsymbol->currentscope = cfg_global;
				temp_globalvar.push_back(tempsymbol);
				cfg_global->AddSymbol(iter.first, tempsymbol);
			}
		}
		//替换cfg中的全局变量为临时全局
		for (int b_index = 0; b_index < basicBlocks.size(); b_index++) {
			vector<DAGNode*>& tempnodes = basicBlocks[b_index]->dag->nodes;
			for (int node_index = 0; node_index < tempnodes.size(); node_index++) {
				if (tempnodes[node_index]->nodeType == VALUE_NODE) {
					string varname = tempnodes[node_index]->symbol->name;
					if (globalscope->Lookup(varname) != nullptr) {
						tempnodes[node_index]->symbol = cfg_global->Lookup(varname);
					}
				}
			}
		}
		//插入临时全局节点
		for (int para_index = 0; para_index < temp_globalvar.size(); para_index++) {
			Symbol* temp_parasymbol = temp_globalvar[para_index];
			//name\type
			DAGNode* storenode = new DAGNode("store_para", STORE);
			DAGNode* varnode = new DAGNode(temp_parasymbol->name, VALUE_NODE);
			//symbol
			varnode->symbol = temp_parasymbol;
			//const
			//inputs/outputs
			storenode->inputs.push_back(varnode);
			varnode->outputs.push_back(storenode);
			//rely
			firstnode->relyNodes.push_back(storenode);
			firstnode = storenode;
			//nodes
			parablock->dag->nodes.push_back(storenode);
			parablock->dag->nodes.push_back(varnode);
		}

		//最后一块要存回

		*/


}





void SSA::postprocess() {


	//修改参数列表名字
	vector<Symbol*> oldpara = symboltable->getGlobalScope()->Symbols[cfgname]->ParamSymbols;
	vector<Symbol*> newpara;
	for (int para_index = 0; para_index < oldpara.size(); para_index++) {
		newpara.push_back(oldpara[para_index]->currentscope->Lookup(oldpara[para_index]->name + "%1"));
	}
	symboltable->getGlobalScope()->Symbols[cfgname]->ParamSymbols.clear();
	symboltable->getGlobalScope()->Symbols[cfgname]->ParamSymbols = newpara;

	//从控制流图中移除store_para块，不是删除
	basicBlocks.erase(find(basicBlocks.begin(), basicBlocks.end(), entry));
	entry = parablock->postBlock[0];
	parablock->postBlock.pop_back();
	entry->preBlock.pop_back();
	//edge删除
	for (int edge_index = 0; edge_index < cfg->edges.size();edge_index++) {
		Edge* iter = cfg->edges[edge_index];
		if (iter->source == parablock) {
			cfg->edges.erase(cfg->edges.begin()+edge_index);
			break;
		}
	}



	


}




void SSA::show(){
	cout << "func:" << cfgname << endl;
	for (int block_index = 0; block_index < basicBlocks.size(); block_index++) {
		BasicBlock* iter = basicBlocks[block_index];
		cout << "/*Block" << iter->id<<"*/" << endl;

		//显示%PHI函数
		//if (phi_para.find(iter) != phi_para.end()) {
		//	cout << "/*" << endl;
		//	for (auto %PHI : phi_para[iter]) {
		//		cout << %PHI.second.name << " = %PHI(" << %PHI.second.para[0];
		//		for (int para_index = 1; para_index < %PHI.second.para.size(); para_index++) {
		//			cout << "," << %PHI.second.para[para_index];
		//		}
		//		cout << ")" << endl;
		//	}
		//	cout << "*/" << endl;
		//}


		//显示节点
		for (auto node_iter : iter->dag->nodes) {
			switch (node_iter->nodeType)
			{
			case VALUE_NODE:
				cout << node_iter->name ;
				/*
				if (node_iter->name.find("%") != std::string::npos) {
					cout <<"#"<< node_iter;
				}
				*/
				cout<< "  ";
				break;
			case STORE:
				cout << "=  ";
				break;
			case CONSTANT:
				if (node_iter->resultType != f32)
					cout << node_iter->value.iValue << "  ";
				else
					cout << node_iter->value.fValue << "  ";
				break;
			case OP_NODE:
				cout << "OPNODE" << node_iter->name << "  ";
				break;
			default:
				break;
			}
		}

		//显示SSA重建
		//for (auto des_iter : phi_des[iter]) {
		//	cout << des_iter.first << "  " << des_iter.second << "  =" << endl;
		//}


		cout << endl << endl;
	}
	cout << endl << endl << endl;
}






//phi_des记录每个块被重建的变量的赋值语句
void SSA::destroy() {



	//提前带冗余记录phi_des，%PHI参数回填到分支块
	for (BasicBlock* iter : basicBlocks) {//每个块
		if (phirec.find(iter) == phirec.end())	continue;
		for (int phi_index = 0; phi_index < phirec[iter].size(); phi_index++) {//每个变量
			//iter块的第phi_index个变量%PHI函数的para_phi_info
			phi_info temp_para = phi_para[iter][phirec[iter][phi_index]];
			for (int para_index = 0; para_index < phi_para[iter][phirec[iter][phi_index]].para.size(); para_index++) {//每个参数
				//phi_des[    phi_para[iter][phirec[iter][phi_index]].branch[para_index]][phi_para[iter][phirec[iter][phi_index]].para[para_index]] = phi_para[iter][phirec[iter][phi_index]].name;
				//本次%PHI函数第para_index要重建回的块
				BasicBlock* temp_b = temp_para.branch[para_index];
				//本次%PHI函数的目标变量
				Symbol* symbol_here = temp_para.name;
				//要重建回的块里对应被重命名的变量
				Symbol* symbol_there = temp_para.para[para_index];
				//将要被赋值的名字，根据源名字回填到块中
				phi_des[temp_b][symbol_there] = symbol_here;
			}
		}
	}


	//插入节点
	//要一个一个马上插入节点，以防句内相同节点指向
	for (int index = 0; index < basicBlocks.size(); index++) {
		//运行修改dag
		BasicBlock* tempblock = basicBlocks[index];
		DAG* tempdag = tempblock->dag;
		DAGNode* branchroot = tempdag->rootNode;
		vector<DAGNode*> lastrely = branchroot->relyNodes;
		DAGNode* lasystore = nullptr;
		for (pair<Symbol*, Symbol*> tempdes : phi_des[tempblock]) {
			Symbol* dessymbol = tempdes.second;
			Symbol* srcsymbol = tempdes.first;
			//新建节点，初始化name和nodetype
			DAGNode* storenode = new DAGNode(" ", STORE);
			DAGNode* loadnode = new DAGNode(" ", LOAD);
			DAGNode* desnode = new DAGNode(dessymbol->name, VALUE_NODE);
			DAGNode* srcnode = new DAGNode(srcsymbol->name, VALUE_NODE);



			//symbol
			desnode->symbol = dessymbol;
			srcnode->symbol = srcsymbol;

			//resulttype
			desnode->resultType = dessymbol->type;
			srcnode->resultType = srcsymbol->type;
			storenode->resultType = desnode->resultType;
			loadnode->resultType = srcnode->resultType;


			//inputs和outputs
			storenode->inputs.push_back(desnode);
			storenode->inputs.push_back(loadnode);
			loadnode->inputs.push_back(srcnode);
			desnode->outputs.push_back(storenode);
			loadnode->outputs.push_back(storenode);
			srcnode->outputs.push_back(loadnode);


			//relynodes
			storenode->relyNodes = lastrely;//拷贝赋值
			lastrely.clear();
			lastrely.push_back(storenode);


			//插入dag的nodes
			tempdag->nodes.insert(tempdag->nodes.end() - 1, { srcnode,loadnode,desnode,storenode });
		}
		branchroot->relyNodes = lastrely;
	}
}






void SSA::destroy2() {

	//提前带冗余记录phi_des，%PHI参数回填到分支块
	//for (BasicBlock* iter : basicBlocks) {//每个块
	//	if (phirec.find(iter) == phirec.end())	continue;
	//	for (int phi_index = 0; phi_index < phirec[iter].size(); phi_index++) {//每个变量
	//		//iter块的第phi_index个变量%PHI函数的para_phi_info
	//		phi_info temp_para = phi_para[iter][phirec[iter][phi_index]];
	//		for (int para_index = 0; para_index < phi_para[iter][phirec[iter][phi_index]].para.size(); para_index++) {//每个参数
	//			//phi_des[    phi_para[iter][phirec[iter][phi_index]].branch[para_index]][phi_para[iter][phirec[iter][phi_index]].para[para_index]] = phi_para[iter][phirec[iter][phi_index]].name;
	//			//本次%PHI函数第para_index要重建回的块
	//			BasicBlock* temp_b = temp_para.branch[para_index];
	//			//本次%PHI函数的目标变量
	//			Symbol* symbol_here = temp_para.name;
	//			//要重建回的块里对应被重命名的变量
	//			Symbol* symbol_there = temp_para.para[para_index];
	//			//将要被赋值的名字，根据源名字回填到块中
	//			phi_des[temp_b][symbol_there] = symbol_here;
	//		}
	//	}
	//}


	//副本隔离
	insertcopies();





}






void SSA::insertcopies() {


	//引入副本
	for (int b_index = 0; b_index < basicBlocks.size(); b_index++) {
		BasicBlock* b = basicBlocks[b_index];
		DAG* tempdag = b->dag;


		//找到本块的%PHI函数节点
		vector<DAGNode*> restphi;
		vector<DAGNode*> rest_phides;//store的目标只可能被删，不可能被替换
		//vector<DAGNode*> rest_phiright;//右值节点，可能是虚拟寄存器，可能是load
		//找到第一条语句节点
		DAGNode* firstnode = tempdag->rootNode;
		if (firstnode->inputs.size() > 1) {
			if (firstnode->inputs[1]->name == "%PHI") {
				//%PHI节点
				restphi.push_back(firstnode->inputs[1]);
				//%PHI函数store左值，一定是变量名
				rest_phides.push_back(firstnode->inputs[0]);
			}
		}
		while (!firstnode->relyNodes.empty()) {
			firstnode = firstnode->relyNodes[0];
			if (firstnode->inputs.size() > 1) {
				if (firstnode->inputs[1]->name == "%PHI") {
					//%PHI节点
					restphi.push_back(firstnode->inputs[1]);
					//%PHI函数左值，一定是变量名
					rest_phides.push_back(firstnode->inputs[0]);
				}
			}

		}



		for (int phi_des_index = 0; phi_des_index < restphi.size(); phi_des_index++) {
			//%PHI的des
			//可能被替换成虚拟寄存器了，可以从phi_oara里面找
			DAGNode* phi_desnode = rest_phides[phi_des_index];
			Symbol* phi_des_iter = phi_desnode->symbol;
			DAGNode* tempphi = restphi[phi_des_index];
			//新建：move(new_phi_desnode   phi_desnode)，理论上应该是本来没有的
			//a0=a0'
			//load a0'形式
			DAGNode* new_phi_desnode = new DAGNode(" ", OP_NODE);
			new_phi_desnode->resultId = cfg->getCount();
			new_phi_desnode->resultType = phi_desnode->resultType;
			if (new_phi_desnode->resultType != f32) new_phi_desnode->inst = MV;
			else if (new_phi_desnode->resultType == f32) new_phi_desnode->inst = FMVS;

			DAGNode* new_phi_desvarnode = new DAGNode(phi_desnode, new_name_symbol(phi_desnode->name + "$", phi_desnode->symbol));
			new_phi_desvarnode->outputs.push_back(new_phi_desnode);
			new_phi_desnode->inputs.push_back(new_phi_desvarnode);

			//没维护：name<-->regID的数据结构
			DAGNode* phi_movenode = new DAGNode(" ", OP_NODE);
			if (phi_des_iter->type != f32) phi_movenode->inst = MV;
			else if (phi_des_iter->type == f32) phi_movenode->inst = FMVS;
			phi_movenode->inputs.push_back(phi_desnode);
			phi_movenode->inputs.push_back(new_phi_desnode);
			phi_desnode->outputs.pop_back();//去掉原依赖
			phi_desnode->outputs.push_back(phi_movenode);
			new_phi_desnode->outputs.push_back(phi_movenode);
			tempdag->nodes.push_back(phi_movenode);
			tempdag->nodes.push_back(new_phi_desnode);
			
			//插入dag最前面语句rely
			firstnode->relyNodes.push_back(phi_movenode);
			firstnode = phi_movenode;





			//%PHI函数参数个数不会减少，但可能被替换
			//检查并行序列

			string origin_name = phi_des_iter->name.substr(0, phi_des_iter->name.find("%"));
			Symbol* origin_symbol = phi_des_iter->currentscope->Lookup(origin_name);
			vector<BasicBlock*> tempbranch = phi_para[b][origin_symbol].branch;
			for (int para_index = 0; para_index < tempbranch.size(); para_index++) {
				//对参数拷贝副本并回填
				BasicBlock* backblock = tempbranch[para_index];
				//%PHI的src，可能是load也可能是虚拟寄存器节点
				DAGNode* phi_srcnode = tempphi->inputs[para_index];
				//在backbrach块中是否有相应虚拟寄存器节点






				//原始src节点
				//Symbol* origin_phi_src = phi_para[b][origin_symbol].para[para_index];



				//在backblock块插入副本，并重建
				if (phi_srcnode->nodeType == REG_NODE) {
					//副本拷贝，一定是寄存器
					//新建:插入副本的左值虚拟寄存器节点
					//a1'=a1
					//a1'是变量名，不是寄存器编号1。a1是寄存器节点%1
					//a1'被命名为$1%1
					DAGNode* copy_des;
					Symbol* origin_regsymbol = phi_srcnode->symbol->currentscope->Lookup(to_string(phi_srcnode->resultId));
					if (origin_regsymbol == nullptr) {
						 copy_des = new DAGNode(phi_srcnode, new_copyname(new_name_symbol(to_string(phi_srcnode->resultId), phi_srcnode->symbol)));
					}
					else {
						copy_des = new DAGNode(phi_srcnode, new_copyname(origin_regsymbol));
					}
				
					copy_des->nodeType = VALUE_NODE;
					copy_des->resultId = -1;

					DAGNode* movenode = new DAGNode(" ", OP_NODE);
					if (copy_des->resultType != f32) movenode->inst = MV;
					else if (copy_des->resultType == f32) movenode->inst = FMVS;
					//跨块的节点重新新建,寄存器节点
					DAGNode* new_phi_srcnode = new DAGNode( phi_srcnode,phi_srcnode->symbol);
				
					//inputs和outpus
					movenode->inputs.push_back(copy_des);
					movenode->inputs.push_back(new_phi_srcnode);
					copy_des->outputs.push_back(movenode);
					new_phi_srcnode->outputs.push_back(movenode);
					//插入nodes
					backblock->dag->nodes.push_back(copy_des);
					backblock->dag->nodes.push_back(movenode);
					backblock->dag->nodes.push_back(new_phi_srcnode);
					//插入语句依赖
					if (!backblock->dag->rootNode->relyNodes.empty()) {
						movenode->relyNodes.push_back(backblock->dag->rootNode->relyNodes[0]);
						backblock->dag->rootNode->relyNodes[0] = movenode;
					}
					else {
						backblock->dag->rootNode->relyNodes.push_back(movenode);
					}




					//跨块重建
					//a0'=a1'
					//a0'是变量名，a1'是load
					DAGNode* out_movenode = new DAGNode(" ", OP_NODE);
					if (new_phi_srcnode->resultType != f32) out_movenode->inst = MV;
					else if (new_phi_srcnode->resultType == f32) out_movenode->inst = FMVS;
					//加入load节点
					DAGNode* copy_load = new DAGNode(" ", OP_NODE);
					copy_load->resultId = cfg->getCount();
					copy_load->resultType = copy_des->resultType;
					if (copy_load->resultType != f32) copy_load->inst = MV;
					else if (copy_load->resultType == f32) copy_load->inst = FMVS;

					copy_load->inputs.push_back(copy_des);
					copy_des->outputs.push_back(copy_load);
					backblock->dag->nodes.push_back(copy_load);
					//跨块的节点重新新建
					DAGNode* newblock_phi_desnode = new DAGNode(new_phi_desvarnode->name, new_phi_desvarnode->nodeType);
					newblock_phi_desnode->resultId = new_phi_desvarnode->resultId;
					newblock_phi_desnode->resultType = new_phi_desvarnode->resultType;
					out_movenode->inputs.push_back(newblock_phi_desnode);
					out_movenode->inputs.push_back(copy_load);
					copy_load->outputs.push_back(out_movenode);
					newblock_phi_desnode->outputs.push_back(out_movenode);
					//插入nodes
					//backblock->dag->nodes.push_back(copy_des);
					backblock->dag->nodes.push_back(out_movenode);
					backblock->dag->nodes.push_back(newblock_phi_desnode);
					//插入语句依赖
					if (!backblock->dag->rootNode->relyNodes.empty()) {
						out_movenode->relyNodes.push_back(backblock->dag->rootNode->relyNodes[0]);
						backblock->dag->rootNode->relyNodes[0] = out_movenode;
					}
					else {
						backblock->dag->rootNode->relyNodes.push_back(out_movenode);
					}


				}


				else if (phi_srcnode->nodeType == LOAD) {

					DAGNode* new_phi_srcnode;
					DAGNode* copy_des;


					//副本拷贝，一定是寄存器
					//新建:插入副本的左值虚拟寄存器节点
					//a1'=a1
					//a1'是变量名，不是寄存器编号1。a1是load a1
					DAGNode* phi_src_varnode = phi_srcnode->inputs[0];
					//a1'被命名为a%1$1
					copy_des = new DAGNode(phi_src_varnode, new_copyname(phi_src_varnode->symbol));
					copy_des->nodeType = VALUE_NODE;
					copy_des->resultId = -1;

					DAGNode* movenode = new DAGNode(" ", OP_NODE);
					if (copy_des->resultType != f32) movenode->inst = MV;
					else if (copy_des->resultType == f32) movenode->inst = FMVS;
					//跨块的节点重新新建,load a1
					new_phi_srcnode = new DAGNode(phi_srcnode, phi_srcnode->symbol);//拷贝LOAD节点
					//LOAD改成move
					new_phi_srcnode->nodeType = OP_NODE;
					if (new_phi_srcnode->resultType != f32) new_phi_srcnode->inst = MV;
					else if (new_phi_srcnode->resultType == f32) new_phi_srcnode->inst = FMVS;
					//
					DAGNode* new_phi_srcvarnode = new DAGNode(phi_src_varnode, phi_src_varnode->symbol);
					backblock->dag->nodes.push_back(new_phi_srcvarnode);
					new_phi_srcnode->inputs.push_back(new_phi_srcvarnode);
					new_phi_srcvarnode->outputs.push_back(new_phi_srcnode);


					//inputs和outpus
					movenode->inputs.push_back(copy_des);
					movenode->inputs.push_back(new_phi_srcnode);
					copy_des->outputs.push_back(movenode);
					new_phi_srcnode->outputs.push_back(movenode);
					//插入nodes
					backblock->dag->nodes.push_back(copy_des);
					backblock->dag->nodes.push_back(movenode);
					backblock->dag->nodes.push_back(new_phi_srcnode);
					//插入语句依赖
					if (!backblock->dag->rootNode->relyNodes.empty()) {
						movenode->relyNodes.push_back(backblock->dag->rootNode->relyNodes[0]);
						backblock->dag->rootNode->relyNodes[0] = movenode;
					}
					else {
						backblock->dag->rootNode->relyNodes.push_back(movenode);
					}




					//跨块重建
					//a0'=a1'
					//a0'是变量名，a1'是load
					DAGNode* out_movenode = new DAGNode(" ", OP_NODE);
					if (new_phi_srcnode->resultType != f32) out_movenode->inst = MV;
					else if (new_phi_srcnode->resultType == f32) out_movenode->inst = FMVS;
					//加入load节点
					DAGNode* copy_load = new DAGNode(" ", OP_NODE);
					copy_load->resultId = cfg->getCount();
					copy_load->resultType = copy_des->resultType;
					if (copy_load->resultType != f32) copy_load->inst = MV;
					else if (copy_load->resultType == f32) copy_load->inst = FMVS;

					copy_load->inputs.push_back(copy_des);
					copy_des->outputs.push_back(copy_load);
					backblock->dag->nodes.push_back(copy_load);
					//跨块的节点重新新建
					DAGNode* newblock_phi_desnode = new DAGNode(new_phi_desvarnode->name, new_phi_desvarnode->nodeType);
					newblock_phi_desnode->resultId = new_phi_desvarnode->resultId;
					newblock_phi_desnode->resultType = new_phi_desvarnode->resultType;
					out_movenode->inputs.push_back(newblock_phi_desnode);
					out_movenode->inputs.push_back(copy_load);
					copy_load->outputs.push_back(out_movenode);
					newblock_phi_desnode->outputs.push_back(out_movenode);
					//插入nodes
					//backblock->dag->nodes.push_back(copy_des);
					backblock->dag->nodes.push_back(out_movenode);
					backblock->dag->nodes.push_back(newblock_phi_desnode);
					//插入语句依赖
					if (!backblock->dag->rootNode->relyNodes.empty()) {
						out_movenode->relyNodes.push_back(backblock->dag->rootNode->relyNodes[0]);
						backblock->dag->rootNode->relyNodes[0] = out_movenode;
					}
					else {
						backblock->dag->rootNode->relyNodes.push_back(out_movenode);
					}



				}

				//被替换成常量
				else if (phi_srcnode->nodeType == OP_NODE) {

					DAGNode* new_phi_srcnode;
					DAGNode* copy_des;
					//load constant


						//就是拿li的节点来重建
					copy_des = phi_srcnode;
					new_phi_srcnode = copy_des;
					backblock->dag->nodes.push_back(copy_des);
					//插入语句依赖
					/*if (!backblock->dag->rootNode->relyNodes.empty()) {
						copy_des->relyNodes.push_back(backblock->dag->rootNode->relyNodes[0]);
						backblock->dag->rootNode->relyNodes[0] = copy_des;
					}
					else {
						backblock->dag->rootNode->relyNodes.push_back(copy_des);
					}*/




					//跨块重建
					//a0'=a1'
					//a0'是变量名，a1'是load
					DAGNode* out_movenode = new DAGNode(" ", OP_NODE);
					if (new_phi_srcnode->resultType != f32) out_movenode->inst = MV;
					else if (new_phi_srcnode->resultType == f32) out_movenode->inst = FMVS;
					//加入load节点
					DAGNode* copy_load = copy_des;
					//跨块的节点重新新建
					DAGNode* newblock_phi_desnode = new DAGNode(new_phi_desvarnode->name, new_phi_desvarnode->nodeType);
					newblock_phi_desnode->resultId = new_phi_desvarnode->resultId;
					newblock_phi_desnode->resultType = new_phi_desvarnode->resultType;
					out_movenode->inputs.push_back(newblock_phi_desnode);
					out_movenode->inputs.push_back(copy_load);
					copy_load->outputs.push_back(out_movenode);
					newblock_phi_desnode->outputs.push_back(out_movenode);
					//插入nodes
					//backblock->dag->nodes.push_back(copy_des);
					backblock->dag->nodes.push_back(out_movenode);
					backblock->dag->nodes.push_back(newblock_phi_desnode);
					//插入语句依赖
					if (!backblock->dag->rootNode->relyNodes.empty()) {
						out_movenode->relyNodes.push_back(backblock->dag->rootNode->relyNodes[0]);
						backblock->dag->rootNode->relyNodes[0] = out_movenode;
					}
					else {
						backblock->dag->rootNode->relyNodes.push_back(out_movenode);
					}



				}


			}



		}



		//在语句中删除%PHI节点
		DAGNode* nowphinode = tempdag->rootNode;
		DAGNode* lastphinode;
		//root节点应该是ret或branch
		lastphinode = nowphinode;
		while (!lastphinode->relyNodes.empty()) {
			nowphinode = lastphinode->relyNodes[0];
			if (nowphinode->inputs.size() > 1) {
				if (nowphinode->inputs[1]->name == "%PHI") {
					if (nowphinode->relyNodes.empty()) {
						lastphinode->relyNodes.pop_back();
					}
					else {
						lastphinode->relyNodes[0] = nowphinode->relyNodes[0];
					}

				}
				else {
					lastphinode = nowphinode;
				}
			}
			else
			{
				lastphinode = nowphinode;
			}
		}




	}


}


vector<string> SSA::phivar_rec(BasicBlock* b) {
	vector<string> res;
	for (auto it : phirec[b]) {
		res.push_back(phi_para[b][it].name->name);
		for (auto para_it : phi_para[b][it].para) {
			res.push_back(para_it->name);
		}
	}

	return res;
}
