#include "../include/backend/DAG2End.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "../include/backend/Mid2End.h"
#include "../include/backend/Mid2EndDAG.h"
#include "../include/backend/Riscv.h"
#include "../include/utils/ConstantCounter.h"
/**
 * @file DAG2End.cpp
 * @brief 中端到后端DAG转化为后端的源文件
 *
 */

// todo 通过名字区分变量的方法不太靠谱(一个函数内的不同变量名字必须不一样)
// todo 重写代码，使其更简洁紧凑
namespace mid2EndDAG {
/**
 * @brief 获取或创建符号寄存器
 *
 * @param [in] name 符号寄存器名字
 * @param [in] function 所处的函数
 * @param [in] isInt 是否是整型
 * @param [in] is64  是否是64位
 * @return 符号寄存器指针
 */
auto DAG2EndBuilder::getOrEmplaceSymRegister(const std::string &name, Function *function, bool isInt, bool is64)
    -> riscv::SymRegister * {
  std::stringstream ss;
  ss << name << "@" << function->getName();
  auto newName = ss.str();
  if (nameRegistersMap.find(newName) == nameRegistersMap.end()) {
    if (isInt) {
      nameRegistersMap.emplace(newName, new riscv::IntSymRegister(is64, name));
    } else {
      nameRegistersMap.emplace(newName, new riscv::FloatSymRegister(name));
    }
  }

  return nameRegistersMap.at(newName).get();
}
/**
 * @brief 创建临时符号寄存器
 *
 * @param [in] isInt 是否为整型
 * @param [in] is64  是否为64位
 * @return 符号寄存器指针
 */
auto DAG2EndBuilder::emplaceTmpSymRegister(bool isInt, bool is64) -> riscv::SymRegister * {
  std::stringstream ss;
  ss << "%" << tmpIndex;
  tmpIndex++;
  auto name = ss.str();
  if (isInt) {
    nameRegistersMap.emplace(name, new riscv::IntSymRegister(is64, name));
  } else {
    nameRegistersMap.emplace(name, new riscv::FloatSymRegister(name));
  }
  return nameRegistersMap.at(name).get();
}
/**
 * @brief 获取或创建存储int常量的符号寄存器
 *
 * @param [in] intValue int常量
 * @return 符号寄存器指针
 */
auto DAG2EndBuilder::getOrEmplaceConstant(int intValue) -> riscv::IntSymRegister * {
  if (DAGIntConstants.find(intValue) != DAGIntConstants.end()) {
    return DAGIntConstants.at(intValue);
  }

  auto tmpReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, false));
  builder.createLiInst(tmpReg, static_cast<int64_t>(intValue));
  DAGIntConstants.emplace(intValue, tmpReg);
  return tmpReg;
}
/**
 * @brief 获取或创建存储uint64常量的符号寄存器
 *
 * @param [in] intValue uint64常量
 * @return 符号寄存器指针
 */
auto DAG2EndBuilder::getOrEmplaceConstant(uint64_t uint64Value) -> riscv::IntSymRegister * {
  if (DAGUint64Constants.find(uint64Value) != DAGUint64Constants.end()) {
    return DAGUint64Constants.at(uint64Value);
  }

  auto tmpReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
  builder.createLiInst(tmpReg, uint64Value);
  DAGUint64Constants.emplace(uint64Value, tmpReg);
  return tmpReg;
}
/**
 * @brief 获取或创建存储float常量的符号寄存器
 *
 * @param [in] intValue float常量
 * @return 符号寄存器指针
 */
auto DAG2EndBuilder::getOrEmplaceConstant(float floatValue) -> riscv::FloatSymRegister * {
  if (DAGFloatConstants.find(floatValue) != DAGFloatConstants.end()) {
    return DAGFloatConstants.at(floatValue);
  }

  auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
  auto tmpReg = dynamic_cast<riscv::FloatSymRegister *>(emplaceTmpSymRegister(false));
  builder.createLlaInst(addrReg, floatGlobalMap.at(floatValue));
  builder.createFlwInst(tmpReg, addrReg, 0);
  DAGFloatConstants.emplace(floatValue, tmpReg);
  return tmpReg;
}
/**
 * @brief 获取操作数名字
 *
 * @param [in] node 操作数节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 操作数名字
 */
auto DAG2EndBuilder::getOperandName(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) -> std::string {
  std::stringstream ss;
  std::string name;
  if (node->getNodeType() == Mid2EndDAGNode::SCALAR) {
    if (tmpIndexMap.find(node) == tmpIndexMap.end()) {
      if (localScalarNameMap.find(function) == localScalarNameMap.end()) {
        assert(pModule->getExternalFunctions().find(function->getName()) != pModule->getExternalFunctions().end());
        unsigned index = 0;
        while (function->getParam(index) != node) {
          index++;
        }
        ss << function->getName() << "%Param" << index;
      } else {
        name = localScalarNameMap.at(function).at(function->getInputBind().at(node));
      }
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
      // if (outpuBind.find(node) != outpuBind.end()) {
      //   for (const auto &var : outpuBind.at(node)) {
      //     if (varInputMap.find(var) != varInputMap.end()) {
      //       auto inputNode = varInputMap.at(var);
      //       bool isReady = true;
      //       for (const auto &user : inputNode->getUsers()) {
      //         if (!isVisited.at(user)) {
      //           isReady = false;
      //           break;
      //         }
      //       }
      //       if (isReady) {
      //         names.emplace_back(localScalarNameMap.at(function).at(var));
      //       } else {
      //         if (tmpIndexMap.find(node) == tmpIndexMap.end()) {
      //           tmpIndexMap.emplace(node, tmpIndex);
      //           ss << "%" << tmpIndex;
      //           names.emplace_back(ss.str());
      //           ss.str("");
      //           tmpIndex++;
      //         }
      //         ss << "%" << tmpIndexMap.at(node);
      //         movMap.emplace(ss.str(), localScalarNameMap.at(function).at(var));
      //       }
      //     } else {
      //       if (localScalarNameMap.at(function).at(var)[0] != '%') {
      //         ss << "%" << tmpIndex;
      //         localScalarNameMap.at(function).at(var) = ss.str();
      //         tmpIndex++;
      //       }
      //       names.emplace_back(localScalarNameMap.at(function).at(var));
      //     }
      //   }
      // } else {
      //   if (node->getNodeType() == Mid2EndDAGNode::CALL &&
      //       dynamic_cast<Mid2EndCallNode *>(node)->getCallee()->getReturnType() == Function::VOID) {
      //     return names;
      //   }
      //   ss << "%" << tmpIndex;
      //   names.emplace_back(ss.str());
      //   tmpIndexMap.emplace(node, tmpIndex);
      //   tmpIndex++;
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
auto DAG2EndBuilder::getOutputNames(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG)
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
        auto name = ss.str();
        if (function->getVarTypeMap().at(var) == 0) {
          if (intReg2regMovMap.find(name) != intReg2regMovMap.end()) {
            intReg2regMovMap.at(name).insert(localScalarNameMap.at(function).at(var));
          } else {
            intReg2regMovMap.emplace(name, std::set<std::string>{localScalarNameMap.at(function).at(var)});
          }
        } else if (function->getVarTypeMap().at(var) == 2) {
          if (floatReg2regMovMap.find(name) != floatReg2regMovMap.end()) {
            floatReg2regMovMap.at(name).insert(localScalarNameMap.at(function).at(var));
          } else {
            floatReg2regMovMap.emplace(name, std::set<std::string>{localScalarNameMap.at(function).at(var)});
          }
        } else {
          assert(false);
        }
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
 * @brief 获取操作数类型
 *
 * @param [in] node 操作数节点
 * @return 操作数类型
 */
auto DAG2EndBuilder::getOperandType(Mid2EndDAGNode *node) -> unsigned int {
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
 * @brief 获取传参物理寄存器
 *
 * @param [in] isInt 是否为整型
 * @param [in] index 第几个参数
 * @return 物理寄存器指针
 */
auto DAG2EndBuilder::getParamPhyReg(bool isInt, unsigned index) -> riscv::PhyRegister * {
  if (isInt) {
    switch (index) {
      case 0:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
      case 1:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a1);
      case 2:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a2);
      case 3:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a3);
      case 4:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a4);
      case 5:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a5);
      case 6:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a6);
      case 7:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a7);
      default:
        assert(false);
    }
  } else {
    switch (index) {
      case 0:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0);
      case 1:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa1);
      case 2:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa2);
      case 3:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa3);
      case 4:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa4);
      case 5:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa5);
      case 6:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa6);
      case 7:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa7);
      default:
        assert(false);
    }
  }
}
/**
 * @brief 获取或创建memset函数指针
 *
 * @return memset函数指针
 */
auto DAG2EndBuilder::getOrEmplaceMemset() -> riscv::Function * {
  auto &backendFunctions = backendModule->getFunctions();
  if (backendFunctions.find("my_memset") != backendFunctions.end()) {
    return backendFunctions.find("my_memset")->second.get();
  }

  auto oldBlock = builder.getCurBlock();
  auto position = builder.getPosition();
  auto backendFunction = backendModule->createFunction("my_memset", riscv::Function::VOID, false);
  auto stackTable = new mid2end::StackTable;
  functionStackMap.emplace(backendFunction, stackTable);
  auto entryBlock = backendFunction->addBasicBlock("");
  auto condBlock = backendFunction->addBasicBlock("condBlock");
  auto bodyBlock = backendFunction->addBasicBlock("bodyBlock");
  auto exitBlock = backendFunction->addBasicBlock("");

  entryBlock->addSuccessor(condBlock);
  condBlock->addPredecessor(entryBlock);
  condBlock->addPredecessor(bodyBlock);
  condBlock->addSuccessor(bodyBlock);
  condBlock->addSuccessor(exitBlock);
  bodyBlock->addPredecessor(condBlock);
  bodyBlock->addSuccessor(condBlock);
  exitBlock->addPredecessor(condBlock);

  builder.setPosition(entryBlock);
  auto a0 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
  auto a1 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a1);
  auto a2 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a2);
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
  auto sp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::sp);
  auto fp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::fp);
  auto ra = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::ra);

  stackTable->addNotSpillParamReg(a0);
  stackTable->addNotSpillParamReg(a1);
  stackTable->addNotSpillParamReg(a2);

  builder.createSdInst(ra, sp, -8);
  builder.createSdInst(fp, sp, -16);
  builder.createAddiInst(fp, sp, 0);
  builder.createAddiInst(sp, sp, 0);
  std::stringstream ss;
  ss << "pointer"
     << "@"
     << "memset";
  auto pointer = new riscv::IntSymRegister(true, "pointer");
  nameRegistersMap.emplace(ss.str(), pointer);
  ss.str("");
  ss << "value"
     << "@"
     << "memset";
  auto value = new riscv::IntSymRegister(false, "value");
  nameRegistersMap.emplace(ss.str(), value);
  ss.str("");
  ss << "size"
     << "@"
     << "memset";
  auto size = new riscv::IntSymRegister(true, "size");
  nameRegistersMap.emplace(ss.str(), size);
  ss.str("");
  ss << "index"
     << "@"
     << "memset";
  auto index = new riscv::IntSymRegister(true, "index");
  nameRegistersMap.emplace(ss.str(), index);
  builder.createAddiInst(pointer, a0, 0);
  builder.createAddiwInst(value, a1, 0);
  builder.createAddiInst(size, a2, 0);
  builder.createAddiInst(index, zero, 0);

  builder.setPosition(condBlock);
  builder.createBne(index, size, bodyBlock);

  builder.setPosition(bodyBlock);
  builder.createSwInst(value, pointer, 0);
  builder.createAddiInst(pointer, pointer, 4);
  builder.createAddiInst(index, index, 1);
  builder.createJalInst(zero, condBlock);

  builder.setPosition(exitBlock);
  builder.createLdInst(ra, fp, -8);
  builder.createLdInst(fp, fp, -16);
  builder.createAddiInst(sp, sp, 0);
  builder.createJalrInst(zero, ra, 0);

  builder.setPosition(oldBlock, position);

  return backendFunction;
}
/**
 * @brief 除化乘
 *
 * @param [in] dividend 被除数
 * @param [in] result 存放结果的符号寄存器
 * @param [in] divisor 除数
 * @return 无返回值
 */
void DAG2EndBuilder::DivideToMultiply(riscv::IntSymRegister *dividend, riscv::IntSymRegister *result, int divisor) {
  assert(divisor != 0);

  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
  unsigned divisorAbs;
  unsigned k = 0;
  if (divisor < 0) {
    divisorAbs = -divisor;
  } else {
    divisorAbs = divisor;
  }

  auto tmp = divisorAbs;
  tmp >>= 1;
  while (tmp > 0) {
    k++;
    tmp >>= 1;
  }

  if (divisorAbs == (1 << k)) {
    auto tmpReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, false));
    if (k > 1) {
      builder.createSraiwInst(tmpReg, dividend, 31);
      if (k < IMM_BIT) {
        builder.createAndiInst(tmpReg, tmpReg, (1 << k) - 1);
      } else {
        auto constReg = getOrEmplaceConstant((1 << k) - 1);
        builder.createAndInst(tmpReg, tmpReg, constReg);
      }
      builder.createAddwInst(tmpReg, tmpReg, dividend);
      builder.createSraiwInst(result, tmpReg, k);
    } else if (k == 1) {
      builder.createSraiwInst(tmpReg, dividend, 31);
      builder.createSubwInst(tmpReg, dividend, tmpReg);
      builder.createSraiwInst(result, tmpReg, k);
    }
    if (k == 0) {
      if (divisor > 0) {
        builder.createAddiwInst(result, dividend, 0);
      } else {
        builder.createSubwInst(result, zero, dividend);
      }
    } else if (divisor < 0) {
      builder.createSubwInst(result, zero, result);
    }
  } else {
    uint64_t m;
    uint64_t tmp = 1;
    tmp <<= (32 + k);
    auto low = tmp / divisorAbs;
    auto high = low + 1;

    double e = high - 1.0 * tmp / divisorAbs;
    m = (e < 1.0 * (1 << k) / divisorAbs) ? high : low;
    auto tmpReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
    auto constReg = getOrEmplaceConstant(m);
    builder.createMulInst(tmpReg, dividend, constReg);
    builder.createSraiInst(tmpReg, tmpReg, 32 + k);
    builder.createAddiwInst(constReg, tmpReg, 0);
    builder.createSrliwInst(constReg, constReg, 31);
    builder.createAddwInst(result, tmpReg, constReg);
    if (divisor < 0) {
      builder.createSubwInst(result, zero, result);
    }
  }
}
/**
 * @brief 处理Pointer节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpretePointer(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto pointerNode = dynamic_cast<Mid2EndPointerNode *>(node);
  auto targetReg = dynamic_cast<riscv::IntSymRegister *>(
      getOrEmplaceSymRegister(getOperandName(node, function, DAG), function, true, true));
  auto &params = function->getParams();
  if (std::find(params.begin(), params.end(), pointerNode) != params.end()) {
  } else if (localArrayMap.find(pointerNode) != localArrayMap.end()) {
    auto localArray = localArrayMap.at(pointerNode).get();
    auto fp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::fp);
    auto offset = functionStackMap.at(backendModule->getFunction(function->getName()))->getLocalArrayAddr(localArray);
    if (offset > static_cast<uint64_t>(-IMM_NEG_MAX)) {
      auto tmpReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
      builder.createLiInst(tmpReg, offset);
      builder.createSubInst(targetReg, fp, tmpReg);
    } else {
      builder.createAddiInst(targetReg, fp, -static_cast<int64_t>(offset));
    }
  } else {
    auto global = backendModule->getGlobal(getOperandName(node, function, DAG));
    builder.createLlaInst(targetReg, global);
  }
}
/**
 * @brief 乘化移位
 *
 * @param [in] multiplier 非常量乘数
 * @param [in] result 存放结果的符号寄存器
 * @param [in] imm  常量乘数
 * @param [in] is64 是否为64位
 * @return 无返回值
 */
// x * 2^m, x * 2^m*(2^n + 1), x * 2^m*(2^n - 1) 形式的化简
void DAG2EndBuilder::MultiplyToShift(riscv::IntSymRegister *multiplier, riscv::IntSymRegister *result, int64_t imm,
                                     bool is64) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);

  if (imm == 0) {
    builder.createAddiInst(result, zero, 0);
    return;
  }
  if (imm == 1) {
    builder.createAddiInst(result, multiplier, 0);
    return;
  }
  if (imm == -1 && !is64) {
    builder.createSubInst(result, zero, multiplier);
    return;
  }

  uint64_t immAbs;
  if (!is64) {
    immAbs = (imm < 0) ? -imm : imm;
  } else {
    immAbs = static_cast<uint64_t>(imm);
  }
  auto bitCal = [](uint64_t imm) -> unsigned {
    unsigned num = 0;
    while ((imm & 1) == 0) {
      imm >>= 1;
      num++;
    }
    return num;
  };

  auto m = bitCal(immAbs);
  if (immAbs >> m == 1) {
    builder.createSlliInst(result, multiplier, m);
    if (imm < 0 && !is64) {
      builder.createSubInst(result, zero, result);
    }
  } else {
    auto n = bitCal((immAbs >> m) + 1);
    if ((immAbs >> m) + 1 == 1 << n) {
      riscv::IntSymRegister *tmpReg;
      if (m == 0) {
        tmpReg = multiplier;
      } else {
        tmpReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
        builder.createSlliInst(tmpReg, multiplier, m);
      }
      builder.createSlliInst(result, multiplier, m + n);
      builder.createSubInst(result, result, tmpReg);
      if (imm < 0 && !is64) {
        builder.createSubInst(result, zero, result);
      }
      return;
    }

    n = bitCal((immAbs >> m) - 1);
    if (immAbs >> m == (1 << n) + 1) {
      riscv::IntSymRegister *tmpReg;
      if (m == 0) {
        tmpReg = multiplier;
      } else {
        tmpReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
        builder.createSlliInst(tmpReg, multiplier, m);
      }
      builder.createSlliInst(result, multiplier, m + n);
      builder.createAddInst(result, result, tmpReg);
      if (imm < 0 && !is64) {
        builder.createSubInst(result, zero, result);
      }
      return;
    }

    if (!is64) {
      auto constReg = getOrEmplaceConstant(static_cast<int>(imm));
      builder.createMulwInst(result, multiplier, constReg);
    } else {
      auto constReg = getOrEmplaceConstant(immAbs);
      builder.createMulInst(result, multiplier, constReg);
    }
  }
}
/**
 * @brief 处理Constant节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteConstant(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
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
      switch (node->getNodeType()) {
        case Mid2EndDAGNode::INT: {
          auto intValue = static_cast<int64_t>(dynamic_cast<Mid2EndIntNode *>(node)->getInt());
          if (isReady) {
            auto dest = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(name, function, true, false));
            builder.createLiInst(dest, intValue);
          } else {
            if (int2regMovMap.find(intValue) == int2regMovMap.end()) {
              int2regMovMap.emplace(intValue, std::set<std::string>{name});
            } else {
              int2regMovMap.at(intValue).insert(name);
            }
          }
          break;
        }
        case Mid2EndDAGNode::FLOAT: {
          auto floatReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(node)->getFloat());
          if (isReady) {
            auto dest = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(name, function, false));
            builder.createFsgnj_sInst(dest, floatReg, floatReg);
          } else {
            if (float2regMovMap.find(floatReg) != float2regMovMap.end()) {
              float2regMovMap.at(floatReg).insert(name);
            } else {
              float2regMovMap.emplace(floatReg, std::set<std::string>{name});
            }
          }
          break;
        }
        case Mid2EndDAGNode::UINT64: {
          auto uint64Value = dynamic_cast<Mid2EndUint64Node *>(node)->getUint64();
          if (isReady) {
            auto dest = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(name, function, true, true));
            builder.createLiInst(dest, uint64Value);
          } else {
            if (int2regMovMap.find(uint64Value) == int2regMovMap.end()) {
              int2regMovMap.emplace(uint64Value, std::set<std::string>{name});
            } else {
              int2regMovMap.at(uint64Value).insert(name);
            }
          }
          break;
        }
        default:
          assert(false);
      }
    }
  }
}
/**
 * @brief 处理Scalar节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteScalarNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  std::stringstream ss;
  auto &inputBind = function->getInputBind();
  auto &outputBind = function->getOutputBind();
  auto inputScalar = inputBind.at(node);
  bool isInt = dynamic_cast<Mid2EndScalarNode *>(node)->isIntScalar();
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
        if (inputScalar != var) {
          if (isInt) {
            auto source = dynamic_cast<riscv::IntSymRegister *>(
                getOrEmplaceSymRegister(getOperandName(node, function, DAG), function, true, false));
            auto dest = dynamic_cast<riscv::IntSymRegister *>(
                getOrEmplaceSymRegister(localScalarNameMap.at(function).at(var), function, true, false));
            builder.createAddiwInst(dest, source, 0);
          } else {
            auto source = dynamic_cast<riscv::FloatSymRegister *>(
                getOrEmplaceSymRegister(getOperandName(node, function, DAG), function, false));
            auto dest = dynamic_cast<riscv::FloatSymRegister *>(
                getOrEmplaceSymRegister(localScalarNameMap.at(function).at(var), function, false));
            builder.createFsgnj_sInst(dest, source, source);
          }
        }
      } else {
        if (tmpIndexMap.find(node) == tmpIndexMap.end()) {
          std::stringstream ss;
          ss << "%" << tmpIndex;
          tmpIndexMap.emplace(node, tmpIndex);
          tmpIndex++;
          if (isInt) {
            auto source = dynamic_cast<riscv::IntSymRegister *>(
                getOrEmplaceSymRegister(localScalarNameMap.at(function).at(inputScalar), function, true, false));
            auto dest = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(ss.str(), function, true, false));
            builder.createAddiwInst(dest, source, 0);
          } else {
            auto source = dynamic_cast<riscv::FloatSymRegister *>(
                getOrEmplaceSymRegister(localScalarNameMap.at(function).at(inputScalar), function, false));
            auto dest = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(ss.str(), function, false));
            builder.createFsgnj_sInst(dest, source, source);
          }
        }
        auto name = getOperandName(node, function, DAG);
        if (function->getVarTypeMap().at(var) == 0) {
          if (intReg2regMovMap.find(name) != intReg2regMovMap.end()) {
            intReg2regMovMap.at(name).insert(localScalarNameMap.at(function).at(var));
          } else {
            intReg2regMovMap.emplace(name, std::set<std::string>{localScalarNameMap.at(function).at(var)});
          }
        } else if (function->getVarTypeMap().at(var) == 2) {
          if (floatReg2regMovMap.find(name) != floatReg2regMovMap.end()) {
            floatReg2regMovMap.at(name).insert(localScalarNameMap.at(function).at(var));
          } else {
            floatReg2regMovMap.emplace(name, std::set<std::string>{localScalarNameMap.at(function).at(var)});
          }
        } else {
          assert(false);
        }
      }
    }
  }
}
/**
 * @brief 处理IntBinary节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteIntBinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto intBinaryNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
  auto lhs = intBinaryNode->getLhs();
  auto rhs = intBinaryNode->getRhs();
  auto lhsName = getOperandName(lhs, function, DAG);
  auto rhsName = getOperandName(rhs, function, DAG);
  auto intBinaryNames = getOutputNames(node, function, DAG);
  auto intBinaryFrontReg =
      dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(intBinaryNames.front(), function, true));
  std::stringstream ss;
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::ADD: {
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          builder.createAddiwInst(intBinaryFrontReg, rhsReg, imm);
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createAddwInst(intBinaryFrontReg, tmpReg, rhsReg);
        }
      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          builder.createAddiwInst(intBinaryFrontReg, lhsReg, imm);
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createAddwInst(intBinaryFrontReg, lhsReg, tmpReg);
        }
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createAddwInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::SUB: {
      auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        if (imm != 0) {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createSubwInst(intBinaryFrontReg, tmpReg, rhsReg);
        } else {
          builder.createSubwInst(intBinaryFrontReg, zero, rhsReg);
        }
      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = -dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          builder.createAddiwInst(intBinaryFrontReg, lhsReg, imm);
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createAddwInst(intBinaryFrontReg, lhsReg, tmpReg);
        }
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createSubwInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::MUL: {
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        MultiplyToShift(rhsReg, intBinaryFrontReg, imm, false);
      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        MultiplyToShift(lhsReg, intBinaryFrontReg, imm, false);
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createMulwInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::DIV: {
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        auto tmpReg = getOrEmplaceConstant(imm);
        builder.createDivwInst(intBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        // auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        // auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        // auto tmpReg = getOrEmplaceConstant(imm);
        // builder.createDivwInst(intBinaryFrontReg, lhsReg, tmpReg);
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        DivideToMultiply(lhsReg, intBinaryFrontReg, imm);
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createDivwInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::REM: {
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        auto tmpReg = getOrEmplaceConstant(imm);
        builder.createRemwInst(intBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto tmpReg = getOrEmplaceConstant(imm);
        builder.createRemwInst(intBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createRemwInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::MAX: {
      assert(false);
      break;
    }
    case Mid2EndDAGNode::MIN: {
      assert(false);
      break;
    }
    case Mid2EndDAGNode::ICMPEQ: {
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          if (imm != 0) {
            builder.createXoriInst(intBinaryFrontReg, rhsReg, imm);
            builder.createSltiuInst(intBinaryFrontReg, intBinaryFrontReg, 1);
          } else {
            builder.createSltiuInst(intBinaryFrontReg, rhsReg, 1);
          }
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createXorInst(intBinaryFrontReg, rhsReg, tmpReg);
          builder.createSltiuInst(intBinaryFrontReg, intBinaryFrontReg, 1);
        }
      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          if (imm != 0) {
            builder.createXoriInst(intBinaryFrontReg, lhsReg, imm);
            builder.createSltiuInst(intBinaryFrontReg, intBinaryFrontReg, 1);
          } else {
            builder.createSltiuInst(intBinaryFrontReg, lhsReg, 1);
          }
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createXorInst(intBinaryFrontReg, lhsReg, tmpReg);
          builder.createSltiuInst(intBinaryFrontReg, intBinaryFrontReg, 1);
        }
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createXorInst(intBinaryFrontReg, lhsReg, rhsReg);
        builder.createSltiuInst(intBinaryFrontReg, intBinaryFrontReg, 1);
      }
      break;
    }
    case Mid2EndDAGNode::ICMPNE: {
      auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          if (imm != 0) {
            builder.createXoriInst(intBinaryFrontReg, rhsReg, imm);
            builder.createSltuInst(intBinaryFrontReg, zero, intBinaryFrontReg);
          } else {
            builder.createSltuInst(intBinaryFrontReg, zero, rhsReg);
          }
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createXorInst(intBinaryFrontReg, rhsReg, tmpReg);
          builder.createSltuInst(intBinaryFrontReg, zero, intBinaryFrontReg);
        }
      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          if (imm != 0) {
            builder.createXoriInst(intBinaryFrontReg, lhsReg, imm);
            builder.createSltuInst(intBinaryFrontReg, zero, intBinaryFrontReg);
          } else {
            builder.createSltuInst(intBinaryFrontReg, zero, lhsReg);
          }
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createXorInst(intBinaryFrontReg, lhsReg, tmpReg);
          builder.createSltuInst(intBinaryFrontReg, zero, intBinaryFrontReg);
        }
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createXorInst(intBinaryFrontReg, lhsReg, rhsReg);
        builder.createSltuInst(intBinaryFrontReg, zero, intBinaryFrontReg);
      }
      break;
    }
    case Mid2EndDAGNode::ICMPLT: {
      auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        if (imm != 0) {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createSltInst(intBinaryFrontReg, tmpReg, rhsReg);
        } else {
          builder.createSltInst(intBinaryFrontReg, zero, rhsReg);
        }
      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          builder.createSltiInst(intBinaryFrontReg, lhsReg, imm);
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createSltInst(intBinaryFrontReg, lhsReg, tmpReg);
        }
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createSltInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::ICMPGT: {
      auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          builder.createSltiInst(intBinaryFrontReg, rhsReg, imm);
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createSltInst(intBinaryFrontReg, rhsReg, tmpReg);
        }
      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        if (imm != 0) {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createSltInst(intBinaryFrontReg, tmpReg, lhsReg);
        } else {
          builder.createSltInst(intBinaryFrontReg, zero, lhsReg);
        }
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createSltInst(intBinaryFrontReg, rhsReg, lhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::ICMPLE: {
      auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          builder.createSltiInst(intBinaryFrontReg, rhsReg, imm);
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createSltInst(intBinaryFrontReg, rhsReg, tmpReg);
        }
      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        if (imm != 0) {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createSltInst(intBinaryFrontReg, tmpReg, lhsReg);
        } else {
          builder.createSltInst(intBinaryFrontReg, zero, lhsReg);
        }
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createSltInst(intBinaryFrontReg, rhsReg, lhsReg);
      }
      builder.createXoriInst(intBinaryFrontReg, intBinaryFrontReg, 1);
      break;
    }
    case Mid2EndDAGNode::ICMPGE: {
      auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
      if (lhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(lhs)->getInt();
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        if (imm != 0) {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createSltInst(intBinaryFrontReg, tmpReg, rhsReg);
        } else {
          builder.createSltInst(intBinaryFrontReg, zero, rhsReg);
        }
      } else if (rhs->getNodeType() == Mid2EndDAGNode::INT) {
        auto imm = dynamic_cast<Mid2EndIntNode *>(rhs)->getInt();
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        if (imm <= IMM_POS_MAX && imm >= IMM_NEG_MAX) {
          builder.createSltiInst(intBinaryFrontReg, lhsReg, imm);
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createSltInst(intBinaryFrontReg, lhsReg, tmpReg);
        }
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, false));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, false));
        builder.createSltInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      builder.createXoriInst(intBinaryFrontReg, intBinaryFrontReg, 1);
      break;
    }
    default:
      assert(false);
  }
  auto iter = std::next(intBinaryNames.begin());
  for (unsigned i = 1; i < intBinaryNames.size(); i++) {
    auto destReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(*iter, function, true, false));
    builder.createAddiwInst(destReg, intBinaryFrontReg, 0);
    iter++;
  }
}
/**
 * @brief 处理Uint64Binary节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteUint64BinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto uint64BinaryNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
  auto lhs = uint64BinaryNode->getLhs();
  auto rhs = uint64BinaryNode->getRhs();
  auto lhsName = getOperandName(lhs, function, DAG);
  auto rhsName = getOperandName(rhs, function, DAG);
  auto uint64BinaryNames = getOutputNames(node, function, DAG);
  auto uint64BinaryFrontReg =
      dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(uint64BinaryNames.front(), function, true, true));
  std::stringstream ss;
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::ADD64: {
      if (lhs->getNodeType() == Mid2EndDAGNode::UINT64 || lhs->getNodeType() == Mid2EndDAGNode::INT) {
        uint64_t imm;
        if (lhs->getNodeType() == Mid2EndDAGNode::UINT64) {
          imm = dynamic_cast<Mid2EndUint64Node *>(lhs)->getUint64();
        } else {
          imm = static_cast<int64_t>(dynamic_cast<Mid2EndIntNode *>(lhs)->getInt());
        }
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, true));
        if (imm <= IMM_MAX) {
          builder.createAddiInst(uint64BinaryFrontReg, rhsReg, static_cast<int>(static_cast<uint32_t>(imm)));
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createAddInst(uint64BinaryFrontReg, tmpReg, rhsReg);
        }
      } else if (rhs->getNodeType() == Mid2EndDAGNode::UINT64 || rhs->getNodeType() == Mid2EndDAGNode::INT) {
        uint64_t imm;
        if (rhs->getNodeType() == Mid2EndDAGNode::UINT64) {
          imm = dynamic_cast<Mid2EndUint64Node *>(rhs)->getUint64();
        } else {
          imm = static_cast<int64_t>(dynamic_cast<Mid2EndIntNode *>(rhs)->getInt());
        }
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, true));
        if (imm <= IMM_MAX) {
          builder.createAddiInst(uint64BinaryFrontReg, lhsReg, static_cast<int>(static_cast<uint32_t>(imm)));
        } else {
          auto tmpReg = getOrEmplaceConstant(imm);
          builder.createAddInst(uint64BinaryFrontReg, lhsReg, tmpReg);
        }
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, true));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, true));
        builder.createAddInst(uint64BinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::MUL64: {
      if (lhs->getNodeType() == Mid2EndDAGNode::UINT64 || lhs->getNodeType() == Mid2EndDAGNode::INT) {
        uint64_t imm;
        if (lhs->getNodeType() == Mid2EndDAGNode::UINT64) {
          imm = dynamic_cast<Mid2EndUint64Node *>(lhs)->getUint64();
        } else {
          imm = static_cast<int64_t>(dynamic_cast<Mid2EndIntNode *>(lhs)->getInt());
        }
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, true));
        MultiplyToShift(rhsReg, uint64BinaryFrontReg, imm, true);
      } else if (rhs->getNodeType() == Mid2EndDAGNode::UINT64 || rhs->getNodeType() == Mid2EndDAGNode::INT) {
        uint64_t imm;
        if (rhs->getNodeType() == Mid2EndDAGNode::UINT64) {
          imm = dynamic_cast<Mid2EndUint64Node *>(rhs)->getUint64();
        } else {
          imm = static_cast<int64_t>(dynamic_cast<Mid2EndIntNode *>(rhs)->getInt());
        }
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, true));
        MultiplyToShift(lhsReg, uint64BinaryFrontReg, imm, true);
      } else {
        auto lhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(lhsName, function, true, true));
        auto rhsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rhsName, function, true, true));
        builder.createMulInst(uint64BinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    default:
      assert(false);
  }
  auto iter = std::next(uint64BinaryNames.begin());
  for (unsigned i = 1; i < uint64BinaryNames.size(); i++) {
    auto destReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(*iter, function, true, true));
    builder.createAddiInst(destReg, uint64BinaryFrontReg, 0);
    iter++;
  }
}
/**
 * @brief 处理FloatBinary节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteFloatBinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto floatBinaryNode = dynamic_cast<Mid2EndBinaryOpNode *>(node);
  auto lhs = floatBinaryNode->getLhs();
  auto rhs = floatBinaryNode->getRhs();
  auto lhsName = getOperandName(lhs, function, DAG);
  auto rhsName = getOperandName(rhs, function, DAG);
  auto floatBinaryNames = getOutputNames(floatBinaryNode, function, DAG);
  std::stringstream ss;
  bool isFloat;
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::FADD: {
      isFloat = true;
      auto floatBinaryFrontReg =
          dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(floatBinaryNames.front(), function, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFadd_sInst(floatBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFadd_sInst(floatBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFadd_sInst(floatBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FSUB: {
      isFloat = true;
      auto floatBinaryFrontReg =
          dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(floatBinaryNames.front(), function, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFsub_sInst(floatBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFsub_sInst(floatBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFsub_sInst(floatBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FMUL: {
      isFloat = true;
      auto floatBinaryFrontReg =
          dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(floatBinaryNames.front(), function, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFmul_sInst(floatBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFmul_sInst(floatBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFmul_sInst(floatBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FDIV: {
      isFloat = true;
      auto floatBinaryFrontReg =
          dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(floatBinaryNames.front(), function, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFdiv_sInst(floatBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFdiv_sInst(floatBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFdiv_sInst(floatBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FEQ: {
      isFloat = false;
      auto intBinaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFeq_sInst(intBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFeq_sInst(intBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFeq_sInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FLT: {
      isFloat = false;
      auto intBinaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFlt_sInst(intBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFlt_sInst(intBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFlt_sInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FLE: {
      isFloat = false;
      auto intBinaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFle_sInst(intBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFle_sInst(intBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFle_sInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FCMPEQ: {
      isFloat = false;
      auto intBinaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFeq_sInst(intBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFeq_sInst(intBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFeq_sInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FCMPNE: {
      isFloat = false;
      auto intBinaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFeq_sInst(intBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFeq_sInst(intBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFeq_sInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      builder.createXoriInst(intBinaryFrontReg, intBinaryFrontReg, 1);
      break;
    }
    case Mid2EndDAGNode::FCMPLT: {
      isFloat = false;
      auto intBinaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFlt_sInst(intBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFlt_sInst(intBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFlt_sInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FCMPGT: {
      isFloat = false;
      auto intBinaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFlt_sInst(intBinaryFrontReg, rhsReg, tmpReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFlt_sInst(intBinaryFrontReg, tmpReg, lhsReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFlt_sInst(intBinaryFrontReg, rhsReg, lhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FCMPLE: {
      isFloat = false;
      auto intBinaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFle_sInst(intBinaryFrontReg, tmpReg, rhsReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFle_sInst(intBinaryFrontReg, lhsReg, tmpReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFle_sInst(intBinaryFrontReg, lhsReg, rhsReg);
      }
      break;
    }
    case Mid2EndDAGNode::FCMPGE: {
      isFloat = false;
      auto intBinaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
      if (lhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(lhs)->getFloat());
        builder.createFle_sInst(intBinaryFrontReg, rhsReg, tmpReg);

      } else if (rhs->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto tmpReg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(rhs)->getFloat());
        builder.createFle_sInst(intBinaryFrontReg, tmpReg, lhsReg);
      } else {
        auto lhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(lhsName, function, false));
        auto rhsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rhsName, function, false));
        builder.createFle_sInst(intBinaryFrontReg, rhsReg, lhsReg);
      }
      break;
    }
    default:
      assert(false);
  }
  if (isFloat) {
    auto floatBinaryFrontReg =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(floatBinaryNames.front(), function, false));
    auto iter = std::next(floatBinaryNames.begin());
    for (unsigned i = 1; i < floatBinaryNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(*iter, function, false));
      builder.createFsgnj_sInst(destReg, floatBinaryFrontReg, floatBinaryFrontReg);
      iter++;
    }
  } else {
    auto intBinaryFrontReg =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(floatBinaryNames.front(), function, true, false));
    auto iter = std::next(floatBinaryNames.begin());
    for (unsigned i = 1; i < floatBinaryNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(*iter, function, true, false));
      builder.createAddiwInst(destReg, intBinaryFrontReg, 0);
      iter++;
    }
  }
}
/**
 * @brief 处理IntUnary节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteIntUnaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto intUnaryNode = dynamic_cast<Mid2EndUnaryOpNode *>(node);
  auto hs = intUnaryNode->getHs();
  auto hsName = getOperandName(hs, function, DAG);
  auto intUnaryNames = getOutputNames(intUnaryNode, function, DAG);
  auto hsReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(hsName, function, true, false));
  unsigned returnType;
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::NEG: {
      auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
      auto intUnaryFrontReg =
          dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(intUnaryNames.front(), function, true, false));
      builder.createSubwInst(intUnaryFrontReg, zero, hsReg);
      returnType = 0;
      break;
    }
    case Mid2EndDAGNode::ITOF: {
      auto floatUnaryFrontReg =
          dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(intUnaryNames.front(), function, false));
      builder.createFcvt_s_wInst(floatUnaryFrontReg, hsReg);
      returnType = 1;
      break;
    }
    case Mid2EndDAGNode::SEXTW: {
      auto intUnaryFrontReg =
          dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(intUnaryNames.front(), function, true, true));
      builder.createSext_wInst(intUnaryFrontReg, hsReg);
      returnType = 3;
      break;
    }
    case Mid2EndDAGNode::NOT: {
      auto intUnaryFrontReg =
          dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(intUnaryNames.front(), function, true, true));
      builder.createSltiuInst(intUnaryFrontReg, hsReg, 1);
      returnType = 0;
      break;
    }
    case Mid2EndDAGNode::BIT_ITOF: {
      auto floatUnaryFrontReg =
          dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(intUnaryNames.front(), function, false));
      builder.createFmv_w_xInst(floatUnaryFrontReg, hsReg);
      returnType = 1;
      break;
    }
    default:
      assert(false);
  }
  if (returnType == 0) {
    auto intUnaryFrontReg =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(intUnaryNames.front(), function, true, false));
    auto iter = std::next(intUnaryNames.begin());
    for (unsigned i = 1; i < intUnaryNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(*iter, function, true, false));
      builder.createAddiwInst(destReg, intUnaryFrontReg, 0);
      iter++;
    }
  } else if (returnType == 1) {
    auto floatUnaryFrontReg =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(intUnaryNames.front(), function, false));
    auto iter = std::next(intUnaryNames.begin());
    for (unsigned i = 1; i < intUnaryNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(*iter, function, false));
      builder.createFsgnj_sInst(destReg, floatUnaryFrontReg, floatUnaryFrontReg);
      iter++;
    }
  } else if (returnType == 3) {
    auto intUnaryFrontReg =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(intUnaryNames.front(), function, true, true));
    auto iter = std::next(intUnaryNames.begin());
    for (unsigned i = 1; i < intUnaryNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(*iter, function, true, true));
      builder.createAddiInst(destReg, intUnaryFrontReg, 0);
      iter++;
    }
  }
}
/**
 * @brief 处理FloatUnary节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteFloatUnaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto floatUnaryNode = dynamic_cast<Mid2EndUnaryOpNode *>(node);
  auto hs = floatUnaryNode->getHs();
  auto hsName = getOperandName(hs, function, DAG);
  auto floatUnaryNames = getOutputNames(floatUnaryNode, function, DAG);
  unsigned returnType;
  auto hsReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(hsName, function, false));
  switch (node->getNodeType()) {
    case Mid2EndDAGNode::FNEG: {
      auto floatUnaryFrontReg =
          dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(floatUnaryNames.front(), function, false));
      builder.createFsgnjn_sInst(floatUnaryFrontReg, hsReg, hsReg);
      returnType = 1;
      break;
    }
    case Mid2EndDAGNode::FTOI: {
      auto intUnaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatUnaryNames.front(), function, true, false));
      builder.createFcvt_w_sInst(intUnaryFrontReg, hsReg);
      returnType = 0;
      break;
    }
    case Mid2EndDAGNode::FNZERO: {
      auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
      auto intUnaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatUnaryNames.front(), function, true, false));
      builder.createFclass_sInst(intUnaryFrontReg, hsReg);
      builder.createAndiInst(intUnaryFrontReg, intUnaryFrontReg, 3 << 3);
      builder.createSltuInst(intUnaryFrontReg, zero, intUnaryFrontReg);
      returnType = 0;
      break;
    }
    case Mid2EndDAGNode::BIT_FTOI: {
      auto intUnaryFrontReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(floatUnaryNames.front(), function, true, false));
      builder.createFmv_x_wInst(intUnaryFrontReg, hsReg);
      returnType = 0;
      break;
    }
    default:
      assert(false);
  }
  if (returnType == 0) {
    auto intUnaryFrontReg =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(floatUnaryNames.front(), function, true, false));
    auto iter = std::next(floatUnaryNames.begin());
    for (unsigned i = 1; i < floatUnaryNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(*iter, function, true, false));
      builder.createAddiwInst(destReg, intUnaryFrontReg, 0);
      iter++;
    }
  } else if (returnType == 1) {
    auto floatUnaryFrontReg =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(floatUnaryNames.front(), function, false));
    auto iter = std::next(floatUnaryNames.begin());
    for (unsigned i = 1; i < floatUnaryNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(*iter, function, false));
      builder.createFsgnj_sInst(destReg, floatUnaryFrontReg, floatUnaryFrontReg);
      iter++;
    }
  }
}
/**
 * @brief 处理Call节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteCallNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto sp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::sp);
  auto callNode = dynamic_cast<Mid2EndCallNode *>(node);
  auto callee = callNode->getCallee();
  auto backendCallee = backendModule->getFunction(callee->getName());
  auto calleeStackTable = functionStackMap.at(backendCallee).get();
  auto callerStackTable = functionStackMap.at(backendModule->getFunction(function->getName())).get();
  std::list<std::string> callNames;
  if (callNode->getCallee()->getReturnType() != Function::VOID) {
    callNames = getOutputNames(callNode, function, DAG);
  }
  std::list<Mid2EndDAGNode *> notSpillParams;
  std::list<Mid2EndDAGNode *> spilledParams;
  std::list<riscv::SymRegister *> spiiledArgs;
  unsigned numIntRegs = 0;
  unsigned numFloatRegs = 0;
  unsigned index = 0;
  for (const auto &param : callNode->getParams()) {
    if (getOperandType(param) == 0 || getOperandType(param) == 3) {
      if (numIntRegs < 8) {
        notSpillParams.emplace_back(param);
      } else {
        if (getOperandType(param) == 0) {
          spiiledArgs.emplace_back(getOrEmplaceSymRegister(
              getOperandName(callee->getParam(index), callee, callee->getEntryDAG()), callee, true, false));
        } else {
          spiiledArgs.emplace_back(getOrEmplaceSymRegister(
              getOperandName(callee->getParam(index), callee, callee->getEntryDAG()), callee, true, true));
        }
        spilledParams.emplace_back(param);
      }
      numIntRegs++;
    } else if (getOperandType(param) == 1) {
      if (numFloatRegs < 8) {
        notSpillParams.emplace_back(param);
      } else {
        spiiledArgs.emplace_back(getOrEmplaceSymRegister(
            getOperandName(callee->getParam(index), callee, callee->getEntryDAG()), callee, false));
        spilledParams.emplace_back(param);
      }
      numFloatRegs++;
    } else {
      assert(false);
    }
    index++;
  }

  if (!spilledParams.empty()) {
    auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
    builder.createAddiInst(addrReg, sp, 0);
    auto spilledParamIter = spilledParams.begin();
    auto spilledArgIter = spiiledArgs.begin();
    Mid2EndDAGNode *param;
    uint64_t offset = calleeStackTable->getSpillParamAddr(*spilledArgIter);
    uint64_t lastAddr = offset;
    for (unsigned int i = 0; i < spilledParams.size(); i++) {
      param = *spilledParamIter;
      if (getOperandType(param) == 0) {
        riscv::IntSymRegister *spilledParamReg;
        if (param->getNodeType() == Mid2EndDAGNode::INT) {
          auto intArg = dynamic_cast<Mid2EndIntNode *>(param)->getInt();
          spilledParamReg = getOrEmplaceConstant(intArg);
        } else {
          spilledParamReg = dynamic_cast<riscv::IntSymRegister *>(
              getOrEmplaceSymRegister(getOperandName(param, function, DAG), function, true, false));
        }
        builder.createSwInst(spilledParamReg, addrReg, 0);
        if (i != spilledParams.size() - 1) {
          spilledArgIter++;
          offset = calleeStackTable->getSpillParamAddr(*spilledArgIter);
          builder.createAddiInst(addrReg, addrReg, offset - lastAddr);
        }
        lastAddr = offset;
      } else if (getOperandType(param) == 1) {
        riscv::FloatSymRegister *spilledParamReg;
        if (param->getNodeType() == Mid2EndDAGNode::FLOAT) {
          auto floatArg = dynamic_cast<Mid2EndFloatNode *>(param)->getFloat();
          spilledParamReg = getOrEmplaceConstant(floatArg);
        } else {
          spilledParamReg = dynamic_cast<riscv::FloatSymRegister *>(
              getOrEmplaceSymRegister(getOperandName(param, function, DAG), function, false));
        }
        builder.createFswInst(spilledParamReg, addrReg, 0);
        if (i != spilledParams.size() - 1) {
          spilledArgIter++;
          offset = calleeStackTable->getSpillParamAddr(*spilledArgIter);
          builder.createAddiInst(addrReg, addrReg, offset - lastAddr);
        }
        lastAddr = offset;
      } else if (getOperandType(param) == 3) {
        riscv::IntSymRegister *spilledParamReg;
        if (param->getNodeType() == Mid2EndDAGNode::UINT64) {
          auto uint64Arg = dynamic_cast<Mid2EndUint64Node *>(param)->getUint64();
          spilledParamReg = getOrEmplaceConstant(uint64Arg);
        } else {
          spilledParamReg = dynamic_cast<riscv::IntSymRegister *>(
              getOrEmplaceSymRegister(getOperandName(param, function, DAG), function, true, true));
        }
        builder.createSdInst(spilledParamReg, addrReg, 0);
        if (i != spilledParams.size() - 1) {
          spilledArgIter++;
          offset = calleeStackTable->getSpillParamAddr(*spilledArgIter);
          builder.createAddiInst(addrReg, addrReg, offset - lastAddr);
        }
        lastAddr = offset;
      }
      spilledParamIter++;
    }
  }
  numIntRegs = 0;
  numFloatRegs = 0;
  std::list<riscv::FloatSymRegister *> floatConstantParamRegs;
  for (const auto &param : notSpillParams) {
    if (param->getNodeType() == Mid2EndDAGNode::FLOAT) {
      auto floatVal = dynamic_cast<Mid2EndFloatNode *>(param)->getFloat();
      auto tmpReg = getOrEmplaceConstant(floatVal);
      floatConstantParamRegs.emplace_back(tmpReg);
    }
  }
  auto floatConstantRegsIter = floatConstantParamRegs.begin();
  for (const auto &param : notSpillParams) {
    if (getOperandType(param) == 0) {
      auto targetReg = dynamic_cast<riscv::IntPhyRegister *>(getParamPhyReg(true, numIntRegs));
      if (param->getNodeType() == Mid2EndDAGNode::INT) {
        auto intArg = dynamic_cast<Mid2EndIntNode *>(param)->getInt();
        builder.createLiInst(targetReg, static_cast<int64_t>(intArg));
      } else {
        auto sourceReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(param, function, DAG), function, true, false));
        builder.createAddiwInst(targetReg, sourceReg, 0);
      }
      numIntRegs++;
      calleeStackTable->addNotSpillParamReg(targetReg);
    } else if (getOperandType(param) == 1) {
      auto targetReg = dynamic_cast<riscv::FloatPhyRegister *>(getParamPhyReg(false, numFloatRegs));
      if (param->getNodeType() == Mid2EndFloatNode::FLOAT) {
        builder.createFsgnj_sInst(targetReg, *floatConstantRegsIter, *floatConstantRegsIter);
        floatConstantRegsIter++;
      } else {
        auto sourceReg = dynamic_cast<riscv::FloatSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(param, function, DAG), function, false));
        builder.createFsgnj_sInst(targetReg, sourceReg, sourceReg);
      }
      numFloatRegs++;
      calleeStackTable->addNotSpillParamReg(targetReg);
    } else if (getOperandType(param) == 3) {
      auto targetReg = dynamic_cast<riscv::IntPhyRegister *>(getParamPhyReg(true, numIntRegs));
      if (param->getNodeType() == Mid2EndDAGNode::UINT64) {
        auto uint64Arg = dynamic_cast<Mid2EndUint64Node *>(param)->getUint64();
        builder.createLiInst(targetReg, uint64Arg);
      } else {
        auto sourceReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(param, function, DAG), function, true, true));
        builder.createAddiInst(targetReg, sourceReg, 0);
      }
      numIntRegs++;
      calleeStackTable->addNotSpillParamReg(targetReg);
    } else {
      assert(false);
    }
  }

  auto callInst = builder.createCallInst(backendCallee);
  callerStackTable->addCalleeStackTable(callInst, calleeStackTable);
  if (backendCallee->getReturnType() == riscv::Function::INT) {
    auto a0 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
    auto callFrontReg =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(callNames.front(), function, true, false));
    builder.createAddiwInst(callFrontReg, a0, 0);
    auto iter = std::next(callNames.begin());
    for (unsigned i = 1; i < callNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(*iter, function, true, false));
      builder.createAddiwInst(destReg, callFrontReg, 0);
      iter++;
    }
  } else if (backendCallee->getReturnType() == riscv::Function::FLOAT) {
    auto fa0 = riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0);
    auto callFrontReg =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(callNames.front(), function, false));
    builder.createFsgnj_sInst(callFrontReg, fa0, fa0);
    auto iter = std::next(callNames.begin());
    for (unsigned i = 1; i < callNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(*iter, function, false));
      builder.createFsgnj_sInst(destReg, callFrontReg, callFrontReg);
      iter++;
    }
  } else if (backendCallee->getReturnType() == riscv::Function::VOID) {
  } else {
    assert(false);
  }
}
/**
 * @brief 处理CondBr节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteCondBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) { assert(false); }
/**
 * @brief 处理Br节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
  auto brNode = dynamic_cast<Mid2EndBrNode *>(node);
  builder.createJalInst(zero, DAGBlockMap.at(brNode->getThenDAG()));
}
/**
 * @brief 处理Return节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteReturnNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
  auto ra = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::ra);
  auto fp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::fp);
  auto sp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::sp);
  auto returnNode = dynamic_cast<Mid2EndReturnNode *>(node);
  auto returnVal = returnNode->getVal();
  if (returnVal != nullptr) {
    if (returnVal->getNodeType() == Mid2EndDAGNode::INT) {
      auto intVal = static_cast<uint32_t>(dynamic_cast<Mid2EndIntNode *>(returnVal)->getInt());
      auto a0 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
      builder.createLiInst(a0, intVal);
    } else if (returnVal->getNodeType() == Mid2EndDAGNode::FLOAT) {
      auto floatVal = dynamic_cast<Mid2EndFloatNode *>(returnVal)->getFloat();
      auto fa0 = riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0);
      auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
      builder.createLlaInst(addrReg, floatGlobalMap.at(floatVal));
      builder.createFlwInst(fa0, addrReg, 0);
    } else {
      if (getOperandType(returnNode->getVal()) == 0) {
        auto a0 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
        auto sourceReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(returnVal, function, DAG), function, true, false));
        builder.createAddiwInst(a0, sourceReg, 0);
      } else {
        auto fa0 = riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0);
        auto sourceReg = dynamic_cast<riscv::FloatSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(returnVal, function, DAG), function, false));
        builder.createFsgnj_sInst(fa0, sourceReg, sourceReg);
      }
    }
  } else {
    if (function->getReturnType() == Function::INT) {
      auto a0 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
      builder.createLiInst(a0, 0);
    } else if (function->getReturnType() == Function::FLOAT) {
      auto fa0 = riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0);
      auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
      if (floatGlobalMap.find(0.0F) != floatGlobalMap.end()) {
        builder.createLlaInst(addrReg, floatGlobalMap.at(0.0F));
        builder.createFlwInst(fa0, addrReg, 0);
      } else {
        std::stringstream ss;
        ss << ".F" << floatIndex;
        floatIndex++;
        ConstantCounter init;
        init.push_back(0.0F);
        auto floatGlobal = backendModule->createGlobal(false, {}, init, ss.str());
        floatGlobalMap.emplace(0.0F, floatGlobal);
        builder.createLlaInst(addrReg, floatGlobal);
        builder.createFlwInst(fa0, addrReg, 0);
      }
    }
  }

  builder.createLdInst(ra, fp, -8);
  builder.createLdInst(fp, fp, -16);
  builder.createAddiInst(sp, sp, 0);
  builder.createJalrInst(zero, ra, 0);
}
/**
 * @brief 处理Load节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteLoadNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) { assert(false); }
/**
 * @brief 处理Store节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteStoreNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) { assert(false); }
/**
 * @brief 处理La节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteLaNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) { assert(false); }
/**
 * @brief 处理Memset节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteMemsetNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
  auto memsetNode = dynamic_cast<Mid2EndMemsetNode *>(node);
  auto offset = 4 * static_cast<uint64_t>(memsetNode->getBegin());
  auto value = memsetNode->getValue();
  auto size = memsetNode->getSize();
  auto pointerName = getOperandName(memsetNode->getPointer(), function, DAG);
  auto pointerReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(pointerName, function, true, true));
  if (size == 1) {
    if (getOperandType(value) == 0) {
      riscv::IntRegister *valueReg;
      if (value->getNodeType() == Mid2EndDAGNode::INT) {
        auto intValue = dynamic_cast<Mid2EndIntNode *>(value)->getInt();
        if (intValue != 0) {
          valueReg = getOrEmplaceConstant(intValue);
        } else {
          valueReg = zero;
        }
      } else {
        valueReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(value, function, DAG), function, true, false));
      }
      if (offset <= static_cast<uint32_t>(IMM_POS_MAX)) {
        builder.createSwInst(valueReg, pointerReg, offset);
      } else {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
        builder.createLiInst(addrReg, offset);
        builder.createAddInst(addrReg, addrReg, pointerReg);
        builder.createSwInst(valueReg, addrReg, 0);
      }
    } else {
      riscv::FloatSymRegister *valueReg;
      if (value->getNodeType() == Mid2EndDAGNode::FLOAT) {
        auto floatValue = dynamic_cast<Mid2EndFloatNode *>(value)->getFloat();
        valueReg = getOrEmplaceConstant(floatValue);
      } else {
        valueReg = dynamic_cast<riscv::FloatSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(value, function, DAG), function, false));
      }
      if (offset <= static_cast<uint32_t>(IMM_POS_MAX)) {
        builder.createFswInst(valueReg, pointerReg, offset);
      } else {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
        builder.createLiInst(addrReg, offset);
        builder.createAddInst(addrReg, addrReg, pointerReg);
        builder.createFswInst(valueReg, addrReg, 0);
      }
    }
  } else {
    auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
    if (offset <= IMM_MAX) {
      builder.createAddiInst(addrReg, pointerReg, static_cast<int>(static_cast<uint32_t>(offset)));
    } else {
      builder.createLiInst(addrReg, offset);
      builder.createAddInst(addrReg, pointerReg, addrReg);
    }
    if (value->getNodeType() == Mid2EndIntNode::INT && dynamic_cast<Mid2EndIntNode *>(value)->getInt() == 0) {
      auto backendFunction = backendModule->getFunction(function->getName());
      auto callerStackTable = functionStackMap.at(backendFunction).get();
      auto a0 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
      auto a1 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a1);
      auto a2 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a2);
      builder.createAddiInst(a0, addrReg, 0);
      builder.createLiInst(a1, 0);
      builder.createLiInst(a2, size);
      auto callInst = builder.createCallInst(getOrEmplaceMemset());
      auto calleeStackTable = functionStackMap.at(backendModule->getFunction("my_memset")).get();
      callerStackTable->addCalleeStackTable(callInst, calleeStackTable);
    } else if (value->getNodeType() == Mid2EndIntNode::FLOAT &&
               dynamic_cast<Mid2EndFloatNode *>(value)->getFloat() == 0.0) {
      auto backendFunction = backendModule->getFunction(function->getName());
      auto callerStackTable = functionStackMap.at(backendFunction).get();
      auto a0 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
      auto a1 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a1);
      auto a2 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a2);
      float floatZero = 0.0;
      builder.createAddiInst(a0, addrReg, 0);
      builder.createLiInst(a1, *(reinterpret_cast<unsigned *>(&floatZero)));
      builder.createLiInst(a2, size);
      auto callInst = builder.createCallInst(getOrEmplaceMemset());
      auto calleeStackTable = functionStackMap.at(backendModule->getFunction("my_memset")).get();
      callerStackTable->addCalleeStackTable(callInst, calleeStackTable);
    } else {
      if (getOperandType(value) == 0) {
        riscv::IntSymRegister *valueReg;
        if (value->getNodeType() == Mid2EndDAGNode::INT) {
          auto intValue = dynamic_cast<Mid2EndIntNode *>(value)->getInt();
          valueReg = getOrEmplaceConstant(intValue);
        } else {
          valueReg = dynamic_cast<riscv::IntSymRegister *>(
              getOrEmplaceSymRegister(getOperandName(value, function, DAG), function, true, false));
        }
        for (unsigned i = 0; i < size; i++) {
          builder.createSwInst(valueReg, addrReg, 0);
          if (i != size - 1) {
            builder.createAddiInst(addrReg, addrReg, 4);
          }
        }
      } else {
        riscv::FloatSymRegister *valueReg;
        if (value->getNodeType() == Mid2EndDAGNode::FLOAT) {
          auto floatValue = dynamic_cast<Mid2EndFloatNode *>(value)->getFloat();
          valueReg = getOrEmplaceConstant(floatValue);
        } else {
          valueReg = dynamic_cast<riscv::FloatSymRegister *>(
              getOrEmplaceSymRegister(getOperandName(value, function, DAG), function, false));
        }
        for (unsigned i = 0; i < size; i++) {
          builder.createFswInst(valueReg, addrReg, 0);
          if (i != size - 1) {
            builder.createAddiInst(addrReg, addrReg, 4);
          }
        }
      }
    }
  }
}
/**
 * @brief 处理RiscvCondBr节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteRiscvCondBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto riscvCondBrNode = dynamic_cast<Mid2EndRiscvCondBrNode *>(node);
  auto lhsNode = riscvCondBrNode->getLhsNode();
  auto rhsNode = riscvCondBrNode->getRhsNode();
  riscv::IntRegister *lhsReg;
  riscv::IntRegister *rhsReg;
  if (lhsNode->getNodeType() == Mid2EndDAGNode::INT) {
    auto intValue = dynamic_cast<Mid2EndIntNode *>(lhsNode)->getInt();
    if (intValue == 0) {
      lhsReg = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
    } else {
      lhsReg = getOrEmplaceConstant(intValue);
    }
  } else {
    lhsReg = dynamic_cast<riscv::IntSymRegister *>(
        getOrEmplaceSymRegister(getOperandName(lhsNode, function, DAG), function, true, false));
  }
  if (rhsNode->getNodeType() == Mid2EndDAGNode::INT) {
    auto intValue = dynamic_cast<Mid2EndIntNode *>(rhsNode)->getInt();
    if (intValue == 0) {
      rhsReg = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
    } else {
      rhsReg = getOrEmplaceConstant(intValue);
    }
  } else {
    rhsReg = dynamic_cast<riscv::IntSymRegister *>(
        getOrEmplaceSymRegister(getOperandName(rhsNode, function, DAG), function, true, false));
  }
  switch (riscvCondBrNode->getNodeType()) {
    case Mid2EndDAGNode::BEQ: {
      builder.createBeq(lhsReg, rhsReg, DAGBlockMap.at(riscvCondBrNode->getThenDAG()));
      break;
    }
    case Mid2EndDAGNode::BNE: {
      builder.createBne(lhsReg, rhsReg, DAGBlockMap.at(riscvCondBrNode->getThenDAG()));
      break;
    }
    case Mid2EndDAGNode::BLT: {
      builder.createBlt(lhsReg, rhsReg, DAGBlockMap.at(riscvCondBrNode->getThenDAG()));
      break;
    }
    case Mid2EndDAGNode::BLTU: {
      builder.createBltu(lhsReg, rhsReg, DAGBlockMap.at(riscvCondBrNode->getThenDAG()));
      break;
    }
    case Mid2EndDAGNode::BGE: {
      builder.createBge(lhsReg, rhsReg, DAGBlockMap.at(riscvCondBrNode->getThenDAG()));
      break;
    }
    case Mid2EndDAGNode::BGEU: {
      builder.createBgeu(lhsReg, rhsReg, DAGBlockMap.at(riscvCondBrNode->getThenDAG()));
      break;
    }
    default:
      assert(false);
  }
}
/**
 * @brief 处理RiscvF3Op节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteRiscvF3OpNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto f3OpNode = dynamic_cast<Mid2EndF3OpNode *>(node);
  auto hs0 = f3OpNode->getHs(0);
  auto hs1 = f3OpNode->getHs(1);
  auto hs2 = f3OpNode->getHs(2);
  auto hs0Name = getOperandName(hs0, function, DAG);
  auto hs1Name = getOperandName(hs1, function, DAG);
  auto hs2Name = getOperandName(hs2, function, DAG);
  auto f3OpNames = getOutputNames(f3OpNode, function, DAG);

  riscv::FloatSymRegister *hs0Reg;
  riscv::FloatSymRegister *hs1Reg;
  riscv::FloatSymRegister *hs2Reg;

  if (hs0->getNodeType() == Mid2EndDAGNode::FLOAT) {
    hs0Reg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(hs0)->getFloat());
  } else {
    hs0Reg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(hs0Name, function, false));
  }
  if (hs1->getNodeType() == Mid2EndDAGNode::FLOAT) {
    hs1Reg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(hs1)->getFloat());
  } else {
    hs1Reg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(hs1Name, function, false));
  }
  if (hs2->getNodeType() == Mid2EndDAGNode::FLOAT) {
    hs2Reg = getOrEmplaceConstant(dynamic_cast<Mid2EndFloatNode *>(hs2)->getFloat());
  } else {
    hs2Reg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(hs2Name, function, false));
  }

  auto f3OpFrontReg =
      dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(f3OpNames.front(), function, false));
  switch (node->getNodeType()) {
    case Mid2EndFloatNode::FMADD: {
      builder.createFmadd_sInst(f3OpFrontReg, hs0Reg, hs1Reg, hs2Reg);
      break;
    }
    case Mid2EndDAGNode::FMSUB: {
      builder.createFmsub_sInst(f3OpFrontReg, hs0Reg, hs1Reg, hs2Reg);
      break;
    }
    case Mid2EndDAGNode::FNMADD: {
      builder.createFnmadd_sInst(f3OpFrontReg, hs0Reg, hs1Reg, hs2Reg);
      break;
    }
    case Mid2EndDAGNode::FNMSUB: {
      builder.createFnmsub_sInst(f3OpFrontReg, hs0Reg, hs1Reg, hs2Reg);
      break;
    }
    default:
      assert(false);
  }
  auto iter = std::next(f3OpNames.begin());
  for (unsigned i = 1; i < f3OpNames.size(); i++) {
    auto destReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(*iter, function, false));
    builder.createFsgnj_sInst(destReg, f3OpFrontReg, f3OpFrontReg);
    iter++;
  }
}
/**
 * @brief 处理RiscvLoad节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteRiscvLoadNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto rvloadNode = dynamic_cast<Mid2EndRVLoadNode *>(node);
  auto rvloadNames = getOutputNames(rvloadNode, function, DAG);
  auto offsetNode = rvloadNode->getOffset();
  auto pointerReg = dynamic_cast<riscv::IntSymRegister *>(
      getOrEmplaceSymRegister(getOperandName(rvloadNode->getPointer(), function, DAG), function, true, true));
  if (getOperandType(node) == 0) {
    auto rvLoadFrontReg =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(rvloadNames.front(), function, true, false));
    if (offsetNode->getNodeType() == Mid2EndDAGNode::UINT64) {
      auto offset = dynamic_cast<Mid2EndUint64Node *>(offsetNode)->getUint64();
      if (offset > static_cast<uint32_t>(IMM_POS_MAX)) {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
        builder.createLiInst(addrReg, offset);
        builder.createAddInst(addrReg, addrReg, pointerReg);
        builder.createLwInst(rvLoadFrontReg, addrReg, 0);
      } else {
        builder.createLwInst(rvLoadFrontReg, pointerReg, static_cast<int>(static_cast<uint32_t>(offset)));
      }
    } else {
      auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
      auto offsetReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(getOperandName(offsetNode, function, DAG), function, true, true));
      builder.createAddInst(addrReg, pointerReg, offsetReg);
      builder.createLwInst(rvLoadFrontReg, addrReg, 0);
    }
    auto iter = std::next(rvloadNames.begin());
    for (unsigned i = 1; i < rvloadNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(*iter, function, true, false));
      builder.createAddiwInst(destReg, rvLoadFrontReg, 0);
      iter++;
    }
  } else {
    auto rvLoadFrontReg =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(rvloadNames.front(), function, false));
    if (offsetNode->getNodeType() == Mid2EndDAGNode::UINT64) {
      auto offset = dynamic_cast<Mid2EndUint64Node *>(offsetNode)->getUint64();
      if (offset > static_cast<uint32_t>(IMM_POS_MAX)) {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
        builder.createLiInst(addrReg, offset);
        builder.createAddInst(addrReg, addrReg, pointerReg);
        builder.createFlwInst(rvLoadFrontReg, addrReg, 0);
      } else {
        builder.createFlwInst(rvLoadFrontReg, pointerReg, static_cast<int>(static_cast<uint32_t>(offset)));
      }
    } else {
      auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
      auto offsetReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(getOperandName(offsetNode, function, DAG), function, true, true));
      builder.createAddInst(addrReg, pointerReg, offsetReg);
      builder.createFlwInst(rvLoadFrontReg, addrReg, 0);
    }
    auto iter = std::next(rvloadNames.begin());
    for (unsigned i = 1; i < rvloadNames.size(); i++) {
      auto destReg = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(*iter, function, false));
      builder.createFsgnj_sInst(destReg, rvLoadFrontReg, rvLoadFrontReg);
      iter++;
    }
  }
}
/**
 * @brief 处理RiscvStore节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::interpreteRiscvStoreNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
  auto rvstoreNode = dynamic_cast<Mid2EndRVStoreNode *>(node);
  auto valueNode = rvstoreNode->getValue();
  auto offsetNode = rvstoreNode->getOffset();
  auto pointerReg = dynamic_cast<riscv::IntSymRegister *>(
      getOrEmplaceSymRegister(getOperandName(rvstoreNode->getPointer(), function, DAG), function, true, true));
  if (getOperandType(valueNode) == 0) {
    riscv::IntRegister *valueReg;
    if (valueNode->getNodeType() == Mid2EndDAGNode::INT) {
      auto intValue = dynamic_cast<Mid2EndIntNode *>(valueNode)->getInt();
      if (intValue != 0) {
        valueReg = getOrEmplaceConstant(intValue);
      } else {
        valueReg = zero;
      }
    } else {
      valueReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(getOperandName(valueNode, function, DAG), function, true, false));
    }
    if (offsetNode->getNodeType() == Mid2EndDAGNode::UINT64) {
      auto offset = dynamic_cast<Mid2EndUint64Node *>(offsetNode)->getUint64();
      if (offset > static_cast<uint32_t>(IMM_POS_MAX)) {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
        builder.createLiInst(addrReg, offset);
        builder.createAddInst(addrReg, addrReg, pointerReg);
        builder.createSwInst(valueReg, addrReg, 0);
      } else {
        builder.createSwInst(valueReg, pointerReg, static_cast<int>(static_cast<uint32_t>(offset)));
      }
    } else {
      auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
      auto offsetReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(getOperandName(offsetNode, function, DAG), function, true, true));
      builder.createAddInst(addrReg, pointerReg, offsetReg);
      builder.createSwInst(valueReg, addrReg, 0);
    }
  } else {
    riscv::FloatSymRegister *valueReg;
    if (valueNode->getNodeType() == Mid2EndDAGNode::FLOAT) {
      auto floatValue = dynamic_cast<Mid2EndFloatNode *>(valueNode)->getFloat();
      valueReg = getOrEmplaceConstant(floatValue);
    } else {
      valueReg = dynamic_cast<riscv::FloatSymRegister *>(
          getOrEmplaceSymRegister(getOperandName(valueNode, function, DAG), function, false));
    }
    if (offsetNode->getNodeType() == Mid2EndDAGNode::UINT64) {
      auto offset = dynamic_cast<Mid2EndUint64Node *>(offsetNode)->getUint64();
      if (offset > static_cast<uint32_t>(IMM_POS_MAX)) {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
        builder.createLiInst(addrReg, offset);
        builder.createAddInst(addrReg, addrReg, pointerReg);
        builder.createFswInst(valueReg, addrReg, 0);
      } else {
        builder.createFswInst(valueReg, pointerReg, static_cast<int>(static_cast<uint32_t>(offset)));
      }
    } else {
      auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
      auto offsetReg = dynamic_cast<riscv::IntSymRegister *>(
          getOrEmplaceSymRegister(getOperandName(offsetNode, function, DAG), function, true, true));
      builder.createAddInst(addrReg, pointerReg, offsetReg);
      builder.createFswInst(valueReg, addrReg, 0);
    }
  }
}

// 可能有冗余的判断条件，例如39.fp_params.sy，导致错误
/**
 * @brief 处理节点
 *
 * @param [in] node 待处理节点
 * @param [in] function 所处的函数
 * @param [in] DAG 所处的DAG
 * @return 无返回值
 */
void DAG2EndBuilder::processNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) {
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
    case NodeType::POINTER: {
      interpretePointer(node, function, DAG);
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
    case NodeType::BIT_ITOF:
    case NodeType::SEXTW: {
      interpreteIntUnaryNode(node, function, DAG);
      break;
    }

    case NodeType::FNEG:
    case NodeType::FNOT:
    case NodeType::FNZERO:
    case NodeType::FTOI:
    case NodeType::BIT_FTOI: {
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
    default: {
      assert(false);
    }
  }
}
/**
 * @brief 处理DAG
 *
 * @param [in] DAG 待处理DAG
 * @return 无返回值
 */
void DAG2EndBuilder::generateDAG(Mid2EndDAG *DAG) {
  auto backendBlock = DAGBlockMap.at(DAG);
  builder.setPosition(backendBlock);
  std::set<Mid2EndPointerNode *> usedPointerNodes;
  for (const auto &node : DAG->getMidDAGNodes()) {
    isVisited.emplace(node.get(), false);
    isAddToVisited.emplace(node.get(), false);
    for (const auto &used : node->getUseds()) {
      if (used->getNodeType() == Mid2EndDAGNode::POINTER) {
        usedPointerNodes.insert(dynamic_cast<Mid2EndPointerNode *>(used));
      }
    }
  }
  for (const auto &input : DAG->getInputNodes()) {
    varInputMap.emplace(DAG->getParent()->getInputBind().at(input), input);
    isAddToVisited.at(input) = true;
    toVisit.emplace_back(input);
  }
  for (const auto &intConstantItem : DAG->getIntConstants()) {
    isAddToVisited.at(intConstantItem.second) = true;
    toVisit.emplace_back(intConstantItem.second);
  }
  for (const auto &floatConstantItem : DAG->getFloatConstants()) {
    isAddToVisited.at(floatConstantItem.second) = true;
    toVisit.emplace_back(floatConstantItem.second);
  }
  for (const auto &uint64ConstantItem : DAG->getUint64Constants()) {
    isAddToVisited.at(uint64ConstantItem.second) = true;
    toVisit.emplace_back(uint64ConstantItem.second);
  }
  for (const auto &pointer : usedPointerNodes) {
    isVisited.emplace(pointer, false);
    isAddToVisited.emplace(pointer, true);
    toVisit.emplace_back(pointer);
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
      processNode(node, DAG->getParent(), DAG);
    }

    for (const auto &user : node->getUsers()) {
      if (user->getParent() == DAG && !isAddToVisited.at(user)) {
        toVisit.emplace_back(user);
        isAddToVisited.at(user) = true;
      }
    }
  }

  for (const auto &movItem : intReg2regMovMap) {
    auto source =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(movItem.first, DAG->getParent(), true, false));
    for (const auto &destName : movItem.second) {
      auto dest =
          dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(destName, DAG->getParent(), true, false));
      builder.createAddiwInst(dest, source, 0);
    }
  }
  for (const auto &movItem : floatReg2regMovMap) {
    auto source =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(movItem.first, DAG->getParent(), false));
    for (const auto &destName : movItem.second) {
      auto dest = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(destName, DAG->getParent(), false));
      builder.createFsgnj_sInst(dest, source, source);
    }
  }
  for (const auto &movItem : int2regMovMap) {
    for (const auto &destName : movItem.second) {
      auto dest =
          dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(destName, DAG->getParent(), true, false));
      builder.createLiInst(dest, movItem.first);
    }
  }
  for (const auto &movItem : float2regMovMap) {
    for (const auto &destName : movItem.second) {
      auto dest =
          dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(destName, DAG->getParent(), true, false));
      builder.createFsgnj_sInst(dest, movItem.first, movItem.first);
    }
  }

  auto terminator = DAG->getTerminatorNode();
  if (terminator != nullptr) {
    processNode(terminator, DAG->getParent(), DAG);
  }

  intReg2regMovMap.clear();
  floatReg2regMovMap.clear();
  int2regMovMap.clear();
  float2regMovMap.clear();
  DAGIntConstants.clear();
  DAGUint64Constants.clear();
  DAGFloatConstants.clear();
  isVisited.clear();
  isAddToVisited.clear();
  toVisit.clear();
  varInputMap.clear();
}
/**
 * @brief 处理函数
 *
 * @param [in] function 待处理函数
 * @return 无返回值
 */
void DAG2EndBuilder::generateFunction(Function *function) {
  for (const auto &DAG : function->getMidDAGs()) {
    generateDAG(DAG.get());
  }
}
/**
 * @brief 处理全局变量
 *
 * @return 无返回值
 */
void DAG2EndBuilder::generateGlobals() {
  std::stringstream ss;
  for (const auto &globalItem : pModule->getGlobals()) {
    auto globalName = globalItem.first;
    auto global = globalItem.second.get();
    auto globalInit = pModule->getGlobalInit(globalName);
    std::vector<unsigned> dims;
    for (const auto &dim : global->getDims()) {
      dims.emplace_back(static_cast<unsigned>(dim));
    }

    backendModule->createGlobal(global->isIntPointer(), dims, globalInit, globalName);
  }
  std::set<float> floatSet;
  for (const auto &functionItem : pModule->getFunctions()) {
    for (const auto &DAG : functionItem.second->getMidDAGs()) {
      for (const auto &floatConstantItem : DAG->getFloatConstants()) {
        floatSet.insert(floatConstantItem.first);
      }
    }
  }
  for (const auto &floatNumber : floatSet) {
    ss << ".F" << floatIndex;
    ConstantCounter init;
    init.push_back(floatNumber);
    auto global = backendModule->createGlobal(false, {}, init, ss.str());
    floatGlobalMap.emplace(floatNumber, global);
    ss.str("");
    floatIndex++;
  }
}
/**
 * @brief 处理模块
 *
 * @return 无返回值
 */
void DAG2EndBuilder::generateModule() {
  backendModule = std::make_unique<riscv::Module>();
  generateGlobals();
  for (const auto &externalFunctionItem : pModule->getExternalFunctions()) {
    using ReturnType = riscv::Function::ReturnType;
    ReturnType returnType;
    if (externalFunctionItem.second->getReturnType() == Function::VOID) {
      returnType = ReturnType::VOID;
    } else if (externalFunctionItem.second->getReturnType() == Function::INT) {
      returnType = ReturnType::INT;
    } else if (externalFunctionItem.second->getReturnType() == Function::FLOAT) {
      returnType = ReturnType::FLOAT;
    } else {
      assert(false);
    }
    auto curFunction = backendModule->createFunction(externalFunctionItem.first, returnType, true);
    auto curStackTable = new mid2end::StackTable;
    functionStackMap.emplace(curFunction, curStackTable);
  }
  for (const auto &functionItem : pModule->getFunctions()) {
    using ReturnType = riscv::Function::ReturnType;
    ReturnType returnType;
    if (functionItem.second->getReturnType() == Function::VOID) {
      returnType = ReturnType::VOID;
    } else if (functionItem.second->getReturnType() == Function::INT) {
      returnType = ReturnType::INT;
    } else if (functionItem.second->getReturnType() == Function::FLOAT) {
      returnType = ReturnType::FLOAT;
    } else {
      assert(false);
    }

    auto function = functionItem.second.get();
    auto backendFunction = backendModule->createFunction(functionItem.first, returnType, false);
    auto curStackTable = new mid2end::StackTable;
    functionStackMap.emplace(backendFunction, curStackTable);
    auto callerStackTable = functionStackMap.at(backendFunction).get();
    for (const auto &localArray : function->getLocalArrays()) {
      auto &params = function->getParams();
      if (std::find(params.begin(), params.end(), localArray.get()) == params.end()) {
        bool isInt = localArray->isIntPointer();
        std::vector<unsigned> dims;
        for (const auto &dim : localArray->getDims()) {
          dims.emplace_back(static_cast<unsigned>(dim));
        }
        localArrayMap.emplace(localArray.get(), new mid2end::LocalArray(
                                                    isInt, dims, localArrayNameMap.at(function).at(localArray.get())));
        callerStackTable->addLocalArray(localArrayMap.at(localArray.get()).get());
      }
    }
    for (const auto &DAG : function->getMidDAGs()) {
      auto backendBlock = backendFunction->addBasicBlock(DAG->getName());
      DAGBlockMap.emplace(DAG.get(), backendBlock);
    }
    for (const auto &DAG : function->getMidDAGs()) {
      auto backendBlock = DAGBlockMap.at(DAG.get());
      for (const auto &pred : DAG->getPredecessors()) {
        backendBlock->addPredecessor(DAGBlockMap.at(pred));
      }
      for (const auto &succ : DAG->getSuccessors()) {
        backendBlock->addSuccessor(DAGBlockMap.at(succ));
      }
    }

    auto entryBlock = backendFunction->getEntryBlock();
    builder.setPosition(entryBlock);
    auto ra = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::ra);
    auto fp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::fp);
    auto sp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::sp);
    builder.createSdInst(ra, sp, -8);
    builder.createSdInst(fp, sp, -16);
    builder.createAddiInst(fp, sp, 0);
    builder.createAddiInst(sp, sp, 0);

    auto entryDAG = function->getEntryDAG();
    std::list<Mid2EndDAGNode *> notSpillParams;
    std::list<Mid2EndDAGNode *> spilledParams;
    unsigned numIntRegs = 0;
    unsigned numFloatRegs = 0;
    for (const auto &param : function->getParams()) {
      if (getOperandType(param) == 0 || getOperandType(param) == 3) {
        if (numIntRegs < 8) {
          notSpillParams.emplace_back(param);
        } else {
          spilledParams.emplace_back(param);
        }
        numIntRegs++;
      } else if (getOperandType(param) == 1) {
        if (numFloatRegs < 8) {
          notSpillParams.emplace_back(param);
        } else {
          spilledParams.emplace_back(param);
        }
        numFloatRegs++;
      } else {
        assert(false);
      }
    }

    numIntRegs = 0;
    numFloatRegs = 0;
    for (const auto &param : notSpillParams) {
      if (getOperandType(param) == 0) {
        auto sourceReg = dynamic_cast<riscv::IntPhyRegister *>(getParamPhyReg(true, numIntRegs));
        auto targetReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(param, function, entryDAG), function, true, false));
        builder.createAddiwInst(targetReg, sourceReg, 0);
        numIntRegs++;
      } else if (getOperandType(param) == 1) {
        auto sourceReg = dynamic_cast<riscv::FloatPhyRegister *>(getParamPhyReg(false, numFloatRegs));
        auto targetReg = dynamic_cast<riscv::FloatSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(param, function, entryDAG), function, false));
        builder.createFsgnj_sInst(targetReg, sourceReg, sourceReg);
        numFloatRegs++;
      } else if (getOperandType(param) == 3) {
        auto sourceReg = dynamic_cast<riscv::IntPhyRegister *>(getParamPhyReg(true, numIntRegs));
        auto targetReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(param, function, entryDAG), function, true, true));
        builder.createAddiInst(targetReg, sourceReg, 0);
        numIntRegs++;
      } else {
        assert(false);
      }
    }

    if (!spilledParams.empty()) {
      uint64_t lastAddr = 0;
      uint64_t offset = 0;
      auto spilledParamsIter = spilledParams.begin();
      Mid2EndDAGNode *param = *spilledParamsIter;
      auto addrReg = dynamic_cast<riscv::IntSymRegister *>(emplaceTmpSymRegister(true, true));
      builder.createAddiInst(addrReg, fp, 0);
      if (getOperandType(param) == 0) {
        auto spilledParamReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(param, function, entryDAG), function, true, false));
        callerStackTable->addSpillParam(spilledParamReg);
        offset = callerStackTable->getSpillParamAddr(spilledParamReg);
        builder.createLwInst(spilledParamReg, addrReg, 0);
      } else if (getOperandType(param) == 1) {
        auto spilledParamReg = dynamic_cast<riscv::FloatSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(param, function, entryDAG), function, false));
        callerStackTable->addSpillParam(spilledParamReg);
        offset = callerStackTable->getSpillParamAddr(spilledParamReg);
        builder.createFlwInst(spilledParamReg, addrReg, 0);
      } else if (getOperandType(param) == 3) {
        auto spilledParamReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(getOperandName(param, function, entryDAG), function, true, true));
        callerStackTable->addSpillParam(spilledParamReg);
        offset = callerStackTable->getSpillParamAddr(spilledParamReg);
        builder.createLdInst(spilledParamReg, addrReg, 0);
      }
      lastAddr = offset;
      for (unsigned int i = 0; i < spilledParams.size() - 1; i++) {
        spilledParamsIter++;
        param = *spilledParamsIter;
        if (getOperandType(param) == 0) {
          auto spilledParamReg = dynamic_cast<riscv::IntSymRegister *>(
              getOrEmplaceSymRegister(getOperandName(param, function, entryDAG), function, true, false));
          callerStackTable->addSpillParam(spilledParamReg);
          offset = callerStackTable->getSpillParamAddr(spilledParamReg);
          builder.createAddiInst(addrReg, addrReg, offset - lastAddr);
          builder.createLwInst(spilledParamReg, addrReg, 0);
        } else if (getOperandType(param) == 1) {
          auto spilledParamReg = dynamic_cast<riscv::FloatSymRegister *>(
              getOrEmplaceSymRegister(getOperandName(param, function, entryDAG), function, false));
          callerStackTable->addSpillParam(spilledParamReg);
          offset = callerStackTable->getSpillParamAddr(spilledParamReg);
          builder.createAddiInst(addrReg, addrReg, offset - lastAddr);
          builder.createFlwInst(spilledParamReg, addrReg, 0);
        } else if (getOperandType(param) == 3) {
          auto spilledParamReg = dynamic_cast<riscv::IntSymRegister *>(
              getOrEmplaceSymRegister(getOperandName(param, function, entryDAG), function, true, true));
          callerStackTable->addSpillParam(spilledParamReg);
          offset = callerStackTable->getSpillParamAddr(spilledParamReg);
          builder.createAddiInst(addrReg, addrReg, offset - lastAddr);
          builder.createLdInst(spilledParamReg, addrReg, 0);
        }
        lastAddr = offset;
      }
    }
  }
  for (const auto &functionItem : pModule->getFunctions()) {
    generateFunction(functionItem.second.get());
  }
};
}  // namespace mid2EndDAG