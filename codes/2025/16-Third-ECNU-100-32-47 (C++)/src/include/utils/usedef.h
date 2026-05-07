#ifndef __AAAC_UTILS_USEDEF_H__
#define __AAAC_UTILS_USEDEF_H__

#include "ADT/CFG.h"
#include "common.h"

namespace aaac {
namespace frontend {
[[nodiscard]]
inline std::list<BasicBlock::iterator>
getDefsOfVar(std::shared_ptr<BasicBlock> bb, sym::Var *var) {
  std::list<BasicBlock::iterator> defs;
  auto beg = bb->begin();
  while (beg != bb->end()) {
    beg = std::find_if(beg, bb->end(), [&](std::shared_ptr<InsnNode> node) {
      return node->isDef(var);
    });
    if (beg != bb->end())
      defs.push_back(beg++);
  }
  return defs;
}

[[nodiscard]]
inline std::list<BasicBlock::iterator>
getUsesOfVar(std::shared_ptr<BasicBlock> bb, sym::Var *var) {
  std::list<BasicBlock::iterator> uses;
  auto beg = bb->begin();
  while (beg != bb->end()) {
    beg = std::find_if(beg, bb->end(), [&](std::shared_ptr<InsnNode> node) {
      return node->isUse(var);
    });
    if (beg != bb->end())
      uses.push_back(beg++);
  }
  return uses;
}

} // namespace frontend
} // namespace aaac

#endif