#ifndef _DOMTREE_H_
#define _DOMTREE_H_

#include <assert.h>

#include <functional>
#include <map>
#include <queue>
#include <unordered_set>

#include "BasicBlock.hh"
#include "CFG.hh"
#include "VisualizeGraph.hh"

using std::map;
using std::queue;
using std::unordered_set;

class Function;

struct DomTreeNode {
  BasicBlock* dominator;
  BBListPtr domChildren;
  BBListPtr dominanceFrontier;
  uint32_t depth;

  DomTreeNode() {
    dominator = nullptr;
    domChildren = new LinkedList<BasicBlock*>();
    dominanceFrontier = new LinkedList<BasicBlock*>();
  }
};

class DomTree {
 private:
  const LinkedList<BasicBlock*>* blocks;
  CFG* cfg;
  unordered_map<BasicBlock*, DomTreeNode*> dtNodeMap;
  // map<BasicBlock*, BasicBlock*> dominators;
  // map<BasicBlock*, BBListPtr> domChildren;
  // map<BasicBlock*, BBListPtr> dominanceFrontier;
  bool dtActive = 0;
  bool dfActive = 0;
  bool depthActive = 0;
  vector<BasicBlock*> dfnToBB;
  map<BasicBlock*, int> bbToDfn;
  vector<int> iDom;
  void dfs(BasicBlock *node, int &d, CFG *cfg);

public:
  DomTree(Function *func);
  DomTree() : dtActive(false) {}

  bool dtReady() { return dtActive; }
  bool dfReady() { return dfActive; }
  bool depthReady() { return depthActive; }

  void setDominator(BasicBlock* bb, BasicBlock* domNode) {
    dtNodeMap[bb]->dominator = domNode;
  }
  void buildDomTree();
  BasicBlock* getDominator(BasicBlock* bb) {
    return dtNodeMap.at(bb)->dominator;
  }
  BBListPtr getDomChildren(BasicBlock* bb) {
    return dtNodeMap.at(bb)->domChildren;
  }
  bool dominates(BasicBlock* parent, BasicBlock* block);
  BBListPtr postOrder();
  BasicBlock* getRoot() { return cfg->getEntry(); }
  void testDomTree();

  void calculateDF();
  BBListPtr getDF(BasicBlock* bb) const {
    return dtNodeMap.at(bb)->dominanceFrontier;
  }

  void mergeChildrenTo(BasicBlock* src, BasicBlock* dest);
  void deleteChildren(BasicBlock* block) {
    dtNodeMap.at(block)->domChildren->clear();
  }
  void deleteParent(BasicBlock* block) {
    dtNodeMap.at(block)->dominator = nullptr;
  }
  void eraseNode(BasicBlock* block);
  void draw();

  void calculateIDF(BBListPtr src, BBListPtr result);
  BasicBlock* findLCA(BasicBlock* bbx, BasicBlock* bby);

  void calculateDepth();
  uint32_t getDepth(BasicBlock* block) { return dtNodeMap.at(block)->depth; }
};

#endif
