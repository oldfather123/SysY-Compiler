#include "ADT/CFG.h"
#include "common.h"
#include "frontend.h"
#include "sym.h"

#include <list>
#include <memory>

namespace aaac {
namespace frontend {
std::unordered_map<sym::Var *, std::vector<frontend::InsnNode *>>
ControlFlowGraph::getUseChain() const {
  std::unordered_map<sym::Var *, std::vector<frontend::InsnNode *>> useChain;
  for (auto bb : this->getBBList()) {
    for (auto insn : *bb) {
    //   this->InsnToBlock[insn.get()] = bb.get();
      for (auto def_var : insn->getDefs()) {
        useChain[def_var.get()] = std::vector<frontend::InsnNode *>();
        useChain[def_var.get()].reserve(2);
      }
    }
  }
  // 对于phi节点的操作数，有可能use在def之前
  for (auto bb : this->getBBList()) {
    for (auto insn : *bb) {
      for (auto use_var : insn->getUses()) {
        useChain[use_var.get()].push_back(insn.get());
      }
    }
  }
  return useChain;
}

bool ControlFlowGraph::isDominate(BasicBlock *a, BasicBlock *b){
    auto current=b;
  while(current!=nullptr){
    if(current == a)
      return true;

    current = current->getDominanceInfo().idom.lock().get();
  }
  return false;
}
} // namespace frontend
} // namespace aaac
