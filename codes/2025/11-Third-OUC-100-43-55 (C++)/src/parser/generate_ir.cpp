#include "generate_ir.h"
#include "instructions.h"
#include "number.h"
#include <algorithm>
using namespace ir;

#define MAKE_I32 std::make_unique<BasicType>(BasicKind::I32)
#define MAKE_F32 std::make_unique<BasicType>(BasicKind::F32)
#define MAKE_VOID std::make_unique<BasicType>(BasicKind::VOID)

void GenerateIR::initBuiltInFuncs() {
  _module->addFunction(_symbolTable->makeFunction(MAKE_I32, "getint"));
  _module->addFunction(_symbolTable->makeFunction(MAKE_I32, "getch"));
  _module->addFunction(_symbolTable->makeFunction(MAKE_F32, "getfloat"));
  auto func = _symbolTable->makeFunction(MAKE_I32, "getarray");
  func->addArg(
      std::make_unique<Argument>(std::make_unique<PointerType>(MAKE_I32), "a"));
  _module->addFunction(std::move(func));
  func = _symbolTable->makeFunction(MAKE_I32, "getfarray");
  func->addArg(
      std::make_unique<Argument>(std::make_unique<PointerType>(MAKE_F32), "a"));
  _module->addFunction(std::move(func));
  func = _symbolTable->makeFunction(MAKE_VOID, "putint");
  func->addArg(std::make_unique<Argument>(MAKE_I32, "a"));
  _module->addFunction(std::move(func));
  func = _symbolTable->makeFunction(MAKE_VOID, "putch");
  func->addArg(std::make_unique<Argument>(MAKE_I32, "a"));
  _module->addFunction(std::move(func));
  func = _symbolTable->makeFunction(MAKE_VOID, "putarray");
  func->addArg(std::make_unique<Argument>(MAKE_I32, "n"));
  func->addArg(std::make_unique<Argument>(MAKE_I32, "a"));
  _module->addFunction(std::move(func));
  func = _symbolTable->makeFunction(MAKE_VOID, "putfloat");
  func->addArg(std::make_unique<Argument>(MAKE_F32, "a"));
  _module->addFunction(std::move(func));
  func = _symbolTable->makeFunction(MAKE_VOID, "putfarray");
  func->addArg(std::make_unique<Argument>(MAKE_I32, "n"));
  func->addArg(std::make_unique<Argument>(MAKE_F32, "a"));
  _module->addFunction(std::move(func));
  func = _symbolTable->makeFunction(MAKE_VOID, "_sysy_starttime");
  func->addArg(std::make_unique<Argument>(MAKE_I32, "lineno"));
  _module->addFunction(std::move(func));
  func = _symbolTable->makeFunction(MAKE_VOID, "_sysy_stoptime");
  func->addArg(std::make_unique<Argument>(MAKE_I32, "lineno"));
  _module->addFunction(std::move(func));
}

void GenerateIR::initSysCalls() {
  auto func = _symbolTable->makeFunction(MAKE_VOID, "memset");
  func->addArg(std::make_unique<Argument>(
      std::make_unique<PointerType>(MAKE_I32), "addr"));
  func->addArg(std::make_unique<Argument>(MAKE_I32, "value"));
  func->addArg(std::make_unique<Argument>(MAKE_I32, "size"));
  _module->addFunction(std::move(func));
}

void GenerateIR::checkTerminator() {
  for (const auto &func : _module->getFunctions()) {
    if (func->empty()) {
      continue;
    }
    for (size_t i = 0; i < func->size() - 1; i++) {
      auto block = func->getBlock(i);
      block->clearUnreachables();
      if (!block->hasTerminator()) {
        block->pushInstruction(std::make_unique<BranchInst>(
            static_cast<BasicBlock *>(func->getBlock(i + 1))));
      }
    }
  }
}

void GenerateIR::processCond(Value *value) {
  if (!value) {
    return;
  }
  auto type = static_cast<BasicType *>(value->getType())->getBasicKind();
  Value *cond;
  std::unique_ptr<Instruction> cmpInst;
  switch (type) {
  case BasicKind::I1:
    cond = value;
    break;
  case BasicKind::I32:
    cmpInst = std::make_unique<CmpInst>(CmpOp::NE, value,
                                        new ConstantNumber(Number(0)));
    cond = cmpInst.get();
    _curBlock->pushInstruction(std::move(cmpInst));
    break;
  case BasicKind::F32:
    cmpInst = std::make_unique<CmpInst>(CmpOp::UNE, value,
                                        new ConstantNumber(Number(0.0f)));
    cond = cmpInst.get();
    _curBlock->pushInstruction(std::move(cmpInst));
    break;
  }
  _curBlock->pushInstruction(
      std::make_unique<BranchInst>(cond, _trueBlock, _falseBlock));
}

void GenerateIR::makeInitVal(std::vector<int> &dimensions,
                             std::map<int, AddExp *> &exps, int base,
                             const InitVal *initVal) {
  int offset = 0;
  for (const auto &child : initVal->initValList) {
    if (child->exp == nullptr) {
      int size = 1;
      for (int i = 1; i < dimensions.size(); i++)
        size *= dimensions[i];
      offset = (offset + size - 1) / size * size;
      std::vector<int> newDimensions(dimensions.begin() + 1, dimensions.end());
      makeInitVal(newDimensions, exps, base + offset, child.get());
      offset += size;
    } else {
      exps[base + offset] = child->exp.get();
      offset++;
    }
  }
}

BasicKind GenerateIR::autoTypePromotion(BasicKind type1, BasicKind type2) {
  if (type1 == BasicKind::F32 || type2 == BasicKind::F32) {
    return BasicKind::F32;
  }
  if (type1 == BasicKind::I32 || type2 == BasicKind::I32) {
    return BasicKind::I32;
  }
  return BasicKind::I1;
}

// The return value of this funciton should not be used to make another unique
// pointer or deleted
Value *GenerateIR::typeConversion(Value *value, BasicKind targetType) {
  if (!value->getType()->isBasic()) {
    return value;
  }
  auto type = static_cast<BasicType *>(value->getType())->getBasicKind();
  if (type == targetType) {
    return value;
  }
  if (value->isConstNum()) {
    auto constant = static_cast<ConstantNumber *>(value);
    switch (targetType) {
    case BasicKind::I32:
      return new ConstantNumber(Number(constant->intValue()));
    case BasicKind::I1:
      return new ConstantNumber(constant->intValue() != 0);
    case BasicKind::F32:
      return new ConstantNumber(Number(constant->floatValue()));
    }
  }
  Instruction *inst = nullptr;
  switch (targetType) {
  case BasicKind::I1:
    switch (type) {
    case BasicKind::I1:
      return value;
    case BasicKind::I32:
      inst = new CmpInst(CmpOp::NE, value, new ConstantNumber(Number(0)));
      break;
    case BasicKind::F32:
      inst = new CmpInst(CmpOp::UNE, value, new ConstantNumber(Number(0.0f)));
      break;
    }
    break;
  case BasicKind::I32:
    switch (type) {
    case BasicKind::I1:
      inst = new CastInst(MAKE_I32, CastOp::ZExt, value);
      break;
    case BasicKind::I32:
      return value;
    case BasicKind::F32:
      inst = new CastInst(MAKE_I32, CastOp::FPToSI, value);
      break;
    }
    break;
  case BasicKind::F32:
    switch (type) {
    case BasicKind::I1:
      inst = new CastInst(MAKE_I32, CastOp::ZExt, value);
      _curBlock->pushInstruction(std::unique_ptr<Instruction>(inst));
      inst = new CastInst(MAKE_F32, CastOp::SIToFP, value);
      break;
    case BasicKind::I32:
      inst = new CastInst(MAKE_F32, CastOp::SIToFP, value);
      break;
    case BasicKind::F32:
      return value;
    }
    break;
  }
  if (inst) {
    _curBlock->pushInstruction(std::unique_ptr<Instruction>(inst));
    return inst;
  }
  return value;
}

void GenerateIR::visit(CompUnit &ast) {
  for (const auto &def : ast.declDefList) {
    def->accept(*this);
  }
}

void GenerateIR::visit(DeclDef &ast) {
  if (ast.decl) {
    ast.decl->accept(*this);
    return;
  }
  ast.funcDef->accept(*this);
}

void GenerateIR::visit(Decl &ast) {
  _isConst = ast.isConst;
  _curTypeKind = ast.bType == BType::INT ? BasicKind::I32 : BasicKind::F32;
  for (auto &def : ast.defList) {
    def->accept(*this);
  }
}

void GenerateIR::handleScalarDef(Def &ast) {
  std::string varName = *ast.id;
  if (_symbolTable->isGlobal() || _isConst) {
    auto number = Number(0);
    if (ast.initVal) {
      ast.initVal->accept(*this);
      number = static_cast<ConstantNumber *>(_curVal)->getValue();
    }
    _module->addGlobal(_symbolTable->makeGlobal(
        _isConst, std::make_unique<BasicType>(_curTypeKind), varName, number));
    return;
  }
  auto allocaInst = _symbolTable->makeLocal(
      std::make_unique<BasicType>(_curTypeKind), varName);
  auto rawAllocaInst = allocaInst.get();
  _entryBlock->pushInstruction(std::move(allocaInst));
  if (ast.initVal) {
    ast.initVal->accept(*this);
    auto val = typeConversion(_curVal, _curTypeKind);
    _curBlock->pushInstruction(std::make_unique<StoreInst>(val, rawAllocaInst));
  }
}

void GenerateIR::handleArrayDef(Def &ast) {
  std::string varName = *ast.id;
  vector<int> dimensions;
  for (auto &exp : ast.arrays) {
    exp->accept(*this);
    auto number = static_cast<ConstantNumber *>(_curVal);
    dimensions.push_back(number->intValue());
  }
  if (_symbolTable->isGlobal() || _isConst) {
    std::map<int, AddExp *> exps;
    if (ast.initVal) {
      makeInitVal(dimensions, exps, 0, ast.initVal.get());
    }
    auto reverseDimensions = dimensions;
    std::reverse(reverseDimensions.begin(), reverseDimensions.end());
    std::unique_ptr<Type> type = std::make_unique<BasicType>(_curTypeKind);
    for (auto dim : reverseDimensions) {
      type = std::make_unique<ArrayType>(std::move(type), dim);
    }

    std::map<int, Number> values;
    for (auto &[index, exp] : exps) {
      exp->accept(*this);
      auto result = static_cast<ConstantNumber *>(_curVal);
      values.insert(std::make_pair(index, result->getValue()));
    }

    _module->addGlobal(
        _symbolTable->makeGlobal(_isConst, std::move(type), varName, values));
    return;
  }
  auto allocaInst = _symbolTable->makeLocal(
      std::make_unique<BasicType>(_curTypeKind), varName, dimensions);
  auto rawAllocaInst = allocaInst.get();
  _entryBlock->pushInstruction(std::move(allocaInst));
  if (ast.initVal) {
    std::map<int, AddExp *> exps;
    makeInitVal(dimensions, exps, 0, ast.initVal.get());
    auto castInst =
        std::make_unique<CastInst>(std::make_unique<PointerType>(MAKE_I32),
                                   CastOp::BitCast, rawAllocaInst);
    auto rawCastInst = castInst.get();
    _curBlock->pushInstruction(std::move(castInst));
    int x = 4;
    for (auto dim : dimensions) {
      x *= dim;
    }
    std::vector<Value *> args{rawCastInst, new ConstantNumber(Number(0)),
                              new ConstantNumber(Number(x))};
    // Try memclr maybe (ATTENTION)
    _curBlock->pushInstruction(
        std::make_unique<CallInst>(_symbolTable->getFunction("memset"), args));

    for (auto &[index, exp] : exps) {
      exp->accept(*this);
      auto val = typeConversion(_curVal, _curTypeKind);
      Value *ptr = rawAllocaInst;
      for (int j = 0; j < dimensions.size(); j++) {
        int num = 1;
        for (int k = j + 1; k < dimensions.size(); k++) {
          num *= dimensions[k];
        }
        int idx = index / num % dimensions[j];
        vector<Value *> indices{new ConstantNumber(Number(0)),
                                new ConstantNumber(Number(idx))};
        auto gepInst = std::make_unique<GetElementPtrInst>(ptr, indices);
        ptr = gepInst.get();
        _curBlock->pushInstruction(std::move(gepInst));
      }
      _curBlock->pushInstruction(std::make_unique<StoreInst>(val, ptr));
    }
  }
}

void GenerateIR::visit(Def &ast) {
  if (ast.arrays.empty()) {
    handleScalarDef(ast);
  } else {
    handleArrayDef(ast);
  }
}

// This is used to handle scalar init value, see makeInitVal() also
void GenerateIR::visit(InitVal &ast) {
  if (ast.exp) {
    ast.exp->accept(*this);
  }
}

std::unique_ptr<BasicType> GenerateIR::handleType(BType &type) {
  switch (type) {
  case BType::INT:
    return MAKE_I32;
  case BType::FLOAT:
    return MAKE_F32;
  case BType::VOID:
    return MAKE_VOID;
  default:
    throw std::runtime_error("Unsupported type in handleType");
  }
}

void GenerateIR::visit(FuncDef &ast) {
  _argToAllocaMap.clear();
  auto retType = handleType(ast.returnType);
  auto type = retType.get();
  auto func = _symbolTable->makeFunction(std::move(retType), *ast.id);
  _curFunction = func.get();
  _symbolTable->in();
  auto entry = std::make_unique<BasicBlock>();
  _entryBlock = entry.get();
  _curFunction->pushBlock(std::move(entry));
  auto ret = std::make_unique<BasicBlock>();
  _retBlock = ret.get();

  if (type->getBasicKind() == BasicKind::VOID) {
    _retVal = nullptr;
    _retBlock->pushInstruction(std::make_unique<RetInst>());
  } else {
    auto allocaInst =
        std::make_unique<AllocaInst>(_curFunction->getType()->clone());
    _retVal = allocaInst.get();
    _entryBlock->pushInstruction(std::move(allocaInst));
    auto loadInst = std::make_unique<LoadInst>(_retVal);
    auto rawLoadInst = loadInst.get();
    _retBlock->pushInstruction(std::move(loadInst));
    _retBlock->pushInstruction(std::make_unique<RetInst>(rawLoadInst));
  }

  auto block = std::make_unique<BasicBlock>();
  _curBlock = block.get();
  _curFunction->pushBlock(std::move(block));

  for (auto &param : ast.funcFParamList) {
    param->accept(*this);
  }

  // _hasBranch = false;
  ast.block->accept(*this);

  // Maybe not the best way to handle default return value (ATTENTION)
  if (type->getBasicKind() != BasicKind::VOID) {
    Constant *retVal;
    if (type->getBasicKind() == BasicKind::I32) {
      retVal = new ConstantNumber(Number(0));
    } else {
      retVal = new ConstantNumber(Number(0.0f));
    }
    _entryBlock->pushInstruction(std::make_unique<StoreInst>(retVal, _retVal));
  }

  _curFunction->pushBlock(std::move(ret));
  _entryBlock->pushInstruction(std::make_unique<BranchInst>(
      static_cast<BasicBlock *>(_curFunction->getBlock(1))));
  _module->addFunction(std::move(func));
  _symbolTable->out();
}

void GenerateIR::visit(Call &ast) {
  auto func = _symbolTable->getFunction(*ast.id);
  std::vector<Value *> args;
  for (auto &exp : ast.funcCParamList) {
    exp->accept(*this);
    auto arg = func->getArg(args.size());
    auto param = _curVal;
    if (arg->getType()->isBasic()) {
      auto type = static_cast<BasicType *>(arg->getType());
      BasicKind typeKind;
      if (type->getBasicKind() == BasicKind::F32) {
        typeKind = BasicKind::F32;
      } else {
        typeKind = BasicKind::I32;
      }
      param = typeConversion(param, typeKind);
    }
    args.push_back(param);
  }
  auto callInst = std::make_unique<CallInst>(func, args);
  _curVal = callInst.get();
  _curBlock->pushInstruction(std::move(callInst));
}

void GenerateIR::visit(FuncFParam &ast) {
  std::unique_ptr<Type> type = handleType(ast.bType);
  if (ast.isArray) {
    vector<int> dimensions;
    for (auto &exp : ast.arrays) {
      exp->accept(*this);
      auto number = static_cast<ConstantNumber *>(_curVal);
      dimensions.push_back(number->intValue());
    }
    auto reverseDimensions = dimensions;
    std::reverse(reverseDimensions.begin(), reverseDimensions.end());
    for (int i : reverseDimensions) {
      type = std::make_unique<ArrayType>(std::move(type), i);
    }
    type = std::make_unique<PointerType>(std::move(type));
  }

  auto arg = _symbolTable->makeArgument(std::move(type), *ast.id);
  auto rawArg = arg.get();
  _curFunction->addArg(std::move(arg));

  // Use the SSA name of the argument here to avoid conflict (ATTENTION)
  auto allocaInst =
      _symbolTable->makeLocal(rawArg->getType()->clone(), rawArg->getName());
  auto rawInst = allocaInst.get();
  _entryBlock->pushInstruction(std::move(allocaInst));
  _curBlock->pushInstruction(std::make_unique<StoreInst>(rawArg, rawInst));
  _argToAllocaMap.insert(std::make_pair(rawArg, rawInst));
}

void GenerateIR::visit(Block &ast) {
  _symbolTable->in();
  for (auto &item : ast.blockItemList) {
    // if (_hasBranch) {
    //   _hasBranch = false;
    //   break;
    // }
    item->accept(*this);
    if (item->stmt) {
      bool hasBranch = false;
      switch (item->stmt->sType) {
      case StmtType::CONTINUE:
      case StmtType::BREAK:
      case StmtType::RET:
        hasBranch = true;
        break;
      }
      if (hasBranch) {
        break;
      }
    }
  }
  _symbolTable->out();
}

void GenerateIR::visit(BlockItem &ast) {
  if (ast.decl) {
    ast.decl->accept(*this);
    return;
  }
  if (ast.stmt) {
    ast.stmt->accept(*this);
  }
}

void GenerateIR::handleAssignStmt(Stmt &ast) {
  _isRealLVal = true;
  ast.lVal->accept(*this);
  auto lVal = _curVal;
  auto type = lVal->getType();
  ast.exp->accept(*this);
  auto rVal = _curVal;
  // Theoretically, type should not be a basic type (ATTENTION)
  if (type->isBasic()) {
    rVal = typeConversion(rVal, static_cast<BasicType *>(type)->getBasicKind());
  } else {
    rVal = typeConversion(
        rVal, static_cast<BasicType *>(type->getBaseType())->getBasicKind());
  }
  _curBlock->pushInstruction(std::make_unique<StoreInst>(rVal, lVal));
}

void GenerateIR::visit(ReturnStmt &ast) {
  if (!ast.exp) {
    _curBlock->pushInstruction(std::make_unique<BranchInst>(_retBlock));
    return;
  }

  ast.exp->accept(*this);
  auto retVal = typeConversion(
      _curVal,
      static_cast<BasicType *>(_curFunction->getType())->getBasicKind());
  _curBlock->pushInstruction(std::make_unique<StoreInst>(retVal, _retVal));
  _curBlock->pushInstruction(std::make_unique<BranchInst>(_retBlock));
  // _hasBranch = true;
}

void GenerateIR::visit(LAndExp &ast) {
  if (ast.lAndExp) {
    auto block = std::make_unique<BasicBlock>();
    BasicBlock *rawBlock = block.get();
    _curFunction->insertBlockAfter(_curBlock, std::move(block));

    auto trueBlock = _trueBlock;
    auto falseBlock = _falseBlock;

    _trueBlock = rawBlock;
    ast.lAndExp->accept(*this);
    processCond(_curVal);

    _curBlock = rawBlock;
    _trueBlock = trueBlock;
    _falseBlock = falseBlock;
    // _hasBranch = false;
  }
  ast.eqExp->accept(*this);
  processCond(_curVal);
}

void GenerateIR::visit(LOrExp &ast) {
  if (ast.lOrExp) {
    auto block = std::make_unique<BasicBlock>();
    BasicBlock *rawBlock = block.get();
    _curFunction->insertBlockAfter(_curBlock, std::move(block));

    auto trueBlock = _trueBlock;
    auto falseBlock = _falseBlock;

    _falseBlock = rawBlock;
    ast.lOrExp->accept(*this);
    processCond(_curVal);

    _curBlock = rawBlock;
    _trueBlock = trueBlock;
    _falseBlock = falseBlock;
    // _hasBranch = false;
  }
  ast.lAndExp->accept(*this);
  processCond(_curVal);
}

void GenerateIR::handleIfElseStmt(IfStmt &ast) {
  auto trueBlock = std::make_unique<BasicBlock>();
  auto falseBlock = std::make_unique<BasicBlock>();
  auto ifEndBlock = std::make_unique<BasicBlock>();
  auto rawTrueBlock = trueBlock.get();
  auto rawFalseBlock = falseBlock.get();
  auto rawIfEndBlock = ifEndBlock.get();
  _trueBlock = rawTrueBlock;
  _falseBlock = rawFalseBlock;
  _curFunction->insertBlockAfter(_curBlock, std::move(trueBlock));
  _curFunction->insertBlockAfter(_trueBlock, std::move(falseBlock));
  _curFunction->insertBlockAfter(_falseBlock, std::move(ifEndBlock));

  ast.cond->accept(*this);
  processCond(_curVal);
  _curBlock = rawTrueBlock;
  ast.ifStmt->accept(*this);
  if (!_curBlock->hasTerminator()) {
    _curBlock->pushInstruction(std::make_unique<BranchInst>(rawIfEndBlock));
  }
  _curBlock = rawFalseBlock;
  ast.elseStmt->accept(*this);
  if (!_curBlock->hasTerminator()) {
    _curBlock->pushInstruction(std::make_unique<BranchInst>(rawIfEndBlock));
  }
  _curBlock = rawIfEndBlock;
}

void GenerateIR::visit(IfStmt &ast) {
  if (ast.elseStmt) {
    handleIfElseStmt(ast);
    return;
  }
  auto trueBlock = std::make_unique<BasicBlock>();
  auto falseBlock = std::make_unique<BasicBlock>();
  auto rawTrueBlock = trueBlock.get();
  auto rawFalseBlock = falseBlock.get();
  _trueBlock = rawTrueBlock;
  _falseBlock = rawFalseBlock;
  _curFunction->insertBlockAfter(_curBlock, std::move(trueBlock));
  _curFunction->insertBlockAfter(_trueBlock, std::move(falseBlock));

  ast.cond->accept(*this);
  processCond(_curVal);
  _curBlock = rawTrueBlock;
  ast.ifStmt->accept(*this);
  if (!_curBlock->hasTerminator()) {
    _curBlock->pushInstruction(std::make_unique<BranchInst>(rawFalseBlock));
  }
  _curBlock = rawFalseBlock;
}

void GenerateIR::visit(WhileStmt &ast) {
  // Protect
  auto tmpCondBlock = _condBlock;
  auto tmpBreakBlock = _breakBlock;

  auto condBlock = std::make_unique<BasicBlock>();
  auto loopBlock = std::make_unique<BasicBlock>();
  auto breakBlock = std::make_unique<BasicBlock>();
  auto rawCondBlock = condBlock.get();
  auto rawLoopBlock = loopBlock.get();
  auto rawBreakBlock = breakBlock.get();
  _condBlock = rawCondBlock;
  _breakBlock = rawBreakBlock;
  _curFunction->insertBlockAfter(_curBlock, std::move(condBlock));
  _curFunction->insertBlockAfter(_condBlock, std::move(loopBlock));
  _curFunction->insertBlockAfter(rawLoopBlock, std::move(breakBlock));

  if (!_curBlock->hasTerminator()) {
    _curBlock->pushInstruction(std::make_unique<BranchInst>(rawCondBlock));
  }
  _curBlock = rawCondBlock;
  _trueBlock = rawLoopBlock;
  _falseBlock = rawBreakBlock;

  ast.cond->accept(*this);
  processCond(_curVal);
  _curBlock = rawLoopBlock;
  ast.stmt->accept(*this);
  if (!_curBlock->hasTerminator()) {
    _curBlock->pushInstruction(std::make_unique<BranchInst>(rawCondBlock));
  }

  _curBlock = rawBreakBlock;
  _condBlock = tmpCondBlock;
  _breakBlock = tmpBreakBlock;
}

void GenerateIR::visit(Stmt &ast) {
  switch (ast.sType) {
  case StmtType::SEMI:
    return;
  case StmtType::ASGN:
    handleAssignStmt(ast);
    return;
  case StmtType::EXP:
    ast.exp->accept(*this);
    return;
  case StmtType::CONTINUE:
    _curBlock->pushInstruction(std::make_unique<BranchInst>(_condBlock));
    // _hasBranch = true;
    return;
  case StmtType::BREAK:
    _curBlock->pushInstruction(std::make_unique<BranchInst>(_breakBlock));
    // _hasBranch = true;
    return;
  case StmtType::RET:
    ast.returnStmt->accept(*this);
    return;
  case StmtType::BLK:
    ast.block->accept(*this);
    return;
  case StmtType::IF:
    ast.ifStmt->accept(*this);
    return;
  case StmtType::WHILE:
    ast.whileStmtAST->accept(*this);
    return;
  }
}

void GenerateIR::visit(RelExp &ast) {
  if (!ast.relExp) {
    ast.addExp->accept(*this);
    return;
  }
  ast.relExp->accept(*this);
  auto val1 = _curVal;
  ast.addExp->accept(*this);
  auto val2 = _curVal;

  auto type1 = static_cast<BasicType *>(val1->getType())->getBasicKind();
  auto type2 = static_cast<BasicType *>(val2->getType())->getBasicKind();
  auto targetType = autoTypePromotion(type1, type2);
  val1 = typeConversion(val1, targetType);
  val2 = typeConversion(val2, targetType);
  CmpOp op;
  switch (ast.op) {
  case RelOp::LT:
    switch (targetType) {
    case BasicKind::I32:
      op = CmpOp::SLT;
      break;
    case BasicKind::F32:
      op = CmpOp::OLT;
      break;
    }
    break;
  case RelOp::GT:
    switch (targetType) {
    case BasicKind::I32:
      op = CmpOp::SGT;
      break;
    case BasicKind::F32:
      op = CmpOp::OGT;
      break;
    }
    break;
  case RelOp::LTE:
    switch (targetType) {
    case BasicKind::I32:
      op = CmpOp::SLE;
      break;
    case BasicKind::F32:
      op = CmpOp::OLE;
      break;
    }
    break;
  case RelOp::GTE:
    switch (targetType) {
    case BasicKind::I32:
      op = CmpOp::SGE;
      break;
    case BasicKind::F32:
      op = CmpOp::OGE;
      break;
    }
    break;
  }
  auto cmpInst = std::make_unique<CmpInst>(op, val1, val2);
  _curVal = cmpInst.get();
  _curBlock->pushInstruction(std::move(cmpInst));
}

// TODO: Need more type check and exception handling
void GenerateIR::visit(EqExp &ast) {
  if (!ast.eqExp) {
    ast.relExp->accept(*this);
    return;
  }
  ast.eqExp->accept(*this);
  auto val1 = _curVal;
  ast.relExp->accept(*this);
  auto val2 = _curVal;

  auto type1 = static_cast<BasicType *>(val1->getType())->getBasicKind();
  auto type2 = static_cast<BasicType *>(val2->getType())->getBasicKind();
  auto targetType = autoTypePromotion(type1, type2);
  val1 = typeConversion(val1, targetType);
  val2 = typeConversion(val2, targetType);

  CmpOp op;
  switch (ast.op) {
  case EqOp::EQ:
    switch (targetType) {
    case BasicKind::I32:
      op = CmpOp::EQ;
      break;
    case BasicKind::F32:
      op = CmpOp::OEQ;
      break;
    }
    break;
  case EqOp::NEQ:
    switch (targetType) {
    case BasicKind::I32:
      op = CmpOp::NE;
      break;
    case BasicKind::F32:
      op = CmpOp::UNE;
      break;
    }
    break;
  }
  auto cmpInst = std::make_unique<CmpInst>(op, val1, val2);
  _curVal = cmpInst.get();
  _curBlock->pushInstruction(std::move(cmpInst));
}

// TODO: Need more type check and exception handling
void GenerateIR::visit(AddExp &ast) {
  if (ast.addExp == nullptr) {
    ast.mulExp->accept(*this);
    return;
  }
  ast.addExp->accept(*this);
  auto val1 = _curVal;
  ast.mulExp->accept(*this);
  auto val2 = _curVal;

  if (val1->isConstNum() && val2->isConstNum()) {
    auto number1 = static_cast<ConstantNumber *>(val1);
    auto number2 = static_cast<ConstantNumber *>(val2);
    switch (ast.op) {
    case AddOp::ADD:
      _curVal = new ConstantNumber(*number1 + *number2);
      break;
    case AddOp::MINUS:
      _curVal = new ConstantNumber(*number1 - *number2);
      break;
    }
    return;
  }
  auto type1 = static_cast<BasicType *>(val1->getType())->getBasicKind();
  auto type2 = static_cast<BasicType *>(val2->getType())->getBasicKind();
  auto targetType = autoTypePromotion(type1, type2);
  val1 = typeConversion(val1, targetType);
  val2 = typeConversion(val2, targetType);
  BinaryOp op;
  switch (ast.op) {
  case AddOp::ADD:
    switch (targetType) {
    case BasicKind::I32:
      op = BinaryOp::ADD;
      break;
    case BasicKind::F32:
      op = BinaryOp::FADD;
      break;
    }
    break;
  case AddOp::MINUS:
    switch (targetType) {
    case BasicKind::I32:
      op = BinaryOp::SUB;
      break;
    case BasicKind::F32:
      op = BinaryOp::FSUB;
      break;
    }
    break;
  }
  auto inst = new BinaryInst(op, val1, val2);
  _curBlock->pushInstruction(std::unique_ptr<Instruction>(inst));
  _curVal = inst;
}

// TODO: Need more type check and exception handling
void GenerateIR::visit(MulExp &ast) {
  if (ast.mulExp == nullptr) {
    ast.unaryExp->accept(*this);
    return;
  }
  ast.mulExp->accept(*this);
  auto val1 = _curVal;
  ast.unaryExp->accept(*this);
  auto val2 = _curVal;

  if (val1->isConstNum() && val2->isConstNum()) {
    auto number1 = static_cast<ConstantNumber *>(val1);
    auto number2 = static_cast<ConstantNumber *>(val2);
    switch (ast.op) {
    case MulOp::MUL:
      _curVal = new ConstantNumber((*number1) * (*number2));
      break;
    case MulOp::DIV:
      _curVal = new ConstantNumber((*number1) / (*number2));
      break;
    case MulOp::MOD:
      _curVal = new ConstantNumber((*number1) % (*number2));
      break;
    }
    return;
  }
  auto type1 = static_cast<BasicType *>(val1->getType())->getBasicKind();
  auto type2 = static_cast<BasicType *>(val2->getType())->getBasicKind();
  auto targetType = autoTypePromotion(type1, type2);
  val1 = typeConversion(val1, targetType);
  val2 = typeConversion(val2, targetType);
  BinaryOp op;
  switch (ast.op) {
  case MulOp::MUL:
    switch (targetType) {
    case BasicKind::I32:
      op = BinaryOp::MUL;
      break;
    case BasicKind::F32:
      op = BinaryOp::FMUL;
      break;
    }
    break;
  case MulOp::DIV:
    switch (targetType) {
    case BasicKind::I32:
      op = BinaryOp::SDIV;
      break;
    case BasicKind::F32:
      op = BinaryOp::FDIV;
      break;
    }
    break;
  case MulOp::MOD:
    switch (targetType) {
    case BasicKind::I32:
      op = BinaryOp::SREM;
      break;
    }
    break;
  }
  auto inst = new BinaryInst(op, val1, val2);
  _curBlock->pushInstruction(std::unique_ptr<Instruction>(inst));
  _curVal = inst;
}

// TODO: Need more type check and exception handling
void GenerateIR::visit(UnaryExp &ast) {
  if (ast.primaryExp) {
    ast.primaryExp->accept(*this);
    return;
  }
  if (ast.call) {
    ast.call->accept(*this);
    return;
  }
  ast.unaryExp->accept(*this);
  Value *val = _curVal;
  if (!val->getType()->isBasic()) {
    throw std::runtime_error("Not a basic type in visit(UnaryExp &ast)");
  }
  if (val->isConstNum()) {
    auto number = static_cast<ConstantNumber *>(val);
    switch (ast.op) {
    case UnaryOp::ADD:
      _curVal = number;
      return;
    case UnaryOp::MINUS:
      _curVal = new ConstantNumber(-(*number));
      return;
    case UnaryOp::NOT:
      _curVal = new ConstantNumber(!(*number));
      return;
    }
  }
  auto type = static_cast<BasicType *>(val->getType())->getBasicKind();
  Instruction *inst;
  switch (ast.op) {
  case UnaryOp::ADD:
    _curVal = val;
    return;
  case UnaryOp::MINUS:
    switch (type) {
    case BasicKind::I1:
      inst = new CastInst(MAKE_I32, CastOp::SExt, val);
      break;
    case BasicKind::I32:
      inst = new BinaryInst(BinaryOp::SUB, new ConstantNumber(Number(0)), val);
      break;
    case BasicKind::F32:
      inst =
          new BinaryInst(BinaryOp::FSUB, new ConstantNumber(Number(0.0f)), val);
      break;
    }
    break;
  case UnaryOp::NOT:
    switch (type) {
    case BasicKind::I1:
      inst = new BinaryInst(BinaryOp::XOR, val, new ConstantNumber(true));
      break;
    case BasicKind::I32:
      inst = new CmpInst(CmpOp::EQ, val, new ConstantNumber(Number(0)));
      break;
    case BasicKind::F32:
      inst = new CmpInst(CmpOp::OEQ, val, new ConstantNumber(Number(0.0f)));
      break;
    }
    break;
  }
  _curBlock->pushInstruction(std::unique_ptr<Instruction>(inst));
  _curVal = inst;
}

void GenerateIR::visit(PrimaryExp &ast) {
  if (ast.exp) {
    ast.exp->accept(*this);
    return;
  }
  if (ast.lval) {
    ast.lval->accept(*this);
    return;
  }
  if (ast.number) {
    ast.number->accept(*this);
    return;
  }
}

void GenerateIR::handleScalarVar(LVal &ast) {
  Value *ptr = _symbolTable->getData(*ast.id);
  if (ptr->isArg()) {
    ptr = _argToAllocaMap[static_cast<Argument *>(ptr)];
  }
  auto type = ptr->getType();
  if (ptr->isGlobal()) {
    auto global = static_cast<GlobalVariable *>(ptr);
    if (global->isConst() && global->isSingle()) {
      _curVal = global->getValue();
      return;
    }
  } else {
    type = type->getBaseType();
  }
  if (type->isArray()) {
    auto arrType = static_cast<ArrayType *>(type);
    // std::vector<Value *> indices(arrType->getDimensions().size(),
    //                              new ConstantNumber(Number(0)));
    std::vector<Value *> indices(2,
                             new ConstantNumber(Number(0)));
    auto gepInst = std::make_unique<GetElementPtrInst>(ptr, indices);
    _curVal = gepInst.get();
    _curBlock->pushInstruction(std::move(gepInst));
    return;
  }
  auto loadInst = std::make_unique<LoadInst>(ptr);
  _curVal = loadInst.get();
  _curBlock->pushInstruction(std::move(loadInst));
}

void GenerateIR::handleArrayVar(LVal &ast) {
  auto ptr = _symbolTable->getData(*ast.id);
  bool isFirstDim = false;
  if (ptr->isArg()) {
    isFirstDim = true;
    ptr = _argToAllocaMap[static_cast<Argument *>(ptr)];
    auto loadInst = std::make_unique<LoadInst>(ptr);
    ptr = loadInst.get();
    _curBlock->pushInstruction(std::move(loadInst));
  }
  for (auto &exp : ast.arrays) {
    exp->accept(*this);
    auto index = typeConversion(_curVal, BasicKind::I32);
    std::vector<Value *> indices1{index};
    std::vector<Value *> indices2{new ConstantNumber(Number(0)), index};
    auto gepInst = isFirstDim
                       ? std::make_unique<GetElementPtrInst>(ptr, indices1)
                       : std::make_unique<GetElementPtrInst>(ptr, indices2);
    ptr = gepInst.get();
    _curBlock->pushInstruction(std::move(gepInst));
    isFirstDim = false;
  }
  if (ptr->getType()->getBaseType()->isArray()) {
    _curVal = ptr;
    return;
  }
  auto loadInst = std::make_unique<LoadInst>(ptr);
  _curVal = loadInst.get();
  _curBlock->pushInstruction(std::move(loadInst));
}

void GenerateIR::visit(LVal &ast) {
  if (!_isRealLVal) {
    if (ast.arrays.empty()) {
      handleScalarVar(ast);
    } else {
      handleArrayVar(ast);
    }
    return;
  }
  _isRealLVal = false;
  // Handle real left value
  auto ptr = _symbolTable->getData(*ast.id);
  bool isArg = false;
  if (ptr->isArg()) {
    isArg = true;
    ptr = _argToAllocaMap[static_cast<Argument *>(ptr)];
  }
  if (ast.arrays.empty()) {
    _curVal = ptr;
    return;
  }
  auto type = ptr->getType();
  if (type->isPointer() && type->getBaseType()->isPointer()) {
    auto loadInst = std::make_unique<LoadInst>(ptr);
    ptr = loadInst.get();
    _curBlock->pushInstruction(std::move(loadInst));
  }

  std::vector<Value *> dimensions;
  for (auto &exp : ast.arrays) {
    exp->accept(*this);
    dimensions.push_back(_curVal);
  }
  bool isFirstDim = true;
  for (auto dim : dimensions) {
    std::unique_ptr<Instruction> gepInst;
    if (isArg && isFirstDim) {
      std::vector<Value *> indices{dim};
      gepInst = std::make_unique<GetElementPtrInst>(ptr, indices);
    } else {
      std::vector<Value *> indices{new ConstantNumber(Number(0)), dim};
      gepInst = std::make_unique<GetElementPtrInst>(ptr, indices);
    }
    ptr = gepInst.get();
    _curBlock->pushInstruction(std::move(gepInst));
    isFirstDim = false;
  }
  _curVal = ptr;
}

void GenerateIR::visit(NumberNode &ast) {
  if (ast.isInt) {
    _curVal = new ConstantNumber(Number(ast.intval));
    return;
  }
  _curVal = new ConstantNumber(Number(ast.floatval));
}

#undef MAKE_I32
#undef MAKE_F32
#undef MAKE_VOID
