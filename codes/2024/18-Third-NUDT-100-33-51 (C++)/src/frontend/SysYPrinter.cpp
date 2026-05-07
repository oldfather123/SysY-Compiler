#include "../include/frontend/SysYPrinter.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include "../include/frontend/IR.h"

namespace sysy {

/**
 * @brief 打印Module中的IR
 *
 * 默认打印在标准输出。若fileName被设置，则打印在指定名称的文件。
 * @param [in] fileName 打印输出的目标文件
 * @return 无返回值
 */
void SysYPrinter::printIR(const std::string &fileName) {
  std::streambuf *oldCoutBuf;
  std::ofstream fon;
  // BasicBlock *globalBlock = functions.find("%")->second->getEntryBlock();
  // BasicBlock::inst_list &instructions = globalBlock->getInstructions();
  if (!fileName.empty()) {
    oldCoutBuf = std::cout.rdbuf();
    fon.open(fileName);

    if (!fon.is_open()) {
      assert(false);
    }

    std::cout.rdbuf(fon.rdbuf());
  }

  const auto &functions = pModule->getFunctions();

  printGlobal();

  std::cout << std::endl;

  auto &main = functions.find("main")->second;
  printFunction(main.get());

  std::cout << std::endl;

  for (const auto &iter : functions) {
    if (iter.second->getName() != "main") {
      printFunction(iter.second.get());
      std::cout << std::endl;
    }
  }

  if (!fileName.empty()) {
    std::cout.rdbuf(oldCoutBuf);
    fon.close();
  }
}

/**
 * @brief 打印全局变量对应的IR
 *
 * @return 无返回值
 */
void SysYPrinter::printGlobal() {
  auto &globals = pModule->getGlobals();

  std::cout << "global:" << std::endl;
  for (const auto &global : globals) {
    std::cout << "   ";
    std::cout << "Alloca"
              << " global " << global->getName() << " ";
    auto type = dynamic_cast<PointerType *>(global->getType())->getBaseType();
    if (type == Type::getFloatType()) {
      std::cout << "float";
    } else {
      std::cout << "int";
    }

    if (global->getNumDims() != 0) {
      for (unsigned i = 0; i < global->getNumDims(); i++) {
        std::cout << "[" << getOperandName(global->getDim(i)) << "]";
      }

      std::cout << "   ";

      std::cout << "{";

      auto values = global->getInitValues();
      auto counterValues = values.getValues();
      auto counterNumbers = values.getNumbers();
      if (type == Type::getFloatType()) {
        std::cout << dynamic_cast<ConstantValue *>(counterValues[0])->getFloat() << ":" << counterNumbers[0];
      } else {
        std::cout << dynamic_cast<ConstantValue *>(counterValues[0])->getInt() << ":" << counterNumbers[0];
      }

      if (type == Type::getFloatType()) {
        for (size_t i = 1; i < counterNumbers.size(); i++) {
          std::cout << ", " << dynamic_cast<ConstantValue *>(counterValues[i])->getFloat() << ":" << counterNumbers[i];
        }
      } else {
        for (size_t i = 1; i < counterNumbers.size(); i++) {
          std::cout << ", " << dynamic_cast<ConstantValue *>(counterValues[i])->getInt() << ":" << counterNumbers[i];
        }
      }
      std::cout << "}";

    } else {
      std::cout << "   ";
      if (type == Type::getFloatType()) {
        std::cout << dynamic_cast<ConstantValue *>(global->getByIndex(0))->getFloat();
      } else {
        std::cout << dynamic_cast<ConstantValue *>(global->getByIndex(0))->getInt();
      }
    }

    std::cout << std::endl;
  }
}

/**
 * @brief 打印函数对应的IR
 *
 * @param [in] function 所要打印的函数
 * @return 无返回值
 */
void SysYPrinter::printFunction(Function *function) {
  std::cout << function->getName() << ":" << std::endl;

  auto entryBlock = function->getEntryBlock();

  std::cout << std::endl;
  std::cout << "   "
            << "arg:";
  if (entryBlock->getNumArguments() != 0) {
    std::cout << entryBlock->getArguments()[0]->getName();
  }

  for (unsigned i = 1; i < entryBlock->getNumArguments(); i++) {
    std::cout << ", " << entryBlock->getArguments()[i]->getName();
  }

  std::cout << std::endl << std::endl;

  for (const auto &blockIter : function->getBasicBlocks()) {
    if (!(blockIter->getName().empty())) {
      std::cout << blockIter->getName() << ":" << std::endl;
    }

    for (const auto &instIter : blockIter->getInstructions()) {
      auto inst = instIter.get();
      std::cout << "   ";
      printInst(inst);
    }
  }
}

/**
 * @brief 获取操作数对应的字符串
 *
 * @param [in] operand 所要打印的操作数
 * @return 操作数对应的字符串
 */
auto SysYPrinter::getOperandName(Value *operand) -> std::string {
  ConstantValue *constOperand = nullptr;
  GlobalValue *globalOperand = nullptr;
  AllocaInst *localOperand = nullptr;
  Instruction *tmpOperand = nullptr;
  ConstantVariable *constVariable = nullptr;

  assert(operand != nullptr);
  constOperand = dynamic_cast<ConstantValue *>(operand);
  if (constOperand != nullptr) {
    if (constOperand->isFloat()) {
      return std::to_string(constOperand->getFloat());
    }
    return std::to_string(constOperand->getInt());
  }
  constVariable = dynamic_cast<ConstantVariable *>(operand);
  if (constVariable != nullptr) {
    return constVariable->getName();
  }
  globalOperand = dynamic_cast<GlobalValue *>(operand);
  if (globalOperand != nullptr) {
    return globalOperand->getName();
  }
  localOperand = dynamic_cast<AllocaInst *>(operand);
  if (localOperand != nullptr) {
    return localOperand->getName();
  }
  tmpOperand = dynamic_cast<Instruction *>(operand);
  if (tmpOperand != nullptr) {
    return tmpOperand->getName();
  }
  assert(false);
}

/**
 * @brief 打印指令
 *
 * @param [in] pInst 所要打印的指令
 * @return 无返回值
 */
void SysYPrinter::printInst(Instruction *pInst) {
  using Kind = Instruction::Kind;

  switch (pInst->getKind()) {
    case (Kind::kAdd): {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "Add"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kSub: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "Sub"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kMul: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "Mul"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kDiv: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "Div"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kRem: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "Rem"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kICmpEQ: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "EQ"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kICmpNE: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "NE"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kICmpLT: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "LT"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kICmpGT: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "GT"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kICmpLE: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "LE"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kICmpGE: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "GE"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFCmpEQ: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FEQ"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFCmpNE: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FNE"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFCmpLT: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FLT"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFCmpGT: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FGT"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFCmpLE: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FLE"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFCmpGE: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FGE"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFAdd: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FAdd"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFSub: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FSub"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFMul: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FMul"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kFDiv: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "FDiv"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kAnd: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "And"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kOr: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      auto lhs = binInst->getLhs();
      auto rhs = binInst->getRhs();
      std::cout << "Or"
                << " " << getOperandName(lhs) << ", " << getOperandName(rhs) << ", " << binInst->getName() << std::endl;
    } break;
    case Kind::kNeg: {
      auto unyInst = dynamic_cast<UnaryInst *>(pInst);
      auto operand = pInst->getOperand(0);
      std::cout << "Neg"
                << " " << getOperandName(operand) << ", " << unyInst->getName() << std::endl;
    } break;
    case Kind::kNot: {
      auto unyInst = dynamic_cast<UnaryInst *>(pInst);
      auto operand = pInst->getOperand(0);
      std::cout << "Not"
                << " " << getOperandName(operand) << ", " << unyInst->getName() << std::endl;
    } break;
    case Kind::kFNeg: {
      auto unyInst = dynamic_cast<UnaryInst *>(pInst);
      auto operand = pInst->getOperand(0);
      std::cout << "FNeg"
                << " " << getOperandName(operand) << ", " << unyInst->getName() << std::endl;
    } break;
    case Kind::kFNot: {
      auto unyInst = dynamic_cast<UnaryInst *>(pInst);
      auto operand = pInst->getOperand(0);
      std::cout << "FNot"
                << " " << getOperandName(operand) << ", " << unyInst->getName() << std::endl;
    } break;
    case Kind::kFtoI: {
      auto unyInst = dynamic_cast<UnaryInst *>(pInst);
      auto operand = pInst->getOperand(0);
      std::cout << "FtoI"
                << " " << getOperandName(operand) << ", " << unyInst->getName() << std::endl;
    } break;
    case Kind::kBitFtoI: {
      auto unyInst = dynamic_cast<UnaryInst *>(pInst);
      auto operand = pInst->getOperand(0);
      std::cout << "BitFtoI"
                << " " << getOperandName(operand) << ", " << unyInst->getName() << std::endl;
    } break;
    case Kind::kItoF: {
      auto unyInst = dynamic_cast<UnaryInst *>(pInst);
      auto operand = pInst->getOperand(0);
      std::cout << "ItoF"
                << " " << getOperandName(operand) << ", " << unyInst->getName() << std::endl;
    } break;
    case Kind::kBitItoF: {
      auto unyInst = dynamic_cast<UnaryInst *>(pInst);
      auto operand = pInst->getOperand(0);
      std::cout << "BitItoF"
                << " " << getOperandName(operand) << ", " << unyInst->getName() << std::endl;
    } break;
    case Kind::kCall: {
      auto callInst = dynamic_cast<CallInst *>(pInst);
      auto function = callInst->getCallee();
      std::cout << "Call"
                << " " << function->getName() << "(";
      auto params = callInst->getArguments();
      if (params.size() != 0) {
        std::cout << getOperandName(params.begin()->get()->getValue());
        for (auto iter = next(params.begin()); iter != params.end(); iter++) {
          std::cout << ", " << getOperandName(iter->get()->getValue());
        }
      }
      std::cout << ")"
                << " " << callInst->getName() << std::endl;
    } break;
    case Kind::kCondBr: {
      auto condBrInst = dynamic_cast<CondBrInst *>(pInst);
      std::cout << "CondBr"
                << " " << getOperandName(condBrInst->getCondition()) << ", " << condBrInst->getThenBlock()->getName()
                << ", " << condBrInst->getElseBlock()->getName() << std::endl;
    } break;
    case Kind::kBr: {
      auto brInst = dynamic_cast<UncondBrInst *>(pInst);
      std::cout << "Br"
                << " " << brInst->getBlock()->getName() << std::endl;
    } break;
    case Kind::kReturn: {
      auto retInst = dynamic_cast<ReturnInst *>(pInst);
      std::cout << "Return"
                << " ";
      if (retInst->getNumOperands() != 0) {
        std::cout << getOperandName(retInst->getOperand(0)) << std::endl;
      } else {
        std::cout << std::endl;
      }
    } break;
    case Kind::kAlloca: {
      auto allocaInst = dynamic_cast<AllocaInst *>(pInst);
      std::cout << "Alloca"
                << " " << allocaInst->getName() << " ";
      auto type = dynamic_cast<PointerType *>(allocaInst->getType());
      if (type->getBaseType() == Type::getFloatType()) {
        std::cout << "float";
      } else {
        std::cout << "int";
      }
      if (allocaInst->getNumDims() != 0) {
        for (const auto &dim : allocaInst->getDims()) {
          std::cout << "[" << getOperandName(dim->getValue()) << "]";
        }
      }
      std::cout << std::endl;
    } break;
    case Kind::kLoad: {
      auto loadInst = dynamic_cast<LoadInst *>(pInst);
      std::cout << "Load"
                << " " << getOperandName(loadInst->getPointer());
      if (loadInst->getNumIndices() != 0) {
        for (const auto &dim : loadInst->getIndices()) {
          std::cout << "[" << getOperandName(dim->getValue()) << "]";
        }
      }
      std::cout << ", " << loadInst->getName() << std::endl;
    } break;
    case Kind::kLa: {
      auto laInst = dynamic_cast<LaInst *>(pInst);
      std::cout << "La"
                << " " << getOperandName(laInst->getPointer());
      if (laInst->getNumIndices() != 0) {
        for (const auto &dim : laInst->getIndices()) {
          std::cout << "[" << getOperandName(dim->getValue()) << "]";
        }
      }
      std::cout << ", " << laInst->getName() << std::endl;
    } break;
    case Kind::kStore: {
      auto storeInst = dynamic_cast<StoreInst *>(pInst);
      std::cout << "store"
                << " " << getOperandName(storeInst->getValue()) << ", ";
      std::cout << getOperandName(storeInst->getPointer());
      if (storeInst->getNumIndices() != 0) {
        for (const auto &dim : storeInst->getIndices()) {
          std::cout << "[" << getOperandName(dim->getValue()) << "]";
        }
      }
      std::cout << std::endl;
    } break;
    case Kind::kMemset: {
      auto memsetInst = dynamic_cast<MemsetInst *>(pInst);
      std::cout << "memset"
                << " " << getOperandName(memsetInst->getPointer()) << " "
                << "begin " << getOperandName(memsetInst->getBegin()) << " "
                << "size " << getOperandName(memsetInst->getSize()) << " "
                << "value " << getOperandName(memsetInst->getValue());
      std::cout << std::endl;
    } break;
    case Kind::kPhi: {
      auto phiInst = dynamic_cast<PhiInst *>(pInst);
      std::cout << "Phi"
                << " ";
      auto opnum = phiInst->getNumOperands();
      for (unsigned i = 0; i < opnum; ++i) {
        std::cout << getOperandName(phiInst->getOperand(i)) << " ";
      }
      std::cout << std::endl;
    } break;
    case Kind::kGetSubArray: {
      auto getSubArrayInst = dynamic_cast<GetSubArrayInst *>(pInst);
      std::cout << "GetSubArray"
                << " " << getOperandName(getSubArrayInst->getFatherArray());
      if (getSubArrayInst->getNumIndices() != 0) {
        for (const auto &index : getSubArrayInst->getIndices()) {
          std::cout << "[" << getOperandName(index->getValue()) << "]";
        }
      }
      std::cout << ", " << getOperandName(getSubArrayInst->getChildArray()) << std::endl;
      break;
    }
    default:
      assert(false);
      break;
  }
}
}  // namespace sysy
