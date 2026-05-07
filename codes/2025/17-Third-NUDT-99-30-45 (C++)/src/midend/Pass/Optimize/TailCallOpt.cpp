#include "TailCallOpt.h"
#include "IR.h"
#include "IRBuilder.h"
#include "SysYIROptUtils.h"
#include <vector>
// #include <iostream>
#include <algorithm>

namespace sysy {

void *TailCallOpt::ID = (void *)&TailCallOpt::ID;

void TailCallOpt::getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {
  analysisInvalidations.insert(&DominatorTreeAnalysisPass::ID);
  analysisInvalidations.insert(&LoopAnalysisPass::ID);
}

bool TailCallOpt::runOnFunction(Function *F, AnalysisManager &AM) {
  std::vector<CallInst *> tailCallInsts;
  // 遍历函数的所有基本块
  for (auto &bb_ptr : F->getBasicBlocks()) {
    auto BB = bb_ptr.get();
    if (BB->getInstructions().empty()) continue; // 跳过空基本块

    auto term_iter = BB->terminator();
    if (term_iter == BB->getInstructions().end()) continue; // 没有终结指令则跳过
    auto term = (*term_iter).get();

    if (!term || !term->isReturn()) continue; // 不是返回指令则跳过
    auto retInst = static_cast<ReturnInst *>(term);

    Instruction *prevInst = nullptr;
    if (BB->getInstructions().size() > 1) {
        auto it = term_iter;
        --it; // 获取返回指令前的指令
        prevInst = (*it).get();
    }

    if (!prevInst || !prevInst->isCall()) continue; // 前一条不是调用指令则跳过
    auto callInst = static_cast<CallInst *>(prevInst);

    // 检查是否为尾递归调用：被调用函数与当前函数相同且返回值与调用结果匹配
    if (callInst->getCallee() == F) {
  // 对于尾递归，返回值应为调用结果或为 void 类型
        if (retInst->getReturnValue() == callInst || 
            (retInst->getReturnValue() == nullptr && callInst->getType()->isVoid())) {
            tailCallInsts.push_back(callInst);
        }
    }
  }

  if (tailCallInsts.empty()) {
    return false;
  }

  // 创建一个新的入口基本块，作为循环的前置块
  auto original_entry = F->getEntryBlock();
  auto new_entry = F->addBasicBlock("tco.entry." + F->getName());
  auto loop_header = F->addBasicBlock("tco.loop_header." + F->getName());
  
  // 将原入口块中的所有指令移动到循环头块
  loop_header->getInstructions().splice(loop_header->end(), original_entry->getInstructions());
  original_entry->setName("tco.pre_header");

  // 为函数参数创建 phi 节点
  builder->setPosition(loop_header, loop_header->begin());
  std::vector<PhiInst *> phis;
  auto original_args = F->getArguments();
  for (auto &arg : original_args) {
    auto phi = builder->createPhiInst(arg->getType(), {}, {}, "tco.phi."+arg->getName());
    phis.push_back(phi);
  }

  // 用 phi 节点替换所有原始参数的使用
  for (size_t i = 0; i < original_args.size(); ++i) {
    original_args[i]->replaceAllUsesWith(phis[i]);
  }

  // 设置 phi 节点的输入值
  for (size_t i = 0; i < phis.size(); ++i) {
    phis[i]->addIncoming(original_args[i], new_entry);
  }

  // 连接各个基本块
  builder->setPosition(original_entry, original_entry->end());
  builder->createUncondBrInst(new_entry);
  original_entry->addSuccessor(new_entry);
  
  builder->setPosition(new_entry, new_entry->end());
  builder->createUncondBrInst(loop_header);
  new_entry->addSuccessor(loop_header);
  loop_header->addPredecessor(new_entry);

  // 处理每一个尾递归调用
  for (auto callInst : tailCallInsts) {
    auto tail_call_block = callInst->getParent();
    
  // 收集尾递归调用的参数
    auto args_range = callInst->getArguments();
    std::vector<Value*> args;
    std::transform(args_range.begin(), args_range.end(), std::back_inserter(args), 
                   [](auto& use_ptr){ return use_ptr->getValue(); });

  // 用新的参数值更新 phi 节点
    for (size_t i = 0; i < phis.size(); ++i) {
        phis[i]->addIncoming(args[i], tail_call_block);
    }

  // 移除原有的调用和返回指令
    auto term_iter = tail_call_block->terminator();
    SysYIROptUtils::usedelete(term_iter);
    auto call_iter = tail_call_block->findInstIterator(callInst);
    SysYIROptUtils::usedelete(call_iter);

  // 添加跳转回循环头块的分支指令
    builder->setPosition(tail_call_block, tail_call_block->end());
    builder->createUncondBrInst(loop_header);
    tail_call_block->addSuccessor(loop_header);
    loop_header->addPredecessor(tail_call_block);
  }

  return true;
}

} // namespace sysy
