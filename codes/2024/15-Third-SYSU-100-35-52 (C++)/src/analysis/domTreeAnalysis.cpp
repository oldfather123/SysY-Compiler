/*
    SPDX-License-Identifier: Apache-2.0
    Copyright 2023 CMMC Authors
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/





#include "Block.h"
#include "Function.h"
#include "domTreeAnalysis.h"

#include "CFGAnalysis.h"
#include <utility>
#define YATCC_UNUSED(x) (void)(x)

// NOLINTBEGIN

DominateAnalysisResult::DominateAnalysisResult(std::unordered_map<BasicBlockPtr, DomTreeNode::NodeIndex> invMap,
                                              std::vector<DomTreeNode> domTree)
   : mDomTreeInvMap{ std::move(invMap) }, mDomTree{ std::move(domTree) } {
   mOrder.reserve(mDomTree.size());
   for(auto& node : mDomTree)
       mOrder.push_back(node.block);

   mReversedOrder.reserve(mDomTree.size());
   std::reverse_copy(mOrder.cbegin(), mOrder.cend(), std::back_inserter(mReversedOrder));
}

BasicBlockPtr DominateAnalysisResult::parent(BasicBlockPtr node) const {
   const auto idx = getIndex(std::move(node));
   if(idx == DomTreeNode::invalidNode)
       return nullptr;
   const auto parentidx = mDomTree[idx].parent();
   if(parentidx == DomTreeNode::invalidNode)
       return nullptr;
   return mDomTree[parentidx].block;
}

DomTreeNode::NodeIndex DominateAnalysisResult::getIndex(BasicBlockPtr block) const {
   const auto iter = mDomTreeInvMap.find(block);
   if(iter != mDomTreeInvMap.cend())
       return iter->second;
   return DomTreeNode::invalidNode;
}

bool DominateAnalysisResult::reachable(BasicBlockPtr block) const {
   return mDomTreeInvMap.count(block);
}

DomTreeNode::NodeIndex DominateAnalysisResult::subTreeSize(BasicBlockPtr node) const {
   const auto iter = mDomTreeInvMap.find(node);
   if(iter != mDomTreeInvMap.cend())
       return mDomTree[iter->second].size;
   return 0;
}

BasicBlockPtr DominateAnalysisResult::lca(BasicBlockPtr a, BasicBlockPtr b) const {
   const auto lhs = mDomTreeInvMap.find(a);
   const auto rhs = mDomTreeInvMap.find(b);
   if(lhs == mDomTreeInvMap.cend() || rhs == mDomTreeInvMap.cend())
       return nullptr;
   auto u = lhs->second, v = rhs->second;
   if(mDomTree[u].depth > mDomTree[v].depth)
       std::swap(u, v);
   auto diff = mDomTree[v].depth - mDomTree[u].depth;
   for(uint32_t i = 0; diff; ++i, diff >>= 1)
       if(diff & 1)
           v = mDomTree[v].ancestor[i];
   if(u != v) {
       for(int32_t i = DomTreeNode::maxDepth; i >= 0 && u != v; --i)
           if(auto pu = mDomTree[u].ancestor[static_cast<uint32_t>(i)], pv = mDomTree[v].ancestor[static_cast<uint32_t>(i)];
              pu != pv)
               u = pu, v = pv;
       u = mDomTree[u].parent();
   }

   return mDomTree[u].block;
}

bool DominateAnalysisResult::dominate(BasicBlockPtr a, BasicBlockPtr b) const {
   const auto lhs = mDomTreeInvMap.find(a);
   const auto rhs = mDomTreeInvMap.find(b);
   if(lhs == mDomTreeInvMap.cend() || rhs == mDomTreeInvMap.cend())
       return false;
   const auto u = lhs->second, v = rhs->second;
   return u <= v && v < u + mDomTree[u].size;
}

std::vector<BasicBlockPtr> DominateAnalysisResult::allDoms(BasicBlockPtr node) const{
    std::vector<BasicBlockPtr> ret;
    const auto idx = getIndex(node);
    if(idx == DomTreeNode::invalidNode)
         return ret;
    for(auto i = idx; i != DomTreeNode::invalidNode; i = mDomTree[i].parent())
         ret.push_back(mDomTree[i].block);
    return ret;
}

DominateAnalysisResult runDominateTreeAnalysis(FunctionPtr func, CFGAnalysisResult cfg) {  // NOLINT
   BasicBlockPtr entryBlk = func->getEntryBlock();

   DomTreeNode::NodeIndex cnt = 0;
   // e 后继  g 前驱 tree 父子关系
   std::unordered_map<BasicBlockPtr, std::unordered_set<BasicBlockPtr>> e, g, tree;
   // 父亲节点
   std::unordered_map<BasicBlockPtr, BasicBlockPtr> fa;
   // 深度优先编号
   std::unordered_map<BasicBlockPtr, uint32_t> dfn;
   // 深度优先序列
   std::vector<BasicBlockPtr> dfseq;
   // 代表元 用于并查集
   std::unordered_map<BasicBlockPtr, BasicBlockPtr> f;

   // 深度优先搜索标记函数
   auto color = [&](auto&& self, BasicBlockPtr cur, BasicBlockPtr parent) {
       assert(cur != nullptr);
       if(dfn.count(cur))  // 当前块已标记，返回
           return;
       if(parent != nullptr)  // 有父节点，建立父子关系
           tree[parent].insert(cur), fa[cur] = parent;
       dfn[cur] = ++cnt;      // 为当前块 分配标号
       dfseq.push_back(cur);  // 将当前块加入序列
       for(auto succBlk : cfg.successors(cur)) {
           e[cur].insert(succBlk), g[succBlk].insert(cur);  // 建立前驱后继关系
           self(self, succBlk, cur);                        // 递归处理后继块
       }
   };
   // 从入口块开始标记
   color(color, entryBlk, nullptr);

   // ran 代表元用于半支配计算  idom 直接支配  sdom 半支配
   std::unordered_map<BasicBlockPtr, BasicBlockPtr> ran, idom, sdom;
   auto merge = [&](auto&& self, BasicBlockPtr cur) {
       if(f[cur] == cur)  // 如果当前块是其自身的代表，返回自身
           return cur;
       BasicBlockPtr res = self(self, f[cur]);           // 递归找到当前块的代表
       if(dfn[sdom[ran[f[cur]]]] < dfn[sdom[ran[cur]]])  // 比较半支配者
           ran[cur] = ran[f[cur]];
       return f[cur] = res;  // 压缩路径
   };

   // 为每个块初始化 存储中间结果 临时表
   std::unordered_map<BasicBlockPtr, std::vector<BasicBlockPtr>> tr;

   for(auto [tblk, dfno] : dfn) {
       YATCC_UNUSED(dfno);
       sdom[tblk] = tblk;
       f[tblk] = ran[tblk] = tblk;
   }
   // 计算每个块的半支配者
   for(auto it = dfseq.rbegin(); it != dfseq.rend(); it++) {
       BasicBlockPtr blk = *it;
       if(blk == entryBlk)  // 跳过入口块
           continue;
       for(BasicBlockPtr fablk : g[blk]) {
           if(!dfn.count(fablk))  // 跳过未标记的块
               continue;
           merge(merge, fablk);
           if(dfn[sdom[ran[fablk]]] < dfn[sdom[blk]])
               sdom[blk] = sdom[ran[fablk]];
       }
       f[blk] = fa[blk];
       tr[sdom[blk]].push_back(blk);
       blk = fa[blk];
       for(BasicBlockPtr tblk : tr[blk]) {
           merge(merge, tblk);
           if(blk == sdom[ran[tblk]])
               idom[tblk] = blk;
           else
               idom[tblk] = ran[tblk];
       }
       tr[blk].clear();
   }

   // 修正每个块的直接支配者
   for(BasicBlockPtr blk : dfseq) {
       if(blk == entryBlk)
           continue;
       if(idom[blk] != sdom[blk])
           idom[blk] = idom[idom[blk]];
   }

   // 构建支配树
   std::unordered_map<BasicBlockPtr, std::vector<BasicBlockPtr>> children;

   for(auto [u, v] : idom)
       children[v].push_back(u);

   std::unordered_map<BasicBlockPtr, DomTreeNode::NodeIndex> invMap;
   std::vector<DomTreeNode> domTree;
   domTree.resize(dfn.size());

   cnt = 0;
   constexpr auto invalidNode = DomTreeNode::invalidNode;

   auto dfs = [&](auto&& self, BasicBlockPtr curBlock, DomTreeNode::NodeIndex p) -> DomTreeNode::NodeIndex {
       const auto u = cnt++;
       invMap[curBlock] = u;
       auto& node = domTree[u];
       node.block = curBlock;
       node.depth = (p == invalidNode ? 0 : domTree[p].depth + 1);
       node.ancestor[0] = p;
       node.size = 1;

       for(uint32_t i = 1; i <= DomTreeNode::maxDepth; i++) {
           const auto v = domTree[u].ancestor[i - 1];
           domTree[u].ancestor[i] = (v == invalidNode ? invalidNode : domTree[v].ancestor[i - 1]);
       }

       if(auto iter = children.find(curBlock); iter != children.cend())
           for(auto v : iter->second) {
               const auto child = self(self, v, u);
               node.size += domTree[child].size;
           }
       return u;
   };

   // 从入口块开始深度优先遍历构建支配树
   dfs(dfs, entryBlk, invalidNode);

   /*
   func.dump(std::cerr);
   for(auto [block, set] : ret) {
       block->dumpAsTarget(std::cerr);
       std::cerr << ": "sv;
       for(auto b : set) {
           b->dumpAsTarget(std::cerr);
           std::cerr << ", "sv;
       }
       std::cerr << std::endl;
   }*/
   return DominateAnalysisResult{ std::move(invMap), std::move(domTree) };
}

DominanceFrontier computeDominanceFrontier(const DominateAnalysisResult& domResult, const CFGAnalysisResult& cfgResult) {
   DominanceFrontier frontier;

   // Initialize an empty frontier set for each block
   for(const auto& block : domResult.blocks()) {
       frontier[block] = std::unordered_set<BasicBlockPtr>();
   }

   // Compute the dominance frontier for each block
   for(const auto& X : domResult.blocks()) {
       // Get the predecessors of X from the CFG
       const auto& predecessors = cfgResult.predecessors(X);

       for(const auto& Y : domResult.blocks()) {
           if(X == Y)
               continue;
           // Check if X dominates a predecessor of Y
           bool dominatesPredecessor = false;
           for(const auto& pred : cfgResult.predecessors(Y)) {
               if(domResult.dominate(X, pred)) {
                   dominatesPredecessor = true;
                   break;
               }
           }
           // If X dominates a predecessor of Y and doesn't strictly dominate Y,
           // then Y is in the dominance frontier of X
           if(dominatesPredecessor && !domResult.dominate(X, Y)) {
               frontier[X].insert(Y);
           }
       }
   }

   return frontier;
}
// NOLINTEND