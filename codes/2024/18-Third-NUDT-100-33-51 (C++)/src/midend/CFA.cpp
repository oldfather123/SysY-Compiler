#include "../include/midend/CFA.h"

/**
 * @file CFA.cpp
 *
 * 定义了CFA的源文件
 */

namespace sysy {
/**
 * @brief 计算必经结点，即支配关系
 *
 * 无参数也无返回值，但会设置块的必经结点集合属性
 *
 * @param [in] void
 * @return 无返回值
 */
auto CFA::computeDom() -> void {
  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    auto func = function.second.get();
    auto basicBlocks = func->getBasicBlocks();
    std::unordered_set<BasicBlock *> N;
    // 一开始把N置为所有block，再暂时把除entry外所有block的必经结点记为N
    auto entry_block = func->getEntryBlock();
    entry_block->setName("Entry");
    entry_block->addDominants(entry_block);
    for (auto &basicBlock : basicBlocks) {
      N.emplace(basicBlock.get());
    }
    for (auto &basicBlock : basicBlocks) {
      if (basicBlock.get() != entry_block) {
        basicBlock->setDominants(N);
      }
    }
    bool changed = true;
    while (changed) {
      changed = false;
      // 循环非start结点
      for (auto &basicBlock : basicBlocks) {
        if (basicBlock.get() != entry_block) {
          auto olddom = basicBlock->getDominants();
          std::unordered_set<BasicBlock *> dom = basicBlock->getPredecessors().front()->getDominants();
          for (auto pred : basicBlock->getPredecessors()) {
            intersect(dom, pred->getDominants());
          }
          dom.emplace(basicBlock.get());
          basicBlock->setDominants(dom);
          if (dom != olddom) {
            changed = true;
          }
        }
      }
    }
  }
}

/**
 * @brief 隐式构造支配树，在支配前驱和支配后继中体现
 *
 * 无参数也无返回值，但会设置块的idom和sdoms属性
 *
 * @param [in] void
 * @return 无返回值
 */
auto CFA::computeDomTree() -> void {
  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    auto func = function.second.get();
    auto basicBlocks = func->getBasicBlocks();
    for (const auto &basicBlock : basicBlocks) {
      // 先将所有结点的idom置为nullptr，sdoms置为空
      basicBlock->setIdom(nullptr);
      basicBlock->clearSdoms();
    }
    for (auto &basicBlock : basicBlocks) {
      // 对于非入口结点，入口结点本身就是根，无直接支配结点
      if (basicBlock.get() != func->getEntryBlock()) {
        auto dominats = basicBlock->getDominants();
        bool found = false;
        std::queue<BasicBlock *> q;
        for (auto pred : basicBlock->getPredecessors()) {
          q.push(pred);
        }
        while (!found && !q.empty()) {
          auto curr = q.front();
          q.pop();
          // 防止循环的情况又碰到了自己
          if (curr == basicBlock.get()) {
            continue;
          }
          if (dominats.count(curr) != 0U) {
            basicBlock->setIdom(curr);
            curr->addSdoms(basicBlock.get());
            found = true;
          } else {
            for (auto pred : curr->getPredecessors()) {
              q.push(pred);
            }
          }
        }
      }
    }
  }
}

/**
 * @brief 计算单个block的支配边界
 *
 * 分为local和up两部分，一种自下而上的方法，参数为单个block，递归实现
 *
 * @param [in] block 一个基本块
 * @return 返回该基本块的支配边界集合
 */
auto CFA::computeDF(BasicBlock *block) -> std::unordered_set<BasicBlock *> {
  std::unordered_set<BasicBlock *> ret_list;
  // 计算 localDF
  for (auto local_successor : block->getSuccessors()) {
    if (local_successor->getIdom() != block) {
      ret_list.emplace(local_successor);
    }
  }
  // 计算 upDF
  for (auto up_successor : block->getSdoms()) {
    auto childrenDF = computeDF(up_successor);
    for (auto w : childrenDF) {
      if (block != w->getIdom() || block == w) {
        ret_list.emplace(w);
      }
    }
  }

  return ret_list;
}

/**
 * @brief 预先计算所有基本块的支配边界
 *
 * 无参数也无返回值，但会设置基本块的支配边界集合属性
 *
 * @param [in] void
 * @return 无返回值
 */
auto CFA::computeDFAll() -> void {
  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    auto func = function.second.get();
    auto basicBlocks = func->getBasicBlocks();
    for (auto &basicBlock : basicBlocks) {
      auto df = computeDF(basicBlock.get());
      basicBlock->setDFs(df);
    }
  }
}

/**
 * @brief CFA，对外的接口
 *
 * 控制流分析
 *
 * @param [in] void
 * @return 无返回值
 */
auto CFA::run() -> void {
  // 计算所有块的必经结点集合
  computeDom();
  // 构造支配树，在支配前驱和后继中体现
  computeDomTree();
  // 计算所有块的支配边界
  computeDFAll();
}

/**
 * @brief 交集，dom = dom & other
 *
 * 没有返回值，但是会将dom和other的交集赋值给dom
 *
 * @param [in] dom 一个基本块集合
 * @param [in] other 另一个基本块集合
 * @return 无返回值
 */
auto CFA::intersect(std::unordered_set<BasicBlock *> &dom, const std::unordered_set<BasicBlock *> &other) -> void {
  if (dom.empty()) {
    return;
  }
  std::unordered_set<BasicBlock *> temp;
  for (auto elem : dom) {
    if (other.count(elem) > 0U) {
      temp.insert(elem);
    }
  }
  dom = std::move(temp);
}
}  // namespace sysy
