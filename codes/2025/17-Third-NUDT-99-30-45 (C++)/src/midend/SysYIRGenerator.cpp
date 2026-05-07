// SysYIRGenerator.cpp
// TODO：类型转换及其检查
// TODO：sysy库函数处理
// TODO：数组处理
// TODO：对while、continue、break的测试
#include "IR.h"
#include <any>
#include <memory>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include "SysYIRGenerator.h"

using namespace std;
namespace sysy {

std::pair<long long, int> calculate_signed_magic(int d) {
    if (d == 0) throw std::runtime_error("Division by zero");
    if (d == 1 || d == -1) return {0, 0}; // Not used by strength reduction

    int k = 0;
    unsigned int ad = (d > 0) ? d : -d;
    unsigned int temp = ad;
    while (temp > 0) {
        temp >>= 1;
        k++;
    }
    if ((ad & (ad - 1)) == 0) { // if power of 2
        k--;
    }

    unsigned __int128 m_val = 1;
    m_val <<= (32 + k - 1);
    unsigned __int128 m_prime = m_val / ad;
    long long m = m_prime + 1;

    return {m, k};
}

// 清除因函数调用而失效的表达式缓存（保守策略）
void SysYIRGenerator::invalidateExpressionsOnCall() {
  availableBinaryExpressions.clear();
  availableUnaryExpressions.clear();
  availableLoads.clear();
  availableGEPs.clear();
}

// 在进入新的基本块时清空所有表达式缓存
void SysYIRGenerator::enterNewBasicBlock() {
  availableBinaryExpressions.clear();
  availableUnaryExpressions.clear();
  availableLoads.clear();
  availableGEPs.clear();
}

// 清除因变量赋值而失效的表达式缓存
// @param storedAddress: store 指令的目标地址 (例如 AllocaInst* 或 GEPInst*)
void SysYIRGenerator::invalidateExpressionsOnStore(Value *storedAddress) {
  // 遍历二元表达式缓存，移除受影响的条目
  // 创建一个临时列表来存储要移除的键，避免在迭代时修改容器
  std::vector<ExpKey> binaryKeysToRemove;
  for (const auto &pair : availableBinaryExpressions) {
    // 检查左操作数
    // 如果左操作数是 LoadInst，并且它从 storedAddress 加载
    if (auto loadInst = dynamic_cast<LoadInst *>(pair.first.left)) {
      if (loadInst->getPointer() == storedAddress) {
        binaryKeysToRemove.push_back(pair.first);
        continue; // 这个表达式已标记为移除，跳到下一个
      }
    }
    // 如果左操作数本身就是被存储的地址 (例如，将一个地址值直接作为操作数，虽然不常见)
    if (pair.first.left == storedAddress) {
      binaryKeysToRemove.push_back(pair.first);
      continue;
    }

    // 检查右操作数，逻辑同左操作数
    if (auto loadInst = dynamic_cast<LoadInst *>(pair.first.right)) {
      if (loadInst->getPointer() == storedAddress) {
        binaryKeysToRemove.push_back(pair.first);
        continue;
      }
    }
    if (pair.first.right == storedAddress) {
      binaryKeysToRemove.push_back(pair.first);
      continue;
    }
  }
  // 实际移除条目
  for (const auto &key : binaryKeysToRemove) {
    availableBinaryExpressions.erase(key);
  }

  // 遍历一元表达式缓存，移除受影响的条目
  std::vector<UnExpKey> unaryKeysToRemove;
  for (const auto &pair : availableUnaryExpressions) {
    // 检查操作数
    if (auto loadInst = dynamic_cast<LoadInst *>(pair.first.operand)) {
      if (loadInst->getPointer() == storedAddress) {
        unaryKeysToRemove.push_back(pair.first);
        continue;
      }
    }
    if (pair.first.operand == storedAddress) {
      unaryKeysToRemove.push_back(pair.first);
      continue;
    }
  }
  // 实际移除条目
  for (const auto &key : unaryKeysToRemove) {
    availableUnaryExpressions.erase(key);
  }
  availableLoads.erase(storedAddress);

  std::vector<GEPKey> gepKeysToRemove;
  for (const auto &pair : availableGEPs) {
    // 检查 GEP 的基指针是否受存储影响
    if (auto loadInst = dynamic_cast<LoadInst *>(pair.first.basePointer)) {
        if (loadInst->getPointer() == storedAddress) {
            gepKeysToRemove.push_back(pair.first);
            continue; // 标记此GEP为移除，跳过后续检查
        }
    }
    // 如果基指针本身就是存储的目标地址 (不常见，但可能)
    if (pair.first.basePointer == storedAddress) {
        gepKeysToRemove.push_back(pair.first);
        continue;
    }

    // 检查 GEP 的每个索引是否受存储影响
    for (const auto &indexVal : pair.first.indices) {
        if (auto loadInst = dynamic_cast<LoadInst *>(indexVal)) {
            if (loadInst->getPointer() == storedAddress) {
                gepKeysToRemove.push_back(pair.first);
                break; // 标记此GEP为移除，并跳出内部循环
            }
        }
        // 如果索引本身就是存储的目标地址
        if (indexVal == storedAddress) {
            gepKeysToRemove.push_back(pair.first);
            break;
        }
    }
  }
  // 实际移除条目
  for (const auto &key : gepKeysToRemove) {
    availableGEPs.erase(key);
  }
}

// std::vector<Value*> BinaryValueStack;  ///< 用于存储value的栈
// std::vector<int> BinaryOpStack;  ///< 用于存储二元表达式的操作符栈 
// // 约定操作符：
// // 1: 'ADD', 2: 'SUB', 3: 'MUL', 4: 'DIV', 5: '%', 6: 'PLUS', 7: 'NEG', 8: 'NOT'
// enum BinaryOp {
//   ADD = 1,
//   SUB = 2,
//   MUL = 3,
//   DIV = 4,
//   MOD = 5,
//   PLUS = 6,
//   NEG = 7,
//   NOT = 8
// };

Value *SysYIRGenerator::promoteType(Value *value, Type *targetType) {
  //如果是常量则直接返回相应的值
  if (targetType == nullptr) {
    return value; // 如果值为空，那就不需要转换
  }
  ConstantInteger* constInt = dynamic_cast<ConstantInteger *>(value);
  ConstantFloating *constFloat = dynamic_cast<ConstantFloating *>(value);
  if (constInt) {
    if (targetType->isFloat()) {
      return ConstantFloating::get(static_cast<float>(constInt->getInt()));
    }
    return constInt; // 如果目标类型是int，直接返回原值
  } else if (constFloat) {
    if (targetType->isInt()) {
      return ConstantInteger::get(static_cast<int>(constFloat->getFloat()));
    }
    return constFloat; // 如果目标类型是float，直接返回原值
  }

  if (value->getType()->isInt() && targetType->isFloat()) {
    return builder.createItoFInst(value);
  } else if (value->getType()->isFloat() && targetType->isInt()) {
    return builder.createFtoIInst(value);
  }
  // 如果类型已经匹配，直接返回原值
  return value;
}

bool SysYIRGenerator::isRightAssociative(int op) {
    return (op == BinaryOp::PLUS || op == BinaryOp::NEG || op == BinaryOp::NOT);
}

void SysYIRGenerator::compute() {

  // 先将中缀表达式转换为后缀表达式
  BinaryRPNStack.clear();
  BinaryOpStack.clear();
  
  int begin = BinaryExpStack.size() - BinaryExpLenStack.back(), end = BinaryExpStack.size();
  
  for (int i = begin; i < end; i++) {
    auto item = BinaryExpStack[i];
    if (std::holds_alternative<sysy::Value *>(item)) {
      // 如果是操作数 (Value*)，直接推入后缀表达式栈
      BinaryRPNStack.push_back(item); // 直接 push_back item (ValueOrOperator类型)
    } else {
      // 如果是操作符
      int currentOp = std::get<int>(item);

      if (currentOp == LPAREN) {
        // 左括号直接入栈
        BinaryOpStack.push_back(currentOp);
      } else if (currentOp == RPAREN) {
        // 右括号：将操作符栈中的操作符弹出并添加到后缀表达式栈，直到遇到左括号
        while (!BinaryOpStack.empty() && BinaryOpStack.back() != LPAREN) {
          BinaryRPNStack.push_back(BinaryOpStack.back()); // 直接 push_back int
          BinaryOpStack.pop_back();
        }
        if (!BinaryOpStack.empty() && BinaryOpStack.back() == LPAREN) {
          BinaryOpStack.pop_back(); // 弹出左括号，但不添加到后缀表达式栈
        } else {
          // 错误：不匹配的右括号
          std::cerr << "Error: Mismatched parentheses in expression." << std::endl;
          return;
        }
      } else {
        // 普通操作符
        while (!BinaryOpStack.empty() && BinaryOpStack.back() != LPAREN) {

          int stackTopOp = BinaryOpStack.back();
          // 如果当前操作符优先级低于栈顶操作符优先级
          // 或者 (当前操作符优先级等于栈顶操作符优先级 并且 栈顶操作符是左结合)
          if (getOperatorPrecedence(currentOp) < getOperatorPrecedence(stackTopOp) ||
              (getOperatorPrecedence(currentOp) == getOperatorPrecedence(stackTopOp) &&
               !isRightAssociative(stackTopOp))) {

            BinaryRPNStack.push_back(stackTopOp);
            BinaryOpStack.pop_back();
          } else {
            break; // 否则当前操作符入栈
          }
        }
        BinaryOpStack.push_back(currentOp); // 当前操作符入栈
      }
    }
  }
  // 遍历结束后，将操作符栈中剩余的所有操作符弹出并添加到后缀表达式栈
  while (!BinaryOpStack.empty()) {
    if (BinaryOpStack.back() == LPAREN) {
      // 错误：不匹配的左括号
      std::cerr << "Error: Mismatched parentheses in expression (unclosed parenthesis)." << std::endl;
      return;
    }
    BinaryRPNStack.push_back(BinaryOpStack.back()); // 直接 push_back int
    BinaryOpStack.pop_back();
  }
  
  // 弹出BinaryExpStack的表达式
  int count = end - begin;
  for (int i = 0; i < count; i++) {
    BinaryExpStack.pop_back();
  }
  if (!BinaryExpLenStack.empty()) {
    BinaryExpLenStack.back() -= count;
  }

  // 计算后缀表达式
  // 每次计算前清空操作数栈
  BinaryValueStack.clear();

  // 遍历后缀表达式栈
  Type *commonType = nullptr;
  for(const auto &item : BinaryRPNStack) {
    if (std::holds_alternative<Value *>(item)) {
      // 如果是操作数 (Value*) 检测他的类型
      Value *value = std::get<Value *>(item);
      if (commonType == nullptr) {
        commonType = value->getType();
      }
      else if (value->getType() != commonType && value->getType()->isFloat()) {
        // 如果当前值的类型与commonType不同且是float类型，则提升为float
        commonType = Type::getFloatType();
        break;
      }
    } else {
      continue;
    }
  }

  for (const auto &item : BinaryRPNStack) {
    if (std::holds_alternative<sysy::Value *>(item)) {
      // 如果是操作数 (Value*)，直接推入操作数栈
      BinaryValueStack.push_back(std::get<sysy::Value *>(item));
    } else {
      // 如果是操作符
      int op = std::get<int>(item);
      Value *resultValue = nullptr;
      Value *lhs = nullptr;
      Value *rhs = nullptr;
      Value *operand = nullptr;

      switch (op) {
      case BinaryOp::ADD:
      case BinaryOp::SUB:
      case BinaryOp::MUL:
      case BinaryOp::DIV:
      case BinaryOp::MOD: {
        // 二元操作符需要两个操作数
        if (BinaryValueStack.size() < 2) {
          std::cerr << "Error: Not enough operands for binary operation: " << op << std::endl;
          return; // 或者抛出异常
        }
        rhs = BinaryValueStack.back();
        BinaryValueStack.pop_back();
        lhs = BinaryValueStack.back();
        BinaryValueStack.pop_back();
        // 类型转换
        lhs = promoteType(lhs, commonType);
        rhs = promoteType(rhs, commonType);

        // 尝试常量折叠
        ConstantValue *lhsConst = dynamic_cast<ConstantValue *>(lhs);
        ConstantValue *rhsConst = dynamic_cast<ConstantValue *>(rhs);

        if (lhsConst && rhsConst) {
          // 如果都是常量，直接计算结果
          if (commonType == Type::getIntType()) {
            int lhsVal = lhsConst->getInt();
            int rhsVal = rhsConst->getInt();
            switch (op) {
            case BinaryOp::ADD: resultValue = ConstantInteger::get(lhsVal + rhsVal); break;
            case BinaryOp::SUB: resultValue = ConstantInteger::get(lhsVal - rhsVal); break;
            case BinaryOp::MUL: resultValue = ConstantInteger::get(lhsVal * rhsVal); break;
            case BinaryOp::DIV:
              if (rhsVal == 0) {
                std::cerr << "Error: Division by zero." << std::endl;
                return;
              }
              resultValue = sysy::ConstantInteger::get(lhsVal / rhsVal); break;
            case BinaryOp::MOD:
              if (rhsVal == 0) {
                std::cerr << "Error: Modulo by zero." << std::endl;
                return;
              }
              resultValue = sysy::ConstantInteger::get(lhsVal % rhsVal); break;
            default:
              std::cerr << "Error: Unknown binary operator for constants: " << op << std::endl;
              return;
            }
          } else if (commonType == Type::getFloatType()) {
            float lhsVal = lhsConst->getFloat();
            float rhsVal = rhsConst->getFloat();
            switch (op) {
            case BinaryOp::ADD: resultValue = ConstantFloating::get(lhsVal + rhsVal); break;
            case BinaryOp::SUB: resultValue = ConstantFloating::get(lhsVal - rhsVal); break;
            case BinaryOp::MUL: resultValue = ConstantFloating::get(lhsVal * rhsVal); break;
            case BinaryOp::DIV:
              if (rhsVal == 0.0f) {
                std::cerr << "Error: Division by zero." << std::endl;
                return;
              }
              resultValue = sysy::ConstantFloating::get(lhsVal / rhsVal); break;
            case BinaryOp::MOD:
              std::cerr << "Error: Modulo operator not supported for float types." << std::endl;
              return;
            default:
              std::cerr << "Error: Unknown binary operator for float constants: " << op << std::endl;
              return;
            }
          } else {
            std::cerr << "Error: Unsupported type for binary constant operation." << std::endl;
            return;
          }
        } else {
          // 否则，创建相应的IR指令
          ExpKey currentExpKey(static_cast<BinaryOp>(op), lhs, rhs);
          auto it = availableBinaryExpressions.find(currentExpKey);

          if (it != availableBinaryExpressions.end()) {
            // 在缓存中找到，重用结果
            resultValue = it->second;
          } else {
            if (commonType == Type::getIntType()) {
              switch (op) {
              case BinaryOp::ADD: resultValue = builder.createAddInst(lhs, rhs); break;
              case BinaryOp::SUB: resultValue = builder.createSubInst(lhs, rhs); break;
              case BinaryOp::MUL: resultValue = builder.createMulInst(lhs, rhs); break;
              case BinaryOp::DIV: resultValue = builder.createDivInst(lhs, rhs); break;
              case BinaryOp::MOD: resultValue = builder.createRemInst(lhs, rhs); break;
              }
            } else if (commonType == Type::getFloatType()) {
              switch (op) {
              case BinaryOp::ADD: resultValue = builder.createFAddInst(lhs, rhs); break;
              case BinaryOp::SUB: resultValue = builder.createFSubInst(lhs, rhs); break;
              case BinaryOp::MUL: resultValue = builder.createFMulInst(lhs, rhs); break;
              case BinaryOp::DIV: resultValue = builder.createFDivInst(lhs, rhs); break;
              case BinaryOp::MOD:
                std::cerr << "Error: Modulo operator not supported for float types." << std::endl;
                return;
              }
            } else {
              std::cerr << "Error: Unsupported type for binary instruction." << std::endl;
              return;
            }
            // 将新创建的指令结果添加到缓存
            availableBinaryExpressions[currentExpKey] = resultValue;
          }
        }
        break;
      }
      case BinaryOp::PLUS:
      case BinaryOp::NEG:
      case BinaryOp::NOT: {
        // 一元操作符需要一个操作数
        if (BinaryValueStack.empty()) {
          std::cerr << "Error: Not enough operands for unary operation: " << op << std::endl;
          return;
        }
        operand = BinaryValueStack.back();
        BinaryValueStack.pop_back();

        operand = promoteType(operand, commonType);

        // 尝试常量折叠
        ConstantInteger *constInt = dynamic_cast<ConstantInteger *>(operand);
        ConstantFloating *constFloat = dynamic_cast<ConstantFloating *>(operand);

        if (constInt || constFloat) {
          // 如果是常量，直接计算结果
          switch (op) {
          case BinaryOp::PLUS: resultValue = operand; break;
          case BinaryOp::NEG: {
            if (constInt) {
              resultValue = constInt->getNeg();
            } else if (constFloat) {
              resultValue = constFloat->getNeg();
            } else {
              std::cerr << "Error: Negation not supported for constant operand type." << std::endl;
              return;
            }
            break;
          }
          case BinaryOp::NOT:
            if (constInt) {
              resultValue = sysy::ConstantInteger::get(constInt->getInt() == 0 ? 1 : 0);
            } else if (constFloat) {
              resultValue = sysy::ConstantInteger::get(constFloat->getFloat() == 0.0f ? 1 : 0);
            } else {
              std::cerr << "Error: Logical NOT not supported for constant operand type." << std::endl;
              return;
            }
            break;
          default:
            std::cerr << "Error: Unknown unary operator for constants: " << op << std::endl;
            return;
          }
        } else {
          // 否则，创建相应的IR指令 (在这里应用CSE)
          UnExpKey currentUnExpKey(static_cast<BinaryOp>(op), operand);
          auto it = availableUnaryExpressions.find(currentUnExpKey);
          if (it != availableUnaryExpressions.end()) {
            // 在缓存中找到，重用结果
            resultValue = it->second;
          } else {
            switch (op) {
              case BinaryOp::PLUS:
                resultValue = operand; // 一元加指令通常直接返回操作数
                break;
              case BinaryOp::NEG: {
                if (commonType == sysy::Type::getIntType()) {
                  resultValue = builder.createNegInst(operand);
                } else if (commonType == sysy::Type::getFloatType()) {
                  resultValue = builder.createFNegInst(operand);
                } else {
                  std::cerr << "Error: Negation not supported for operand type." << std::endl;
                  return;
                }
                break;
              }
              case BinaryOp::NOT:
                // 逻辑非
                if (commonType == sysy::Type::getIntType()) {
                  resultValue = builder.createNotInst(operand);
                } else if (commonType == sysy::Type::getFloatType()) {
                  resultValue = builder.createFNotInst(operand);
                } else {
                  std::cerr << "Error: Logical NOT not supported for operand type." << std::endl;
                  return;
                }
                break;
              default:
                std::cerr << "Error: Unknown unary operator for instructions: " << op << std::endl;
                return;
            }
            // 将新创建的指令结果添加到缓存
            availableUnaryExpressions[currentUnExpKey] = resultValue;
          }
        }
        break;
      }
      default:
        std::cerr << "Error: Unknown operator " << op << " encountered in RPN stack." << std::endl;
        return;
      }

      // 将计算结果或指令结果推入操作数栈
      if (resultValue) {
        BinaryValueStack.push_back(resultValue);
      } else {
        std::cerr << "Error: Result value is null after processing operator " << op << "!" << std::endl;
        return;
      }

    }
  }

  // 后缀表达式处理完毕，操作数栈的栈顶就是最终结果
  if (BinaryValueStack.empty()) {
    std::cerr << "Error: No values left in BinaryValueStack after processing RPN." << std::endl;
    return;
  }
  if (BinaryValueStack.size() > 1) {
    std::cerr
        << "Warning: Multiple values left in BinaryValueStack after processing RPN. Expression might be malformed."
        << std::endl;
  }
  BinaryRPNStack.clear(); // 清空后缀表达式栈
  BinaryOpStack.clear(); // 清空操作符栈
  return;
}

Value* SysYIRGenerator::computeExp(SysYParser::ExpContext *ctx, Type* targetType){
  if (ctx->addExp() == nullptr) {
    assert(false && "ExpContext should have an addExp child!");
  }
  BinaryExpLenStack.push_back(0); // 进入新的层次时Push 0
  visitAddExp(ctx->addExp());

  // if(targetType == nullptr) {
  //   targetType = Type::getIntType(); // 默认目标类型为int
  // }

  compute();
  // 最后一个Value应该是最终结果

  Value* result = BinaryValueStack.back();
  BinaryValueStack.pop_back(); // 移除结果值

  result = promoteType(result, targetType); // 确保结果类型符合目标类型
  // 检查当前层次的操作符数量
  int ExpLen = BinaryExpLenStack.back();
  BinaryExpLenStack.pop_back(); // 离开层次时将该层次
  if (ExpLen > 0) {
    std::cerr << "Warning: There are still " << ExpLen << " binary val or op left unprocessed in this level!" << std::endl;
    return nullptr;
  }
  return result;
}

Value* SysYIRGenerator::computeAddExp(SysYParser::AddExpContext *ctx, Type* targetType){
  // 根据AddExpContext中的操作符和操作数计算加法表达式
  // 这里假设AddExpContext已经被正确填充
  if (ctx->mulExp().size() == 0) {
    assert(false && "AddExpContext should have a mulExp child!");
  }
  BinaryExpLenStack.push_back(0); // 进入新的层次时Push 0
  visitMulExp(ctx->mulExp(0));
  // BinaryValueStack.push_back(result);

  for (int i = 1; i < ctx->mulExp().size(); i++) {
    auto opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2*i-1]);
    int opType = opNode->getSymbol()->getType();
    switch(opType) {
      case SysYParser::ADD: BinaryExpStack.push_back(BinaryOp::ADD); BinaryExpLenStack.back()++; break;
      case SysYParser::SUB: BinaryExpStack.push_back(BinaryOp::SUB); BinaryExpLenStack.back()++; break;
      default: assert(false && "Unexpected operator in AddExp.");
    }
    // BinaryExpStack.push_back(opType);
    visitMulExp(ctx->mulExp(i));
    // BinaryValueStack.push_back(operand);
  }
  // if(targetType == nullptr) {
  //   targetType = Type::getIntType(); // 默认目标类型为int
  // }
  // 根据后缀表达式的逻辑计算
  compute();
  // 最后一个Value应该是最终结果

  Value* result = BinaryValueStack.back();
  BinaryValueStack.pop_back(); // 移除最后一个值，因为它已经被计算
  result = promoteType(result, targetType); // 确保结果类型符合目标类型

  int ExpLen = BinaryExpLenStack.back();
  BinaryExpLenStack.pop_back(); // 离开层次时将该层次
  if (ExpLen > 0) {
    std::cerr << "Warning: There are still " << ExpLen << " binary val or op left unprocessed in this level!" << std::endl;
    return nullptr;
  }
  return result;
}

Type* SysYIRGenerator::buildArrayType(Type* baseType, const std::vector<Value*>& dims){
  Type* currentType = baseType;
    // 从最内层维度开始构建 ArrayType
    // 例如对于 int arr[2][3]，先处理 [3]，再处理 [2]
    // 注意：SysY 的 dims 是从最外层到最内层，所以我们需要反向迭代
    // 或者调整逻辑，使得从内到外构建 ArrayType
    // 假设 dims 列表是 [dim1, dim2, dim3...] (例如 [2, 3] for int[2][3])
    // 我们需要从最内层维度开始向外构建 ArrayType
    for (int i = dims.size() - 1; i >= 0; --i) {
        // 维度大小必须是常量，否则无法构建 ArrayType
        ConstantInteger* constDim = dynamic_cast<ConstantInteger*>(dims[i]);
        if (constDim == nullptr) {
            // 如果维度不是常量，可能需要特殊处理，例如将其视为指针
            // 对于函数参数 int arr[] 这种，第一个维度可以为未知
            // 在这里，我们假设所有声明的数组维度都是常量
            assert(false && "Array dimension must be a constant integer!");
            return nullptr;
        }
        unsigned dimSize = constDim->getInt();
        currentType = Type::getArrayType(currentType, dimSize);
    }
    return currentType;
}

// @brief: 获取 GEP 指令的地址
// @param basePointer: GEP 的基指针，已经过适当的加载/处理，类型为 LLVM IR 中的指针类型。
//                     例如，对于局部数组，它是 AllocaInst；对于参数数组，它是 LoadInst 的结果。
// @param indices: 已经包含了所有必要的偏移索引 (包括可能的初始 0 索引，由 visitLValue 准备)。
// @return: 计算得到的地址值 (也是一个指针类型)
Value* SysYIRGenerator::getGEPAddressInst(Value* basePointer, const std::vector<Value*>& indices) {
    // 检查 basePointer 是否为指针类型
    assert(basePointer->getType()->isPointer() && "Base pointer must be a pointer type!");

    // `indices` 向量现在由调用方（如 visitLValue, visitVarDecl, visitAssignStmt）负责完整准备，
    // 包括是否需要添加初始的 `0` 索引。
    // 所以这里直接将其传递给 `builder.createGetElementPtrInst`。
    GEPKey key = {basePointer, indices};

    // 尝试从缓存中查找
    auto it = availableGEPs.find(key);
    if (it != availableGEPs.end()) {
        return it->second; // 缓存命中，返回已有的 GEPInst*
    }

    // 缓存未命中，创建新的 GEPInst
    Value* gepInst = builder.createGetElementPtrInst(basePointer, indices); // 假设 builder 提供了 createGEPInst 方法
    availableGEPs[key] = gepInst; // 将新的 GEPInst* 加入缓存

    return gepInst;
}

/*
 * @brief: visit compUnit
 * @details:
 *      compUnit: (globalDecl | funcDef)+;
 */
std::any SysYIRGenerator::visitCompUnit(SysYParser::CompUnitContext *ctx) {
  // create the IR module
  auto pModule = new Module();
  assert(pModule);
  module.reset(pModule);

  // SymbolTable::ModuleScope scope(symbols_table);

  Utils::initExternalFunction(pModule, &builder);

  pModule->enterNewScope();
  visitChildren(ctx);
  pModule->leaveScope();

  Utils::modify_timefuncname(pModule);
  return pModule;
}

std::any SysYIRGenerator::visitGlobalConstDecl(SysYParser::GlobalConstDeclContext *ctx){
  auto constDecl = ctx->constDecl();
  Type* type = std::any_cast<Type *>(visitBType(constDecl->bType()));
  for (const auto &constDef : constDecl->constDef()) {
    std::vector<Value *> dims = {};
    std::string name = constDef->Ident()->getText();
    auto constExps = constDef->constExp();
    if (!constExps.empty()) {
      for (const auto &constExp : constExps) {
        dims.push_back(std::any_cast<Value *>(visitConstExp(constExp)));
      }
    }

    ArrayValueTree* root = std::any_cast<ArrayValueTree *>(constDef->constInitVal()->accept(this));
    ValueCounter values;
    Utils::tree2Array(type, root, dims, dims.size(), values, &builder);
    delete root;
    // 创建全局常量变量，并更新符号表
    Type* variableType = type;
    if (!dims.empty()) { // 如果有维度，说明是数组
        variableType = buildArrayType(type, dims); // 构建完整的 ArrayType
    }
    module->createConstVar(name, Type::getPointerType(variableType), values);
  }
  return std::any();
}

std::any SysYIRGenerator::visitGlobalVarDecl(SysYParser::GlobalVarDeclContext *ctx) {
  auto varDecl = ctx->varDecl();
  Type* type = std::any_cast<Type *>(visitBType(varDecl->bType()));
  for (const auto &varDef : varDecl->varDef()) {
    std::vector<Value *> dims = {};
    std::string name = varDef->Ident()->getText();
    auto constExps = varDef->constExp();
    if (!constExps.empty()) {
      for (const auto &constExp : constExps) {
        dims.push_back(std::any_cast<Value *>(visitConstExp(constExp)));
      }
    }

    ValueCounter values = {};
    if (varDef->initVal() != nullptr) {
      ArrayValueTree* root = std::any_cast<ArrayValueTree *>(varDef->initVal()->accept(this));
      Utils::tree2Array(type, root, dims, dims.size(), values, &builder);
      delete root;
    }
    // 创建全局变量，并更新符号表
    Type* variableType = type;
    if (!dims.empty()) { // 如果有维度，说明是数组
        variableType = buildArrayType(type, dims); // 构建完整的 ArrayType
    }
    module->createGlobalValue(name, Type::getPointerType(variableType), values);
  }
  return std::any();
}

std::any SysYIRGenerator::visitConstDecl(SysYParser::ConstDeclContext *ctx) {
  Type *type = std::any_cast<Type *>(visitBType(ctx->bType()));
  for (const auto constDef : ctx->constDef()) {
    std::vector<Value *> dims = {};
    std::string name = constDef->Ident()->getText();
    auto constExps = constDef->constExp();
    if (!constExps.empty()) {
      for (const auto constExp : constExps) {
        dims.push_back(std::any_cast<Value *>(visitConstExp(constExp)));
      }
    }

    Type *variableType = type;
    if (!dims.empty()) {
      variableType = buildArrayType(type, dims); // 构建完整的 ArrayType
    }

    // 显式地为局部常量在栈上分配空间
    // alloca 的类型将是指针指向常量类型，例如 `int*` 或 `int[2][3]*`
    // 将alloca全部集中到entry中
    auto entry = builder.getBasicBlock()->getParent()->getEntryBlock();
    auto it = builder.getPosition();
    auto nowblk = builder.getBasicBlock();
    builder.setPosition(entry, entry->terminator());
    AllocaInst *alloca = builder.createAllocaInst(Type::getPointerType(variableType), name);
    builder.setPosition(nowblk, it);

    ArrayValueTree *root = std::any_cast<ArrayValueTree *>(constDef->constInitVal()->accept(this));
    ValueCounter values;
    Utils::tree2Array(type, root, dims, dims.size(), values, &builder);
    delete root;

    // 根据维度信息进行 store 初始化
    if (dims.empty()) { // 标量常量初始化
      // 局部常量必须有初始值，且通常是单个值
      if (!values.getValues().empty()) {
        builder.createStoreInst(values.getValue(0), alloca);
      } else {
        // 错误处理：局部标量常量缺少初始化值
        // 或者可以考虑默认初始化为0，但这通常不符合常量的语义
        assert(false && "Local scalar constant must have an initialization value!");
        return std::any(); // 直接返回，避免继续执行
      }
    } else { // 数组常量初始化
      const std::vector<sysy::Value *> &counterValues = values.getValues();
      const std::vector<unsigned> &counterNumbers = values.getNumbers();
      int numElements = 1;
      std::vector<int> dimSizes;
      for (Value *dimVal : dims) {
        if (ConstantInteger *constInt = dynamic_cast<ConstantInteger *>(dimVal)) {
          int dimSize = constInt->getInt();
          numElements *= dimSize;
          dimSizes.push_back(dimSize);
        }
        // TODO else 错误处理：数组维度必须是常量（对于静态分配）
        else {
          assert(false && "Array dimension must be a constant integer!");
          return std::any(); // 直接返回，避免继续执行
        }
      }
      unsigned int elementSizeInBytes = type->getSize();
      unsigned int totalSizeInBytes = numElements * elementSizeInBytes;

      // 检查是否所有初始化值都是零
      bool allValuesAreZero = false;
      if (counterValues.empty()) { // 如果没有提供初始化值，通常视为全零初始化
        allValuesAreZero = true;
      } else {
        allValuesAreZero = true;
        for (Value *val : counterValues) {
          if (ConstantInteger *constInt = dynamic_cast<ConstantInteger *>(val)) {
            if (constInt->getInt() != 0) {
              allValuesAreZero = false;
              break;
            }
          } else { // 如果不是常量整数，则不能确定是零
            allValuesAreZero = false;
            break;
          }
        }
      }

      if (allValuesAreZero) {
        builder.createMemsetInst(alloca, ConstantInteger::get(0), ConstantInteger::get(totalSizeInBytes),
                                 ConstantInteger::get(0));
      } else {
        int linearIndexOffset = 0; // 用于追踪当前处理的线性索引的偏移量
        for (int k = 0; k < counterValues.size(); ++k) {
          // 当前 Value 的值和重复次数
          Value *currentValue = counterValues[k];
          unsigned currentRepeatNum = counterNumbers[k];

          // 检查是否是0，并且重复次数足够大（例如 >16），才用 memset
          if (ConstantInteger *constInt = dynamic_cast<ConstantInteger *>(currentValue)) {
            if (constInt->getInt() == 0 && currentRepeatNum >= 16) { // 阈值可调整（如16、32等）
              // 计算 memset 的起始地址（基于当前线性偏移量）
              std::vector<Value *> memsetStartIndices;
              int tempLinearIndex = linearIndexOffset;

              // 将线性索引转换为多维索引
              for (int dimIdx = dimSizes.size() - 1; dimIdx >= 0; --dimIdx) {
                memsetStartIndices.insert(memsetStartIndices.begin(),
                                          ConstantInteger::get(static_cast<int>(tempLinearIndex % dimSizes[dimIdx])));
                tempLinearIndex /= dimSizes[dimIdx];
              }

              // 构造 GEP 计算 memset 的起始地址
              std::vector<Value *> gepIndicesForMemset;
              gepIndicesForMemset.push_back(ConstantInteger::get(0)); // 跳过 alloca 类型
              gepIndicesForMemset.insert(gepIndicesForMemset.end(), memsetStartIndices.begin(),
                                         memsetStartIndices.end());

              Value *memsetPtr = builder.createGetElementPtrInst(alloca, gepIndicesForMemset);

              // 计算 memset 的字节数 = 元素个数 × 元素大小
              Type *elementType = type;;
              uint64_t elementSize = elementType->getSize();
              Value *size = ConstantInteger::get(currentRepeatNum * elementSize);

              // 生成 memset 指令（假设你的 IRBuilder 有 createMemset 方法）
              builder.createMemsetInst(memsetPtr, ConstantInteger::get(0), size, ConstantInteger::get(0));

              // 跳过这些已处理的0
              linearIndexOffset += currentRepeatNum;
              continue; // 直接进入下一次循环
            }
          }

          for (unsigned i = 0; i < currentRepeatNum; ++i) {
            // 对于非零值，生成对应的 store 指令
            std::vector<Value *> currentIndices;
            int tempLinearIndex = linearIndexOffset + i; // 使用偏移量和当前重复次数内的索引

            // 将线性索引转换为多维索引
            for (int dimIdx = dimSizes.size() - 1; dimIdx >= 0; --dimIdx) {
              currentIndices.insert(currentIndices.begin(),
                                    ConstantInteger::get(static_cast<int>(tempLinearIndex % dimSizes[dimIdx])));
              tempLinearIndex /= dimSizes[dimIdx];
            }

            // 对于局部数组，alloca 本身就是 GEP 的基指针。
            // GEP 的第一个索引必须是 0，用于“步过”整个数组。
            std::vector<Value *> gepIndicesForInit;
            gepIndicesForInit.push_back(ConstantInteger::get(0));
            gepIndicesForInit.insert(gepIndicesForInit.end(), currentIndices.begin(), currentIndices.end());

            // 计算元素的地址
            Value *elementAddress = getGEPAddressInst(alloca, gepIndicesForInit);
            // 生成 store 指令
            builder.createStoreInst(currentValue, elementAddress);
          }
          // 更新线性索引偏移量，以便下一次迭代从正确的位置开始
          linearIndexOffset += currentRepeatNum;
        }
      }
    }

    // 更新符号表，将常量名称与 AllocaInst 关联起来
    module->addVariable(name, alloca);
  }
  return std::any();
}

std::any SysYIRGenerator::visitVarDecl(SysYParser::VarDeclContext *ctx) {
  Type* type = std::any_cast<Type *>(visitBType(ctx->bType()));
  for (const auto varDef : ctx->varDef()) {
    std::vector<Value *> dims = {};
    std::string name = varDef->Ident()->getText();
    auto constExps = varDef->constExp();
    if (!constExps.empty()) {
      for (const auto &constExp : constExps) {
        dims.push_back(std::any_cast<Value *>(visitConstExp(constExp)));
      }
    }

    Type* variableType = type;
    if (!dims.empty()) { // 如果有维度，说明是数组
        variableType = buildArrayType(type, dims); // 构建完整的 ArrayType
    }

    // 对于数组，alloca 的类型将是指针指向数组类型，例如 `int[2][3]*`
    // 对于标量，alloca 的类型将是指针指向标量类型，例如 `int*`
    auto entry = builder.getBasicBlock()->getParent()->getEntryBlock();
    auto it = builder.getPosition();
    auto nowblk = builder.getBasicBlock();
    builder.setPosition(entry, entry->terminator());
    AllocaInst *alloca = builder.createAllocaInst(Type::getPointerType(variableType), name);
    builder.setPosition(nowblk, it);

    if (varDef->initVal() != nullptr) {
      ValueCounter values;
      ArrayValueTree* root = std::any_cast<ArrayValueTree *>(varDef->initVal()->accept(this));
      Utils::tree2Array(type, root, dims, dims.size(), values, &builder);
      delete root;

      if (dims.empty()) { // 标量变量初始化
        builder.createStoreInst(values.getValue(0), alloca);
      } else { // 数组变量初始化
        const std::vector<sysy::Value *> &counterValues = values.getValues();
        const std::vector<unsigned> &counterNumbers = values.getNumbers();
        int numElements = 1;
        std::vector<int> dimSizes;
        for (Value *dimVal : dims) {
          if (ConstantInteger *constInt = dynamic_cast<ConstantInteger *>(dimVal)) {
            int dimSize = constInt->getInt();
            numElements *= dimSize;
            dimSizes.push_back(dimSize);
          }
          // TODO else 错误处理：数组维度必须是常量（对于静态分配）
        }
        unsigned int elementSizeInBytes = type->getSize();
        unsigned int totalSizeInBytes = numElements * elementSizeInBytes;

        bool allValuesAreZero = false;
        if (counterValues.empty()) {
          allValuesAreZero = true;
        }
        else {
          allValuesAreZero = true;
          for (Value *val : counterValues){
            if (ConstantInteger *constInt = dynamic_cast<ConstantInteger *>(val)){
              if (constInt->getInt() != 0){
                allValuesAreZero = false;
                break;
              }
            }
            else{
              allValuesAreZero = false;
              break;
            }
          }
        }

        if (allValuesAreZero) {
          builder.createMemsetInst(
              alloca,
              ConstantInteger::get(0),
              ConstantInteger::get(totalSizeInBytes),
              ConstantInteger::get(0));
        }
        else {

          int linearIndexOffset = 0; // 用于追踪当前处理的线性索引的偏移量
          for (int k = 0; k < counterValues.size(); ++k) {
            // 当前 Value 的值和重复次数
            Value *currentValue = counterValues[k];
            unsigned currentRepeatNum = counterNumbers[k];
            // 检查是否是0，并且重复次数足够大（例如 >16），才用 memset
            if (ConstantInteger *constInt = dynamic_cast<ConstantInteger *>(currentValue)) {
              if (constInt->getInt() == 0 && currentRepeatNum >= 16) { // 阈值可调整（如16、32等）
                // 计算 memset 的起始地址（基于当前线性偏移量）
                std::vector<Value *> memsetStartIndices;
                int tempLinearIndex = linearIndexOffset;

                // 将线性索引转换为多维索引
                for (int dimIdx = dimSizes.size() - 1; dimIdx >= 0; --dimIdx) {
                  memsetStartIndices.insert(memsetStartIndices.begin(),
                                            ConstantInteger::get(static_cast<int>(tempLinearIndex % dimSizes[dimIdx])));
                  tempLinearIndex /= dimSizes[dimIdx];
                }

                // 构造 GEP 计算 memset 的起始地址
                std::vector<Value *> gepIndicesForMemset;
                gepIndicesForMemset.push_back(ConstantInteger::get(0)); // 跳过 alloca 类型
                gepIndicesForMemset.insert(gepIndicesForMemset.end(), memsetStartIndices.begin(),
                                           memsetStartIndices.end());

                Value *memsetPtr = builder.createGetElementPtrInst(alloca, gepIndicesForMemset);

                // 计算 memset 的字节数 = 元素个数 × 元素大小
                Type *elementType = type;
                ;
                uint64_t elementSize = elementType->getSize();
                Value *size = ConstantInteger::get(currentRepeatNum * elementSize);

                // 生成 memset 指令（假设你的 IRBuilder 有 createMemset 方法）
                builder.createMemsetInst(memsetPtr, ConstantInteger::get(0), size, ConstantInteger::get(0));

                // 跳过这些已处理的0
                linearIndexOffset += currentRepeatNum;
                continue; // 直接进入下一次循环
              }
            }
            for (unsigned i = 0; i < currentRepeatNum; ++i) {
              std::vector<Value *> currentIndices;
              int tempLinearIndex = linearIndexOffset + i; // 使用偏移量和当前重复次数内的索引

              // 将线性索引转换为多维索引
              for (int dimIdx = dimSizes.size() - 1; dimIdx >= 0; --dimIdx) {
                currentIndices.insert(currentIndices.begin(),
                                      ConstantInteger::get(static_cast<int>(tempLinearIndex % dimSizes[dimIdx])));
                tempLinearIndex /= dimSizes[dimIdx];
              }

              // 对于局部数组，alloca 本身就是 GEP 的基指针。
              // GEP 的第一个索引必须是 0，用于“步过”整个数组。
              std::vector<Value *> gepIndicesForInit;
              gepIndicesForInit.push_back(ConstantInteger::get(0));
              gepIndicesForInit.insert(gepIndicesForInit.end(), currentIndices.begin(), currentIndices.end());

              // 计算元素的地址
              Value *elementAddress = getGEPAddressInst(alloca, gepIndicesForInit);
              // 生成 store 指令
              builder.createStoreInst(currentValue, elementAddress);
            }
            // 更新线性索引偏移量，以便下一次迭代从正确的位置开始
            linearIndexOffset += currentRepeatNum;
          }
        }
      }
    }
    else { // 如果没有显式初始化值，默认对数组进行零初始化
      if (!dims.empty()) { // 只有数组才需要默认的零初始化
        int numElements = 1;
        for (Value *dimVal : dims) {
          if (ConstantInteger *constInt = dynamic_cast<ConstantInteger *>(dimVal)) {
            numElements *= constInt->getInt();
          }
        }
        unsigned int elementSizeInBytes = type->getSize();
        unsigned int totalSizeInBytes = numElements * elementSizeInBytes;

        builder.createMemsetInst(
            alloca,
            ConstantInteger::get(0),
            ConstantInteger::get(totalSizeInBytes),
            ConstantInteger::get(0)
          );
      }
    }

    module->addVariable(name, alloca);
  }

  return std::any();
}

std::any SysYIRGenerator::visitBType(SysYParser::BTypeContext *ctx) {
  return ctx->INT() != nullptr ? Type::getIntType() : Type::getFloatType();
}

std::any SysYIRGenerator::visitScalarInitValue(SysYParser::ScalarInitValueContext *ctx) {
  // Value* value = std::any_cast<Value *>(visitExp(ctx->exp()));
  Value* value = computeExp(ctx->exp());
  ArrayValueTree* result = new ArrayValueTree();
  result->setValue(value);
  return result;
}

std::any SysYIRGenerator::visitArrayInitValue(SysYParser::ArrayInitValueContext *ctx) {
  std::vector<ArrayValueTree *> children;
  for (const auto &initVal : ctx->initVal()) 
    children.push_back(std::any_cast<ArrayValueTree *>(initVal->accept(this)));
  ArrayValueTree* result = new ArrayValueTree();
  result->addChildren(children);
  return result;
}

std::any SysYIRGenerator::visitConstScalarInitValue(SysYParser::ConstScalarInitValueContext *ctx) {
  Value* value = std::any_cast<Value *>(visitConstExp(ctx->constExp()));
  ArrayValueTree* result = new ArrayValueTree();
  result->setValue(value);
  return result;
}

std::any SysYIRGenerator::visitConstExp(SysYParser::ConstExpContext *ctx){
  return computeAddExp(ctx->addExp());
}

std::any  SysYIRGenerator::visitConstArrayInitValue(SysYParser::ConstArrayInitValueContext *ctx) {
  std::vector<ArrayValueTree *> children;
  for (const auto &constInitVal : ctx->constInitVal()) 
    children.push_back(std::any_cast<ArrayValueTree *>(constInitVal->accept(this)));
  ArrayValueTree* result = new ArrayValueTree();
  result->addChildren(children);
  return result;
}

std::any SysYIRGenerator::visitFuncType(SysYParser::FuncTypeContext *ctx) {
  if (ctx->INT() != nullptr) 
    return Type::getIntType();
  if (ctx->FLOAT() != nullptr) 
    return Type::getFloatType();
  return Type::getVoidType();
}

std::any SysYIRGenerator::visitFuncDef(SysYParser::FuncDefContext *ctx){
  // 更新作用域
  module->enterNewScope();
  // 清除CSE缓存
  enterNewBasicBlock();

  auto name = ctx->Ident()->getText();
  std::vector<Type *> paramActualTypes;
  std::vector<std::string> paramNames;
  std::vector<std::vector<Value *>> paramDims;

  if (ctx->funcFParams() != nullptr) {
    auto params = ctx->funcFParams()->funcFParam();
    for (const auto &param : params) {
      Type* baseBType = std::any_cast<Type *>(visitBType(param->bType()));
      std::string paramName = param->Ident()->getText();
      
      // 用于收集当前参数的维度信息（如果它是数组）
      std::vector<Value *> currentParamDims; 
      if (!param->LBRACK().empty()) { // 如果参数声明中有方括号，说明是数组
        // SysY 数组参数的第一个维度可以是未知的（例如 int arr[] 或 int arr[][10]）
        // 这里的 ConstantInteger::get(-1) 表示未知维度，但对于 LLVM 类型构建，我们主要关注已知维度
        currentParamDims.push_back(ConstantInteger::get(-1)); // 标记第一个维度为未知
        for (const auto &exp : param->exp()) {
          // 访问表达式以获取维度大小，这些维度必须是常量
          Value* dimVal = computeExp(exp);
          // 确保维度是常量整数，否则 buildArrayType 会断言失败
          assert(dynamic_cast<ConstantInteger*>(dimVal) && "Array dimension in parameter must be a constant integer!");
          currentParamDims.push_back(dimVal);
        }
      }

      // 根据解析出的信息，确定参数在 LLVM IR 中的实际类型
      Type* actualParamType;
      if (currentParamDims.empty()) { // 情况1：标量参数 (e.g., int x)
        actualParamType = baseBType; // 实际类型就是基本类型
      } else { // 情况2&3：数组参数 (e.g., int arr[] 或 int arr[][10])
        // 数组参数在函数传递时会退化为指针。
        // 这个指针指向的类型是除第一维外，由后续维度构成的数组类型。
        
        // 从 currentParamDims 中移除第一个标记未知维度的 -1
        std::vector<Value*> fixedDimsForTypeBuilding;
        if (currentParamDims.size() > 1) { // 如果有固定维度 (e.g., int arr[][10])
            // 复制除第一个 -1 之外的所有维度
            fixedDimsForTypeBuilding.assign(currentParamDims.begin() + 1, currentParamDims.end());
        }
        
        Type* pointedToArrayType = baseBType; // 从基本类型开始构建
        // 从最内层维度向外层构建数组类型
        // buildArrayType 期望 dims 是从最外层到最内层，但它内部反向迭代，所以这里直接传入
        // 例如，对于 int arr[][10]，fixedDimsForTypeBuilding 包含 [10]，构建出 [10 x i32]
        if (!fixedDimsForTypeBuilding.empty()) {
          pointedToArrayType = buildArrayType(baseBType, fixedDimsForTypeBuilding);
        }
        
        // 实际参数类型是指向这个构建好的数组类型的指针
        actualParamType = Type::getPointerType(pointedToArrayType); // e.g., i32* 或 [10 x i32]*
      }
      
      paramActualTypes.push_back(actualParamType); // 存储参数的实际 LLVM IR 类型
      paramNames.push_back(paramName); // 存储参数名称
    
    }
  }

  Type* returnType = std::any_cast<Type *>(visitFuncType(ctx->funcType()));
  Type* funcType = Type::getFunctionType(returnType, paramActualTypes);
  Function* function = module->createFunction(name, funcType);
  BasicBlock* entry = function->getEntryBlock();
  builder.setPosition(entry, entry->end());

  for(int i = 0; i < paramActualTypes.size(); ++i) {
    Argument* arg = new Argument(paramActualTypes[i], function, i, paramNames[i]);
    function->insertArgument(arg);
  }

  // 先将所有参数名字注册到符号表中，确保alloca不会使用相同的名字
  for (int i = 0; i < paramNames.size(); ++i) {
    // 预先注册参数名字，这样addVariable就会使用不同的后缀
    module->registerParameterName(paramNames[i]);
  }

  auto funcArgs = function->getArguments();
  std::vector<AllocaInst *> allocas;
  for (int i = 0; i < paramActualTypes.size(); ++i) {
    // 使用函数特定的前缀来确保参数alloca名字唯一
    std::string allocaName = name + "_param_" + paramNames[i];
    AllocaInst *alloca = builder.createAllocaInst(Type::getPointerType(paramActualTypes[i]), allocaName);
    // 直接设置唯一名字，不依赖addVariable的命名逻辑
    alloca->setName(allocaName);
    allocas.push_back(alloca);
    // 直接添加到符号表，使用原参数名作为查找键
    module->addVariableDirectly(paramNames[i], alloca);
  }

  for(int i = 0; i < paramActualTypes.size(); ++i) {
    Value *argValue = funcArgs[i];
    builder.createStoreInst(argValue, allocas[i]);
  }

  // 在处理函数体之前，创建一个新的基本块作为函数体的实际入口
  // 这样 entryBB 就可以在完成初始化后跳转到这里
  BasicBlock* funcBodyEntry = function->addBasicBlock("funcBodyEntry_" + name);
  
  // 从 entryBB 无条件跳转到 funcBodyEntry
  builder.createUncondBrInst(funcBodyEntry);
  BasicBlock::conectBlocks(entry, funcBodyEntry); // 连接 entryBB 和 funcBodyEntry
  builder.setPosition(funcBodyEntry,funcBodyEntry->end()); // 将插入点设置到 funcBodyEntry

  for (auto item : ctx->blockStmt()->blockItem()) {
    visitBlockItem(item);
  }

  // 如果函数没有显式的返回语句，且返回类型不是 void，则需要添加一个默认的返回值
  ReturnInst* retinst = nullptr;
  retinst = dynamic_cast<ReturnInst*>(builder.getBasicBlock()->terminator()->get());

  if (!retinst) {
    if (returnType->isVoid()) {
        builder.createReturnInst();
    } else if (returnType->isInt()) {
        builder.createReturnInst(ConstantInteger::get(0)); // 默认返回 0
    } else if (returnType->isFloat()) {
        builder.createReturnInst(ConstantFloating::get(0.0f)); // 默认返回 0.0f
    } else {
        assert(false && "Function with no explicit return and non-void type should return a value.");
    }
  }

  module->leaveScope();

  return std::any();
}

std::any SysYIRGenerator::visitBlockStmt(SysYParser::BlockStmtContext *ctx) {
  module->enterNewScope();
  for (auto item : ctx->blockItem()) 
    visitBlockItem(item);
  module->leaveScope();
  return 0;
}

std::any SysYIRGenerator::visitAssignStmt(SysYParser::AssignStmtContext *ctx) {
  auto lVal = ctx->lValue();
  std::string name = lVal->Ident()->getText();
  Value* LValue = nullptr;
  Value* variable = module->getVariable(name); // 左值

  vector<Value *> indices;
  if (lVal->exp().size() > 0) {
    // 如果有下标，访问表达式获取下标值
    for (auto &exp : lVal->exp()) {
      Value* indexValue = std::any_cast<Value *>(computeExp(exp));
      indices.push_back(indexValue);
    }
  }
  if (indices.empty()) {
    // variable 本身就是指向标量的指针 (e.g., int* %a)
    if (dynamic_cast<AllocaInst*>(variable) || dynamic_cast<GlobalValue*>(variable)) {
        LValue = variable;
    }
    
    // 标量变量的类型推断
    Type* LType = builder.getIndexedType(variable->getType(), indices);
    
    Value* RValue = computeExp(ctx->exp(), LType); // 右值计算
    Type* RType = RValue->getType();

    // TODO:computeExp处理了类型转换，可以考虑删除判断逻辑
    if (LType != RType) {
      ConstantValue *constValue = dynamic_cast<ConstantValue *>(RValue);
      if (constValue != nullptr) {
        if (LType == Type::getFloatType()) {
          if(dynamic_cast<ConstantInteger *>(constValue)) {
            // 如果是整型常量，转换为浮点型
            RValue = ConstantFloating::get(static_cast<float>(constValue->getInt()));
          } else if (dynamic_cast<ConstantFloating *>(constValue)) {
            // 如果是浮点型常量，直接使用
            RValue = ConstantFloating::get(static_cast<float>(constValue->getFloat()));
          }
        } else { // 假设如果不是浮点型，就是整型
          if(dynamic_cast<ConstantFloating *>(constValue)) {
            // 如果是浮点型常量，转换为整型
            RValue = ConstantInteger::get(static_cast<int>(constValue->getFloat()));
          } else if (dynamic_cast<ConstantInteger *>(constValue)) {
            // 如果是整型常量，直接使用
            RValue = ConstantInteger::get(static_cast<int>(constValue->getInt()));
          }
        }
      } else {
        if (LType == Type::getFloatType() && RType != Type::getFloatType()) {
          RValue = builder.createItoFInst(RValue);
        } else if (LType != Type::getFloatType() && RType == Type::getFloatType()) {
          RValue = builder.createFtoIInst(RValue);
        }
        // 如果两者都是同一类型，就不需要转换
      }
    }
    
    builder.createStoreInst(RValue, LValue);
  }
  else {
    // 对于数组或多维数组的左值处理
    // 需要获取 GEP 地址
    Value* gepBasePointer = nullptr;
    std::vector<Value*> gepIndices; 
    if (AllocaInst *alloc = dynamic_cast<AllocaInst *>(variable)) {
      Type* allocatedType = alloc->getType()->as<PointerType>()->getBaseType(); 
      if (allocatedType->isPointer()) {
        // 尝试从缓存中获取 builder.createLoadInst(alloc) 的结果
        auto it = availableLoads.find(alloc);
        if (it != availableLoads.end()) {
            gepBasePointer = it->second; // 缓存命中，重用
        } else {
            gepBasePointer = builder.createLoadInst(alloc); // 缓存未命中，创建新的 LoadInst
            availableLoads[alloc] = gepBasePointer; // 将结果加入缓存
        }
        // --- CSE 结束 ---
        // gepBasePointer = builder.createLoadInst(alloc);
        gepIndices = indices; 
      } else {
        gepBasePointer = alloc;
        gepIndices.push_back(ConstantInteger::get(0));
        gepIndices.insert(gepIndices.end(), indices.begin(), indices.end());
      }
    } else if (GlobalValue *glob = dynamic_cast<GlobalValue *>(variable)) {
      // 情况 B: 全局变量 (GlobalValue)
      gepBasePointer = glob; 
      gepIndices.push_back(ConstantInteger::get(0));
      gepIndices.insert(gepIndices.end(), indices.begin(), indices.end());
    } else if (ConstantVariable *constV = dynamic_cast<ConstantVariable *>(variable)) {
      gepBasePointer = constV;
      gepIndices.push_back(ConstantInteger::get(0));
      gepIndices.insert(gepIndices.end(), indices.begin(), indices.end());
    } 
    // 左值为地址
    LValue = getGEPAddressInst(gepBasePointer, gepIndices);
    
    // 数组变量的类型推断，使用gepIndices和gepBasePointer的类型
    Type* LType = builder.getIndexedType(gepBasePointer->getType(), gepIndices);
    
    Value* RValue = computeExp(ctx->exp(), LType); // 右值计算
    Type* RType = RValue->getType();

    // TODO:computeExp处理了类型转换，可以考虑删除判断逻辑
    if (LType != RType) {
      ConstantValue *constValue = dynamic_cast<ConstantValue *>(RValue);
      if (constValue != nullptr) {
        if (LType == Type::getFloatType()) {
          if(dynamic_cast<ConstantInteger *>(constValue)) {
            // 如果是整型常量，转换为浮点型
            RValue = ConstantFloating::get(static_cast<float>(constValue->getInt()));
          } else if (dynamic_cast<ConstantFloating *>(constValue)) {
            // 如果是浮点型常量，直接使用
            RValue = ConstantFloating::get(static_cast<float>(constValue->getFloat()));
          }
        } else { // 假设如果不是浮点型，就是整型
          if(dynamic_cast<ConstantFloating *>(constValue)) {
            // 如果是浮点型常量，转换为整型
            RValue = ConstantInteger::get(static_cast<int>(constValue->getFloat()));
          } else if (dynamic_cast<ConstantInteger *>(constValue)) {
            // 如果是整型常量，直接使用
            RValue = ConstantInteger::get(static_cast<int>(constValue->getInt()));
          }
        }
      } else {
        if (LType == Type::getFloatType() && RType != Type::getFloatType()) {
          RValue = builder.createItoFInst(RValue);
        } else if (LType != Type::getFloatType() && RType == Type::getFloatType()) {
          RValue = builder.createFtoIInst(RValue);
        }
        // 如果两者都是同一类型，就不需要转换
      }
    }
    
    builder.createStoreInst(RValue, LValue);
  }

  invalidateExpressionsOnStore(LValue);
  return std::any();
}


std::any SysYIRGenerator::visitExpStmt(SysYParser::ExpStmtContext *ctx) {
  // 访问表达式
  if (ctx->exp() != nullptr) {
    computeExp(ctx->exp());
  }
  return std::any();
}

std::any SysYIRGenerator::visitIfStmt(SysYParser::IfStmtContext *ctx) {
  // labels string stream

  std::stringstream labelstring;
  Function * function = builder.getBasicBlock()->getParent();

  BasicBlock* thenBlock = new BasicBlock(function);
  BasicBlock* exitBlock = new BasicBlock(function);

  if (ctx->stmt().size() > 1) {
    BasicBlock* elseBlock = new BasicBlock(function);

    builder.pushTrueBlock(thenBlock);
    builder.pushFalseBlock(elseBlock);
    // 访问条件表达式
    visitCond(ctx->cond());
    builder.popTrueBlock();
    builder.popFalseBlock();

    labelstring << "if_then.L" << builder.getLabelIndex();
    thenBlock->setName(labelstring.str());
    labelstring.str("");
    function->addBasicBlock(thenBlock);
    builder.setPosition(thenBlock, thenBlock->end());
    // CSE清除缓存
    enterNewBasicBlock();
    
    auto block = dynamic_cast<SysYParser::BlockStmtContext *>(ctx->stmt(0));
    // 如果是块语句，直接访问
    // 否则访问语句
    if (block != nullptr) {
      visitBlockStmt(block);
    } else {
      module->enterNewScope();
      ctx->stmt(0)->accept(this);
      module->leaveScope();
    }
    builder.createUncondBrInst(exitBlock);
    BasicBlock::conectBlocks(builder.getBasicBlock(), exitBlock);

    labelstring << "if_else.L" << builder.getLabelIndex();
    elseBlock->setName(labelstring.str());
    labelstring.str("");
    function->addBasicBlock(elseBlock);
    builder.setPosition(elseBlock, elseBlock->end());
    // CSE清除缓存
    enterNewBasicBlock();
    
    block = dynamic_cast<SysYParser::BlockStmtContext *>(ctx->stmt(1));
    if (block != nullptr) {
      visitBlockStmt(block);
    } else {
      module->enterNewScope();
      ctx->stmt(1)->accept(this);
      module->leaveScope();
    }
    builder.createUncondBrInst(exitBlock);
    BasicBlock::conectBlocks(builder.getBasicBlock(), exitBlock);

    labelstring << "if_exit.L" << builder.getLabelIndex();
    exitBlock->setName(labelstring.str());
    labelstring.str("");
    function->addBasicBlock(exitBlock);
    builder.setPosition(exitBlock, exitBlock->end());
    // CSE清除缓存
    enterNewBasicBlock();
    
  } else {
    builder.pushTrueBlock(thenBlock);
    builder.pushFalseBlock(exitBlock);
    visitCond(ctx->cond());
    builder.popTrueBlock();
    builder.popFalseBlock();

    labelstring << "if_then.L" << builder.getLabelIndex();
    thenBlock->setName(labelstring.str());
    labelstring.str("");
    function->addBasicBlock(thenBlock);
    builder.setPosition(thenBlock, thenBlock->end());
    // CSE清除缓存
    enterNewBasicBlock();
    
    auto block = dynamic_cast<SysYParser::BlockStmtContext *>(ctx->stmt(0));
    if (block != nullptr) {
      visitBlockStmt(block);
    } else {
      module->enterNewScope();
      ctx->stmt(0)->accept(this);
      module->leaveScope();
    }
    builder.createUncondBrInst(exitBlock);
    BasicBlock::conectBlocks(builder.getBasicBlock(), exitBlock);

    labelstring << "if_exit.L" << builder.getLabelIndex();
    exitBlock->setName(labelstring.str());
    labelstring.str("");
    function->addBasicBlock(exitBlock);
    builder.setPosition(exitBlock, exitBlock->end());
    // CSE清除缓存
    enterNewBasicBlock();
    
  }
  return std::any();
}

std::any SysYIRGenerator::visitWhileStmt(SysYParser::WhileStmtContext *ctx) {
  // while structure:
  // curblock -> headBlock -> bodyBlock -> exitBlock
  BasicBlock* curBlock = builder.getBasicBlock();
  Function* function = builder.getBasicBlock()->getParent();

  std::stringstream labelstring;
  labelstring << "while_head.L" << builder.getLabelIndex();
  BasicBlock *headBlock = function->addBasicBlock(labelstring.str());
  labelstring.str("");
  builder.createUncondBrInst(headBlock);
  BasicBlock::conectBlocks(curBlock, headBlock);
  builder.setPosition(headBlock, headBlock->end());
  // CSE清除缓存
  enterNewBasicBlock();
    
  BasicBlock* bodyBlock = new BasicBlock(function);
  BasicBlock* exitBlock = new BasicBlock(function);

  builder.pushTrueBlock(bodyBlock);
  builder.pushFalseBlock(exitBlock);
  // 访问条件表达式
  visitCond(ctx->cond());
  builder.popTrueBlock();
  builder.popFalseBlock();

  labelstring << "while_body.L" << builder.getLabelIndex();
  bodyBlock->setName(labelstring.str());
  labelstring.str("");
  function->addBasicBlock(bodyBlock);
  builder.setPosition(bodyBlock, bodyBlock->end());
  // CSE清除缓存
  enterNewBasicBlock();

  builder.pushBreakBlock(exitBlock);
  builder.pushContinueBlock(headBlock);
  
  auto block = dynamic_cast<SysYParser::BlockStmtContext *>(ctx->stmt());

  if( block != nullptr) {
    visitBlockStmt(block);
  } else {
    module->enterNewScope();
    ctx->stmt()->accept(this);
    module->leaveScope();
  }

  builder.createUncondBrInst(headBlock);
  BasicBlock::conectBlocks(builder.getBasicBlock(), headBlock);
  builder.popBreakBlock();
  builder.popContinueBlock();

  labelstring << "while_exit.L" << builder.getLabelIndex();
  exitBlock->setName(labelstring.str());
  labelstring.str("");
  function->addBasicBlock(exitBlock);
  builder.setPosition(exitBlock, exitBlock->end());
    // CSE清除缓存
  enterNewBasicBlock();

  return std::any();
}

std::any SysYIRGenerator::visitBreakStmt(SysYParser::BreakStmtContext *ctx) {
  BasicBlock* breakBlock = builder.getBreakBlock();
  builder.createUncondBrInst(breakBlock);
  BasicBlock::conectBlocks(builder.getBasicBlock(), breakBlock);
  return std::any();
}

std::any SysYIRGenerator::visitContinueStmt(SysYParser::ContinueStmtContext *ctx) {
  BasicBlock* continueBlock = builder.getContinueBlock();
  builder.createUncondBrInst(continueBlock);
  BasicBlock::conectBlocks(builder.getBasicBlock(), continueBlock);
  return std::any();
}

std::any SysYIRGenerator::visitReturnStmt(SysYParser::ReturnStmtContext *ctx) {
  Value* returnValue = nullptr;
  Type* funcType = builder.getBasicBlock()->getParent()->getReturnType();
  if (ctx->exp() != nullptr) {
    returnValue = computeExp(ctx->exp(), funcType);
  }
  // TODOL 考虑删除类型转换判断逻辑
  if (returnValue != nullptr && funcType!= returnValue->getType()) {
    ConstantValue * constValue = dynamic_cast<ConstantValue *>(returnValue);
    if (constValue != nullptr) {
      if (funcType == Type::getFloatType()) {
        if(dynamic_cast<ConstantInteger *>(constValue)) {
          // 如果是整型常量，转换为浮点型
          returnValue = ConstantFloating::get(static_cast<float>(constValue->getInt()));
        } else if (dynamic_cast<ConstantFloating *>(constValue)) {
          // 如果是浮点型常量，直接使用
          returnValue = ConstantFloating::get(static_cast<float>(constValue->getInt()));
        }
      } else {
        if(dynamic_cast<ConstantFloating *>(constValue)) {
          // 如果是浮点型常量，转换为整型
          returnValue = ConstantInteger::get(static_cast<int>(constValue->getFloat()));
        } else if (dynamic_cast<ConstantInteger *>(constValue)) {
          // 如果是整型常量，直接使用
          returnValue = ConstantInteger::get(static_cast<int>(constValue->getFloat()));
        }
      }
    } else {
      if (funcType == Type::getFloatType()) {
        returnValue = builder.createItoFInst(returnValue);
      } else {
        returnValue = builder.createFtoIInst(returnValue);
      }
    }
  }
  builder.createReturnInst(returnValue);

  return std::any();
}


// 辅助函数：计算给定类型中嵌套的数组维度数量
// 例如：
// - 对于 i32* 类型，它指向 i32，维度为 0。
// - 对于 [10 x i32]* 类型，它指向 [10 x i32]，维度为 1。
// - 对于 [20 x [10 x i32]]* 类型，它指向 [20 x [10 x i32]]，维度为 2。
unsigned SysYIRGenerator::countArrayDimensions(Type* type) {
    unsigned dims = 0;
    Type* currentType = type;

    // 如果是指针类型，先获取它指向的基础类型
    if (currentType->isPointer()) {
        currentType = currentType->as<PointerType>()->getBaseType();
    }

    // 递归地计算数组的维度层数
    while (currentType && currentType->isArray()) {
        dims++;
        currentType = currentType->as<ArrayType>()->getElementType();
    }
    return dims;
}

std::any SysYIRGenerator::visitLValue(SysYParser::LValueContext *ctx) {
  std::string name = ctx->Ident()->getText();
  Value* variable = module->getVariable(name);

  Value* value = nullptr;

  std::vector<Value *> dims;
  for (const auto &exp : ctx->exp()) {
    Value* expValue = std::any_cast<Value *>(computeExp(exp));
    dims.push_back(expValue);
  }

  // 1. 获取变量的声明维度数量
  unsigned declaredNumDims = countArrayDimensions(variable->getType());

  // 2. 处理常量变量 (ConstantVariable) 且所有索引都是常量的情况
  ConstantVariable* constVar = dynamic_cast<ConstantVariable *>(variable);
  if (constVar != nullptr) {
    bool allIndicesConstant = true;
    for (const auto &dim : dims) {
      if (dynamic_cast<ConstantValue *>(dim) == nullptr) {
        allIndicesConstant = false;
        break;
      }
    }
    // 如果是常量变量且所有索引都是常量，并且不是数组名单独出现的情况
    if (allIndicesConstant && !dims.empty()) {
      // 如果是常量变量且所有索引都是常量，直接通过 getByIndices 获取编译时值
      // 这个方法会根据索引深度返回最终的标量值或指向子数组的指针 (作为 ConstantValue/Variable)
      return constVar->getByIndices(dims);
    }
    // 如果dims为空，检查是否是常量标量
    if (dims.empty() && declaredNumDims == 0) {
      // 常量标量，直接返回其值
      // 默认传入空索引列表，表示访问标量本身
      return constVar->getByIndices(dims);
    }
    // 如果dims为空但不是标量（数组名单独出现），需要走GEP路径来实现数组到指针的退化
  }

  // 3. 处理可变变量 (AllocaInst/GlobalValue) 或带非常量索引的常量变量
  // 这里区分标量访问和数组元素/子数组访问
  Value *targetAddress = nullptr;
  // 检查是否是访问标量变量本身（没有索引，且声明维度为0）
  if (dims.empty() && declaredNumDims == 0) {
    if (dynamic_cast<AllocaInst*>(variable) || dynamic_cast<GlobalValue*>(variable)) {
        targetAddress = variable;
    } 
    else {
        assert(false && "Unhandled scalar variable type in LValue access.");
        return static_cast<Value*>(nullptr);
    }
  } else {
    Value* gepBasePointer = nullptr;
    std::vector<Value*> gepIndices;
    if (AllocaInst *alloc = dynamic_cast<AllocaInst *>(variable)) {
      Type* allocatedType = alloc->getType()->as<PointerType>()->getBaseType(); 
      if (allocatedType->isPointer()) {
        gepBasePointer = builder.createLoadInst(alloc);
        gepIndices = dims; 
      } else {
        gepBasePointer = alloc;
        gepIndices.push_back(ConstantInteger::get(0));
        if (dims.empty() && declaredNumDims > 0) {
          // 数组名单独出现（没有索引）：在SysY中，多维数组名应该退化为指向第一行的指针
          // 对于二维数组 T[M][N]，退化为 T(*)[N]，需要GEP: getelementptr T[M][N], T[M][N]* ptr, i32 0, i32 0
          // 第一个i32 0: 选择数组本身，第二个i32 0: 选择第0行
          // 结果类型: T[N]*
          gepIndices.push_back(ConstantInteger::get(0));
        } else {
          // 正常的数组元素访问
          gepIndices.insert(gepIndices.end(), dims.begin(), dims.end());
        }
      }
    } else if (GlobalValue *glob = dynamic_cast<GlobalValue *>(variable)) {
      gepBasePointer = glob;
      gepIndices.push_back(ConstantInteger::get(0));
      if (dims.empty() && declaredNumDims > 0) {
        // 全局数组名单独出现（没有索引）：应该退化为指向第一行的指针
        // 需要添加一个额外的i32 0索引
        gepIndices.push_back(ConstantInteger::get(0));
      } else {
        // 正常的数组元素访问
        gepIndices.insert(gepIndices.end(), dims.begin(), dims.end());
      }
    } else if (ConstantVariable *constV = dynamic_cast<ConstantVariable *>(variable)) {
      gepBasePointer = constV;
      gepIndices.push_back(ConstantInteger::get(0));
      if (dims.empty() && declaredNumDims > 0) {
        // 常量数组名单独出现（没有索引）：应该退化为指向第一行的指针
        // 需要添加一个额外的i32 0索引
        gepIndices.push_back(ConstantInteger::get(0));
      } else {
        // 正常的数组元素访问
        gepIndices.insert(gepIndices.end(), dims.begin(), dims.end());
      }
    } else {
      assert(false && "LValue variable type not supported for GEP base pointer.");
      return static_cast<Value *>(nullptr);
    }

    targetAddress = getGEPAddressInst(gepBasePointer, gepIndices);

  }

  // 如果提供的索引数量少于声明的维度数量，则表示访问的是子数组，返回其地址 (无需加载)
  if (dims.size() < declaredNumDims) {
      value = targetAddress;
  } else {
    // value = builder.createLoadInst(targetAddress);
    auto it = availableLoads.find(targetAddress);
    if (it != availableLoads.end()) {
      value = it->second; // 缓存命中，重用已有的 LoadInst 结果
    } else {
      // 缓存未命中，创建新的 LoadInst
      value = builder.createLoadInst(targetAddress);
      availableLoads[targetAddress] = value; // 将新的 LoadInst 结果加入缓存
    }
  }

  return value;
}

std::any SysYIRGenerator::visitPrimaryExp(SysYParser::PrimaryExpContext *ctx) {
  if (ctx->exp() != nullptr) {
    BinaryExpStack.push_back(BinaryOp::LPAREN);BinaryExpLenStack.back()++;
    visitExp(ctx->exp());
    BinaryExpStack.push_back(BinaryOp::RPAREN);BinaryExpLenStack.back()++;
  }
    
  if (ctx->lValue() != nullptr) {
    // 如果是 lValue，将value压入栈中
    BinaryExpStack.push_back(std::any_cast<Value *>(visitLValue(ctx->lValue())));BinaryExpLenStack.back()++;
  }
  if (ctx->number() != nullptr) {
    BinaryExpStack.push_back(std::any_cast<Value *>(visitNumber(ctx->number())));BinaryExpLenStack.back()++;
  }
  if (ctx->string() != nullptr) {
    cout << "String literal not supported in SysYIRGenerator." << endl;
  }
  return std::any();
}

std::any SysYIRGenerator::visitNumber(SysYParser::NumberContext *ctx) {
  if (ctx->ILITERAL() != nullptr) {
    int value = std::stol(ctx->ILITERAL()->getText(), nullptr, 0);
    return static_cast<Value *>(ConstantInteger::get(value));
  } else if (ctx->FLITERAL() != nullptr) {
    float value = std::stof(ctx->FLITERAL()->getText());
    return static_cast<Value *>(ConstantFloating::get(value));
  }
  throw std::runtime_error("Unknown number type.");
  return std::any(); // 不会到达这里
}

std::any SysYIRGenerator::visitCall(SysYParser::CallContext *ctx) {
  std::string funcName = ctx->Ident()->getText();
  Function *function = module->getFunction(funcName);
  if (function == nullptr) {
    function = module->getExternalFunction(funcName);
    if (function == nullptr) {
      std::cout << "The function " << funcName << " no defined." << std::endl;
      assert(function);
    }
  }

  std::vector<Value *> args = {};
  if (funcName == "starttime" || funcName == "stoptime") {
    args.emplace_back(
        ConstantInteger::get(static_cast<int>(ctx->getStart()->getLine())));
  } else {
    if (ctx->funcRParams() != nullptr) {
      args = std::any_cast<std::vector<Value *>>(visitFuncRParams(ctx->funcRParams()));
    }

    // 获取形参列表。`getArguments()` 返回的是 `Argument*` 的集合，
    // 每个 `Argument` 代表一个函数形参，其 `getType()` 就是指向形参的类型的指针类型。
    const auto& formalParams = function->getArguments();

    // 检查实参和形参数量是否匹配。
    if (args.size() != function->getNumArguments()) {
      std::cerr << "Error: Function call argument count mismatch for function '" << funcName << "'." << std::endl;
      assert(false && "Function call argument count mismatch!");
    }

    for (int i = 0; i < args.size(); i++) {
      // 形参的类型 (e.g., i32, float, i32*, [10 x i32]*)
      Type* formalParamExpectedValueType = formalParams[i]->getType();
      // 实参的实际类型 (e.g., i32, float, i32*, [67 x i32]*)
      Type* actualArgType = args[i]->getType();
      // 如果实参类型与形参类型不匹配，则尝试进行类型转换
      if (formalParamExpectedValueType != actualArgType) {
        ConstantValue *constValue = dynamic_cast<ConstantValue *>(args[i]);
        if (constValue != nullptr) {
          if (formalParamExpectedValueType->isInt() && actualArgType->isFloat()) {
            args[i] = ConstantInteger::get(static_cast<int>(constValue->getFloat()));
          } else if (formalParamExpectedValueType->isFloat() && actualArgType->isInt()) {
            args[i] = ConstantFloating::get(static_cast<float>(constValue->getInt()));
          } else {
            // 如果是常量但不是简单的 int/float 标量转换，
            // 或者是指针常量需要 bitcast，则让它进入非常量转换逻辑。
            // 例如，一个常量数组的地址，需要 bitcast 成另一种指针类型。
            // 目前不知道样例有没有这种情况，所以这里不做处理。
          }
        } 
        else {
          // 1. 标量值类型转换 (例如：int_reg 到 float_reg，float_reg 到 int_reg)
          if (formalParamExpectedValueType->isInt() && actualArgType->isFloat()) {
            args[i] = builder.createFtoIInst(args[i]);
          } else if (formalParamExpectedValueType->isFloat() && actualArgType->isInt()) {
            args[i] = builder.createItoFInst(args[i]);
          }
          // 2. 指针类型转换 (例如数组退化：`[N x T]*` 到 `T*`，或兼容指针类型之间)
          // 这种情况常见于数组参数，实参可能是一个更具体的数组指针类型，
          // 而形参是其退化后的基础指针类型。
          else if (formalParamExpectedValueType->isPointer() && actualArgType->isPointer()) {
            // 检查是否是数组指针到元素指针的decay
            // 例如：[N x T]* -> T*
            auto formalPtrType = formalParamExpectedValueType->as<PointerType>();
            auto actualPtrType = actualArgType->as<PointerType>();
            
            if (formalPtrType && actualPtrType && actualPtrType->getBaseType()->isArray()) {
              auto actualArrayType = actualPtrType->getBaseType()->as<ArrayType>();
              if (actualArrayType && 
                  formalPtrType->getBaseType() == actualArrayType->getElementType()) {
                // 这是数组decay的情况，添加GEP来获取数组的第一个元素
                std::vector<Value*> indices;
                indices.push_back(ConstantInteger::get(0)); // 第一个索引：解引用指针
                indices.push_back(ConstantInteger::get(0)); // 第二个索引：获取数组第一个元素
                args[i] = getGEPAddressInst(args[i], indices);
              }
            }
          }
          // 3. 其他未预期的类型不匹配
          // 如果代码执行到这里，说明存在编译器前端未处理的类型不兼容或错误。
          else {
            // assert(false && "Unhandled type mismatch for function call argument.");
          }
        }
      }
    }
  }

  return static_cast<Value *>(builder.createCallInst(function, args));
}

std::any SysYIRGenerator::visitUnaryExp(SysYParser::UnaryExpContext *ctx) {
  if (ctx->primaryExp() != nullptr) {
    visitPrimaryExp(ctx->primaryExp());
  } else if (ctx->call() != nullptr) {
    BinaryExpStack.push_back(std::any_cast<Value *>(visitCall(ctx->call())));BinaryExpLenStack.back()++;
    invalidateExpressionsOnCall();
  } else if (ctx->unaryOp() != nullptr) {
      // 遇到一元操作符，将其压入 BinaryExpStack
      auto opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->unaryOp()->children[0]);
      int opType = opNode->getSymbol()->getType();
      switch(opType) {
          case SysYParser::ADD: BinaryExpStack.push_back(BinaryOp::PLUS); BinaryExpLenStack.back()++; break;
          case SysYParser::SUB: BinaryExpStack.push_back(BinaryOp::NEG);  BinaryExpLenStack.back()++; break;
          case SysYParser::NOT: BinaryExpStack.push_back(BinaryOp::NOT);  BinaryExpLenStack.back()++; break;
          default: assert(false && "Unexpected operator in UnaryExp.");
      }
      visitUnaryExp(ctx->unaryExp());
  }
  return std::any();
}

std::any SysYIRGenerator::visitFuncRParams(SysYParser::FuncRParamsContext *ctx) {
  std::vector<Value *> params;
  for (const auto &exp : ctx->exp()) {
    auto param = std::any_cast<Value *>(computeExp(exp));
    params.push_back(param);
  }
    
  return params;
}


std::any SysYIRGenerator::visitMulExp(SysYParser::MulExpContext *ctx) {
  visitUnaryExp(ctx->unaryExp(0));

  for (int i = 1; i < ctx->unaryExp().size(); i++) {
    auto opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2*i-1]);
    int opType = opNode->getSymbol()->getType();
    switch(opType) {
      case SysYParser::MUL: BinaryExpStack.push_back(BinaryOp::MUL); BinaryExpLenStack.back()++; break;
      case SysYParser::DIV: BinaryExpStack.push_back(BinaryOp::DIV); BinaryExpLenStack.back()++; break;
      case SysYParser::MOD: BinaryExpStack.push_back(BinaryOp::MOD); BinaryExpLenStack.back()++; break;
      default: assert(false && "Unexpected operator in MulExp.");
    }
    visitUnaryExp(ctx->unaryExp(i));
  }
  return std::any();
}


std::any SysYIRGenerator::visitAddExp(SysYParser::AddExpContext *ctx) {
  visitMulExp(ctx->mulExp(0));

  for (int i = 1; i < ctx->mulExp().size(); i++) {
    auto opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2*i-1]);
    int opType = opNode->getSymbol()->getType();
    switch(opType) {
      case SysYParser::ADD: BinaryExpStack.push_back(BinaryOp::ADD); BinaryExpLenStack.back()++; break;
      case SysYParser::SUB: BinaryExpStack.push_back(BinaryOp::SUB); BinaryExpLenStack.back()++; break;
      default: assert(false && "Unexpected operator in AddExp.");
    }
    visitMulExp(ctx->mulExp(i));
  }
  return std::any();
}

std::any SysYIRGenerator::visitRelExp(SysYParser::RelExpContext *ctx) {
  Value* result = computeAddExp(ctx->addExp(0));

  for (int i = 1; i < ctx->addExp().size(); i++) {
    auto opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2*i-1]);
    int opType = opNode->getSymbol()->getType();

    Value* operand = computeAddExp(ctx->addExp(i));

    Type* resultType = result->getType();
    Type* operandType = operand->getType();

    ConstantValue* constResult = dynamic_cast<ConstantValue *>(result);
    ConstantValue* constOperand = dynamic_cast<ConstantValue *>(operand);

    // 常量比较
    if ((constResult != nullptr) && (constOperand != nullptr)) {
      auto operand1 = constResult->isFloat() ? constResult->getFloat()
                                             : constResult->getInt();
      auto operand2 = constOperand->isFloat() ? constOperand->getFloat()
                                              : constOperand->getInt();

      if (opType == SysYParser::LT)      result = ConstantInteger::get(operand1 < operand2 ? 1 : 0);
      else if (opType == SysYParser::GT) result = ConstantInteger::get(operand1 > operand2 ? 1 : 0);
      else if (opType == SysYParser::LE) result = ConstantInteger::get(operand1 <= operand2 ? 1 : 0);
      else if (opType == SysYParser::GE) result = ConstantInteger::get(operand1 >= operand2 ? 1 : 0);
      else                          assert(false);

    } else {
      Type* resultType = result->getType();
      Type* operandType = operand->getType();
      Type* floatType = Type::getFloatType();

      // 浮点数处理
      if (resultType == floatType || operandType == floatType) {
        if (resultType != floatType) {
          if (constResult != nullptr){
            if(dynamic_cast<ConstantInteger *>(constResult)) {
              // 如果是整型常量，转换为浮点型
              result = ConstantFloating::get(static_cast<float>(constResult->getInt()));
            } else if (dynamic_cast<ConstantFloating *>(constResult)) {
              // 如果是浮点型常量，直接使用
              result = ConstantFloating::get(static_cast<float>(constResult->getFloat()));
            }
          }
          else 
            result = builder.createItoFInst(result);
          
        }
        if (operandType != floatType) {
          if (constOperand != nullptr) {
            if(dynamic_cast<ConstantInteger *>(constOperand)) {
              // 如果是整型常量，转换为浮点型
              operand = ConstantFloating::get(static_cast<float>(constOperand->getInt()));
            } else if (dynamic_cast<ConstantFloating *>(constOperand)) {
              // 如果是浮点型常量，直接使用
              operand = ConstantFloating::get(static_cast<float>(constOperand->getFloat()));
            }
          }
          else 
            operand = builder.createItoFInst(operand);
          
        }

        if (opType == SysYParser::LT)      result = builder.createFCmpLTInst(result, operand);
        else if (opType == SysYParser::GT) result = builder.createFCmpGTInst(result, operand);
        else if (opType == SysYParser::LE) result = builder.createFCmpLEInst(result, operand);
        else if (opType == SysYParser::GE) result = builder.createFCmpGEInst(result, operand);
        else                          assert(false);

      } else {
        // 整数处理
        if (opType == SysYParser::LT)      result = builder.createICmpLTInst(result, operand);
        else if (opType == SysYParser::GT) result = builder.createICmpGTInst(result, operand);
        else if (opType == SysYParser::LE) result = builder.createICmpLEInst(result, operand);
        else if (opType == SysYParser::GE) result = builder.createICmpGEInst(result, operand);
        else                          assert(false);

      }
    }
  }

  return result;
}


std::any SysYIRGenerator::visitEqExp(SysYParser::EqExpContext *ctx) {
  // TODO：其实已经保证了result是一个int类型的值可以删除冗余判断逻辑
  Value * result = std::any_cast<Value *>(visitRelExp(ctx->relExp(0)));

  for (int i = 1; i < ctx->relExp().size(); i++) {
    auto opNode = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2*i-1]);
    int opType = opNode->getSymbol()->getType();

    Value * operand = std::any_cast<Value *>(visitRelExp(ctx->relExp(i)));

    ConstantValue* constResult = dynamic_cast<ConstantValue *>(result);
    ConstantValue* constOperand = dynamic_cast<ConstantValue *>(operand);

    if ((constResult != nullptr) && (constOperand != nullptr)) {
      auto operand1 = constResult->isFloat() ? constResult->getFloat()
                                             : constResult->getInt();
      auto operand2 = constOperand->isFloat() ? constOperand->getFloat()
                                              : constOperand->getInt();

      if (opType == SysYParser::EQ)      result = ConstantInteger::get(operand1 == operand2 ? 1 : 0);
      else if (opType == SysYParser::NE) result = ConstantInteger::get(operand1 != operand2 ? 1 : 0);
      else                          assert(false);
      
    } else {
      Type* resultType = result->getType();
      Type* operandType = operand->getType();
      Type* floatType = Type::getFloatType();

      if (resultType == floatType || operandType == floatType) {
        if (resultType != floatType) {
          if (constResult != nullptr){
            if(dynamic_cast<ConstantInteger *>(constResult)) {
              // 如果是整型常量，转换为浮点型
              result = ConstantFloating::get(static_cast<float>(constResult->getInt()));
            } else if (dynamic_cast<ConstantFloating *>(constResult)) {
              // 如果是浮点型常量，直接使用
              result = ConstantFloating::get(static_cast<float>(constResult->getFloat()));
            }
          }
          else 
            result = builder.createItoFInst(result);
        }
        if (operandType != floatType) {
          if (constOperand != nullptr) {
            if(dynamic_cast<ConstantInteger *>(constOperand)) {
              // 如果是整型常量，转换为浮点型
              operand = ConstantFloating::get(static_cast<float>(constOperand->getInt()));
            } else if (dynamic_cast<ConstantFloating *>(constOperand)) {
              // 如果是浮点型常量，直接使用
              operand = ConstantFloating::get(static_cast<float>(constOperand->getFloat()));
            }
          }
          else
            operand = builder.createItoFInst(operand);
        }

        if (opType == SysYParser::EQ)      result = builder.createFCmpEQInst(result, operand);
        else if (opType == SysYParser::NE) result = builder.createFCmpNEInst(result, operand);
        else                          assert(false);

      } else {

        if (opType == SysYParser::EQ)      result = builder.createICmpEQInst(result, operand);
        else if (opType == SysYParser::NE) result = builder.createICmpNEInst(result, operand);
        else                          assert(false);

      }
    }
  }

  if (ctx->relExp().size() == 1) {
    ConstantValue * constResult = dynamic_cast<ConstantValue *>(result);
    // 如果只有一个关系表达式，则将结果转换为0或1
    if (constResult != nullptr) {
      if (constResult->isFloat()) 
        result = ConstantInteger::get(constResult->getFloat() != 0.0F ? 1 : 0);
      else
        result = ConstantInteger::get(constResult->getInt() != 0 ? 1 : 0);
    }
  }

  return result;
}

std::any SysYIRGenerator::visitLAndExp(SysYParser::LAndExpContext *ctx){
  std::stringstream labelstring;
  BasicBlock *curBlock = builder.getBasicBlock();
  Function *function = builder.getBasicBlock()->getParent();
  BasicBlock *trueBlock = builder.getTrueBlock();
  BasicBlock *falseBlock = builder.getFalseBlock();
  auto conds = ctx->eqExp();

  for (int i = 0; i < conds.size() - 1; i++) {

    labelstring << "AND.L" << builder.getLabelIndex();
    BasicBlock *newtrueBlock = function->addBasicBlock(labelstring.str());
    labelstring.str("");

    auto cond = std::any_cast<Value *>(visitEqExp(ctx->eqExp(i)));
    builder.createCondBrInst(cond, newtrueBlock, falseBlock);

    BasicBlock::conectBlocks(curBlock, newtrueBlock);
    BasicBlock::conectBlocks(curBlock, falseBlock);

    curBlock = newtrueBlock;
    builder.setPosition(curBlock, curBlock->end());
  }

  auto cond = std::any_cast<Value *>(visitEqExp(conds.back()));
  builder.createCondBrInst(cond, trueBlock, falseBlock);

  BasicBlock::conectBlocks(curBlock, trueBlock);
  BasicBlock::conectBlocks(curBlock, falseBlock);

  return std::any();
}

auto SysYIRGenerator::visitLOrExp(SysYParser::LOrExpContext *ctx) -> std::any {
  std::stringstream labelstring;
  BasicBlock *curBlock = builder.getBasicBlock();
  Function *function = curBlock->getParent();
  auto conds = ctx->lAndExp();

  for (int i = 0; i < conds.size() - 1; i++) {
    labelstring << "OR.L" << builder.getLabelIndex();
    BasicBlock *newFalseBlock = function->addBasicBlock(labelstring.str());
    labelstring.str("");

    builder.pushFalseBlock(newFalseBlock);
    visitLAndExp(ctx->lAndExp(i));
    builder.popFalseBlock();

    builder.setPosition(newFalseBlock, newFalseBlock->end());
  }

  visitLAndExp(conds.back());

  return std::any();
}

// attention : 这里的type是数组元素的type
void Utils::tree2Array(Type *type, ArrayValueTree *root,
                        const std::vector<Value *> &dims, unsigned numDims,
                        ValueCounter &result, IRBuilder *builder) {
  Value* value = root->getValue();
  auto &children = root->getChildren();
  // 类型转换
  if (value != nullptr) {
    if (type == value->getType()) {
      result.push_back(value);
    } else {
      if (type == Type::getFloatType()) {
        ConstantValue* constValue = dynamic_cast<ConstantValue *>(value);
        if (constValue != nullptr) {
          if(dynamic_cast<ConstantInteger *>(constValue)) 
            result.push_back(ConstantFloating::get(static_cast<float>(constValue->getInt())));
          else if (dynamic_cast<ConstantFloating *>(constValue)) 
            result.push_back(ConstantFloating::get(static_cast<float>(constValue->getFloat())));
          else 
            assert(false && "Unknown constant type for float conversion.");
        }
        else 
          result.push_back(builder->createItoFInst(value));
        
      } else {
        ConstantValue* constValue = dynamic_cast<ConstantValue *>(value);
        if (constValue != nullptr){
          if(dynamic_cast<ConstantInteger *>(constValue)) 
            result.push_back(ConstantInteger::get(constValue->getInt()));
          else if (dynamic_cast<ConstantFloating *>(constValue)) 
            result.push_back(ConstantInteger::get(static_cast<int>(constValue->getFloat())));
          else 
            assert(false && "Unknown constant type for int conversion.");
        }
        else 
          result.push_back(builder->createFtoIInst(value));
        
      }
    }
    return;
  }

  auto beforeSize = result.size();
  for (const auto &child : children) {
    int begin = result.size();
    int newNumDims = 0;
    for (unsigned i = 0; i < numDims - 1; i++) {
      auto dim = dynamic_cast<ConstantValue *>(*(dims.rbegin() + i))->getInt();
      if (begin % dim == 0) {
        newNumDims += 1;
        begin /= dim;
      } else {
        break;
      }
    }
    tree2Array(type, child.get(), dims, newNumDims, result, builder);
  }
  auto afterSize = result.size();

  int blockSize = 1;
  for (unsigned i = 0; i < numDims; i++) {
    blockSize *= dynamic_cast<ConstantValue *>(*(dims.rbegin() + i))->getInt();
  }

  int num = blockSize - afterSize + beforeSize;
  if (num > 0) {
    if (type == Type::getFloatType())
      result.push_back(ConstantFloating::get(0.0F), num);
    else 
      result.push_back(ConstantInteger::get(0), num);
  }
}

void Utils::createExternalFunction(
    const std::vector<Type *> &paramTypes,
    const std::vector<std::string> &paramNames,
    const std::vector<std::vector<Value *>> &paramDims, Type *returnType,
    const std::string &funcName, Module *pModule, IRBuilder *pBuilder) {
  // 根据paramDims调整参数类型，数组参数需要转换为指针类型
  std::vector<Type *> adjustedParamTypes = paramTypes;
  for (int i = 0; i < paramTypes.size() && i < paramDims.size(); ++i) {
    if (!paramDims[i].empty()) {
      // 如果参数有维度信息，说明是数组参数，转换为指针类型
      adjustedParamTypes[i] = Type::getPointerType(paramTypes[i]);
    }
  }
  auto funcType = Type::getFunctionType(returnType, adjustedParamTypes);
  auto function = pModule->createExternalFunction(funcName, funcType);
  auto entry = function->getEntryBlock();
  pBuilder->setPosition(entry, entry->end());

  for (int i = 0; i < paramTypes.size(); ++i) {
    auto arg = new Argument(adjustedParamTypes[i], function, i, paramNames[i]);
    auto alloca = pBuilder->createAllocaInst(
        Type::getPointerType(adjustedParamTypes[i]), paramNames[i]);
    function->insertArgument(arg);
    auto store = pBuilder->createStoreInst(arg, alloca);
    pModule->addVariable(paramNames[i], alloca);
  }
}

void Utils::initExternalFunction(Module *pModule, IRBuilder *pBuilder) {
  std::vector<Type *> paramTypes;
  std::vector<std::string> paramNames;
  std::vector<std::vector<Value *>> paramDims;
  Type *returnType;
  std::string funcName;

  returnType = Type::getIntType();
  funcName = "getint";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);
  funcName = "getch";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);
  paramTypes.push_back(Type::getIntType());
  paramNames.emplace_back("x");
  paramDims.push_back(std::vector<Value *>{ConstantInteger::get(-1)});
  funcName = "getarray";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  returnType = Type::getFloatType();
  paramTypes.clear();
  paramNames.clear();
  paramDims.clear();
  funcName = "getfloat";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  returnType = Type::getIntType();
  paramTypes.push_back(Type::getFloatType());
  paramNames.emplace_back("x");
  paramDims.push_back(std::vector<Value *>{ConstantInteger::get(-1)});
  funcName = "getfarray";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  returnType = Type::getVoidType();

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramDims.clear();
  paramDims.emplace_back();
  funcName = "putint";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  funcName = "putch";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramTypes.push_back(Type::getIntType());
  paramDims.clear();
  paramDims.emplace_back();
  paramDims.push_back(std::vector<Value *>{ConstantInteger::get(-1)});
  paramNames.clear();
  paramNames.emplace_back("n");
  paramNames.emplace_back("a");
  funcName = "putarray";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getFloatType());
  paramDims.clear();
  paramDims.emplace_back();
  paramNames.clear();
  paramNames.emplace_back("a");
  funcName = "putfloat";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramTypes.push_back(Type::getFloatType());
  paramDims.clear();
  paramDims.emplace_back();
  paramDims.push_back(std::vector<Value *>{ConstantInteger::get(-1)});
  paramNames.clear();
  paramNames.emplace_back("n");
  paramNames.emplace_back("a");
  funcName = "putfarray";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramDims.clear();
  paramDims.emplace_back();
  paramNames.clear();
  paramNames.emplace_back("__LINE__");
  funcName = "starttime";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramDims.clear();
  paramDims.emplace_back();
  paramNames.clear();
  paramNames.emplace_back("__LINE__");
  funcName = "stoptime";
  Utils::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

}

void Utils::modify_timefuncname(Module *pModule){
  auto starttimeFunc = pModule->getExternalFunction("starttime");
  auto stoptimeFunc = pModule->getExternalFunction("stoptime");
  starttimeFunc->setName("_sysy_starttime");
  stoptimeFunc->setName("_sysy_stoptime");

}

} // namespace sysy