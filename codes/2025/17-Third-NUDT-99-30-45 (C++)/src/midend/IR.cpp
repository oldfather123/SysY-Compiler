#include "IR.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <iomanip>
#include <vector>
#include "IRBuilder.h"

using namespace std;

inline std::string getMachineCode(float fval) {
  // 如果是0，直接返回
  if (fval == 0.0f) {
    return "0x0000000000000000";
  }
  
  // 正确的float到double扩展：先转换值，再获取double的位表示
  double dval = static_cast<double>(fval);
  uint64_t bits = *reinterpret_cast<uint64_t*>(&dval);
  
  std::stringstream ss;
  ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << bits;
  return ss.str();
}

/**
 * @file IR.cpp
 *
 * @brief 定义IR相关类型与操作的源文件
 */
namespace sysy {

// Global cleanup function implementation
void cleanupIRPools() {
  // Clean up the main memory pools that cause leaks
  ConstantValue::cleanup();
  UndefinedValue::cleanup();
  
  // Note: Type pools (PointerType, FunctionType, ArrayType) use unique_ptr
  // and will be automatically cleaned up when the program exits.
  // For more thorough cleanup during program execution, consider refactoring
  // to use singleton pattern with explicit cleanup methods.
}

/*相关打印函数*/

template <typename T>
ostream &interleave(std::ostream &os, const T &container, const std::string sep = ", ") {
  auto b = container.begin(), e = container.end();
  if (b == e)
    return os;
  os << *b;
  for (b = std::next(b); b != e; b = std::next(b))
    os << sep << *b;
  return os;
}

template <typename T>
ostream &interleave_call(std::ostream &os, const T &container, const std::string sep = ", ") {
  auto b = container.begin(), e = container.end();
  b = std::next(b); // Skip the first element
  if (b == e)
    return os;
  os << *b;
  for (b = std::next(b); b != e; b = std::next(b))
    os << sep << *b;
  return os;
}

static inline ostream &printVarName(ostream &os, const Value *var)
{
  if (dynamic_cast<const GlobalValue*>(var) != nullptr || 
      dynamic_cast<const ConstantVariable*>(var) != nullptr) {
    return os << "@" << var->getName();
  } else {
    return os << "%" << var->getName();
  }
}
static inline ostream &printBlockName(ostream &os, const BasicBlock *block) {
  return os <<  block->getName();
}
static inline ostream &printFunctionName(ostream &os, const Function *fn) {
  return os << '@' << fn->getName();
}
static inline ostream &printOperand(ostream &os, const Value *value) {
  auto constValue = dynamic_cast<const ConstantValue*>(value);
  if (constValue != nullptr) {
    if (auto constInt = dynamic_cast<const ConstantInteger*>(constValue)) {
      os << constInt->getInt();
    } else if (auto constFloat = dynamic_cast<const ConstantFloating*>(constValue)) {
      float f = constFloat->getFloat();
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "%.17g", f);
      std::string str(buffer);
      if (str.find('.') == std::string::npos && str.find('e') == std::string::npos) {
          str += ".0";
      }
      os << str;
  
    } else if (auto undefVal = dynamic_cast<const UndefinedValue*>(constValue)) {
      os << "undef";
    }
    return os;
  }
  return printVarName(os, value);
}

inline std::ostream& operator<<(std::ostream& os, const Type& type) {
  type.print(os);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const Value& value) {
  value.print(os);
  return os;
}
//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

auto Type::getIntType() -> Type * {
  static Type intType(kInt);
  return &intType;
}

auto Type::getFloatType() -> Type * {
  static Type floatType(kFloat);
  return &floatType;
}

auto Type::getVoidType() -> Type * {
  static Type voidType(kVoid);
  return &voidType;
}

auto Type::getLabelType() -> Type * {
  static Type labelType(kLabel);
  return &labelType;
}

auto Type::getPointerType(Type *baseType) -> Type * {
  // forward to PointerType
  return PointerType::get(baseType);
}

auto Type::getFunctionType(Type *returnType, const std::vector<Type *> &paramTypes) -> Type * {
  // forward to FunctionType
  return FunctionType::get(returnType, paramTypes);
}

auto Type::getArrayType(Type *elementType, unsigned numElements) -> Type * {
  // forward to ArrayType
  return ArrayType::get(elementType, numElements);
}

auto Type::getSize() const -> unsigned {
  switch (kind) {
    case kInt:
    case kFloat:
      return 4;
    case kLabel:
    case kPointer:
    case kFunction:
      return 8;
    case Kind::kArray: {
      const ArrayType* arrType = static_cast<const ArrayType*>(this);
      return arrType->getElementType()->getSize() * arrType->getNumElements();
    }
    case kVoid:
      return 0;
  }
  return 0;
}


void Type::print(ostream &os) const {
  auto kind = getKind();
  switch (kind){
    case kInt:   os << "i32"; break;
    case kFloat: os << "float"; break;
    case kVoid:  os << "void"; break;
    case kPointer:
      static_cast<const PointerType *>(this)->getBaseType()->print(os);
      os << "*";
      break;
    case kFunction:
      static_cast<const FunctionType *>(this)->getReturnType()->print(os);
      os << "(";
      interleave(os, static_cast<const FunctionType *>(this)->getParamTypes());
      os << ")";
      break;
    case kArray:
      os << "[";
      os << static_cast<const ArrayType *>(this)->getNumElements();
      os << " x ";
      static_cast<const ArrayType *>(this)->getElementType()->print(os);
      os << "]";
      break;
    case kLabel:
    default:
      cerr << "error type\n";
      break;
  }
}

void Use::print(std::ostream& os) const {
  os << "Use[" << index << "]: ";
  if (value) {
    printVarName(os, value);
  } else {
    os << "null";
  }
  os << " used by ";
  if (user) {
    os << "User@" << user;
  } else {
    os << "null";
  }
}

PointerType* PointerType::get(Type *baseType) {
  static std::map<Type *, std::unique_ptr<PointerType>> pointerTypes;
  auto iter = pointerTypes.find(baseType);
  if (iter != pointerTypes.end()) {
    return iter->second.get();
  }
  auto type = new PointerType(baseType);
  assert(type);
  auto result = pointerTypes.emplace(baseType, type);
  return result.first->second.get();
}

void PointerType::cleanup() {
  // Note: Due to static variable scoping, we can't directly access
  // the static map here. The cleanup will happen when the program exits.
  // For more thorough cleanup, consider using a singleton pattern.
}

FunctionType*FunctionType::get(Type *returnType, const std::vector<Type *> &paramTypes) {
  static std::set<std::unique_ptr<FunctionType>> functionTypes;
  auto iter =
      std::find_if(functionTypes.begin(), functionTypes.end(), [&](const std::unique_ptr<FunctionType> &type) -> bool {
        if (returnType != type->getReturnType() ||
            paramTypes.size() != static_cast<size_t>(type->getParamTypes().size())) {
          return false;
        }
        return std::equal(paramTypes.begin(), paramTypes.end(), type->getParamTypes().begin());
      });
  if (iter != functionTypes.end()) {
    return iter->get();
  }
  auto type = new FunctionType(returnType, paramTypes);
  assert(type);
  auto result = functionTypes.emplace(type);
  return result.first->get();
}

void FunctionType::cleanup() {
  // Note: Due to static variable scoping, we can't directly access
  // the static set here. The cleanup will happen when the program exits.
  // For more thorough cleanup, consider using a singleton pattern.
}

ArrayType *ArrayType::get(Type *elementType, unsigned numElements) {
  static std::set<std::unique_ptr<ArrayType>> arrayTypes;
  auto iter = std::find_if(arrayTypes.begin(), arrayTypes.end(), [&](const std::unique_ptr<ArrayType> &type) -> bool {
    return elementType == type->getElementType() && numElements == type->getNumElements();
  });
  if (iter != arrayTypes.end()) {
    return iter->get();
  }
  auto type = new ArrayType(elementType, numElements);
  assert(type);
  auto result = arrayTypes.emplace(type);
  return result.first->get();
}

void ArrayType::cleanup() {
  // Note: Due to static variable scoping, we can't directly access
  // the static set here. The cleanup will happen when the program exits.
  // For more thorough cleanup, consider using a singleton pattern.
}

void Argument::print(std::ostream& os) const {
  os << *getType() << " %" << getName();
}

// 辅助函数：递归生成多维数组的初始化格式
static void printArrayInitializer(std::ostream& os, Type* arrayType, const ValueCounter& initValues, size_t& valueIndex) {
  if (auto arrType = arrayType->as<ArrayType>()) {
    auto elementType = arrType->getElementType();
    auto numElements = arrType->getNumElements();
    
    os << "[";
    for (unsigned i = 0; i < numElements; ++i) {
      if (i > 0) os << ", ";
      
      if (elementType->isArray()) {
        // 嵌套数组，递归处理
        os << *elementType << " ";
        printArrayInitializer(os, elementType, initValues, valueIndex);
      } else {
        // 基础类型元素
        os << *elementType << " ";
        if (valueIndex < initValues.size()) {
          const auto& values = initValues.getValues();
          const auto& numbers = initValues.getNumbers();
          
          // 找到当前应该使用的值
          size_t currentValueIdx = 0;
          size_t currentCount = 0;
          for (size_t j = 0; j < values.size(); ++j) {
            if (currentCount + numbers[j] > valueIndex) {
              currentValueIdx = j;
              break;
            }
            currentCount += numbers[j];
          }
          
          printOperand(os, values[currentValueIdx]);
          valueIndex++;
        } else {
          // 没有更多初始化值，使用默认值
          if (elementType->isInt()) {
            os << "0";
          } else if (elementType->isFloat()) {
            os << "0.0";
          }
          valueIndex++;
        }
      }
    }
    os << "]";
  }
}

void GlobalValue::print(std::ostream& os) const {
  // 输出全局变量的LLVM IR格式
  os << "@" << getName() << " = global ";
  
  auto baseType = getType()->as<PointerType>()->getBaseType();
  os << *baseType << " ";
  
  // 输出初始化值
  if (initValues.size() == 0) {
    // 没有初始化值，使用zeroinitializer
    os << "zeroinitializer";
  } else {
    // 检查是否所有值都是零值
    bool allZero = true;
    const auto& values = initValues.getValues();
    for (Value* val : values) {
      if (auto constVal = dynamic_cast<ConstantValue*>(val)) {
        if (!constVal->isZero()) {
          allZero = false;
          break;
        }
      } else {
        allZero = false;
        break;
      }
    }
    
    if (allZero) {
      // 所有值都是零，使用zeroinitializer
      os << "zeroinitializer";
    } else if (initValues.size() == 1) {
      // 单个初始值 - 如果是标量零值也考虑使用zeroinitializer
      auto singleVal = initValues.getValue(0);
      if (auto constVal = dynamic_cast<ConstantValue*>(singleVal)) {
        if (constVal->isZero() && (baseType->isInt() || baseType->isFloat())) {
          // 标量零值使用zeroinitializer
          os << "zeroinitializer";
        } else {
          // 非零值或非基本类型，打印实际值
          printOperand(os, singleVal);
        }
      } else {
        // 非常量值，打印实际值
        printOperand(os, singleVal);
      }
    } else {
      // 数组初始值 - 根据数组维度生成正确的初始化格式
      if (baseType->isArray()) {
        size_t valueIndex = 0;
        printArrayInitializer(os, baseType, initValues, valueIndex);
      } else {
        // 单个非零标量值
        auto singleVal = initValues.getValue(0);
        printOperand(os, singleVal);
      }
    }
  }
}

void ConstantVariable::print(std::ostream& os) const {
  // 输出常量的LLVM IR格式 
  os << "@" << getName() << " = constant ";
  
  auto baseType = getType()->as<PointerType>()->getBaseType();
  os << *baseType << " ";
  
  // 输出初始化值
  if (initValues.size() == 0) {
    // 没有初始化值，使用zeroinitializer
    os << "zeroinitializer";
  } else {
    // 检查是否所有值都是零值
    bool allZero = true;
    const auto& values = initValues.getValues();
    for (Value* val : values) {
      if (auto constVal = dynamic_cast<ConstantValue*>(val)) {
        if (!constVal->isZero()) {
          allZero = false;
          break;
        }
      } else {
        allZero = false;
        break;
      }
    }
    
    if (allZero) {
      // 所有值都是零，使用zeroinitializer
      os << "zeroinitializer";
    } else if (initValues.size() == 1) {
      // 单个初始值 - 如果是标量零值也考虑使用zeroinitializer
      auto singleVal = initValues.getValue(0);
      if (auto constVal = dynamic_cast<ConstantValue*>(singleVal)) {
        if (constVal->isZero() && (baseType->isInt() || baseType->isFloat())) {
          // 标量零值使用zeroinitializer
          os << "zeroinitializer";
        } else {
          // 非零值或非基本类型，打印实际值
          printOperand(os, singleVal);
        }
      } else {
        // 非常量值，打印实际值
        printOperand(os, singleVal);
      }
    } else {
      // 数组初始值 - 根据数组维度生成正确的初始化格式
      if (baseType->isArray()) {
        size_t valueIndex = 0;
        printArrayInitializer(os, baseType, initValues, valueIndex);
      } else {
        // 单个非零标量值
        auto singleVal = initValues.getValue(0);
        printOperand(os, singleVal);
      }
    }
  }
}

void ConstantVariable::print_init(std::ostream& os) const {
  // 只输出初始化值部分
  if (initValues.size() > 0) {
    if (initValues.size() == 1) {
      // 单个初始值
      initValues.getValue(0)->print(os);
    } else {
      // 数组初始值 - 根据数组维度生成正确的初始化格式
      auto baseType = getType()->as<PointerType>()->getBaseType();
      if (baseType->isArray()) {
        size_t valueIndex = 0;
        printArrayInitializer(os, baseType, initValues, valueIndex);
      } else {
        // 单个非零标量值
        auto singleVal = initValues.getValue(0);
        singleVal->print(os);
      }
    }
  } else {
    os << "zeroinitializer";
  }
}

// void Value::replaceAllUsesWith(Value *value) {
//   for (auto &use : uses) {
//     auto user = use->getUser();
//     assert(user && "Use's user cannot be null");
//     user->setOperand(use->getIndex(), value);
//   }
//   uses.clear();
// }
void Value::replaceAllUsesWith(Value *value) {
  // 1. 创建 uses 列表的副本进行迭代。
  //    这样做是为了避免在迭代过程中，由于 setOperand 间接调用 removeUse 或 addUse
  //    导致原列表被修改，从而引发迭代器失效问题。
  std::list<std::shared_ptr<Use>> uses_copy = uses; 

  for (auto &use_ptr : uses_copy) { // 遍历副本
    // 2. 检查 shared_ptr<Use> 本身是否为空。这是最常见的崩溃原因之一。
    if (use_ptr == nullptr) {
      std::cerr << "Warning: Encountered a null std::shared_ptr<Use> in Value::uses list. Skipping this entry." << std::endl;
      // 在一个健康的 IR 中，这种情况不应该发生。如果经常出现，说明你的 Use 创建或管理有问题。
      continue; // 跳过空的智能指针
    }
    
    // 3. 检查 Use 对象内部的 User* 是否为空。
    User* user_val = use_ptr->getUser(); 
    if (user_val == nullptr) {
      std::cerr << "Warning: Use object (" << use_ptr.get() << ") has a null User* in replaceAllUsesWith. Skipping this entry. This indicates IR corruption." << std::endl;
      // 同样，在一个健康的 IR 中，Use 对象的 User* 不应该为空。
      continue; // 跳过用户指针为空的 Use 对象
    }
    
    // 如果走到这里，use_ptr 和 user_val 都是有效的，可以安全调用 setOperand
    user_val->setOperand(use_ptr->getIndex(), value);
  }

  // 4. 处理完所有 use 之后，清空原始的 uses 列表。
  //    replaceAllUsesWith 的目的就是将所有使用关系从当前 Value 转移走，
  //    所以最后清空列表是正确的。
  uses.clear(); 
}

// Implementations for static members

std::unordered_map<ConstantValueKey, ConstantValue*, ConstantValueHash, ConstantValueEqual> ConstantValue::mConstantPool;
std::unordered_map<Type*, UndefinedValue*> UndefinedValue::UndefValues;

ConstantValue* ConstantValue::get(Type* type, ConstantValVariant val) {
  ConstantValueKey key = {type, val};
  auto it = mConstantPool.find(key);
  if (it != mConstantPool.end()) {
    return it->second;
  }

  ConstantValue* newConstant = nullptr;
  if (std::holds_alternative<int>(val)) {
    newConstant = new ConstantInteger(type, std::get<int>(val));
  } else if (std::holds_alternative<float>(val)) {
    newConstant = new ConstantFloating(type, std::get<float>(val));
  } else {
    assert(false && "Unsupported ConstantValVariant type");
  }

  mConstantPool[key] = newConstant;
  return newConstant;
}

void ConstantValue::cleanup() {
  for (auto& pair : mConstantPool) {
    delete pair.second;
  }
  mConstantPool.clear();
}

ConstantInteger* ConstantInteger::get(Type* type, int val) {
  return dynamic_cast<ConstantInteger*>(ConstantValue::get(type, val));
}

ConstantFloating* ConstantFloating::get(Type* type, float val) {
  return dynamic_cast<ConstantFloating*>(ConstantValue::get(type, val));
}

UndefinedValue* UndefinedValue::get(Type* type) {
  assert(!type->isVoid() && "Cannot get UndefinedValue of void type!");

  auto it = UndefValues.find(type);
  if (it != UndefValues.end()) {
    return it->second;
  }

  UndefinedValue* newUndef = new UndefinedValue(type);
  UndefValues[type] = newUndef;
  return newUndef;
}

void UndefinedValue::cleanup() {
  for (auto& pair : UndefValues) {
    delete pair.second;
  }
  UndefValues.clear();
}

void ConstantValue::print(std::ostream &os) const {
  if(dynamic_cast<const ConstantInteger*>(this)) {
    dynamic_cast<const ConstantInteger*>(this)->print(os);
  } else if(dynamic_cast<const ConstantFloating*>(this)) {
    dynamic_cast<const ConstantFloating*>(this)->print(os);
  } else if(dynamic_cast<const UndefinedValue*>(this)) {
    dynamic_cast<const UndefinedValue*>(this)->print(os);
  } else {
    os << "unknown constant type";
  }
}

void ConstantInteger::print(std::ostream &os) const {
  os << this->getInt();
}
void ConstantFloating::print(std::ostream &os) const {
  float f = this->getFloat();
  
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%.17g", f);
  std::string str(buffer);
  if (str.find('.') == std::string::npos && str.find('e') == std::string::npos) {
       str += ".0";
  }
  os  << str;
}
void UndefinedValue::print(std::ostream &os) const {
  os << this->getType() << " undef";
}

void BasicBlock::print(std::ostream &os) const {
  assert(this->getName() != "" && "BasicBlock name cannot be empty");
  os << "  ";
  printBlockName(os, this);
  os << ":\n";
  
  bool reachedTerminator = false;
  for (auto &inst : instructions) {
    // 跳过终结指令后的死代码
    if (reachedTerminator) {
      continue;
    }
    
    os << "    ";
    
    // 特殊处理逻辑非指令
    if (auto* unaryInst = dynamic_cast<UnaryInst*>(inst.get())) {
      if (unaryInst->getKind() == Instruction::kNot && unaryInst->getType()->isInt()) {
        // 生成两行：先比较，再扩展
        os << "%tmp_not_" << unaryInst->getName() << " = icmp eq " 
           << *unaryInst->getOperand()->getType() << " ";
        printOperand(os, unaryInst->getOperand());
        os << ", 0\n    %";
        os << unaryInst->getName() << " = zext i1 %tmp_not_" << unaryInst->getName() << " to i32";
        os << '\n';
        
        // 检查当前指令是否是终结指令
        if (inst->isTerminator()) {
          reachedTerminator = true;
        }
        continue;
      }
    }
    
    os << *inst << '\n';
    
    // 检查当前指令是否是终结指令
    if (inst->isTerminator()) {
      reachedTerminator = true;
    }
  }
}

void PhiInst::print(std::ostream &os) const {
  printVarName(os, this);
  os << " = " << getKindString() << " " << *getType() << " ";
  for (unsigned i = 0; i < vsize; ++i) {
    if (i > 0) {
      os << ", ";
    }
    os << " [";
    printOperand(os, getIncomingValue(i));
    os << ", %";
    printBlockName(os, getIncomingBlock(i));
    os << "]";
  }
}

CallInst::CallInst(Function *callee, const std::vector<Value *> &args, BasicBlock *parent, const std::string &name)
    : Instruction(kCall, callee->getReturnType(), parent, name) {
  addOperand(callee);
  for (auto arg : args) {
    addOperand(arg);
  }
}

Function *CallInst::getCallee() const {
  return dynamic_cast<Function *>(getOperand(0));
}

void CallInst::print(std::ostream &os) const {
  if(!getType()->isVoid()) {
    printVarName(os, this) << " = ";
  }
  os << getKindString() << " " << *getType() << " " ;
  printFunctionName(os, getCallee());
  os << "(";
  
  // 打印参数，跳过第一个操作数（函数本身）
  for (unsigned i = 1; i < getNumOperands(); ++i) {
    if (i > 1) os << ", ";
    auto arg = getOperand(i);
    os << *arg->getType() << " ";
    printOperand(os, arg);
  }
  os << ")";
}

// 情况比较复杂就不用getkindstring了
void UnaryInst::print(std::ostream &os) const {
  printVarName(os, this) << " = ";
  switch (getKind()) {
  case kNeg:
    os << "sub i32 0, ";
    printOperand(os, getOperand());
    break;
  case kFNeg:
    os << "fsub float 0.0, ";
    printOperand(os, getOperand());
    break;
  case kNot:
    // 在BasicBlock::print中特殊处理整数逻辑非，这里不应该执行到
    os << "xor " << *getOperand()->getType() << " ";
    printOperand(os, getOperand());
    os << ", -1";
    break;
  case kFNot:
    os << "fcmp une " << *getOperand()->getType() << " ";
    printOperand(os, getOperand());
    os << ", 0.0";
    return;
  case kFtoI:
    os << "fptosi " << *getOperand()->getType() << " ";
    printOperand(os, getOperand());
    os << " to " << *getType();
    return;
  case kItoF:
    os << "sitofp " << *getOperand()->getType() << " ";
    printOperand(os, getOperand());
    os << " to " << *getType();
    return;
  default:
    os << "error unary inst";
    return;
  }
}



void AllocaInst::print(std::ostream &os) const {
  PointerType *ptrType = dynamic_cast<PointerType *>(getType());
  Type *baseType = ptrType->getBaseType();
  printVarName(os, this);
  os <<  " = " << getKindString() << " " << *baseType;
}

void BinaryInst::print(std::ostream &os) const {
  auto kind = getKind();
  
  // 检查是否为比较指令
  if (kind == kICmpEQ || kind == kICmpNE || kind == kICmpLT || 
      kind == kICmpGT || kind == kICmpLE || kind == kICmpGE) {
    // 整数比较指令 - 生成临时i1结果然后转换为i32
    // 使用指令地址和操作数地址的组合哈希来确保唯一性
    auto inst_hash = std::hash<const void*>{}(static_cast<const void*>(this));
    auto lhs_hash = std::hash<const void*>{}(static_cast<const void*>(getLhs()));
    auto rhs_hash = std::hash<const void*>{}(static_cast<const void*>(getRhs()));
    size_t combined_hash = inst_hash ^ (lhs_hash << 1) ^ (rhs_hash << 2);
    std::string tmpName = "tmp_icmp_" + std::to_string(combined_hash % 1000000007);
    os << "%" << tmpName << " = " << getKindString() << " " << *getLhs()->getType() << " ";
    printOperand(os, getLhs());
    os << ", ";
    printOperand(os, getRhs());
    os << "\n    ";
    printVarName(os, this) << " = zext i1 %" << tmpName << " to i32";
  } else if (kind == kFCmpEQ || kind == kFCmpNE || kind == kFCmpLT || 
             kind == kFCmpGT || kind == kFCmpLE || kind == kFCmpGE) {
    // 浮点比较指令 - 生成临时i1结果然后转换为i32
    // 使用指令地址和操作数地址的组合哈希来确保唯一性
    auto inst_hash = std::hash<const void*>{}(static_cast<const void*>(this));
    auto lhs_hash = std::hash<const void*>{}(static_cast<const void*>(getLhs()));
    auto rhs_hash = std::hash<const void*>{}(static_cast<const void*>(getRhs()));
    size_t combined_hash = inst_hash ^ (lhs_hash << 1) ^ (rhs_hash << 2);
    std::string tmpName = "tmp_fcmp_" + std::to_string(combined_hash % 1000000007);
    os << "%" << tmpName << " = " << getKindString() << " " << *getLhs()->getType() << " ";
    printOperand(os, getLhs());
    os << ", ";
    printOperand(os, getRhs());
    os << "\n    ";
    printVarName(os, this) << " = zext i1 %" << tmpName << " to i32";
  } else if(kind == kMulh){
    // 模拟高位乘法：先扩展为i64，乘法，右移32位，截断为i32
    static int mulhCount = 0;
    mulhCount++;
    std::string lhsName = getLhs()->getName();
    std::string rhsName = getRhs()->getName();
    std::string tmpLhs = "tmp_mulh_lhs_" + std::to_string(mulhCount) + "_" + lhsName;
    std::string tmpRhs = "tmp_mulh_rhs_" + std::to_string(mulhCount) +  rhsName;
    std::string tmpMul = "tmp_mulh_mul_" + std::to_string(mulhCount) + getName();
    std::string tmpHigh = "tmp_mulh_high_" + std::to_string(mulhCount) + getName();
    // printVarName(os, this) << " = "; // 输出最终变量名

    // os << "; mulh emulation\n    ";
    os << "%" << tmpLhs << " = sext i32 ";
    printOperand(os, getLhs());
    os << " to i64\n    ";
    os << "%" << tmpRhs << " = sext i32 ";
    printOperand(os, getRhs());
    os << " to i64\n    ";
    os << "%" << tmpMul << " = mul i64 %" << tmpLhs << ", %" << tmpRhs << "\n    ";
    os << "%" << tmpHigh << " = ashr i64 %" << tmpMul << ", 32\n    ";
    printVarName(os, this) << " = trunc i64 %" << tmpHigh << " to i32";
  }else {
    // 算术和逻辑指令
    printVarName(os, this) << " = ";
    os << getKindString() << " " << *getType() << " ";
    printOperand(os, getLhs());
    os << ", ";
    printOperand(os, getRhs());
  }
}

void ReturnInst::print(std::ostream &os) const {
  os << "ret ";
  if (hasReturnValue()) {
    os << *getReturnValue()->getType() << " ";
    printOperand(os, getReturnValue());
  } else {
    os << "void";
  }
}

void UncondBrInst::print(std::ostream &os) const {
  os << "br label %";
  printBlockName(os, getBlock());
}

void CondBrInst::print(std::ostream &os) const {
  Value* condition = getCondition();
  
  // 使用多个因素的组合哈希来确保唯一性
  std::string condName = condition->getName();
  if (condName.empty()) {
    // 使用条件值地址的哈希值作为唯一标识
    auto ptr_hash = std::hash<const void*>{}(static_cast<const void*>(condition));
    condName = "const_" + std::to_string(ptr_hash % 1000000007);
  }
  
  // 组合指令地址、条件地址和目标块地址的哈希来确保唯一性
  auto inst_hash = std::hash<const void*>{}(static_cast<const void*>(this));
  auto cond_hash = std::hash<const void*>{}(static_cast<const void*>(condition));
  auto then_hash = std::hash<const void*>{}(static_cast<const void*>(getThenBlock()));
  auto else_hash = std::hash<const void*>{}(static_cast<const void*>(getElseBlock()));
  size_t combined_hash = inst_hash ^ (cond_hash << 1) ^ (then_hash << 2) ^ (else_hash << 3);
  std::string uniqueSuffix = std::to_string(combined_hash % 1000000007);
  
  os << "%tmp_cond_" << condName << "_" << uniqueSuffix << " = icmp ne i32 ";
  printOperand(os, condition);
  os << ", 0\n    br i1 %tmp_cond_" << condName << "_" << uniqueSuffix;
  
  os << ", label %";
  printBlockName(os, getThenBlock());
  os << ", label %";
  printBlockName(os, getElseBlock());
}

void GetElementPtrInst::print(std::ostream &os) const {
  printVarName(os, this) << " = getelementptr ";
  
  // 获取基指针的基类型
  auto basePtr = getBasePointer();
  auto basePtrType = basePtr->getType()->as<PointerType>();
  auto baseType = basePtrType->getBaseType();
  
  os << *baseType << ", " << *basePtr->getType() << " ";
  printOperand(os, basePtr);
  
  // 打印索引 - 使用getIndex方法而不是getIndices
  for (unsigned i = 0; i < getNumIndices(); ++i) {
    auto index = getIndex(i);
    os << ", " << *index->getType() << " ";
    printOperand(os, index);
  }
}

void LoadInst::print(std::ostream &os) const {
  printVarName(os, this) << " = load " << *getType() << ", " << *getPointer()->getType() << " ";
  printOperand(os, getPointer());
}

void MemsetInst::print(std::ostream &os) const {
  Value* ptr = getPointer();
  
  // Generate a temporary bitcast instruction before the memset call
  // This is done at print time to avoid modifying the IR structure
  os << "%tmp_bitcast_" << ptr->getName() << " = bitcast " << *ptr->getType() << " ";
  printOperand(os, ptr);
  os << " to i8*\n    ";
  
  // Now call memset with the bitcast result
  os << "call void @llvm.memset.p0i8.i32(i8* %tmp_bitcast_" << ptr->getName() << ", i8 ";
  printOperand(os, getValue());
  os << ", i32 ";
  printOperand(os, getSize());
  os << ", i1 false)";
}

void StoreInst::print(std::ostream &os) const {
  os << "store " << *getValue()->getType() << " ";
  printOperand(os, getValue());
  os << ", " << *getPointer()->getType() << " ";
  printOperand(os, getPointer());
}




auto Function::getCalleesWithNoExternalAndSelf() -> std::set<Function *> {
  std::set<Function *> result;
  for (auto callee : callees) {
    if (parent->getExternalFunctions().count(callee->getName()) == 0U && callee != this) {
      result.insert(callee);
    }
  }
  return result;
}

void Value::removeAllUses() {
  while (!uses.empty()) {
    auto use = uses.back();
    uses.pop_back();
    if (use && use->getUser()) {
      auto user = use->getUser();
      int index = use->getIndex();
      user->removeOperand(index); ///< 从User中移除该操作数
    } else {
      // 如果use或user为null，输出警告信息
      assert(use != nullptr && "Use cannot be null");
      assert(use->getUser() != nullptr && "Use's user cannot be null");
    }
  }
  uses.clear();
}

/**
 * 设置操作数
 */
void User::setOperand(unsigned index, Value *newvalue) {
  if (index >= operands.size()) {
    std::cerr << "index=" << index << ", but mOperands max size=" << operands.size() << std::endl;
    assert(index < operands.size());
  }
  std::shared_ptr<Use> olduse = operands[index];
  Value *oldValue = olduse->getValue();
  if (oldValue != newvalue) {
    // 如果新值和旧值不同，先移除旧值的使用关系
    oldValue->removeUse(olduse);
    // 设置新的操作数
    operands[index] = std::make_shared<Use>(index, this, newvalue);
    newvalue->addUse(operands[index]);
  }
  else {
    // 如果新值和旧值相同，直接更新use的索引
    operands[index]->setValue(newvalue);
  }
}
/**
 * 替换操作数
 */
void User::replaceOperand(unsigned index, Value *value) {
  assert(index < getNumOperands());
  auto &use = operands[index];
  use->getValue()->removeUse(use);
  use->setValue(value);
  value->addUse(use);
}

/**
 * 移除操作数
 */
void User::removeOperand(unsigned index) {
  assert(index < getNumOperands() && "Index out of range in removeOperand");
  std::shared_ptr<Use> useToRemove = operands.at(index);
  Value *valueToRemove = useToRemove->getValue();
  if(valueToRemove) {
    if(valueToRemove == this) {
      std::cerr << "Cannot remove operand that is the same as the User itself." << std::endl;
    }
    valueToRemove->removeUse(useToRemove);
  }
  operands.erase(operands.begin() + index);
  unsigned newIndex = 0;
  for(auto it = operands.begin(); it != operands.end(); ++it, ++newIndex) {
    (*it)->setIndex(newIndex);  // 更新剩余操作数的索引
  }
}


/**
 * phi相关函数
 */

Value* PhiInst::getValfromBlk(BasicBlock* blk) {
  refreshMap();
  if( blk2val.find(blk) != blk2val.end()) {
    return blk2val.at(blk);
  }
  return nullptr;
}

BasicBlock* PhiInst::getBlkfromVal(Value* val) {
  // 返回第一个值对应的基本块
  for(unsigned i = 0; i < vsize; i++) {
    if(getIncomingValue(i) == val) {
      return getIncomingBlock(i);
    }
  }
  return nullptr;
}

void PhiInst::removeIncomingValue(Value* val){
  //根据value删除对应的基本块和值
  unsigned i = 0;
  BasicBlock* blk = getBlkfromVal(val);
  if(blk == nullptr) {
    return; // 如果val没有对应的基本块，直接返回
  }
  for(i = 0; i < vsize; i++) {
    if(getIncomingValue(i) == val) {
      break;
    }
  }
  removeIncoming(i);
}

void PhiInst::removeIncomingBlock(BasicBlock* blk){
  //根据Blk删除对应的基本块和值
  unsigned i = 0;
  Value* val = getValfromBlk(blk);
  if(val == nullptr) {
    return; // 如果blk没有对应的值，直接返回
  }
  for(i = 0; i < vsize; i++) {
    if(getIncomingBlock(i) == blk) {
      break;
    }
  }
  removeIncoming(i);
}

void PhiInst::setIncomingValue(unsigned k, Value* val) {
  assert(k < vsize && "PhiInst: index out of range");
  assert(val != nullptr && "PhiInst: value cannot be null");
  refreshMap();
  blk2val.erase(getIncomingBlock(k));
  setOperand(2 * k, val);
  blk2val[getIncomingBlock(k)] = val;
}

void PhiInst::setIncomingBlock(unsigned k, BasicBlock* blk) {
  assert(k < vsize && "PhiInst: index out of range");
  assert(blk != nullptr && "PhiInst: block cannot be null");
  refreshMap();
  auto oldVal = getIncomingValue(k);
  blk2val.erase(getIncomingBlock(k));
  setOperand(2 * k + 1, blk);
  blk2val[blk] = oldVal;
}

void PhiInst::replaceIncomingValue(Value* newVal, Value* oldVal) {
  refreshMap();
  assert(blk2val.find(getBlkfromVal(oldVal)) != blk2val.end() && "PhiInst: oldVal not found in blk2val");
  auto blk = getBlkfromVal(oldVal);
  removeIncomingValue(oldVal);
  addIncoming(newVal, blk);
}

void PhiInst::replaceIncomingBlock(BasicBlock* newBlk, BasicBlock* oldBlk) {
  refreshMap();
  assert(blk2val.find(oldBlk) != blk2val.end() && "PhiInst: oldBlk not found in blk2val");
  auto val = blk2val[oldBlk];
  removeIncomingBlock(oldBlk);
  addIncoming(val, newBlk);
}

void PhiInst::replaceIncomingValue(Value *oldValue, Value *newValue, BasicBlock *newBlock) {
  refreshMap();
  assert(blk2val.find(getBlkfromVal(oldValue)) != blk2val.end() && "PhiInst: oldValue not found in blk2val");
  auto oldBlock = getBlkfromVal(oldValue);
  removeIncomingValue(oldValue);
  addIncoming(newValue, newBlock);
}

void PhiInst::replaceIncomingBlock(BasicBlock *oldBlock, BasicBlock *newBlock, Value *newValue) {
  refreshMap();
  assert(blk2val.find(oldBlock) != blk2val.end() && "PhiInst: oldBlock not found in blk2val");
  auto oldValue = blk2val[oldBlock];
  removeIncomingBlock(oldBlock);
  addIncoming(newValue, newBlock);
}


/**
 * 获取变量指针
 * 如果在当前作用域或父作用域中找到变量，则返回该变量的指针，否则返回nullptr
 */
auto SymbolTable::getVariable(const std::string &name) const -> Value * {
  auto node = curNode;
  while (node != nullptr) {
    auto iter = node->varList.find(name);
    if (iter != node->varList.end()) {
      return iter->second;
    }
    node = node->pNode;
  }

  return nullptr;
}
/**
 * 添加变量到符号表
 */
auto SymbolTable::addVariable(const std::string &name, Value *variable) -> Value * {
  Value *result = nullptr;
  if (curNode != nullptr) {
    std::stringstream ss;
    auto iter = variableIndex.find(name);
    
    // 处理超长变量名（超过100字符）
    std::string displayName = name;
    if (name.length() > 100) {
      // 计算简单哈希
      std::hash<std::string> hasher;
      size_t hash = hasher(name);
      // 截断到前100个字符 + 哈希后缀
      displayName = name.substr(0, 100) + "_hash_" + std::to_string(hash);
    }
    
    // 区分全局变量和局部变量的命名
    bool isGlobalVariable = (dynamic_cast<GlobalValue *>(variable) != nullptr) || 
                           (dynamic_cast<ConstantVariable *>(variable) != nullptr);
    
    int index = 0;
    if (iter != variableIndex.end()) {
      index = iter->second;
      iter->second += 1;
    } else {
      variableIndex.emplace(name, 1);
    }
    
    if (isGlobalVariable) {
      // 全局变量不使用 local_ 前缀
      ss << displayName << index;
    } else {
      // 局部变量使用 "local_" 前缀确保不会和函数参数名冲突
      ss << "local_" << displayName << index;
    }
    
    variable->setName(ss.str());
    curNode->varList.emplace(name, variable);
    auto global = dynamic_cast<GlobalValue *>(variable);
    auto constvar = dynamic_cast<ConstantVariable *>(variable);
    if (global != nullptr) {
      globals.emplace_back(global);
    } else if (constvar != nullptr) {
      globalconsts.emplace_back(constvar);
    }

    result = variable;
  }

  return result;
}

/**
 * 注册函数参数名字到符号表，确保后续的alloca变量不会使用相同的名字
 */
void SymbolTable::registerParameterName(const std::string &name) {
  // 由于局部变量现在使用 "local_" 前缀，不需要复杂的冲突避免逻辑
  // 这个方法现在主要用于参数相关的记录，可以简化
}

/**
 * 直接添加变量到当前作用域，不进行重命名
 */
void SymbolTable::addVariableDirectly(const std::string &name, Value *variable) {
  if (curNode != nullptr) {
    curNode->varList.emplace(name, variable);
    auto global = dynamic_cast<GlobalValue *>(variable);
    auto constvar = dynamic_cast<ConstantVariable *>(variable);
    if (global != nullptr) {
      globals.emplace_back(global);
    } else if (constvar != nullptr) {
      globalconsts.emplace_back(constvar);
    }
  }
}

/**
 * 获取全局变量
 */
auto SymbolTable::getGlobals() -> std::vector<std::unique_ptr<GlobalValue>> & { return globals; }
/**
 *  获取常量
 */
auto SymbolTable::getConsts() const -> const std::vector<std::unique_ptr<ConstantVariable>> & { return globalconsts; }
/**
 * 进入新的作用域
 */
void SymbolTable::enterNewScope() {
  auto newNode = new SymbolTableNode;
  nodeList.emplace_back(newNode);
  if (curNode != nullptr) {
    curNode->children.emplace_back(newNode);
  }
  newNode->pNode = curNode;
  curNode = newNode;
}
/**
 * 进入全局作用域
 */
void SymbolTable::enterGlobalScope() { curNode = nodeList.front().get(); }
/**
 * 离开作用域
 */
void SymbolTable::leaveScope() { curNode = curNode->pNode; }
/**
 * 是否位于全局作用域
 */
auto SymbolTable::isInGlobalScope() const -> bool { return curNode->pNode == nullptr; }

/**
 *移动指令
 */
auto BasicBlock::moveInst(iterator sourcePos, iterator targetPos, BasicBlock *block) -> iterator {
  auto inst = sourcePos->release();
  inst->setParent(block);
  block->instructions.emplace(targetPos, inst);
  return instructions.erase(sourcePos);
}

/**
 * 为Value重命名以符合LLVM IR格式
 */
void renameValues(Function* function) {
  std::unordered_map<Value*, std::string> valueNames;
  unsigned tempCounter = 0;
  unsigned labelCounter = 0;
  
  // 检查名字是否需要重命名（只有纯数字或空名字才需要重命名）
  auto needsRename = [](const std::string& name) {
    if (name.empty()) return true;
    
    // 检查是否为纯数字
    for (char c : name) {
      if (!std::isdigit(c)) {
        return false; // 包含非数字字符，不需要重命名
      }
    }
    return true; // 纯数字或空字符串，需要重命名
  };
  
  // 重命名函数参数
  for (auto arg : function->getArguments()) {
    if (needsRename(arg->getName())) {
      valueNames[arg] = "%" + std::to_string(tempCounter++);
      arg->setName(valueNames[arg].substr(1)); // 去掉%前缀，因为printVarName会加上
    }
  }
  
  // 重命名基本块
  for (auto& block : function->getBasicBlocks()) {
    if (needsRename(block->getName())) {
      valueNames[block.get()] = "label" + std::to_string(labelCounter++);
      block->setName(valueNames[block.get()]);
    }
  }
  
  // 重命名指令
  for (auto& block : function->getBasicBlocks()) {
    bool reachedTerminator = false;
    for (auto& inst : block->getInstructions()) {
      // 跳过终结指令后的死代码
      if (reachedTerminator) {
        continue;
      }
      
      // 检查当前指令是否是终结指令
      if (inst->isTerminator()) {
        reachedTerminator = true;
      }
      
      // 只有产生值的指令需要重命名
      if (!inst->getType()->isVoid() && needsRename(inst->getName())) {
        valueNames[inst.get()] = "%" + std::to_string(tempCounter++);
        inst->setName(valueNames[inst.get()].substr(1)); // 去掉%前缀
      }
    }
  }
}

void Function::print(std::ostream& os) const {
  // 重命名所有值
  auto* mutableThis = const_cast<Function*>(this);
  renameValues(mutableThis);
  
  // 打印函数签名
  os << "define " << *getReturnType() << " ";
  printFunctionName(os, this);
  os << "(";
  
  // 打印参数列表
  const auto& args = const_cast<Function*>(this)->getArguments();
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) os << ", ";
    os << *args[i]->getType() << " ";
    printVarName(os, args[i]);
  }
  os << ") {\n";
  
  // 打印基本块
  for (auto& block : const_cast<Function*>(this)->getBasicBlocks()) {
    block->print(os);
  } 
  
  os << "}\n";
}

void Module::print(std::ostream& os) const {
  // 打印全局变量声明
  for (auto& globalVar : const_cast<Module*>(this)->getGlobals()) {
    globalVar->print(os);
    os << "\n";
  }
  
  // 打印常量声明
  for (auto& constVar : getConsts()) {
    constVar->print(os);
    os << "\n";
  }
  
  // 打印外部函数声明
  for (auto& extFunc : getExternalFunctions()) {
    os << "declare " << *extFunc.second->getReturnType() << " ";
    printFunctionName(os, extFunc.second.get());
    os << "(";
    
    const auto& paramTypes = extFunc.second->getParamTypes();
    bool first = true;
    for (auto paramType : paramTypes) {
      if (!first) os << ", ";
      os << *paramType;
      first = false;
    }
    os << ")\n";
  }
  
  // Always declare memset intrinsic when needed
  os << "declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i1)\n";
  
  if (!getExternalFunctions().empty()) {
    os << "\n"; // 外部函数和普通函数之间加空行
  }
  
  // 打印函数定义
  for (auto& func : const_cast<Module*>(this)->getFunctions()) {
    func.second->print(os);
    os << "\n"; // 函数之间加空行
  }
}

void Module::cleanup() {
  // 清理所有函数中的使用关系
  for (auto& func : functions) {
    if (func.second) {
      func.second->cleanup();
    }
  }
  
  for (auto& extFunc : externalFunctions) {
    if (extFunc.second) {
      extFunc.second->cleanup();
    }
  }
  
  // 清理符号表
  variableTable.cleanup();
  
  // 清理函数表
  functions.clear();
  externalFunctions.clear();
}

void Function::cleanup() {
  // 首先清理所有基本块中的使用关系
  for (auto& block : blocks) {
    if (block) {
      block->cleanup();
    }
  }
  
  // 然后安全地清理参数列表中的使用关系
  for (auto arg : arguments) {
    if (arg) {
      // 清理参数的所有使用关系
      arg->cleanup();
      // 现在安全地删除参数对象
      delete arg;
    }
  }
  
  // 清理基本块列表
  blocks.clear();
  arguments.clear();
  callees.clear();
}

void BasicBlock::cleanup() {
  // 直接清理指令列表，让析构函数自然处理
  instructions.clear();
  
  // 清理前驱后继关系
  predecessors.clear();
  successors.clear();
}

void User::cleanup() {
  // 简单清理：让shared_ptr的析构函数自动处理Use关系
  // 这样避免了手动管理可能已经被释放的Value对象
  operands.clear();
}

void SymbolTable::cleanup() {
  // 清理全局变量和常量
  globals.clear();
  globalconsts.clear();
  
  // 清理符号表节点
  nodeList.clear();
  
  // 重置当前节点
  curNode = nullptr;
  
  // 清理变量索引
  variableIndex.clear();
}

void Argument::cleanup() {
  // Argument继承自Value，清理其使用列表
  uses.clear();
}

}  // namespace sysy
