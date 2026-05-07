//
// Created by q1w2e3r4 on 23-7-27.
//

#include "HIR-opt/DomTreePass.h"

set<BasicBlock *> vis;
void postOrder(stack<BasicBlock*>& st, BasicBlock *bb);

void DomTreePass::run() {
    for(auto& it: globalUnit->func_table){
        Function * func = it.second;
        if(!func->block_list.empty()){
            vis.clear();
            DomNode* root = Gen_Dom_Tree(func->block_list[0]);
            func->dom_root = root;
        }
    }
}

void Dom_dfs(DomNode* node);

DomNode *DomTreePass::Gen_Dom_Tree(BasicBlock *entry) {
/* simple test
    auto b0 = new BasicBlock(0);
    auto b1 = new BasicBlock(1);
    auto b2 = new BasicBlock(2);
    auto b3 = new BasicBlock(3);
    auto b4 = new BasicBlock(4);
    auto b5 = new BasicBlock(5);
    auto b6 = new BasicBlock(6);
    auto b7 = new BasicBlock(7);
    auto b8 = new BasicBlock(8);
    b0->succ.push_back(b1);
    b1->succ.push_back(b2); b1->succ.push_back(b5); b1->pred.push_back(b0); b1->pred.push_back(b3);
    b2->succ.push_back(b3); b2->pred.push_back(b1);
    b3->succ.push_back(b1); b3->succ.push_back(b4); b3->pred.push_back(b2); b3->pred.push_back(b7);
    b4->pred.push_back(b3);
    b5->pred.push_back(b1), b5->succ.push_back(b6); b5->succ.push_back(b8);
    b6->pred.push_back(b5); b6->succ.push_back(b7);
    b8->pred.push_back(b5); b8->succ.push_back(b7);
    b7->pred.push_back(b6), b7->pred.push_back(b8); b7->succ.push_back(b3);
    entry = b0;
*/
    // step 1 generate all DonNode -> store to vector
    stack<BasicBlock *> st;
    map<BasicBlock *,int> mapper;
    vis.clear();
    postOrder(st,entry);

    vector<DomNode*> nodes;
    int idx = 0;
    while(!st.empty()){
        BasicBlock * bb = st.top(); st.pop();
        DomNode * dom = new DomNode(idx,bb);
        mapper[bb] = idx;
        nodes.push_back(dom);
        idx ++;
    }

    // map (connection between BasicBlock*) -> (connection between index)
    int n = nodes.size();
    vector<vector<int>> preds(n);
    //cout << "Mapper:" << endl;
    for(int i=0;i<n;i++){
        BasicBlock * block = nodes[i]->bb;
        //cout << i << " : " << block->label << endl;
        for(auto pred: block->pred){
            preds[i].push_back(mapper[pred]);
        }
    }

    // step 2 Now we have all nodes,ready to gen DomNode
    vector<set<int>> out(n);
    out[0].insert(0);
    for(int i=1;i<n;i++){
        for(int j=0;j<n;j++){
            out[i].insert(j);
        }
    }
    bool changed = true;
    while(changed){
        changed = false;
        for(int i=1;i<n;i++){
            set<int> cur;
            for(int j=0;j<n;j++) cur.insert(j);
            for(auto p: preds[i]){
                set<int> tmp;
                set_intersection(cur.begin(),cur.end(),out[p].begin(), out[p].end(),std::inserter(tmp, tmp.begin()));
                cur = std::move(tmp);
            }
            cur.insert(i);

            if(out[i] != cur){
                changed = true;
                out[i] = cur;
            }

//            cout << "Dom" << i << ": ";
//            for(auto t: out[i]){
//                cout << t << " ";
//            }
//            cout << endl;
        }
    }

//    cout << "DOM:" << endl;
//    int tot = 0;
//    for(auto s:out){
//        cout << tot << " : ";
//        for(int i:s){
//            cout << i << ",";
//        }
//        tot ++;
//        cout << endl;
//    }

    // step 3: use DOM to get IDom,then build Dom tree;(int idx -> DomNode*)
    vector<int> idom(n,0);
//    cout << "IDOM:" << endl;
    for(int i=1;i<n;i++){
        DomNode *node = nodes[i];
        auto it = out[i].begin();
        while(it != out[i].end()){
            node->DOM.push_back(nodes[*it]);
            it++;
        }
        // IDOM should be the biggest id of DOM nodes,right?
        --it; --it; // *(it-1) = itself, *(it-2) = IDOM
        node->IDOM = nodes[*it];
        idom[i] = (*it);
        nodes[(*it)]->child.push_back(node);
//        cout << node->nodeId << " : " << nodes[(*it)]->nodeId << endl;
    }

    // step 4: calculate DF
    for(int now=1;now<n;now++){
        int pred_cnt = preds[now].size();
        if(pred_cnt >= 2){
            for(auto p: preds[now]){
                int runner = p;
                while(runner != idom[now]){
                    nodes[runner]->DF.push_back(nodes[now]);
                    runner = idom[runner];
                }
            }
        }
    }

//    cout << "DF:" << endl;
//    for(int i=0;i<n;i++){
//        cout << "node " << i << " : ";
//        for(auto j:nodes[i]->DF){
//            cout << j->nodeId << ",";
//        }
//        cout << endl;
//    }

    // step 5: mark the depth of dom node
    nodes[0]->depth = 1;
    Dom_dfs(nodes[0]);

    return nodes[0];
}

void Dom_dfs(DomNode* node){
//    cerr << node->bb->label << " " << node->depth << endl;
    for(auto child: node->child){
        child->depth = node->depth + 1;
        Dom_dfs(child);
    }
}

void postOrder(stack<BasicBlock*>& st, BasicBlock *bb) {
    vis.insert(bb);
    if(!bb->succ.empty()){
        for (auto it: bb->succ) {
            if(!vis.count(it))
                postOrder(st, it);
        }
    }
    st.push(bb);
}
