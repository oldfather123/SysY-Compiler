//
// Created by 牛奕博 on 2023/7/21.
//

#ifndef ANUC_FUNCTIONCALLANALYSIS_H
#define ANUC_FUNCTIONCALLANALYSIS_H
#include <set>
#include <map>
#include "core.h"
#include "callGraph.h"
using namespace std;
namespace anuc {
    class FunctionCallAnalysis {
        Module *M;
        map<Function*,CallNode*> funcToNode;
        set<CallNode*> callNodes;
        void buildGraphNode(Function *func) {
            auto it = funcToNode.find(func);
            CallNode *node;
            if(it != funcToNode.end()) node = it->second;
            else {
                node = new CallNode(func);
                funcToNode.insert({func, node});
            }
       }

        void buildGraph(CallNode *callNode, CallNode *calledNode) {
            callNode->calls.insert(calledNode);
            calledNode->calledBy.insert(callNode);
        }

    public:
        FunctionCallAnalysis(Module *M) :M(M) {}
        //计算函数DAG图
        map<Function*,CallNode*> &runAnalysis() {
            //初始化所有函数节点
            for(auto it1 = M->getBegin(); it1 != M->getEnd(); ++it1) {
                Function *func = &*it1;
                buildGraphNode(func);
            }
            for(auto &e: M->getExternFuncs()) buildGraphNode(e);

            //遍历函数，通过call指令存储调用信息
            for(auto it1 = M->getBegin(); it1 != M->getEnd(); ++it1) {
                Function *func = &*it1;
                CallNode *callNode = funcToNode[func];
                buildGraphNode(func);
                for(auto it2 = func->getBegin(); it2 != func->getEnd(); ++it2) {
                    BasicBlock *bb  = &*it2;
                    for(auto it3 = bb->getBegin(); it3 != bb->getEnd(); ++it3) {
                        Instruction *s = &*it3;
                        if(!isa<CallInst>(s)) continue;
                        CallInst *call = cast<CallInst>(s);
                        Function *called = call->getFunc();
                        buildGraph(callNode, funcToNode[called]);
                    }
                }
            }
            return funcToNode;
        }
    };
}



#endif //ANUC_FUNCTIONCALLANALYSIS_H
