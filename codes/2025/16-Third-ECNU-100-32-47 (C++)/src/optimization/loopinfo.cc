#include "loopinfo.h"
#include "ADT/CFG.h"
#include "common.h"
#include "frontend.h"
#include "sym.h"
#include <algorithm>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <stack>
#include <utility>

namespace aaac {
namespace optimization {

// A Dom B
// TODO:使用路径压缩优化（？）


std::set<std::unique_ptr<NaturalLoop>> NaturalLoopAnalyzer::getAllLoops() {
  std::set<std::unique_ptr<NaturalLoop>> result;
  // std::set<BackEdge> back_edges;
  std::map<frontend::BasicBlock *,std::set<frontend::BasicBlock *>> back_edges;
  // std::deque<frontend::BasicBlock *> path;

  // 寻找BackEdge
  this->cfg->buildDomTree();
  auto find_backedge = [&](std::shared_ptr<sym::Function> function,
                           std::shared_ptr<frontend::BasicBlock> bb) {
    // path.push_back(bb.get());
    for (auto succ_edge : bb->getSuccEdges()) {
      if (cfg->isDominate(succ_edge.getDest().get(), bb.get())) {
        if(back_edges.count(succ_edge.getDest().get())==0){
          back_edges[succ_edge.getDest().get()]=std::set<frontend::BasicBlock *>();
        }
        back_edges[succ_edge.getDest().get()].insert(bb.get());
      }
    }
  };
  this->cfg->forwardTraverse(find_backedge,
                             frontend::ControlFlowGraph::dummyHandler);

  // 填充循环内节点
  std::map<frontend::BasicBlock *, std::unique_ptr<NaturalLoop>> head_to_loop;
  for (auto [head,tail_list] : back_edges) {
    auto loop = std::make_unique<NaturalLoop>(head, tail_list);

    loop->insertLoopNode(head);

    std::stack<frontend::BasicBlock *> worklist;
    // worklist.push(tail);
    for(auto tail:tail_list){
      worklist.push(tail);
    }
    while (!worklist.empty()) {
      auto bb = worklist.top();
      worklist.pop();
      if (loop->getLoopNodes().count(bb)==0) {
        loop->insertLoopNode(bb);
        for (auto pred_edge : bb->getPredEdges()) {
          worklist.push(pred_edge.getDest().get());
        }
      }
    }
    head_to_loop[head] = std::move(loop);
  }

  // 计算拓扑
  std::map<frontend::BasicBlock *,frontend::BasicBlock *> parent_loop_head;
  for (auto &[head_1, loop_1] : head_to_loop) {
    parent_loop_head[head_1]=nullptr;
    // 判断loop_2是否为loop_1的直接父循环
    for (auto &[head_2, loop_2] : head_to_loop) {
      if (loop_2.get() == loop_1.get()) {
        continue;
      }
      if (parent_loop_head[head_1]==nullptr ||
          parent_loop_head[head_1]->getDominanceInfo().level <
              head_2->getDominanceInfo().level) {
        if (std::find(loop_2->getLoopNodes().begin(),
                      loop_2->getLoopNodes().end(),
                      head_1) != loop_2->getLoopNodes().end()) {
          parent_loop_head[head_1]=head_2;
        }
      }
    }
  }

  // 构建拓扑
  std::map<frontend::BasicBlock *, NaturalLoop *> head_to_loop_ptr;
  for(auto &[head,loop]:head_to_loop){
    head_to_loop_ptr[head]=loop.get();
  }
  for(auto [inner_head,outer_head]:parent_loop_head){
    if(outer_head!=nullptr){
      head_to_loop_ptr[outer_head]->addNestedLoop(std::move(head_to_loop[inner_head]));
    }else{
      result.insert(std::move(head_to_loop[inner_head]));
    }
  }

  std::cout << "Total loops:" << head_to_loop.size() << std::endl;
  std::cout << "Toplevel loops:" << result.size() << std::endl;

  return result;
}
} // namespace optimization
} // namespace aaac

void analyzeLoopInfo(aaac::frontend::ControlFlowGraph *cfg){
  aaac::optimization::NaturalLoopAnalyzer analyzer(cfg);
  analyzer.getAllLoops();
}
