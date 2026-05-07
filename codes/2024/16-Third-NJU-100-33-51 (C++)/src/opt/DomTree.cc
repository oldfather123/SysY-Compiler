// Reference: https://oi-wiki.org/graph/dominator-tree

#include "DomTree.hh"

#include "Function.hh"

vector<BasicBlock *> dfsPred;  // fth
vector<int> dsTree;            // fa
vector<int> sDom;              // sdom
vector<int> mn;
vector<vector<int>> sDomSucc;

void DomTree::dfs(BasicBlock *node, int &d, CFG *cfg) {
  dfnToBB.push_back(node);
  bbToDfn[node] = d++;
  for (BasicBlock *succ : *cfg->getSuccOf(node)) {
    if (bbToDfn.find(succ) == bbToDfn.end()) {
      dfsPred[d] = node;
      dfs(succ, d, cfg);
    }
  }
}

int find(int x) {
  if (dsTree[x] == x) {
    return x;
  }
  int tmp = dsTree[x];
  dsTree[x] = find(dsTree[x]);
  if (sDom[mn[tmp]] < sDom[mn[x]]) {
    mn[x] = mn[tmp];
  }
  return dsTree[x];
}

void DomTree::buildDomTree() {
  int size = blocks->getSize();
  dfnToBB.clear();
  dfnToBB.reserve(size);

  bbToDfn.clear();

  dfsPred = vector<BasicBlock *>(size);

  dsTree.clear();
  dsTree.reserve(size);

  sDom.clear();
  sDom.reserve(size);

  mn.clear();
  mn.reserve(size);

  sDomSucc = vector<vector<int>>(size, vector<int>());

  iDom = vector<int>(size);

  int depth = 0;
  dfs(cfg->getEntry(), depth, cfg);
  for (int i = 0; i < size; i++) {
    dsTree.push_back(i);
    sDom.push_back(i);
    mn.push_back(i);
  }

  for (int i = depth - 1; i >= 1; i--) {
    BasicBlock *bb = dfnToBB[i];
    int res = i;
    for (BasicBlock *pred : *cfg->getPredOf(bb)) {
      auto item = bbToDfn.find(pred);
      if (item == bbToDfn.end()) {
        // Unuse block
        continue;
      }
      int pDfn = item->second;
      find(pDfn);
      if (pDfn < i) {
        res = std::min(res, pDfn);
      } else {
        res = std::min(res, sDom[mn[pDfn]]);
      }
    }
    sDom[i] = res;
    dsTree[i] = bbToDfn[dfsPred[i]];
    sDomSucc[sDom[i]].push_back(i);

    int u = bbToDfn[dfsPred[i]];
    for (int uSucc : sDomSucc[u]) {
      find(uSucc);
      if (sDom[mn[uSucc]] == u) {
        iDom[uSucc] = u;
      } else {
        iDom[uSucc] = mn[uSucc];
      }
    }
    sDomSucc[u].clear();
  }
  for (int i = 1; i < size; i++) {
    if (iDom[i] != sDom[i]) {
      iDom[i] = iDom[iDom[i]];
    }
    setDominator(dfnToBB[i], dfnToBB[iDom[i]]);
    dtNodeMap[dfnToBB[iDom[i]]]->domChildren->pushBack(dfnToBB[i]);
  }
  dtActive = true;
  dfActive = false;
}

DomTree::DomTree(Function *func) : dtActive(false) {
  blocks = func->getBasicBlocks();
  cfg = func->getCFG();
  if (!cfg) cfg = func->buildCFG();
  for (BasicBlock *bb : *blocks) {
    dtNodeMap[bb] = new DomTreeNode();
  }
}

void DomTree::draw() {
  vector<std::pair<string, string>> edges;
  for (auto &item : dtNodeMap) {
    if (item.second->dominator) {
      edges.push_back(
          {item.second->dominator->getName(), item.first->getName()});
    } else {
      edges.push_back({"eee", item.first->getName()});
    }
  }
  visualizeGraph(edges);
}

void DomTree::calculateDF() {
  assert(cfg);
  assert(blocks);
  if (!dtReady()) {
    buildDomTree();
  }
  for (BasicBlock *bb : *blocks) {
    dtNodeMap[bb]->dominanceFrontier = new LinkedList<BasicBlock *>();
  }
  for (BasicBlock *bb : *blocks) {
    if (cfg->getPredOf(bb)->getSize() > 1) {
      for (BasicBlock *pred : *cfg->getPredOf(bb)) {
        BasicBlock *runner = pred;
        BasicBlock *idom = getDominator(bb);
        while (runner != idom) {
          dtNodeMap[runner]->dominanceFrontier->pushBack(bb);
          runner = getDominator(runner);
        }
      }
    }
  }
  dfActive = true;
  // #ifdef DEBUG_MODE
  //   for (BasicBlock *bb : *blocks) {
  //     std::cout << bb->getName() << ": ";
  //     for (BasicBlock *df : *getDF(bb)) {
  //       std::cout << df->getName() << " ";
  //     }
  //     std::cout << std::endl;
  //   }
  // #endif
}

void DomTree::calculateIDF(BBListPtr src, BBListPtr result) {
  unordered_set<BasicBlock *> resultSet;
  queue<BasicBlock *> worklist;
  for (BasicBlock *bb : *src) {
    for (BasicBlock *df : *getDF(bb)) {
      resultSet.insert(df);
      worklist.push(df);
    }
  }
  while (!worklist.empty()) {
    BasicBlock *bb = worklist.front();
    worklist.pop();
    for (BasicBlock *df : *getDF(bb)) {
      if (resultSet.count(df) == 0) {
        resultSet.insert(df);
        worklist.push(df);
      }
    }
  }
  for (BasicBlock *bb : resultSet) {
    result->pushBack(bb);
  }
}

void DomTree::mergeChildrenTo(BasicBlock *src, BasicBlock *dest) {
  BBListPtr srcList = dtNodeMap[src]->domChildren;
  BBListPtr destList = dtNodeMap[dest]->domChildren;
  for (BasicBlock *domChild : *srcList) {
    destList->pushBack(domChild);
    setDominator(domChild, dest);
  }
  dtNodeMap[src]->domChildren->clear();
}

BBListPtr DomTree::postOrder() {
  BBListPtr postOrderList = new LinkedList<BasicBlock *>();

  std::function<void(BasicBlock *)> postDfs = [&](BasicBlock *node) -> void {
    for (BasicBlock *child : *getDomChildren(node)) {
      postDfs(child);
    }
    postOrderList->pushBack(node);
  };

  postDfs(cfg->getEntry());
  return postOrderList;
}

bool DomTree::dominates(BasicBlock *parent, BasicBlock *block) {
  BasicBlock *ptr = block;
  while (ptr != parent) {
    auto it = dtNodeMap.find(ptr);
    if (it->second->dominator == nullptr) {
      break;
    }
    ptr = it->second->dominator;
  }
  return ptr == parent;
}

void DomTree::calculateDepth() {
  depthActive = 1;
  std::function<void(DomTreeNode *, uint32_t)> dfs = [&](DomTreeNode *node,
                                                         uint32_t depth) {
    node->depth = depth;
    for (BasicBlock *child : *node->domChildren) {
      dfs(dtNodeMap[child], depth + 1);
    }
  };

  dfs(dtNodeMap[cfg->getEntry()], 0);
}

BasicBlock *DomTree::findLCA(BasicBlock *bbx, BasicBlock *bby) {
  int delta = getDepth(bbx) - getDepth(bby);
  if (delta < 0) {
    std::swap(bbx, bby);
    delta = -delta;
  }
  for (int i = 0; i < delta; i++) {
    bbx = getDominator(bbx);
  }
  while (bbx != bby) {
    bbx = getDominator(bbx);
    bby = getDominator(bby);
  }
  return bbx;
}

void DomTree::eraseNode(BasicBlock *block) {
  dtNodeMap.erase(block);
}
