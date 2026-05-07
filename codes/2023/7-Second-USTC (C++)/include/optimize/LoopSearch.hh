#pragma once

#include "Pass.hh"
#include "Module.hh"

#include <unordered_map>
#include <unordered_set>

struct CFGNode
{
public:
    BasicBlock *bb;
    std::unordered_set<CFGNode *> succ;
    std::unordered_set<CFGNode *> prev;
    int dfn;    
    int low;
    bool inStack;
    CFGNode(BasicBlock *bb_, int dfn_, int low_, bool inStack_)
        : bb(bb_), dfn(dfn_), low(low_), inStack(inStack_) {}
    
    std::unordered_set<CFGNode *>& get_prev(){return prev;}
};
using BBLoop = std::unordered_set<BasicBlock *>;
using NodeLoop = std::unordered_set<CFGNode *>;
using NodeLoopSet = std::unordered_set<std::unordered_set<CFGNode *>*>;
using BBLoopSet = std::unordered_set<std::unordered_set<BasicBlock *>*>;


class LoopSearch : public Pass
{
public:
    LoopSearch(Module *module, bool print_ir=false) : Pass(module, print_ir) {}
    void execute() final;
    const std::string get_name() const override {return name;}
    
private:
    int idxCnt;
    std::vector<CFGNode *> stack;
    Function *func_;
    Function *main_func;
    const std::string name = "LoopSearch";
    // 所有循环
    BBLoopSet loopSet;
    // 函数对应的所有循环
    std::unordered_map<Function *, BBLoopSet> func2Loop;
    std::unordered_map<BasicBlock *, BBLoop *> base2Loop;
    std::unordered_map<BBLoop *, BasicBlock *> loop2Base;
    std::unordered_map<BasicBlock *, BasicBlock *> bb2Base;


public:
    void build_cfg(Function *func, std::unordered_set<CFGNode* >& nodes);
    bool find_scc(NodeLoop &nodes, NodeLoopSet& result);
    void tarjan(CFGNode *now, NodeLoopSet& result);
    // 寻找循环入口节点
    CFGNode *find_loop_base(NodeLoop *set,  NodeLoop &reserved);
    void build_loopBB_from_loopNode(BBLoop &loopBB, NodeLoop &loopNode)
    {
        for(auto node : loopNode)
        {
            loopBB.insert(node->bb);
        }
    }
    

    void set_bb2Base(BasicBlock *block, BasicBlock *base);
    void set_bb2Base(BBLoop *loop, BasicBlock *base);
    void map_bb_and_loop(BBLoop *loop, BasicBlock *base);
    void delete_base_from_nodes(CFGNode *base, NodeLoop &nodes, NodeLoop &reserved);
    void find_loop_in_func(Function *func);
    BBLoopSet &get_loop_set() { return loopSet; }
    BasicBlock *get_loop_base(BBLoop *loop) { return loop2Base[loop];}

    void printSCC(BBLoop * loopBB, int idx=0)
    {
        std::cout<<"找到循环"<<idx<<" :"<<"("<<(*loopBB).size()<<")"<<std::endl;
        for(auto bb : *loopBB)
        {
            std::cout << bb->get_name() << std::endl;
        }
    }

    void debug_print_succ(CFGNode *node)
    {
        std::cout << "succ of " << node->bb->get_name() << ": ";
        for(auto succ : node->succ)
        {
            std::cout << succ->bb->get_name() << ", ";
        }
        std::cout << std::endl;
    }

    void debug_print_prev(CFGNode *node)
    {
        std::cout << "prev of " << node->bb->get_name() << ": ";
        for(auto prev : node->prev)
        {
            std::cout << prev->bb->get_name() << ", ";
        }
        std::cout << std::endl;
    }

    void debug_print_succ(BasicBlock *block)
    {
        std::cout << "succ of " << block->get_name() << ": ";
        for(auto succ : block->get_succ_basic_blocks())
        {
            std::cout << succ->get_name() << ", ";
        }
        std::cout << std::endl;
    }

    void debug_print_prev(BasicBlock *block)
    {
        std::cout << "prev of " << block->get_name() << ": ";
        for(auto prev : block->get_pre_basic_blocks())
        {
            std::cout << prev->get_name() << ", ";
        }
        std::cout << std::endl;
    }
};

// struct CFGNode
// {
//     BasicBlock *bb;
//     std::set<CFGNode *> succ;
//     std::set<CFGNode *> prev;
//     int dfn;    
//     int low;
//     bool inStack;
//     CFGNode(BasicBlock *bb_, int dfn_, int low_, bool inStack_)
//         : bb(bb_), dfn(dfn_), low(low_), inStack(inStack_) {}
// };