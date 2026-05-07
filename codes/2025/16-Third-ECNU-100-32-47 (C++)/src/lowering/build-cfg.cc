#include "backend.h"
#include "common.h"
#include "frontend.h"
#include "global.h"
#include "sym.h"
#include "ADT/CFG.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace aaac {
namespace frontend {

/**
 * @brief 用LinearizedBind填充Function结构
 *
 * 该函数在构造控制流图结构的同时输出Insn，为之后生成ssa阶段做准备
 */
std::shared_ptr<ControlFlowGraph> LinearizedFunction::build_cfg() {
  // 当前函数，用于构造CFG
  auto function = this->getFunction();
  auto cfg = std::make_unique<ControlFlowGraph>(function,this->decls);
  std::shared_ptr<BasicBlock> bb = nullptr;
  // 通过记录标签名和BasicBlock之间的关系从而创建边
  std::unordered_map<std::string, std::shared_ptr<BasicBlock>> label_bb;
  bool need_newbb = true;
  std::shared_ptr<InsnNode> prev = std::make_shared<NopNode>();
  // 第一遍循环，新建所有的basicblock，同时在label_bb中建立label和bb的对应关系
    for (auto node : this->nodelist) {
      if (need_newbb) {
        std::string index;
        bb = cfg->createBasicBlock();

        // 规范化BasicBlock,保证每个bb以Label开头
        if (node->getCode() != NodeCode::Label) {
          auto new_label = frontend::LabelNode::generateTemp();
          index = new_label->getName();
          bb->push_back(new_label);
      } else {
        index = std::dynamic_pointer_cast<LabelNode>(node)->getName();
      }
      bb->push_back(node);

      // 创建索引
      label_bb[index] = bb;
      if (!cfg->hasStartBasicBlock()) {
        cfg->setStartBasicBlock(bb);
      }

      need_newbb = false;
      prev = node;
      continue;
    }

    switch (node->getCode()) {
    case NodeCode::Label:
      // 如果前面是Label，不需要创建新的BasicBlock
      // 否则，结束当前BasicBlock
      // 规范化：由于这里是fall-through的，我们需要加上一个跳转指令来方便后续操作的进行
      
      /*
        这里有点问题，一个最简单的例子是：
          Condition (fparm_0 == fparm_1) <L_100>, <L_101>
          Label <L_100>:
          t_1 =  $1
          Return t_1
          Label <L_101>:
          t_2 =  $0
          Return t_2
        如果是这样的判断，会生成：
          DFS Order: 0 | Parent: 0 | Children: []
          DomLevel: 0 | IDom: null | Children: []
          [0x557bd4e058f0]
                          Label <L_103>:
                          Condition (fparm_0 == fparm_1) <L_100>, <L_101>
                          Jump <L_100>
          {JUMP:0x557bd4e05b90} 

          DFS Order: 0 | Parent: 0 | Children: []
          DomLevel: 0 | IDom: null | Children: []
          [0x557bd4e05b90]
                          Label <L_100>:
                          t_1 =  $1
                          return_var_1000 =  t_1
                          Jump <exit_label_102>
          {JUMP:0x557bd4e06010} 

          DFS Order: 0 | Parent: 0 | Children: []
          DomLevel: 0 | IDom: null | Children: []
          [0x557bd4e05dd0]
                          Label <L_101>:
                          t_2 =  $0
                          return_var_1000 =  t_2
                          Jump <exit_label_102>


          DFS Order: 0 | Parent: 0 | Children: []
          DomLevel: 0 | IDom: null | Children: []
          [0x557bd4e06010]
                          Label <exit_label_102>:
                          Return return_var_1000
        可以看到，这里的控制流是不对的
      */ 
      if (prev->getCode() != NodeCode::Label) {
        // add_and_save(
        //     JumpNode::create(std::dynamic_pointer_cast<LabelNode>(node)));
        if (prev->getCode() != NodeCode::Jump && prev->getCode() != NodeCode::Condition) {
          bb->push_back(
              JumpNode::create(std::dynamic_pointer_cast<LabelNode>(node)));
        }
        bb=cfg->createBasicBlock();
      }
      label_bb[std::dynamic_pointer_cast<LabelNode>(node)->getName()] = bb;
      bb->push_back(node);
      break;
    case NodeCode::Jump:
    case NodeCode::Condition:
      // 这两类Node都会让原来的BasicBlock结束，之后我们将need_newbb置位，下一次循环时会重新创建BasicBlock
      bb->push_back(node);
      need_newbb = true;
      break;
    case NodeCode::Return:
      // Return节点需要一个额外的操作，将function的exit_block设置
      bb->push_back(node);
      cfg->setExitBasicBlock(bb);
      break;
    default:
      // 对于其他类型的Node，我们只要把相应的Insn添加到bb_insn中即可
      bb->push_back(node);
      break;
    }
    prev = node;
  }

  std::unordered_set<BasicBlock *> vis;
  std::function<int(std::shared_ptr<BasicBlock>)> build_bb_edges;
  build_bb_edges = [&vis, &label_bb,
                    &build_bb_edges](std::shared_ptr<BasicBlock> bb) -> int {
    // 标记为已访问
    if (vis.count(bb.get()))
      return 0;
    vis.insert(bb.get());

    // 处理它的后继
    auto lastNode = bb->back();
    if (auto jumpNode = std::dynamic_pointer_cast<JumpNode>(lastNode)) {
      std::string idx = jumpNode->getLabel()->getName();
      if (label_bb.count(idx)) {
        bb->connect(label_bb.at(idx),BasicBlock::EdgeFlag::JUMP);
      } else {
        return 1;
      }
    } else if (auto conditionNode =
                   std::dynamic_pointer_cast<ConditionNode>(lastNode)) {
      std::string idx_true = conditionNode->getTrueLabel()->getName();
      std::string idx_false = conditionNode->getFalseLabel()->getName();
      if (label_bb.count(idx_true)) {
        bb->connect(label_bb.at(idx_true),BasicBlock::EdgeFlag::THEN);
      } else {
        return 1;
      }
      if (label_bb.count(idx_false)) {
        bb->connect(label_bb.at(idx_false),BasicBlock::EdgeFlag::ELSE);
      } else {
        return 1;
      }
    } else if (auto returnNode =
                   std::dynamic_pointer_cast<ReturnNode>(lastNode)) {
      return 0; // 遍历到这里说明正常结束了
    } else {
      return 1; // 除了上面几种情况之外，理论上不会有别的Node出现在bb结尾
    }

    // 对当前节点的所有后继运行这个函数
    int ret = 0;
    for (auto &succ : bb->getSuccEdges()) {
      ret += build_bb_edges(succ.getDest());
    }
    return ret;
  };
  int errnum=build_bb_edges(cfg->getStartBasicBlock());
  Assert(errnum==0,"build_cfg: Error at build edge!");
  return cfg;
}

} // namespace frontend
} // namespace aaac
