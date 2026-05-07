/**
 * @File Name: SysyPhiDelete.cpp
 * @brief  reg2mem
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#include "../include/backend/SysyPhiDelete.h"
#include <cstddef>
#include <iostream>
#include <list>
#include <memory>
#include "../include/frontend/IR.h"
#include "../include/frontend/IRBuilder.h"

namespace sysy {

/**
 * @brief  删除phi节点
 */
auto SysyPhiDelete::PhiDelete() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    auto basicBlocks = function.second->getBasicBlocks();
    for (auto &basicBlock : basicBlocks) {
      // if (basicBlock->getNumInstructions() == 0) {
      //   std ::cout << "the numinstructions is " << basicBlock->getNumInstructions() << std::endl;
      //   std ::cout << "the block name is " << basicBlock->getName() << std::endl;
      //   std ::cout << "the Successors num is " << basicBlock->getNumSuccessors() << std::endl;
      // }

      // using inst_list = std::list<std::unique_ptr<Instruction>>;

      for (auto iter = basicBlock->begin(); iter != basicBlock->end();) {
        auto &instruction = *iter;
        if (instruction->isPhi()) {
          auto predBlocks = basicBlock->getPredecessors();
          // mv 的目的位置
          auto destination = instruction->getOperand(0);
          // if (sec == destination) {
          //   std::cout << "this is " << instruction->getNumOperands() << std::endl;
          // }
          int predBlockindex = 0;
          for (auto &predBlock : predBlocks) {
            ++predBlockindex;
            // 判断前驱块儿只有一个后继还是多个后继
            // 如果有多个
            auto source = instruction->getOperand(predBlockindex);
            if (source == destination) {
              continue;
            }
            // std::cout << predBlock->getNumSuccessors() << std::endl;
            if (predBlock->getNumSuccessors() > 1) {
              // 创建一个basicblock
              auto newbasicBlock = function.second->addBasicBlock();
              std::stringstream ss;
              ss << "L" << pBuilder->getLabelIndex();
              newbasicBlock->setName(ss.str());
              ss.str("");
              // newbasicBlock->setName();
              // std::cout << "the block name is " << basicBlock->getName() << std::endl;
              // for (auto pb : basicBlock->getPredecessors()) {
              //   // newbasicBlock->addPredecessor(pb);
              //   std::cout << pb->getName() << std::endl;
              // }
              // // 修改前驱后继关系
              basicBlock->replacePredecessor(predBlock, newbasicBlock);
              // predBlock = newbasicBlock;
              newbasicBlock->addPredecessor(predBlock);
              newbasicBlock->addSuccessor(basicBlock.get());
              predBlock->removeSuccessor(basicBlock.get());
              predBlock->addSuccessor(newbasicBlock);
              // std::cout << "the block name is " << basicBlock->getName() << std::endl;
              // for (auto pb : basicBlock->getPredecessors()) {
              //   // newbasicBlock->addPredecessor(pb);
              //   std::cout << pb->getName() << std::endl;
              // }
              // sysy::BasicBlock::conectBlocks(newbasicBlock, static_cast<BasicBlock *>(basicBlock.get()));
              // 若后为跳转指令，应该修改跳转指令所到达的位置
              auto thelastinst = predBlock->end();
              (--thelastinst);

              if (thelastinst->get()->isConditional() || thelastinst->get()->isUnconditional()) {  // 如果是跳转指令
                auto opnum = thelastinst->get()->getNumOperands();
                for (size_t i = 0; i < opnum; i++) {
                  if (thelastinst->get()->getOperand(i) == basicBlock.get()) {
                    thelastinst->get()->replaceOperand(i, newbasicBlock);
                  }
                }
              }
              // 在新块中插入mv指令
              pBuilder->setPosition(newbasicBlock, newbasicBlock->end());
              // pBuilder->createStoreInst(source, destination);
              if (source->isInt() || source->isFloat()) {
                pBuilder->createStoreInst(source, destination);
              } else {
                auto loadInst = pBuilder->createLoadInst(source);
                pBuilder->createStoreInst(loadInst, destination);
              }
              // pBuilder->createMoveInst(Instruction::kMove, destination->getType(), destination, source,
              // newbasicBlock);
              pBuilder->setPosition(newbasicBlock, newbasicBlock->end());
              pBuilder->createUncondBrInst(basicBlock.get(), {});
            } else {
              // 如果前驱块只有一个后继
              auto thelastinst = predBlock->end();
              (--thelastinst);
              // std::cout << predBlock->getName() << std::endl;
              // std::cout << thelastinst->get() << std::endl;
              // std::cout << "First point  11 " << std::endl;
              if (thelastinst->get()->isConditional() || thelastinst->get()->isUnconditional()) {
                // 在跳转语句前insert mv指令
                pBuilder->setPosition(predBlock, thelastinst);
              } else {
                // 在末尾加insert mv指令
                pBuilder->setPosition(predBlock, predBlock->end());
              }

              if (source->isInt() || source->isFloat()) {
                pBuilder->createStoreInst(source, destination);
              } else {
                auto loadInst = pBuilder->createLoadInst(source);
                pBuilder->createStoreInst(loadInst, destination);
              }
              // pBuilder->createMoveInst(Instruction::kMove, destination->getType(), destination, source, predBlock);
              // std::cout << thelastinst->get()->isUnconditional() << std::endl;
            }
          }
          auto &instructions = basicBlock->getInstructions();
          usedelete(iter->get());
          iter = instructions.erase(iter);
          if (basicBlock->getNumInstructions() == 0) {
            if (basicBlock->getNumSuccessors() == 1) {
              pBuilder->setPosition(basicBlock.get(), basicBlock->end());
              pBuilder->createUncondBrInst(basicBlock->getSuccessors()[0], {});
            }
          }
        } else {
          break;
        }
      }
    }
  }
}

/**
 * @brief  删除指令的Use关系
 * @param  instr: 要删除use关系的指令
 */
auto SysyPhiDelete::usedelete(Instruction *instr) -> void {
  for (auto &use1 : instr->getOperands()) {
    auto val1 = use1->getValue();
    // std::cout << val1->getName() << std::endl;
    val1->removeUse(use1);
  }
}
}  // namespace sysy
