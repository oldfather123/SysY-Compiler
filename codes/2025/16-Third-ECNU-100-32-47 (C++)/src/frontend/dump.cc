#include "ADT/CFG.h"
#include "common.h"
#include "frontend.h"
#include "sym.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

/**
 * @file 该文件实现了前端和符号表中所有的字符串转储函数（dump）实现
 */

namespace aaac {
namespace sym {

/**
 * @brief 将Var转换成字符串
 *
 * 直接输出它的名字，后续看类型系统如何实现决定是否添加/如何添加名称输出
 */
std::ostream &Var::dump(std::ostream &os) const { return os << this->var_name; }

std::ostream &Function::dump(std::ostream &os) const {
  os << this->name << "(";
  for (const auto &param : this->parameters) {
    os << *param << ",";
  }
  os << ")";

  return os;
}
} // namespace sym
namespace frontend {
/**
 * @brief 将LabelNode转换成字符串
 *
 *  格式如下：
 *  LABEL_NODE <LABEL_NAME>:
 */
std::ostream &LabelNode::dump(std::ostream &os) const {
  return os << "Label"
            << " <" << this->label << ">:";
}

/**这个函数是为了方便AssignNode中OpCode的输出
 *但是脱离了这里这个函数也没有什么其他的作用
 *因此我们实现为static
 */
static std::ostream &operator<<(std::ostream &os,
                                const aaac::frontend::OpCode &o) {
  std::string op_str = " ERROR_OP";
  switch (o) {
  case OpCode::Add:
    op_str = "+";
    break;
  case OpCode::Sub:
  case OpCode::Negate:
    op_str = "-";
    break;
  case OpCode::Mul:
    op_str = "*";
    break;
  case OpCode::Div:
    op_str = "/";
    break;
  case OpCode::Mod:
    op_str = "%";
    break;
  case OpCode::LShift:
    op_str = "<<";
    break;
  case OpCode::RShift:
    op_str = ">>";
    break;
  case OpCode::AddressOf:
  case OpCode::BitAnd:
    op_str = "&";
    break;
  case OpCode::BitOr:
    op_str = "|";
    break;
  case OpCode::BitXor:
    op_str = "^";
    break;
  case OpCode::BitNot:
    op_str = "~";
    break;
  case OpCode::Eq:
    op_str = "==";
    break;
  case OpCode::Neq:
    op_str = "!=";
    break;
  case OpCode::Geq:
    op_str = ">=";
    break;
  case OpCode::Gt:
    op_str = ">";
    break;
  case OpCode::Leq:
    op_str = "<=";
    break;
  case OpCode::Lt:
    op_str = "<";
    break;
  case OpCode::LogNot:
    op_str = "!";
    break;
  case OpCode::Assign:
    op_str = "";
    break;
  case OpCode::Interger_cst:
    op_str = "INT_CST";
    break;
  case OpCode::Float_cst:
    op_str = "FLOAT_CST";
    break;
    // default:
    //   break;
  }
  return os << op_str;
}

/**
 * @brief 将赋值指令转成字符串并输出
 *
 * 格式如下：
 * 对于一元运算符：dest = op operand1
 * 对于二元运算符：dest = operand1 op operand2
 * 对于更多元的运算符（暂时好像没有出现）： dest = op <operand1, operand2, ...>
 */
std::ostream &AssignNode::dump(std::ostream &os) const {
  auto op_iter = this->operands.begin();
  Assert(this->operands.size() != 0, "No operands in assignNode");
  if (this->operands.size() == 1) {
    os << *(this->result) << " = " << this->opcode << " " << *op_iter;
  } else if (this->operands.size() == 2) {
    os << *(this->result) << " = " << *op_iter++ << " " << this->opcode << " "
       << *op_iter;
  } else {
    os << *(this->result) << " = " << this->opcode << " < " << *op_iter++;
    while (op_iter != this->operands.end()) {
      os << " , " << *op_iter++;
    }
  }
  return os;
}

/** TODO: documentation
 */
std::ostream &LoadNode::dump(std::ostream &os) const {
  os << "Load " << *var << " <- " << *array;
  if (indexes.size() > 0) {
    os << "[";
    bool first = true;
    for (auto index : indexes) {
      if (!first) {
        os << ",";
      }
      first = false;
      os << index;
    }
    os << "]";
  }
  return os;
}

/** TODO: documentaion
 */
std::ostream &StoreNode::dump(std::ostream &os) const {
  os << "Store " << *array << "[";
  bool first = true;
  for (auto index : indexes) {
    if (!first) {
      os << ",";
    }
    first = false;
    os << index;
  }
  os << "] <- " << value;
  return os;
}

/**
 * @brief 将CallNode转换成字符串
 *
 * 格式如下
 *  <result> = func_name(arg1,arg2,...)
 */
std::ostream &CallNode::dump(std::ostream &os) const {
  if (!this->returnValueDiscarded()) {
    os << *(this->return_value) << " = ";
  }
  os << this->getFunction()->getName() << "(";
  for (auto &i : this->args) {
    os << i << ", ";
  }
  return os << ")";
}

/**
 * @brief 将ConditionNode转换成字符串
 *
 * 格式如下
 *  Condition (op1 OP op2) <true label>, <false label>
 * 特殊情况：
 * 一元运算符（!）
 * Condition (OP op1) <true label>, <false label>
 */
std::ostream &ConditionNode::dump(std::ostream &os) const {
  os << "Condition (";

  auto it = this->operands.begin();
  if (it != this->operands.end()) {
    os << *it++;

    if (it != this->operands.end()) {
      os << " " << this->opcode << " " << *it;
    } else {
      os << this->opcode; // 一元运算符，如 LogNot
    }
  }

  os << ") ";

  if (this->true_label) {
    os << "<" << this->true_label->getName() << ">";
  } else {
    os << "<null>";
  }

  os << ", ";

  if (this->false_label) {
    os << "<" << this->false_label->getName() << ">";
  } else {
    os << "<null>";
  }

  return os;
}
/**
 * @brief 将ReturnNode转换成字符串
 *
 * 格式如下
 *  Return return_value
 */
std::ostream &ReturnNode::dump(std::ostream &os) const {
  if (this->returnWithoutValue())
    return os << "Return";
  else
    return os << "Return " << this->value;
}
/**
 * @brief 将JumpNode转换成字符串
 *
 * 格式如下
 *  Jump <label>
 */
std::ostream &JumpNode::dump(std::ostream &os) const {
  return os << "Jump <" << this->label->getName() << ">";
}

/**
 * @brief 将NopNode转换成字符串
 *
 * Nop
 */
std::ostream &NopNode::dump(std::ostream &os) const { return os << "Nop"; }

std::ostream &PhiNode::dump(std::ostream &os) const {
  os << "PHI " << dest_var->getName() << " = [";
  for (const auto &operand : operands) {
    if (operand.first) {
      std::visit([&, bb = operand.first](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<aaac::sym::Var>>) {
          os << arg->getName() << "@" << bb << ", ";
        } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float>) {
          os << "$" << arg << "@" << bb << ", ";
        } else {
          os << "<<UNDEF>>@" << bb << ", ";
        }
      }, operand.second);
    }
  }
  return os << "]";
}

/**
 * @brief 将LinearizedBind转换成字符串
 */
std::ostream &LinearizedFunction::dump(std::ostream &os) const {
  for (auto &v : this->decls) {
    os << *v << "\n";
  }
  os << "-=-=-\n";
  for (auto &i : this->nodelist) {
    os << *i << "\n";
  }
  return os;
}

/**
 * @brief 将BindNode转换成字符串
 *
 * 格式如下
 *  {
 *    VARS
 *
 *    BODY
 *  }
 */
std::ostream &FunctionNode::dump(std::ostream &os) const {
  os << *(this->function.lock()) << "{\n";
  for (auto &v : this->decls) {
    os << *v << " " << *v->getType() << "\n";
  }
  os << "\n";
  for (auto &i : this->body) {
    os << *i << "\n";
  }
  return os << "}\n";
}

/**
 * @brief 将Module转换成字符串
 * 格式如下
 * BindNode
 *
 * BindNode
 */
std::ostream &Module::dump(std::ostream &os) const {
  for (auto &binding : bindings) {
    binding->dump(os);
    os << "\n";
  }
  return os;
}

/**
 * @brief 将InitializeNode转换成字符串
 */
std::ostream &InitializeNode::dump(std::ostream &os) const {
  os << "Initialize " << *var;
  if (indexes.size() > 0) {
    os << "[";
    bool first = true;
    for (auto index : indexes) {
      if (!first) {
        os << " ";
      }
      first = false;
      os << index;
    }
    os << "]";
  }
  os << " <- ";
  if (std::holds_alternative<int>(value)) {
    return os << "$" << std::get<int>(value);
  } else if (std::holds_alternative<float>(value)) {
    return os << "$" << std::get<float>(value);
  }
  return os;
}

/**
 * @brief 将BasicBlock转换成字符串
 *
 * 格式如下：
 * [BasicBlock在内存的地址]
 *      指令序列
 *{Type:Target 1}{Type:Target 2}...
 */
std::ostream &BasicBlock::dump(std::ostream &os) const {
  os << "[" << shared_from_this().get() << "]"
     << "\n";
  for (auto &i : this->nodes) {
    os << "\t\t" << *i << "\n";
  }
  // for (auto &e : this->getSuccEdges()) {
  //   os << "{";
  //   switch (e.getFlag()) {
  //   case JUMP:
  //     os << "JUMP";
  //     break;
  //   case THEN:
  //     os << "THEN";
  //     break;
  //   case ELSE:
  //     os << "ELSE";
  //     break;
  //   default:
  //     os << "UNKNOWN";
  //     break;
  //   }
  //   os << ":" << e.getBB().get() << "} ";
  // }
  // os << "\n";
  return os;
}

// static int get_index(const std::vector<BasicBlock> &bbs,
// std::shared_ptr<BasicBlock> bb){
//   auto iter=std::find(bbs.begin(),bbs.end(),bb);
//   return std::distance(bbs.begin(), iter);
// }

/**
 * @brief 将ControlFlowGraph转换成字符串
 */
std::ostream &ControlFlowGraph::dump(std::ostream &os) const {
  // int count=0;
  // os << "\n=== DFS Tree ===\n";
  // this->forwardTraverse(
  //       [&](std::shared_ptr<sym::Function> func, std::shared_ptr<BasicBlock>
  //       bb) {
  //           os << "\nBB" << bb->getDFSInfo().order << ": ";
  //           bb->getDFSInfo().dump(os);
  //       },
  //       dummyHandler
  //   );
  for (auto &i : this->bbs) {
    os << i->getDFSInfo() << "\n";
    os << i->getDominanceInfo() << "\n";
    os << *i << "\n";
  }
  return os;
}

std::ostream &DFSInfo::dump(std::ostream &os) const {
  auto parent_ptr = parent.lock();
  os << "DFS Order: " << order
     << " | Parent: " << (parent_ptr ? parent_ptr->getDFSInfo().order : 0)
     << " | Children: [";

  for (auto it = children.begin(); it != children.end(); ++it) {
    if (auto child = it->lock()) {
      os << child->getDFSInfo().order;
      if (it != std::prev(children.end()))
        os << ", ";
    }
  }
  return os << "]";
}

// 在DominanceInfo类中添加dump方法
std::ostream &DominanceInfo::dump(std::ostream &os) const {
  os << "DomLevel: " << level << " | IDom: ";

  if (auto idom_ptr = idom.lock()) {
    os << idom_ptr->getDFSInfo().order;
  } else {
    os << "null";
  }

  os << " | Children: [";
  for (auto it = children.begin(); it != children.end(); ++it) {
    if (auto child = it->lock()) {
      os << child->getDFSInfo().order;
      if (it != std::prev(children.end()))
        os << ", ";
    }
  }
  return os << "]";
}

} // namespace frontend
} // namespace aaac

/**
 * @brief 将Operand转换成字符串
 */
std::ostream &operator<<(std::ostream &os, const aaac::frontend::Operand &o) {
  if (std::holds_alternative<int>(o)) {
    return os << "$" << std::get<int>(o);
  } else if (std::holds_alternative<float>(o)) {
    return os << "$" << std::get<float>(o);
  } else if (std::holds_alternative<std::shared_ptr<aaac::sym::Var>>(o)) {
    return os << *(std::get<std::shared_ptr<aaac::sym::Var>>(o));
  } else {
    return os << "ERROR_OPERAND";
  }
}
