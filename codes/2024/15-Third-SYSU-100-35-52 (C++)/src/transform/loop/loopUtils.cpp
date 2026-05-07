#include "loopUtils.h"

// body has  header latch and ....
// allowInnerLoop控制是否允许内部循环的存在，而needSubLoop控制是否需要至少一个内部循环存在
bool collectLoopBody(BasicBlock* header, BasicBlock* latch, const DominateAnalysisResult& dom, const CFGAnalysisResult& cfg,
                     std::unordered_set<BasicBlock*>& body, bool allowInnerLoop, bool needSubLoop) {
    body.insert(header);
    std::stack<BasicBlock*> stack;
    stack.push(latch);
    while(!stack.empty()) {
        const auto u = stack.top();
        stack.pop();
        if(body.insert(u).second) {
            for(auto v : cfg.predecessors(u)) {
                stack.push(v);
            }
        }
    }

    // if(body.size() <= 1)
    //     return false;
    bool hasSubLoop = false;
    for(auto block : body) {

        if(block == header)
            continue;
        for(auto succ : cfg.successors(block)) {
            if(block != latch && !body.count(succ)) {
                return false;
            }

            if(dom.dominate(succ, block) && succ != header) {
                if(!allowInnerLoop)
                    return false;
                hasSubLoop = true;
            }
            if(succ == header && block != latch)
                return false;
        }
    }
    return !(needSubLoop && !hasSubLoop);  // NOLINT
}