#include "ADT/CFG.h"
#include "common.h"
#include "frontend.h"
#include "sym.h"
#include "utils/Timer.h"
#include "utils/usedef.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <set>

namespace aaac {
namespace frontend {
// static bool isLiveInSingleBlock(const BBProp<UseDefInfo> &info, sym::Var
// *var) {
//   BasicBlock *def_block = nullptr;

//   for (const auto &[bb, use_def] : info) {
//     bool has_use = use_def.use.find(var) != use_def.use.end();
//     bool has_def = use_def.def.find(var) != use_def.def.end();

//     if (has_use || has_def) {
//       if (!def_block) {
//         def_block = bb; // 首次遇到use/def的基本块，记录下来
//       } else if (def_block != bb) {
//         return false; // 出现在了不同的基本块中，不满足条件
//       }
//     }
//   }

//   return def_block != nullptr; // 只有当变量被使用或定义时才返回true
// }
static bool singleDef(const BBProp<UseDefInfo> &info, sym::Var *var) {
  BasicBlock *def_block = nullptr;

  for (const auto &[bb, use_def] : info) {
    if (use_def.def.count(var) > 0) {
      if (!def_block) {
        def_block = bb; // 第一次遇到定义
      } else if (def_block != bb) {
        return false; // 出现多个定义，不满足条件
      }
    }
  }

  return def_block != nullptr; // 只有当变量被定义过才返回 true
}
std::shared_ptr<sym::Var>
ControlFlowGraph::createNewVarWithVersion(std::shared_ptr<sym::Var> var,
                                          unsigned int &version) {
  auto v = sym::Var::create(var->getType(),
                            var->getName() + "." + std::to_string(version++));
  this->locals.push_back(v);
  return v;
}

std::vector<std::weak_ptr<BasicBlock>>
ControlFlowGraph::findAllDefs(const BBProp<UseDefInfo> &useDef,
                              std::shared_ptr<sym::Var> var) {
  std::vector<std::weak_ptr<BasicBlock>> defs;
  defs.reserve(8); // 预分配空间，大多数变量定义点较少
  
  // 使用引用避免多次查找
  const auto& varPtr = var.get();
  
  for (const auto& [bb, info] : useDef) {
    if (info.def.count(varPtr)) {
      defs.emplace_back(bb->shared_from_this());
    }
  }
  return defs;
}

std::vector<std::weak_ptr<BasicBlock>>
ControlFlowGraph::findAllUses(const BBProp<UseDefInfo> &useDef,
                              std::shared_ptr<sym::Var> var) {
  std::vector<std::weak_ptr<BasicBlock>> uses;
  auto findUses = [&](std::shared_ptr<sym::Function> func,
                      std::shared_ptr<BasicBlock> bb) {
    if (useDef.at(bb.get()).use.count(var.get())) {
      uses.emplace_back(bb);
    }
  };
  this->forwardTraverse(findUses, dummyHandler);
  return uses;
}

std::unordered_set<BasicBlock *>
ControlFlowGraph::getLiveBlocksForVar(const BBProp<LivenessInfo> &liveness,
                                      sym::Var *var) {
  std::unordered_set<BasicBlock *> liveBlocks;

  for (auto bb : bbs) {
    auto bbPtr = bb.get();
    if (bbPtr && liveness.count(bbPtr) && liveness.at(bbPtr).in.count(var)) {
      liveBlocks.insert(bbPtr);
    }
  }

  return liveBlocks;
}

/**
 * @brief 构建DFS树
 *
 * 按照DFS遍历的顺序为CFG分配DFS序号
 *
 * @return 返回一个DFS序号到BasicBlock的映射，方便下一步Semi-NCA算法的运行
 */
std::vector<std::weak_ptr<BasicBlock>> ControlFlowGraph::buildDFSTree() {
  unsigned int lastNum = 1;
  std::stack<std::weak_ptr<BasicBlock>> parentStack{};
  std::vector<std::weak_ptr<BasicBlock>> DFSNumMap{
      {},
  };
  TraverseHandler dfsnum = [&lastNum, &parentStack,
                            &DFSNumMap](std::shared_ptr<sym::Function> func,
                                        std::shared_ptr<BasicBlock> bb) {
    bb->getDFSInfo().order = lastNum++;
    if (!parentStack.empty()) {
      auto p = parentStack.top().lock();
      bb->getDFSInfo().parent = p;
      p->getDFSInfo().children.emplace_back(bb);
    } else {
      bb->getDFSInfo().parent = {};
    }
    parentStack.push(bb);
    DFSNumMap.emplace_back(bb);
  };
  TraverseHandler popParent =
      [&parentStack](std::shared_ptr<sym::Function> func,
                     std::shared_ptr<BasicBlock> bb) { parentStack.pop(); };
  forwardTraverse(dfsnum, popParent);
  return DFSNumMap;
}

struct ControlFlowGraph::BBCompare {
  bool operator()(const BasicBlock *a, const BasicBlock *b) const {
    // 处理空指针（用于稳定性）
    if (!a || !b) {
      return a < b;
    }

    // 1. 比较支配层级（level降序）
    auto a_level = a->getDominanceInfo()->level;
    auto b_level = b->getDominanceInfo()->level;

    if (a_level != b_level) {
      return a_level < b_level; // level大的排前面
    }

    // 2. level相同则比较DFS序号（order降序）
    auto a_order = a->getDFSInfo()->order;
    auto b_order = b->getDFSInfo()->order;

    return a_order < b_order; // order大的排前面
  }
};
/**
 * @brief 根据已有的DefBlocks，计算这些DefBlocks的IDF
 *
 * @param def_blocks 某个变量的def位置
 */
std::vector<std::weak_ptr<BasicBlock>> ControlFlowGraph::calculateIDF(
    const std::vector<std::weak_ptr<BasicBlock>> &defBlocks,
    const std::unordered_set<BasicBlock *> &liveInBlocks) {
  std::vector<std::weak_ptr<BasicBlock>> result;
  std::priority_queue<BasicBlock *, std::vector<BasicBlock *>,
                      ControlFlowGraph::BBCompare>
      q;
  std::unordered_set<BasicBlock *> visitedSubtree, visitedQueue;

  // 构建优先队列
  for (const auto &block : defBlocks) {
    auto bb = block.lock().get();
    if (!bb)
      continue;
    q.push(bb);
    visitedSubtree.insert(bb);
  }

  while (!q.empty()) {
    auto subtreeRoot = q.top();
    q.pop();

    auto subtreeRootDTLevel = subtreeRoot->getDominanceInfo().level;
    std::stack<BasicBlock *> worklist;
    worklist.push(subtreeRoot);

    while (!worklist.empty()) {
      auto cur = worklist.top();
      worklist.pop();

      // 处理当前节点的所有 CFG 后继
      for (auto &succEdge : cur->getSuccEdges()) {
        auto succBB = succEdge.getDest();

        // 空指针保护
        if (!succBB || visitedQueue.count(succBB.get()))
          continue;

        auto succDTLevel = succBB->getDominanceInfo().level;

        // 跳过支配树子树内部的节点
        if (succDTLevel > subtreeRootDTLevel)
          continue;

        //该块不是 live-in 块，则跳过
        if (liveInBlocks.find(succBB.get()) == liveInBlocks.end())
          continue;

        // 加入结果集
        visitedQueue.insert(succBB.get());
        result.emplace_back(succBB);

        // 判断是否需要继续传播
        auto it = std::find_if(defBlocks.begin(), defBlocks.end(),
                               [succBB](const std::weak_ptr<BasicBlock> &wp) {
                                 auto sp = wp.lock();
                                 return sp && sp.get() == succBB.get();
                               });
        if (it == defBlocks.end()) {
          q.push(succBB.get());
        }
      }

      // 处理支配树中的子节点
      for (auto &childW : cur->getDominanceInfo().children) {
        auto child = childW.lock().get();
        if (!child || visitedSubtree.count(child))
          continue;
        visitedSubtree.insert(child);
        worklist.push(child);
      }
    }
  }

  return result;
}

/**
 * @brief 构建Dominator Tree
 *
 * 使用Semi-NCA算法，构造Dominator Tree
 */
void ControlFlowGraph::buildDomTree() {
  // 在开始计算前先清空先前的DomTree和DFS计算结果
  for(auto bb : this->bbs){
    bb->getDominanceInfo() = DominanceInfo();
    bb->getDFSInfo() = DFSInfo();
  }
  
  auto DFSNumMap=this->buildDFSTree();
  const size_t totalBBNum = DFSNumMap.size();
  for (auto bb : bbs) {
    // 暂存parent，这里的parent另有他用
    bb->getDominanceInfo().idom = bb->getDFSInfo().parent;
    bb->getDominanceInfo().best = bb;
    bb->getDominanceInfo().semidom = bb;
  }

  // 然后，应用Semidominator节点的性质
  for (size_t reverseIndex = totalBBNum - 1; reverseIndex >= 2;
       --reverseIndex) {
    // 我们需要按DFS序号从大到小的顺序遍历所有的DFS Node
    // 对于开始节点可以不用做，因此下界为2
    auto currentBB = DFSNumMap[reverseIndex].lock();

    // 路径压缩的过程中会让parent排除路径中不可能成为真正semidominator的semidominator候选，因此我们在这里更新它的semidominator信息而不是在上一个循环中
    currentBB->getDominanceInfo().semidom = currentBB->getDFSInfo().parent;
    for (auto pred : currentBB->getPredEdges()) {
      auto predBB = pred.getDest();

      auto semiCandidate = ancestorWithLowestSemiDom(predBB, currentBB)
                               ->getDominanceInfo()
                               .semidom.lock();
      unsigned int semiCandidateOrder = semiCandidate->getDFSInfo().order;
      unsigned int semiOrder =
          currentBB->getDominanceInfo().semidom.lock()->getDFSInfo().order;
      // 更新为DFSTree序号最小的semidominator
      if (semiCandidateOrder < semiOrder)
        currentBB->getDominanceInfo().semidom = std::weak_ptr(semiCandidate);
    }
  }

  // 计算NCA从而得到idom信息从而构建出Dominator Tree
  for (size_t index = 2; index < totalBBNum; ++index) {
    auto currentBB = DFSNumMap[index].lock();
    const unsigned int semidomOrder =
        currentBB->getDominanceInfo().semidom.lock()->getDFSInfo().order;
    auto reverseIterator = currentBB->getDominanceInfo().idom.lock();

    // 不断回溯直到找到一个序号小于等于semidom的节点，根据Semi-NCA论文中论证的，这就是idom
    while (reverseIterator->getDFSInfo().order > semidomOrder)
      reverseIterator = reverseIterator->getDominanceInfo().idom.lock();

    // 边连接
    reverseIterator->getDominanceInfo().children.emplace_back(currentBB);
    currentBB->getDominanceInfo().idom = reverseIterator;
  }
  // start_bb.lock()->getDominanceInfo().children.push_back(
  //     DFSNumMap[2].lock());
  // DFSNumMap[2].lock()->getDominanceInfo().idom = start_bb;

  this->setDomTreeLevel();
}

/**
 * @brief 获得一个节点有着最小semidominator的祖先节点
 *
 */
std::shared_ptr<BasicBlock> ControlFlowGraph::ancestorWithLowestSemiDom(
    std::shared_ptr<BasicBlock> pred, std::shared_ptr<BasicBlock> current) {
  unsigned int currentDFSNum = current->getDFSInfo().order;
  auto predParent = pred->getDFSInfo().parent.lock();

  if (!predParent || predParent->getDFSInfo().order < currentDFSNum)
    return pred->getDominanceInfo().best.lock();

  // 递归调用并压缩路径
  auto bestCandidate = ancestorWithLowestSemiDom(predParent, current);
  pred->getDFSInfo().parent = predParent->getDFSInfo().parent;

  if (bestCandidate->getDominanceInfo().semidom.lock()->getDFSInfo().order <
      pred->getDominanceInfo()
          .best.lock()
          ->getDominanceInfo()
          .semidom.lock()
          ->getDFSInfo()
          .order)
    pred->getDominanceInfo().best = bestCandidate;

  return pred->getDominanceInfo().best.lock();
}

/**
 * @brief 设置Dominator Tree的层级
 */
void ControlFlowGraph::setDomTreeLevel() {
  std::function<void(std::shared_ptr<BasicBlock>, unsigned int)> setLevel;
  setLevel = [&](std::shared_ptr<BasicBlock> bb, unsigned int level) -> void {
    bb->getDominanceInfo().level = level;
    for (auto child : bb->getDominanceInfo().children)
      setLevel(child.lock(), level + 1);
  };
  setLevel(start_bb.lock(), 0);
}

/**
 * @brief 根据已有的Use/def情况计算活跃性
 *
 *
 */
BBProp<LivenessInfo>
ControlFlowGraph::calculateLiveness(const BBProp<UseDefInfo> &useDef) {
  BBProp<LivenessInfo> result;
  for (auto bb : bbs) {
    result[bb.get()] = LivenessInfo();
  }
  bool changed = false;
  TraverseHandler calculate = [&useDef, &result,
                               &changed](std::shared_ptr<sym::Function> func,
                                         std::shared_ptr<BasicBlock> bb) {
    Assert(useDef.count(bb.get()), "Missing Use Def information");
    auto &BBLivenessInfo = result[bb.get()];
    const auto &BBUseDefInfo = useDef.at(bb.get());

    // 计算 live_out = U successor.live_in
    std::set<sym::Var *> new_out;
    for (const auto &succ_edge : bb->getSuccEdges()) {
      auto succ_bb = succ_edge.getDest();
      if (succ_bb && result.count(succ_bb.get())) {
        new_out.insert(result[succ_bb.get()].in.begin(),
                       result[succ_bb.get()].in.end());
      }
    }

    // 计算 live_in = use U (out - def)
    std::unordered_set<sym::Var *> temp_diff;
    std::set_difference(new_out.begin(), new_out.end(),
                        BBUseDefInfo.def.begin(), BBUseDefInfo.def.end(),
                        std::inserter(temp_diff, temp_diff.end()));

    std::set<sym::Var *> new_in;
    std::set_union(BBUseDefInfo.use.begin(), BBUseDefInfo.use.end(),
                   temp_diff.begin(), temp_diff.end(),
                   std::inserter(new_in, new_in.end()));
    if (new_in != BBLivenessInfo.in) {
      BBLivenessInfo.in = new_in;
      changed = true;
    }
    if (new_out != BBLivenessInfo.out) {
      BBLivenessInfo.out = new_out;
    }
    // 检查是否有变化
    // if (new_in != BBLivenessInfo.in || new_out != BBLivenessInfo.out) {
    //   in_diff += new_in.size() - BBLivenessInfo.in.size();
    //   out_diff += new_out.size() - BBLivenessInfo.out.size();
    //   BBLivenessInfo.in = new_in;
    //   BBLivenessInfo.out = new_out;
    //   changed = true;
    // }
  };

  int count = 0;
  WorklistHandler handler = [&](std::shared_ptr<sym::Function> func,
                                std::shared_ptr<BasicBlock> bb,
                                std::vector<std::shared_ptr<BasicBlock>> &inserts)
  {
    changed = false;
    calculate(func,bb);
    // 在某些情况下，exit BB不含变量，因此并不会changed，但是此时也需要插入前驱结点
    if (changed) {
      for (const auto &succ_edge : bb->getPredEdges()) {
        inserts.push_back(succ_edge.getDest());
      }
    }
    count++;
  };

  backwardWorklist(handler);
  std::cout << function.lock()->getName() << " ";
  std::cout << "Liveness total iteration " << count << std::endl;
  // do {
  //   changed = false;
    // backwardTraverse(calculate, dummyHandler);
  // } while (changed);

  return result;
}

/**
 * @brief 计算Use-Def信息
 *
 * 得到的信息是每个基本块中有哪些变量被使用了，以及哪些变量被定义了
 */
BBProp<UseDefInfo> ControlFlowGraph::calculateUseDef() {
  BBProp<UseDefInfo> result;

  for (auto bb : bbs) {
    result[bb.get()] = UseDefInfo();
    std::set<sym::Var *> inblock_def;

    auto &info = result[bb.get()];
    for (auto node : *bb) {
      for (auto &use : node->getUses()) {
        if (inblock_def.count(use.get()) == 0) {
          info.use.insert(use.get());
        }
      }
      for (auto &def : node->getDefs()) {
        inblock_def.insert(def.get());
      }
    }
    info.def = inblock_def;
  }

  return result;
}

/**
 * @brief 根据calculateIDF的计算结果插入phinode
 *
 * 插入的phiNode在后续的rename过程中才被分配版本号，保证了统一性
 *
 * 调用了这个函数后，基本块的第一个Node不一定是label了，还可能是phi
 */
void ControlFlowGraph::insertPhiNode(
    const std::vector<std::weak_ptr<BasicBlock>> &idf,
    std::shared_ptr<sym::Var> var,
    std::unordered_map<PhiNode *, std::shared_ptr<sym::Var>> &phiToIndex,
    std::unordered_set<BasicBlock *> &liveins) {
  std::unordered_map<BasicBlock *, std::shared_ptr<PhiNode>> BBToPhiNode;

  for (auto &bb_weak_ref : idf) {
    auto bb = bb_weak_ref.lock();
    if (BBToPhiNode.count(bb.get()))
      continue;

    auto phiNode = PhiNode::create(var);
    BBToPhiNode[bb.get()] = phiNode;
    bb->push_front(phiNode);
    phiToIndex[phiNode.get()] = var;
  }
}

/**
 * @brief 在插入了phiNode之后进行变量重命名
 */
// void ControlFlowGraph::renameVariables(
//     const std::vector<std::weak_ptr<BasicBlock>> &defBlocks) {}

void ControlFlowGraph::renameUsesNotPhi(
    std::shared_ptr<InsnNode> node,
    std::unordered_map<sym::Var *, std::shared_ptr<sym::Var>> &stack,
    std::unordered_map<sym::Var *, unsigned int> &version) {
  // 不处理
  if (node->getCode() == NodeCode::Phi) {
    return;
  } else {
    auto uses = node->getUses();
    for (auto &use : uses) {
      if (use->getType()->isArrayType() || use->isGlobal())
        continue;
      std::stringstream ss;
      ss << *use;
      Assert(stack[use.get()] != nullptr, "Null income value,current var is:%s",
             ss.str().c_str());
      node->replaceUsesWith(use, stack[use.get()]);
    }
  }
}

void ControlFlowGraph::renameDefs(
    std::shared_ptr<InsnNode> node,
    std::unordered_map<sym::Var *, std::shared_ptr<sym::Var>> &stack,
    std::unordered_map<sym::Var *, unsigned int> &version) {
  auto defs = node->getDefs();
  for (auto &def : defs) {
    if (def->getType()->isArrayType() || def->isGlobal()) {
      continue;
    }
    auto idx = def.get();
    auto new_var = this->createNewVarWithVersion(def, version[idx]);
    stack[idx] = new_var;
    node->replaceDefWith(new_var);
  }
}

/**
 * @brief 利用相关函数将ControlFlowGraph进行SSA构建
 */
void ControlFlowGraph::buildSSA() {
  std::unordered_map<sym::Var *, std::shared_ptr<sym::Var>> renameStack;
  std::unordered_map<sym::Var *, unsigned int>
      renameVersion; // 版本号，用于在新建变量压栈时为其命名
  std::unordered_map<PhiNode *, std::shared_ptr<sym::Var>> phiToIndex;

  this->buildDomTree();

  auto useDefInfo =
      this->calculateUseDef(); //计算出的临时use-def链，它很可能在变量重命名后失效
  utils::Timer timer;
  // 为每个变量分别计算idf并插入phi节点
  auto livenessInfo = this->calculateLiveness(useDefInfo);
  std::cout << "Time to calculateLiveness " << timer.elapsed() << " ms\n";

  timer.reset();
  size_t varCount = 0;
  long long defBlocksTime = 0;
  long long liveInBlocksTime = 0;
  long long idfTime = 0;
  long long phiTime = 0;
  long long stepTime = 0;

  for (auto var : this->locals) {
    varCount++;
    renameVersion[var.get()] = 1;
    renameStack[var.get()] = var;

    stepTime = timer.elapsed();
    auto defBlocks = this->findAllDefs(useDefInfo, var);
    defBlocksTime += timer.elapsed() - stepTime;

    stepTime = timer.elapsed();
    auto liveInBlocks = getLiveBlocksForVar(livenessInfo, var.get());
    liveInBlocksTime += timer.elapsed() - stepTime;

    stepTime = timer.elapsed();
    auto idf = this->calculateIDF(defBlocks, liveInBlocks);
    idfTime += timer.elapsed() - stepTime;

    stepTime = timer.elapsed();
    this->insertPhiNode(idf, var, phiToIndex, liveInBlocks);
    phiTime += timer.elapsed() - stepTime;
  }

  std::cout << "Variable processing stats:\n";
  std::cout << "  Total variables processed: " << varCount << "\n";
  std::cout << "  Time spent in findAllDefs: " << defBlocksTime << " ms\n";
  std::cout << "  Time spent in getLiveBlocks: " << liveInBlocksTime << " ms\n";
  std::cout << "  Time spent in calculateIDF: " << idfTime << " ms\n";
  std::cout << "  Time spent in insertPhiNode: " << phiTime << " ms\n";
  std::cout << "  Total variable processing time: " << timer.elapsed() << " ms\n";
  // std::cout << "*************************" << std::endl;
  // std::cout << *this << std::endl;

  // 插入phi节点后需要进行变量的重命名
  std::function<void(std::shared_ptr<BasicBlock>)> renameFunc;
  renameFunc = [&renameFunc, &renameStack, &renameVersion, &phiToIndex,
                this](std::shared_ptr<BasicBlock> bb) {
    // 暂存变量重命名栈的状态以便恢复
    auto renameStackBottom = renameStack;
    // 重命名所有可能需要重命名的Node
    for (auto &node : *bb) {
      // Phi节点的特殊性使得我们需要手动处理它
      renameUsesNotPhi(node, renameStack, renameVersion);
      renameDefs(node, renameStack, renameVersion);
    }
    // 这里重命名phiNode的use
    for (auto &succ : bb->getSuccEdges()) {
      auto succBB = succ.getDest();
      for (auto &node : *succBB) {
        if (auto phiNode = std::dynamic_pointer_cast<PhiNode>(node)) {
          auto idx = phiToIndex.at(phiNode.get());
          // Assert(renameStack[idx.get()] != nullptr, "Null income value");
          // 作为undef一样的占位符使用
          auto operand = renameStack[idx.get()];
          phiNode->addOperand(operand, bb);
        } else {
          break; // 所有的phiNode一定在基本块开始的位置
        }
      }
    }
    // 递归调用
    for (auto &DTChild : bb->getDominanceInfo().children) {
      renameFunc(DTChild.lock());
    }
    renameStack = renameStackBottom;
  };

  timer.reset();
  renameFunc(this->start_bb.lock());
  std::cout << "Time to rename var : " << timer.elapsed() << " ms\n";

  // 调整位置，把LabelNode移到开头
  TraverseHandler adjustPosition = [](std::shared_ptr<sym::Function> func,
                                      std::shared_ptr<BasicBlock> bb) {
    auto beg = bb->begin();
    while (beg != bb->end()) {
      if ((*beg)->getCode() == NodeCode::Label) {
        auto labelNode = std::dynamic_pointer_cast<LabelNode>(*beg);
        bb->insertNodeBefore(bb->begin(), labelNode);
        beg = bb->removeNode(beg);
      } else {
        ++beg;
      }
    }
  };
  this->forwardTraverse(adjustPosition, dummyHandler);
}

} // namespace frontend
} // namespace aaac
