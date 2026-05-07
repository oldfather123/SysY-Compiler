#include "Dom.h"
#include <algorithm> // for std::set_intersection, std::reverse
#include <iostream>  // for debug output
#include <limits>    // for std::numeric_limits
#include <queue>
#include <functional> // for std::function
#include <map>
#include <vector>
#include <set>

namespace sysy {

// ==============================================================
// DominatorTreeAnalysisPass 的静态ID
// ==============================================================
void *DominatorTreeAnalysisPass::ID = (void *)&DominatorTreeAnalysisPass::ID;

// ==============================================================
// DominatorTree 结果类的实现
// ==============================================================

// 构造函数：初始化关联函数，但不进行计算
DominatorTree::DominatorTree(Function *F) : AssociatedFunction(F) {
  // 构造时不需要计算，在分析遍运行里计算并填充
}

// Getter 方法 (保持不变)
const std::set<BasicBlock *> *DominatorTree::getDominators(BasicBlock *BB) const {
  auto it = Dominators.find(BB);
  if (it != Dominators.end()) {
    return &(it->second);
  }
  return nullptr;
}

BasicBlock *DominatorTree::getImmediateDominator(BasicBlock *BB) const {
  auto it = IDoms.find(BB);
  if (it != IDoms.end()) {
    return it->second;
  }
  return nullptr;
}

const std::set<BasicBlock *> *DominatorTree::getDominanceFrontier(BasicBlock *BB) const {
  auto it = DominanceFrontiers.find(BB);
  if (it != DominanceFrontiers.end()) {
    return &(it->second);
  }
  return nullptr;
}

const std::set<BasicBlock *> *DominatorTree::getDominatorTreeChildren(BasicBlock *BB) const {
  auto it = DominatorTreeChildren.find(BB);
  if (it != DominatorTreeChildren.end()) {
    return &(it->second);
  }
  return nullptr;
}

// 辅助函数：打印 BasicBlock 集合 (保持不变)
void printBBSet(const std::string &prefix, const std::set<BasicBlock *> &s) {
  if (!DEBUG)
    return;
  std::cout << prefix << "{";
  bool first = true;
  for (const auto &bb : s) {
    if (!first)
      std::cout << ", ";
    std::cout << bb->getName();
    first = false;
  }
  std::cout << "}" << std::endl;
}

// 辅助函数：计算逆后序遍历 (RPO) - 保持不变
std::vector<BasicBlock*> DominatorTree::computeReversePostOrder(Function* F) {
    std::vector<BasicBlock*> postOrder;
    std::set<BasicBlock*> visited;
    
    std::function<void(BasicBlock*)> dfs_rpo =
        [&](BasicBlock* bb) {
        visited.insert(bb);
        for (BasicBlock* succ : bb->getSuccessors()) {
            if (visited.find(succ) == visited.end()) {
                dfs_rpo(succ);
            }
        }
        postOrder.push_back(bb);
    };

    dfs_rpo(F->getEntryBlock());
    std::reverse(postOrder.begin(), postOrder.end());
    
    if (DEBUG) {
        std::cout << "--- Computed RPO: ";
        for (BasicBlock* bb : postOrder) {
            std::cout << bb->getName() << " ";
        }
        std::cout << "---" << std::endl;
    }
    return postOrder;
}

// computeDominators 方法 (保持不变，因为它它是独立于IDom算法的)
void DominatorTree::computeDominators(Function *F) {
  if (DEBUG)
    std::cout << "--- Computing Dominators ---" << std::endl;

  BasicBlock *entryBlock = F->getEntryBlock();
  std::vector<BasicBlock*> bbs_rpo = computeReversePostOrder(F);

  for (BasicBlock *bb : bbs_rpo) {
    if (bb == entryBlock) {
      Dominators[bb].clear();
      Dominators[bb].insert(bb);
      if (DEBUG) std::cout << "Init Dominators[" << bb->getName() << "]: {" << bb->getName() << "}" << std::endl;
    } else {
      Dominators[bb].clear();
      for (BasicBlock *all_bb : bbs_rpo) {
        Dominators[bb].insert(all_bb);
      }
      if (DEBUG) {
        std::cout << "Init Dominators[" << bb->getName() << "]: ";
        printBBSet("", Dominators[bb]);
      }
    }
  }

  bool changed = true;
  int iteration = 0;
  while (changed) {
    changed = false;
    iteration++;
    if (DEBUG) std::cout << "Iteration " << iteration << std::endl;

    for (BasicBlock *bb : bbs_rpo) {
      if (bb == entryBlock) continue;

      std::set<BasicBlock *> newDom;
      bool firstPredProcessed = false;

      for (BasicBlock *pred : bb->getPredecessors()) {
        if(DEBUG){
          std::cout << "  Processing predecessor: " << pred->getName() << std::endl;
        }
          if (!firstPredProcessed) {
              newDom = Dominators[pred];
              firstPredProcessed = true;
          } else {
              std::set<BasicBlock *> intersection;
              std::set_intersection(newDom.begin(), newDom.end(), Dominators[pred].begin(), Dominators[pred].end(),
                                     std::inserter(intersection, intersection.begin()));
              newDom = intersection;
          }
      }
      newDom.insert(bb);

      if (newDom != Dominators[bb]) {
        if (DEBUG) {
          std::cout << "  Dominators[" << bb->getName() << "] changed from ";
          printBBSet("", Dominators[bb]);
          std::cout << "  to ";
          printBBSet("", newDom);
        }
        Dominators[bb] = newDom;
        changed = true;
      }
    }
  }
  if (DEBUG)
    std::cout << "--- Dominators Computation Finished ---" << std::endl;
}

// ==============================================================
// Lengauer-Tarjan 算法辅助数据结构和函数 (私有成员)
// ==============================================================

// DFS 遍历，填充 dfnum_map, vertex_vec, parent_map
// 对应用户代码的 dfs 函数
void DominatorTree::dfs_lt_helper(BasicBlock* u) {
    dfnum_map[u] = df_counter;
    if (df_counter >= vertex_vec.size()) { // 动态调整大小
        vertex_vec.resize(df_counter + 1);
    }
    vertex_vec[df_counter] = u;
    if (DEBUG) std::cout << "  DFS: Visiting " << u->getName() << ", dfnum = " << df_counter << std::endl;
    df_counter++;

    for (BasicBlock* v : u->getSuccessors()) {
        if (dfnum_map.find(v) == dfnum_map.end()) { // 如果 v 未访问过
            parent_map[v] = u;
            if (DEBUG) std::cout << "    DFS: Setting parent[" << v->getName() << "] = " << u->getName() << std::endl;
            dfs_lt_helper(v);
        }
    }
}

// 并查集：找到集合的代表，并进行路径压缩
// 同时更新 label，确保 label[i] 总是指向其祖先链中 sdom_map 最小的节点
// 对应用户代码的 find 函数，也包含了 eval 的逻辑
BasicBlock* DominatorTree::evalAndCompress_lt_helper(BasicBlock* i) {
    if (DEBUG) std::cout << "    Eval: Processing " << i->getName() << std::endl;
    // 如果 i 是根 (ancestor_map[i] == nullptr)
    if (ancestor_map.find(i) == ancestor_map.end() || ancestor_map[i] == nullptr) {
        if (DEBUG) std::cout << "      Eval: " << i->getName() << " is root, returning itself." << std::endl;
        return i; // 根节点自身就是路径上sdom最小的，因为它没有祖先
    }
    
    // 如果 i 的祖先不是根，则递归查找并进行路径压缩
    BasicBlock* root_ancestor = evalAndCompress_lt_helper(ancestor_map[i]);
    
    // 路径压缩时，根据 sdom_map 比较并更新 label_map
    // 确保 label_map[i] 存储的是 i 到 root_ancestor 路径上 sdom_map 最小的节点
    // 注意：这里的 ancestor_map[i] 已经被递归调用压缩过一次了，所以是root_ancestor的旧路径
    // 应该比较的是 label_map[ancestor_map[i]] 和 label_map[i]
    if (sdom_map.count(label_map[ancestor_map[i]]) && // 确保 label_map[ancestor_map[i]] 存在 sdom
        sdom_map.count(label_map[i]) &&                // 确保 label_map[i] 存在 sdom
        dfnum_map[sdom_map[label_map[ancestor_map[i]]]] < dfnum_map[sdom_map[label_map[i]]]) {
        if (DEBUG) std::cout << "      Eval: Updating label for " << i->getName() << " from " 
                              << label_map[i]->getName() << " to " << label_map[ancestor_map[i]]->getName() << std::endl;
        label_map[i] = label_map[ancestor_map[i]];
    }
    
    ancestor_map[i] = root_ancestor; // 执行路径压缩：将 i 直接指向其所属集合的根
    if (DEBUG) std::cout << "      Eval: Path compression for " << i->getName() << ", new ancestor = " 
                          << (root_ancestor ? root_ancestor->getName() : "nullptr") << std::endl;
    
    return label_map[i]; // <-- **将这里改为返回 label_map[i]**
}

// Link 函数：将 v 加入 u 的 DFS 树子树中 (实际上是并查集操作)
// 对应用户代码的 fa[u] = fth[u];
void DominatorTree::link_lt_helper(BasicBlock* u_parent, BasicBlock* v_child) {
    ancestor_map[v_child] = u_parent; // 设置并查集父节点
    label_map[v_child] = v_child;     // 初始化 label 为自身
    if (DEBUG) std::cout << "  Link: " << v_child->getName() << " linked to " << u_parent->getName() << std::endl;
}

// ==============================================================
// Lengauer-Tarjan 算法实现 computeIDoms
// ==============================================================
void DominatorTree::computeIDoms(Function *F) {
    if (DEBUG) std::cout << "--- Computing Immediate Dominators (IDoms) using Lengauer-Tarjan ---" << std::endl;

    BasicBlock *entryBlock = F->getEntryBlock();

    // 1. 初始化所有 LT 相关的数据结构
    dfnum_map.clear();
    vertex_vec.clear();
    parent_map.clear();
    sdom_map.clear();
    idom_map.clear();
    bucket_map.clear();
    ancestor_map.clear();
    label_map.clear();
    df_counter = 0; // DFS 计数器从 0 开始

    // 预分配 vertex_vec 的大小，避免频繁resize
    vertex_vec.resize(F->getBasicBlocks().size() + 1); 
    // 在 DFS 遍历之前，先为所有基本块初始化 sdom 和 label
    // 这是 Lengauer-Tarjan 算法的要求，确保所有节点在 Phase 2 开始前都在 map 中
    for (auto &bb_ptr : F->getBasicBlocks()) {
        BasicBlock* bb = bb_ptr.get();
        sdom_map[bb] = bb; // sdom(bb) 初始化为 bb 自身
        label_map[bb] = bb; // label(bb) 初始化为 bb 自身 (用于 Union-Find 的路径压缩)
    }
    // 确保入口块也被正确初始化（如果它不在 F->getBasicBlocks() 的正常迭代中）
    sdom_map[entryBlock] = entryBlock;
    label_map[entryBlock] = entryBlock;
    // Phase 1: DFS 遍历并预处理
    // 对应用户代码的 dfs(st)
    dfs_lt_helper(entryBlock);
    idom_map[entryBlock] = nullptr; // 入口块没有即时支配者
    if (DEBUG) std::cout << "  IDom[" << entryBlock->getName() << "] = nullptr" << std::endl;

    if (DEBUG) std::cout << "  Sdom[" << entryBlock->getName() << "] = " << entryBlock->getName() << std::endl;
    
    // 初始化并查集的祖先和 label
    for (auto const& [bb_key, dfn_val] : dfnum_map) {
        ancestor_map[bb_key] = nullptr; // 初始为独立集合的根
        label_map[bb_key] = bb_key;   // 初始 label 为自身
    }

    if (DEBUG) {
        std::cout << "  --- DFS Phase Complete ---" << std::endl;
        std::cout << "  dfnum_map:" << std::endl;
        for (auto const& [bb, dfn] : dfnum_map) {
            std::cout << "    " << bb->getName() << " -> " << dfn << std::endl;
        }
        std::cout << "  vertex_vec (by dfnum):" << std::endl;
        for (size_t k = 0; k < df_counter; ++k) {
            if (vertex_vec[k]) std::cout << "    [" << k << "] -> " << vertex_vec[k]->getName() << std::endl;
        }
        std::cout << "  parent_map:" << std::endl;
        for (auto const& [child, parent] : parent_map) {
            std::cout << "    " << child->getName() << " -> " << (parent ? parent->getName() : "nullptr") << std::endl;
        }
        std::cout << "  ------------------------" << std::endl;
    }


    // Phase 2: 计算半支配者 (sdom)
    // 对应用户代码的 for (int i = dfc; i >= 2; --i) 循环的上半部分
    // 按照 DFS 编号递减的顺序遍历所有节点 (除了 entryBlock，它的 DFS 编号是 0)
    if (DEBUG) std::cout << "--- Phase 2: Computing Semi-Dominators (sdom) ---" << std::endl;
    for (int i = df_counter - 1; i >= 1; --i) { // 从 DFS 编号最大的节点开始，到 1
        BasicBlock* w = vertex_vec[i]; // 当前处理的节点
        if (DEBUG) std::cout << "  Processing node w: " << w->getName() << " (dfnum=" << i << ")" << std::endl;


        // 对于 w 的每个前驱 v
        for (BasicBlock* v : w->getPredecessors()) {
            if (DEBUG) std::cout << "    Considering predecessor v: " << v->getName() << std::endl;
            // 如果前驱 v 未被 DFS 访问过 (即不在 dfnum_map 中)，则跳过
            if (dfnum_map.find(v) == dfnum_map.end()) {
                if (DEBUG) std::cout << "      Predecessor " << v->getName() << " not in DFS tree, skipping." << std::endl;
                continue; 
            }

            // 调用 evalAndCompress 来找到 v 在其 DFS 树祖先链上具有最小 sdom 的节点
            BasicBlock* u_with_min_sdom_on_path = evalAndCompress_lt_helper(v);
            if (DEBUG) std::cout << "      Eval(" << v->getName() << ") returned " 
                                  << u_with_min_sdom_on_path->getName() << std::endl;
            if (DEBUG && sdom_map.count(u_with_min_sdom_on_path) && sdom_map.count(w)) {
                std::cout << "      Comparing sdom: dfnum[" << sdom_map[u_with_min_sdom_on_path]->getName() << "] (" << dfnum_map[sdom_map[u_with_min_sdom_on_path]] 
                          << ") vs dfnum[" << sdom_map[w]->getName() << "] (" << dfnum_map[sdom_map[w]] << ")" << std::endl;
            }
            // 比较 sdom(u) 和 sdom(w)
            if (sdom_map.count(u_with_min_sdom_on_path) && sdom_map.count(w) &&
                dfnum_map[sdom_map[u_with_min_sdom_on_path]] < dfnum_map[sdom_map[w]]) {
                if (DEBUG) std::cout << "      Updating sdom[" << w->getName() << "] from " 
                                      << sdom_map[w]->getName() << " to " 
                                      << sdom_map[u_with_min_sdom_on_path]->getName() << std::endl;
                sdom_map[w] = sdom_map[u_with_min_sdom_on_path]; // 更新 sdom(w)
                if (DEBUG) std::cout << "      Sdom update applied. New sdom[" << w->getName() << "] = " << sdom_map[w]->getName() << std::endl;
            }
        }
        
        // 将 w 加入 sdom(w) 对应的桶中
        bucket_map[sdom_map[w]].push_back(w);
        if (DEBUG) std::cout << "    Adding " << w->getName() << " to bucket of sdom(" << w->getName() << "): " 
                              << sdom_map[w]->getName() << std::endl;

        // 将 w 的父节点加入并查集 (link 操作)
        if (parent_map.count(w) && parent_map[w] != nullptr) {
            link_lt_helper(parent_map[w], w);
        }
        
        // Phase 3-part 1: 处理 parent[w] 的桶中所有节点，确定部分 idom
        if (parent_map.count(w) && parent_map[w] != nullptr) {
            BasicBlock* p = parent_map[w]; // p 是 w 的父节点
            if (DEBUG) std::cout << "    Processing bucket for parent " << p->getName() << std::endl;

            // 注意：这里需要复制桶的内容，因为原始桶在循环中会被clear
            std::vector<BasicBlock*> nodes_in_p_bucket_copy = bucket_map[p];
            for (BasicBlock* y : nodes_in_p_bucket_copy) {
                if (DEBUG) std::cout << "      Processing node y from bucket: " << y->getName() << std::endl;
                // 找到 y 在其 DFS 树祖先链上具有最小 sdom 的节点
                BasicBlock* u = evalAndCompress_lt_helper(y);
                if (DEBUG) std::cout << "        Eval(" << y->getName() << ") returned " << u->getName() << std::endl;
                
                // 确定 idom(y)
                // if sdom(eval(y)) == sdom(parent(w)), then idom(y) = parent(w)
                // else idom(y) = eval(y)
                if (sdom_map.count(u) && sdom_map.count(p) &&
                    dfnum_map[sdom_map[u]] < dfnum_map[sdom_map[p]]) {
                    idom_map[y] = u; // 确定的 idom
                    if (DEBUG) std::cout << "        IDom[" << y->getName() << "] set to " << u->getName() << std::endl;
                } else {
                    idom_map[y] = p; // p 是 y 的 idom
                    if (DEBUG) std::cout << "        IDom[" << y->getName() << "] set to " << p->getName() << std::endl;
                }
            }
            bucket_map[p].clear(); // 清空桶，防止重复处理
            if (DEBUG) std::cout << "    Cleared bucket for parent " << p->getName() << std::endl;
        }
    }

    // Phase 3-part 2: 最终确定 idom (处理那些 idom != sdom 的节点)
    if (DEBUG) std::cout << "--- Phase 3: Finalizing Immediate Dominators (idom) ---" << std::endl;
    for (int i = 1; i < df_counter; ++i) { // 从 DFS 编号最小的节点 (除了 entryBlock) 开始
        BasicBlock* w = vertex_vec[i];
        if (DEBUG) std::cout << "  Finalizing node w: " << w->getName() << std::endl;
        if (idom_map.count(w) && sdom_map.count(w) && idom_map[w] != sdom_map[w]) {
            // idom[w] 的 idom 是其真正的 idom
            if (DEBUG) std::cout << "    idom[" << w->getName() << "] (" << idom_map[w]->getName() 
                                  << ") != sdom[" << w->getName() << "] (" << sdom_map[w]->getName() << ")" << std::endl;
            if (idom_map.count(idom_map[w])) {
                idom_map[w] = idom_map[idom_map[w]];
                if (DEBUG) std::cout << "    Updating idom[" << w->getName() << "] to idom(idom(w)): " 
                                      << idom_map[w]->getName() << std::endl;
            } else {
                 if (DEBUG) std::cout << "    Warning: idom(idom(" << w->getName() << ")) not found, leaving idom[" << w->getName() << "] as is." << std::endl;
            }
        }
        if (DEBUG) {
            std::cout << "  Final IDom[" << w->getName() << "] = " << (idom_map[w] ? idom_map[w]->getName() : "nullptr") << std::endl;
        }
    }

    // 将计算结果从 idom_map 存储到 DominatorTree 的成员变量 IDoms 中
    IDoms = idom_map; 

    if (DEBUG) std::cout << "--- Immediate Dominators Computation Finished ---" << std::endl;
}

// ==============================================================
// computeDominanceFrontiers 和 computeDominatorTreeChildren (保持不变)
// ==============================================================

void DominatorTree::computeDominanceFrontiers(Function *F) {
  if (DEBUG)
    std::cout << "--- Computing Dominance Frontiers ---" << std::endl;

  for (const auto &bb_ptr_X : F->getBasicBlocks()) {
    BasicBlock *X = bb_ptr_X.get();
    DominanceFrontiers[X].clear();

    for (const auto &bb_ptr_Z : F->getBasicBlocks()) {
      BasicBlock *Z = bb_ptr_Z.get();
      const std::set<BasicBlock *> *domsOfZ = getDominators(Z);

      if (!domsOfZ || domsOfZ->find(X) == domsOfZ->end()) { // Z 不被 X 支配
        continue;
      }

      for (BasicBlock *Y : Z->getSuccessors()) {
        const std::set<BasicBlock *> *domsOfY = getDominators(Y);
        // 如果 Y == X，或者 Y 不被 X 严格支配 (即 Y 不被 X 支配)
        if (Y == X || (domsOfY && domsOfY->find(X) == domsOfY->end())) {
          DominanceFrontiers[X].insert(Y);
        }
      }
    }
    if (DEBUG) {
      std::cout << "  DF(" << X->getName() << "): ";
      printBBSet("", DominanceFrontiers[X]);
    }
  }
  if (DEBUG)
    std::cout << "--- Dominance Frontiers Computation Finished ---" << std::endl;
}

void DominatorTree::computeDominatorTreeChildren(Function *F) {
  if (DEBUG)
    std::cout << "--- Computing Dominator Tree Children ---" << std::endl;
  // 首先清空，确保重新计算时是空的
  for (auto &bb_ptr : F->getBasicBlocks()) {
    DominatorTreeChildren[bb_ptr.get()].clear();
  }

  for (auto &bb_ptr : F->getBasicBlocks()) {
    BasicBlock *B = bb_ptr.get();
    BasicBlock *A = getImmediateDominator(B); // A 是 B 的即时支配者

    if (A) { // 如果 B 有即时支配者 A (即 B 不是入口块)
      DominatorTreeChildren[A].insert(B);
      if (DEBUG) {
        std::cout << "  " << B->getName() << " is child of " << A->getName() << std::endl;
      }
    }
  }
  if (DEBUG)
    std::cout << "--- Dominator Tree Children Computation Finished ---" << std::endl;
}

// ==============================================================
// DominatorTreeAnalysisPass 的实现 (保持不变)
// ==============================================================

bool DominatorTreeAnalysisPass::runOnFunction(Function *F, AnalysisManager &AM) {
  // 每次运行时清空旧数据，确保重新计算
  CurrentDominatorTree = std::make_unique<DominatorTree>(F);

  CurrentDominatorTree->computeDominators(F);
  CurrentDominatorTree->computeIDoms(F); // 修正后的LT算法
  CurrentDominatorTree->computeDominanceFrontiers(F);
  CurrentDominatorTree->computeDominatorTreeChildren(F);
  return false;
}

std::unique_ptr<AnalysisResultBase> DominatorTreeAnalysisPass::getResult() {
  return std::move(CurrentDominatorTree);
}

} // namespace sysy