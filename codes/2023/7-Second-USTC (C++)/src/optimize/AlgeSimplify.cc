#include "AlgeSimplify.hh"

void AlgeSimplify::execute()
{
    //先来一轮公共子表达式删除,提高运行速度
    auto csee = LocalComSubExprEli(m_);
    csee.execute();

    //构建支配树
    auto dom_tree = DominateTree(m_);
    dom_tree.execute();

    for(auto func : module_->get_functions())
    {
        if(func->get_basic_blocks().empty())
            continue;
        if (func->is_declaration())
            continue;

        dom_tree.get_reverse_post_order(func);

        // 构建代数表达式树
        for(auto bb : dom_tree.reverse_post_order)
        {
            auto instructions = bb->get_instructions();
            for(auto inst_iter = instructions.begin(); inst_iter != instructions.end(); inst_iter++)
            {
                auto inst = *inst_iter;
                if(Utils::is_integer_alge_inst(inst))
                {
                    build_expr_tree(inst, bb, inst_iter);
                }
            }
        }

        // 重结合
        for(auto root : rootSet)
        {
            reassociation(root);
        }

       
        // std::cout<<"===== before =====: "<<std::endl;
        // for(auto bb : func->get_basic_blocks())
        //     for(auto insts : bb->get_instructions())
        //     {
        //         std::cout << insts->print() << std::endl;
        //     }
        
        replace_instruction(func);

        
        // std::cout<<"===== after =====: "<<std::endl;
        // for(auto bb : func->get_basic_blocks())
        //     for(auto insts : bb->get_instructions())
        //     {
        //         std::cout << insts->print() << std::endl;
        //     }
        
        rootSet.clear();
        value2Node.clear();
    }
    
    auto csee2 = LocalComSubExprEli(m_);
    auto dce = DeadCodeEliWithBr(m_);
    csee2.execute();
    dce.execute();
}

AlgeSimplify::ExprNode* AlgeSimplify::get_node(Value* val)
{
    auto node = value2Node.find(val);
    if(node == value2Node.end())
    {
        auto newNode = new ExprNode(val, nullptr, nullptr, ExprNode::AlgeOp::LEAF, module_);
        value2Node[val] = newNode;
        return newNode;
    }
    else
    {
        return (*node).second;
    }
}

void AlgeSimplify::build_expr_tree(Instruction* inst, BasicBlock* bb, InstIter& inst_iter)
{
    auto nodeIter = value2Node.find(inst);
    if(nodeIter == value2Node.end())
    {
        auto left = get_node(inst->get_operand(0));
        auto right = get_node(inst->get_operand(1));

        auto newNode = new ExprNode({inst, left, right, module_});
        value2Node[inst] = newNode;
        rootSet.insert(newNode);
        rootSet.erase(left);
        rootSet.erase(right);
        newNode->parent = bb;
        newNode->inst_iter = inst_iter;
    }
    else
    {
        auto node = (*nodeIter).second;
        auto left = get_node(inst->get_operand(0));
        auto right = get_node(inst->get_operand(1));
        node->lChild = left;
        node->rChild = right;
        node->parent = bb;
        node->inst_iter = inst_iter;
    }
}

void AlgeSimplify::reassociation(ExprNode* node)
{
    if(node->dealed == true)
        return;

    if(node->op == ExprNode::AlgeOp::LEAF)
    {
        if (!node->isConst)
            node->add_term(node->val, 1, true);
        return;
    }
        
    reassociation(node->lChild);
    reassociation(node->rChild);
    switch(node->op)
    {
        case ExprNode::AlgeOp::ADD:
            add_node(node);
            break;
        case ExprNode::AlgeOp::SUB:
            sub_node(node);
            break;
        case ExprNode::AlgeOp::MUL:
            mul_node(node);
            break;
        case ExprNode::AlgeOp::DIV:
            div_node(node);
            break;
    }

    node->dealed = true;
}


void AlgeSimplify::add_node(ExprNode* node)
{

    if(node->type == ExprNode::ExprType::INT)
    {
        node->needReplace = true;
        node->intake_node(node->lChild, 1, true);
        node->intake_node(node->rChild, 1, true);
        noUseNodes.insert(node->lChild);
        noUseNodes.insert(node->rChild);
    }
    if (node->termList.empty()) 
    {
        node->isConst = true;
    }
}


void AlgeSimplify::sub_node(ExprNode* node)
{
    if(node->type == ExprNode::ExprType::INT)
    {
        node->needReplace = true;
        node->intake_node(node->lChild, 1, true);
        node->intake_node(node->rChild, -1, true);
        noUseNodes.insert(node->lChild);
        noUseNodes.insert(node->rChild);
    }
    if (node->termList.empty()) 
    {
        node->isConst = true;
    }
}


void AlgeSimplify::mul_node(ExprNode* node)
{
    if(node->type == ExprNode::ExprType::INT)
    {
        //若两个子节点都是常量，则置当前节点为常量
        if(node->lChild->isConst && node->rChild->isConst)
        {
            node->constVal = node->lChild->get_const_int_val() * node->rChild->get_const_int_val();
            node->isConst = true;
        }
        else if (node->lChild->isConst)
        {//若左子节点为常量
            node->intake_node(node->rChild, node->lChild->get_const_int_val(), true);
            noUseNodes.insert(node->rChild);
            node->isConst = false;
            node->needReplace = true;
        }
        else if (node->rChild->isConst)
        {//若右子节点为常量
            // std::cout << "node->rChild->get_const_int_val() :" << node->rChild->get_const_int_val() << std::endl;
            node->intake_node(node->lChild, node->rChild->get_const_int_val(), true);
            noUseNodes.insert(node->lChild);
            node->isConst = false;
            node->needReplace = true;
        }
        else
        {
            node->add_term(node->val, 1, true);
        }
    }
}


void AlgeSimplify::div_node(ExprNode* node)
{
    if (node->lChild->isConst && node->rChild->isConst) 
    {
        node->isConst = true;
        if (node->type == ExprNode::ExprType::INT) 
        {
            node->constVal = node->lChild->get_const_int_val() / node->rChild->get_const_int_val();
            node->newVal = ConstantInt::get(std::get<int>(node->constVal), m_);
        } else 
        {
            node->constVal = node->lChild->get_const_float_val() / node->rChild->get_const_float_val();
            node->newVal = ConstantFP::get(std::get<float>(node->constVal), m_);
        }
        return;
    }
    else if (node->rChild->isConst)
    {//若右子节点为常量
        std::cout<<"++++++++++++++++++除法++++++++++++++++++"<<std::endl;
        std::cout<<"常量: "<<node->rChild->get_const_int_val()<<std::endl;
        int dividend = node->rChild->get_const_int_val();
        //若为叶节点
        if(node->lChild->op == ExprNode::AlgeOp::LEAF)
        {
            node->add_term(node->lChild->val, dividend, false);
            // node->needReplace = true;
            return;
        }
        std::cout<<"size: "<<node->lChild->termList.size()<<std::endl;
        bool test = true;
        for(auto termPair : node->lChild->termList)
        {
            int num = termPair.second;
            if(!(num % dividend == 0))
            {//被除数中有一项的个数不能整除除数
                test = false;
                // std::cout<<"除不尽"<<std::endl;
                // std::cout<<"项: "<<termPair.first->value->print()<<std::endl;
                // std::cout<<"exp: "<<termPair.first->exp<<std::endl;
                // std::cout<<"数量: "<<num<<std::endl;
            }
        }
        if(test == true)
        {
            std::cout<<"除尽"<<std::endl;
            //将被除数的所有项加到当前节点，并且置项数为原项数处以除数的结果
            node->intake_node(node->lChild, 1, true);
            noUseNodes.insert(node->lChild);
            node->isConst = false;
            for(auto termPair : node->termList)
            {
                auto exp = termPair.first->exp;
                auto term = termPair.first;
                int num = termPair.second;
                if(exp)
                {//该项为乘项
                    node->termList[term] = num/dividend;
                }
                else
                {//该项为除项
                    node->termList[term] = num*dividend;
                }
            }
            node->set_const_val(node->get_const_int_val() / dividend);
            node->needReplace = true;
            return;
        }
    }

    node->add_term(node->val, 1, true);
}

void AlgeSimplify::replace_instruction(Function* func) 
{
    for (auto bb : func->get_basic_blocks()) 
    {
        auto &instructions = bb->get_instructions();
        for(auto inst_iter = instructions.begin(); inst_iter != instructions.end(); inst_iter++) 
        {
            auto instr = *inst_iter;
            if (Utils::is_integer_alge_inst(instr)) 
            {
                auto treeNode = value2Node[instr];
                if(noUseNodes.find(treeNode) != noUseNodes.end())
                    continue;
                if (treeNode != nullptr && treeNode->needReplace) 
                {   
                    // std::cout << "instr: " <<  instr->print() << std::endl;
                    // std::cout << "op: " << ExprNode::get_op_name(treeNode->op) << std::endl;
                    std::map<int, std::vector<Value*>, std::greater<int>> mulTermNumMap;
                    std::map<int, std::vector<Value*>, std::greater<int>> divTermNumMap;
                    for (auto term : treeNode->termList) 
                    {//乘法列表
                        if(term.first->exp == false)
                            continue;
                        if (mulTermNumMap.find(term.second) == mulTermNumMap.end())
                            mulTermNumMap.insert({term.second, {term.first->value}});
                        else
                            mulTermNumMap[term.second].push_back(term.first->value);
                    }
                    for (auto term : treeNode->termList) 
                    {//除法列表
                        if(term.first->exp == true)
                            continue;
                        if (divTermNumMap.find(term.second) == divTermNumMap.end())
                            divTermNumMap.insert({term.second, {term.first->value}});
                        else
                            divTermNumMap[term.second].push_back(term.first->value);
                    }

                    Value* newVal = ConstantInt::get(0, m_);
                    // std::cout << "constVal: " << treeNode->get_const_int_val() << std::endl;
                    // std::cout << "terms: " << std::endl;
                    for (auto termList : mulTermNumMap) 
                    {
                        
                        auto newTerm = termList.second[0];
                        for (int i = 1; i < termList.second.size(); i++) 
                        {
                            newTerm = Utils::insert_add_before(newTerm, termList.second[i], m_, bb, inst_iter);
                        }
                        if (termList.first > 0) 
                        {
                            newTerm = Utils::insert_mul_before(newTerm, ConstantInt::get(termList.first, m_), m_, bb, inst_iter);
                            newVal = Utils::insert_add_before(newVal, newTerm, m_, bb, inst_iter);
                        } 
                        else 
                        {
                            newTerm = Utils::insert_mul_before(newTerm, ConstantInt::get(-termList.first, m_), m_, bb, inst_iter);
                            newVal = Utils::insert_sub_before(newVal, newTerm, m_, bb, inst_iter);
                        }
                    }
                    for (auto termList : divTermNumMap) 
                    {
                        // for (int i = 0; i < termList.second.size(); i++) 
                        // {
                        //     std::cout << termList.second[i]->print() << " , " << termList.first << std::endl;
                        // }
                        
                        auto newTerm = termList.second[0];
                        for (int i = 1; i < termList.second.size(); i++) 
                        {
                            newTerm = Utils::insert_add_before(newTerm, termList.second[i], m_, bb, inst_iter);
                        }
                        if (termList.first > 0) 
                        {
                            newTerm = Utils::insert_sdiv_before(newTerm, ConstantInt::get(termList.first, m_), m_, bb, inst_iter);
                            newVal = Utils::insert_add_before(newVal, newTerm, m_, bb, inst_iter);
                        } 
                        else 
                        {
                            newTerm = Utils::insert_sdiv_before(newTerm, ConstantInt::get(-termList.first, m_), m_, bb, inst_iter);
                            newVal = Utils::insert_sub_before(newVal, newTerm, m_, bb, inst_iter);
                        }
                    }
                    newVal =
                        Utils::insert_add_before(newVal, ConstantInt::get(treeNode->get_const_int_val(), m_), m_, bb, inst_iter);
                    if (newVal != instr){
                        // std::cout << "instr replace_all_use_with: " <<  instr->print() << std::endl;
                        instr->replace_all_use_with(newVal);
                    }
                }
            }
        }
    }
}