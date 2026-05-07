#include "LoopSearch.hh"

void LoopSearch::build_cfg(Function *func, std::unordered_set<CFGNode* >& nodes)
{
    //创建CFG节点，并维护与bb的对应关系
    std::unordered_map<BasicBlock*, CFGNode* > bb2Node;
    for(auto bb:func->get_basic_blocks())
    {
        auto node = new CFGNode(bb, -1, -1, false);
        nodes.insert(node);
        bb2Node.insert({bb, node});
    }

    for(auto bb:func->get_basic_blocks())
    {
        auto node = bb2Node[bb];
        for(auto prev : bb->get_pre_basic_blocks())
        {
            node->prev.insert(bb2Node[prev]);
        }
        for(auto succ : bb->get_succ_basic_blocks())
        {
            node->succ.insert(bb2Node[succ]);
        }
    }
}

bool LoopSearch::find_scc(NodeLoop &nodes, NodeLoopSet& result)
{
    idxCnt = 0;
    stack.clear();
    for(auto node:nodes)
    {
        if(node->dfn == -1)
        {
            tarjan(node, result);
        }
    }
    
    return result.size();
}

void LoopSearch::tarjan(CFGNode *now, NodeLoopSet& result)
{

    stack.push_back(now);
    now->inStack = true;
    now->low = idxCnt;
    now->dfn = idxCnt;
    idxCnt++;

    for(auto succ : now->succ)
    {
        if(succ->dfn == -1)
        {
            tarjan(succ, result);
            now->low = std::min(now->low, succ->low);
        }
        else if(succ->inStack)
        {
            now->low = std::min(succ->dfn, now->low);
        }
    }

    if(now->low == now->dfn)
    {
        NodeLoop *loop = new NodeLoop;
        while(1)
        {
            CFGNode *top = stack.back();
            stack.pop_back();
            top->inStack = false;
            loop->insert(top);
            if(top == now)
            {
                break;
            }
        }

        //将找到的scc加入集合
        if(loop->size() > 1)
        {
            result.insert(loop);
        }
        else if(loop->size() == 1)
        {//自环
            auto n = *loop->begin();
            for(auto succ : n->succ)
            {
                if(succ == n)
                {
                    result.insert(loop);
                    break;
                }
            }
        }
    } 
}

CFGNode *LoopSearch::find_loop_base(NodeLoop *set,  NodeLoop &reserved)
{
    CFGNode *base{nullptr};
    for(auto node : *set)
    {
        for(auto prev : node->prev)
        {
            if(set->find(prev) == set->end())
            {
                base = node;
            }
        }
    }
    if(base != nullptr)
    {
        return base;
    }

    for(auto node : reserved)
    {
        for(auto succ : node->succ)
        {
            if(set->find(succ) != set->end())
            {
                base = succ;
            }
        }
    }
    return base;
}


void LoopSearch::execute()
{
    m_->set_print_name();
    for(auto func : m_->get_functions())
    {
        if (func->get_basic_blocks().empty()) 
            continue;

        find_loop_in_func(func);
    }
}

void LoopSearch::set_bb2Base(BasicBlock *block, BasicBlock *base)
{
    if(bb2Base.find(block) != bb2Base.end())
        bb2Base[block] = base;
    else
        bb2Base.insert({block, base});
}


void LoopSearch::set_bb2Base(BBLoop *loop, BasicBlock *base)
{
    for(auto block : *loop)
        set_bb2Base(block, base);
}

void LoopSearch::delete_base_from_nodes(CFGNode *base, NodeLoop &nodes, NodeLoop &reserved)
{
    reserved.insert(base);
    nodes.erase(base);

    for(auto succ : base->succ){
        succ->prev.erase(base);
    }
    for(auto prev : base->prev)
    {
        prev->succ.erase(base);
    }
}

void LoopSearch::map_bb_and_loop(BBLoop *loop, BasicBlock *base)
{
    base2Loop.insert({base, loop});
    loop2Base.insert({loop, base});
}

void LoopSearch::find_loop_in_func(Function *func)
{
    NodeLoop nodes;
    NodeLoop reserved;
    NodeLoopSet SCCs;

    build_cfg(func, nodes);

    int SCCIdx = 0;
    while(find_scc(nodes, SCCs))
    {
        assert(SCCs.size());
        for(auto SCC : SCCs)
        {
            SCCIdx++;
            auto base = find_loop_base(SCC, reserved);
            auto bbSet = new BBLoop();
            build_loopBB_from_loopNode(*bbSet, *SCC);
            loopSet.insert(bbSet);
            func2Loop[func].insert(bbSet);

            map_bb_and_loop(bbSet, base->bb);
            set_bb2Base(bbSet, base->bb);
            delete_base_from_nodes(base, nodes, reserved);
            // std::cout<<"base is : "<<base->bb->get_name()<<std::endl;
            // printSCC(bbSet, SCCIdx);
        }
        for(auto node : nodes)
        {
            node->dfn = -1;
            node->low = -1;
            node->inStack = false;
        }
        SCCs.clear();
    }
}