#include "frontend.h"
#include "ADT/CFG.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace aaac {
namespace optimization {
class CopyPropagationContext {
private:
  std::vector<frontend::BasicBlock::iterator> CopyInstructions;

public:
  CopyPropagationContext() = default;
  void findCopies(frontend::ControlFlowGraph *cfg);
  void eliminateCopies(frontend::ControlFlowGraph *cfg);
  void CopyPropagation(frontend::ControlFlowGraph *cfg);
};

void CopyPropagationContext::findCopies(frontend::ControlFlowGraph *cfg){
    auto preorder = [&](std::shared_ptr<sym::Function> func, 
                       std::shared_ptr<frontend::BasicBlock> bb) {
        for (auto it = bb->begin(); it != bb->end(); ++it) {
            if (auto assign = std::dynamic_pointer_cast<frontend::CopyAssignNode>(*it)) {
              this->CopyInstructions.push_back(it);
              // auto rhs=assign->getOperands().front();
              // this->Alias[assign.get()]=rhs;
            }
        }
    };
    cfg->forwardTraverse(preorder,frontend::ControlFlowGraph::dummyHandler);
}

void CopyPropagationContext::eliminateCopies(frontend::ControlFlowGraph *cfg){
  auto useChain=cfg->getUseChain();
    for (auto copyIt : CopyInstructions) {
        auto assign = std::dynamic_pointer_cast<frontend::CopyAssignNode>(*copyIt);
        auto def = assign->getDefs().front();
        auto new_op=assign->getOperands().front();
        for(auto use:useChain[def.get()]){
          use->replaceUsesWith(def, new_op);
        }
        (*copyIt)->getParent()->removeNode(copyIt);
    }
}

void CopyPropagationContext::CopyPropagation(frontend::ControlFlowGraph *cfg){
    this->findCopies(cfg);
    this->eliminateCopies(cfg);
}

} // namespace optimization
} // namespace aaac

void CopyPropagation(aaac::frontend::ControlFlowGraph *cfg){
  auto context=aaac::optimization::CopyPropagationContext();
  context.CopyPropagation(cfg);
}
