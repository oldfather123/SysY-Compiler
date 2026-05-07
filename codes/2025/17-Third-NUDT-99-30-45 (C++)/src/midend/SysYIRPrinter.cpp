#include "SysYIRPrinter.h"
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include "IR.h" // 确保IR.h包含了ArrayType、GetElementPtrInst等的定义

namespace sysy {

void SysYPrinter::printIR() {
  const auto &functions = pModule->getFunctions();

  //TODO: Print target datalayout and triple (minimal required by LLVM)

  printGlobalVariable();
  printGlobalConstant();

  for (const auto &iter : functions) {
    if (iter.second->getName() == "main") {
      printFunction(iter.second.get());
      break;
    }
  }

  for (const auto &iter : functions) {
    if (iter.second->getName() != "main") {
      printFunction(iter.second.get());
    }
  }
}

std::string SysYPrinter::getTypeString(Type *type) {
  if (type->isVoid()) {
      return "void";
  } else if (type->isInt()) {
    return "i32";
  } else if (type->isFloat()) {
    return "float";
  } else if (auto ptrType = dynamic_cast<PointerType*>(type)) {
    // 递归打印指针指向的类型，然后加上 '*'
    return getTypeString(ptrType->getBaseType()) + "*";
  } else if (auto funcType = dynamic_cast<FunctionType*>(type)) {
    // 对于函数类型，打印其返回类型
    // 注意：这里可能需要更完整的函数签名打印，取决于你的IR表示方式
    // 比如：`retType (paramType1, paramType2, ...)`
    // 但为了简化和LLVM IR兼容性，通常在定义时完整打印
    return getTypeString(funcType->getReturnType());
  } else if (auto arrayType = dynamic_cast<ArrayType*>(type)) { // 新增：处理数组类型
    // 打印格式为 [num_elements x element_type]
    return "[" + std::to_string(arrayType->getNumElements()) + " x " + getTypeString(arrayType->getElementType()) + "]";
  }
  assert(false && "Unsupported type");
  return "";
}

std::string SysYPrinter::getValueName(Value *value) {
  if (auto global = dynamic_cast<GlobalValue*>(value)) {
    return "@" + global->getName();
  } else if (auto inst = dynamic_cast<Instruction*>(value)) {
    return "%" + inst->getName();
  } else if (auto constInt = dynamic_cast<ConstantInteger*>(value)) { // 优先匹配具体的常量类型
    return std::to_string(constInt->getInt());
  } else if (auto constFloat = dynamic_cast<ConstantFloating*>(value)) { // 优先匹配具体的常量类型
    std::ostringstream oss;
    oss << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10) << constFloat->getFloat();
    return oss.str();
  } else if (auto constUndef = dynamic_cast<UndefinedValue*>(value)) { // 如果有Undef类型
    return "undef";
  } else if (auto constVal = dynamic_cast<ConstantValue*>(value)) { // fallback for generic ConstantValue
    // 这里的逻辑可能需要根据你ConstantValue的实际设计调整
    // 确保它能处理所有可能的ConstantValue
    if (auto constInt = dynamic_cast<ConstantInteger*>(value)) { // 优先匹配具体的常量类型
    return std::to_string(constInt->getInt());
    } else if (auto constFloat = dynamic_cast<ConstantFloating*>(value)) { // 优先匹配具体的常量类型
      std::ostringstream oss;
      oss << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10) << constFloat->getFloat();
      return oss.str();
    }
  } else if (auto constVar = dynamic_cast<ConstantVariable*>(value)) {
    return constVar->getName(); // 假设ConstantVariable有自己的名字或通过getByIndices获取值
  } else if (auto argVar = dynamic_cast<Argument*>(value)) {
    return "%" + argVar->getName(); // 假设ArgumentVariable有自己的名字
  }
  assert(false && "Unknown value type or unable to get value name");
  return "";
}

std::string SysYPrinter::getBlockName(BasicBlock *block) {
  static int blockId = 0; // 用于生成唯一的基本块ID
  if (block->getName().empty()) {
    return "bb" + std::to_string(blockId++); // 如果没有名字，生成一个唯一的基本块ID
  } else {
    return block->getName();
  }
}

void SysYPrinter::printType(Type *type) {
  std::cout << getTypeString(type);
}

void SysYPrinter::printValue(Value *value) {
  std::cout << getValueName(value);
}

void SysYPrinter::printGlobalVariable() {
  auto &globals = pModule->getGlobals();

  for (const auto &global : globals) {
    std::cout << "@" << global->getName() << " = global ";
    
    // 全局变量的类型是一个指针，指向其基类型 (可能是 ArrayType 或 Integer/FloatType)
    auto globalVarBaseType = dynamic_cast<PointerType *>(global->getType())->getBaseType();
    printType(globalVarBaseType); // 打印全局变量的实际类型 (例如 i32 或 [10 x i32])
    
    std::cout << " ";
    
    // 检查是否是数组类型 (通过检查 globalVarBaseType 是否是 ArrayType)
    if (globalVarBaseType->isArray()) {
      // 数组初始化器
      std::cout << "["; // LLVM IR 数组初始化器格式: [type value, type value, ...]
      auto values = global->getInitValues(); // 假设 getInitValues() 返回一个 ValueCounter
      const std::vector<sysy::Value *> &counterValues = values.getValues(); // 获取所有值
      
      for (size_t i = 0; i < counterValues.size(); i++) {
        if (i > 0) std::cout << ", ";
        // 打印元素类型，这个元素类型应该是数组的最终元素类型，例如 i32 或 float
        // 可以从 globalVarBaseType 逐层剥离得到最终元素类型，但这里简化为直接从值获取
        printType(counterValues[i]->getType());
        std::cout << " ";
        printValue(counterValues[i]);
      }
      std::cout << "]";
    } else {
      // 标量初始化器
      // 假设标量全局变量的初始化值通过 getByIndex(0) 获取
      Value* initVal = global->getByIndex(0);
      printType(initVal->getType()); // 打印标量值的类型
      std::cout << " ";
      printValue(initVal); // 打印标量值
    }
    
    std::cout << ", align 4" << std::endl;
  }
}


void SysYPrinter::printGlobalConstant() {
  auto &globalConstants = pModule->getConsts();

  for (const auto &globalConstant : globalConstants) {
    std::cout << "@" << globalConstant->getName() << " = global constant ";
    
    // 全局变量的类型是一个指针，指向其基类型 (可能是 ArrayType 或 Integer/FloatType)
    auto globalVarBaseType = dynamic_cast<PointerType *>(globalConstant->getType())->getBaseType();
    printType(globalVarBaseType); // 打印全局变量的实际类型 (例如 i32 或 [10 x i32])
    
    std::cout << " ";
    
    // 检查是否是数组类型 (通过检查 globalVarBaseType 是否是 ArrayType)
    if (globalVarBaseType->isArray()) {
      // 数组初始化器
      std::cout << "["; // LLVM IR 数组初始化器格式: [type value, type value, ...]
      auto values = globalConstant->getInitValues(); // 假设 getInitValues() 返回一个 ValueCounter
      const std::vector<sysy::Value *> &counterValues = values.getValues(); // 获取所有值
      
      for (size_t i = 0; i < counterValues.size(); i++) {
        if (i > 0) std::cout << ", ";
        // 打印元素类型，这个元素类型应该是数组的最终元素类型，例如 i32 或 float
        // 可以从 globalVarBaseType 逐层剥离得到最终元素类型，但这里简化为直接从值获取
        printType(counterValues[i]->getType());
        std::cout << " ";
        printValue(counterValues[i]);
      }
      std::cout << "]";
    } else {
      // 标量初始化器
      // 假设标量全局变量的初始化值通过 getByIndex(0) 获取
      Value* initVal = globalConstant->getByIndex(0);
      printType(initVal->getType()); // 打印标量值的类型
      std::cout << " ";
      printValue(initVal); // 打印标量值
    }
    
    std::cout << ", align 4" << std::endl;
  }
}

void SysYPrinter::printBlock(BasicBlock *block) {
  std::cout << getBlockName(block);
}

void SysYPrinter::printFunction(Function *function) {
  // Function signature
  std::cout << "define ";
  printType(function->getReturnType());
  std::cout << " @" << function->getName() << "(";
  
  auto entryBlock = function->getEntryBlock();
  const auto &args_types = function->getParamTypes();
  auto &args = function->getArguments();
  
  int i = 0;
  for (const auto &args_type : args_types) {
    if (i > 0) std::cout << ", ";
    printType(args_type);
    std::cout << " %" << args[i]->getName();
    i++;
  }
  
  std::cout << ") {" << std::endl;
  
  // Function body
  for (const auto &blockIter : function->getBasicBlocks()) {
    // Basic block label
    BasicBlock* blockPtr = blockIter.get();
    if (!blockPtr->getName().empty()) {
      std::cout << blockPtr->getName() << ":" << std::endl;
    }
    
    // Instructions
    for (const auto &instIter : blockIter->getInstructions()) {
      auto inst = instIter.get();
      std::cout << "  ";
      printInst(inst);
    }
  }
  
  std::cout << "}" << std::endl << std::endl;
}

void SysYPrinter::printInst(Instruction *pInst) {
  using Kind = Instruction::Kind;

  switch (pInst->getKind()) {
    case Kind::kAdd:
    case Kind::kSub:
    case Kind::kMul:
    case Kind::kDiv:
    case Kind::kRem:
    case Kind::kSrl:
    case Kind::kSll:
    case Kind::kSra:
    case Kind::kMulh:
    case Kind::kFAdd:
    case Kind::kFSub:
    case Kind::kFMul:
    case Kind::kFDiv:
    case Kind::kICmpEQ:
    case Kind::kICmpNE:
    case Kind::kICmpLT:
    case Kind::kICmpGT:
    case Kind::kICmpLE:
    case Kind::kICmpGE:
    case Kind::kFCmpEQ:
    case Kind::kFCmpNE:
    case Kind::kFCmpLT:
    case Kind::kFCmpGT:
    case Kind::kFCmpLE:
    case Kind::kFCmpGE:
    case Kind::kAnd:
    case Kind::kOr: {
      auto binInst = dynamic_cast<BinaryInst *>(pInst);
      
      // Print result variable if exists
      if (!binInst->getName().empty()) {
        std::cout << "%" << binInst->getName() << " = ";
      }
      
      // Operation name
      switch (pInst->getKind()) {
        case Kind::kAdd: std::cout << "add"; break;
        case Kind::kSub: std::cout << "sub"; break;
        case Kind::kMul: std::cout << "mul"; break;
        case Kind::kDiv: std::cout << "sdiv"; break;
        case Kind::kRem: std::cout << "srem"; break;
        case Kind::kSrl: std::cout << "lshr"; break;
        case Kind::kSll: std::cout << "shl"; break;
        case Kind::kSra: std::cout << "ashr"; break;
        case Kind::kMulh: std::cout << "mulh"; break;
        case Kind::kFAdd: std::cout << "fadd"; break;
        case Kind::kFSub: std::cout << "fsub"; break;
        case Kind::kFMul: std::cout << "fmul"; break;
        case Kind::kFDiv: std::cout << "fdiv"; break;
        case Kind::kICmpEQ: std::cout << "icmp eq"; break;
        case Kind::kICmpNE: std::cout << "icmp ne"; break;
        case Kind::kICmpLT: std::cout << "icmp slt"; break; // LLVM uses slt/sgt for signed less/greater than
        case Kind::kICmpGT: std::cout << "icmp sgt"; break;
        case Kind::kICmpLE: std::cout << "icmp sle"; break;
        case Kind::kICmpGE: std::cout << "icmp sge"; break;
        case Kind::kFCmpEQ: std::cout << "fcmp oeq"; break; // oeq for ordered equal
        case Kind::kFCmpNE: std::cout << "fcmp one"; break; // one for ordered not equal
        case Kind::kFCmpLT: std::cout << "fcmp olt"; break; // olt for ordered less than
        case Kind::kFCmpGT: std::cout << "fcmp ogt"; break; // ogt for ordered greater than
        case Kind::kFCmpLE: std::cout << "fcmp ole"; break; // ole for ordered less than or equal
        case Kind::kFCmpGE: std::cout << "fcmp oge"; break; // oge for ordered greater than or equal
        case Kind::kAnd: std::cout << "and"; break;
        case Kind::kOr: std::cout << "or"; break;
        default: break; // Should not reach here
      }
      
      // Types and operands
      std::cout << " ";
      // For comparison operations, print operand types instead of result type
      if (pInst->getKind() >= Kind::kICmpEQ && pInst->getKind() <= Kind::kFCmpGE) {
        printType(binInst->getLhs()->getType());
      } else {
        printType(binInst->getType());
      }
      std::cout << " ";
      printValue(binInst->getLhs());
      std::cout << ", ";
      printValue(binInst->getRhs());
      
      std::cout << std::endl;
    } break;
    
    case Kind::kNeg:
    case Kind::kNot:
    case Kind::kFNeg:
    case Kind::kFtoI:
    case Kind::kBitFtoI:
    case Kind::kItoF:
    case Kind::kBitItoF: {
      auto unyInst = dynamic_cast<UnaryInst *>(pInst);
      
      if (!unyInst->getName().empty()) {
        std::cout << "%" << unyInst->getName() << " = ";
      }
      
      switch (pInst->getKind()) {
        case Kind::kNeg: std::cout << "sub "; break; // integer negation is `sub i32 0, operand`
        case Kind::kNot: std::cout << "xor "; break; // logical/bitwise NOT is `xor i32 -1, operand` or `xor i1 true, operand`
        case Kind::kFNeg: std::cout << "fneg "; break; // float negation
        case Kind::kFtoI: std::cout << "fptosi "; break; // float to signed integer
        case Kind::kBitFtoI: std::cout << "bitcast "; break; // bitcast float to int
        case Kind::kItoF: std::cout << "sitofp "; break; // signed integer to float
        case Kind::kBitItoF: std::cout << "bitcast "; break; // bitcast int to float
        default: break; // Should not reach here
      }
      
      printType(unyInst->getOperand()->getType()); // Print operand type
      std::cout << " ";
      
      // Special handling for integer negation and logical NOT
      if (pInst->getKind() == Kind::kNeg) {
        std::cout << "0, "; // for 'sub i32 0, operand'
      } else if (pInst->getKind() == Kind::kNot) {
        // For logical NOT (i1 -> i1), use 'xor i1 true, operand'
        // For bitwise NOT (i32 -> i32), use 'xor i32 -1, operand'
        if (unyInst->getOperand()->getType()->isInt()) { // Assuming i32 for bitwise NOT
            std::cout << "NOT, "; // or specific bitmask for NOT
        } else { // Assuming i1 for logical NOT
            std::cout << "true, ";
        }
      }
      
      printValue(pInst->getOperand(0));
      
      // For type conversions (fptosi, sitofp, bitcast), need to specify destination type
      if (pInst->getKind() == Kind::kFtoI || pInst->getKind() == Kind::kItoF ||
          pInst->getKind() == Kind::kBitFtoI || pInst->getKind() == Kind::kBitItoF) {
        std::cout << " to ";
        printType(unyInst->getType()); // Print result type
      }
      
      std::cout << std::endl;
    } break;
    
    case Kind::kCall: {
      auto callInst = dynamic_cast<CallInst *>(pInst);
      auto function = callInst->getCallee();
      
      if (!callInst->getName().empty()) {
        std::cout << "%" << callInst->getName() << " = ";
      }
      
      std::cout << "call ";
      printType(callInst->getType()); // Return type of the call
      std::cout << " @" << function->getName() << "(";
      
      auto params = callInst->getArguments();
      bool first = true;
      for (auto &param : params) {
        if (!first) std::cout << ", ";
        first = false;
        printType(param->getValue()->getType()); // Type of argument
        std::cout << " ";
        printValue(param->getValue()); // Value of argument
      }
      
      std::cout << ")" << std::endl;
    } break;
    
    case Kind::kCondBr: {
      auto condBrInst = dynamic_cast<CondBrInst *>(pInst);
      std::cout << "br i1 "; // Condition type should be i1
      printValue(condBrInst->getCondition());
      std::cout << ", label %" << condBrInst->getThenBlock()->getName();
      std::cout << ", label %" << condBrInst->getElseBlock()->getName();
      std::cout << std::endl;
    } break;
    
    case Kind::kBr: {
      auto brInst = dynamic_cast<UncondBrInst *>(pInst);
      std::cout << "br label %" << brInst->getBlock()->getName();
      std::cout << std::endl;
    } break;
    
    case Kind::kReturn: {
      auto retInst = dynamic_cast<ReturnInst *>(pInst);
      std::cout << "ret ";
      if (retInst->getNumOperands() != 0) {
        printType(retInst->getOperand(0)->getType());
        std::cout << " ";
        printValue(retInst->getOperand(0));
      } else {
        std::cout << "void";
      }
      std::cout << std::endl;
    } break;

    case Kind::kUnreachable: {
      std::cout << "Unreachable" << std::endl;
      
    } break;

    case Kind::kAlloca: {
      auto allocaInst = dynamic_cast<AllocaInst *>(pInst);
      std::cout << "%" << allocaInst->getName() << " = alloca ";
      
      // AllocaInst 的类型现在应该是一个 PointerType，指向正确的 ArrayType 或 ScalarType
      // 例如：alloca i32, align 4  或者 alloca [10 x i32], align 4
      // auto allocatedType = dynamic_cast<PointerType *>(allocaInst->getType())->getBaseType();
      auto allocatedType = allocaInst->getAllocatedType();
      printType(allocatedType);
      
      std::cout << ", align 4" << std::endl;
    } break;
    
    case Kind::kLoad: {
      auto loadInst = dynamic_cast<LoadInst *>(pInst);
      std::cout << "%" << loadInst->getName() << " = load ";
      printType(loadInst->getType()); // 加载的结果类型
      std::cout << ", ";
      printType(loadInst->getPointer()->getType()); // 指针类型
      std::cout << " ";
      printValue(loadInst->getPointer()); // 要加载的地址
      
      std::cout << ", align 4" << std::endl;
    } break;
    
    case Kind::kStore: {
      auto storeInst = dynamic_cast<StoreInst *>(pInst);
      std::cout << "store ";
      printType(storeInst->getValue()->getType()); // 要存储的值的类型
      std::cout << " ";
      printValue(storeInst->getValue()); // 要存储的值
      std::cout << ", ";
      printType(storeInst->getPointer()->getType()); // 目标指针的类型
      std::cout << " ";
      printValue(storeInst->getPointer()); // 目标地址
      
      
      std::cout << ", align 4" << std::endl;
    } break;

    case Kind::kGetElementPtr: { // 新增：GetElementPtrInst 打印
      auto gepInst = dynamic_cast<GetElementPtrInst*>(pInst);
      std::cout << "%" << gepInst->getName() << " = getelementptr inbounds "; // 假设总是 inbounds
      
      // GEP 的第一个操作数是基指针，其类型是一个指向聚合类型的指针
      // 第一个参数是基指针所指向的聚合类型的类型 (e.g., [10 x i32])
      auto basePtrType = dynamic_cast<PointerType*>(gepInst->getBasePointer()->getType());
      printType(basePtrType->getBaseType()); // 打印基指针指向的类型
      
      std::cout << ", ";
      printType(gepInst->getBasePointer()->getType()); // 打印基指针自身的类型 (e.g., [10 x i32]*)
      std::cout << " ";
      printValue(gepInst->getBasePointer()); // 打印基指针
      
      // 打印所有索引
      for (auto indexVal : gepInst->getIndices()) { // 使用 getIndices() 迭代器
        std::cout << ", ";
        printType(indexVal->getValue()->getType()); // 打印索引的类型 (通常是 i32)
        std::cout << " ";
        printValue(indexVal->getValue()); // 打印索引值
      }
      std::cout << std::endl;
    } break;
    
    case Kind::kMemset: {
      auto memsetInst = dynamic_cast<MemsetInst *>(pInst);
      std::cout << "call void @llvm.memset.p0.";
      printType(memsetInst->getPointer()->getType());
      std::cout << "(";
      printType(memsetInst->getPointer()->getType());
      std::cout << " ";
      printValue(memsetInst->getPointer());
      std::cout << ", i8 ";
      printValue(memsetInst->getValue());
      std::cout << ", i32 ";
      printValue(memsetInst->getSize());
      std::cout << ", i1 false)" << std::endl; // alignment for memset is typically i1
    } break;
    
    case Kind::kPhi: {
      auto phiInst = dynamic_cast<PhiInst *>(pInst);
      // Phi 指令的名称通常是结果变量
      std::cout << "%" << phiInst->getName() << " = phi ";
      printType(phiInst->getType()); // Phi 结果类型
      
      // Phi 指令的操作数是成对的 [value, basic_block]
      // 这里假设 getOperands() 返回的是 (val1, block1, val2, block2...)
      // 如果你的 PhiInst 存储方式是 getIncomingValues() 和 getIncomingBlocks()，请相应调整
      // LLVM IR 格式: phi type [value1, block1], [value2, block2]
      bool firstPair = true;
      for (unsigned i = 0; i < phiInst->getNumIncomingValues(); ++i) { // 遍历成对的操作数
        if (!firstPair) std::cout << ", ";
        firstPair = false;
        std::cout << "[ ";
        printValue(phiInst->getIncomingValue(i));
        std::cout << ", %";
        printBlock(phiInst->getIncomingBlock(i));
        std::cout << " ]";
      }
      std::cout << std::endl;
    } break;
    
    // 以下两个 Kind 应该删除或替换为 kGEP
    // case Kind::kLa: { /* REMOVED */ } break;
    // case Kind::kGetSubArray: { /* REMOVED */ } break;
    
    default:
      assert(false && "Unsupported instruction kind in SysYPrinter");
      break;
  }
}

}  // namespace sysy