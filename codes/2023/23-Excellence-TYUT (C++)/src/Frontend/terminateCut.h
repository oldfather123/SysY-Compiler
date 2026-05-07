//
// Created by 牛奕博 on 2023/7/23.
//

#ifndef ANUC_TERMINATECUT_H
#define ANUC_TERMINATECUT_H
#include "core.h"
#include <queue>
//对终结指令之后的部分进行截断
//并且删除不可达的基本块
namespace anuc {
    class TerminateCut {
        Module *M;
        //广度优先搜索所有不可达基本块，并进行删除
        void eraseUnreachableBB(queue<BasicBlock*> &bbList) {
            vector<BasicBlock*> eraseBBs;
            while(!bbList.empty()) {
                auto front = bbList.front();
                bbList.pop();
                //遍历该节点的后继节点，如果后继节点的前驱为1，也可以删除
                for(auto &succ:front->getSucc()) {
                    if(succ->getPred().size() == 1) bbList.push(succ);
                }
                front->eraseFromParent();
            }
        }
    public:
        TerminateCut(Module *M):M(M) {}
        void run() {
            for(auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                BasicBlock *entry = (&*fn)->getEnrty();
                queue<BasicBlock*> unreachableBB;
                for(auto b = (&*fn)->getBegin(); b != (&*fn)->getEnd();) {
                    BasicBlock *bb = &*b;
                    ++b;
                    if(bb->getPred().empty() && bb != entry) {
                        unreachableBB.push(bb);
                        continue;
                    }
                    Instruction *t = bb->getTerminated();
                    if(!t) cout << bb->getName() << endl;
                    t = t ->getNext();
                    while(t != (&*bb->getEnd())) {
                        Instruction *s = t;
                        t = t->getNext();
                        s->eraseFromParent();
                    }
                }
                eraseUnreachableBB(unreachableBB);
            }
        }
    };
}



#endif //ANUC_TERMINATECUT_H
