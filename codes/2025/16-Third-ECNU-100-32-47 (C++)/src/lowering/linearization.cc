#include "common.h"
#include "frontend.h"
#include "global.h"
#include "AST/type.h"

#include <memory>
#include <vector>

/**
 * @file linearization.cc
 *
 * 实现了BindNode::linearize，这个函数用来将原始的BindNode扁平化，同时统一出口点，将所有的ReturnNode改写为向返回值赋值然后
 */

namespace aaac {
namespace frontend {
void LinearizedFunction::extend(LinearizedFunction &&other) {
  for (auto var : other.decls) {
    this->addVariable(var);
  }
  for (auto node : other.nodelist) {
    this->nodelist.push_back(node);
  }
}

std::unique_ptr<LinearizedFunction> FunctionNode::linearize() const {
  // 将当前作用域中的所有变量添加到新建的作用域中
  auto linearized_nodes =
      std::make_unique<LinearizedFunction>(this->getFunction(),this->getModule(),this->decls);
  std::vector<InsnNodeList::iterator> ret_sites;
  ret_sites.reserve(6);
  
  auto ret_label_node = LabelNode::generateTemp("exit_label_");
  auto ret_typename =
      this->getFunction()->getReturnTypeName();
  std::shared_ptr<sym::Var> ret_var = nullptr;
  if (ret_typename != TypeFactory::VoidTy) {
    // ret_var = globals.genTempVar(linearized_nodes_scope,ret_typename, "return_var_");
    ret_var=linearized_nodes->generateTempVariable(ret_typename,"return_var_");
  }


  for (auto node : this->body) {
    // // 对嵌套的bind语句，我们需要递归调用这个函数
    // // 这里可能用不到，也有可能在将来的内联优化中使用到，先保留着
    // if (auto sub_bind = std::dynamic_pointer_cast<FunctionNode>(node)) {
    //   auto sub_linearized_nodes = sub_bind->linearize();
    //   linearized_nodes->extend(std::move(*sub_linearized_nodes));

    // } else {
    linearized_nodes->appendNode(node);
    // }

    // 记录可能的ReturnNode
    if (std::dynamic_pointer_cast<ReturnNode>(linearized_nodes->back())) {
      ret_sites.push_back(linearized_nodes->lastNodeIter());
    }
  }

  /*
   * 现在，我们的程序已经基本被线性化，接下来需要统一出口点，只保留末端的ReturnNode
   * 其他的ReturnNode修改为使用一个JumpNode跳转到尾端的label
   */

  // 这个函数无返回节点（因此一定是无返回值的）
  if (ret_sites.empty()) {
    Assert(ret_typename == TypeFactory::VoidTy, "Non-void function returns void");
  } else {
    for (auto ret_site : ret_sites) {
      auto old_retnode = std::dynamic_pointer_cast<ReturnNode>(*ret_site);
      auto jump_node = JumpNode::create(ret_label_node);

      if (!old_retnode->returnWithoutValue()) {
        Assert(ret_var,"void function returns non-void");
        auto new_movenode =
            CopyAssignNode::create(ret_var,
                                    OperandList{old_retnode->getReturnValue()});
        linearized_nodes->insertNodeBefore(ret_site, new_movenode);
      } else {
        Assert(ret_typename == TypeFactory::VoidTy, "Non-void function returns void");
      }

      linearized_nodes->insertNodeBefore(ret_site, jump_node);
      linearized_nodes->removeNode(ret_site);
    }
  }
  // 所有的ReturnNode在此时都应该改为一个目标为此处的JumpNode
  linearized_nodes->appendNode(ret_label_node);
  linearized_nodes->appendNode(ReturnNode::create(ret_var));
  return linearized_nodes;
}

} // namespace frontend
} // namespace aaac
