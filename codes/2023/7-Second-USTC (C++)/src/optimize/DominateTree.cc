#include "Pass.hh"
#include "DominateTree.hh"

void DominateTree::execute() {
    for(auto func:m_->get_functions()) 
    {
        for(auto bb : func->get_basic_blocks())
        {
            bb->dom_frontier_reset();
        }
    }

    for(auto func:m_->get_functions()) {
        if(func->get_basic_blocks().empty())
            continue;
            
        get_idom(func);
        get_df(func);
        //// debug_print_bb2int();
        //// debug_print_reverse_post_order();
        //// debug_print_df(func);
    }
}

void DominateTree::get_post_order(BasicBlock *bb, std::set<BasicBlock* >& visited) {
    if(visited.find(bb) != visited.end())
        return;
    visited.insert(bb);

    for(auto succ : bb->get_succ_basic_blocks()) {
        if(visited.find(succ) == visited.end()){
            get_post_order(succ,visited);
        }
    }

    bb2int[bb] = reverse_post_order.size();
    reverse_post_order.push_back(bb);
}

void DominateTree::get_reverse_post_order(Function * func) {
    doms.clear();
    reverse_post_order.clear();
    bb2int.clear();

    auto root = func->get_entry_block();
    std::set<BasicBlock* > visited = {};
    get_post_order(root,visited);
    reverse_post_order.reverse();
}

BasicBlock* DominateTree::intersect(BasicBlock* finger1, BasicBlock* finger2) {
    while(finger1 != finger2) {
        if(bb2int[finger1] < bb2int[finger2]) {
            finger1 = doms[bb2int[finger1]];
        }
        if(bb2int[finger2] < bb2int[finger1]) {
            finger2 = doms[bb2int[finger2]];
        }
    }
    return finger1;
}


void DominateTree::get_idom(Function *func) {

    get_reverse_post_order(func);
    
    auto root = func->get_entry_block();
    int root_id = bb2int[root];

    //& init doms
    for(int i = 0;i < root_id;i++){
        doms.push_back(nullptr);
    }
    doms.push_back(root);

    bool changed = true;
    while(changed)
    {
        changed = false;
        for(auto bb : reverse_post_order)
        {
            BasicBlock* new_idom;
            //& get the first processed bb
            for(auto prev : bb->get_pre_basic_blocks()) {
                if(doms[bb2int[prev]] != nullptr) {
                    new_idom = prev;
                    break;
                }
            }
            //& if all previous bbs have not been processed, then jump this bb
            if(new_idom == nullptr)
                continue;
            
            //& process this bb
            for(auto prev : bb->get_pre_basic_blocks()) {
                if(doms[bb2int[prev]] == nullptr)
                    continue;
                
                new_idom = intersect(prev, new_idom);
            }
            if(doms[bb2int[bb]] != new_idom) {
                changed = true;
                doms[bb2int[bb]] = new_idom;
            }
        }
    }
    for(auto bb:reverse_post_order){
        bb->set_idom(doms[bb2int[bb]]);
    }
}

void DominateTree::get_df(Function *func) {
    auto bbs = func->get_basic_blocks();

    for(auto bb : bbs) {
        if(bb->get_pre_basic_blocks().size() >= 2) {
            for(auto prev : bb->get_pre_basic_blocks()) {
                auto runner = prev;
                while(runner != doms[bb2int[bb]]) {
                    runner->add_dom_frontier(bb);
                    runner = doms[bb2int[runner]];
                }
            }
        }
    }
}

void DominateTree::debug_print_bb2int() {
    LOG(DEBUG) << "bb2int size is: " << bb2int.size();
    for(auto pair : bb2int) {
        LOG(DEBUG) << "bb2int[" << pair.first->get_name() << "]" << " = " << pair.second;
    }
}


void DominateTree::debug_print_reverse_post_order() {
    LOG(DEBUG) << "reverse_post_order size is: " << reverse_post_order.size();
    for(auto bb : reverse_post_order) {
        LOG(DEBUG) << bb->get_name();
    }
}

void DominateTree::debug_print_df(Function * func) {
    for(auto bb : func->get_basic_blocks()) {
        LOG(DEBUG) << bb->get_name() << " DFs :";
        for(auto df : bb->get_dom_frontier()) {
            LOG(DEBUG) << df->get_name();
        }
    }
}