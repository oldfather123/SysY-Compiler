#include "../include/backend/DAGPrinter.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include "../include/backend/Mid2EndDAG.h"
/**
 * @file DAGPrinter.cpp
 * @brief 中端到后端DAG打印器的源文件
 *
 */

namespace mid2EndDAG {
/**
 * @brief 获取操作数名字
 *
 * @param [in] node 操作数节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 操作数名字
 */
auto Mid2EndDAGPrinter::getOperandName(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) -> std::string {
  std::stringstream ss;
  std::string name;
  if (node->getNodeType() == Mid2EndDAGNode::SCALAR) {
    if (tmpIndexMap.find(node) == tmpIndexMap.end()) {
      name = localScalarNameMap.at(function).at(function->getInputBind().at(node));
    } else {
      ss << "%" << tmpIndexMap.at(node);
      name = ss.str();
    }
  } else if (node->getNodeType() == Mid2EndDAGNode::POINTER) {
    auto pointerNode = dynamic_cast<Mid2EndPointerNode *>(node);
    if (localArrayNameMap.at(function).find(pointerNode) != localArrayNameMap.at(function).end()) {
      name = localArrayNameMap.at(function).at(dynamic_cast<Mid2EndPointerNode *>(node));
    } else {
      for (const auto &globalItem : pModule->getGlobals()) {
        if (globalItem.second.get() == pointerNode) {
          name = globalItem.first;
          break;
        }
      }
    }
  } else if (node->getNodeType() == Mid2EndDAGNode::INT) {
    ss << dynamic_cast<Mid2EndIntNode *>(node)->getInt();
    name = ss.str();
  } else if (node->getNodeType() == Mid2EndDAGNode::FLOAT) {
    ss << dynamic_cast<Mid2EndFloatNode *>(node)->getFloat();
    name = ss.str();
  } else if (node->getNodeType() == Mid2EndDAGNode::UINT64) {
    ss << dynamic_cast<Mid2EndUint64Node *>(node)->getUint64();
    name = ss.str();
  } else {
    if (tmpIndexMap.find(node) != tmpIndexMap.end()) {
      ss << "%" << tmpIndexMap.at(node);
      name = ss.str();
    } else if (specialNames.find(node) != specialNames.end()) {
      name = specialNames.at(node);
    } else {
      assert(false);
    }
  }
  return name;
}
/**
 * @brief 获取输出节点名字列表
 *
 * @param [in] node 输出节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 输出节点名字列表
 */
auto Mid2EndDAGPrinter::getOutputNames(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG)
    -> std::list<std::string> {
  std::list<std::string> outputNames;
  auto &outputBind = DAG->getOutputNodes();
  if (outputBind.find(node) != outputBind.end()) {
    bool isOk = true;
    for (const auto &var : function->getOutputBind().at(node)) {
      bool isReady = true;
      if (varInputMap.find(var) != varInputMap.end()) {
        auto inputNode = varInputMap.at(var);
        if (inputNode != node) {
          if (outputBind.find(inputNode) != outputBind.end() && !isVisited.at(inputNode)) {
            isReady = false;
          } else {
            for (const auto &user : inputNode->getUsers()) {
              if (!isVisited.at(user)) {
                isReady = false;
                break;
              }
            }
          }
        }
      }
      if (isReady) {
        outputNames.emplace_back(localScalarNameMap.at(function).at(var));
      } else {
        isOk = false;
        std::stringstream ss;
        if (tmpIndexMap.find(node) == tmpIndexMap.end()) {
          ss << "%" << tmpIndex;
          outputNames.emplace_back(ss.str());
          tmpIndexMap.emplace(node, tmpIndex);
          tmpIndex++;
          ss.str("");
        }
        ss << "%" << tmpIndexMap.at(node);
        movMap.emplace(ss.str(), localScalarNameMap.at(function).at(var));
      }
    }
    if (isOk) {
      specialNames.emplace(node, outputNames.front());
    }
  } else {
    std::stringstream ss;
    ss << "%" << tmpIndex;
    outputNames.emplace_back(ss.str());
    tmpIndexMap.emplace(node, tmpIndex);
    tmpIndex++;
  }

  return outputNames;
}
/**
 * @brief 打印Constant节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteConstant(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto &outputBind = function->getOutputBind();
  if (outputBind.find(node) != outputBind.end()) {
    for (const auto &var : outputBind.at(node)) {
      bool isReady = true;
      if (varInputMap.find(var) != varInputMap.end()) {
        auto inputNode = varInputMap.at(var);
        if (inputNode != node) {
          if (outputBind.find(inputNode) != outputBind.end() && !isVisited.at(inputNode)) {
            isReady = false;
          } else {
            for (const auto &user : inputNode->getUsers()) {
              if (!isVisited.at(user)) {
                isReady = false;
                break;
              }
            }
          }
        }
      }
      auto name = localScalarNameMap.at(function).at(var);
      if (isReady) {
        std::cout << "  ";
        std::cout << "Mov " << getOperandName(node, function, DAG) << ", " << name << std::endl;
      } else {
        movMap.emplace(getOperandName(node, function, DAG), name);
      }
    }
  }
}
/**
 * @brief 打印Scalar节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteScalarNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  std::stringstream ss;
  auto &inputBind = function->getInputBind();
  auto &outputBind = function->getOutputBind();
  auto inputScalar = inputBind.at(node);
  if (outputBind.find(node) != outputBind.end()) {
    for (const auto &var : outputBind.at(node)) {
      bool isReady = true;
      if (varInputMap.find(var) != varInputMap.end()) {
        auto inputNode = varInputMap.at(var);
        if (inputNode != node) {
          if (outputBind.find(inputNode) != outputBind.end() && !isVisited.at(inputNode)) {
            isReady = false;
          } else {
            for (const auto &user : inputNode->getUsers()) {
              if (!isVisited.at(user)) {
                isReady = false;
                break;
              }
            }
          }
        }
      }
      if (isReady) {
        if (var != inputScalar) {
          std::cout << "  ";
          std::cout << "Mov " << getOperandName(node, function, DAG) << ", " << localScalarNameMap.at(function).at(var)
                    << std::endl;
        }
      } else {
        if (tmpIndexMap.find(node) == tmpIndexMap.end()) {
          std::cout << "  ";
          std::cout << "Mov " << localScalarNameMap.at(function).at(inputScalar) << ", "
                    << "%" << tmpIndex << std::endl;
          tmpIndexMap.emplace(node, tmpIndex);
          tmpIndex++;
        }
        movMap.emplace(getOperandName(node, function, DAG), localScalarNameMap.at(function).at(var));
      }
    }
  }
}
/**
 * @brief 打印IntBinary节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteIntBinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto intBinaryNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
  auto lhs = intBinaryNode->getLhs();
  auto rhs = intBinaryNode->getRhs();
  auto lhsName = getOperandName(lhs, function, DAG);
  auto rhsName = getOperandName(rhs, function, DAG);
  auto intBinaryNames = getOutputNames(node, function, DAG);
  std::cout << "  ";
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::ADD:
      std::cout << "Add ";
      break;
    case Mid2EndDAGNode::SUB:
      std::cout << "Sub ";
      break;
    case Mid2EndDAGNode::MUL:
      std::cout << "Mul ";
      break;
    case Mid2EndDAGNode::DIV:
      std::cout << "Div ";
      break;
    case Mid2EndDAGNode::REM:
      std::cout << "Rem ";
      break;
    case Mid2EndDAGNode::MIN:
      std::cout << "Min ";
      break;
    case Mid2EndDAGNode::MAX:
      std::cout << "Max ";
      break;
    case Mid2EndDAGNode::ICMPEQ:
      std::cout << "EQ ";
      break;
    case Mid2EndDAGNode::ICMPNE:
      std::cout << "NE ";
      break;
    case Mid2EndDAGNode::ICMPLT:
      std::cout << "LT ";
      break;
    case Mid2EndDAGNode::ICMPLE:
      std::cout << "LE ";
      break;
    case Mid2EndDAGNode::ICMPGT:
      std::cout << "GT ";
      break;
    case Mid2EndDAGNode::ICMPGE:
      std::cout << "GE ";
      break;
    default:
      assert(false);
  }
  std::cout << lhsName << ", " << rhsName << ", " << intBinaryNames.front() << std::endl;
  auto iter = std::next(intBinaryNames.begin());
  for (unsigned i = 1; i < intBinaryNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << intBinaryNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印Uint64Binary节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteUint64BinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto uint64BinaryNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
  auto lhs = uint64BinaryNode->getLhs();
  auto rhs = uint64BinaryNode->getRhs();
  auto lhsName = getOperandName(lhs, function, DAG);
  auto rhsName = getOperandName(rhs, function, DAG);
  auto uint64BinaryNames = getOutputNames(node, function, DAG);
  std::cout << "  ";
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::ADD64:
      std::cout << "Add64 ";
      break;
    case Mid2EndDAGNode::MUL64:
      std::cout << "Mul64 ";
      break;
    default:
      assert(false);
  }
  std::cout << lhsName << ", " << rhsName << ", " << uint64BinaryNames.front() << std::endl;
  auto iter = std::next(uint64BinaryNames.begin());
  for (unsigned i = 1; i < uint64BinaryNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << uint64BinaryNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印FloatBinary节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteFloatBinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto floatBinaryNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
  auto lhs = floatBinaryNode->getLhs();
  auto rhs = floatBinaryNode->getRhs();
  auto lhsName = getOperandName(lhs, function, DAG);
  auto rhsName = getOperandName(rhs, function, DAG);
  auto floatBinaryNames = getOutputNames(floatBinaryNode, function, DAG);
  std::cout << "  ";
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::FADD:
      std::cout << "FAdd ";
      break;
    case Mid2EndDAGNode::FSUB:
      std::cout << "FSub ";
      break;
    case Mid2EndDAGNode::FMUL:
      std::cout << "FMul ";
      break;
    case Mid2EndDAGNode::FDIV:
      std::cout << "FDiv ";
      break;
    case Mid2EndDAGNode::FCMPEQ:
      std::cout << "FEQ ";
      break;
    case Mid2EndDAGNode::FCMPNE:
      std::cout << "FNE ";
      break;
    case Mid2EndDAGNode::FCMPLT:
      std::cout << "FLT ";
      break;
    case Mid2EndDAGNode::FCMPLE:
      std::cout << "FLE ";
      break;
    case Mid2EndDAGNode::FCMPGT:
      std::cout << "FGT ";
      break;
    case Mid2EndDAGNode::FCMPGE:
      std::cout << "FGE ";
      break;
    case Mid2EndDAGNode::FEQ:
      std::cout << "R_FEQ ";
      break;
    case Mid2EndDAGNode::FLT:
      std::cout << "R_FLT ";
      break;
    case Mid2EndDAGNode::FLE:
      std::cout << "R_FLE ";
      break;
    default:
      assert(false);
  }
  std::cout << lhsName << ", " << rhsName << ", " << floatBinaryNames.front() << std::endl;
  auto iter = std::next(floatBinaryNames.begin());
  for (unsigned i = 1; i < floatBinaryNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << floatBinaryNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印IntUnary节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteIntUnaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto intUnaryNode = dynamic_cast<Mid2EndUnaryOpNode *>(node);
  auto hs = intUnaryNode->getHs();
  auto hsName = getOperandName(hs, function, DAG);
  auto intUnaryNames = getOutputNames(intUnaryNode, function, DAG);
  std::cout << "  ";
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::NEG:
      std::cout << "Neg ";
      break;
    case Mid2EndDAGNode::NOT:
      std::cout << "Not ";
      break;
    case Mid2EndDAGNode::ITOF:
      std::cout << "ItoF ";
      break;
    case Mid2EndDAGNode::FNZERO:
      std::cout << "FNZERO ";
      break;
    case Mid2EndDAGNode::SEXTW:
      std::cout << "Sext_w ";
      break;
    default:
      assert(false);
  }
  std::cout << hsName << ", " << intUnaryNames.front() << std::endl;
  auto iter = std::next(intUnaryNames.begin());
  for (unsigned i = 1; i < intUnaryNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << intUnaryNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印FloatUnary节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteFloatUnaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto floatUnaryNode = dynamic_cast<Mid2EndUnaryOpNode *>(node);
  auto hs = floatUnaryNode->getHs();
  auto hsName = getOperandName(hs, function, DAG);
  auto floatUnaryNames = getOutputNames(floatUnaryNode, function, DAG);
  std::cout << "  ";
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::FNEG:
      std::cout << "Fneg ";
      break;
    case Mid2EndDAGNode::FNOT:
      std::cout << "Fnot ";
      break;
    case Mid2EndDAGNode::FTOI:
      std::cout << "FtoI ";
      break;
    default:
      assert(false);
  }
  std::cout << hsName << ", " << floatUnaryNames.front() << std::endl;
  auto iter = std::next(floatUnaryNames.begin());
  for (unsigned i = 1; i < floatUnaryNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << floatUnaryNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印Call节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteCallNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto callNode = dynamic_cast<Mid2EndCallNode *>(node);
  std::list<std::string> callNames;
  if (callNode->getCallee()->getReturnType() != Function::VOID) {
    callNames = getOutputNames(callNode, function, DAG);
  }
  std::cout << "  ";
  std::cout << "Call " << callNode->getCallee()->getName() << "(";
  if (!callNode->getParams().empty()) {
    std::cout << getOperandName(callNode->getParam(0), function, DAG);
  }
  for (unsigned i = 1; i < callNode->getParams().size(); i++) {
    std::cout << ", " << getOperandName(callNode->getParam(i), function, DAG);
  }
  std::cout << ") ";
  if (!callNames.empty()) {
    std::cout << callNames.front() << std::endl;
    auto iter = std::next(callNames.begin());
    for (unsigned i = 1; i < callNames.size(); i++) {
      std::cout << "  ";
      std::cout << "Mov " << callNames.front() << ", " << *iter << std::endl;
      iter++;
    }
  } else {
    std::cout << std::endl;
  }
}
/**
 * @brief 打印CondBr节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteCondBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto condBrNode = dynamic_cast<Mid2EndCondBrNode *>(node);
  std::cout << "  ";
  std::cout << "CondBr " << getOperandName(condBrNode->getCond(), function, DAG) << ", "
            << condBrNode->getThenDAG()->getName() << ", " << condBrNode->getElseDAG()->getName() << std::endl;
}
/**
 * @brief 打印Br节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto brNode = dynamic_cast<Mid2EndBrNode *>(node);
  std::cout << "  ";
  std::cout << "Br " << brNode->getThenDAG()->getName() << std::endl;
}
/**
 * @brief 打印Return节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteReturnNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto returnNode = dynamic_cast<Mid2EndReturnNode *>(node);
  std::cout << "  ";
  std::cout << "Return ";
  if (returnNode->getVal() != nullptr) {
    std::cout << getOperandName(returnNode->getVal(), function, DAG);
  }
  std::cout << std::endl;
}
/**
 * @brief 打印Load节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteLoadNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto loadNode = dynamic_cast<Mid2EndLoadNode *>(node);
  auto loadNames = getOutputNames(loadNode, function, DAG);
  std::cout << "  ";
  std::cout << "Load " << getOperandName(loadNode->getPointer(), function, DAG);
  for (const auto &index : loadNode->getIndices()) {
    std::cout << "[" << getOperandName(index, function, DAG) << "]";
  }
  std::cout << ", " << loadNames.front() << std::endl;
  auto iter = std::next(loadNames.begin());
  for (unsigned i = 1; i < loadNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << loadNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印Store节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteStoreNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto storeNode = dynamic_cast<Mid2EndStoreNode *>(node);
  std::cout << "  ";
  std::cout << "Store " << getOperandName(storeNode->getValue(), function, DAG) << ", "
            << getOperandName(storeNode->getPointer(), function, DAG);
  for (const auto &index : storeNode->getIndices()) {
    std::cout << "[" << getOperandName(index, function, DAG) << "]";
  }
  std::cout << std::endl;
}
/**
 * @brief 打印La节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteLaNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto laNode = dynamic_cast<Mid2EndLaNode *>(node);
  auto laNames = getOutputNames(laNode, function, DAG);
  std::cout << "  ";
  std::cout << "La " << getOperandName(laNode->getPointer(), function, DAG);
  for (const auto &index : laNode->getIndices()) {
    std::cout << "[" << getOperandName(index, function, DAG) << "]";
  }
  std::cout << ", " << laNames.front() << std::endl;
  auto iter = std::next(laNames.begin());
  for (unsigned int i = 1; i < laNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << laNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印GetSubArray节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteGetSubArrayNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto getSubArrayNode = dynamic_cast<Mid2EndGetSubArrayNode *>(node);
  auto getSubArrayNames = getOutputNames(getSubArrayNode, function, DAG);
  std::cout << "  ";
  std::cout << "GetSubArray " << getOperandName(getSubArrayNode->getPointer(), function, DAG);
  for (const auto &index : getSubArrayNode->getIndices()) {
    std::cout << "[" << getOperandName(index, function, DAG) << "]";
  }
  std::cout << ", " << getSubArrayNames.front() << std::endl;
  auto iter = std::next(getSubArrayNames.begin());
  for (unsigned int i = 1; i < getSubArrayNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << getSubArrayNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印Memset节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteMemsetNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto memsetNode = dynamic_cast<Mid2EndMemsetNode *>(node);
  std::cout << "  ";
  std::cout << "Memset " << getOperandName(memsetNode->getPointer(), function, DAG) << " "
            << "begin " << memsetNode->getBegin() << " "
            << "size " << memsetNode->getSize() << " "
            << "value " << getOperandName(memsetNode->getValue(), function, DAG) << std::endl;
}
/**
 * @brief 打印RiscvCondBr节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteRiscvCondBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto riscvCondBrNode = dynamic_cast<Mid2EndRiscvCondBrNode *>(node);
  std::cout << "  ";
  switch (riscvCondBrNode->getNodeType()) {
    case Mid2EndDAGNode::BEQ: {
      std::cout << "BEQ ";
      break;
    }
    case Mid2EndDAGNode::BNE: {
      std::cout << "BNE ";
      break;
    }
    case Mid2EndDAGNode::BLT: {
      std::cout << "BLT ";
      break;
    }
    case Mid2EndDAGNode::BLTU: {
      std::cout << "BLTU ";
      break;
    }
    case Mid2EndDAGNode::BGE: {
      std::cout << "BGE ";
      break;
    }
    case Mid2EndDAGNode::BGEU: {
      std::cout << "BGEU ";
      break;
    }
    default:
      assert(false);
  }
  std::cout << getOperandName(riscvCondBrNode->getLhsNode(), function, DAG) << ", "
            << getOperandName(riscvCondBrNode->getRhsNode(), function, DAG) << ", "
            << riscvCondBrNode->getThenDAG()->getName() << std::endl;
}
/**
 * @brief 打印RiscvF3Op节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteRiscvF3OpNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto f3OpNode = dynamic_cast<Mid2EndF3OpNode *>(node);
  auto hs0 = f3OpNode->getHs(0);
  auto hs1 = f3OpNode->getHs(1);
  auto hs2 = f3OpNode->getHs(2);
  auto hs0Name = getOperandName(hs0, function, DAG);
  auto hs1Name = getOperandName(hs1, function, DAG);
  auto hs2Name = getOperandName(hs2, function, DAG);
  auto f3OpNames = getOutputNames(f3OpNode, function, DAG);
  std::cout << "  ";
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::FMADD:
      std::cout << "FMADD ";
      break;
    case Mid2EndDAGNode::FMSUB:
      std::cout << "FMSUB ";
      break;
    case Mid2EndDAGNode::FNMADD:
      std::cout << "FNMADD ";
      break;
    case Mid2EndDAGNode::FNMSUB:
      std::cout << "FNMSUB ";
      break;
    default:
      assert(false);
  }
  std::cout << hs0Name << ", " << hs1Name << ", " << hs2Name << ", " << f3OpNames.front() << std::endl;
  auto iter = std::next(f3OpNames.begin());
  for (unsigned i = 1; i < f3OpNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << f3OpNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印RiscvLoad节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteRiscvLoadNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto rvloadNode = dynamic_cast<Mid2EndRVLoadNode *>(node);
  auto rvloadNames = getOutputNames(rvloadNode, function, DAG);
  std::cout << "  ";
  std::cout << "RVLoad " << getOperandName(rvloadNode->getPointer(), function, DAG);
  std::cout << "[" << getOperandName(rvloadNode->getOffset(), function, DAG) << "]";
  std::cout << ", " << rvloadNames.front() << std::endl;
  auto iter = std::next(rvloadNames.begin());
  for (unsigned i = 1; i < rvloadNames.size(); i++) {
    std::cout << "  ";
    std::cout << "Mov " << rvloadNames.front() << ", " << *iter << std::endl;
    iter++;
  }
}
/**
 * @brief 打印RiscvStore节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG  所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::interpreteRiscvStoreNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto rvStoreNode = dynamic_cast<Mid2EndRVStoreNode *>(node);
  std::cout << "  ";
  std::cout << "RVStore " << getOperandName(rvStoreNode->getValue(), function, DAG) << ", "
            << getOperandName(rvStoreNode->getPointer(), function, DAG);
  std::cout << "[" << getOperandName(rvStoreNode->getOffset(), function, DAG) << "]";
  std::cout << std::endl;
}
/**
 * @brief 打印节点
 *
 * @param [in] node 待打印节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::printNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  using NodeType = Mid2EndDAGNode::NodeType;
  std::stringstream ss;
  switch (node->getNodeType()) {
    case NodeType::INT:
    case NodeType::UINT64:
    case NodeType::FLOAT: {
      interpreteConstant(node, function, DAG);
      break;
    }
    case NodeType::SCALAR: {
      interpreteScalarNode(node, function, DAG);
      break;
    }
    case NodeType::ADD:
    case NodeType::SUB:
    case NodeType::MUL:
    case NodeType::DIV:
    case NodeType::REM:
    case NodeType::ICMPEQ:
    case NodeType::ICMPNE:
    case NodeType::ICMPLT:
    case NodeType::ICMPGT:
    case NodeType::ICMPLE:
    case NodeType::ICMPGE:
    case NodeType::MIN:
    case NodeType::MAX: {
      interpreteIntBinaryNode(node, function, DAG);
      break;
    }
    case NodeType::ADD64:
    case NodeType::MUL64: {
      interpreteUint64BinaryNode(node, function, DAG);
      break;
    }

    case NodeType::FADD:
    case NodeType::FSUB:
    case NodeType::FMUL:
    case NodeType::FDIV:
    case NodeType::FCMPEQ:
    case NodeType::FCMPNE:
    case NodeType::FCMPLT:
    case NodeType::FCMPGT:
    case NodeType::FCMPLE:
    case NodeType::FCMPGE:
    case NodeType::FEQ:
    case NodeType::FLT:
    case NodeType::FLE: {
      interpreteFloatBinaryNode(node, function, DAG);
      break;
    }

    case NodeType::NEG:
    case NodeType::NOT:
    case NodeType::ITOF:
    case NodeType::FNZERO:
    case NodeType::SEXTW: {
      interpreteIntUnaryNode(node, function, DAG);
      break;
    }
    case NodeType::FNEG:
    case NodeType::FNOT:
    case NodeType::FTOI: {
      interpreteFloatUnaryNode(node, function, DAG);
      break;
    }

    case NodeType::CALL: {
      interpreteCallNode(node, function, DAG);
      break;
    }
    case NodeType::CONDBR: {
      interpreteCondBrNode(node, function, DAG);
      break;
    }
    case NodeType::BR: {
      interpreteBrNode(node, function, DAG);
      break;
    }
    case NodeType::RETURN: {
      interpreteReturnNode(node, function, DAG);
      break;
    }
    case NodeType::LOAD: {
      interpreteLoadNode(node, function, DAG);
      break;
    }
    case NodeType::RVLOAD: {
      interpreteRiscvLoadNode(node, function, DAG);
      break;
    }
    case NodeType::STORE: {
      interpreteStoreNode(node, function, DAG);
      break;
    }
    case NodeType::RVSTORE: {
      interpreteRiscvStoreNode(node, function, DAG);
      break;
    }
    case NodeType::LA: {
      interpreteLaNode(node, function, DAG);
      break;
    }
    case NodeType::GETSUBARRAY: {
      interpreteGetSubArrayNode(node, function, DAG);
      break;
    }
    case NodeType::MEMSET: {
      interpreteMemsetNode(node, function, DAG);
      break;
    }
    case NodeType::BEQ:
    case NodeType::BNE:
    case NodeType::BLT:
    case NodeType::BLTU:
    case NodeType::BGE:
    case NodeType::BGEU: {
      interpreteRiscvCondBrNode(node, function, DAG);
      break;
    }
    case NodeType::FMADD:
    case NodeType::FNMADD:
    case NodeType::FMSUB:
    case NodeType::FNMSUB: {
      interpreteRiscvF3OpNode(node, function, DAG);
      break;
    }
    default:
      break;
  }
}
/**
 * @brief 打印DAG
 *
 * @param [in] DAG 待打印的DAG
 * @return 无返回值
 */
void Mid2EndDAGPrinter::printDAG(Mid2EndDAG *DAG) {
  for (const auto &node : DAG->getMidDAGNodes()) {
    isVisited.emplace(node.get(), false);
    isAddToVisited.emplace(node.get(), false);
  }
  for (const auto &input : DAG->getInputNodes()) {
    isAddToVisited.at(input) = true;
    toVisit.emplace_back(input);
  }
  for (const auto &localArray : DAG->getParent()->getLocalArrays()) {
    isVisited.emplace(localArray.get(), true);
    isAddToVisited.emplace(localArray.get(), true);
  }
  for (const auto &globalItem : DAG->getParent()->getParent()->getGlobals()) {
    isVisited.emplace(globalItem.second.get(), true);
    isAddToVisited.emplace(globalItem.second.get(), true);
  }

  for (const auto input : DAG->getInputNodes()) {
    varInputMap.emplace(DAG->getParent()->getInputBind().at(input), input);
  }

  bool isReady;
  for (const auto &node : DAG->getMidDAGNodes()) {
    if (!isAddToVisited.at(node.get())) {
      isReady = true;
      for (const auto &used : node->getUseds()) {
        if (!isVisited.at(used)) {
          isReady = false;
          break;
        }
      }
      if (isReady) {
        isAddToVisited.at(node.get()) = true;
        toVisit.emplace_back(node.get());
      }
    }
  }

  while (!toVisit.empty()) {
    auto node = toVisit.front();
    toVisit.pop_front();
    bool isReady = true;
    for (const auto &used : node->getUseds()) {
      if (!isVisited.at(used)) {
        isReady = false;
        break;
      }
    }
    if (!isReady) {
      toVisit.push_back(node);
      continue;
    }

    isVisited.at(node) = true;
    if (DAG->getTerminatorNode() != node) {
      printNode(node, DAG->getParent(), DAG);
    }

    for (const auto &user : node->getUsers()) {
      if (!isAddToVisited.at(user)) {
        toVisit.emplace_back(user);
        isAddToVisited.at(user) = true;
      }
    }
  }

  for (const auto &movItem : movMap) {
    std::cout << "  ";
    std::cout << "Mov " << movItem.first << ", " << movItem.second << std::endl;
  }

  auto terminator = DAG->getTerminatorNode();
  if (terminator != nullptr) {
    printNode(terminator, DAG->getParent(), DAG);
  }

  movMap.clear();
  isVisited.clear();
  isAddToVisited.clear();
  toVisit.clear();
  varInputMap.clear();
}
/**
 * @brief 打印函数
 *
 * @param [in] function 待打印的函数
 * @return 无返回值
 */
void Mid2EndDAGPrinter::printFunction(Function *function) {
  std::cout << function->getName() << ": " << std::endl;
  std::cout << "  "
            << "arg:" << std::endl;
  for (const auto &param : function->getParams()) {
    auto scalarParam = dynamic_cast<Mid2EndScalarNode *>(param);
    auto pointerParam = dynamic_cast<Mid2EndPointerNode *>(param);
    if (scalarParam != nullptr) {
      auto localScalar = function->getInputBind().at(scalarParam);
      std::cout << "      " << localScalarNameMap.at(function).at(localScalar) << std::endl;
    } else {
      std::cout << "      " << localArrayNameMap.at(function).at(pointerParam) << std::endl;
    }
  }

  for (const auto &DAG : function->getMidDAGs()) {
    if (!DAG->getName().empty()) {
      std::cout << DAG->getName() << ":" << std::endl;
    }
    printDAG(DAG.get());
  }
}
/**
 * @brief 打印模块
 *
 * @return 无返回值
 */
void Mid2EndDAGPrinter::printModule() {
  std::cout << "global:" << std::endl;
  for (const auto &globalItem : pModule->getGlobals()) {
    std::cout << "  " << globalItem.first << " ";
    if (globalItem.second->isIntPointer()) {
      std::cout << "int";
    } else {
      std::cout << "float";
    }
    for (const auto &index : globalItem.second->getDims()) {
      std::cout << "[" << index << "]";
    }
    auto &init = pModule->getGlobalInit(globalItem.first);
    auto &values = init.getValues();
    auto &numbers = init.getNumbers();
    if (globalItem.second->isIntPointer()) {
      if (numbers.size() == 1 && numbers[0] == 1) {
        std::cout << "  " << values[0].intValue << std::endl;
      } else {
        std::cout << "  "
                  << "{";
        std::cout << values[0].intValue << ":" << numbers[0];
        for (unsigned i = 1; i < numbers.size(); i++) {
          std::cout << ", " << values[i].intValue << ":" << numbers[i];
        }
        std::cout << "}" << std::endl;
      }
    } else {
      if (numbers.size() == 1 && numbers[0] == 1) {
        std::cout << "  " << values[0].floatValue << std::endl;
      } else {
        std::cout << "  "
                  << "{";
        std::cout << values[0].floatValue << ":" << numbers[0];
        for (unsigned i = 1; i < numbers.size(); i++) {
          std::cout << ", " << values[i].floatValue << ":" << numbers[i];
        }
        std::cout << "}" << std::endl;
      }
    }
  }
  for (const auto &functionItem : pModule->getFunctions()) {
    printFunction(functionItem.second.get());
  }
};
}  // namespace mid2EndDAG