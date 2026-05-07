#include "../include/backend/DAGBuilder.h"
#include <sys/types.h>
#include <algorithm>
#include <any>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <sstream>
#include <stack>
#include <vector>
#include "../include/backend/Mid2EndDAG.h"
#include "../include/frontend/IR.h"

/**
 * @file DAGBuilder.cpp
 * @brief 中端到后端DAG构建器的源文件
 *
 */

namespace mid2EndDAG {
/**
 * @brief 初始化基本元素
 *
 * @param [in] midModule 待处理的中端IR模块
 * @return 无返回值
 */
void Mid2EndDAGBuilder::initBasicElements(sysy::Module *midModule) {
  pModule.reset(new Module);
  Function::ReturnType returnType;
  for (const auto &functionItem : midModule->getFunctions()) {
    auto midReturnType = functionItem.second->getReturnType();
    if (midReturnType == sysy::Type::getFloatType()) {
      returnType = Function::FLOAT;
    } else if (midReturnType == sysy::Type::getIntType()) {
      returnType = Function::INT;
    } else if (midReturnType == sysy::Type::getVoidType()) {
      returnType = Function::VOID;
    } else {
      assert(false);
    }
    auto function = pModule->createFunction(functionItem.first, returnType);
    for (const auto &block : functionItem.second->getBasicBlocks()) {
      auto DAG = function->addMidDAG(block->getName());
      blockDAGMap.emplace(block.get(), DAG);
    }
    localArrayNameMap.emplace(function, std::map<Mid2EndPointerNode *, std::string>{});
  }

  for (const auto &externalFunctionItem : midModule->getExternalFunctions()) {
    auto midReturnType = externalFunctionItem.second->getReturnType();
    if (midReturnType == sysy::Type::getFloatType()) {
      returnType = Function::FLOAT;
    } else if (midReturnType == sysy::Type::getIntType()) {
      returnType = Function::INT;
    } else if (midReturnType == sysy::Type::getVoidType()) {
      returnType = Function::VOID;
    } else {
      assert(false);
    }
    auto externalFunction = pModule->createExternalFunction(externalFunctionItem.first, returnType);
    auto DAG = externalFunction->addMidDAG("Entry");
    localArrayNameMap.emplace(externalFunction, std::map<Mid2EndPointerNode *, std::string>{});
    for (const auto &param : externalFunctionItem.second->getEntryBlock()->getArguments()) {
      externalFunction->addParam(getOrEmplaceDAGNode(param, DAG));
    }
  }
  valueDAGNodeMap.clear();
  valueScalarOutputMap.clear();
  pointerLoadStoreMap.clear();
  valueLocalArrayMap.clear();
  inputValueMap.clear();
  valueOutputMap.clear();

  for (const auto &global : midModule->getGlobals()) {
    bool isInt = global->getType() == sysy::Type::getPointerType(sysy::Type::getIntType());
    std::list<int> dims;
    for (const auto &dim : global->getDims()) {
      dims.emplace_back(dynamic_cast<sysy::ConstantValue *>(dim->getValue())->getInt());
    }
    ConstantCounter init;
    auto &values = global->getInitValues().getValues();
    auto &numbers = global->getInitValues().getNumbers();
    if (isInt) {
      for (unsigned int i = 0; i < numbers.size(); i++) {
        init.push_back(dynamic_cast<sysy::ConstantValue *>(values[i])->getInt(), numbers[i]);
      }
    } else {
      for (unsigned int i = 0; i < numbers.size(); i++) {
        init.push_back(dynamic_cast<sysy::ConstantValue *>(values[i])->getFloat(), numbers[i]);
      }
    }
    auto globalNode = pModule->createGlobal(isInt, dims, init, global->getName());
    valueLocalArrayMap.emplace(global.get(), globalNode);
  }

  for (const auto &functionItem : midModule->getFunctions()) {
    auto function = functionItem.second.get();
    for (const auto &block : function->getBasicBlocks()) {
      auto DAG = blockDAGMap.at(block.get());
      for (const auto &pred : block->getPredecessors()) {
        DAG->addPredecessor(blockDAGMap.at(pred));
      }
      for (const auto &succ : block->getSuccessors()) {
        DAG->addSuccessor(blockDAGMap.at(succ));
      }
    }
  }
}

// todo 计时函数位置需要确定化。
// todo 可仔细考虑函数内是否对传入数组或外部全局变量进行读写操作，减少伪依赖边
// 目前的依赖判断只看原始祖先数组
// 为了简单起见，保持call指令的相对顺序
// 全局变量与call指令可能存在依赖，需要加入伪依赖边
// 由于数组可能具有隐性依赖，保守地保持同一数组的load/store/memset/call顺序,通过添加伪依赖边实现
/**
 * @brief 获取指令对应的节点类型
 *
 * @param [in] kind 指令类型
 * @return 节点类型
 */
auto Mid2EndDAGBuilder::getNodeKind(sysy::Instruction::Kind kind) -> Mid2EndDAGNode::NodeType {
  Mid2EndDAGNode::NodeType nodeType;
  switch (kind) {
    case sysy::Instruction::kAdd:
      nodeType = Mid2EndDAGNode::ADD;
      break;
    case sysy::Instruction::kSub:
      nodeType = Mid2EndDAGNode::SUB;
      break;
    case sysy::Instruction::kMul:
      nodeType = Mid2EndDAGNode::MUL;
      break;
    case sysy::Instruction::kDiv:
      nodeType = Mid2EndDAGNode::DIV;
      break;
    case sysy::Instruction::kRem:
      nodeType = Mid2EndDAGNode::REM;
      break;
    case sysy::Instruction::kICmpEQ:
      nodeType = Mid2EndDAGNode::ICMPEQ;
      break;
    case sysy::Instruction::kICmpNE:
      nodeType = Mid2EndDAGNode::ICMPNE;
      break;
    case sysy::Instruction::kICmpLT:
      nodeType = Mid2EndDAGNode::ICMPLT;
      break;
    case sysy::Instruction::kICmpGT:
      nodeType = Mid2EndDAGNode::ICMPGT;
      break;
    case sysy::Instruction::kICmpLE:
      nodeType = Mid2EndDAGNode::ICMPLE;
      break;
    case sysy::Instruction::kICmpGE:
      nodeType = Mid2EndDAGNode::ICMPGE;
      break;
    case sysy::Instruction::kFAdd:
      nodeType = Mid2EndDAGNode::FADD;
      break;
    case sysy::Instruction::kFSub:
      nodeType = Mid2EndDAGNode::FSUB;
      break;
    case sysy::Instruction::kFMul:
      nodeType = Mid2EndDAGNode::FMUL;
      break;
    case sysy::Instruction::kFDiv:
      nodeType = Mid2EndDAGNode::FDIV;
      break;
    case sysy::Instruction::kFCmpEQ:
      nodeType = Mid2EndDAGNode::FCMPEQ;
      break;
    case sysy::Instruction::kFCmpNE:
      nodeType = Mid2EndDAGNode::FCMPNE;
      break;
    case sysy::Instruction::kFCmpLT:
      nodeType = Mid2EndDAGNode::FCMPLT;
      break;
    case sysy::Instruction::kFCmpGT:
      nodeType = Mid2EndDAGNode::FCMPGT;
      break;
    case sysy::Instruction::kFCmpLE:
      nodeType = Mid2EndDAGNode::FCMPLE;
      break;
    case sysy::Instruction::kFCmpGE:
      nodeType = Mid2EndDAGNode::FCMPGE;
      break;
    case sysy::Instruction::kNeg:
      nodeType = Mid2EndDAGNode::NEG;
      break;
    case sysy::Instruction::kNot:
      nodeType = Mid2EndDAGNode::NOT;
      break;
    case sysy::Instruction::kFNeg:
      nodeType = Mid2EndDAGNode::FNEG;
      break;
    case sysy::Instruction::kFNot:
      nodeType = Mid2EndDAGNode::FNOT;
      break;
    case sysy::Instruction::kFtoI:
      nodeType = Mid2EndDAGNode::FTOI;
      break;
    case sysy::Instruction::kItoF:
      nodeType = Mid2EndDAGNode::ITOF;
      break;
    case sysy::Instruction::kCall:
      nodeType = Mid2EndDAGNode::CALL;
      break;
    case sysy::Instruction::kCondBr:
      nodeType = Mid2EndDAGNode::CONDBR;
      break;
    case sysy::Instruction::kBr:
      nodeType = Mid2EndDAGNode::BR;
      break;
    case sysy::Instruction::kReturn:
      nodeType = Mid2EndDAGNode::RETURN;
      break;
    case sysy::Instruction::kMemset:
      nodeType = Mid2EndDAGNode::MEMSET;
      break;
    case sysy::Instruction::kBitFtoI:
      nodeType = Mid2EndDAGNode::BIT_FTOI;
      break;
    case sysy::Instruction::kBitItoF:
      nodeType = Mid2EndDAGNode::BIT_ITOF;
      break;
    default:
      assert(false);
  }

  return nodeType;
}
/**
 * @brief 获取或创建DAG节点
 *
 * @param [in] value 待转换的value
 * @param [in] DAG 所处的DAG
 * @return 转换成的DAG节点
 */
auto Mid2EndDAGBuilder::getOrEmplaceDAGNode(sysy::Value *value, Mid2EndDAG *DAG) -> Mid2EndDAGNode * {
  Mid2EndDAGNode *result;
  if (valueDAGNodeMap.find(value) != valueDAGNodeMap.end()) {
    result = valueDAGNodeMap[value];
  } else {
    auto constValue = dynamic_cast<sysy::ConstantValue *>(value);
    auto local = dynamic_cast<sysy::AllocaInst *>(value);
    auto global = dynamic_cast<sysy::GlobalValue *>(value);
    auto constArray = dynamic_cast<sysy::ConstantVariable *>(value);
    auto loadInst = dynamic_cast<sysy::LoadInst *>(value);
    auto binaryInst = dynamic_cast<sysy::BinaryInst *>(value);
    auto unaryInst = dynamic_cast<sysy::UnaryInst *>(value);
    auto callInst = dynamic_cast<sysy::CallInst *>(value);
    auto laInst = dynamic_cast<sysy::LaInst *>(value);
    auto getSubArrayInst = dynamic_cast<sysy::GetSubArrayInst *>(value);
    auto storeInst = dynamic_cast<sysy::StoreInst *>(value);
    auto memsetInst = dynamic_cast<sysy::MemsetInst *>(value);
    auto condbrInst = dynamic_cast<sysy::CondBrInst *>(value);
    auto brInst = dynamic_cast<sysy::UncondBrInst *>(value);
    auto returnInst = dynamic_cast<sysy::ReturnInst *>(value);
    if (constValue != nullptr) {
      if (constValue->isFloat()) {
        result = DAG->addMidDAGNode(new Mid2EndFloatNode(constValue->getFloat(), DAG));
      } else {
        result = DAG->addMidDAGNode(new Mid2EndIntNode(constValue->getInt(), DAG));
      }
      valueDAGNodeMap.emplace(value, result);
    } else if (local != nullptr) {
      bool isInt = local->getType() == sysy::Type::getPointerType(sysy::Type::getIntType());
      if (local->getNumDims() == 0) {
        result = DAG->addMidDAGNode(new Mid2EndScalarNode(isInt, DAG));
        valueDAGNodeMap.emplace(value, result);
        valueScalarOutputMap.emplace(value, result);
        inputValueMap.emplace(dynamic_cast<Mid2EndScalarNode *>(result), value);
      } else {
        if (valueLocalArrayMap.find(local) != valueLocalArrayMap.end()) {
          result = valueLocalArrayMap.at(local);
        } else {
          if (local->getDefineInst() == nullptr) {
            std::list<int> dims;
            for (const auto &dim : local->getDims()) {
              dims.emplace_back(dynamic_cast<sysy::ConstantValue *>(dim->getValue())->getInt());
            }
            result = new Mid2EndPointerNode(isInt, dims, nullptr);
            valueLocalArrayMap.emplace(value, dynamic_cast<Mid2EndPointerNode *>(result));
            DAG->getParent()->addLocalArray(dynamic_cast<Mid2EndPointerNode *>(result));
            localArrayNameMap.at(DAG->getParent())
                .emplace(dynamic_cast<Mid2EndPointerNode *>(result), local->getName());
          } else {
            bool isOutDAGDefined = blockDAGMap.at(local->getDefineInst()->getParent()) != DAG;
            if (isOutDAGDefined) {
              auto ancestor = dynamic_cast<Mid2EndPointerNode *>(getOrEmplaceDAGNode(
                  dynamic_cast<sysy::Value *>(dynamic_cast<sysy::LVal *>(local)->getAncestorLVal()), DAG));
              std::list<int> dims;
              for (const auto &dim : local->getLValDims()) {
                dims.emplace_back(dynamic_cast<sysy::ConstantValue *>(dim)->getInt());
              }
              result = DAG->addMidDAGNode(new Mid2EndScalarNode(true, DAG, true, ancestor, dims));
              valueDAGNodeMap.emplace(value, result);
              inputValueMap.emplace(dynamic_cast<Mid2EndScalarNode *>(result), value);
            } else {
              assert(false);
            }
          }
        }
      }
    } else if (global != nullptr) {
      result = pModule->getGlobal(global->getName());
    } else if (constArray != nullptr) {
      if (pModule->getGlobals().find(constArray->getName()) != pModule->getGlobals().end()) {
        result = pModule->getGlobal(constArray->getName());
      } else {
        bool isInt = constArray->getType() == sysy::Type::getPointerType(sysy::Type::getIntType());
        std::list<int> dims;
        for (const auto &dim : constArray->getDims()) {
          dims.emplace_back(dynamic_cast<sysy::ConstantValue *>(dim->getValue())->getInt());
        }
        ConstantCounter init;
        auto values = constArray->getInitValues().getValues();
        auto numbers = constArray->getInitValues().getNumbers();
        if (isInt) {
          for (unsigned int i = 0; i < numbers.size(); i++) {
            init.push_back(dynamic_cast<sysy::ConstantValue *>(values[i])->getInt(), numbers[i]);
          }
        } else {
          for (unsigned int i = 0; i < numbers.size(); i++) {
            init.push_back(dynamic_cast<sysy::ConstantValue *>(values[i])->getFloat(), numbers[i]);
          }
        }
        result = pModule->createGlobal(isInt, dims, init, constArray->getName());
      }
    } else if (loadInst != nullptr) {
      bool isOutDAGDefined = blockDAGMap.at(loadInst->getParent()) != DAG;
      if (isOutDAGDefined) {
        bool isInt = loadInst->getType() == sysy::Type::getIntType();
        result = DAG->addMidDAGNode(new Mid2EndScalarNode(isInt, DAG));
        valueDAGNodeMap.emplace(value, result);
        inputValueMap.emplace(dynamic_cast<Mid2EndScalarNode *>(result), value);
      } else {
        auto pointer = dynamic_cast<sysy::AllocaInst *>(loadInst->getPointer());
        auto pointerNode = getOrEmplaceDAGNode(loadInst->getPointer(), DAG);
        constexpr auto mask = Mid2EndDAGNode::POINTER | Mid2EndDAGNode::GETSUBARRAY | Mid2EndDAGNode::SCALAR;
        if ((pointerNode->getNodeType() & mask) == 0 || (pointer != nullptr && pointer->getNumDims() == 0)) {
          result = pointerNode;
        } else {
          std::list<Mid2EndDAGNode *> indices;
          for (const auto &index : loadInst->getIndices()) {
            indices.emplace_back(getOrEmplaceDAGNode(index->getValue(), DAG));
          }
          Mid2EndPointerNode *midPointerNode;
          if (pointerNode->getNodeType() == Mid2EndDAGNode::POINTER) {
            midPointerNode = dynamic_cast<Mid2EndPointerNode *>(pointerNode);
          } else if (pointerNode->getNodeType() == Mid2EndDAGNode::GETSUBARRAY) {
            midPointerNode = dynamic_cast<Mid2EndGetSubArrayNode *>(pointerNode)->getAncestor();
          } else if (pointerNode->getNodeType() == Mid2EndDAGNode::SCALAR) {
            midPointerNode = dynamic_cast<Mid2EndPointerNode *>(getOrEmplaceDAGNode(
                dynamic_cast<sysy::Value *>(dynamic_cast<sysy::LVal *>(loadInst->getPointer())->getAncestorLVal()),
                DAG));
          } else {
            assert(false);
          }
          bool isFold = false;
          Mid2EndDAGNode *loadStoreNode;
          constexpr auto mask = Mid2EndDAGNode::LOAD | Mid2EndDAGNode::STORE;
          if (pointerLoadStoreMap.find(midPointerNode) != pointerLoadStoreMap.end()) {
            loadStoreNode = pointerLoadStoreMap.at(midPointerNode);
            if ((loadStoreNode->getNodeType() & mask) != 0 && loadStoreNode->getParent() == DAG) {
              if (loadStoreNode->getNodeType() == Mid2EndDAGNode::LOAD) {
                if (dynamic_cast<Mid2EndLoadNode *>(loadStoreNode)->getPointer() == pointerNode) {
                  auto alterindices = dynamic_cast<Mid2EndLoadNode *>(loadStoreNode)->getIndices();
                  auto iter = indices.begin();
                  isFold = true;
                  for (auto &alterIndex : alterindices) {
                    if (*iter != alterIndex) {
                      isFold = false;
                      break;
                    }
                    iter++;
                  }
                }
              } else {
                if (dynamic_cast<Mid2EndStoreNode *>(loadStoreNode)->getPointer() == pointerNode) {
                  auto alterindices = dynamic_cast<Mid2EndStoreNode *>(loadStoreNode)->getIndices();
                  auto iter = indices.begin();
                  isFold = true;
                  for (auto &alterIndex : alterindices) {
                    if (*iter != alterIndex) {
                      isFold = false;
                      break;
                    }
                    iter++;
                  }
                }
              }
            }
          }

          if (isFold) {
            if (loadStoreNode->getNodeType() == Mid2EndDAGNode::LOAD) {
              result = loadStoreNode;
            } else {
              result = dynamic_cast<Mid2EndStoreNode *>(loadStoreNode)->getValue();
            }
            valueDAGNodeMap.emplace(value, result);
          } else {
            result = DAG->addMidDAGNode(new Mid2EndLoadNode(pointerNode, indices, DAG));
            if (pointerLoadStoreMap.find(midPointerNode) == pointerLoadStoreMap.end()) {
              pointerLoadStoreMap.emplace(midPointerNode, result);
            } else if (result != pointerLoadStoreMap.at(midPointerNode)) {
              result->addUsed(pointerLoadStoreMap.at(midPointerNode));
              pointerLoadStoreMap.at(midPointerNode)->addUser(result);
              pointerLoadStoreMap.at(midPointerNode) = result;
            }
            valueDAGNodeMap.emplace(value, result);
          }
        }
        bool isOutDAGUsed = false;
        for (const auto &use : loadInst->getUses()) {
          if (blockDAGMap.at(dynamic_cast<sysy::Instruction *>(use->getUser())->getParent()) != DAG) {
            isOutDAGUsed = true;
            break;
          }
        }
        if (isOutDAGUsed) {
          valueScalarOutputMap.emplace(value, result);
        }
      }
    } else if (binaryInst != nullptr) {
      bool isOutDAGDefined = blockDAGMap.at(binaryInst->getParent()) != DAG;
      if (isOutDAGDefined) {
        bool isInt = binaryInst->getType() == sysy::Type::getIntType();
        result = DAG->addMidDAGNode(new Mid2EndScalarNode(isInt, DAG));
        valueDAGNodeMap.emplace(value, result);
        inputValueMap.emplace(dynamic_cast<Mid2EndScalarNode *>(result), value);
      } else {
        auto nodeType = getNodeKind(binaryInst->getKind());
        auto lhsNode = getOrEmplaceDAGNode(binaryInst->getLhs(), DAG);
        auto rhsNode = getOrEmplaceDAGNode(binaryInst->getRhs(), DAG);
        bool isFold = false;
        if (lhsNode->getNumUsers() < rhsNode->getNumUsers()) {
          for (const auto &user : lhsNode->getUsers()) {
            if (user->getNodeType() == nodeType &&
                ((user->getUsed(0) == lhsNode && user->getUsed(1) == rhsNode) ||
                 (binaryInst->isCommutative() && user->getUsed(1) == lhsNode && user->getUsed(0) == rhsNode))) {
              result = user;
              isFold = true;
              break;
            }
          }
        } else {
          for (const auto &user : rhsNode->getUsers()) {
            if (user->getNodeType() == nodeType &&
                ((user->getUsed(0) == lhsNode && user->getUsed(1) == rhsNode) ||
                 (binaryInst->isCommutative() && user->getUsed(1) == lhsNode && user->getUsed(0) == rhsNode))) {
              result = user;
              isFold = true;
              break;
            }
          }
        }
        if (!isFold) {
          result = DAG->addMidDAGNode(new Mid2EndBinaryOpNode(nodeType, lhsNode, rhsNode, DAG));
          valueDAGNodeMap.emplace(value, result);
        }

        bool isOutDAGUsed = false;
        for (const auto &use : binaryInst->getUses()) {
          if (blockDAGMap.at(dynamic_cast<sysy::Instruction *>(use->getUser())->getParent()) != DAG) {
            isOutDAGUsed = true;
            break;
          }
        }
        if (isOutDAGUsed) {
          valueScalarOutputMap.emplace(value, result);
        }
      }
    } else if (unaryInst != nullptr) {
      bool isOutDAGDefined = blockDAGMap.at(unaryInst->getParent()) != DAG;
      if (isOutDAGDefined) {
        bool isInt = unaryInst->getType() == sysy::Type::getIntType();
        result = DAG->addMidDAGNode(new Mid2EndScalarNode(isInt, DAG));
        valueDAGNodeMap.emplace(value, result);
        inputValueMap.emplace(dynamic_cast<Mid2EndScalarNode *>(result), value);
      } else {
        auto nodeType = getNodeKind(unaryInst->getKind());
        auto hsNode = getOrEmplaceDAGNode(unaryInst->getOperand(), DAG);
        bool isFold = false;
        for (const auto &user : hsNode->getUsers()) {
          if (user->getNodeType() == nodeType) {
            result = user;
            isFold = true;
            break;
          }
        }
        if (!isFold) {
          result = DAG->addMidDAGNode(new Mid2EndUnaryOpNode(nodeType, hsNode, DAG));
          valueDAGNodeMap.emplace(value, result);
        }
        bool isOutDAGUsed = false;
        for (const auto &use : unaryInst->getUses()) {
          if (blockDAGMap.at(dynamic_cast<sysy::Instruction *>(use->getUser())->getParent()) != DAG) {
            isOutDAGUsed = true;
            break;
          }
        }
        if (isOutDAGUsed) {
          valueScalarOutputMap.emplace(value, result);
        }
      }
    } else if (callInst != nullptr) {
      bool isOutDAGDefined = blockDAGMap.at(callInst->getParent()) != DAG;
      if (isOutDAGDefined) {
        bool isInt = callInst->getType() == sysy::Type::getIntType();
        result = DAG->addMidDAGNode(new Mid2EndScalarNode(isInt, DAG));
        valueDAGNodeMap.emplace(value, result);
        inputValueMap.emplace(dynamic_cast<Mid2EndScalarNode *>(result), value);
      } else {
        std::list<Mid2EndDAGNode *> params;
        for (const auto &param : callInst->getArguments()) {
          params.emplace_back(getOrEmplaceDAGNode(param->getValue(), DAG));
        }
        auto callee = callInst->getCallee();
        result = DAG->addMidDAGNode(new Mid2EndCallNode(pModule->getFunction(callee->getName()), params, DAG));
        for (const auto &paramNode : params) {
          Mid2EndPointerNode *pointerNode = nullptr;
          if (paramNode->getNodeType() == Mid2EndDAGNode::GETSUBARRAY) {
            pointerNode = dynamic_cast<Mid2EndGetSubArrayNode *>(paramNode)->getAncestor();
          } else if (paramNode->getNodeType() == Mid2EndDAGNode::LA) {
            pointerNode = dynamic_cast<Mid2EndLaNode *>(paramNode)->getPointer();
          } else if (paramNode->getNodeType() == Mid2EndDAGNode::SCALAR) {
            auto value = inputValueMap.at(dynamic_cast<Mid2EndScalarNode *>(paramNode));
            auto laInst = dynamic_cast<sysy::LaInst *>(value);
            if (laInst != nullptr) {
              pointerNode = dynamic_cast<Mid2EndPointerNode *>(getOrEmplaceDAGNode(laInst->getPointer(), DAG));
            }
          }
          if (pointerNode != nullptr) {
            if (pointerLoadStoreMap.find(pointerNode) == pointerLoadStoreMap.end()) {
              pointerLoadStoreMap.emplace(pointerNode, result);
            } else if (result != pointerLoadStoreMap.at(pointerNode)) {
              result->addUsed(pointerLoadStoreMap.at(pointerNode));
              pointerLoadStoreMap.at(pointerNode)->addUser(result);
              pointerLoadStoreMap.at(pointerNode) = result;
            }
          }
        }
        if (pModule->getExternalFunctions().find(callee->getName()) == pModule->getExternalFunctions().end()) {
          for (const auto &globalItem : DAG->getParent()->getParent()->getGlobals()) {
            auto pointerNode = dynamic_cast<Mid2EndPointerNode *>(globalItem.second.get());
            if (pointerLoadStoreMap.find(pointerNode) == pointerLoadStoreMap.end()) {
              pointerLoadStoreMap.emplace(pointerNode, result);
            } else if (pointerLoadStoreMap.at(pointerNode) != result) {
              result->addUsed(pointerLoadStoreMap.at(pointerNode));
              pointerLoadStoreMap.at(pointerNode)->addUser(result);
              pointerLoadStoreMap.at(pointerNode) = result;
            }
          }
        }
        if (lastCallNode == nullptr) {
          lastCallNode = dynamic_cast<Mid2EndCallNode *>(result);
        } else {
          result->addUsed(lastCallNode);
          lastCallNode->addUser(result);
          lastCallNode = dynamic_cast<Mid2EndCallNode *>(result);
        }
        valueDAGNodeMap.emplace(value, result);
        bool isOutDAGUsed = false;
        for (const auto &use : callInst->getUses()) {
          if (blockDAGMap.at(dynamic_cast<sysy::Instruction *>(use->getUser())->getParent()) != DAG) {
            isOutDAGUsed = true;
            break;
          }
        }
        if (isOutDAGUsed) {
          valueScalarOutputMap.emplace(value, result);
        }
      }

    } else if (laInst != nullptr) {
      bool isOutDAGDefined = blockDAGMap.at(laInst->getParent()) != DAG;
      if (isOutDAGDefined) {
        result = DAG->addMidDAGNode(new Mid2EndScalarNode(true, DAG));
        valueDAGNodeMap.emplace(value, result);
        inputValueMap.emplace(dynamic_cast<Mid2EndScalarNode *>(result), value);
      } else {
        std::list<Mid2EndDAGNode *> indices;
        for (const auto &index : laInst->getIndices()) {
          indices.emplace_back(getOrEmplaceDAGNode(index->getValue(), DAG));
        }
        auto pointerNode = dynamic_cast<Mid2EndPointerNode *>(getOrEmplaceDAGNode(laInst->getPointer(), DAG));
        bool isFold = false;
        Mid2EndLaNode *alterLaNode;
        for (const auto &node : DAG->getMidDAGNodes()) {
          if (node->getNodeType() == Mid2EndDAGNode::LA &&
              dynamic_cast<Mid2EndLaNode *>(node.get())->getPointer() == pointerNode) {
            alterLaNode = dynamic_cast<Mid2EndLaNode *>(node.get());
            auto alterIndices = alterLaNode->getIndices();
            if (alterIndices.size() == indices.size()) {
              isFold = true;
              auto iter = indices.begin();
              for (unsigned int i = 0; i < indices.size(); i++) {
                if (*iter != alterIndices[i]) {
                  isFold = false;
                  break;
                }
                iter++;
              }
            }
          }
        }
        if (isFold) {
          result = alterLaNode;
          valueDAGNodeMap.emplace(value, alterLaNode);
        } else {
          result = DAG->addMidDAGNode(new Mid2EndLaNode(pointerNode, indices, DAG));
          valueDAGNodeMap.emplace(value, result);
        }
        bool isOutDAGUsed = false;
        for (const auto &use : laInst->getUses()) {
          if (blockDAGMap.at(dynamic_cast<sysy::Instruction *>(use->getUser())->getParent()) != DAG) {
            isOutDAGUsed = true;
            break;
          }
        }
        if (isOutDAGUsed) {
          valueScalarOutputMap.emplace(value, result);
        }
      }
    } else if (getSubArrayInst != nullptr) {
      std::list<Mid2EndDAGNode *> indices;
      for (const auto &index : getSubArrayInst->getIndices()) {
        indices.emplace_back(getOrEmplaceDAGNode(index->getValue(), DAG));
      }
      auto pointerNode = getOrEmplaceDAGNode(getSubArrayInst->getFatherArray(), DAG);
      bool isFold = false;
      Mid2EndGetSubArrayNode *alterGetSubArrayNode;
      for (const auto &node : DAG->getMidDAGNodes()) {
        if (node->getNodeType() == Mid2EndDAGNode::GETSUBARRAY &&
            dynamic_cast<Mid2EndGetSubArrayNode *>(node.get())->getPointer() == pointerNode) {
          alterGetSubArrayNode = dynamic_cast<Mid2EndGetSubArrayNode *>(node.get());
          auto alterIndices = alterGetSubArrayNode->getIndices();
          if (alterIndices.size() == indices.size()) {
            isFold = true;
            auto iter = indices.begin();
            for (unsigned int i = 0; i < indices.size(); i++) {
              if (*iter != alterIndices[i]) {
                isFold = false;
                break;
              }
              iter++;
            }
          }
        }
      }
      if (isFold) {
        result = alterGetSubArrayNode;
        valueDAGNodeMap.emplace(getSubArrayInst->getChildArray(), alterGetSubArrayNode);
      } else {
        auto ancestor = dynamic_cast<Mid2EndPointerNode *>(
            getOrEmplaceDAGNode(dynamic_cast<sysy::Value *>(getSubArrayInst->getFatherLVal()->getAncestorLVal()), DAG));
        result = DAG->addMidDAGNode(new Mid2EndGetSubArrayNode(pointerNode, indices, ancestor, DAG));
        valueDAGNodeMap.emplace(getSubArrayInst->getChildArray(), result);
      }

      bool isOutDAGUsed = false;
      for (const auto &use : getSubArrayInst->getChildArray()->getUses()) {
        if (blockDAGMap.at(dynamic_cast<sysy::Instruction *>(use->getUser())->getParent()) != DAG) {
          isOutDAGUsed = true;
          break;
        }
      }
      if (isOutDAGUsed) {
        valueScalarOutputMap.emplace(getSubArrayInst->getChildArray(), result);
      }
    } else if (storeInst != nullptr) {
      auto pointerNode = getOrEmplaceDAGNode(storeInst->getPointer(), DAG);
      auto valueNode = getOrEmplaceDAGNode(storeInst->getValue(), DAG);
      std::list<Mid2EndDAGNode *> indices;
      for (const auto &index : storeInst->getIndices()) {
        indices.emplace_back(getOrEmplaceDAGNode(index->getValue(), DAG));
      }
      if (pointerNode->getNodeType() == Mid2EndDAGNode::POINTER) {
        auto midPointerNode = dynamic_cast<Mid2EndPointerNode *>(pointerNode);
        result = DAG->addMidDAGNode(new Mid2EndStoreNode(midPointerNode, valueNode, indices, DAG));
        if (pointerLoadStoreMap.find(midPointerNode) == pointerLoadStoreMap.end()) {
          pointerLoadStoreMap.emplace(midPointerNode, result);
        } else if (result != pointerLoadStoreMap.at(midPointerNode)) {
          result->addUsed(pointerLoadStoreMap.at(midPointerNode));
          pointerLoadStoreMap.at(midPointerNode)->addUser(result);
          pointerLoadStoreMap.at(midPointerNode) = result;
        }
        valueDAGNodeMap.emplace(value, result);
      } else if (pointerNode->getNodeType() == Mid2EndDAGNode::GETSUBARRAY) {
        auto midSubArrayNode = dynamic_cast<Mid2EndGetSubArrayNode *>(pointerNode);
        result = DAG->addMidDAGNode(new Mid2EndStoreNode(midSubArrayNode, valueNode, indices, DAG));
        auto midPointerNode = midSubArrayNode->getAncestor();
        if (pointerLoadStoreMap.find(midPointerNode) == pointerLoadStoreMap.end()) {
          pointerLoadStoreMap.emplace(midPointerNode, result);
        } else if (result != pointerLoadStoreMap.at(midPointerNode)) {
          result->addUsed(pointerLoadStoreMap.at(midPointerNode));
          pointerLoadStoreMap.at(midPointerNode)->addUser(result);
          pointerLoadStoreMap.at(midPointerNode) = result;
        }
        valueDAGNodeMap.emplace(value, result);
      } else if (pointerNode->getNodeType() == Mid2EndDAGNode::SCALAR &&
                 dynamic_cast<Mid2EndScalarNode *>(pointerNode)->is64Scalar()) {
        result = DAG->addMidDAGNode(new Mid2EndStoreNode(pointerNode, valueNode, indices, DAG));
        auto midPointerNode = dynamic_cast<Mid2EndPointerNode *>(getOrEmplaceDAGNode(
            dynamic_cast<sysy::Value *>(dynamic_cast<sysy::LVal *>(storeInst->getPointer())->getAncestorLVal()), DAG));
        if (pointerLoadStoreMap.find(midPointerNode) == pointerLoadStoreMap.end()) {
          pointerLoadStoreMap.emplace(midPointerNode, result);
        } else if (result != pointerLoadStoreMap.at(midPointerNode)) {
          result->addUsed(pointerLoadStoreMap.at(midPointerNode));
          pointerLoadStoreMap.at(midPointerNode)->addUser(result);
          pointerLoadStoreMap.at(midPointerNode) = result;
        }
        valueDAGNodeMap.emplace(value, result);
      } else {
        valueDAGNodeMap.at(storeInst->getPointer()) = valueNode;
        valueScalarOutputMap.at(storeInst->getPointer()) = valueNode;
        result = valueNode;
      }
    } else if (memsetInst != nullptr) {
      auto pointerNode = getOrEmplaceDAGNode(memsetInst->getPointer(), DAG);
      Mid2EndPointerNode *midPointerNode;
      if (pointerNode->getNodeType() == Mid2EndDAGNode::POINTER) {
        midPointerNode = dynamic_cast<Mid2EndPointerNode *>(pointerNode);
      } else if (pointerNode->getNodeType() == Mid2EndDAGNode::GETSUBARRAY) {
        midPointerNode = dynamic_cast<Mid2EndGetSubArrayNode *>(pointerNode)->getAncestor();
      } else {
        assert(false);
      }
      auto valueNode = getOrEmplaceDAGNode(memsetInst->getValue(), DAG);
      unsigned begin = dynamic_cast<sysy::ConstantValue *>(memsetInst->getBegin())->getInt();
      unsigned size = dynamic_cast<sysy::ConstantValue *>(memsetInst->getSize())->getInt();
      result = DAG->addMidDAGNode(new Mid2EndMemsetNode(pointerNode, valueNode, begin, size, DAG));
      if (pointerLoadStoreMap.find(midPointerNode) == pointerLoadStoreMap.end()) {
        pointerLoadStoreMap.emplace(midPointerNode, result);
      } else if (result != pointerLoadStoreMap.at(midPointerNode)) {
        result->addUsed(pointerLoadStoreMap.at(midPointerNode));
        pointerLoadStoreMap.at(midPointerNode)->addUser(result);
        pointerLoadStoreMap.at(midPointerNode) = result;
      }
      valueDAGNodeMap.emplace(value, result);
    } else if (condbrInst != nullptr) {
      auto condNode = getOrEmplaceDAGNode(condbrInst->getCondition(), DAG);
      result = DAG->addMidDAGNode(new Mid2EndCondBrNode(condNode, blockDAGMap.at(condbrInst->getThenBlock()),
                                                        blockDAGMap.at(condbrInst->getElseBlock()), DAG));
      valueDAGNodeMap.emplace(value, result);
      DAG->setTerminatorNode(result);
    } else if (brInst != nullptr) {
      result = DAG->addMidDAGNode(new Mid2EndBrNode(blockDAGMap.at(brInst->getBlock()), DAG));
      valueDAGNodeMap.emplace(value, result);
      DAG->setTerminatorNode(result);
    } else if (returnInst != nullptr) {
      auto returnValue = returnInst->getReturnValue();
      if (returnValue != nullptr) {
        auto valueNode = getOrEmplaceDAGNode(returnInst->getReturnValue(), DAG);
        result = DAG->addMidDAGNode(new Mid2EndReturnNode(valueNode, DAG));
        valueDAGNodeMap.emplace(value, result);
      } else {
        result = DAG->addMidDAGNode(new Mid2EndReturnNode(nullptr, DAG));
        valueDAGNodeMap.emplace(value, result);
      }
      DAG->setTerminatorNode(result);
    } else {
      assert(false);
    }
  }
  return result;
}
/**
 * @brief 构建DAG
 *
 * @param [in] midModule 待转换的中端IR模块
 * @return 无返回值
 */
void Mid2EndDAGBuilder::buildDAG(sysy::Module *midModule) {
  for (const auto &functionItem : midModule->getFunctions()) {
    auto function = functionItem.second.get();
    for (const auto &block : function->getBasicBlocks()) {
      auto DAG = blockDAGMap.at(block.get());
      valueOutputMap.emplace(DAG, std::map<sysy::Value *, Mid2EndDAGNode *>{});
      for (const auto &inst : block->getInstructions()) {
        getOrEmplaceDAGNode(inst.get(), DAG);
      }
      calOutputNode(DAG);
      valueDAGNodeMap.clear();
      valueScalarOutputMap.clear();
      pointerLoadStoreMap.clear();
      lastCallNode = nullptr;
    }
    calInputNode();
    initParams(functionItem.second.get());
    bindInputOutput(pModule->getFunction(function->getName()));

    valueLocalArrayMap.clear();
    inputValueMap.clear();
    valueOutputMap.clear();
  }
}
/**
 * @brief 计算输入节点
 *
 * @return 无返回值
 */
void Mid2EndDAGBuilder::calInputNode() {
  for (const auto inputValueItem : inputValueMap) {
    auto DAG = inputValueItem.first->getParent();
    DAG->inputNodes.insert(inputValueItem.first);
  }
}
/**
 * @brief 计算输出节点
 *
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGBuilder::calOutputNode(Mid2EndDAG *DAG) {
  for (const auto &valueScalarItem : valueScalarOutputMap) {
    DAG->outputNodes.insert(valueScalarItem.second);
    valueOutputMap.at(DAG).emplace(valueScalarItem);
  }
}
/**
 * @brief 初始化函数参数
 *
 * @param [in] function 待处理的函数
 * @return 无返回值
 */
void Mid2EndDAGBuilder::initParams(sysy::Function *function) {
  auto DAGFunction = pModule->getFunction(function->getName());
  auto entryDAG = DAGFunction->getEntryDAG();
  std::map<sysy::Value *, Mid2EndScalarNode *> valueInputMap;
  for (const auto &input : entryDAG->inputNodes) {
    valueInputMap.emplace(inputValueMap.at(input), input);
  }
  for (const auto &param : function->getEntryBlock()->getArguments()) {
    if (param->getNumDims() == 0) {
      DAGFunction->addParam(valueInputMap.at(param));
    } else {
      DAGFunction->addParam(valueLocalArrayMap.at(param));
    }
  }
}
/**
 * @brief 绑定输入输出节点
 *
 * @param [in] function 待处理的函数
 * @return 无返回值
 */
void Mid2EndDAGBuilder::bindInputOutput(Function *function) {
  localScalarNameMap.emplace(function, std::map<uint64_t, std::string>{});
  std::set<sysy::Value *> values;
  std::map<sysy::Value *, uint64_t> valueNumberMap;
  for (const auto &inputValueItem : inputValueMap) {
    values.insert(inputValueItem.second);
  }
  uint64_t number = 0;
  for (const auto &value : values) {
    valueNumberMap.emplace(value, number);
    if (dynamic_cast<sysy::AllocaInst *>(value) != nullptr) {
      localScalarNameMap.at(function).emplace(number, value->getName());
    } else {
      std::stringstream ss;
      ss << "T" << value->getName();
      localScalarNameMap.at(function).emplace(number, ss.str());
    }
    number++;
  }
  for (const auto &value : values) {
    if (value->getType() == sysy::Type::getIntType() ||
        value->getType() == sysy::Type::getPointerType(sysy::Type::getIntType())) {
      function->varTypeMap.emplace(valueNumberMap.at(value), 0);
    } else {
      function->varTypeMap.emplace(valueNumberMap.at(value), 2);
    }
  }

  for (const auto &inputValueItem : inputValueMap) {
    function->inputBind.emplace(inputValueItem.first, valueNumberMap.at(inputValueItem.second));
  }

  for (const auto &DAG : function->getMidDAGs()) {
    auto &valueOutputMapInDAG = valueOutputMap.at(DAG.get());
    for (const auto &valueOutputItem : valueOutputMapInDAG) {
      if (valueNumberMap.find(valueOutputItem.first) != valueNumberMap.end()) {
        if (function->outputBind.find(valueOutputItem.second) == function->outputBind.end()) {
          function->outputBind.emplace(valueOutputItem.second, std::set<uint64_t>{});
        }
        function->outputBind.at(valueOutputItem.second).insert(valueNumberMap.at(valueOutputItem.first));
      }
    }
  }
}
/**
 * @brief 获取int常量节点
 *
 * @param [in] value int常量
 * @param [in] DAG 所处的DAG
 * @return int常量节点
 */
auto Mid2EndDAGSimplifier::getOrEmplaceConstant(int value, Mid2EndDAG *DAG) -> Mid2EndIntNode * {
  auto &intConstants = DAG->getIntConstants();
  if (intConstants.find(value) != intConstants.end()) {
    return intConstants.at(value);
  }

  return dynamic_cast<Mid2EndIntNode *>(DAG->addMidDAGNode(new Mid2EndIntNode(value, DAG)));
}
/**
 * @brief 获取float常量节点
 *
 * @param [in] value float常量
 * @param [in] DAG 所处的DAG
 * @return float常量节点
 */
auto Mid2EndDAGSimplifier::getOrEmplaceConstant(float value, Mid2EndDAG *DAG) -> Mid2EndFloatNode * {
  auto &floatConstants = DAG->getFloatConstants();
  if (floatConstants.find(value) != floatConstants.end()) {
    return floatConstants.at(value);
  }

  return dynamic_cast<Mid2EndFloatNode *>(DAG->addMidDAGNode(new Mid2EndFloatNode(value, DAG)));
}
/**
 * @brief 获取uint64常量节点
 *
 * @param [in] value uint64常量
 * @param [in] DAG 所处的DAG
 * @return uint64常量节点
 */
auto Mid2EndDAGSimplifier::getOrEmplaceUint64(uint64_t value, Mid2EndDAG *DAG) -> Mid2EndUint64Node * {
  auto &uint64Constants = DAG->getUint64Constants();
  if (uint64Constants.find(value) != uint64Constants.end()) {
    return uint64Constants.at(value);
  }
  return dynamic_cast<Mid2EndUint64Node *>(DAG->addMidDAGNode(new Mid2EndUint64Node(value, DAG)));
}
/**
 * @brief 获取操作数类型
 *
 * @param [in] node 操作数节点
 * @return 操作数类型
 */
auto Mid2EndDAGSimplifier::getOperandType(Mid2EndDAGNode *node) -> unsigned int {
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::SCALAR: {
      if (dynamic_cast<Mid2EndScalarNode *>(node)->isIntScalar()) {
        return 0;
      }
      return 1;
    }
    case Mid2EndDAGNode::CALL: {
      auto callNode = dynamic_cast<Mid2EndCallNode *>(node);
      auto returnType = callNode->getCallee()->getReturnType();
      if (returnType == Function::INT) {
        return 0;
      }
      if (returnType == Function::FLOAT) {
        return 1;
      }
      return 2;
    }
    case Mid2EndDAGNode::LOAD: {
      auto loadNode = dynamic_cast<Mid2EndLoadNode *>(node);
      auto pointer = loadNode->getAncestorPointer();
      if (pointer->isIntPointer()) {
        return 0;
      }
      return 1;
    }
    case Mid2EndDAGNode::RVLOAD: {
      auto rvLoadNode = dynamic_cast<Mid2EndRVLoadNode *>(node);
      if (rvLoadNode->isIntValue()) {
        return 0;
      }
      return 1;
    }
    case Mid2EndDAGNode::MIN: {
      auto minNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
      return getOperandType(minNode->getLhs());
    }
    case Mid2EndDAGNode::MAX: {
      auto maxNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
      return getOperandType(maxNode->getLhs());
    }
    case Mid2EndDAGNode::INT:
    case Mid2EndDAGNode::ADD:
    case Mid2EndDAGNode::SUB:
    case Mid2EndDAGNode::MUL:
    case Mid2EndDAGNode::DIV:
    case Mid2EndDAGNode::REM:
    case Mid2EndDAGNode::ICMPEQ:
    case Mid2EndDAGNode::ICMPNE:
    case Mid2EndDAGNode::ICMPLT:
    case Mid2EndDAGNode::ICMPGT:
    case Mid2EndDAGNode::ICMPLE:
    case Mid2EndDAGNode::ICMPGE:
    case Mid2EndDAGNode::FCMPEQ:
    case Mid2EndDAGNode::FCMPNE:
    case Mid2EndDAGNode::FCMPLT:
    case Mid2EndDAGNode::FCMPGT:
    case Mid2EndDAGNode::FCMPLE:
    case Mid2EndDAGNode::FCMPGE:
    case Mid2EndDAGNode::NEG:
    case Mid2EndDAGNode::NOT:
    case Mid2EndDAGNode::FNOT:
    case Mid2EndDAGNode::FTOI:
    case Mid2EndDAGNode::FEQ:
    case Mid2EndDAGNode::FLT:
    case Mid2EndDAGNode::FLE:
    case Mid2EndDAGNode::FNZERO:
    case Mid2EndDAGNode::BIT_FTOI:
      return 0;
    case Mid2EndDAGNode::FLOAT:
    case Mid2EndDAGNode::FADD:
    case Mid2EndDAGNode::FSUB:
    case Mid2EndDAGNode::FMUL:
    case Mid2EndDAGNode::FDIV:
    case Mid2EndDAGNode::FNEG:
    case Mid2EndDAGNode::ITOF:
    case Mid2EndDAGNode::FMADD:
    case Mid2EndDAGNode::FMSUB:
    case Mid2EndDAGNode::FNMADD:
    case Mid2EndDAGNode::FNMSUB:
    case Mid2EndDAGNode::BIT_ITOF:
      return 1;
    case Mid2EndDAGNode::CONDBR:
    case Mid2EndDAGNode::BR:
    case Mid2EndDAGNode::RETURN:
    case Mid2EndDAGNode::STORE:
    case Mid2EndDAGNode::RVSTORE:
    case Mid2EndDAGNode::MEMSET:
    case Mid2EndDAGNode::BEQ:
    case Mid2EndDAGNode::BNE:
    case Mid2EndDAGNode::BLT:
    case Mid2EndDAGNode::BGE:
    case Mid2EndDAGNode::BLTU:
    case Mid2EndDAGNode::BGEU:
      return 2;
    case Mid2EndDAGNode::POINTER:
    case Mid2EndDAGNode::LA:
    case Mid2EndDAGNode::GETSUBARRAY:
    case Mid2EndDAGNode::UINT64:
    case Mid2EndDAGNode::ADD64:
    case Mid2EndDAGNode::MUL64:
    case Mid2EndDAGNode::SEXTW:
      return 3;
  }
}
/**
 * @brief 合并常量节点
 *
 * @param [in] node 待合并的节点
 * @param [in] DAG 所处的DAG
 * @param [in] function 所处的function
 * @param [out] deletedNodes 待删除的节点集合
 * @param [out] addedNodes 新增的可能可合并节点集合
 * @return 是否被合并
 */
auto Mid2EndDAGSimplifier::constantCombNode(Mid2EndDAGNode *node, Mid2EndDAG *DAG, Function *function,
                                            std::set<Mid2EndDAGNode *> &deletedNodes,
                                            std::set<Mid2EndDAGNode *> &addedNodes) -> bool {
  constexpr auto uint64BinaryMask = Mid2EndDAGNode::ADD64 | Mid2EndDAGNode::MUL64;
  constexpr auto intBinaryMask = Mid2EndDAGNode::ADD | Mid2EndDAGNode::SUB | Mid2EndDAGNode::MUL | Mid2EndDAGNode::DIV |
                                 Mid2EndDAGNode::REM | Mid2EndDAGNode::ICMPEQ | Mid2EndDAGNode::ICMPNE |
                                 Mid2EndDAGNode::ICMPLT | Mid2EndDAGNode::ICMPLE | Mid2EndDAGNode::ICMPGT |
                                 Mid2EndDAGNode::ICMPGE;
  constexpr auto floatBinaryMask = Mid2EndDAGNode::FADD | Mid2EndDAGNode::FSUB | Mid2EndDAGNode::FMUL |
                                   Mid2EndDAGNode::FDIV | Mid2EndDAGNode::FCMPEQ | Mid2EndDAGNode::FCMPNE |
                                   Mid2EndDAGNode::FCMPLT | Mid2EndDAGNode::FCMPLE | Mid2EndDAGNode::FCMPGT |
                                   Mid2EndDAGNode::FCMPGE;
  constexpr auto intUnaryMask = Mid2EndDAGNode::NEG | Mid2EndDAGNode::NOT | Mid2EndDAGNode::FTOI |
                                Mid2EndDAGNode::SEXTW | Mid2EndDAGNode::BIT_FTOI;
  constexpr auto floatUnaryMask =
      Mid2EndDAGNode::FNEG | Mid2EndDAGNode::FNOT | Mid2EndDAGNode::ITOF | Mid2EndDAGNode::BIT_ITOF;
  constexpr auto mask = uint64BinaryMask | intBinaryMask | floatBinaryMask | intUnaryMask | floatUnaryMask;
  bool changed = false;
  Mid2EndDAGNode *result = nullptr;
  if ((node->getNodeType() & intBinaryMask) != 0) {
    auto intBinaryNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
    auto lhsNode = intBinaryNode->getLhs();
    auto rhsNode = intBinaryNode->getRhs();
    if (lhsNode->getNodeType() == Mid2EndDAGNode::INT && rhsNode->getNodeType() == Mid2EndDAGNode::INT) {
      switch (node->getNodeType()) {
        case Mid2EndDAGNode::ADD: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() + dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt(),
              DAG);
          break;
        }
        case Mid2EndDAGNode::SUB: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() - dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt(),
              DAG);
          break;
        }
        case Mid2EndDAGNode::MUL: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() * dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt(),
              DAG);
          break;
        }
        case Mid2EndDAGNode::DIV: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() / dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt(),
              DAG);
          break;
        }
        case Mid2EndDAGNode::REM: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() % dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt(),
              DAG);
          break;
        }
        case Mid2EndDAGNode::ICMPEQ: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() == dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt()
                  ? 1
                  : 0,
              DAG);
          break;
        }
        case Mid2EndDAGNode::ICMPNE: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() != dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt()
                  ? 1
                  : 0,
              DAG);
          break;
        }
        case Mid2EndDAGNode::ICMPLT: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() < dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt() ? 1
                                                                                                                    : 0,
              DAG);
          break;
        }
        case Mid2EndDAGNode::ICMPLE: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() <= dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt()
                  ? 1
                  : 0,
              DAG);
          break;
        }
        case Mid2EndDAGNode::ICMPGT: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() > dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt() ? 1
                                                                                                                    : 0,
              DAG);
          break;
        }
        case Mid2EndDAGNode::ICMPGE: {
          result = getOrEmplaceConstant(
              dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt() >= dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt()
                  ? 1
                  : 0,
              DAG);
          break;
        }
        default:
          assert(false);
      }
      for (const auto &user : intBinaryNode->getUsers()) {
        if ((user->getNodeType() & mask) != 0) {
          addedNodes.insert(user);
        }
        user->replaceUsed(intBinaryNode, result);
        result->addUser(user);
      }
      changed = true;
    }
  } else if ((node->getNodeType() & floatBinaryMask) != 0) {
    auto floatBinaryNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
    auto lhsNode = floatBinaryNode->getLhs();
    auto rhsNode = floatBinaryNode->getRhs();
    if (lhsNode->getNodeType() == Mid2EndDAGNode::FLOAT && rhsNode->getNodeType() == Mid2EndDAGNode::FLOAT) {
      switch (node->getNodeType()) {
        case Mid2EndDAGNode::FADD: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() +
                                            dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat(),
                                        DAG);
          break;
        }
        case Mid2EndDAGNode::FSUB: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() -
                                            dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat(),
                                        DAG);
          break;
        }
        case Mid2EndDAGNode::FMUL: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() *
                                            dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat(),
                                        DAG);
          break;
        }
        case Mid2EndDAGNode::FDIV: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() /
                                            dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat(),
                                        DAG);
          break;
        }
        case Mid2EndDAGNode::FCMPEQ: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() ==
                                                dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat()
                                            ? 1
                                            : 0,
                                        DAG);
          break;
        }
        case Mid2EndDAGNode::FCMPNE: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() !=
                                                dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat()
                                            ? 1
                                            : 0,
                                        DAG);
          break;
        }
        case Mid2EndDAGNode::FCMPLT: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() <
                                                dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat()
                                            ? 1
                                            : 0,
                                        DAG);
          break;
        }
        case Mid2EndDAGNode::FCMPLE: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() <=
                                                dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat()
                                            ? 1
                                            : 0,
                                        DAG);
          break;
        }
        case Mid2EndDAGNode::FCMPGT: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() >
                                                dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat()
                                            ? 1
                                            : 0,
                                        DAG);
          break;
        }
        case Mid2EndDAGNode::FCMPGE: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhsNode)->getFloat() >=
                                                dynamic_cast<Mid2EndFloatNode *>(rhsNode)->getFloat()
                                            ? 1
                                            : 0,
                                        DAG);
          break;
        }
        default:
          assert(false);
      }
      for (const auto &user : floatBinaryNode->getUsers()) {
        if ((user->getNodeType() & mask) != 0) {
          addedNodes.insert(user);
        }
        user->replaceUsed(floatBinaryNode, result);
        result->addUser(user);
      }
      changed = true;
    }
  } else if ((node->getNodeType() & intUnaryMask) != 0) {
    auto intUnaryNode = dynamic_cast<Mid2EndUnaryOpNode *>(node);
    auto hsNode = intUnaryNode->getHs();
    if (hsNode->getNodeType() == Mid2EndDAGNode::INT) {
      switch (node->getNodeType()) {
        case Mid2EndDAGNode::NEG: {
          result = getOrEmplaceConstant(-dynamic_cast<Mid2EndIntNode *>(hsNode)->getInt(), DAG);
          break;
        }
        case Mid2EndDAGNode::NOT: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndIntNode *>(hsNode)->getInt() == 0 ? 1 : 0, DAG);
          break;
        }
        case Mid2EndDAGNode::ITOF: {
          result = getOrEmplaceConstant(static_cast<float>(dynamic_cast<Mid2EndIntNode *>(hsNode)->getInt()), DAG);
          break;
        }
        case Mid2EndDAGNode::SEXTW: {
          result = getOrEmplaceUint64(static_cast<uint64_t>(dynamic_cast<Mid2EndIntNode *>(hsNode)->getInt()), DAG);
          break;
        }
        case Mid2EndDAGNode::BIT_ITOF: {
          int operand = dynamic_cast<Mid2EndIntNode *>(hsNode)->getInt();
          result = getOrEmplaceConstant(*reinterpret_cast<float *>(&operand), DAG);
        }
        default:
          assert(false);
      }

      for (const auto &user : intUnaryNode->getUsers()) {
        if ((user->getNodeType() & mask) != 0) {
          addedNodes.insert(user);
        }
        user->replaceUsed(intUnaryNode, result);
        result->addUser(user);
      }
      changed = true;
    }
  } else if ((node->getNodeType() & floatUnaryMask) != 0) {
    auto floatUnaryNode = dynamic_cast<Mid2EndUnaryOpNode *>(node);
    auto hsNode = floatUnaryNode->getHs();
    if (hsNode->getNodeType() == Mid2EndDAGNode::FLOAT) {
      switch (node->getNodeType()) {
        case Mid2EndDAGNode::FNEG: {
          result = getOrEmplaceConstant(-dynamic_cast<Mid2EndFloatNode *>(hsNode)->getFloat(), DAG);
          break;
        }
        case Mid2EndDAGNode::FNOT: {
          result = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(hsNode)->getFloat() == 0.0F ? 1 : 0, DAG);
          break;
        }
        case Mid2EndDAGNode::FTOI: {
          result = getOrEmplaceConstant(static_cast<int>(dynamic_cast<Mid2EndFloatNode *>(hsNode)->getFloat()), DAG);
          break;
        }
        case Mid2EndDAGNode::BIT_FTOI: {
          float operand = dynamic_cast<Mid2EndFloatNode *>(hsNode)->getFloat();
          result = getOrEmplaceConstant(*reinterpret_cast<int *>(&operand), DAG);
        }
        default:
          assert(false);
      }

      for (const auto &user : floatUnaryNode->getUsers()) {
        if ((user->getNodeType() & mask) != 0) {
          addedNodes.insert(user);
        }
        user->replaceUsed(floatUnaryNode, result);
        result->addUser(user);
      }
      changed = true;
    }
  } else if ((node->getNodeType() & uint64BinaryMask) != 0) {
    auto uint64BinaryNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
    auto lhsNode = uint64BinaryNode->getLhs();
    auto rhsNode = uint64BinaryNode->getRhs();
    auto lhsNodeType = lhsNode->getNodeType();
    auto rhsNodeType = rhsNode->getNodeType();
    uint64_t constLhs;
    uint64_t constRhs;
    bool isReady = true;
    if (lhsNodeType == Mid2EndDAGNode::UINT64) {
      constLhs = dynamic_cast<Mid2EndUint64Node *>(lhsNode)->getUint64();
    } else if (lhsNodeType == Mid2EndDAGNode::INT) {
      constLhs = static_cast<int64_t>(dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt());
    } else {
      isReady = false;
    }
    if (isReady) {
      if (rhsNodeType == Mid2EndDAGNode::UINT64) {
        constRhs = dynamic_cast<Mid2EndUint64Node *>(rhsNode)->getUint64();
      } else if (rhsNodeType == Mid2EndDAGNode::INT) {
        constRhs = static_cast<int64_t>(dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt());
      } else {
        isReady = false;
      }
    }
    if (isReady) {
      switch (node->getNodeType()) {
        case Mid2EndDAGNode::ADD64: {
          result = getOrEmplaceUint64(constLhs + constRhs, DAG);
          break;
        }
        case Mid2EndDAGNode::MUL64: {
          result = getOrEmplaceUint64(constLhs * constRhs, DAG);
          break;
        }
        default:
          assert(false);
      }
      for (const auto &user : uint64BinaryNode->getUsers()) {
        if ((user->getNodeType() & mask) != 0) {
          addedNodes.insert(user);
        }
        user->replaceUsed(uint64BinaryNode, result);
        result->addUser(user);
      }
      changed = true;
    }
  } else {
    assert(false);
  }
  if (changed) {
    deletedNodes.insert(node);
    if (DAG->getOutputNodes().find(node) != DAG->getOutputNodes().end()) {
      DAG->outputNodes.erase(node);
      DAG->outputNodes.insert(result);
      auto vars = function->outputBind.at(node);
      function->outputBind.erase(node);
      if (function->outputBind.find(result) != function->outputBind.end()) {
        function->outputBind.at(result).merge(vars);
      } else {
        function->outputBind.emplace(result, vars);
      }
    }
    DAG->removeMidDAGNode(node);
  }
  return changed;
}
/**
 * @brief 常量合并
 *
 * @return 无返回值
 */
void Mid2EndDAGSimplifier::constantComb() {
  constexpr auto mask = Mid2EndDAGNode::ADD | Mid2EndDAGNode::SUB | Mid2EndDAGNode::MUL | Mid2EndDAGNode::DIV |
                        Mid2EndDAGNode::REM | Mid2EndDAGNode::ICMPEQ | Mid2EndDAGNode::ICMPNE | Mid2EndDAGNode::ICMPLT |
                        Mid2EndDAGNode::ICMPLE | Mid2EndDAGNode::ICMPGT | Mid2EndDAGNode::ICMPGE |
                        Mid2EndDAGNode::FADD | Mid2EndDAGNode::FSUB | Mid2EndDAGNode::FMUL | Mid2EndDAGNode::FDIV |
                        Mid2EndDAGNode::FCMPEQ | Mid2EndDAGNode::FCMPNE | Mid2EndDAGNode::FCMPLT |
                        Mid2EndDAGNode::FCMPLE | Mid2EndDAGNode::FCMPGT | Mid2EndDAGNode::FCMPGE | Mid2EndDAGNode::NEG |
                        Mid2EndDAGNode::NOT | Mid2EndDAGNode::FNEG | Mid2EndDAGNode::FNOT | Mid2EndDAGNode::FTOI |
                        Mid2EndDAGNode::ITOF | Mid2EndDAGNode::ADD64 | Mid2EndDAGNode::MUL64 | Mid2EndDAGNode::SEXTW |
                        Mid2EndDAGNode::BIT_FTOI | Mid2EndDAGNode::BIT_ITOF;
  for (const auto &functionItem : pModule->getFunctions()) {
    for (const auto &DAG : functionItem.second->getMidDAGs()) {
      std::set<Mid2EndDAGNode *> constantUsedNode;
      for (const auto &intItem : DAG->getIntConstants()) {
        for (const auto &user : intItem.second->getUsers()) {
          if ((user->getNodeType() & mask) != 0) {
            constantUsedNode.insert(user);
          }
        }
      }
      for (const auto &floatItem : DAG->getFloatConstants()) {
        for (const auto &user : floatItem.second->getUsers()) {
          if ((user->getNodeType() & mask) != 0) {
            constantUsedNode.insert(user);
          }
        }
      }
      for (const auto &uint64Item : DAG->getUint64Constants()) {
        for (const auto &user : uint64Item.second->getUsers()) {
          if ((user->getNodeType() & mask) != 0) {
            constantUsedNode.insert(user);
          }
        }
      }
      bool changed;
      do {
        changed = false;
        std::set<Mid2EndDAGNode *> deletedNodes;
        std::set<Mid2EndDAGNode *> addedNodes;
        for (const auto &node : constantUsedNode) {
          if (constantCombNode(node, DAG.get(), functionItem.second.get(), deletedNodes, addedNodes)) {
            changed = true;
          }
        }
        for (const auto &node : addedNodes) {
          constantUsedNode.insert(node);
        }
        for (const auto &node : deletedNodes) {
          constantUsedNode.erase(node);
        }
      } while (changed);
    }
  }
  deleteUselessNodes();
}
/**
 * @brief 删除死DAG
 *
 * @return 无返回值
 */
void Mid2EndDAGSimplifier::deletDeadDAGs() {
  std::map<Mid2EndDAG *, bool> isAddedtoVisitStack;
  std::stack<Mid2EndDAG *> visitStack;
  for (const auto &functionItem : pModule->getFunctions()) {
    std::list<Mid2EndDAG *> DAGList;
    for (const auto &DAG : functionItem.second->getMidDAGs()) {
      isAddedtoVisitStack.emplace(DAG.get(), false);
      DAGList.emplace_back(DAG.get());
    }
    auto entryDAG = functionItem.second->getEntryDAG();
    visitStack.push(entryDAG);
    isAddedtoVisitStack.at(entryDAG) = true;
    while (!visitStack.empty()) {
      auto curDAG = visitStack.top();
      visitStack.pop();
      for (const auto &succ : curDAG->getSuccessors()) {
        if (!isAddedtoVisitStack[succ]) {
          visitStack.push(succ);
          isAddedtoVisitStack[succ] = true;
        }
      }
    }

    for (const auto &DAG : DAGList) {
      if (!isAddedtoVisitStack[DAG]) {
        functionItem.second->removeMidDAG(DAG);
      }
    }
  }
}

// 需要插入新的块，并重命名、重排序块
// 条件可能会跨块，导致只能做部分合并
// 可能会产生死块，需要将其删除,避免其影响块排序
// 删除条件时，保守不删除Call
/**
 * @brief 合并跳转
 *
 * @return 无返回值
 */
void Mid2EndDAGSimplifier::combCondBr() {
  uint64_t begin = 0;
  for (const auto &functionItem : pModule->getFunctions()) {
    for (const auto &DAG : functionItem.second->getMidDAGs()) {
      auto terminator = DAG->getTerminatorNode();
      if (terminator != nullptr && terminator->getNodeType() == Mid2EndDAGNode::CONDBR) {
        auto condBrNode = dynamic_cast<Mid2EndCondBrNode *>(terminator);
        auto cond = condBrNode->getCond();
        switch (cond->getNodeType()) {
          case Mid2EndDAGNode::INT: {
            auto intValue = dynamic_cast<Mid2EndIntNode *>(cond)->getInt();
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(condBrNode);
            Mid2EndDAGNode *result;
            if (intValue != 0) {
              result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              if (thenDAG != elseDAG) {
                elseDAG->removePredecessor(DAG.get());
                DAG->removeSuccessor(elseDAG);
              }
            } else {
              result = DAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, DAG.get()));
              if (thenDAG != elseDAG) {
                thenDAG->removePredecessor(DAG.get());
                DAG->removeSuccessor(thenDAG);
              }
            }
            DAG->setTerminatorNode(result);
            break;
          }
          case Mid2EndDAGNode::FLOAT: {
            auto floatValue = dynamic_cast<Mid2EndFloatNode *>(cond)->getFloat();
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(condBrNode);
            Mid2EndDAGNode *result;
            if (floatValue != 0.0F) {
              result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              if (thenDAG != elseDAG) {
                elseDAG->removePredecessor(DAG.get());
                DAG->removeSuccessor(elseDAG);
              }
            } else {
              result = DAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, DAG.get()));
              if (thenDAG != elseDAG) {
                thenDAG->removePredecessor(DAG.get());
                DAG->removeSuccessor(thenDAG);
              }
            }
            DAG->setTerminatorNode(result);
            break;
          }
          case Mid2EndDAGNode::ICMPEQ: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto lhsNode = condNode->getLhs();
              auto rhsNode = condNode->getRhs();
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(lhsNode, rhsNode, thenDAG, Mid2EndDAGNode::BEQ, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::ICMPNE: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto lhsNode = condNode->getLhs();
              auto rhsNode = condNode->getRhs();
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(lhsNode, rhsNode, thenDAG, Mid2EndDAGNode::BNE, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::ICMPLT: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto lhsNode = condNode->getLhs();
              auto rhsNode = condNode->getRhs();
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(lhsNode, rhsNode, thenDAG, Mid2EndDAGNode::BLT, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::ICMPGT: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto lhsNode = condNode->getLhs();
              auto rhsNode = condNode->getRhs();
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(rhsNode, lhsNode, thenDAG, Mid2EndDAGNode::BLT, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::ICMPLE: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto lhsNode = condNode->getLhs();
              auto rhsNode = condNode->getRhs();
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(rhsNode, lhsNode, thenDAG, Mid2EndDAGNode::BGE, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::ICMPGE: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto lhsNode = condNode->getLhs();
              auto rhsNode = condNode->getRhs();
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(lhsNode, rhsNode, thenDAG, Mid2EndDAGNode::BGE, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::FCMPEQ: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto lhsNode = condNode->getLhs();
            auto rhsNode = condNode->getRhs();
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto newCondNode = getOrEmplaceBinaryNode(Mid2EndDAGNode::FEQ, lhsNode, rhsNode, DAG.get());
              auto zero = getOrEmplaceConstant(0, DAG.get());
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(zero, newCondNode, thenDAG, Mid2EndDAGNode::BLTU, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::FCMPNE: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto lhsNode = condNode->getLhs();
            auto rhsNode = condNode->getRhs();
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto newCondNode = getOrEmplaceBinaryNode(Mid2EndDAGNode::FEQ, lhsNode, rhsNode, DAG.get());
              auto zero = getOrEmplaceConstant(0, DAG.get());
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(zero, newCondNode, thenDAG, Mid2EndDAGNode::BGEU, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::FCMPLT: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto lhsNode = condNode->getLhs();
            auto rhsNode = condNode->getRhs();
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto newCondNode = getOrEmplaceBinaryNode(Mid2EndDAGNode::FLT, lhsNode, rhsNode, DAG.get());
              auto zero = getOrEmplaceConstant(0, DAG.get());
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(zero, newCondNode, thenDAG, Mid2EndDAGNode::BLTU, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::FCMPGT: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto lhsNode = condNode->getLhs();
            auto rhsNode = condNode->getRhs();
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto newCondNode = getOrEmplaceBinaryNode(Mid2EndDAGNode::FLT, rhsNode, lhsNode, DAG.get());
              auto zero = getOrEmplaceConstant(0, DAG.get());
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(zero, newCondNode, thenDAG, Mid2EndDAGNode::BLTU, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::FCMPLE: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto lhsNode = condNode->getLhs();
            auto rhsNode = condNode->getRhs();
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto newCondNode = getOrEmplaceBinaryNode(Mid2EndDAGNode::FLE, lhsNode, rhsNode, DAG.get());
              auto zero = getOrEmplaceConstant(0, DAG.get());
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(zero, newCondNode, thenDAG, Mid2EndDAGNode::BLTU, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::FCMPGE: {
            auto condNode = dynamic_cast<Mid2EndBinaryOpNode *>(cond);
            auto lhsNode = condNode->getLhs();
            auto rhsNode = condNode->getRhs();
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto newCondNode = getOrEmplaceBinaryNode(Mid2EndDAGNode::FLT, rhsNode, lhsNode, DAG.get());
              auto zero = getOrEmplaceConstant(0, DAG.get());
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(zero, newCondNode, thenDAG, Mid2EndDAGNode::BLTU, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::NOT: {
            auto condNode = dynamic_cast<Mid2EndUnaryOpNode *>(cond);
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto hsNode = condNode->getHs();
              auto zero = getOrEmplaceConstant(0, DAG.get());
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(zero, hsNode, thenDAG, Mid2EndDAGNode::BGEU, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          case Mid2EndDAGNode::FNOT: {
            auto condNode = dynamic_cast<Mid2EndUnaryOpNode *>(cond);
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            if (thenDAG != elseDAG) {
              auto newNode = getOrEmplaceUnaryNode(Mid2EndDAGNode::FNZERO, condNode->getHs(), DAG.get());
              auto zero = getOrEmplaceConstant(0, DAG.get());
              auto result = DAG->addMidDAGNode(
                  new Mid2EndRiscvCondBrNode(zero, newNode, thenDAG, Mid2EndDAGNode::BGEU, DAG.get()));
              DAG->setTerminatorNode(result);

              auto newDAG = DAG->getParent()->addMidDAG("");
              result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
              newDAG->setTerminatorNode(result);

              DAG->removeSuccessor(elseDAG);
              elseDAG->removePredecessor(DAG.get());
              DAG->addSuccessor(newDAG);
              newDAG->addPredecessor(DAG.get());
              newDAG->addSuccessor(elseDAG);
              elseDAG->addPredecessor(newDAG);
            } else {
              auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
              DAG->setTerminatorNode(result);
            }
            break;
          }
          default: {
            Mid2EndDAGNode *result;
            auto thenDAG = condBrNode->getThenDAG();
            auto elseDAG = condBrNode->getElseDAG();
            DAG->removeMidDAGNode(terminator);
            auto condType = getOperandType(cond);
            auto zero = getOrEmplaceConstant(0, DAG.get());
            if (condType == 0) {
              if (thenDAG != elseDAG) {
                result = DAG->addMidDAGNode(
                    new Mid2EndRiscvCondBrNode(zero, cond, thenDAG, Mid2EndDAGNode::BLTU, DAG.get()));
                DAG->setTerminatorNode(result);

                auto newDAG = DAG->getParent()->addMidDAG("");
                result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
                newDAG->setTerminatorNode(result);

                DAG->removeSuccessor(elseDAG);
                elseDAG->removePredecessor(DAG.get());
                DAG->addSuccessor(newDAG);
                newDAG->addPredecessor(DAG.get());
                newDAG->addSuccessor(elseDAG);
                elseDAG->addPredecessor(newDAG);
              } else {
                if (cond->getNumUsers() == 0 && DAG->getOutputNodes().find(cond) == DAG->getOutputNodes().end()) {
                  if (cond->getNodeType() != Mid2EndDAGNode::CALL) {
                    DAG->removeMidDAGNode(cond);
                  }
                }
                auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
                DAG->setTerminatorNode(result);
              }
            } else if (condType == 1) {
              if (thenDAG != elseDAG) {
                auto newNode = getOrEmplaceUnaryNode(Mid2EndDAGNode::FNZERO, cond, DAG.get());
                result = DAG->addMidDAGNode(
                    new Mid2EndRiscvCondBrNode(zero, newNode, thenDAG, Mid2EndDAGNode::BLTU, DAG.get()));
                DAG->setTerminatorNode(result);

                auto newDAG = DAG->getParent()->addMidDAG("");
                result = newDAG->addMidDAGNode(new Mid2EndBrNode(elseDAG, newDAG));
                newDAG->setTerminatorNode(result);

                DAG->removeSuccessor(elseDAG);
                elseDAG->removePredecessor(DAG.get());
                DAG->addSuccessor(newDAG);
                newDAG->addPredecessor(DAG.get());
                newDAG->addSuccessor(elseDAG);
                elseDAG->addPredecessor(newDAG);
              } else {
                if (cond->getNumUsers() == 0 && DAG->getOutputNodes().find(cond) == DAG->getOutputNodes().end()) {
                  if (cond->getNodeType() != Mid2EndDAGNode::CALL) {
                    DAG->removeMidDAGNode(cond);
                  }
                }
                auto result = DAG->addMidDAGNode(new Mid2EndBrNode(thenDAG, DAG.get()));
                DAG->setTerminatorNode(result);
              }
            } else {
              assert(false);
            }
            break;
          }
        }
      }
    }
    functionItem.second->sortDAGs();
    begin = functionItem.second->renameDAGs(begin);
  }
  deletDeadDAGs();
  deleteUselessNodes();
}
/**
 * @brief 自动机运行
 *
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void FInstReduceDFA::act(Mid2EndDAG *DAG) {
  auto &outputNodes = DAG->getOutputNodes();
  bool isFinished = false;
  while (!isFinished) {
    switch (state) {
      case Mul: {
        switch (nextNode->getNodeType()) {
          case Mid2EndDAGNode::FNEG: {
            state = NegMul;
            break;
          }
          case Mid2EndDAGNode::FADD: {
            auto addNode = dynamic_cast<Mid2EndBinaryOpNode *>(nextNode);
            if (addNode->getLhs() != prevNode) {
              operands.emplace_back(addNode->getLhs());
            } else {
              operands.emplace_back(addNode->getRhs());
            }
            state = FMADD;
            opType = state;
            break;
          }
          case Mid2EndDAGNode::FSUB: {
            auto subNode = dynamic_cast<Mid2EndBinaryOpNode *>(nextNode);
            if (subNode->getLhs() != prevNode) {
              operands.emplace_back(subNode->getLhs());
              state = FNMADD;
            } else {
              operands.emplace_back(subNode->getRhs());
              state = FMSUB;
            }
            opType = state;
            break;
          }
          default: {
            state = END;
            break;
          }
        }
        break;
      }
      case NegMul: {
        switch (nextNode->getNodeType()) {
          case Mid2EndDAGNode::FNEG: {
            state = Mul;
            break;
          }
          case Mid2EndDAGNode::FADD: {
            auto addNode = dynamic_cast<Mid2EndBinaryOpNode *>(nextNode);
            if (addNode->getLhs() != prevNode) {
              operands.emplace_back(addNode->getLhs());
            } else {
              operands.emplace_back(addNode->getRhs());
            }
            state = FNMADD;
            opType = state;
            break;
          }
          case Mid2EndDAGNode::FSUB: {
            auto subNode = dynamic_cast<Mid2EndBinaryOpNode *>(nextNode);
            if (subNode->getLhs() != prevNode) {
              operands.emplace_back(subNode->getLhs());
              state = FMADD;
            } else {
              operands.emplace_back(subNode->getRhs());
              state = FNMSUB;
            }
            opType = state;
            break;
          }
          default: {
            state = END;
            break;
          }
        }
        break;
      }
      case FMADD: {
        if (nextNode->getNodeType() == Mid2EndDAGNode::FNEG) {
          state = FNMSUB;
        } else {
          state = END;
        }
        break;
      }
      case FMSUB: {
        if (nextNode->getNodeType() == Mid2EndDAGNode::FNEG) {
          state = FNMADD;
        } else {
          state = END;
        }
        break;
      }
      case FNMADD: {
        if (nextNode->getNodeType() == Mid2EndDAGNode::FNEG) {
          state = FMSUB;
        } else {
          state = END;
        }
        break;
      }
      case FNMSUB: {
        if (nextNode->getNodeType() == Mid2EndDAGNode::FNEG) {
          state = FMADD;
        } else {
          state = END;
        }
        break;
      }
      case END: {
        isFinished = true;
        break;
      }
    }
    if (state != END) {
      if (prevNode->getNumUsers() == 1 && outputNodes.find(prevNode) == outputNodes.end()) {
        reducedNodes.emplace_back(nextNode);
      } else {
        state = END;
      }
    }
    prevNode = nextNode;
  }
}

// 默认不输入指针和全局变量
// 仅用于删除无用节点
/**
 * @brief 删除一个节点
 *
 * @param [in] node 待删除节点
 * @return 是否被删除
 */
auto Mid2EndDAGSimplifier::deleteOneNode(Mid2EndDAGNode *node) -> bool {
  auto DAG = node->getParent();
  auto function = DAG->getParent();
  auto entryDAG = function->getEntryDAG();
  auto &params = function->getParams();
  assert(DAG != nullptr);
  assert(function != nullptr);
  if (node->getNumUsers() == 0 && DAG->outputNodes.find(node) == DAG->outputNodes.end() &&
      (DAG != entryDAG || std::find(params.begin(), params.end(), node) == params.end())) {
    DAG->removeMidDAGNode(node);
    function->inputBind.erase(node);
    function->outputBind.erase(node);
    return true;
  }
  return false;
}
/**
 * @brief 删除无用节点
 *
 * @return 无返回值
 */
void Mid2EndDAGSimplifier::deleteUselessNodes() {
  constexpr auto mask = Mid2EndDAGNode::POINTER | Mid2EndDAGNode::CALL | Mid2EndDAGNode::CONDBR | Mid2EndDAGNode::BR |
                        Mid2EndDAGNode::RETURN | Mid2EndDAGNode::STORE | Mid2EndDAGNode::MEMSET | Mid2EndDAGNode::BEQ |
                        Mid2EndDAGNode::BNE | Mid2EndDAGNode::BLT | Mid2EndDAGNode::BGE | Mid2EndDAGNode::BLTU |
                        Mid2EndDAGNode::BGEU | Mid2EndDAGNode::RVSTORE;
  for (const auto &functionItem : pModule->getFunctions()) {
    for (const auto &DAG : functionItem.second->getMidDAGs()) {
      std::list<Mid2EndDAGNode *> toDeleteNodes;
      for (const auto &node : DAG->getMidDAGNodes()) {
        if ((node->getNodeType() & mask) == 0) {
          toDeleteNodes.emplace_back(node.get());
        }
      }
      bool deleted;
      do {
        deleted = false;
        auto iter = toDeleteNodes.begin();
        while (iter != toDeleteNodes.end()) {
          auto node = *iter;
          if (deleteOneNode(node)) {
            deleted = true;
            iter = toDeleteNodes.erase(iter);
          } else {
            iter++;
          }
        }
      } while (deleted);
    }
  }
}

// 内联和化乘为除可能带来更多的合并可能
/**
 * @brief 浮点算术运算合并
 *
 * @return 无返回值
 * @warning 可能带来精度问题
 */
void Mid2EndDAGSimplifier::combFloatArithmeticOp() {
  FInstReduceDFA dfa;
  for (const auto &fucntionItem : pModule->getFunctions()) {
    for (const auto &DAG : fucntionItem.second->getMidDAGs()) {
      std::set<Mid2EndBinaryOpNode *> fmulNodes;
      for (const auto &node : DAG->getMidDAGNodes()) {
        if (node->getNodeType() == Mid2EndDAGNode::FMUL) {
          fmulNodes.insert(dynamic_cast<Mid2EndBinaryOpNode *>(node.get()));
        }
      }
      for (const auto &fmulNode : fmulNodes) {
        dfa.init(fmulNode);
        dfa.act(DAG.get());
        if (dfa.isCanReduced()) {
          auto &reducedNodes = dfa.getReducedNodes();
          auto iter = reducedNodes.begin();
          while (iter != std::prev(reducedNodes.end())) {
            DAG->removeMidDAGNode(*iter);
            iter++;
          }
          auto &operands = dfa.getOperands();
          Mid2EndDAGNode *result = nullptr;
          switch (dfa.getOpType()) {
            case FInstReduceDFA::FMADD:
              result = DAG->addMidDAGNode(
                  new Mid2EndF3OpNode(Mid2EndDAGNode::FMADD, operands[0], operands[1], operands[2], DAG.get()));
              break;
            case FInstReduceDFA::FNMADD:
              result = DAG->addMidDAGNode(
                  new Mid2EndF3OpNode(Mid2EndDAGNode::FNMADD, operands[0], operands[1], operands[2], DAG.get()));
              break;
            case FInstReduceDFA::FMSUB:
              result = DAG->addMidDAGNode(
                  new Mid2EndF3OpNode(Mid2EndDAGNode::FMSUB, operands[0], operands[1], operands[2], DAG.get()));
              break;
            case FInstReduceDFA::FNMSUB:
              result = DAG->addMidDAGNode(
                  new Mid2EndF3OpNode(Mid2EndDAGNode::FNMSUB, operands[0], operands[1], operands[2], DAG.get()));
              break;
            default:
              assert(false);
          }

          auto backNode = reducedNodes.back();
          for (const auto &user : backNode->getUsers()) {
            user->replaceUsed(backNode, result);
            result->addUser(user);
          }

          auto &outputNodes = DAG->getOutputNodes();
          if (outputNodes.find(backNode) != outputNodes.end()) {
            DAG->outputNodes.erase(backNode);
            DAG->outputNodes.insert(result);
            auto vars = fucntionItem.second->outputBind.at(backNode);
            fucntionItem.second->outputBind.erase(backNode);
            if (fucntionItem.second->outputBind.find(result) != fucntionItem.second->outputBind.end()) {
              fucntionItem.second->outputBind.at(result).merge(vars);
            } else {
              fucntionItem.second->outputBind.emplace(result, vars);
            }
          }
          DAG->removeMidDAGNode(backNode);
        }
        dfa.clear();
      }
    }
  }
}
/**
 * @brief 获取或创建二元节点
 *
 * @param [in] nodeType 节点类型
 * @param [in] lhsNode 左操作数节点
 * @param [in] rhsNode 右操作数节点
 * @param [in] DAG 所处的DAG
 * @return 二元节点
 */
auto Mid2EndDAGSimplifier::getOrEmplaceBinaryNode(Mid2EndDAGNode::NodeType nodeType, Mid2EndDAGNode *lhsNode,
                                                  Mid2EndDAGNode *rhsNode, Mid2EndDAG *DAG) -> Mid2EndBinaryOpNode * {
  constexpr auto mask = Mid2EndDAGNode::ADD | Mid2EndDAGNode::MUL | Mid2EndDAGNode::ICMPEQ | Mid2EndDAGNode::ICMPNE |
                        Mid2EndDAGNode::FADD | Mid2EndDAGNode::FMUL | Mid2EndDAGNode::FCMPEQ | Mid2EndDAGNode::FCMPNE |
                        Mid2EndDAGNode::Mid2EndDAGNode::MIN | Mid2EndDAGNode::MAX | Mid2EndDAGNode::FEQ |
                        Mid2EndDAGNode::ADD64 | Mid2EndDAGNode::MUL64;
  bool isFold = false;
  Mid2EndBinaryOpNode *result;
  if (lhsNode->getNumUsers() < rhsNode->getNumUsers()) {
    for (const auto &user : lhsNode->getUsers()) {
      if (user->getNodeType() == nodeType &&
          ((user->getUsed(0) == lhsNode && user->getUsed(1) == rhsNode) ||
           ((nodeType & mask) != 0 && user->getUsed(1) == lhsNode && user->getUsed(0) == rhsNode))) {
        result = dynamic_cast<Mid2EndBinaryOpNode *>(user);
        isFold = true;
        break;
      }
    }
  } else {
    for (const auto &user : rhsNode->getUsers()) {
      if (user->getNodeType() == nodeType &&
          ((user->getUsed(0) == lhsNode && user->getUsed(1) == rhsNode) ||
           ((nodeType & mask) != 0 && user->getUsed(1) == lhsNode && user->getUsed(0) == rhsNode))) {
        result = dynamic_cast<Mid2EndBinaryOpNode *>(user);
        isFold = true;
        break;
      }
    }
  }
  if (!isFold) {
    result = dynamic_cast<Mid2EndBinaryOpNode *>(
        DAG->addMidDAGNode(new Mid2EndBinaryOpNode(nodeType, lhsNode, rhsNode, DAG)));
  }

  return result;
}
/**
 * @brief 获取或创建一元节点
 *
 * @param [in] nodeType 节点类型
 * @param [in] hsNode 操作数节点
 * @param [in] DAG 所处的DAG
 * @return 一元节点
 */
auto Mid2EndDAGSimplifier::getOrEmplaceUnaryNode(Mid2EndDAGNode::NodeType nodeType, Mid2EndDAGNode *hsNode,
                                                 Mid2EndDAG *DAG) -> Mid2EndUnaryOpNode * {
  Mid2EndUnaryOpNode *result;
  bool isFold = false;
  for (const auto &user : hsNode->getUsers()) {
    if (user->getNodeType() == nodeType) {
      result = dynamic_cast<Mid2EndUnaryOpNode *>(user);
      isFold = true;
      break;
    }
  }
  if (!isFold) {
    result = dynamic_cast<Mid2EndUnaryOpNode *>(DAG->addMidDAGNode(new Mid2EndUnaryOpNode(nodeType, hsNode, DAG)));
  }
  return result;
}

/**
 * @brief 计算索引
 *
 * @param [in] dims 维度列表
 * @param [in] indices 索引列表
 * @param [in] DAG 所处的DAG
 * @return 计算结果节点
 */
template <typename ContainerT>
auto Mid2EndDAGSimplifier::calculateIndices(std::list<int> dims, const ContainerT &indices, Mid2EndDAG *DAG)
    -> Mid2EndDAGNode * {
  if (indices.empty()) {
    return getOrEmplaceUint64(0, DAG);
  }
  auto dimIter = std::next(dims.begin());
  Mid2EndDAGNode *offset;
  uint64_t constantOffset;
  bool isConstant = false;
  if (indices.front()->getNodeType() == Mid2EndDAGNode::INT) {
    isConstant = true;
    constantOffset = static_cast<uint64_t>(dynamic_cast<Mid2EndIntNode *>(indices.front())->getInt());
  } else {
    offset = indices.front();
  }
  auto indexIter = indices.begin();
  for (unsigned int i = 1; i < dims.size(); i++) {
    if (isConstant) {
      constantOffset = constantOffset * static_cast<uint64_t>(*dimIter);
    } else {
      auto dimNode = getOrEmplaceUint64(static_cast<uint64_t>(*dimIter), DAG);
      offset = getOrEmplaceBinaryNode(Mid2EndDAGNode::MUL64, dimNode, offset, DAG);
    }
    if (i < indices.size()) {
      indexIter++;
      if (isConstant && (*indexIter)->getNodeType() == Mid2EndDAGNode::INT) {
        constantOffset = static_cast<uint64_t>(dynamic_cast<Mid2EndIntNode *>(*indexIter)->getInt()) + constantOffset;
      } else if (isConstant && (*indexIter)->getNodeType() != Mid2EndDAGNode::INT) {
        offset = getOrEmplaceUint64(constantOffset, DAG);
        auto index = *indexIter;
        offset = getOrEmplaceBinaryNode(Mid2EndDAGNode::ADD64, offset, index, DAG);
        isConstant = false;
      } else if (!isConstant && (*indexIter)->getNodeType() == Mid2EndDAGNode::INT) {
        auto index =
            getOrEmplaceUint64(static_cast<uint64_t>(dynamic_cast<Mid2EndIntNode *>(*indexIter)->getInt()), DAG);
        offset = getOrEmplaceBinaryNode(Mid2EndDAGNode::ADD64, offset, index, DAG);
      } else {
        auto index = *indexIter;
        offset = getOrEmplaceBinaryNode(Mid2EndDAGNode::ADD64, offset, index, DAG);
      }
    }
    dimIter++;
  }

  if (isConstant) {
    offset = getOrEmplaceUint64(4 * constantOffset, DAG);
  } else {
    offset = getOrEmplaceBinaryNode(Mid2EndDAGNode::MUL64, offset, getOrEmplaceUint64(4, DAG), DAG);
  }
  return offset;
}
// 将索引计算单独分离出来
/**
 * @brief 索引计算展开
 *
 * @return 无返回值
 */
void Mid2EndDAGSimplifier::expandIndicesNodes() {
  constexpr auto mask = Mid2EndDAGNode::LOAD | Mid2EndDAGNode::STORE | Mid2EndDAGNode::GETSUBARRAY | Mid2EndDAGNode::LA;
  for (const auto &functionItem : pModule->getFunctions()) {
    for (const auto &DAG : functionItem.second->getMidDAGs()) {
      std::map<Mid2EndDAGNode *, std::list<int>> nodeDimsMap;
      std::map<Mid2EndLoadNode *, bool> loadIntMap;
      for (const auto &node : DAG->getMidDAGNodes()) {
        if ((node->getNodeType() & mask) != 0) {
          Mid2EndDAGNode *pointer;
          switch (node->getNodeType()) {
            case Mid2EndDAGNode::LOAD: {
              auto loadNode = dynamic_cast<Mid2EndLoadNode *>(node.get());
              pointer = loadNode->getPointer();
              loadIntMap.emplace(loadNode, loadNode->getAncestorPointer()->isIntPointer());
              break;
            }
            case Mid2EndDAGNode::STORE: {
              auto storeNode = dynamic_cast<Mid2EndStoreNode *>(node.get());
              pointer = storeNode->getPointer();
              break;
            }
            case Mid2EndDAGNode::LA: {
              auto getLaNode = dynamic_cast<Mid2EndLaNode *>(node.get());
              pointer = getLaNode->getPointer();
              break;
            }
            case Mid2EndDAGNode::GETSUBARRAY: {
              auto getSubArrayNode = dynamic_cast<Mid2EndGetSubArrayNode *>(node.get());
              pointer = getSubArrayNode->getPointer();
              break;
            }
            default:
              assert(false);
          }
          std::list<int> dims;
          if (pointer->getNodeType() == Mid2EndDAGNode::GETSUBARRAY) {
            dims = dynamic_cast<Mid2EndGetSubArrayNode *>(pointer)->getDims();
          } else if (pointer->getNodeType() == Mid2EndDAGNode::POINTER) {
            dims = dynamic_cast<Mid2EndPointerNode *>(pointer)->getDims();
          } else if (pointer->getNodeType() == Mid2EndDAGNode::SCALAR) {
            dims = dynamic_cast<Mid2EndScalarNode *>(pointer)->getDims();
          }
          nodeDimsMap.emplace(node.get(), dims);
        }
      }

      for (const auto &nodeDimsItem : nodeDimsMap) {
        auto node = nodeDimsItem.first;
        auto dims = nodeDimsItem.second;
        Mid2EndDAGNode *result = nullptr;
        switch (node->getNodeType()) {
          case Mid2EndDAGNode::LOAD: {
            auto loadNode = dynamic_cast<Mid2EndLoadNode *>(node);
            auto pointer = loadNode->getPointer();
            auto offset = calculateIndices(dims, loadNode->getIndices(), DAG.get());
            result = DAG->addMidDAGNode(new Mid2EndRVLoadNode(pointer, offset, loadIntMap.at(loadNode), DAG.get()));
            for (const auto &user : loadNode->getUsers()) {
              user->replaceUsed(loadNode, result);
              result->addUser(user);
            }
            auto iter = std::next(loadNode->getUseds().begin(), 1 + loadNode->getNumIndices());
            while (iter != loadNode->getUseds().end()) {
              result->addUsed(*iter);
              (*iter)->addUser(result);
              iter++;
            }
            if (DAG->outputNodes.find(loadNode) != DAG->outputNodes.end()) {
              auto function = DAG->getParent();
              DAG->outputNodes.insert(result);
              auto vars = function->outputBind.at(loadNode);
              function->outputBind.erase(loadNode);
              if (function->outputBind.find(result) != function->outputBind.end()) {
                function->outputBind.at(result).merge(vars);
              } else {
                function->outputBind.emplace(result, vars);
              }
            }
            DAG->removeMidDAGNode(loadNode);
            break;
          }
          case Mid2EndDAGNode::STORE: {
            auto storeNode = dynamic_cast<Mid2EndStoreNode *>(node);
            auto pointer = storeNode->getPointer();
            auto value = storeNode->getValue();
            auto offset = calculateIndices(dims, storeNode->getIndices(), DAG.get());
            result = DAG->addMidDAGNode(new Mid2EndRVStoreNode(pointer, value, offset, DAG.get()));
            for (const auto &user : storeNode->getUsers()) {
              user->replaceUsed(storeNode, result);
              result->addUser(user);
            }

            auto iter = std::next(storeNode->getUseds().begin(), 2 + storeNode->getNumIndices());
            while (iter != storeNode->getUseds().end()) {
              result->addUsed(*iter);
              (*iter)->addUser(result);
              iter++;
            }
            DAG->removeMidDAGNode(storeNode);
            break;
          }
          case Mid2EndDAGNode::LA: {
            auto laNode = dynamic_cast<Mid2EndLaNode *>(node);
            auto pointer = laNode->getPointer();
            auto offset = calculateIndices(pointer->getDims(), laNode->getIndices(), DAG.get());
            // 引入冗余的ADD64 result,0,pointer ,使得pointer局部化
            // if (offset->getNodeType() == Mid2EndDAGNode::UINT64 &&
            //     dynamic_cast<Mid2EndUint64Node *>(offset)->getUint64() == 0) {
            //   result = pointer;
            // } else {}
            result = getOrEmplaceBinaryNode(Mid2EndDAGNode::ADD64, pointer, offset, DAG.get());
            for (const auto &user : laNode->getUsers()) {
              user->replaceUsed(laNode, result);
              result->addUser(user);
            }

            if (DAG->outputNodes.find(laNode) != DAG->outputNodes.end()) {
              auto function = DAG->getParent();
              DAG->outputNodes.insert(result);
              auto vars = function->outputBind.at(laNode);
              function->outputBind.erase(laNode);
              if (function->outputBind.find(result) != function->outputBind.end()) {
                function->outputBind.at(result).merge(vars);
              } else {
                function->outputBind.emplace(result, vars);
              }
            }
            DAG->removeMidDAGNode(laNode);
            break;
          }
          case Mid2EndDAGNode::GETSUBARRAY: {
            auto getSubArrayNode = dynamic_cast<Mid2EndGetSubArrayNode *>(node);
            // 引入冗余的ADD64 result,0,pointer ,使得pointer局部化
            auto pointer = getSubArrayNode->getPointer();
            auto offset = calculateIndices(dims, getSubArrayNode->getIndices(), DAG.get());
            result = getOrEmplaceBinaryNode(Mid2EndDAGNode::ADD64, pointer, offset, DAG.get());
            for (const auto &user : getSubArrayNode->getUsers()) {
              user->replaceUsed(getSubArrayNode, result);
              result->addUser(user);
            }

            if (DAG->outputNodes.find(getSubArrayNode) != DAG->outputNodes.end()) {
              auto function = DAG->getParent();
              DAG->outputNodes.insert(result);
              auto vars = function->outputBind.at(getSubArrayNode);
              function->outputBind.erase(getSubArrayNode);
              if (function->outputBind.find(result) != function->outputBind.end()) {
                function->outputBind.at(result).merge(vars);
              } else {
                function->outputBind.emplace(result, vars);
              }
            }
            DAG->removeMidDAGNode(getSubArrayNode);
            break;
          }
          default:
            assert(false);
        }
      }
    }
  }
  deleteUselessNodes();
}
/**
 * @brief 比较两个基本块中的表达式是否等价
 *
 * @return 是否等价
 */
auto exprEval(Mid2EndDAGNode *root1, Mid2EndDAGNode *root2) -> bool { return false; }

/**
 * @brief 生成MIN/MAX节点
 *
 * 若MIN/MAX类型为整型，默认其第二个操作数必须为load的内存值，以便转化为atomic min/max
 * @return 无返回值
 */
// 目前只考虑整型
void Mid2EndDAGSimplifier::generateMinMax() {
  for (const auto &functionItem : pModule->getFunctions()) {
    auto function = functionItem.second.get();
    auto DAGIter = function->getMidDAGs().begin();
    while (DAGIter != function->getMidDAGs().end()) {
      auto DAG = DAGIter->get();
      DAGIter++;
      constexpr auto mask =
          Mid2EndDAGNode::ICMPLT | Mid2EndDAGNode::ICMPGT | Mid2EndDAGNode::ICMPLE | Mid2EndDAGNode::ICMPGE;
      if (DAG->terminatorNode == nullptr || DAG->terminatorNode->getNodeType() != Mid2EndDAGNode::CONDBR ||
          (dynamic_cast<Mid2EndCondBrNode *>(DAG->getTerminatorNode())->getCond()->getNodeType() & mask) == 0) {
        continue;
      }

      auto cond =
          dynamic_cast<Mid2EndBinaryOpNode *>(dynamic_cast<Mid2EndCondBrNode *>(DAG->getTerminatorNode())->getCond());
      Mid2EndDAGNode *hs1;
      Mid2EndLoadNode *hs2;
      if (cond->getLhs()->getNodeType() == Mid2EndDAGNode::LOAD) {
        hs2 = dynamic_cast<Mid2EndLoadNode *>(cond->getLhs());
        hs1 = cond->getRhs();
      } else if (cond->getRhs()->getNodeType() == Mid2EndDAGNode::LOAD) {
        hs2 = dynamic_cast<Mid2EndLoadNode *>(cond->getRhs());
        hs1 = cond->getLhs();
      } else {
        continue;
      }

      auto &successors = DAG->getSuccessors();
      if (successors.size() != 2) {
        continue;
      }

      auto DAG1 = *successors.begin();
      auto DAG2 = *std::next(successors.begin());
      Mid2EndDAG *thenDAG;
      Mid2EndDAG *exitDAG;
      if (DAG1->getNumSuccessors() == 1 && *DAG1->getSuccessors().begin() == DAG2) {
        thenDAG = DAG1;
        exitDAG = DAG2;
      } else if (DAG2->getNumSuccessors() == 1 && *DAG2->getSuccessors().begin() == DAG1) {
        thenDAG = DAG2;
        exitDAG = DAG1;
      } else {
        continue;
      }

      auto predicate = [](const std::unique_ptr<Mid2EndDAGNode> &node) -> bool {
        return node->getNodeType() == Mid2EndDAGNode::STORE;
      };
      auto storeIter = std::find_if(thenDAG->getMidDAGNodes().begin(), thenDAG->getMidDAGNodes().end(), predicate);
      if (storeIter == thenDAG->getMidDAGNodes().end()) {
        continue;
      }
      auto storeNode = dynamic_cast<Mid2EndStoreNode *>(storeIter->get());

      std::map<Mid2EndDAGNode *, bool> storeUsed;
      for (const auto &node : thenDAG->getMidDAGNodes()) {
        storeUsed.emplace(node.get(), false);
      }
      std::list<Mid2EndDAGNode *> toExtendedNodes;
      toExtendedNodes.emplace_back(storeNode);
      storeUsed.at(storeNode) = true;

      while (!toExtendedNodes.empty()) {
        auto node = toExtendedNodes.front();
        toExtendedNodes.pop_front();
        for (const auto &used : node->getUseds()) {
          if (used->getParent() == thenDAG && !storeUsed.at(used)) {
            toExtendedNodes.emplace_back(used);
            storeUsed.at(used) = true;
          }
        }
      }

      bool isOnlyStore = true;
      for (const auto &node : thenDAG->getMidDAGNodes()) {
        if (!storeUsed.at(node.get()) && node->getNodeType() != Mid2EndDAGNode::BR) {
          isOnlyStore = false;
          break;
        }
      }
      if (!isOnlyStore) {
        continue;
      }

      // exprComp需要实现
    }
  }
}
}  // namespace mid2EndDAG