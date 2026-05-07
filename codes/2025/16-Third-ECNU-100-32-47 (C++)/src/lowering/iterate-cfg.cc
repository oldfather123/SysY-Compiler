#include "common.h"
#include "frontend.h"
#include "ADT/CFG.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <utility>
#include <vector>
namespace aaac {
namespace frontend {
    void ControlFlowGraph::doForwardTraverse(std::shared_ptr<BasicBlock> bb,std::unordered_set<BasicBlock *> &visited,const ControlFlowGraph::TraverseHandler &preorder,const ControlFlowGraph::TraverseHandler &postorder){
        if(visited.count(bb.get()))
            return;
        visited.insert(bb.get());

        preorder(this->function.lock(),bb);

        for(auto succ:bb->getSuccEdges()){
            doForwardTraverse(succ.getDest(),visited,preorder,postorder);
        }

        postorder(this->function.lock(),bb);
    }

    void ControlFlowGraph::doBackwardTraverse(std::shared_ptr<BasicBlock> bb,std::unordered_set<BasicBlock *> &visited,const ControlFlowGraph::TraverseHandler &preorder,const ControlFlowGraph::TraverseHandler &postorder){
        if(visited.count(bb.get()))
            return;
        visited.insert(bb.get());

        preorder(this->function.lock(),bb);

        for(auto pred:bb->getPredEdges()){
            doBackwardTraverse(pred.getDest(), visited, preorder, postorder);
        }

        postorder(this->function.lock(),bb);
    }
void ControlFlowGraph::backwardTraverse(
    std::shared_ptr<BasicBlock> bb,
    const ControlFlowGraph::TraverseHandler &preorder,
    const ControlFlowGraph::TraverseHandler &postorder){
        std::unordered_set<BasicBlock *> visited;
        doBackwardTraverse(bb, visited, preorder, postorder);
    };
void ControlFlowGraph::forwardTraverse(
    std::shared_ptr<BasicBlock> bb,
    const ControlFlowGraph::TraverseHandler &preorder,
    const ControlFlowGraph::TraverseHandler &postorder){
        std::unordered_set<BasicBlock *> visited;
        doForwardTraverse(bb, visited, preorder, postorder);
    };

void ControlFlowGraph::backwardWorklist(std::shared_ptr<BasicBlock> start_bb,
                std::shared_ptr<BasicBlock> exit_bb, const WorklistHandler &handler) {
    std::priority_queue<std::pair<int,std::shared_ptr<BasicBlock>>> worklist;
    std::unordered_set<std::shared_ptr<BasicBlock>> waiting;
    std::unordered_map<std::shared_ptr<BasicBlock>, int> prio;
    int num = 0;
    // 1. 使用BFS进行初始化,将所有BB加入worklist,并且计算优先级
    // 保证在迭代计算的时候某个节点的一定在其前驱之前计算完成
    std::queue<std::shared_ptr<BasicBlock>> init;
    prio[start_bb] = num++;
    init.push(start_bb);
    worklist.push(std::make_pair(prio[start_bb], start_bb));
    waiting.insert(start_bb);
    while(!init.empty()) {
        std::queue<std::shared_ptr<BasicBlock>> temp;
        while(!init.empty()) {
            auto top = init.front();
            init.pop();
            for(auto &succ : top->getSuccEdges()) {
                if(waiting.count(succ.getDest()) == 0) {
                    prio[succ.getDest()] = num++;
                    temp.push(succ.getDest());
                    worklist.push(std::make_pair(prio[succ.getDest()],succ.getDest()));
                    waiting.insert(succ.getDest());
                }
            }
        }
        std::swap(init,temp);
    }
    // 2. 开始迭代
    while (!worklist.empty()) {
        auto [p,target] = worklist.top();
        worklist.pop();
        waiting.erase(target);
        std::vector<std::shared_ptr<BasicBlock>> inserts;
        inserts.reserve(16);
        handler(this->function.lock(),target,inserts);
        for(auto &insert: inserts) {
            if (waiting.count(insert) == 0) {
                worklist.push(std::make_pair(prio[insert],insert));
                waiting.insert(insert);
            }
        }
    }
}

} // namespace frontend
} // namespace aaac
