#include "mem2reg.h"


//block和root为当前节点，dfsNum和rootNum是按照前序遍历的顺序的节点编号，used是标志每个节点是否被访问过，且记录每个bb的dfs下标
//block组织的是图G，root组织的是dfs树
void dfsTree(BasicBlockPtr block,vector<BasicBlockPtr>&dfsNum, vector<node*>&rootNum,unordered_map<BasicBlockPtr, int>& used, node* root){
    // if(used.find(block)!=used.end()){
    //     return;
    // }
    dfsNum.push_back(block);
    rootNum.push_back(root);
    used[block] =dfsNum.size()-1;
    for(auto& succ:(block->succBasicBlocks)){
        if(used.find(succ.first)==used.end()){
            root->son.push_back(new node(succ.first));
            root->son.back()->father = root;
            dfsTree(succ.first, dfsNum, rootNum, used,root->son.back());
        }
    }
}

int mEval(int id, vector<node *>& root,vector<BasicBlockPtr>& dfsNum, vector<int >& semi, unordered_map<BasicBlockPtr , int>& used){
    if(root[id]->bb == dfsNum[id]){
        return id;
    }
    else{
        queue<node*> list;
        int ret = id;
        for(int i=0;i<root[id]->son.size();i++){
            list.push(root[id]->son[i]);
        }
        while(!list.empty()){
            node* t = list.front();
            list.pop();
            if(semi[used[t->bb]]<semi[ret]){
                ret = used[t->bb];
            }
            for(int i=0;i<t->son.size();i++){
                list.push(t->son[i]);
            }
        }
        return ret;
    }
}
void mLink(vector<node*>&root,int id, int fatherId){
    root[fatherId]->son.push_back(root[id]);
    root[id] = root[fatherId];
}

//rootNum方便获取父亲节点
void calSemiDominator(vector<BasicBlockPtr>& dfsNum, vector<node*>&rootNum, vector<int>&semi, unordered_map<BasicBlockPtr , int>& used){
    vector<node *> root(semi.size());

    
    //每个节点的semi初始化为自身，且每个节点创造一个单例
    for(int i=0;i<semi.size();i++){
        semi[i] = i;
        root[i] = new node(dfsNum[i]);
    }

    for(int i=dfsNum.size()-1;i>0;i--){
        for(auto &pred:(dfsNum[i]->predBasicBlocks)){
            int q = mEval(used[pred.first], root, dfsNum, semi, used);
            if(semi[q]<semi[i]) semi[i] = semi[q];
        }
        mLink(root, i, used[rootNum[i]->father->bb]);
    }
}

void domTree(BlockPtr block){
    //有bug

    vector<BasicBlockPtr> dfsNum;
    vector<node*> rootNum;
    unordered_map<BasicBlockPtr , int> used;
    node* dfsRoot = new node(block->basicBlocks[0]);
    dfsTree(block->basicBlocks[0], dfsNum, rootNum, used, dfsRoot);

    // vector<int> semi(dfsNum.size()); //存储dfsNum中的下标
    // calSemiDominator(dfsNum, rootNum, semi, used);
    

    // node* root = new node(dfsNum[0]);
    // unordered_map<int, node*> mMap;
    // mMap[0] = root;
    // vector<int> idom(dfsNum.size());
    // idom[0]=0;
    // for(int i=1;i<dfsNum.size();i++){
    //     int parentId = used[rootNum[i]->father->bb];
    //     node * now = mMap[parentId];
    //     int nowId = parentId;
    //     while(nowId>semi[i]){
    //         now = now->father;
    //         nowId = used[now->bb];
    //     }
    //     idom[i] = nowId ;
    //     node *temp = new node(dfsNum[i]);
        
    //     temp->father = now;

    //     now->son.push_back(temp);
    //     mMap[i] = temp;
    //     dfsNum[i]->directDominator =now->bb;
    //     now->bb->dominatorSon[dfsNum[i]] = true;
    // }

    vector<unordered_map<BasicBlockPtr, bool>> Dom(dfsNum.size());
    Dom[0][dfsNum[0]] = true;

    for(int i=1;i<dfsNum.size();i++){
        for(int j=0;j<dfsNum.size();j++){
            Dom[i][dfsNum[j]] = true;
        }
    }
    bool change = true;
    while(change){
        change = false;
        for(int i=0;i<dfsNum.size();i++){
            unordered_map<BasicBlockPtr, bool> newDom;
            for(int j=0;j<dfsNum.size();j++){
                bool flag = false;
                for(auto& pred:(dfsNum[i]->predBasicBlocks)){
                    flag = true;
                    if(Dom[used[pred.first]].find(dfsNum[j])==Dom[used[pred.first]].end()){
                        flag = false;
                        break;
                    }
                }
                if(flag){
                    newDom[dfsNum[j]] = true;
                }
            }
            newDom[dfsNum[i]] = true;
            if(Dom[i]!=newDom){
                Dom[i] = newDom;
                change = true;
            }
        }
    }

    // for(int i=0;i<dfsNum.size();i++){
    //     cerr<<"label:               "<<dfsNum[i]->label->name<<endl;
    //     for(auto& d:Dom[i]){
    //         cerr<<d.first->label->name<<endl;
    //     }
    // }


    for(int i=1 ;i<dfsNum.size();i++){
        queue<BasicBlockPtr> list;
        unordered_map<BasicBlockPtr, bool> vis;
        vis[dfsNum[i]] = true;
        for(auto&pred:(dfsNum[i]->predBasicBlocks)){
            list.push(pred.first);
            vis[pred.first] = true;
        }
        while(!list.empty()){
            auto t = list.front();
            list.pop();
            if(Dom[i].find(t)!=Dom[i].end()){
                dfsNum[i]->directDominator = t;
                t->dominatorSon[dfsNum[i]] = true;
                break;
            }
            for(auto&pred:(t->predBasicBlocks)){
                if(vis.find(pred.first)==vis.end()){
                    list.push(pred.first);
                    vis[pred.first] = true;
                }
            }

        }
    }

    //计算DF
    for(int i=0;i<dfsNum.size();i++){
        if(dfsNum[i]->predBasicBlocks.size()>=2){
            for(auto &pred:(dfsNum[i]->predBasicBlocks)){
                auto runner = pred.first;
                while(runner!=dfsNum[i]->directDominator){
                    runner->DF[dfsNum[i]] = true;
                    if(runner->directDominator){
                        runner = runner->directDominator;
                    }
                    else{
                        break;
                    }
                }
            }
        }
    }
}

bool isADominatorB(BasicBlockPtr A, BasicBlockPtr B, BasicBlockPtr entry){
    if(A == entry){
        return true;
    }
    while(B!=entry){
        if(B == A){
            return true;
        }
        B = B->directDominator;
    }
    return false;
}


struct valBB{
    ValuePtr val;
    BasicBlockPtr BB;
    valBB(ValuePtr iv,BasicBlockPtr iBB):val{iv},BB{iBB}{};
};


//删除没有前驱的块
void eraseNotPredBB(BlockPtr block){
    vector<BasicBlockPtr> newBB;
    newBB.push_back(block->basicBlocks[0]);
    for(int i=1;i<block->basicBlocks.size();i++){
        if(block->basicBlocks[i]->predBasicBlocks.size()>0){
            newBB.push_back(block->basicBlocks[i]);
        }
        else{
            //to-do  递归删除
            for(auto &succ:(block->basicBlocks[i]->succBasicBlocks)){
                succ.first->predBasicBlocks.erase(block->basicBlocks[i]);
            }
        }
    }
    block->basicBlocks = newBB;
}

void mem2reg(BlockPtr block){
    eraseNotPredBB(block);

    // for(auto &bb:(block->basicBlocks)){
    //     cerr<<"label: "<<bb->label->name<<endl;
    //     if(bb->directDominator){
    //         cerr<<"dirdom : "<<bb->directDominator->label->name<<endl;
    //     }
    // }

    auto entry = block->basicBlocks[0];
    unordered_map<ValuePtr, vector<BasicBlockPtr>> defBB;
    unordered_map<ValuePtr, vector<BasicBlockPtr>> useBB;

    //去除对应alloca的entry
    vector<InstructionPtr> newEntryInst;
    for(int i=0;i<entry->instructions.size();i++){
        if(entry->instructions[i]->type==Alloca){
            auto tI = ((AllocaInstruction*)(entry->instructions[i].get()));
            if(tI->des->type->isFloat()||tI->des->type->isInt()||tI->des->type->isPtr()){
                //记录需要删除的alloca变量
                defBB[((AllocaInstruction*)(entry->instructions[i].get()))->des] = {};
                useBB[((AllocaInstruction*)(entry->instructions[i].get()))->des] = {};
            }
            else{
                newEntryInst.emplace_back(entry->instructions[i]);
            }
        }
        else{
            newEntryInst.emplace_back(entry->instructions[i]);
        }
    }
    //记录store的块
    for(auto &bb:(block->basicBlocks)){
        for(auto &inc:(bb->instructions)){
            if(inc->type == Store&&defBB.find(((StoreInstruction*)(inc.get()))->des)!=defBB.end()){
                defBB[((StoreInstruction*)(inc.get()))->des].emplace_back(bb);
            }
        }
    }
    //记录load的块
    for(auto &bb:(block->basicBlocks)){
        for(auto &inc:(bb->instructions)){
            if(inc->type == Load&&useBB.find(((LoadInstruction*)(inc.get()))->from)!=useBB.end()){
                useBB[((LoadInstruction*)(inc.get()))->from].emplace_back(bb);
            }
        }
    }
    // for(auto &valDef:defBB){
    //     unordered_map<BasicBlockPtr,bool> checkWorkList;
    //     queue<BasicBlockPtr> workList;
    //     for(auto &t:(valDef.second)){
    //         if(checkWorkList.find(t)==checkWorkList.end()){
    //             checkWorkList[t] = true;
    //             workList.push(t);
    //         }
    //     }
    //     unordered_map<BasicBlockPtr,bool> finish;
    //     while(!workList.empty()){
    //         auto now = workList.front();
    //         workList.pop();
    //         //cerr<<"block :"<<now->label->name<<endl;
    //         for(auto &df:now->DF){
    //             //cerr<<"df: "<<df.first->label->name<<endl; 
    //             if(finish.find(df.first)==finish.end()){
    //                 auto mBegin = df.first->instructions.begin();
    //                 df.first->instructions.emplace(mBegin,InstructionPtr(new PhiInstruction(df.first, valDef.first)));

    //                 finish[df.first] = true;
    //                 if(checkWorkList.find(df.first)==checkWorkList.end()){
    //                     workList.push(df.first);
    //                 }
    //             }
    //         }
    //     }
    // }

    unordered_map<BasicBlockPtr,ValuePtr> inworkList;
    unordered_map<BasicBlockPtr,ValuePtr> inserted;
    for(auto&bb:block->basicBlocks){
        inworkList[bb] = nullptr;
        inserted[bb] = nullptr;
    }
    queue<BasicBlockPtr> workList;

    for(auto &valDef:defBB){
        queue<BasicBlockPtr> liveInBBWorkList;
        unordered_map<BasicBlockPtr, bool> liveInBB;
        //从vector变成unordered_map提升效率
        unordered_map<BasicBlockPtr, bool> defMap;
        for(int i=0;i<valDef.second.size();i++){
            defMap[valDef.second[i]] = true; 
        }

        for(auto &use:(useBB[valDef.first])){
            if(defMap.find(use) != defMap.end()){
                bool flag = true;
                for(auto & I:(use->instructions)){
                    if(I->type == Store){
                        auto tempI = (StoreInstruction*)(I.get());
                        if(tempI->des!=valDef.first){
                            continue;
                        }
                        flag = false;
                        break;
                    }
                    else if(I->type == Load){
                        auto tempI = (LoadInstruction*)(I.get());
                        if(tempI->from!=valDef.first){
                            continue;
                        }
                        break;
                    }
                }
                if(flag){
                    liveInBBWorkList.push(use);
                }
            }
            else{
                liveInBBWorkList.push(use);
            }
        }

        while(!liveInBBWorkList.empty()){
            auto tempBB = liveInBBWorkList.front();
            liveInBBWorkList.pop();
            if(liveInBB.find(tempBB)!=liveInBB.end()){
                continue;
            }
            liveInBB[tempBB] = true;
            for(auto& pred:(tempBB->predBasicBlocks)){
                if(defMap.find(pred.first)!=defMap.end()){
                    continue;
                }
                liveInBBWorkList.push(pred.first);
            }
        }


        for(auto &t:(valDef.second)){
            inworkList[t] = valDef.first;
            workList.push(t);
        }
        //unordered_map<BasicBlockPtr,bool> finish;
        while(!workList.empty()){
            auto now = workList.front();
            workList.pop();
            //cerr<<"block :"<<now->label->name<<endl;
            for(auto &df:now->DF){
                //cerr<<"df: "<<df.first->label->name<<endl; 
                if(inserted[df.first] != valDef.first && liveInBB.find(df.first) != liveInBB.end()){

                    auto mBegin = df.first->instructions.begin();
                    df.first->instructions.emplace(mBegin,InstructionPtr(new PhiInstruction(df.first, valDef.first)));
                    inserted[df.first] = valDef.first;
                    //finish[df.first] = true;
                    if(inworkList[df.first]!= valDef.first){
                        inworkList[df.first] = valDef.first;
                        
                        workList.push(df.first);
                    }
                }
            }
        }
    }




    entry->instructions = newEntryInst;
    //每个变量一个stack
    unordered_map<ValuePtr ,vector<valBB>> stak;
    for(auto & varDef: defBB){
        //默认初始化为0
        if(varDef.first->type->isInt())
            stak[varDef.first] = {valBB(ValuePtr(new Const(Type::getInt(), 0)),entry)};
        else{
            stak[varDef.first] = {valBB(ValuePtr(new Const(Type::getFloat(), 0)),entry)};
        }
    }

    stack<BasicBlockPtr> list; 
    vector<InstructionPtr> newInst;
    list.push(entry);


    unordered_map<BasicBlockPtr,bool> visited;
    visited[entry] = true;

    
    while(!list.empty()){
        auto bb = list.top();
        //cerr<<bb->label->name<<endl;
        list.pop();
        //先记录要留下的指令，再统一删除，防止超时
        newInst.clear();
        //cerr<<"label:    "<<bb->label->name<<endl;
        for(int i=0;i<bb->instructions.size();i++){
            if(bb->instructions[i]->type == Load){
                auto to = ((LoadInstruction*)(bb->instructions[i].get()))->to;
                auto from = ((LoadInstruction*)(bb->instructions[i].get()))->from;
                //cerr<<"load:   "<<to->name<<" "<<from->name<<endl;
                if(stak.find(from)!=stak.end()){
                    //cerr<<"mmmm:   "<<to->name<<" "<<from->name<<endl;
                    //cerr<<"load pre: "<< stak[from.get()].back().val->name<<endl;
                    // while(!isADominatorB(stak[from.get()].back().BB, bb, entry.get())){
                    //     stak[from.get()].pop_back();
                    //     //cerr<<"load pre: "<< stak[from.get()].back().val->name<<endl;
                    // }
                    int stakIdx = stak[from].size()-1;
                    while(!isADominatorB(stak[from][stakIdx].BB, bb, entry)){
                        stakIdx--;
                    }
                    //cerr<<"after pre: "<< stak[from.get()].back().val->name<<endl;
                    //replaceVarByVar(to,stak[from.get()].back().val);
                    replaceVarByVar(to,stak[from][stakIdx].val);
                    if(!rmInstructionUse(bb->instructions[i], from)){
                        cerr<<"error\n";
                    }

                }
                else{
                    newInst.emplace_back(bb->instructions[i]);
                }
            }
            //定义
            else if(bb->instructions[i]->type == Store){
                auto des = ((StoreInstruction*)(bb->instructions[i].get()))->des;
                auto value = ((StoreInstruction*)(bb->instructions[i].get()))->value;
                //cerr<<"store:   "<<des->name<<" "<<value->name<<endl;

                if(stak.find(des)!=stak.end()){
                    //cerr<<"kkk:   "<<des->name<<endl;
                    // while(!isADominatorB(stak[des.get()].back().BB, bb, entry.get())){
                    //     stak[des.get()].pop_back();
                    // }
                    int stakIdx = stak[des].size()-1;
                    while(!isADominatorB(stak[des][stakIdx].BB, bb, entry)){
                        stakIdx--;
                    }
                    stak[des].emplace_back(valBB(value,bb));
                    if(!rmInstructionUse(bb->instructions[i], des)){
                        cerr<<"error\n";
                    }
                    if(!rmInstructionUse(bb->instructions[i], value)){
                        cerr<<"error\n";
                    }
                }
                else{
                    newInst.emplace_back(bb->instructions[i]);
                }
            }
            //定义
            else if(bb->instructions[i]->type == Phi){
                auto val = ((PhiInstruction*)(bb->instructions[i].get()))->val;
                auto reg = ((PhiInstruction*)(bb->instructions[i].get()))->reg;
                //cerr<<"phi:   "<<reg->name<<" "<<val->name<<endl;
                if(stak.find(val)!=stak.end()){
                    //cerr<<"phi:   "<<reg->name<<" "<<val->name<<endl;
                    int stakIdx = stak[val].size()-1;
                    while(!isADominatorB(stak[val][stakIdx].BB, bb, entry)){
                        stakIdx--;
                    }

                    // while(!isADominatorB(stak[val].back().BB, bb, entry)){
                    //     stak[val].pop_back();
                    // }

                    stak[val].emplace_back(valBB(reg,bb));
                }
                newInst.emplace_back(bb->instructions[i]);
                //phi指令不用删除
            }
            else{
                newInst.emplace_back(bb->instructions[i]);
            }
        }

        for(auto &succ:(bb->succBasicBlocks)){
            //cerr<<succ.first->label->name<<endl;
            for(auto & ins:(succ.first->instructions)){
                if(ins->type == Phi){
                    auto tempIns = (PhiInstruction*)(ins.get());
                    auto val = tempIns->val;
                    auto reg = tempIns->reg;
                    
                    if(stak.find(val)!=stak.end()){
                        //while(!isADominatorB(stak[val].back().BB, succ.first, entry)){
                        //cerr<<reg->name<<val->name<<endl;
                        //cerr<<"pre:    "<<stak[val].back().val->name<<endl;
                        // while(!isADominatorB(stak[val].back().BB, bb, entry)){
                        //     stak[val].pop_back();
                        // }
                        int stakIdx = stak[val].size()-1;
                         while(!isADominatorB(stak[val][stakIdx].BB, bb, entry)){
                            stakIdx--;
                        }  
                        //cerr<<"after:    "<<stak[val].back().val->name<<endl;
                        // auto newVal = stak[val].back().val;
                        auto newVal = stak[val][stakIdx].val;
                        tempIns->addFrom(ValuePtr(newVal),BasicBlockPtr(bb));
                        newUse(newVal.get(),ins.get());
                    }
                }
                //Phi一定在最前面
                else{break;}
            }
        }

        bb->instructions = newInst;
        // for(auto &domson:(bb->dominatorSon)){
        //     list.push(domson.first);
        // }
        for(auto &succ:(bb->succBasicBlocks)){
            if(visited.find(succ.first)==visited.end()){
                list.push(succ.first);
                visited[succ.first] = true;
            }
        }
    }

    // unordered_map<Value* ,ValuePtr> stak;
    // for(auto & varDef: defBB){
    //     //默认初始化为0
    //     stak[varDef.first] = ValuePtr(new Const(Type::getInt(), 0));
    // }

    // queue<BasicBlock*> list; 
    // vector<InstructionPtr> newInst;
    // list.push(entry.get());

    // unordered_map<BasicBlock*,bool> visited;
    // visited[entry.get()] = true;
    // while(!list.empty()){
    //     auto bb = list.front();
    //     list.pop();
    //     //先记录要留下的指令，再统一删除，防止超时
    //     newInst.clear();
    //     for(int i=0;i<bb->instructions.size();i++){
    //         if(bb->instructions[i]->type == Load){
    //             auto to = ((LoadInstruction*)(bb->instructions[i].get()))->to;
    //             auto from = ((LoadInstruction*)(bb->instructions[i].get()))->from;
    //             if(stak.find(from.get())!=stak.end()){
    //                 replaceVarByVar(to,stak[from.get()]);
    //             }
    //             else{
    //                 newInst.emplace_back(bb->instructions[i]);
    //             }
    //         }
    //         //定义
    //         else if(bb->instructions[i]->type == Store){
    //             auto des = ((StoreInstruction*)(bb->instructions[i].get()))->des;
    //             auto value = ((StoreInstruction*)(bb->instructions[i].get()))->value;
    //             if(stak.find(des.get())!=stak.end()){
    //                 stak[des.get()]=value;
    //             }
    //             else{
    //                 newInst.emplace_back(bb->instructions[i]);
    //             }
    //         }
    //         //定义
    //         else if(bb->instructions[i]->type == Phi){
    //             auto val = ((PhiInstruction*)(bb->instructions[i].get()))->val;
    //             auto reg = ((PhiInstruction*)(bb->instructions[i].get()))->reg;
    //             if(stak.find(val.get())!=stak.end()){
    //                 stak[val.get()]=reg;
    //             }
    //             newInst.emplace_back(bb->instructions[i]);
    //             //phi指令不用删除
    //         }
    //         else{
    //             newInst.emplace_back(bb->instructions[i]);
    //         }
    //     }

    //     for(auto &succ:(bb->succBasicBlocks)){
    //         for(auto & ins:(succ.first->instructions)){
    //             if(ins->type == Phi){
    //                 auto tempIns = (PhiInstruction*)(ins.get());
    //                 auto val = tempIns->val;
    //                 if(stak.find(val.get())!=stak.end()){
    //                     auto newVal = stak[val.get()];
    //                     tempIns->addFrom(ValuePtr(newVal),BasicBlockPtr(bb));
    //                     newUse(newVal.get(),ins.get());
    //                 }
    //             }
    //             //Phi一定在最前面
    //             else{break;}
    //         }
    //     }

    //     bb->instructions = newInst;
    //     for(auto &succ:(bb->succBasicBlocks)){
    //         if(visited.find(succ.first)==visited.end()){
    //             list.push(succ.first);
    //             visited[succ.first] = true;
    //         }
    //     }
    // }

}
