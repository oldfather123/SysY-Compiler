#include "ADT/CFG.h"
#include "common.h"
#include "frontend.h"
#include "sym.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <memory>
#include <queue>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace aaac {
namespace optimization {

enum ValueLatticeStatus { UNKNOWN, CONSTANT, VARIABLE };

struct ValueLattice {
  ValueLatticeStatus status;
  std::variant<int, float> value;
  ValueLattice()
      : status(ValueLatticeStatus::UNKNOWN), value(0) {}
  ValueLattice(ValueLatticeStatus status, int value)
      : status(status), value(value){};
  ValueLattice(ValueLatticeStatus status, float value)
      : status(status), value(value){};
};

class SCCPContext {
private:
  std::unordered_map<sym::Var *, ValueLattice> VarLattice;
  std::unordered_map<sym::Var *, frontend::Operand> VarOperand;
  std::queue<frontend::InsnNode *> InstructionWorklist;
  std::queue<frontend::BasicBlock *> EdgeWorklist;
  std::unordered_map<frontend::BasicBlock *, frontend::BasicBlock::Edge>
      EdgesNotExecutable;
  std::unordered_set<frontend::BasicBlock *> VisitedBlocks;
  std::unordered_map<sym::Var *, std::vector<frontend::InsnNode *>> UseChain;

  void init(frontend::ControlFlowGraph *cfg);
  void transform(frontend::ControlFlowGraph *cfg);
  bool worklistsNotEmpty();
  bool executeOnInstruction(frontend::ControlFlowGraph *cfg,frontend::InsnNode *insn);
  bool executeOnEdge(frontend::ControlFlowGraph *cfg,frontend::BasicBlock *bb);
  bool notExecutable(frontend::BasicBlock *pred, frontend::BasicBlock *succ);
  void rewriteConstantValues();
  void cleanDeadCode(frontend::ControlFlowGraph *cfg);
  void rewriteControlFlow(frontend::ControlFlowGraph *cfg);
  void addAllUsesToWorklist(sym::Var *var);
public:
  void SCCP(frontend::ControlFlowGraph *cfg);
  SCCPContext() = default;
};

// PhiNode中不适用，因为需要比较值
static ValueLatticeStatus calculateMeet(ValueLatticeStatus lhs,
                                        ValueLatticeStatus rhs) {
  if (lhs == VARIABLE || rhs == VARIABLE) {
    return VARIABLE;
  }

  if (lhs == UNKNOWN || rhs == UNKNOWN) {
    return UNKNOWN;
  }

  // 现在已知lhs和rhs都是CONSTANT了
  return CONSTANT;
}

// 这个函数用于计算Phi的会合
static ValueLattice calculatePhiMeet(ValueLattice lhs, ValueLattice rhs) {
  if (lhs.status == VARIABLE || rhs.status == VARIABLE) {
    return ValueLattice(VARIABLE, 0);
  }

  if (lhs.status == UNKNOWN)
    return rhs;

  if (rhs.status == UNKNOWN)
    return lhs;

  Assert(lhs.status == CONSTANT && rhs.status == CONSTANT, "wrong calculate.");

  if (lhs.value == rhs.value) {
    return lhs;
  } else {
    return ValueLattice(VARIABLE, 0);
  }
}

static std::variant<int, float> calculateLattice(frontend::OpCode code,
                                                 std::variant<int, float> op1,
                                                 std::variant<int, float> op2) {
  std::variant<int, float> value;
  switch (code) {
  case frontend::OpCode::Add:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs + rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Sub:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs - rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Mul:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs * rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Div:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs / rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Mod: {
    assert(std::holds_alternative<int>(op1));
    assert(std::holds_alternative<int>(op2));
    int res = std::get<int>(op1) % std::get<int>(op2);
    value = std::variant<int, float>(res);
    // floating point exception
    // value = std::visit(
    //     [](int lhs, int rhs) { return std::variant<int, float>(lhs % rhs); },
    //     op1, op2);
    break;
  }
  case frontend::OpCode::Assign:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs - rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Negate:
    value =
        std::visit([](auto op) { return std::variant<int, float>(-op); }, op1);
    break;
  case frontend::OpCode::LShift:
    value = std::visit(
        [](int lhs, int rhs) { return std::variant<int, float>(lhs << rhs); },
        op1, op2);
    break;
  case frontend::OpCode::RShift:
    value = std::visit(
        [](int lhs, int rhs) { return std::variant<int, float>(lhs >> rhs); },
        op1, op2);
    break;
  case frontend::OpCode::BitAnd:
    value = std::visit(
        [](int lhs, int rhs) { return std::variant<int, float>(lhs & rhs); },
        op1, op2);
    break;
  case frontend::OpCode::BitOr:
    value = std::visit(
        [](int lhs, int rhs) { return std::variant<int, float>(lhs | rhs); },
        op1, op2);
    break;
  case frontend::OpCode::BitXor:
    value = std::visit(
        [](int lhs, int rhs) { return std::variant<int, float>(lhs ^ rhs); },
        op1, op2);
    break;
  case frontend::OpCode::BitNot:
    value =
        std::visit([](int op) { return std::variant<int, float>(~op); }, op1);
    break;
  case frontend::OpCode::Interger_cst:
    value = std::visit(
        [](auto op) { return std::variant<int, float>(static_cast<int>(op)); },
        op1);
    break;
  case frontend::OpCode::Float_cst:
    value = std::visit(
        [](auto op) {
          return std::variant<int, float>(static_cast<float>(op));
        },
        op1);
    break;
  case frontend::OpCode::AddressOf:
    Assert(false, "Constant has no address");
    break;
  case frontend::OpCode::Eq:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs == rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Neq:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs != rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Leq:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs <= rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Lt:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs < rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Geq:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs >= rhs); },
        op1, op2);
    break;
  case frontend::OpCode::Gt:
    value = std::visit(
        [](auto lhs, auto rhs) { return std::variant<int, float>(lhs > rhs); },
        op1, op2);
    break;
  case frontend::OpCode::LogNot:
    value =
        std::visit([](auto op) { return std::variant<int, float>(!op); }, op1);
    break;
  }
  return value;
}

void SCCPContext::init(frontend::ControlFlowGraph *cfg) {
  auto function = cfg->getFunction();
  for (auto param : function->getParameters()) {
    this->VarLattice[param.get()] = ValueLattice(VARIABLE, 0);
  }
  for (auto bb : cfg->getBBList()) {
    for (auto insn : *bb) {
      for (auto def_var : insn->getDefs()) {
        this->VarOperand[def_var.get()] = frontend::Operand(def_var);
        this->VarLattice[def_var.get()] = ValueLattice();
      }
    }
  }
  this->UseChain=cfg->getUseChain();
  this->EdgeWorklist.push(cfg->getStartBasicBlock().get());
}

bool SCCPContext::executeOnInstruction(frontend::ControlFlowGraph *cfg,frontend::InsnNode *insn) {
  bool changed = false;
  // TODO:可达性似乎没检查
  switch (insn->getCode()) {
  case frontend::NodeCode::Assign: {
    auto assign_insn = dynamic_cast<frontend::AssignNode *>(insn);
    auto def = assign_insn->getDefs().front();
    auto &def_lattice = this->VarLattice[def.get()];
    std::vector<ValueLattice> op_lattice;
    ValueLatticeStatus new_status = UNKNOWN;
    for (auto op : assign_insn->getOperands()) {
      std::visit(
          [this, &op_lattice, &new_status](auto &&op) {
            using T = std::decay_t<decltype(op)>;
            if constexpr (std::is_same_v<T, std::shared_ptr<sym::Var>>) {
              op_lattice.push_back(this->VarLattice[op.get()]);
            } else {
              op_lattice.push_back(ValueLattice(CONSTANT, op));
            }
            // new_status = calculateMeet(new_status, op_lattice.back().status);
          },
          op);
    }

    new_status = op_lattice[0].status;
    for (size_t i = 1; i < op_lattice.size(); i++) {
      new_status = calculateMeet(new_status, op_lattice[i].status);
    }

    changed = (new_status != def_lattice.status);
    def_lattice.status = new_status;
    if (changed) {
      this->addAllUsesToWorklist(def.get());
    }

    if (def_lattice.status == CONSTANT) {
      auto op1 = op_lattice[0].value;
      decltype(op1) op2;
      if (op_lattice.size() > 1) {
        op2 = op_lattice[1].value;
      }
      def_lattice.value = calculateLattice(assign_insn->getOpCode(), op1, op2);
    }

    break;
  }
  // 跳转函数驱动EdgesWorklist
  case frontend::NodeCode::Condition: {
    auto condition_insn = dynamic_cast<frontend::ConditionNode *>(insn);
    std::vector<ValueLattice> op_lattice;
    ValueLatticeStatus condition_lattice_status = UNKNOWN;
    for (auto op : condition_insn->getOperands()) {
      std::visit(
          [this, &op_lattice, &condition_lattice_status](auto &&op) {
            using T = std::decay_t<decltype(op)>;
            if constexpr (std::is_same_v<T, std::shared_ptr<sym::Var>>) {
              op_lattice.push_back(this->VarLattice[op.get()]);
            } else {
              op_lattice.push_back(ValueLattice(CONSTANT, op));
            }
            // condition_lattice_status =
            // calculateMeet(condition_lattice_status,
            //                                          op_lattice.back().status);
          },
          op);
    }

    condition_lattice_status = op_lattice[0].status;
    for (size_t i = 1; i < op_lattice.size(); i++) {
      condition_lattice_status =
          calculateMeet(condition_lattice_status, op_lattice[i].status);
    }

    // Assert(condition_lattice_status != UNKNOWN,
    //        "Maybe undefined var in conditional statement");
    auto current_block = insn->getParent().get();
    auto true_branch = std::find_if(
        current_block->getSuccEdges().begin(),
        current_block->getSuccEdges().end(), [&](auto &edge) {
          return edge.getFlag() == frontend::BasicBlock::EdgeFlag::THEN;
        });
    auto false_branch = std::find_if(
        current_block->getSuccEdges().begin(),
        current_block->getSuccEdges().end(), [&](auto &edge) {
          return edge.getFlag() == frontend::BasicBlock::EdgeFlag::ELSE;
        });

    if (condition_lattice_status == CONSTANT) {
      auto op1 = op_lattice[0].value;
      decltype(op1) op2;
      if (op_lattice.size() > 1) {
        op2 = op_lattice[1].value;
      }
      auto condition_value = std::get<int>(
          calculateLattice(condition_insn->getOpCode(), op1, op2));
      if (condition_value) { // True
        this->EdgeWorklist.push(true_branch->getDest().get());
        this->EdgesNotExecutable[insn->getParent().get()] = *false_branch;
        std::cout << insn->getParent().get() << " -T-> "
                  << true_branch->getDest() << "\n";
      } else {
        this->EdgeWorklist.push(false_branch->getDest().get());
        this->EdgesNotExecutable[insn->getParent().get()] = *true_branch;
        std::cout << insn->getParent().get() << " -F-> "
                  << false_branch->getDest() << "\n";
      }
    } else {
      this->EdgeWorklist.push(true_branch->getDest().get());
      this->EdgeWorklist.push(false_branch->getDest().get());
      // 可能在第一次的时候是const，但是后续发现并不是const，此时需要去除EdgesNotExecutable
      this->EdgesNotExecutable.erase(insn->getParent().get());
      assert(this->EdgesNotExecutable.count(insn->getParent().get()) == 0);
      std::cout << insn->getParent().get() << " -B-> "
                << true_branch->getDest() << "\n";
      std::cout << insn->getParent().get() << " -B-> "
                << false_branch->getDest() << "\n";
    }

    break;
  }
  case frontend::NodeCode::Jump: {
    auto current_bb = insn->getParent().get();
    auto edge = current_bb->getSuccEdge();
    this->EdgeWorklist.push(edge.getDest().get());
    break;
  }
  case frontend::NodeCode::Phi: {
    auto phi_insn = dynamic_cast<frontend::PhiNode *>(insn);
    auto def = phi_insn->getDestVar();
    // 漏了引用？
    auto &def_lattice = this->VarLattice[def.get()];
    auto new_lattice = ValueLattice();
    for (auto [bb, var] : phi_insn->getOperands()) {
      if (VisitedBlocks.count(bb) == 0) {
        // 前驱未处理过，因此是UNKNOWN
        new_lattice = calculatePhiMeet(new_lattice, ValueLattice());
        continue;
      }
      // notExecutable中会对EdgesNotExecutable使用at，因此需要判一下count，防止报错
      // 同时，由于上面判过了是否处理过该基本快，因此此处如果count==0，这就表示这是一条可被执行的路径
      if (EdgesNotExecutable.count(bb) == 0 ||
          !notExecutable(bb, insn->getParent().get())) {
        auto phi_op_lattice = std::visit(
            [this](auto op) {
              using T = std::decay_t<decltype(op)>;
              if constexpr (std::is_same_v<T, int>) {
                return ValueLattice(CONSTANT, op);
              } else if constexpr (std::is_same_v<T, float>) {
                return ValueLattice(CONSTANT, op);
              } else if constexpr (std::is_same_v<T,
                                                  std::shared_ptr<sym::Var>>) {
                return this->VarLattice[op.get()];
              }
              return ValueLattice();
            },
            var);
        new_lattice = calculatePhiMeet(new_lattice, phi_op_lattice);
      }
    }

    changed = (new_lattice.status != def_lattice.status);
    def_lattice = new_lattice;
    if (changed) {
      this->addAllUsesToWorklist(phi_insn->getDestVar().get());
    }

    // TODO: 向CFGWorklist加入

    break;
  }
  // 这些暂且认为都是无法在编译时推定常数的，或者它们压根没有def且与控制流无关
  case frontend::NodeCode::Base:
  case frontend::NodeCode::Load:
  case frontend::NodeCode::Store:
  case frontend::NodeCode::Call:
  case frontend::NodeCode::Label:
  case frontend::NodeCode::Return:
  case frontend::NodeCode::Nop: {
    for (auto defs : insn->getDefs()) {
      this->VarLattice[defs.get()].status = VARIABLE;
    }
    break;
  }
  }
  return changed;
}

bool SCCPContext::executeOnEdge(frontend::ControlFlowGraph *cfg,frontend::BasicBlock *bb) {
  bool changed = false;
  for (auto node : *bb) {
    if (node->getCode() == frontend::NodeCode::Phi)
      changed |= this->executeOnInstruction(cfg,node.get());
    else {
      if (this->VisitedBlocks.count(bb) == 0) {
        changed |= this->executeOnInstruction(cfg,node.get());
      }
    }
  }
  this->VisitedBlocks.insert(bb);
  return changed;
}

bool SCCPContext::worklistsNotEmpty() {
  return this->EdgeWorklist.size() > 0 || this->InstructionWorklist.size() > 0;
}

bool SCCPContext::notExecutable(frontend::BasicBlock *pred,
                                frontend::BasicBlock *succ) {
  return this->EdgesNotExecutable.at(pred).getDest().get() == succ;
}

void SCCPContext::rewriteConstantValues() {
  std::set<sym::Var *> phi;
  for (auto [var, uses_list] : this->UseChain) {
    if (this->VarLattice[var].status == ValueLatticeStatus::CONSTANT) {
      frontend::Operand constant_value;
      std::visit([&constant_value](
                     auto value) { constant_value = frontend::Operand(value); },
                 this->VarLattice[var].value);
      for (auto use_insn : uses_list) {
        use_insn->replaceUsesWith(this->VarOperand[var], constant_value);
      }
    }
  }
}

void SCCPContext::cleanDeadCode(frontend::ControlFlowGraph *cfg) {
  frontend::ControlFlowGraph::TraverseHandler erase_constant_defs =
      [this](std::shared_ptr<sym::Function> func,
             std::shared_ptr<frontend::BasicBlock> bb) {
        for (auto beg = bb->begin(); beg != bb->end(); ++beg) {
          if ((*beg)->getDefs().size() == 0)
            continue;
          auto def =
              (*beg)->getDefs().front(); // 暂时应该没有def大于1的情况出现
          if (this->VarLattice[def.get()].status ==
              ValueLatticeStatus::CONSTANT) {
            beg = bb->removeNode(beg);
            --beg;
          }
        }
      };

  cfg->forwardTraverse(erase_constant_defs,
                       frontend::ControlFlowGraph::dummyHandler);
}

void SCCPContext::rewriteControlFlow(frontend::ControlFlowGraph *cfg) {
  // 把条件跳转转换成直接跳转，删除不可达的边
  for (auto [bb, dead_edge] : this->EdgesNotExecutable) {
    bb->disconnect(dead_edge.getDest());

    auto direct_jump = bb->getSuccEdge();
    // 修改为JUMP（原先为THEN或ELSE）
    bb->disconnect(direct_jump.getDest());
    auto condition_node = bb->end();
    bb->removeNode(--condition_node);
    bb->connect(direct_jump.getDest(), frontend::BasicBlock::JUMP);
    bb->push_back(frontend::JumpNode::create(
        std::dynamic_pointer_cast<frontend::LabelNode>(
            direct_jump.getDest()->front())));
    dead_edge.getDest()->removeFromInPhi(bb);
    // TODO:检查下一个Block的Phi函数有没有和这条边相关联的参数？
  }
  // 顺便删一下DeadBasicBlock
  // TODO
  for (auto bb : cfg->getBBList()) {
    if (VisitedBlocks.count(bb.get())) {
      continue;
    }
    bb->clearNodes();
  }
}

void SCCPContext::transform(frontend::ControlFlowGraph *cfg) {
  this->rewriteConstantValues();
  this->cleanDeadCode(cfg);
  this->rewriteControlFlow(cfg);
}

void SCCPContext::SCCP(frontend::ControlFlowGraph *cfg) {
  this->init(cfg);
  // 先做BB，再做inst，否则，可能会先做一个没有处理到的phi指令，
  while (this->worklistsNotEmpty()) {
    while (!this->EdgeWorklist.empty()) {
      auto bb = this->EdgeWorklist.front();
      this->EdgeWorklist.pop();
      this->executeOnEdge(cfg,bb);
    }
    while (!this->InstructionWorklist.empty()) {
      auto insn = this->InstructionWorklist.front();
      this->InstructionWorklist.pop();
      this->executeOnInstruction(cfg,insn);
    }
  }
  // for (auto ptr : VisitedBlocks) {
  //   printf("%p ", ptr);
  // }
  // std::cout << "\n";
  this->transform(cfg);
}

void SCCPContext::addAllUsesToWorklist(sym::Var *var) {
  for (auto insn : this->UseChain.at(var)) {
    this->InstructionWorklist.push(insn);
  }
}

} // namespace optimization
} // namespace aaac

void SCCP(aaac::frontend::ControlFlowGraph *cfg) {
  auto context = aaac::optimization::SCCPContext();
  context.SCCP(cfg);
}
