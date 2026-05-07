#include "../include/midend/FunctionCache.h"
#include <algorithm>
#include <cassert>
#include <sstream>
#include <vector>
#include "../include/frontend/IR.h"
#include "../include/frontend/IRBuilder.h"
#include "../include/midend/FuncAnalysis.h"
/**
 * @file FunctionCache.cpp
 * @brief 实现FunctionCache优化的源文件
 *
 */

namespace sysy {
auto FunctionCacheBuilder::isFunctionCachedAble(Function *function) -> bool {
  unsigned num = 0;
  for (const auto &block : function->getBasicBlocks()) {
    for (const auto &inst : block->getInstructions()) {
      if (inst->getKind() == Instruction::kCall) {
        auto callInst = dynamic_cast<CallInst *>(inst.get());
        if (callInst->getCallee() == function) {
          num++;
        }
      }
    }
  }
  return function->getEntryBlock()->getNumArguments() == 2 && num >= 2 &&
         function->getReturnType() != Type::getVoidType() &&
         (function->getAttribute() & Function::FunctionAttribute::Pure) != 0;
}

void FunctionCacheBuilder::run() {
  IRBuilder builder;
  int tableSize = 1021;
  bool isHasCacheArray = false;
  GlobalValue *cacheArray = nullptr;
  for (const auto &functionItem : pModule->getFunctions()) {
    auto function = functionItem.second.get();
    if (isFunctionCachedAble(function)) {
      if (!isHasCacheArray) {
        isHasCacheArray = true;
        ValueCounter init;
        init.push_back(ConstantValue::get(0), 4 * tableSize);
        cacheArray = pModule->createGlobalValue("functionCacheArray", Type::getPointerType(Type::getIntType()),
                                                std::vector<Value *>{ConstantValue::get(4 * tableSize)}, init);
      }
      auto entryBlock = function->getEntryBlock();
      auto condBlock = new BasicBlock(function);
      auto exitBlock = new BasicBlock(function);

      builder.setPosition(condBlock, condBlock->end());
      auto pos = entryBlock->begin();
      while (pos != entryBlock->end()) {
        if (std::find(entryBlock->getArguments().begin(), entryBlock->getArguments().end(), pos->get()) !=
            entryBlock->getArguments().end()) {
          pos = entryBlock->moveInst(pos, condBlock->begin(), condBlock);
        } else {
          pos++;
        }
      }
      for (const auto &arg : entryBlock->getArguments()) {
        condBlock->insertArgument(arg);
      }
      entryBlock->getArguments().clear();

      auto inputArgs = condBlock->getArguments();
      std::vector<Value *> callArgs;
      callArgs.emplace_back(cacheArray);
      assert(inputArgs.size() == 2);
      for (const auto &inputArg : inputArgs) {
        if (inputArg->getType() == Type::getPointerType(Type::getIntType()) && inputArg->getNumDims() == 0) {
          callArgs.emplace_back(inputArg);
        } else if (inputArg->getType() == Type::getPointerType(Type::getFloatType()) && inputArg->getNumDims() == 0) {
          std::stringstream ss;
          ss << "$casted_" << inputArg->getName();
          callArgs.emplace_back(builder.createBitFtoIInst(inputArg, ss.str()));
        } else {
          assert(false);
        }
      }
      auto offset = builder.createCallInst(pModule->getExternalFunction("functionCacheLookup"), callArgs, "$offset");
      auto isHasValOffset = builder.createAddInst(builder.createMulInst(ConstantValue::get(4), offset),
                                                  ConstantValue::get(3), "$isHasValOffset");
      auto valOffset = builder.createAddInst(builder.createMulInst(ConstantValue::get(4), offset),
                                             ConstantValue::get(2), "$valOffset");
      auto isHasVal = builder.createLoadInst(cacheArray, {isHasValOffset}, "$hasVal");
      builder.createCondBrInst(isHasVal, exitBlock, function->getEntryBlock(), {}, {});

      builder.setPosition(exitBlock, exitBlock->end());
      auto val = builder.createLoadInst(cacheArray, {valOffset}, "$val");
      if (function->getReturnType() == Type::getIntType()) {
        builder.createReturnInst(val);
      } else {
        builder.createReturnInst(builder.createBitItoFInst(val, "$casted_val"));
      }

      std::list<BasicBlock *> newBlockList;
      for (const auto &block : function->getBasicBlocks()) {
        auto terminator = block->getInstructions().back().get();
        if (terminator->getKind() == Instruction::kReturn) {
          auto returnInst = dynamic_cast<ReturnInst *>(terminator);
          auto retVal = returnInst->getReturnValue();
          builder.setPosition(block.get(), std::prev(block->end(), 1));
          builder.createStoreInst(ConstantValue::get(1), cacheArray, {isHasValOffset});
          if (function->getReturnType() == Type::getFloatType()) {
            builder.createStoreInst(builder.createBitFtoIInst(retVal), cacheArray, {valOffset});
          } else {
            builder.createStoreInst(retVal, cacheArray, {valOffset});
          }
        }
      }

      function->addBasicBlockFront(exitBlock);
      function->addBasicBlockFront(condBlock);
    }
  }
}

}  // namespace sysy