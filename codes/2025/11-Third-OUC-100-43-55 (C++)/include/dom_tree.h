#ifndef PASS_DOM_TREE_H
#define PASS_DOM_TREE_H
#include "function.h"

namespace pass {

class DomTree {
private:
  ir::Function *_function;
  std::unordered_map<ir::BasicBlock *, std::unordered_set<ir::BasicBlock *>>
      _predMap;
  std::unordered_map<ir::BasicBlock *, std::unordered_set<ir::BasicBlock *>>
      _domMap;
  std::unordered_map<ir::BasicBlock *, ir::BasicBlock *> _immDomMap;
  std::unordered_map<ir::BasicBlock *, std::unordered_set<ir::BasicBlock *>>
      _domTree;
  std::unordered_map<ir::BasicBlock *, std::unordered_set<ir::BasicBlock *>>
      _domFrontier;

  void analyzePred();
  void analyzeDom();
  void analyzeImmDom();
  void analyzeDomTree();
  void analyzeDomFrontier();

public:
  explicit DomTree(ir::Function *function) : _function(function) {}

  std::unordered_map<ir::BasicBlock *, ir::BasicBlock *> &getImmdom() {
    analyzePred();
    analyzeDom();
    analyzeImmDom();
    return _immDomMap;
  }

  std::unordered_map<ir::BasicBlock *, std::unordered_set<ir::BasicBlock *>> &
  getDomTree() {
    analyzePred();
    analyzeDom();
    analyzeImmDom();
    analyzeDomTree();
    return _domTree;
  }
  std::unordered_map<ir::BasicBlock *, std::unordered_set<ir::BasicBlock *>> &
  getDomFrontire() {
    analyzePred();
    analyzeDom();
    analyzeImmDom();
    analyzeDomFrontier();
    return _domFrontier;
  }
};

} // namespace pass

#endif // PASS_DOM_TREE_HPP
