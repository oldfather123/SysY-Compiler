#include "SCCP.h"
#include "Dom.h"
#include "Liveness.h"
#include "SideEffectAnalysis.h"
#include <algorithm>
#include <cassert>
#include <cmath>  // For std::fmod, std::fabs
#include <limits> // For std::numeric_limits
#include <set>    // For std::set in isKnownPureFunction

namespace sysy {

// Pass ID for SCCP
void *SCCP::ID = (void *)&SCCP::ID;

// SCCPContext methods
SSAPValue SCCPContext::Meet(const SSAPValue &a, const SSAPValue &b) {
  if (a.state == LatticeVal::Bottom || b.state == LatticeVal::Bottom) {
    return SSAPValue(LatticeVal::Bottom);
  }
  if (a.state == LatticeVal::Top) {
    return b;
  }
  if (b.state == LatticeVal::Top) {
    return a;
  }
  // Both are constants
  if (a.constant_type != b.constant_type) {
    return SSAPValue(LatticeVal::Bottom); // 不同类型的常量，结果为 Bottom
  }
  if (a.constantVal == b.constantVal) {
    return a; // 相同常量
  }
  return SSAPValue(LatticeVal::Bottom); // 相同类型但值不同，结果为 Bottom
}

SSAPValue SCCPContext::GetValueState(Value *v) {
  if (auto constVal = dynamic_cast<ConstantValue *>(v)) {
    // 特殊处理 UndefinedValue：将其视为 Bottom
    if (dynamic_cast<UndefinedValue *>(constVal)) {
      return SSAPValue(LatticeVal::Bottom);
    }
    // 处理常规的 ConstantInteger 和 ConstantFloating
    if (constVal->getType()->isInt()) {
      return SSAPValue(constVal->getInt());
    } else if (constVal->getType()->isFloat()) {
      return SSAPValue(constVal->getFloat());
    } else {
      // 对于其他 ConstantValue 类型（例如，ConstantArray 等），
      // 如果它们的具体值不能用于标量常量传播，则保守地视为 Bottom。
      return SSAPValue(LatticeVal::Bottom);
    }
  }
  if (valueState.count(v)) {
    return valueState[v];
  }
  return SSAPValue(); // 默认初始化为 Top
}

void SCCPContext::UpdateState(Value *v, SSAPValue newState) {
  SSAPValue oldState = GetValueState(v);
  if (newState != oldState) {
    if (DEBUG) {
      std::cout << "Updating state for " << v->getName() << " from (";
      if (oldState.state == LatticeVal::Top)
        std::cout << "Top";
      else if (oldState.state == LatticeVal::Constant) {
        if (oldState.constant_type == ValueType::Integer)
          std::cout << "Const<int>(" << std::get<int>(oldState.constantVal) << ")";
        else
          std::cout << "Const<float>(" << std::get<float>(oldState.constantVal) << ")";
      } else
        std::cout << "Bottom";
      std::cout << ") to (";
      if (newState.state == LatticeVal::Top)
        std::cout << "Top";
      else if (newState.state == LatticeVal::Constant) {
        if (newState.constant_type == ValueType::Integer)
          std::cout << "Const<int>(" << std::get<int>(newState.constantVal) << ")";
        else
          std::cout << "Const<float>(" << std::get<float>(newState.constantVal) << ")";
      } else
        std::cout << "Bottom";
      std::cout << ")" << std::endl;
    }

    valueState[v] = newState;
    // 如果状态发生变化，将所有使用者添加到指令工作列表
    for (auto &use_ptr : v->getUses()) {
      if (auto userInst = dynamic_cast<Instruction *>(use_ptr->getUser())) {
        instWorkList.push(userInst);
      }
    }
  }
}

void SCCPContext::AddEdgeToWorkList(BasicBlock *fromBB, BasicBlock *toBB) {
  // 检查边是否已经访问过，防止重复处理
  if (visitedCFGEdges.count({fromBB, toBB})) {
    return;
  }
  visitedCFGEdges.insert({fromBB, toBB});

  if (DEBUG) {
    std::cout << "Adding edge to worklist: " << fromBB->getName() << " -> " << toBB->getName() << std::endl;
  }
  edgeWorkList.push({fromBB, toBB});
}

void SCCPContext::MarkBlockExecutable(BasicBlock *block) {
  if (executableBlocks.insert(block).second) { // insert 返回 pair，second 为 true 表示插入成功
    if (DEBUG) {
      std::cout << "Marking block " << block->getName() << " as executable." << std::endl;
    }
    // 将新可执行块中的所有指令添加到指令工作列表
    for (auto &inst_ptr : block->getInstructions()) {
      instWorkList.push(inst_ptr.get());
    }
  }
}

// 辅助函数：对二元操作进行常量折叠
SSAPValue SCCPContext::ComputeConstant(BinaryInst *binaryInst, SSAPValue lhsVal, SSAPValue rhsVal) {
  // 确保操作数是常量
  if (lhsVal.state != LatticeVal::Constant || rhsVal.state != LatticeVal::Constant) {
    return SSAPValue(LatticeVal::Bottom); // 如果不是常量，则不能折叠
  }

  // 处理整数运算 (kAdd, kSub, kMul, kDiv, kRem, kICmp*, kAnd, kOr)
  if (lhsVal.constant_type == ValueType::Integer && rhsVal.constant_type == ValueType::Integer) {
    int lhs = std::get<int>(lhsVal.constantVal);
    int rhs = std::get<int>(rhsVal.constantVal);
    int result = 0;

    switch (binaryInst->getKind()) {
    case Instruction::kAdd:
      result = lhs + rhs;
      break;
    case Instruction::kSub:
      result = lhs - rhs;
      break;
    case Instruction::kMul:
      result = lhs * rhs;
      break;
    case Instruction::kDiv:
      if (rhs == 0)
        return SSAPValue(LatticeVal::Bottom); // 除零
      result = lhs / rhs;
      break;
    case Instruction::kRem:
      if (rhs == 0)
        return SSAPValue(LatticeVal::Bottom); // 模零
      result = lhs % rhs;
      break;
    case Instruction::kICmpEQ:
      result = (lhs == rhs);
      break;
    case Instruction::kICmpNE:
      result = (lhs != rhs);
      break;
    case Instruction::kICmpLT:
      result = (lhs < rhs);
      break;
    case Instruction::kICmpGT:
      result = (lhs > rhs);
      break;
    case Instruction::kICmpLE:
      result = (lhs <= rhs);
      break;
    case Instruction::kICmpGE:
      result = (lhs >= rhs);
      break;
    case Instruction::kAnd:
      result = (lhs && rhs);
      break;
    case Instruction::kOr:
      result = (lhs || rhs);
      break;
    default:
      return SSAPValue(LatticeVal::Bottom); // 未知或不匹配的二元操作
    }
    return SSAPValue(result);
  }
  // 处理浮点运算 (kFAdd, kFSub, kFMul, kFDiv, kFCmp*)
  else if (lhsVal.constant_type == ValueType::Float && rhsVal.constant_type == ValueType::Float) {
    float lhs = std::get<float>(lhsVal.constantVal);
    float rhs = std::get<float>(rhsVal.constantVal);
    float f_result = 0.0f;
    int i_result = 0; // For comparison results

    switch (binaryInst->getKind()) {
    case Instruction::kFAdd:
      f_result = lhs + rhs;
      break;
    case Instruction::kFSub:
      f_result = lhs - rhs;
      break;
    case Instruction::kFMul:
      f_result = lhs * rhs;
      break;
    case Instruction::kFDiv:
      if (rhs == 0.0f)
        return SSAPValue(LatticeVal::Bottom); // 除零
      f_result = lhs / rhs;
      break;
    // kRem 不支持浮点数，但如果你的 IR 定义了浮点模运算，需要使用 std::fmod
    case Instruction::kFCmpEQ:
      i_result = (lhs == rhs);
      return SSAPValue(i_result);
    case Instruction::kFCmpNE:
      i_result = (lhs != rhs);
      return SSAPValue(i_result);
    case Instruction::kFCmpLT:
      i_result = (lhs < rhs);
      return SSAPValue(i_result);
    case Instruction::kFCmpGT:
      i_result = (lhs > rhs);
      return SSAPValue(i_result);
    case Instruction::kFCmpLE:
      i_result = (lhs <= rhs);
      return SSAPValue(i_result);
    case Instruction::kFCmpGE:
      i_result = (lhs >= rhs);
      return SSAPValue(i_result);
    default:
      return SSAPValue(LatticeVal::Bottom); // 未知或不匹配的浮点二元操作
    }
    return SSAPValue(f_result);
  }

  return SSAPValue(LatticeVal::Bottom); // 类型不匹配或不支持的类型组合
}

// 辅助函数：对一元操作进行常量折叠
SSAPValue SCCPContext::ComputeConstant(UnaryInst *unaryInst, SSAPValue operandVal) {
  if (operandVal.state != LatticeVal::Constant) {
    return SSAPValue(LatticeVal::Bottom);
  }

  if (operandVal.constant_type == ValueType::Integer) {
    int val = std::get<int>(operandVal.constantVal);
    switch (unaryInst->getKind()) {
    case Instruction::kAdd:
      return SSAPValue(val);
    case Instruction::kNeg:
      return SSAPValue(-val);
    case Instruction::kNot:
      return SSAPValue(!val);
    default:
      return SSAPValue(LatticeVal::Bottom);
    }
  } else if (operandVal.constant_type == ValueType::Float) {
    float val = std::get<float>(operandVal.constantVal);
    switch (unaryInst->getKind()) {
    case Instruction::kAdd:
      return SSAPValue(val);
    case Instruction::kFNeg:
      return SSAPValue(-val);
    case Instruction::kFNot:
      return SSAPValue(static_cast<int>(val == 0.0f)); // 浮点数非，0.0f 为真，其他为假
    default:
      return SSAPValue(LatticeVal::Bottom);
    }
  }
  return SSAPValue(LatticeVal::Bottom);
}

// 辅助函数：检查是否为已知的纯函数
bool SCCPContext::isKnownPureFunction(const std::string &funcName) const {
  // SysY中一些已知的纯函数（不修改全局状态，结果只依赖参数）
  static const std::set<std::string> knownPureFunctions = {
    // 数学函数（如果有的话）
    // "abs", "fabs", "sqrt", "sin", "cos"
    // SysY标准中基本没有纯函数，大多数都有I/O副作用
  };
  
  return knownPureFunctions.find(funcName) != knownPureFunctions.end();
}

// 辅助函数：计算纯函数的常量结果
SSAPValue SCCPContext::computePureFunctionResult(CallInst *call, const std::vector<SSAPValue> &argValues) {
  Function *calledFunc = call->getCallee();
  if (!calledFunc) {
    return SSAPValue(LatticeVal::Bottom);
  }
  
  std::string funcName = calledFunc->getName();
  
  // 目前SysY中没有标准的纯函数，这里预留扩展空间
  // 未来可以添加数学函数的常量折叠
  /*
  if (funcName == "abs" && argValues.size() == 1) {
    if (argValues[0].constant_type == ValueType::Integer) {
      int val = std::get<int>(argValues[0].constantVal);
      return SSAPValue(std::abs(val));
    }
  }
  */
  
  return SSAPValue(LatticeVal::Bottom);
}

// 辅助函数：查找存储到指定位置的常量值
SSAPValue SCCPContext::findStoredConstantValue(Value *ptr, BasicBlock *currentBB) {
  if (!aliasAnalysis) {
    if (DEBUG) {
      std::cout << "SCCP: No alias analysis available" << std::endl;
    }
    return SSAPValue(LatticeVal::Bottom);
  }
  
  if (DEBUG) {
    std::cout << "SCCP: Searching for stored constant value for ptr" << std::endl;
  }
  
  // 从当前块的指令列表末尾向前查找最近的Store
  std::vector<Instruction*> instructions;
  for (auto it = currentBB->begin(); it != currentBB->end(); ++it) {
    instructions.push_back(it->get());
  }
  
  for (int i = instructions.size() - 1; i >= 0; --i) {
    Instruction *prevInst = instructions[i];
    
    if (prevInst->isStore()) {
      StoreInst *storeInst = static_cast<StoreInst *>(prevInst);
      Value *storePtr = storeInst->getPointer();
      
      if (DEBUG) {
        std::cout << "SCCP: Checking store instruction" << std::endl;
      }
      
      // 使用别名分析检查Store是否针对相同的内存位置
      auto aliasResult = aliasAnalysis->queryAlias(ptr, storePtr);
      
      if (DEBUG) {
        std::cout << "SCCP: Alias result: " << (int)aliasResult << std::endl;
      }
      
      if (aliasResult == AliasType::SELF_ALIAS) {
        // 找到了对相同位置的Store，获取存储的值
        Value *storedValue = storeInst->getValue();
        
        if (DEBUG) {
          std::cout << "SCCP: Found matching store, checking value type" << std::endl;
        }
        
        // 检查存储的值是否为常量
        if (auto constInt = dynamic_cast<ConstantInteger *>(storedValue)) {
          int val = std::get<int>(constInt->getVal());
          if (DEBUG) {
            std::cout << "SCCP: Found constant integer value: " << val << std::endl;
          }
          return SSAPValue(val);
        } else if (auto constFloat = dynamic_cast<ConstantFloating *>(storedValue)) {
          float val = std::get<float>(constFloat->getVal());
          if (DEBUG) {
            std::cout << "SCCP: Found constant float value: " << val << std::endl;
          }
          return SSAPValue(val);
        } else {
          // 存储的值不是常量，检查其SCCP状态
          SSAPValue storedState = GetValueState(storedValue);
          if (storedState.state == LatticeVal::Constant) {
            if (DEBUG) {
              std::cout << "SCCP: Found SCCP constant value" << std::endl;
            }
            return storedState;
          } else {
            if (DEBUG) {
              std::cout << "SCCP: Stored value is not constant" << std::endl;
            }
          }
        }
        
        // 找到了最近的Store但不是常量，停止查找
        if (DEBUG) {
          std::cout << "SCCP: Found non-constant store, stopping search" << std::endl;
        }
        break;
      }
    }
  }
  
  if (DEBUG) {
    std::cout << "SCCP: No constant value found" << std::endl;
  }
  return SSAPValue(LatticeVal::Bottom);
}

// 辅助函数：动态检查数组访问是否为常量索引（考虑SCCP状态）
bool SCCPContext::hasRuntimeConstantAccess(Value *ptr) {
  if (auto gep = dynamic_cast<GetElementPtrInst *>(ptr)) {
    if (DEBUG) {
      std::cout << "SCCP: Checking runtime constant access for GEP instruction" << std::endl;
    }
    
    // 检查所有索引是否为常量或SCCP传播的常量
    bool allConstantIndices = true;
    for (auto indexUse : gep->getIndices()) {
      Value* index = indexUse->getValue();
      
      // 首先检查是否为编译时常量
      if (auto constInt = dynamic_cast<ConstantInteger *>(index)) {
        if (DEBUG) {
          std::cout << "SCCP: Index is compile-time constant integer: " << std::get<int>(constInt->getVal()) << std::endl;
        }
        continue;
      }
      if (auto constFloat = dynamic_cast<ConstantFloating *>(index)) {
        if (DEBUG) {
          std::cout << "SCCP: Index is compile-time constant float: " << std::get<float>(constFloat->getVal()) << std::endl;
        }
        continue;
      }
      
      // 检查是否为SCCP传播的常量
      SSAPValue indexState = GetValueState(index);
      if (indexState.state == LatticeVal::Constant) {
        if (DEBUG) {
          std::cout << "SCCP: Index is SCCP constant: ";
          if (indexState.constant_type == ValueType::Integer) {
            std::cout << std::get<int>(indexState.constantVal);
          } else {
            std::cout << std::get<float>(indexState.constantVal);
          }
          std::cout << std::endl;
        }
        continue;
      }
      
      // 如果任何一个索引不是常量，返回false
      if (DEBUG) {
        std::cout << "SCCP: Index is not constant, access is not constant" << std::endl;
      }
      allConstantIndices = false;
      break;
    }
    
    if (DEBUG) {
      std::cout << "SCCP: hasRuntimeConstantAccess result: " << (allConstantIndices ? "true" : "false") << std::endl;
    }
    return allConstantIndices;
  }
  
  // 对于非GEP指令，回退到别名分析的静态结果
  if (aliasAnalysis) {
    return aliasAnalysis->hasConstantAccess(ptr);
  }
  
  return false;
}

// 辅助函数：处理单条指令
void SCCPContext::ProcessInstruction(Instruction *inst) {
  SSAPValue oldState = GetValueState(inst);
  SSAPValue newState;

  if (!executableBlocks.count(inst->getParent())) {
    // 如果指令所在的块不可执行，其值应保持 Top
    // 除非它之前已经是 Bottom，因为 Bottom 是单调的
    if (oldState.state != LatticeVal::Bottom) {
      newState = SSAPValue(); // Top
    } else {
      newState = oldState; // 保持 Bottom
    }
    UpdateState(inst, newState);
    return; // 不处理不可达块中的指令的实际值
  }

  if(DEBUG) {
    std::cout << "Processing instruction: " << inst->getName() << " in block " << inst->getParent()->getName() << std::endl;
    std::cout << "Old state: ";
    if (oldState.state == LatticeVal::Top) {
      std::cout << "Top";
    } else if (oldState.state == LatticeVal::Constant) {
      if (oldState.constant_type == ValueType::Integer) {
        std::cout << "Const<int>(" << std::get<int>(oldState.constantVal) << ")";
      } else {
        std::cout << "Const<float>(" << std::get<float>(oldState.constantVal) << ")";
      }
    } else {
      std::cout << "Bottom";
    }
  }

  switch (inst->getKind()) {
  case Instruction::kAdd:
  case Instruction::kSub:
  case Instruction::kMul:
  case Instruction::kDiv:
  case Instruction::kRem:
  case Instruction::kICmpEQ:
  case Instruction::kICmpNE:
  case Instruction::kICmpLT:
  case Instruction::kICmpGT:
  case Instruction::kICmpLE:
  case Instruction::kICmpGE:
  case Instruction::kFAdd:
  case Instruction::kFSub:
  case Instruction::kFMul:
  case Instruction::kFDiv:
  case Instruction::kFCmpEQ:
  case Instruction::kFCmpNE:
  case Instruction::kFCmpLT:
  case Instruction::kFCmpGT:
  case Instruction::kFCmpLE:
  case Instruction::kFCmpGE:
  case Instruction::kAnd:
  case Instruction::kOr: {
    BinaryInst *binaryInst = static_cast<BinaryInst *>(inst);
    SSAPValue lhs = GetValueState(binaryInst->getOperand(0));
    SSAPValue rhs = GetValueState(binaryInst->getOperand(1));
    // 如果任一操作数是 Bottom，结果就是 Bottom
    if (lhs.state == LatticeVal::Bottom || rhs.state == LatticeVal::Bottom) {
      newState = SSAPValue(LatticeVal::Bottom);
    } else if (lhs.state == LatticeVal::Top || rhs.state == LatticeVal::Top) {
      newState = SSAPValue(); // Top
    } else {                  // 都是常量
      newState = ComputeConstant(binaryInst, lhs, rhs);
    }
    break;
  }
  case Instruction::kNeg:
  case Instruction::kNot:
  case Instruction::kFNeg:
  case Instruction::kFNot: {
    UnaryInst *unaryInst = static_cast<UnaryInst *>(inst);
    SSAPValue operand = GetValueState(unaryInst->getOperand());
    if (operand.state == LatticeVal::Bottom) {
      newState = SSAPValue(LatticeVal::Bottom);
    } else if (operand.state == LatticeVal::Top) {
      newState = SSAPValue(); // Top
    } else {                  // 是常量
      newState = ComputeConstant(unaryInst, operand);
    }
    break;
  }
  // 直接处理类型转换指令
  case Instruction::kFtoI: {
    SSAPValue operand = GetValueState(inst->getOperand(0));
    if (operand.state == LatticeVal::Constant && operand.constant_type == ValueType::Float) {
      newState = SSAPValue(static_cast<int>(std::get<float>(operand.constantVal)));
    } else if (operand.state == LatticeVal::Bottom) {
      newState = SSAPValue(LatticeVal::Bottom);
    } else { // Top
      newState = SSAPValue();
    }
    break;
  }
  case Instruction::kItoF: {
    SSAPValue operand = GetValueState(inst->getOperand(0));
    if (operand.state == LatticeVal::Constant && operand.constant_type == ValueType::Integer) {
      newState = SSAPValue(static_cast<float>(std::get<int>(operand.constantVal)));
    } else if (operand.state == LatticeVal::Bottom) {
      newState = SSAPValue(LatticeVal::Bottom);
    } else { // Top
      newState = SSAPValue();
    }
    break;
  }
  case Instruction::kBitFtoI: {
    SSAPValue operand = GetValueState(inst->getOperand(0));
    if (operand.state == LatticeVal::Constant && operand.constant_type == ValueType::Float) {
      float fval = std::get<float>(operand.constantVal);
      newState = SSAPValue(*reinterpret_cast<int *>(&fval));
    } else if (operand.state == LatticeVal::Bottom) {
      newState = SSAPValue(LatticeVal::Bottom);
    } else { // Top
      newState = SSAPValue();
    }
    break;
  }
  case Instruction::kBitItoF: {
    SSAPValue operand = GetValueState(inst->getOperand(0));
    if (operand.state == LatticeVal::Constant && operand.constant_type == ValueType::Integer) {
      int ival = std::get<int>(operand.constantVal);
      newState = SSAPValue(*reinterpret_cast<float *>(&ival));
    } else if (operand.state == LatticeVal::Bottom) {
      newState = SSAPValue(LatticeVal::Bottom);
    } else { // Top
      newState = SSAPValue();
    }
    break;
  }
  case Instruction::kLoad: {
    // 使用别名分析和副作用分析改进Load指令的处理
    Value *ptr = inst->getOperand(0);
    
    // 首先检查是否是全局常量
    if (auto globalVal = dynamic_cast<GlobalValue *>(ptr)) {
      // 如果 GlobalValue 有初始化器，并且它是常量，我们可以传播
      // TODO: 检查全局变量的初始化器进行常量传播
      newState = SSAPValue(LatticeVal::Bottom);
    } else if (aliasAnalysis && sideEffectAnalysis) {
      // 使用别名分析和副作用分析进行更精确的Load分析
      if (aliasAnalysis->isLocalArray(ptr) && (aliasAnalysis->hasConstantAccess(ptr) || hasRuntimeConstantAccess(ptr))) {
        // 对于局部数组的常量索引访问，检查是否有影响该位置的Store
        bool mayBeModified = false;
        
        if (DEBUG) {
          std::cout << "SCCP: Analyzing local array with constant access for modification" << std::endl;
        }
        
        // 遍历指令所在块之前的所有指令，查找可能修改该内存位置的Store
        BasicBlock *currentBB = inst->getParent();
        auto instPos = currentBB->findInstIterator(inst);
        
        SSAPValue foundConstantValue = SSAPValue(LatticeVal::Bottom);
        bool hasFoundDefinitiveStore = false;
        
        for (auto it = currentBB->begin(); it != instPos; ++it) {
          Instruction *prevInst = it->get();
          
          if (prevInst->isStore()) {
            StoreInst *storeInst = static_cast<StoreInst *>(prevInst);
            Value *storePtr = storeInst->getPointer();
            
            // 使用别名分析判断Store是否可能影响当前Load
            auto aliasResult = aliasAnalysis->queryAlias(ptr, storePtr);
            if (DEBUG) {
              std::cout << "SCCP: Checking store with alias result: " << (int)aliasResult << std::endl;
            }
            
            if (aliasResult == AliasType::SELF_ALIAS) {
              // 找到对相同位置的精确Store
              Value *storedValue = storeInst->getValue();
              
              if (DEBUG) {
                std::cout << "SCCP: Found exact store to same location, checking value" << std::endl;
              }
              
              // 检查存储的值是否为常量
              if (auto constInt = dynamic_cast<ConstantInteger *>(storedValue)) {
                int val = std::get<int>(constInt->getVal());
                if (DEBUG) {
                  std::cout << "SCCP: Store contains constant integer: " << val << std::endl;
                }
                foundConstantValue = SSAPValue(val);
                hasFoundDefinitiveStore = true;
                // 继续遍历，查找是否有更后面的Store覆盖这个值
              } else if (auto constFloat = dynamic_cast<ConstantFloating *>(storedValue)) {
                float val = std::get<float>(constFloat->getVal());
                if (DEBUG) {
                  std::cout << "SCCP: Store contains constant float: " << val << std::endl;
                }
                foundConstantValue = SSAPValue(val);
                hasFoundDefinitiveStore = true;
              } else {
                // 存储的值不是编译时常量，检查其SCCP状态
                SSAPValue storedState = GetValueState(storedValue);
                if (storedState.state == LatticeVal::Constant) {
                  if (DEBUG) {
                    std::cout << "SCCP: Store contains SCCP constant" << std::endl;
                  }
                  foundConstantValue = storedState;
                  hasFoundDefinitiveStore = true;
                } else {
                  if (DEBUG) {
                    std::cout << "SCCP: Store contains non-constant value" << std::endl;
                  }
                  // 非常量Store覆盖了之前的常量，无法传播
                  foundConstantValue = SSAPValue(LatticeVal::Bottom);
                  hasFoundDefinitiveStore = true;
                }
              }
            } else if (aliasResult != AliasType::NO_ALIAS) {
              // 可能有别名，但不确定
              if (DEBUG) {
                std::cout << "SCCP: Found store with uncertain alias, stopping propagation" << std::endl;
              }
              mayBeModified = true;
              break;
            }
          } else if (prevInst->isCall()) {
            // 检查函数调用是否可能修改该内存位置
            if (sideEffectAnalysis->mayModifyMemory(prevInst)) {
              // 进一步检查是否可能影响局部数组
              if (!aliasAnalysis->isLocalArray(ptr) || 
                  sideEffectAnalysis->mayModifyGlobal(prevInst)) {
                mayBeModified = true;
                break;
              }
            }
          } else if (prevInst->isMemset()) {
            // Memset指令可能影响内存，但只有在它在相关Store之前时才阻止常量传播
            MemsetInst *memsetInst = static_cast<MemsetInst *>(prevInst);
            Value *memsetPtr = memsetInst->getOperand(0);
            
            auto aliasResult = aliasAnalysis->queryAlias(ptr, memsetPtr);
            if (aliasResult != AliasType::NO_ALIAS) {
              // Memset可能影响这个位置，但我们继续查找是否有Store覆盖了memset
              // 不立即设置mayBeModified = true，让后续的Store有机会覆盖
              if (DEBUG) {
                std::cout << "SCCP: Found memset that may affect location, but continuing to check for overwriting stores" << std::endl;
              }
            }
          }
        }
        
        if (DEBUG) {
          std::cout << "SCCP: mayBeModified = " << (mayBeModified ? "true" : "false") << std::endl;
          std::cout << "SCCP: hasFoundDefinitiveStore = " << (hasFoundDefinitiveStore ? "true" : "false") << std::endl;
        }
        
        if (!mayBeModified) {
          if (hasFoundDefinitiveStore && foundConstantValue.state == LatticeVal::Constant) {
            // 直接使用找到的常量值
            if (DEBUG) {
              std::cout << "SCCP: Using found constant value from store analysis: ";
              if (foundConstantValue.constant_type == ValueType::Integer) {
                std::cout << std::get<int>(foundConstantValue.constantVal);
              } else {
                std::cout << std::get<float>(foundConstantValue.constantVal);
              }
              std::cout << std::endl;
            }
            newState = foundConstantValue;
          } else {
            // 如果没有发现修改该位置的指令，尝试用旧方法找到对应的Store值
            SSAPValue constantValue = findStoredConstantValue(ptr, inst->getParent());
            if (constantValue.state == LatticeVal::Constant) {
              if (DEBUG) {
                std::cout << "SCCP: Found constant value for array load using fallback method: ";
                if (constantValue.constant_type == ValueType::Integer) {
                  std::cout << std::get<int>(constantValue.constantVal);
                } else {
                  std::cout << std::get<float>(constantValue.constantVal);
                }
                std::cout << std::endl;
              }
              newState = constantValue;
            } else {
              newState = SSAPValue(LatticeVal::Bottom);
            }
          }
        } else {
          newState = SSAPValue(LatticeVal::Bottom);
        }
      } else {
        // 非局部数组或非常量访问，保守处理
        newState = SSAPValue(LatticeVal::Bottom);
      }
    } else {
      // 没有分析信息时保守处理
      newState = SSAPValue(LatticeVal::Bottom);
    }
    
    if (DEBUG && aliasAnalysis && sideEffectAnalysis) {
      std::cout << "SCCP: Load instruction analysis - " 
                << (aliasAnalysis->isLocalArray(ptr) ? "local array" : "other")
                << ", static constant access: " 
                << (aliasAnalysis->hasConstantAccess(ptr) ? "yes" : "no")
                << ", runtime constant access: "
                << (hasRuntimeConstantAccess(ptr) ? "yes" : "no") << std::endl;
    }
    break;
  }
  case Instruction::kStore:
    // Store 指令不产生值，其 SSAPValue 不重要
    newState = SSAPValue(); // 保持 Top
    break;
  case Instruction::kCall: {
    // 使用副作用分析改进Call指令处理
    CallInst *callInst = static_cast<CallInst *>(inst);
    
    if (sideEffectAnalysis) {
      const auto &sideEffect = sideEffectAnalysis->getInstructionSideEffect(callInst);
      
      // 检查是否为纯函数且所有参数都是常量
      if (sideEffect.isPure && sideEffect.type == SideEffectType::NO_SIDE_EFFECT) {
        // 对于纯函数，检查所有参数是否都是常量
        bool allArgsConstant = true;
        std::vector<SSAPValue> argValues;
        
        for (unsigned i = 0; i < callInst->getNumOperands() - 1; ++i) { // 减1排除函数本身
          SSAPValue argVal = GetValueState(callInst->getOperand(i));
          argValues.push_back(argVal);
          if (argVal.state != LatticeVal::Constant) {
            allArgsConstant = false;
            break;
          }
        }
        
        if (allArgsConstant) {
          // 对于参数全为常量的纯函数，可以尝试常量折叠
          // 但由于实际执行函数比较复杂，这里先标记为可优化
          // TODO: 实现具体的纯函数常量折叠
          Function *calledFunc = callInst->getCallee();
          if (calledFunc && isKnownPureFunction(calledFunc->getName())) {
            // 对已知的纯函数进行常量计算
            newState = computePureFunctionResult(callInst, argValues);
          } else {
            newState = SSAPValue(LatticeVal::Bottom);
          }
        } else {
          // 参数不全是常量，但函数无副作用
          newState = SSAPValue(LatticeVal::Bottom);
        }
        
        if (DEBUG) {
          std::cout << "SCCP: Pure function call with " 
                    << (allArgsConstant ? "constant" : "non-constant") << " arguments" << std::endl;
        }
      } else {
        // 有副作用的函数调用，保守处理
        newState = SSAPValue(LatticeVal::Bottom);
        if (DEBUG) {
          std::cout << "SCCP: Function call with side effects" << std::endl;
        }
      }
    } else {
      // 没有副作用分析时，保守处理所有Call
      newState = SSAPValue(LatticeVal::Bottom);
    }
    break;
  }
  case Instruction::kGetElementPtr: {
    // GEP 指令计算地址，通常其结果值（地址指向的内容）是 Bottom
    // 除非所有索引和基指针都是常量，指向一个确定常量值的内存位置
    bool all_ops_constant = true;
    for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
      if (GetValueState(inst->getOperand(i)).state != LatticeVal::Constant) {
        all_ops_constant = false;
        break;
      }
    }
    // 即使地址是常量，地址处的内容通常不是。所以通常是 Bottom
    newState = SSAPValue(LatticeVal::Bottom);
    break;
  }
  case Instruction::kPhi: {
    PhiInst *phi = static_cast<PhiInst *>(inst);
    if(DEBUG) {
      std::cout << "Processing Phi node: " << phi->getName() << std::endl;
    }
    // 标准SCCP的phi节点处理：
    // 只考虑可执行前驱，但要保证单调性
    SSAPValue currentPhiState = GetValueState(phi);
    SSAPValue phiResult = SSAPValue(); // 初始为 Top
    bool hasAnyExecutablePred = false;
    
    for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
      BasicBlock *incomingBlock = phi->getIncomingBlock(i);
      
      if (executableBlocks.count(incomingBlock)) {
        hasAnyExecutablePred = true;
        Value *incomingVal = phi->getIncomingValue(i);
        SSAPValue incomingState = GetValueState(incomingVal);
        if(DEBUG) {
          std::cout << "  Incoming from block " << incomingBlock->getName() 
                    << " with value " << incomingVal->getName() << " state: ";
          if (incomingState.state == LatticeVal::Top)
            std::cout << "Top";
          else if (incomingState.state == LatticeVal::Constant) {
            if (incomingState.constant_type == ValueType::Integer)
              std::cout << "Const<int>(" << std::get<int>(incomingState.constantVal) << ")";
            else
              std::cout << "Const<float>(" << std::get<float>(incomingState.constantVal) << ")";
          } else
            std::cout << "Bottom";
          std::cout << std::endl;
        }
        phiResult = Meet(phiResult, incomingState);
        
        if (phiResult.state == LatticeVal::Bottom) {
          break; // 提前退出优化
        }
      }
      // 不可执行前驱暂时被忽略
      // 这是标准SCCP的做法，依赖于单调性保证正确性
    }
    
    if (!hasAnyExecutablePred) {
      // 没有可执行前驱，保持Top状态
      newState = SSAPValue();
    } else {
      // 关键修复：使用严格的单调性
      // 确保phi的值只能从Top -> Constant -> Bottom单向变化
      if (currentPhiState.state == LatticeVal::Top) {
        // 从Top状态，可以变为任何计算结果
        newState = phiResult;
      } else if (currentPhiState.state == LatticeVal::Constant) {
        // 从Constant状态，只能保持相同常量或变为Bottom
        if (phiResult.state == LatticeVal::Constant && 
            currentPhiState.constantVal == phiResult.constantVal &&
            currentPhiState.constant_type == phiResult.constant_type) {
          // 保持相同的常量
          newState = currentPhiState;
        } else {
          // 不同的值，必须变为Bottom
          newState = SSAPValue(LatticeVal::Bottom);
        }
      } else {
        // 已经是Bottom，保持Bottom
        newState = currentPhiState;
      }
    }
    break;
  }
  case Instruction::kAlloca: // 对应 kAlloca
    // Alloca 分配内存，返回一个指针，其内容是 Bottom
    newState = SSAPValue(LatticeVal::Bottom);
    break;
  case Instruction::kBr:     // 对应 kBr
  case Instruction::kCondBr: // 对应 kCondBr
  case Instruction::kReturn: // 对应 kReturn
  case Instruction::kUnreachable: // 对应 kUnreachable
    // 终结符指令不产生值
    newState = SSAPValue(); // 保持 Top
    break;
  case Instruction::kMemset:
    // Memset 不产生值，但有副作用，不进行常量传播
    newState = SSAPValue(LatticeVal::Bottom);
    break;
  default:
    if (DEBUG) {
      std::cout << "Unimplemented instruction kind in SCCP: " << inst->getKind() << std::endl;
    }
    newState = SSAPValue(LatticeVal::Bottom); // 未知指令保守处理为 Bottom
    break;
  }
  UpdateState(inst, newState);

  // 特殊处理终结符指令，影响 CFG 边的可达性
  if (inst->isTerminator()) {
    if (inst->isBranch()) {

      if (inst->isCondBr()) { // 使用 kCondBr
        CondBrInst *branchInst = static_cast<CondBrInst *>(inst);
        SSAPValue condVal = GetValueState(branchInst->getOperand(0));
        if (condVal.state == LatticeVal::Constant) {
          bool condition_is_true = false;
          if (condVal.constant_type == ValueType::Integer) {
            condition_is_true = (std::get<int>(condVal.constantVal) != 0);
          } else if (condVal.constant_type == ValueType::Float) {
            condition_is_true = (std::get<float>(condVal.constantVal) != 0.0f);
          }

          if (condition_is_true) {
            AddEdgeToWorkList(branchInst->getParent(), branchInst->getThenBlock());
          } else {
            AddEdgeToWorkList(branchInst->getParent(), branchInst->getElseBlock());
          }
        } else { // 条件是 Top 或 Bottom，两条路径都可能
          AddEdgeToWorkList(branchInst->getParent(), branchInst->getThenBlock());
          AddEdgeToWorkList(branchInst->getParent(), branchInst->getElseBlock());
        }
      } else { // 无条件分支 (kBr)
        UncondBrInst *branchInst = static_cast<UncondBrInst *>(inst);
        AddEdgeToWorkList(branchInst->getParent(), branchInst->getBlock());
      }
    }
  }

  if (DEBUG) {
    std::cout << "New state: ";
    if (newState.state == LatticeVal::Top) {
      std::cout << "Top";
    } else if (newState.state == LatticeVal::Constant) {
      if (newState.constant_type == ValueType::Integer) {
        std::cout << "Const<int>(" << std::get<int>(newState.constantVal) << ")";
      } else {
        std::cout << "Const<float>(" << std::get<float>(newState.constantVal) << ")";
      }
    } else {
      std::cout << "Bottom";
    }
    std::cout << std::endl;
  }
}

// 辅助函数：处理单条控制流边
void SCCPContext::ProcessEdge(const std::pair<BasicBlock *, BasicBlock *> &edge) {
  BasicBlock *fromBB = edge.first;
  BasicBlock *toBB = edge.second;

  // 检查目标块是否已经可执行
  bool wasAlreadyExecutable = executableBlocks.count(toBB) > 0;
  
  // 标记目标块为可执行（如果还不是的话）
  MarkBlockExecutable(toBB);
  
  // 如果目标块之前就已经可执行，那么需要重新处理其中的phi节点
  // 因为现在有新的前驱变为可执行，phi节点的值可能需要更新
  if (wasAlreadyExecutable) {
    for (auto &inst_ptr : toBB->getInstructions()) {
      if (dynamic_cast<PhiInst *>(inst_ptr.get())) {
        instWorkList.push(inst_ptr.get());
      }
    }
  }
  // 如果目标块是新变为可执行的，MarkBlockExecutable已经添加了所有指令
}

// 阶段1: 常量传播与折叠
bool SCCPContext::PropagateConstants(Function *func) {
  bool changed = false;

  // 初始化：所有值 Top，所有块不可执行
  for (auto &bb_ptr : func->getBasicBlocks()) {
    executableBlocks.erase(bb_ptr.get());
    for (auto &inst_ptr : bb_ptr->getInstructions()) {
      valueState[inst_ptr.get()] = SSAPValue(); // Top
    }
  }

  // 初始化函数参数为Bottom（因为它们在编译时是未知的）
  for (auto arg : func->getArguments()) {
    valueState[arg] = SSAPValue(LatticeVal::Bottom);
    if (DEBUG) {
      std::cout << "Initializing function argument " << arg->getName() << " to Bottom" << std::endl;
    }
  }

  // 标记入口块为可执行
  if (!func->getBasicBlocks().empty()) {
    MarkBlockExecutable(func->getEntryBlock());
  }

  // 主循环：标准的SCCP工作列表算法
  // 交替处理边工作列表和指令工作列表直到不动点
  while (!instWorkList.empty() || !edgeWorkList.empty()) {
    // 处理所有待处理的CFG边
    while (!edgeWorkList.empty()) {
      ProcessEdge(edgeWorkList.front());
      edgeWorkList.pop();
    }

    // 处理所有待处理的指令
    while (!instWorkList.empty()) {
      Instruction *inst = instWorkList.front();
      instWorkList.pop();
      ProcessInstruction(inst);
    }
  }

  // 应用常量替换和死代码消除
  std::vector<Instruction *> instsToDelete;
  for (auto &bb_ptr : func->getBasicBlocks()) {
    BasicBlock *bb = bb_ptr.get();
    if (!executableBlocks.count(bb)) {
      // 整个块是死块，标记所有指令删除
      for (auto &inst_ptr : bb->getInstructions()) {
        instsToDelete.push_back(inst_ptr.get());
      }
      changed = true;
      continue;
    }
    for (auto it = bb->begin(); it != bb->end();) {
      Instruction *inst = it->get();
      SSAPValue ssaPVal = GetValueState(inst);

      if (ssaPVal.state == LatticeVal::Constant) {
        ConstantValue *constVal = nullptr;
        if (ssaPVal.constant_type == ValueType::Integer) {
          constVal = ConstantInteger::get(std::get<int>(ssaPVal.constantVal));
        } else if (ssaPVal.constant_type == ValueType::Float) {
          constVal = ConstantFloating::get(std::get<float>(ssaPVal.constantVal));
        } else {
          constVal = UndefinedValue::get(inst->getType()); // 不应发生
        }

        if (DEBUG) {
          std::cout << "Replacing " << inst->getName() << " with constant ";
          if (ssaPVal.constant_type == ValueType::Integer)
            std::cout << std::get<int>(ssaPVal.constantVal);
          else
            std::cout << std::get<float>(ssaPVal.constantVal);
          std::cout << std::endl;
        }
        inst->replaceAllUsesWith(constVal);
        instsToDelete.push_back(inst);
        ++it;
        changed = true;
      } else {
        // 如果操作数是常量，直接替换为常量值（常量折叠）
        for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
          Value *operand = inst->getOperand(i);
          SSAPValue opVal = GetValueState(operand);
          if (opVal.state == LatticeVal::Constant) {
            ConstantValue *constOp = nullptr;
            if (opVal.constant_type == ValueType::Integer) {
              constOp = ConstantInteger::get(std::get<int>(opVal.constantVal));
            } else if (opVal.constant_type == ValueType::Float) {
              constOp = ConstantFloating::get(std::get<float>(opVal.constantVal));
            } else {
              constOp = UndefinedValue::get(operand->getType());
            }

            if (constOp != operand) {
              inst->setOperand(i, constOp);
              changed = true;
            }
          }
        }
        ++it;
      }
    }
  }

  // 实际删除指令
  // TODO: 删除的逻辑需要考虑修改
  for (Instruction *inst : instsToDelete) {
    // 在尝试删除之前，先检查指令是否仍然附加到其父基本块。
    // 如果它已经没有父块，可能说明它已被其他方式处理或已处于无效状态。
    if (inst->getParent() != nullptr) {
      // 调用负责完整删除的函数，该函数应负责清除uses并将其从父块中移除。
      SysYIROptUtils::usedelete(inst);
    } 
    else {
        // 指令已不属于任何父块，无需再次删除。
        if (DEBUG) {
            std::cerr << "Info: Instruction " << inst->getName() << " was already detached or is not in a parent block." << std::endl;
        }
    }
  }
  return changed;
}

// 阶段2: 控制流简化
bool SCCPContext::SimplifyControlFlow(Function *func) {
  bool changed = false;

  // 重新确定可达块，因为 PropagateConstants 可能改变了分支条件
  std::unordered_set<BasicBlock *> newReachableBlocks = FindReachableBlocks(func);

  // 移除不可达块
  std::vector<BasicBlock *> blocksToDelete;
  for (auto &bb_ptr : func->getBasicBlocks()) {
    if (bb_ptr.get() == func->getEntryBlock())
      continue; // 入口块不能删除
    if (newReachableBlocks.find(bb_ptr.get()) == newReachableBlocks.end()) {
      blocksToDelete.push_back(bb_ptr.get());
      changed = true;
    }
  }
  for (BasicBlock *bb : blocksToDelete) {
    RemoveDeadBlock(bb, func);
  }

  // 简化分支指令
  for (auto &bb_ptr : func->getBasicBlocks()) {
    BasicBlock *bb = bb_ptr.get();
    if (!newReachableBlocks.count(bb))
      continue; // 只处理可达块

    Instruction *terminator = bb->terminator()->get();
    if (terminator->isBranch()) {

      if (terminator->isCondBr()) { // 检查是否是条件分支 (kCondBr)
        CondBrInst *branchInst = static_cast<CondBrInst *>(terminator);
        SSAPValue condVal = GetValueState(branchInst->getOperand(0));
        if (condVal.state == LatticeVal::Constant) {
          bool condition_is_true = false;
          if (condVal.constant_type == ValueType::Integer) {
            condition_is_true = (std::get<int>(condVal.constantVal) != 0);
          } else if (condVal.constant_type == ValueType::Float) {
            condition_is_true = (std::get<float>(condVal.constantVal) != 0.0f);
          }
          SimplifyBranch(branchInst, condition_is_true);
          changed = true;
        }
      }
    }
  }

  return changed;
}

// 查找所有可达的基本块 (基于常量条件)
std::unordered_set<BasicBlock *> SCCPContext::FindReachableBlocks(Function *func) {
  std::unordered_set<BasicBlock *> reachable;
  std::queue<BasicBlock *> q;

  if (func->getEntryBlock()) {
    q.push(func->getEntryBlock());
    reachable.insert(func->getEntryBlock());
  }

  while (!q.empty()) {
    BasicBlock *currentBB = q.front();
    q.pop();

    Instruction *terminator = currentBB->terminator()->get();
    if (!terminator)
      continue;

    if (terminator->isBranch()) {
      if (terminator->isCondBr()) { // 检查是否是条件分支 (kCondBr)
        CondBrInst *branchInst = static_cast<CondBrInst *>(terminator);
        SSAPValue condVal = GetValueState(branchInst->getOperand(0));
        if (condVal.state == LatticeVal::Constant) {
          bool condition_is_true = false;
          if (condVal.constant_type == ValueType::Integer) {
            condition_is_true = (std::get<int>(condVal.constantVal) != 0);
          } else if (condVal.constant_type == ValueType::Float) {
            condition_is_true = (std::get<float>(condVal.constantVal) != 0.0f);
          }

          if (condition_is_true) {
            BasicBlock *trueBlock = branchInst->getThenBlock();
            if (reachable.find(trueBlock) == reachable.end()) {
              reachable.insert(trueBlock);
              q.push(trueBlock);
            }
          } else {
            BasicBlock *falseBlock = branchInst->getElseBlock();
            if (reachable.find(falseBlock) == reachable.end()) {
              reachable.insert(falseBlock);
              q.push(falseBlock);
            }
          }
        } else { // 条件是 Top 或 Bottom，两条路径都可达
          for (auto succ : branchInst->getSuccessors()) {
            if (reachable.find(succ) == reachable.end()) {
              reachable.insert(succ);
              q.push(succ);
            }
          }
        }
      } else { // 无条件分支 (kBr)
        UncondBrInst *branchInst = static_cast<UncondBrInst *>(terminator);
        BasicBlock *targetBlock = branchInst->getBlock();
        if (reachable.find(targetBlock) == reachable.end()) {
          reachable.insert(targetBlock);
          q.push(targetBlock);
        }
      }
    } else if (terminator->isReturn() || terminator->isUnreachable()) {
      // ReturnInst 没有后继，不需要处理
      // UnreachableInst 也没有后继，不需要处理
    }
  }
  return reachable;
}

// 移除死块
void SCCPContext::RemoveDeadBlock(BasicBlock *bb, Function *func) {
  if (DEBUG) {
    std::cout << "Removing dead block: " << bb->getName() << std::endl;
  }
  // 首先更新其所有前驱的终结指令，移除指向死块的边
  std::vector<BasicBlock *> preds_to_update;
  for (auto &pred : bb->getPredecessors()) {
    if (pred != nullptr) { // 检查是否为空指针
        preds_to_update.push_back(pred);
    }
  }
  for (BasicBlock *pred : preds_to_update) {
    if (executableBlocks.count(pred)) {
      UpdateTerminator(pred, bb);
    }
  }

  // 移除其后继的 Phi 节点的入边
  std::vector<BasicBlock *> succs_to_update;
  for (auto succ : bb->getSuccessors()) {
    succs_to_update.push_back(succ);
  }
  for (BasicBlock *succ : succs_to_update) {
    RemovePhiIncoming(succ, bb);
    succ->removePredecessor(bb);
  }

  func->removeBasicBlock(bb); // 从函数中移除基本块
}

// 简化分支（将条件分支替换为无条件分支）
void SCCPContext::SimplifyBranch(CondBrInst *brInst, bool condVal) {
  BasicBlock *parentBB = brInst->getParent();
  BasicBlock *trueBlock = brInst->getThenBlock();
  BasicBlock *falseBlock = brInst->getElseBlock();

  if (DEBUG) {
    std::cout << "Simplifying branch in " << parentBB->getName() << ": cond is " << (condVal ? "true" : "false")
              << std::endl;
  }

  builder->setPosition(parentBB, parentBB->findInstIterator(brInst));
  if (condVal) {                            // 条件为真，跳转到真分支
    builder->createUncondBrInst(trueBlock); // 插入无条件分支 kBr
    SysYIROptUtils::usedelete(brInst);      // 移除旧的条件分支指令
    parentBB->removeSuccessor(falseBlock);
    falseBlock->removePredecessor(parentBB);
    RemovePhiIncoming(falseBlock, parentBB);
  } else {                                   // 条件为假，跳转到假分支
    builder->createUncondBrInst(falseBlock); // 插入无条件分支 kBr
    SysYIROptUtils::usedelete(brInst);       // 移除旧的条件分支指令
    parentBB->removeSuccessor(trueBlock);
    trueBlock->removePredecessor(parentBB);
    RemovePhiIncoming(trueBlock, parentBB);
  }
}

// 更新前驱块的终结指令（当一个后继块被移除时）
void SCCPContext::UpdateTerminator(BasicBlock *predBB, BasicBlock *removedSucc) {
  Instruction *terminator = predBB->terminator()->get();
  if (!terminator)
    return;

  if (terminator->isBranch()) {
    if (terminator->isCondBr()) { // 如果是条件分支
      CondBrInst *branchInst = static_cast<CondBrInst *>(terminator);
      if (branchInst->getThenBlock() == removedSucc) {
        if (DEBUG) {
          std::cout << "Updating cond br in " << predBB->getName() << ": True block (" << removedSucc->getName()
                    << ") removed. Converting to Br to " << branchInst->getElseBlock()->getName() << std::endl;
        }
        builder->setPosition(predBB, predBB->findInstIterator(branchInst));
        builder->createUncondBrInst(branchInst->getElseBlock());
        SysYIROptUtils::usedelete(branchInst);
        predBB->removeSuccessor(removedSucc);
      } else if (branchInst->getElseBlock() == removedSucc) {
        if (DEBUG) {
          std::cout << "Updating cond br in " << predBB->getName() << ": False block (" << removedSucc->getName()
                    << ") removed. Converting to Br to " << branchInst->getThenBlock()->getName() << std::endl;
        }
        builder->setPosition(predBB, predBB->findInstIterator(branchInst));
        builder->createUncondBrInst(branchInst->getThenBlock());
        SysYIROptUtils::usedelete(branchInst);
        predBB->removeSuccessor(removedSucc);
      }
    } else { // 无条件分支 (kBr)
      UncondBrInst *branchInst = static_cast<UncondBrInst *>(terminator);
      if (branchInst->getBlock() == removedSucc) {
        if (DEBUG) {
          std::cout << "Updating unconditional br in " << predBB->getName() << ": Target block ("
                    << removedSucc->getName() << ") removed. Replacing with Unreachable." << std::endl;
        }
        SysYIROptUtils::usedelete(branchInst);
        predBB->removeSuccessor(removedSucc);
        builder->setPosition(predBB, predBB->end());
        builder->createUnreachableInst();
      }
    }
  }
}

// 移除 Phi 节点的入边（当其前驱块被移除时）
void SCCPContext::RemovePhiIncoming(BasicBlock *phiParentBB, BasicBlock *removedPred) { // 修正 removedPred 类型
  std::vector<Instruction *> insts_to_check;
  for (auto &inst_ptr : phiParentBB->getInstructions()) {
    insts_to_check.push_back(inst_ptr.get());
  }

  for (Instruction *inst : insts_to_check) {
    if (auto phi = dynamic_cast<PhiInst *>(inst)) {
      phi->removeIncomingBlock(removedPred);
    }
  }
}

// 运行 SCCP 优化
void SCCPContext::run(Function *func, AnalysisManager &AM) {
  bool changed_constant_propagation = PropagateConstants(func);
  bool changed_control_flow = SimplifyControlFlow(func);

  // 如果任何一个阶段修改了 IR，标记分析结果为失效
  bool changed = changed_constant_propagation || changed_control_flow;
  changed |= SysYIROptUtils::eliminateRedundantPhisInFunction(func);
}

// SCCP Pass methods
bool SCCP::runOnFunction(Function *F, AnalysisManager &AM) {
  if (DEBUG) {
    std::cout << "Running SCCP on function: " << F->getName() << std::endl;
  }
  
  SCCPContext context(builder);
  
  // 获取别名分析结果
  if (auto *aliasResult = AM.getAnalysisResult<AliasAnalysisResult, SysYAliasAnalysisPass>(F)) {
    context.setAliasAnalysis(aliasResult);
    if (DEBUG) {
      std::cout << "SCCP: Using alias analysis results" << std::endl;
    }
  }
  
  // 获取副作用分析结果（Module级别）
  if (auto *sideEffectResult = AM.getAnalysisResult<SideEffectAnalysisResult, SysYSideEffectAnalysisPass>()) {
    context.setSideEffectAnalysis(sideEffectResult);
    if (DEBUG) {
      std::cout << "SCCP: Using side effect analysis results" << std::endl;
    }
  }
  
  context.run(F, AM);
  return true;
}

void SCCP::getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {
  // 声明依赖别名分析和副作用分析
  analysisDependencies.insert(&SysYAliasAnalysisPass::ID);
  analysisDependencies.insert(&SysYSideEffectAnalysisPass::ID);
  
  // analysisInvalidations.insert(nullptr); // 表示使所有默认分析失效
  analysisInvalidations.insert(&DominatorTreeAnalysisPass::ID); // 支配树可能受影响
  analysisInvalidations.insert(&LivenessAnalysisPass::ID); // 活跃性分析很可能失效
}

} // namespace sysy