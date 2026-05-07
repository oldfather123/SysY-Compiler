#ifndef AAAC_FRONTEND_ADT_CFG_H
#define AAAC_FRONTEND_ADT_CFG_H

#include "AST/type.h"
#include "common.h"
#include "frontend.h"
#include "sym.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <set>

namespace aaac {
namespace frontend {

class BasicBlock;

/**
 * @brief DFS树的相关性质，只可以由ControlFlowGraph类访问
 */
class DFSInfo : public utils::BaseProperty, public DebugDumpImpl {
private:
  friend class ControlFlowGraph;
  friend class DominanceInfo;
  unsigned int order;
  std::weak_ptr<BasicBlock> parent;
  std::vector<std::weak_ptr<BasicBlock>> children;

public:
  std::ostream &dump(std::ostream &os) const override;
};

/**
 * @brief 某一个BasicBlock的Dominance信息
 */
class DominanceInfo : public utils::BaseProperty, public DebugDumpImpl {
public:
  unsigned int level;
  std::weak_ptr<BasicBlock> semidom;
  std::weak_ptr<BasicBlock> idom;
  std::weak_ptr<BasicBlock> best;
  std::vector<std::weak_ptr<BasicBlock>> children;

public:
  std::ostream &dump(std::ostream &os) const override;
};

/**
 * @brief 某一个基本块的活跃变量信息
 */
class LivenessInfo {
private:
  friend class ControlFlowGraph;
  std::set<sym::Var *> in;
  std::set<sym::Var *> out;

public:
    LivenessInfo() : in(), out(){};
    const std::set<sym::Var*>& getIn() const { return in; }
    const std::set<sym::Var*>& getOut() const { return out; }
};

/**
 * @brief 某个基本块的Use-Def信息
 */
struct UseDefInfo {
  std::set<sym::Var *> use;
  std::set<sym::Var *> def;

  UseDefInfo() : use(), def(){};
};

/**
 * @brief 用Node IR构建CFG时使用的基本块
 *
 * 同时，可以将其看作Node的容器，它同样支持for..in遍历其中的Nodes
 */
class BasicBlock : public std::enable_shared_from_this<BasicBlock>,
                   public DebugDumpImpl {
  friend class ControlFlowGraph;

public:
  enum EdgeFlag : uint8_t {
    ILLEGAL,
    PREV, // 标记当前边仅用于记录前驱块
    JUMP,
    THEN,
    ELSE,
  };
  class Edge {
  private:
    std::weak_ptr<BasicBlock> bb;
    EdgeFlag flag;

  public:
    std::shared_ptr<BasicBlock> getDest() const { return bb.lock(); }
    EdgeFlag getFlag() const { return flag; }
    Edge(std::shared_ptr<BasicBlock> bb, EdgeFlag ef) : bb(bb), flag(ef){};
    Edge() = default;
    ~Edge() = default;
    bool operator==(const Edge &other) const {
      return bb.lock() == other.bb.lock() && flag == other.flag;
    }
    Edge(const Edge &other) = default;
  };
  using EdgeList = std::vector<Edge>;

private:
  static inline std::unordered_set<std::type_index> allowed_types_ = {
      std::type_index(typeid(DFSInfo)), std::type_index(typeid(DominanceInfo))};
  EdgeList preds;
  EdgeList succs;

  InsnNodeList nodes;

  // /**
  //  * @brief 用于快速返回一个shared_ptr的vector
  //  */
  // std::vector<std::shared_ptr<BasicBlock>> lockAll(const
  // std::vector<std::weak_ptr<BasicBlock>> &vec)const;

public:
  using iterator = InsnNodeList::iterator;
  using const_iterator = InsnNodeList::const_iterator;
  BasicBlock() : preds(), succs(), nodes() {
    preds.reserve(config::RESERVE_BASIC_BLOCK_PREDS);
    succs.reserve(config::RESERVE_BASIC_BLOCK_SUCCS);
  }

  std::shared_ptr<InsnNode> front() const { return nodes.front(); }
  std::shared_ptr<InsnNode> back() const { return nodes.back(); }

  iterator begin() { return nodes.begin(); }
  iterator end() { return nodes.end(); }
  const_iterator begin() const { return nodes.begin(); }
  const_iterator end() const { return nodes.end(); }
  // 插入节点到指定位置前
  InsnNodeList::iterator insertNodeBefore(InsnNodeList::iterator pos,
                                          std::shared_ptr<InsnNode> node) {
    node->setParent(shared_from_this());
    return nodes.insert(pos, node);
  }

  // 插入节点到指定位置后
  InsnNodeList::iterator insertNodeAfter(InsnNodeList::iterator pos,
                                         std::shared_ptr<InsnNode> node) {
    node->setParent(shared_from_this());
    return nodes.insert(std::next(pos), node);
  }

  InsnNodeList::iterator getLastLabel() {
    auto label = nodes.begin();
    while (label != nodes.end() && (*label)->isLabel()) {
      label ++;
    }
    return --label;
  }
  // 如果不存在phi节点，那么返回最后一个label的迭代器
  InsnNodeList::iterator getLastPhi() {
    auto phi = std::next(getLastLabel());
    while (phi != nodes.end() && (*phi)->getCode() == NodeCode::Phi) {
      phi ++;
    }
    return --phi;
  }

  // 删除指定位置的节点
  InsnNodeList::iterator removeNode(InsnNodeList::iterator pos) {
    return nodes.erase(pos);
  }
  void push_front(std::shared_ptr<InsnNode> node) { node->setParent(shared_from_this());nodes.push_front(node); }
  void push_back(std::shared_ptr<InsnNode> node) { node->setParent(shared_from_this());nodes.push_back(node); }

  bool isEmpty() {
    auto it = std::find_if(nodes.begin(), nodes.end(),
    [](std::shared_ptr<InsnNode> node) {
      return !node->isCTI() && !node->isLabel();
    });
    return it == nodes.end();
  }
  void clearNodes() {
    for (auto it = nodes.begin(); it != nodes.end();) {
      auto node = *it;
      if (node->isCTI() || node->isLabel()) {
        it++;
        continue;
      }
      it = nodes.erase(it);
    }
  }
  void replaceFromInPhi(std::shared_ptr<BasicBlock> oldFrom, std::shared_ptr<BasicBlock> newFrom) {
    for (auto node : nodes) {
      if (node->getCode() == NodeCode::Phi) {
        auto phi = std::static_pointer_cast<PhiNode>(node);
        std::vector<Operand> buf;
        buf.reserve(8);
        for (auto &[from, var]: phi->getOperands()) {
          if (from == oldFrom.get()) {
            buf.push_back(var);
          }
        }
        phi->removeOperandByBasicBlock(oldFrom.get());
        for(auto var: buf) {
          phi->addOperand(var, newFrom);
        }
      }
    }
  }
  void removeFromInPhi(std::shared_ptr<BasicBlock> bb) {
    removeFromInPhi(bb.get());
  }
  void removeFromInPhi(BasicBlock *bb) {
    for (auto node: nodes) {
      if (node->getCode() == NodeCode::Phi) {
        auto phi = std::static_pointer_cast<PhiNode>(node);
        phi->removeOperandByBasicBlock(bb);
      }
    }
  }

  /**
   * @brief 连接两个BasicBlock,将当前BasicBlock作为传入的BasicBlock的前驱块
   */
  void connect(std::shared_ptr<BasicBlock> succ, EdgeFlag flag) {
    this->succs.emplace_back(succ, flag);
    succ->preds.emplace_back(shared_from_this(), PREV);
  }
  /**
   * @brief 相反，删除两者之间的连接，如果删除失败，则返回false
   */
  bool disconnect(std::shared_ptr<BasicBlock> succBB) {
    auto succ_it =
        std::find_if(succs.begin(), succs.end(), [&](const Edge &edge) {
          return edge.getDest() == succBB;
        });
    if (succ_it == succs.end())
      return false;
    auto this_it = std::find_if(
        succBB->preds.begin(), succBB->preds.end(),
        [&](const Edge &edge) { return edge.getDest() == shared_from_this(); });
    Assert(this_it != succBB->preds.end(),
           "BasicBlock::disconnect: succBB not found in preds.");
    succBB->preds.erase(this_it);
    succs.erase(succ_it);
    return true;
  }
  EdgeFlag getEdgeFlag(std::shared_ptr<BasicBlock> succBB) {
    auto succ_it = std::find_if(succs.begin(), succs.end(),
      [&](const Edge &edge) {
        return edge.getDest() == succBB;
      }
    );
    if (succ_it == succs.end()) {
      return ILLEGAL;
    }
    return succ_it->getFlag();
  }

  const EdgeList &getPredEdges() const { return preds; }
  const EdgeList &getSuccEdges() const { return succs; }
  EdgeList &getPredEdges() { return preds; }
  EdgeList &getSuccEdges() { return succs; }
  Edge getPredEdge(size_t idx = 0) const { return preds[idx]; }
  Edge getSuccEdge(size_t idx = 0) const { return succs[idx]; }
  std::ostream &dump(std::ostream &os) const override;
  DFSInfo &getDFSInfo() { return utils::g_property_mgr.init<DFSInfo>(this); }
  const DFSInfo *getDFSInfo() const {
    return utils::g_property_mgr.get<DFSInfo>(this);
  }
  DominanceInfo &getDominanceInfo() {
    return utils::g_property_mgr.init<DominanceInfo>(this);
  }
  const DominanceInfo *getDominanceInfo() const {
    return utils::g_property_mgr.get<DominanceInfo>(this);
  }
  std::shared_ptr<LabelNode> getLabel() const {
    auto label = std::dynamic_pointer_cast<LabelNode>(this->nodes.front());
    Assert(label, "Bad basic block");
    return label;
  }
};

template <typename T> using BBProp = std::unordered_map<BasicBlock *, T>;
/**
 * @brief 表示一个函数的控制流图（Control Flow Graph）
 *
 * 将符号表中的函数和程序控制流图整合起来，便于后续的SSA和优化等操作
 */
class ControlFlowGraph : public DebugDumpImpl {
public:
  using TraverseHandler = std::function<void(std::shared_ptr<sym::Function>,
                                             std::shared_ptr<BasicBlock>)>;
  using WorklistHandler = std::function<void(
      std::shared_ptr<sym::Function>, std::shared_ptr<BasicBlock>,
      std::vector<std::shared_ptr<BasicBlock>> &)>;

  static inline const TraverseHandler dummyHandler =
      [](std::shared_ptr<sym::Function>, std::shared_ptr<BasicBlock>) {};
  struct BBCompare;

private:
  std::weak_ptr<sym::Function> function;
  std::weak_ptr<BasicBlock> start_bb;
  std::weak_ptr<BasicBlock> exit_bb;
  std::vector<std::shared_ptr<BasicBlock>> bbs;
  sym::VarList locals;
  std::unordered_map<frontend::InsnNode *, frontend::BasicBlock *> InsnToBlock;

  std::unordered_map<std::shared_ptr<BasicBlock>, std::shared_ptr<backend::BasicBlock>> fe_to_be;

  void doForwardTraverse(std::shared_ptr<BasicBlock> bb,
                         std::unordered_set<BasicBlock *> &visited,
                         const TraverseHandler &preorder,
                         const TraverseHandler &postorder);
  void doBackwardTraverse(std::shared_ptr<BasicBlock> bb,
                          std::unordered_set<BasicBlock *> &visited,
                          const TraverseHandler &preorder,
                          const TraverseHandler &postorder);
  std::shared_ptr<BasicBlock>
  ancestorWithLowestSemiDom(std::shared_ptr<BasicBlock> pred,
                            std::shared_ptr<BasicBlock> current);
  bool isStrictlyDominate(std::shared_ptr<BasicBlock> D,
                          std::shared_ptr<BasicBlock> X);
  std::shared_ptr<sym::Var>
  createNewVarWithVersion(std::shared_ptr<sym::Var> var, unsigned int &version);
  std::unordered_set<BasicBlock *>
  getLiveBlocksForVar(const BBProp<LivenessInfo> &liveness, sym::Var *var);

public:
  ControlFlowGraph(std::shared_ptr<sym::Function> function,
                   const sym::VarList &decls)
      : function(function), start_bb({}), exit_bb({}), bbs(), locals(decls) {
    bbs.reserve(config::RESERVE_CFG_BASIC_BLOCKS);
  }

  std::unordered_map<std::shared_ptr<BasicBlock>, std::shared_ptr<backend::BasicBlock>> &
  getBBMap() { return fe_to_be; }

  void setStartBasicBlock(std::shared_ptr<BasicBlock> start_bb) {
    this->start_bb = start_bb;
  }
  void setExitBasicBlock(std::shared_ptr<BasicBlock> exit_bb) {
    this->exit_bb = exit_bb;
  }
  std::shared_ptr<BasicBlock> getStartBasicBlock() const {
    return this->start_bb.lock();
  }
  std::shared_ptr<BasicBlock> getExitBasicBlock() const {
    return this->exit_bb.lock();
  }
  bool isDominate(BasicBlock *D, BasicBlock *X);
  bool hasStartBasicBlock() const { return this->start_bb.use_count() != 0; }
  bool hasExitBasicBlock() const { return this->exit_bb.use_count() != 0; }
  std::shared_ptr<BasicBlock> createBasicBlock() {
    auto new_bb = std::make_shared<BasicBlock>();
    bbs.push_back(new_bb);
    return new_bb;
  }

  using bb_iterator = std::vector<std::shared_ptr<BasicBlock>>::iterator;
  using bb_const_iterator = std::vector<std::shared_ptr<BasicBlock>>::const_iterator;
  void addBasicBlock(std::shared_ptr<BasicBlock> bb) { bbs.push_back(bb); }
  bb_iterator removeBasicBlock(bb_iterator it) { return bbs.erase(it); }
  bb_iterator removeBasicBlock(std::shared_ptr<BasicBlock> bb) {
    auto it = std::find(bbs.begin(),bbs.end(),bb);
    if (it == bbs.end()) {
      return it;
    }
    return bbs.erase(it);
  }
  const std::vector<std::shared_ptr<BasicBlock>> &getBBList() const {
    return this->bbs;
  }
  std::vector<std::shared_ptr<BasicBlock>> &getBBList() {
    return this->bbs;
  }

  void forwardTraverse(const TraverseHandler &preorder,
                       const TraverseHandler &postorder) {
    forwardTraverse(this->start_bb.lock(), preorder, postorder);
  };
  void backwardTraverse(const TraverseHandler &preorder,
                        const TraverseHandler &postorder) {
    backwardTraverse(this->exit_bb.lock(), preorder, postorder);
  };
  void forwardTraverse(std::shared_ptr<BasicBlock> bb,
                       const TraverseHandler &preorder,
                       const TraverseHandler &postorder);
  void backwardTraverse(std::shared_ptr<BasicBlock> bb,
                        const TraverseHandler &preorder,
                        const TraverseHandler &postorder);
  void backwardWorklist(const WorklistHandler &handler) {
    backwardWorklist(this->start_bb.lock(),this->exit_bb.lock(), handler);
  }
  void backwardWorklist(std::shared_ptr<BasicBlock> start_bb,std::shared_ptr<BasicBlock> exit_bb,
                        const WorklistHandler &handler);
  std::vector<std::weak_ptr<BasicBlock>>
  findAllUses(const BBProp<UseDefInfo> &useDef, std::shared_ptr<sym::Var> var);
  std::vector<std::weak_ptr<BasicBlock>>
  findAllDefs(const BBProp<UseDefInfo> &useDef, std::shared_ptr<sym::Var> var);
  std::vector<std::weak_ptr<BasicBlock>> buildDFSTree();
  void buildDomTree();
  void setDomTreeLevel();
  BBProp<LivenessInfo> calculateLiveness(const BBProp<UseDefInfo> &useDef);
  std::vector<std::weak_ptr<BasicBlock>>
  calculateIDF(const std::vector<std::weak_ptr<BasicBlock>> &defBlocks,
               const std::unordered_set<BasicBlock *> &liveInBlocks);
  BBProp<UseDefInfo> calculateUseDef();
  void insertPhiNode(
      const std::vector<std::weak_ptr<BasicBlock>> &idf,
      std::shared_ptr<sym::Var> var,
      std::unordered_map<PhiNode *, std::shared_ptr<sym::Var>> &phiToIndex,
      std::unordered_set<BasicBlock *> &liveins);
  void renameUsesNotPhi(
      std::shared_ptr<InsnNode> node,
      std::unordered_map<sym::Var *, std::shared_ptr<sym::Var>> &stack,
      std::unordered_map<sym::Var *, unsigned int> &version);
  void
  renameDefs(std::shared_ptr<InsnNode> node,
             std::unordered_map<sym::Var *, std::shared_ptr<sym::Var>> &stack,
             std::unordered_map<sym::Var *, unsigned int> &version);
  void buildSSA();
  void eliminatePhi();

  bool isParam(std::shared_ptr<sym::Var> var) {
    auto func_param = this->function.lock()->getParameters();
    return std::find(func_param.begin(), func_param.end(), var) !=
           func_param.end();
  }
  bool isLocal(std::shared_ptr<sym::Var> var) {
    return std::find(locals.begin(), locals.end(), var) != locals.end();
  }
  const sym::VarList &getLocals() const { return this->locals; }
  std::shared_ptr<sym::Function> getFunction() const {
    return this->function.lock();
  }
  std::ostream &dump(std::ostream &os) const override;

  std::unordered_map<sym::Var *, std::vector<frontend::InsnNode *>>
  getUseChain() const;
};
} // namespace frontend
} // namespace aaac

namespace std {
template <> struct hash<aaac::frontend::BasicBlock::Edge> {
  size_t operator()(const aaac::frontend::BasicBlock::Edge &edge) const {
    return hash<void *>{}(edge.getDest().get()) ^
           hash<int>{}(static_cast<int>(edge.getFlag()));
  }
};
} // namespace std

#endif
