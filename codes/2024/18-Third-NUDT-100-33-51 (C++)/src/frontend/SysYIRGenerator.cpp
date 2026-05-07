#include "../include/frontend/SysYIRGenerator.h"
#include "../SysYParser.h"
#include "../include/frontend/IR.h"
#include "../include/frontend/IRBuilder.h"
#include <cassert>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

/**
 * @file SysYIRGenerator.cpp
 * @brief 该源文件包含对ANTRL生成的AST树每个节点的访问函数的重载实现
 *
 * 每个访问函数是SysyIRGenenrator的成员函数，其可能包含对子节点访问函数的调用，总体访问顺序为自上而下。
 * 完成所有访问后，最终生成中间IR。IR的生成实现了短路计算以及对非条件表达式的常量传播。
 */

/**
 * @brief 整个SySy编译器使用的命名空间
 */
namespace sysy {

/**
 * @brief AST树根节点CompUnit的访问函数
 *
 * 初始化模块Module，创立全局作用域，并向下访问子节点。
 *
 * @param [in] ctx 指向CompUnit节点的指针
 * @return 指向Module对象的指针pModule
 * @sa Module
 */
auto SysYIRGenerator::visitCompUnit(SysYParser::CompUnitContext *ctx)
    -> std::any {
  auto pModule = new Module();
  assert(pModule);
  module.reset(pModule);

  Helper::initExternalFunction(pModule, &builder);

  pModule->enterNewScope();
  visitChildren(ctx);
  pModule->leaveScope();
  return pModule;
}

/**
 * @brief AST树GlobalConstDecl节点的访问函数
 *
 * 为全局常量声明语句中的常量创建Alloca指令，并存入符号表SymbolTable。
 *
 * @param [in] ctx 指向GlobalConstDecl节点的指针
 * @retval 0 表示正常运行
 * @sa SymbolTable
 */
auto SysYIRGenerator::visitGlobalConstDecl(
    SysYParser::GlobalConstDeclContext *ctx) -> std::any {
  auto constDecl = ctx->constDecl();
  auto type = std::any_cast<Type *>(visitBType(constDecl->bType()));
  for (const auto &constDef : constDecl->constDef()) {
    std::vector<Value *> dims = {};
    auto name = constDef->Ident()->getText();
    auto constExps = constDef->constExp();
    if (!constExps.empty()) {
      for (const auto &constExp : constExps) {
        dims.push_back(std::any_cast<Value *>(visitConstExp(constExp)));
      }
    }

    auto root =
        std::any_cast<ValueTreeNode *>(constDef->constInitVal()->accept(this));
    ValueCounter values;
    Helper::tree2Array(type, root, dims, dims.size(), values, &builder);
    delete root;

    module->createConstVar(name, Type::getPointerType(type), values, dims);
  }
  return 0;
}

/**
 * @brief AST树GlobalVarDecl节点的访问函数
 *
 * 为全局变量声明语句中的变量创建Alloca指令，并存入符号表SymbolTable。
 *
 * @param [in] ctx 指向GlobalVarDecl节点的指针
 * @retval 0 表示正常运行
 * @sa SymbolTable
 */
auto SysYIRGenerator::visitGlobalVarDecl(SysYParser::GlobalVarDeclContext *ctx)
    -> std::any {
  auto varDecl = ctx->varDecl();
  auto type = std::any_cast<Type *>(visitBType(varDecl->bType()));
  for (const auto &varDef : varDecl->varDef()) {
    std::vector<Value *> dims = {};
    auto name = varDef->Ident()->getText();
    auto constExps = varDef->constExp();
    if (!constExps.empty()) {
      for (const auto &constExp : constExps) {
        dims.push_back(std::any_cast<Value *>(visitConstExp(constExp)));
      }
    }

    ValueCounter values;

    if (varDef->initVal() != nullptr) {
      auto root =
          std::any_cast<ValueTreeNode *>(varDef->initVal()->accept(this));
      Helper::tree2Array(type, root, dims, dims.size(), values, &builder);
      delete root;
    }

    module->createGlobalValue(name, Type::getPointerType(type), dims, values);
  }

  return 0;
}

/**
 * @brief AST树VarDecl节点的访问函数
 *
 * 为局部变量声明语句中的变量创建Alloca指令，并存入符号表SymbolTable。
 *
 * @param [in] ctx 指向VarDecl节点的指针
 * @retval 0 表示正常运行
 * @sa SymbolTable
 */
auto SysYIRGenerator::visitVarDecl(SysYParser::VarDeclContext *ctx)
    -> std::any {
  auto type = std::any_cast<Type *>(visitBType(ctx->bType()));
  for (auto varDef : ctx->varDef()) {
    std::vector<Value *> dims = {};
    auto name = varDef->Ident()->getText();
    auto constExps = varDef->constExp();
    if (!constExps.empty()) {
      for (const auto &constExp : constExps) {
        dims.push_back(std::any_cast<Value *>(visitConstExp(constExp)));
      }
    }

    auto alloca =
        builder.createAllocaInst(Type::getPointerType(type), dims, name);

    if (varDef->initVal() != nullptr) {
      ValueCounter values;
      auto root =
          std::any_cast<ValueTreeNode *>(varDef->initVal()->accept(this));
      Helper::tree2Array(type, root, dims, dims.size(), values, &builder);
      delete root;
      if (dims.empty()) {
        builder.createStoreInst(values.getValue(0), alloca);
      } else {
        auto counterNumbers = values.getNumbers();
        auto counterValues = values.getValues();
        unsigned begin = 0;
        for (size_t i = 0; i < counterNumbers.size(); i++) {
          builder.createMemsetInst(
              alloca, ConstantValue::get(static_cast<int>(begin)),
              ConstantValue::get(static_cast<int>(counterNumbers[i])),
              counterValues[i]);
          begin += counterNumbers[i];
        }
      }
    }
    module->addVariable(name, alloca);
  }
  return 0;
}

/**
 * @brief AST树ConstDecl节点的访问函数
 *
 * 为局部常量声明语句中的常量创建Alloca指令，并存入符号表SymbolTable。
 *
 * @param [in] ctx 指向ConstDecl节点的指针
 * @retval 0 表示正常运行
 * @sa SymbolTable
 */
auto SysYIRGenerator::visitConstDecl(SysYParser::ConstDeclContext *ctx)
    -> std::any {
  auto type = std::any_cast<Type *>(visitBType(ctx->bType()));
  for (auto constDef : ctx->constDef()) {
    std::vector<Value *> dims = {};
    auto name = constDef->Ident()->getText();
    auto constExps = constDef->constExp();
    if (!constExps.empty()) {
      for (const auto &constExp : constExps) {
        dims.push_back(std::any_cast<Value *>(visitConstExp(constExp)));
      }
    }

    auto root =
        std::any_cast<ValueTreeNode *>(constDef->constInitVal()->accept(this));
    ValueCounter values;
    Helper::tree2Array(type, root, dims, dims.size(), values, &builder);
    delete root;

    module->createConstVar(name, Type::getPointerType(type), values, dims);
  }
  return 0;
}

/**
 * @brief AST树ScalarInitValue节点的访问函数
 *
 * 将单个非常量值存入单节点树，该树可被tree2array函数解析并转化成ValueCounter的形式。
 *
 * @param [in] ctx 指向ScalarInitValue节点的指针
 * @return 树的根节点root
 * @sa Helper::tree2Array(Type *type, ValueTreeNode *root, std::vector<Value *>
 * &dims, unsigned numDims, ValueCounter &result, IRBuilder *builder),
 * ValueCounter
 */
auto SysYIRGenerator::visitScalarInitValue(
    SysYParser::ScalarInitValueContext *ctx) -> std::any {
  auto alloca = std::any_cast<Value *>(visitExp(ctx->exp()));
  auto result = new ValueTreeNode();
  result->setValue(alloca);

  return result;
}

/**
 * @brief AST树ArrayInitValue节点的访问函数
 *
 * 将数组的非常量初始值以树的形式存储，该树可被tree2array函数解析并转化成ValueCounter的形式。
 *
 * @param [in] ctx 指向ArrayInitValue节点的指针
 * @return 树的根节点root
 * @sa Helper::tree2Array(Type *type, ValueTreeNode *root, std::vector<Value *>
 * &dims, unsigned numDims, ValueCounter &result, IRBuilder *builder),
 * ValueCounter
 */
auto SysYIRGenerator::visitArrayInitValue(
    SysYParser::ArrayInitValueContext *ctx) -> std::any {
  std::vector<ValueTreeNode *> children;
  for (const auto &initVal : ctx->initVal()) {
    children.push_back(std::any_cast<ValueTreeNode *>(initVal->accept(this)));
  }

  auto root = new ValueTreeNode();
  root->addChildren(children);

  return root;
}

/**
 * @brief AST树ConstScalarInitValue节点的访问函数
 *
 * 将单个常量值存入单节点树，该树可被tree2array函数解析并转化成ValueCounter的形式。
 *
 * @param [in] ctx 指向ConstScalarInitValue节点的指针
 * @return 树的根节点root
 * @sa Helper::tree2Array(Type *type, ValueTreeNode *root, std::vector<Value *>
 * &dims, unsigned numDims, ValueCounter &result, IRBuilder *builder),
 * ValueCounter
 */
auto SysYIRGenerator::visitConstScalarInitValue(
    SysYParser::ConstScalarInitValueContext *ctx) -> std::any {
  auto alloca = std::any_cast<Value *>(visitConstExp(ctx->constExp()));
  auto root = new ValueTreeNode();
  root->setValue(alloca);

  return root;
}

/**
 * @brief AST树ConstArrayInitValue节点的访问函数
 *
 * 将数组的常量初始值以树的形式存储，该树可被tree2array函数解析并转化成ValueCounter的形式。
 *
 * @param [in] ctx 指向ConstArrayInitValue节点的指针
 * @return 树的根节点root
 * @sa Helper::tree2Array(Type *type, ValueTreeNode *root, std::vector<Value *>
 * &dims, unsigned numDims, ValueCounter &result, IRBuilder *builder),
 * ValueCounter
 */
auto SysYIRGenerator::visitConstArrayInitValue(
    SysYParser::ConstArrayInitValueContext *ctx) -> std::any {
  std::vector<ValueTreeNode *> children;
  for (const auto &constInitVal : ctx->constInitVal()) {
    children.push_back(
        std::any_cast<ValueTreeNode *>(constInitVal->accept(this)));
  }

  auto root = new ValueTreeNode();
  root->addChildren(children);

  return root;
}

/**
 * @brief AST树BType节点的访问函数
 *
 * 返回该Btype节点所代表的类型(int/float)。
 *
 * @param [in] ctx 指向BType节点的指针
 * @return 节点代表的类型
 * @sa Type
 */
auto SysYIRGenerator::visitBType(SysYParser::BTypeContext *ctx) -> std::any {
  return ctx->INT() != nullptr ? Type::getIntType() : Type::getFloatType();
}

/**
 * @brief AST树FuncDef节点的访问函数
 *
 * 创建Function对象，并为其形参列表创建Alloca指令，录入符号表SymbolTable，
 * 创建局部作用域并访问其子节点。
 *
 * @param [in] ctx 指向FuncDef节点的指针
 * @return 节点代表的类型
 * @sa Function, SymbolTable
 */
auto SysYIRGenerator::visitFuncDef(SysYParser::FuncDefContext *ctx)
    -> std::any {
  module->enterNewScope();

  auto name = ctx->Ident()->getText();
  std::vector<Type *> paramTypes;
  std::vector<std::string> paramNames;
  std::vector<std::vector<Value *>> paramDims;

  if (ctx->funcFParams() != nullptr) {
    auto params = ctx->funcFParams()->funcFParam();
    for (const auto &param : params) {
      paramTypes.push_back(std::any_cast<Type *>(visitBType(param->bType())));
      paramNames.push_back(param->Ident()->getText());
      std::vector<Value *> dims = {};
      if (param->funcFParamIndices() != nullptr) {
        dims.push_back(ConstantValue::get(-1)); // 第一个维度不确定
        for (const auto &exp : param->funcFParamIndices()->exp()) {
          dims.push_back(std::any_cast<Value *>(visitExp(exp)));
        }
      }
      paramDims.emplace_back(dims);
    }
  }

  Type *returnType = std::any_cast<Type *>(visitFuncType(ctx->funcType()));
  auto funcType = Type::getFunctionType(returnType, paramTypes);
  auto function = module->createFunction(name, funcType);
  auto entry = function->getEntryBlock();
  builder.setPosition(entry, entry->end());

  for (size_t i = 0; i < paramTypes.size(); ++i) {
    auto alloca = builder.createAllocaInst(Type::getPointerType(paramTypes[i]),
                                           paramDims[i], paramNames[i]);
    entry->insertArgument(alloca);
    module->addVariable(paramNames[i], alloca);
  }

  for (auto item : ctx->block()->blockItem()) {
    visitBlockItem(item);
  }

  module->leaveScope();

  return 0;
}

/**
 * @brief AST树FuncType节点的访问函数
 *
 * 返回FuncType节点代表的函数返回值类型(void/int/float)。
 *
 * @param [in] ctx 指向FuncDef节点的指针
 * @return 函数返回值类型
 * @sa Type
 */
auto SysYIRGenerator::visitFuncType(SysYParser::FuncTypeContext *ctx)
    -> std::any {
  if (ctx->INT() != nullptr) {
    return Type::getIntType();
  }
  if (ctx->FLOAT() != nullptr) {
    return Type::getFloatType();
  }
  return Type::getVoidType();
}

/**
 * @brief AST树Block节点的访问函数
 *
 * 创建局部作用域并访问子节点。
 *
 * @param [in] ctx 指向Block节点的指针
 * @retval 0 表示运行正常
 */
auto SysYIRGenerator::visitBlock(SysYParser::BlockContext *ctx) -> std::any {
  module->enterNewScope();

  for (auto item : ctx->blockItem()) {
    visitBlockItem(item);
  }

  module->leaveScope();
  return 0;
}

/**
 * @brief AST树AssignStmt节点的访问函数
 *
 * 创建assign指令。若被赋值的变量与要赋予的值类型不一致，且要赋予的值不是常量表达式，则额外创建IToFInst或FtoIInst指令，
 * 否则创建类型转换后的字面值。
 *
 * @param [in] ctx 指向AssignStmt节点的指针
 * @retval 0 表示运行正常
 */
auto SysYIRGenerator::visitAssignStmt(SysYParser::AssignStmtContext *ctx)
    -> std::any {
  auto lVal = ctx->lVal();
  auto name = lVal->Ident()->getText();
  std::vector<Value *> dims;
  for (const auto &exp : lVal->exp()) {
    dims.push_back(std::any_cast<Value *>(visitExp(exp)));
  }

  auto variable = module->getVariable(name);
  auto value = std::any_cast<Value *>(visitExp(ctx->exp()));
  auto variableType =
      dynamic_cast<PointerType *>(variable->getType())->getBaseType();

  if (variableType != value->getType()) {
    auto constValue = dynamic_cast<ConstantValue *>(value);
    if (constValue != nullptr) {
      if (variableType == Type::getFloatType()) {
        value = ConstantValue::get(static_cast<float>(constValue->getInt()));
      } else {
        value = ConstantValue::get(static_cast<int>(constValue->getFloat()));
      }
    } else {
      if (variableType == Type::getFloatType()) {
        value = builder.createIToFInst(value);
      } else {
        value = builder.createFtoIInst(value);
      }
    }
  }
  builder.createStoreInst(value, variable, dims, variable->getName());

  return 0;
}

/**
 * @brief AST树IfStmt节点的访问函数
 *
 * 为if ...
 * 语句创建thenBlock、exitBlock基本块；为if...else...语句创建thenBlock、elseBlock、exitBlock基本块。
 * 创建局部作用域并为thenBlock、elseBlock生成IR，生成基本块之间的控制流图关系以及基本块的跳转标签。
 *
 * @param [in] ctx 指向IfStmt节点的指针
 * @retval 0 表示运行正常
 */
auto SysYIRGenerator::visitIfStmt(SysYParser::IfStmtContext *ctx) -> std::any {
  std::stringstream ss;
  auto function = builder.getBasicBlock()->getParent();

  // 标签需要滞后确定
  auto thenBlock = new BasicBlock(function);
  auto exitBlock = new BasicBlock(function);

  if (ctx->stmt().size() > 1) {
    auto elseBlock = new BasicBlock(function);

    builder.pushTrueBlock(thenBlock);
    builder.pushFalseBlock(elseBlock);
    visitCond(ctx->cond());
    builder.popTrueBlock();
    builder.popFalseBlock();

    ss << "L" << builder.getLabelIndex();
    thenBlock->setName(ss.str());
    ss.str("");
    function->addBasicBlock(thenBlock);
    builder.setPosition(thenBlock, thenBlock->end());

    auto block = dynamic_cast<SysYParser::BlockStmtContext *>(ctx->stmt(0));
    if (block != nullptr) {
      visitBlockStmt(block);
    } else {
      module->enterNewScope();
      ctx->stmt(0)->accept(this);
      module->leaveScope();
    }
    builder.createUncondBrInst(exitBlock, {});
    BasicBlock::conectBlocks(builder.getBasicBlock(), exitBlock);

    ss << "L" << builder.getLabelIndex();
    elseBlock->setName(ss.str());
    ss.str("");
    function->addBasicBlock(elseBlock);
    builder.setPosition(elseBlock, elseBlock->end());

    block = dynamic_cast<SysYParser::BlockStmtContext *>(ctx->stmt(1));
    if (block != nullptr) {
      visitBlockStmt(block);
    } else {
      module->enterNewScope();
      ctx->stmt(1)->accept(this);
      module->leaveScope();
    }
    BasicBlock::conectBlocks(builder.getBasicBlock(), exitBlock);

    ss << "L" << builder.getLabelIndex();
    exitBlock->setName(ss.str());
    ss.str("");
    function->addBasicBlock(exitBlock);
    builder.setPosition(exitBlock, exitBlock->end());

  } else {
    builder.pushTrueBlock(thenBlock);
    builder.pushFalseBlock(exitBlock);
    visitCond(ctx->cond());
    builder.popTrueBlock();
    builder.popFalseBlock();

    ss << "L" << builder.getLabelIndex();
    thenBlock->setName(ss.str());
    ss.str("");
    function->addBasicBlock(thenBlock);
    builder.setPosition(thenBlock, thenBlock->end());

    auto block = dynamic_cast<SysYParser::BlockStmtContext *>(ctx->stmt(0));
    if (block != nullptr) {
      visitBlockStmt(block);
    } else {
      module->enterNewScope();
      ctx->stmt(0)->accept(this);
      module->leaveScope();
    }
    BasicBlock::conectBlocks(builder.getBasicBlock(), exitBlock);

    ss << "L" << builder.getLabelIndex();
    exitBlock->setName(ss.str());
    ss.str("");
    function->addBasicBlock(exitBlock);
    builder.setPosition(exitBlock, exitBlock->end());
  }
  return 0;
}

/**
 * @brief AST树WhileStmt节点的访问函数
 *
 * 为while语句创建headBlock、thenBlock、elseBlock、exitBlock基本块。
 * 创建局部作用域并为headBlock、thenBlock、elseBlock生成IR，生成基本块之间的控制流图关系以及基本块的跳转标签。
 *
 * @param [in] ctx 指向WhileStmt节点的指针
 * @retval 0 表示运行正常
 */
auto SysYIRGenerator::visitWhileStmt(SysYParser::WhileStmtContext *ctx)
    -> std::any {
  std::stringstream ss;
  auto curBlock = builder.getBasicBlock();
  auto function = curBlock->getParent();

  ss << "L" << builder.getLabelIndex();
  BasicBlock *headBlock = function->addBasicBlock(ss.str());
  ss.str("");
  BasicBlock::conectBlocks(curBlock, headBlock);
  builder.setPosition(headBlock, headBlock->end());

  // 标签需要滞后确定
  auto *thenBlock = new BasicBlock(function);
  auto *exitBlock = new BasicBlock(function);

  builder.pushTrueBlock(thenBlock);
  builder.pushFalseBlock(exitBlock);
  visitCond(ctx->cond());
  builder.popTrueBlock();
  builder.popFalseBlock();

  ss << "L" << builder.getLabelIndex();
  thenBlock->setName(ss.str());
  ss.str("");
  function->addBasicBlock(thenBlock);
  builder.setPosition(thenBlock, thenBlock->end());

  builder.pushBreakBlock(exitBlock);
  builder.pushContinueBlock(headBlock);
  auto block = dynamic_cast<SysYParser::BlockStmtContext *>(ctx->stmt());
  if (block != nullptr) {
    visitBlockStmt(block);
  } else {
    module->enterNewScope();
    ctx->stmt()->accept(this);
    module->leaveScope();
  }
  builder.createUncondBrInst(headBlock, {});
  BasicBlock::conectBlocks(builder.getBasicBlock(), headBlock);
  builder.popBreakBlock();
  builder.popContinueBlock();

  ss << "L" << builder.getLabelIndex();
  exitBlock->setName(ss.str());
  ss.str("");
  function->addBasicBlock(exitBlock);
  builder.setPosition(exitBlock, exitBlock->end());

  return 0;
}

/**
 * @brief AST树BreakStmt节点的访问函数
 *
 * 生成跳转到exitBlock开头的无条件跳转指令。
 *
 * @param [in] ctx 指向BreakStmt节点的指针
 * @retval 0 表示运行正常
 */
auto SysYIRGenerator::visitBreakStmt(SysYParser::BreakStmtContext *ctx)
    -> std::any {
  auto block = builder.getBreakBlock();
  builder.createUncondBrInst(block, {});
  BasicBlock::conectBlocks(builder.getBasicBlock(), block);
  return 0;
}

/**
 * @brief AST树ContinueStmt节点的访问函数
 *
 * 生成跳转到headBlock开头的无条件跳转指令。
 *
 * @param [in] ctx 指向ContinueStmt节点的指针
 * @retval 0 表示运行正常
 */
auto SysYIRGenerator::visitContinueStmt(SysYParser::ContinueStmtContext *ctx)
    -> std::any {
  auto block = builder.getContinueBlock();
  builder.createUncondBrInst(block, {});
  BasicBlock::conectBlocks(builder.getBasicBlock(), block);
  return 0;
}

/**
 * @brief AST树ReturnStmt节点的访问函数
 *
 * 生成Return指令。若函数返回值类型与要返回的值的类型不一致，且要返回的值不是常量表达式，则额外生成IToFInst或
 * FtoIInst指令，否则创建类型转换后的字面值。
 *
 * @param [in] ctx 指向ReturnStmt节点的指针
 * @retval 0 表示运行正常
 */
auto SysYIRGenerator::visitReturnStmt(SysYParser::ReturnStmtContext *ctx)
    -> std::any {
  Value *value = nullptr;
  if (ctx->exp() != nullptr) {
    value = std::any_cast<Value *>(visitExp(ctx->exp()));
  }

  auto returnType = builder.getBasicBlock()->getParent()->getReturnType();
  if (value != nullptr && returnType != value->getType()) {
    auto constValue = dynamic_cast<ConstantValue *>(value);
    if (constValue != nullptr) {
      if (returnType == Type::getFloatType()) {
        value = ConstantValue::get(static_cast<float>(constValue->getInt()));
      } else {
        value = ConstantValue::get(static_cast<int>(constValue->getFloat()));
      }
    } else {
      if (returnType == Type::getFloatType()) {
        value = builder.createIToFInst(value);
      } else {
        value = builder.createFtoIInst(value);
      }
    }
  }
  builder.createReturnInst(value);

  return 0;
}

/**
 * @brief AST树LVal节点的访问函数
 *
 * 若左值为一个非常量变量的值，则生成Load指令并返回该指令；若为一个
 * 常量变量的值，则直接返回对应的字面值。特别地，若该左值用于赋予全局变量初值，则
 * 该左值一定属于一个全局变量。无论该左值是否属于一个常量全局变量，都返回对应的字面值，
 * 以保证全局变量初值编译时确定。
 *
 * @param [in] ctx 指向LVal节点的指针
 * @return 左值
 */
auto SysYIRGenerator::visitLVal(SysYParser::LValContext *ctx) -> std::any {
  auto name = ctx->Ident()->getText();
  auto variable = module->getVariable(name);

  Value *value;
  std::vector<Value *> indices;
  for (const auto &exp : ctx->exp()) {
    indices.push_back(std::any_cast<Value *>(visitExp(exp)));
  }

  bool indicesConstant = true;
  for (const auto &index : indices) {
    if (dynamic_cast<ConstantValue *>(index) == nullptr) {
      indicesConstant = false;
      break;
    }
  }

  auto constVar = dynamic_cast<ConstantVariable *>(variable);
  auto globalVar = dynamic_cast<GlobalValue *>(variable);
  auto localVar = dynamic_cast<AllocaInst *>(variable);
  if (constVar != nullptr && indicesConstant) {
    value = constVar->getByIndices(indices);
  } else if (module->isInGlobalArea() && (globalVar != nullptr)) {
    assert(indicesConstant);
    value = globalVar->getByIndices(indices);
  } else {
    if ((globalVar != nullptr && globalVar->getNumDims() > indices.size()) ||
        (localVar != nullptr && localVar->getNumDims() > indices.size()) ||
        (constVar != nullptr && constVar->getNumDims() > indices.size())) {
      // value = builder.createLaInst(variable, indices);
      auto getArrayInst =
          builder.createGetSubArray(dynamic_cast<LVal *>(variable), indices);
      value = getArrayInst->getChildArray();
    } else {
      value = builder.createLoadInst(variable, indices);
    }
  }

  return value;
}

/**
 * @brief AST树PrimaryExp节点的访问函数
 *
 * 调用子节点的访问函数并返回其返回值。
 *
 * @param [in] ctx 指向PrimaryExp节点的指针
 * @return 原始表达式的值
 */
auto SysYIRGenerator::visitPrimaryExp(SysYParser::PrimaryExpContext *ctx)
    -> std::any {
  if (ctx->exp() != nullptr) {
    return visitExp(ctx->exp());
  }
  if (ctx->lVal() != nullptr) {
    return visitLVal(ctx->lVal());
  }
  return visitNumber(ctx->number());
}

/**
 * @brief AST树Number节点的访问函数
 *
 * 将Number节点代表的字符串转化为对应的数值并返回。
 *
 * @param [in] ctx 指向Number节点的指针
 * @return 数值
 */
auto SysYIRGenerator::visitNumber(SysYParser::NumberContext *ctx) -> std::any {
  auto intorFloat = ctx->IntConst();
  if (intorFloat != nullptr) {
    auto intString = ctx->IntConst()->getText();
    int intNum = std::stol(intString, nullptr, 0);
    return static_cast<Value *>(ConstantValue::get(intNum));
  }
  auto floatString = ctx->FloatConst()->getText();
  float floatNum = std::stof(floatString);

  return static_cast<Value *>(ConstantValue::get(floatNum));
}

/**
 * @brief AST树CallExp节点的访问函数
 *
 * 创建Call指令。若形式参数与传入参数类型不同，且传入参数不是常量表达式，则创建额外的IToFInst或FtoIInst指令，
 * 否则创建类型转换后的字面值。
 *
 * @param [in] ctx 指向CallExp节点的指针
 * @return 创建的Call指令
 */
auto SysYIRGenerator::visitCallExp(SysYParser::CallExpContext *ctx)
    -> std::any {
  auto name = ctx->Ident()->getText();
  auto function = module->getFunction(name);
  if (function == nullptr) {
    function = module->getExternalFunction(name);
    if (function == nullptr) {
      std::cout << "The function " << name << " has no definition."
                << std::endl;
      assert(function);
    }
  }
  std::vector<Value *> args = {};
  if (name == "starttime" || name == "stoptime") {
    args.emplace_back(
        ConstantValue::get(static_cast<int>(ctx->getStart()->getLine())));
  } else {
    if (ctx->funcRParams() != nullptr) {
      args = std::any_cast<std::vector<Value *>>(
          visitFuncRParams(ctx->funcRParams()));
    }

    auto params = function->getEntryBlock()->getArguments();
    for (size_t i = 0; i < args.size(); i++) {
      if (params[i]->getType() != args[i]->getType() &&
          (params[i]->getNumDims() != 0 ||
           params[i]->getType()->as<PointerType>()->getBaseType() !=
               args[i]->getType())) {
        auto constValue = dynamic_cast<ConstantValue *>(args[i]);
        if (constValue != nullptr) {
          if (params[i]->getType() ==
              Type::getPointerType(Type::getFloatType())) {
            args[i] =
                ConstantValue::get(static_cast<float>(constValue->getInt()));
          } else {
            args[i] =
                ConstantValue::get(static_cast<int>(constValue->getFloat()));
          }
        } else {
          if (params[i]->getType() ==
              Type::getPointerType(Type::getFloatType())) {
            args[i] = builder.createIToFInst(args[i]);
          } else {
            args[i] = builder.createFtoIInst(args[i]);
          }
        }
      }
    }
  }

  return static_cast<Value *>(builder.createCallInst(function, args));
}

/**
 * @brief AST树UnaryExp节点的访问函数
 *
 * 创建并返回一元运算指令或直接返回原始表达式/Call语句的值。若一元运算指令的操作数是常量，
 * 则直接生成字面值结果并返回。
 *
 * @param [in] ctx 指向UnaryExp节点的指针
 * @return 一元表达式的值
 */
auto SysYIRGenerator::visitUnaryExp(SysYParser::UnaryExpContext *ctx)
    -> std::any {
  if (ctx->primaryExp() != nullptr) {
    return visitPrimaryExp(ctx->primaryExp());
  }
  if (ctx->callExp() != nullptr) {
    return visitCallExp(ctx->callExp());
  }
  auto value = std::any_cast<Value *>(visitUnaryExp(ctx->unaryExp()));
  auto result = value;
  if (ctx->unaryOp()->getText() == "-") {
    auto constValue = dynamic_cast<ConstantValue *>(value);
    if (constValue != nullptr) {
      if (constValue->isFloat()) {
        result = ConstantValue::get(-constValue->getFloat());
      } else {
        result = ConstantValue::get(-constValue->getInt());
      }
    } else if (value != nullptr) {
      if (value->getType() == Type::getIntType()) {
        result = builder.createNegInst(value);
      } else {
        result = builder.createFNegInst(value);
      }
    } else {
      assert(false);
    }
  } else if (ctx->unaryOp()->getText() == "!") {
    auto constValue = dynamic_cast<ConstantValue *>(value);
    if (constValue != nullptr) {
      if (constValue->isFloat()) {
        result =
            ConstantValue::get(1 - (constValue->getFloat() != 0.0F ? 1 : 0));
      } else {
        result = ConstantValue::get(1 - (constValue->getInt() != 0 ? 1 : 0));
      }
    } else if (value != nullptr) {
      if (value->getType() == Type::getIntType()) {
        result = builder.createNotInst(value);
      } else {
        result = builder.createFNotInst(value);
      }
    } else {
      assert(false);
    }
  }

  return result;
}

/**
 * @brief AST树FuncRParams节点的访问函数
 *
 * 返回函数实参列表
 *
 * @param [in] ctx 指向FuncRParams节点的指针
 * @return 函数实参列表
 */
auto SysYIRGenerator::visitFuncRParams(SysYParser::FuncRParamsContext *ctx)
    -> std::any {
  std::vector<Value *> params;

  for (const auto &exp : ctx->exp()) {
    params.push_back(std::any_cast<Value *>(visitExp(exp)));
  }

  return params;
}

/**
 * @brief AST树MulExp节点的访问函数
 *
 * 创建乘法类运算指令。若乘法类运算指令的两个操作数类型不同，且两个操作数均不是常量表达式，
 * 则创建额外的IToFInst或FtoIInst指令，否则创建类型转换后的结果字面值。
 *
 * @param [in] ctx 指向MulExp节点的指针
 * @return 乘法类运算结果
 */
auto SysYIRGenerator::visitMulExp(SysYParser::MulExpContext *ctx) -> std::any {
  auto result = std::any_cast<Value *>(visitUnaryExp(ctx->unaryExp(0)));

  for (size_t i = 1; i < ctx->unaryExp().size(); i++) {
    auto op = ctx->mulOp(i - 1)->getText();
    auto operand = std::any_cast<Value *>(visitUnaryExp(ctx->unaryExp(i)));

    auto resultType = result->getType();
    auto operandType = operand->getType();

    if (resultType == Type::getFloatType() ||
        operandType == Type::getFloatType()) {
      if (operandType != Type::getFloatType()) {
        auto constValue = dynamic_cast<ConstantValue *>(operand);
        if (constValue != nullptr) {
          operand =
              ConstantValue::get(static_cast<float>(constValue->getInt()));
        } else {
          operand = builder.createIToFInst(operand);
        }
      } else if (resultType != Type::getFloatType()) {
        auto constResult = dynamic_cast<ConstantValue *>(result);
        if (constResult != nullptr) {
          result =
              ConstantValue::get(static_cast<float>(constResult->getInt()));
        } else {
          result = builder.createIToFInst(result);
        }
      }

      auto constResult = dynamic_cast<ConstantValue *>(result);
      auto constOperand = dynamic_cast<ConstantValue *>(operand);
      if (op == "*") {
        if ((constOperand != nullptr) && (constResult != nullptr)) {
          result = ConstantValue::get(constResult->getFloat() *
                                      constOperand->getFloat());
        } else {
          result = builder.createFMulInst(result, operand);
        }
      } else if (op == "/") {
        if ((constOperand != nullptr) && (constResult != nullptr)) {
          result = ConstantValue::get(constResult->getFloat() /
                                      constOperand->getFloat());
        } else {
          result = builder.createFDivInst(result, operand);
        }
      } else {
        assert(false);
      }
    } else {
      auto constResult = dynamic_cast<ConstantValue *>(result);
      auto constOperand = dynamic_cast<ConstantValue *>(operand);
      if (op == "*") {
        if ((constOperand != nullptr) && (constResult != nullptr)) {
          result = ConstantValue::get(constResult->getInt() *
                                      constOperand->getInt());
        } else {
          result = builder.createMulInst(result, operand);
        }
      } else if (op == "/") {
        if ((constOperand != nullptr) && (constResult != nullptr)) {
          result = ConstantValue::get(constResult->getInt() /
                                      constOperand->getInt());
        } else {
          result = builder.createDivInst(result, operand);
        }
      } else {
        if ((constOperand != nullptr) && (constResult != nullptr)) {
          result = ConstantValue::get(constResult->getInt() %
                                      constOperand->getInt());
        } else {
          result = builder.createRemInst(result, operand);
        }
      }
    }
  }

  return result;
}

/**
 * @brief AST树AddExp节点的访问函数
 *
 * 创建加法类运算指令。若加法类运算指令的两个操作数类型不同，且两个操作数均不是常量表达式，
 * 则创建额外的IToFInst或FtoIInst指令，否则创建类型转换后的结果字面值。
 *
 * @param [in] ctx 指向AddExp节点的指针
 * @return 加法类运算结果
 */
auto SysYIRGenerator::visitAddExp(SysYParser::AddExpContext *ctx) -> std::any {
  auto result = std::any_cast<Value *>(visitMulExp(ctx->mulExp(0)));

  for (size_t i = 1; i < ctx->mulExp().size(); i++) {
    auto op = ctx->addOp(i - 1)->getText();
    auto operand = std::any_cast<Value *>(visitMulExp(ctx->mulExp(i)));

    auto resultType = result->getType();
    auto operandType = operand->getType();

    if (resultType == Type::getFloatType() ||
        operandType == Type::getFloatType()) {
      if (operandType != Type::getFloatType()) {
        auto constOperand = dynamic_cast<ConstantValue *>(operand);
        if (constOperand != nullptr) {
          operand =
              ConstantValue::get(static_cast<float>(constOperand->getInt()));
        } else {
          operand = builder.createIToFInst(operand);
        }
      } else if (resultType != Type::getFloatType()) {
        auto constResult = dynamic_cast<ConstantValue *>(result);
        if (constResult != nullptr) {
          result =
              ConstantValue::get(static_cast<float>(constResult->getInt()));
        } else {
          result = builder.createIToFInst(result);
        }
      }

      auto constResult = dynamic_cast<ConstantValue *>(result);
      auto constOperand = dynamic_cast<ConstantValue *>(operand);
      if (op == "+") {
        if ((constResult != nullptr) && (constOperand != nullptr)) {
          result = ConstantValue::get(constResult->getFloat() +
                                      constOperand->getFloat());
        } else {
          result = builder.createFAddInst(result, operand);
        }
      } else {
        if ((constResult != nullptr) && (constOperand != nullptr)) {
          result = ConstantValue::get(constResult->getFloat() -
                                      constOperand->getFloat());
        } else {
          result = builder.createFSubInst(result, operand);
        }
      }
    } else {
      auto constResult = dynamic_cast<ConstantValue *>(result);
      auto constOperand = dynamic_cast<ConstantValue *>(operand);
      if (op == "+") {
        if ((constResult != nullptr) && (constOperand != nullptr)) {
          result = ConstantValue::get(constResult->getInt() +
                                      constOperand->getInt());
        } else {
          result = builder.createAddInst(result, operand);
        }
      } else {
        if ((constResult != nullptr) && (constOperand != nullptr)) {
          result = ConstantValue::get(constResult->getInt() -
                                      constOperand->getInt());
        } else {
          result = builder.createSubInst(result, operand);
        }
      }
    }
  }

  return result;
}

/**
 * @brief AST树RelExp节点的访问函数
 *
 * 创建关系类运算指令。若关系类运算指令的两个操作数类型不同，且两个操作数均不是常量表达式，
 * 则创建额外的IToFInst或FtoIInst指令，否则创建类型转换后的结果字面值。
 *
 * @param [in] ctx 指向RelExp节点的指针
 * @return 关系类运算结果
 */
auto SysYIRGenerator::visitRelExp(SysYParser::RelExpContext *ctx) -> std::any {
  auto result = std::any_cast<Value *>(visitAddExp(ctx->addExp(0)));

  for (size_t i = 1; i < ctx->addExp().size(); i++) {
    auto op = ctx->relOp(i - 1)->getText();
    auto operand = std::any_cast<Value *>(visitAddExp(ctx->addExp(i)));

    auto constResult = dynamic_cast<ConstantValue *>(result);
    auto constOperand = dynamic_cast<ConstantValue *>(operand);

    if ((constResult != nullptr) && (constOperand != nullptr)) {
      auto operand1 = constResult->isFloat() ? constResult->getFloat()
                                             : constResult->getInt();
      auto operand2 = constOperand->isFloat() ? constOperand->getFloat()
                                              : constOperand->getInt();

      if (op == "<") {
        result = ConstantValue::get(operand1 < operand2 ? 1 : 0);
      } else if (op == ">") {
        result = ConstantValue::get(operand1 > operand2 ? 1 : 0);
      } else if (op == "<=") {
        result = ConstantValue::get(operand1 <= operand2 ? 1 : 0);
      } else if (op == ">=") {
        result = ConstantValue::get(operand1 >= operand2 ? 1 : 0);
      } else {
        assert(false);
      }
    } else {
      auto resultType = result->getType();
      auto operandType = operand->getType();
      auto floatType = Type::getFloatType();

      if (resultType == floatType || operandType == floatType) {
        if (resultType != floatType) {
          if (constResult != nullptr) {
            result =
                ConstantValue::get(static_cast<float>(constResult->getInt()));
          } else {
            result = builder.createIToFInst(result);
          }
        }
        if (operandType != floatType) {
          if (constOperand != nullptr) {
            operand =
                ConstantValue::get(static_cast<float>(constOperand->getInt()));
          } else {
            operand = builder.createIToFInst(operand);
          }
        }

        if (op == "<") {
          result = builder.createFCmpLTInst(result, operand);
        } else if (op == ">") {
          result = builder.createFCmpGTInst(result, operand);
        } else if (op == "<=") {
          result = builder.createFCmpLEInst(result, operand);
        } else if (op == ">=") {
          result = builder.createFCmpGEInst(result, operand);
        } else {
          assert(false);
        }
      } else {
        if (op == "<") {
          result = builder.createICmpLTInst(result, operand);
        } else if (op == ">") {
          result = builder.createICmpGTInst(result, operand);
        } else if (op == "<=") {
          result = builder.createICmpLEInst(result, operand);
        } else if (op == ">=") {
          result = builder.createICmpGEInst(result, operand);
        } else {
          assert(false);
        }
      }
    }
  }

  return result;
}

/**
 * @brief AST树EqExp节点的访问函数
 *
 * 创建相等类运算指令。若相等类运算指令的两个操作数类型不同，且两个操作数均不是常量表达式，
 * 则创建额外的IToFInst或FtoIInst指令，否则创建类型转换后的结果字面值。特别地，若该EqExp节点
 * 只有一个relExp子节点，则根据其是否为0/0.0F返回1或0。
 *
 * @param [in] ctx 指向EqExp节点的指针
 * @return 相等类运算结果
 */
auto SysYIRGenerator::visitEqExp(SysYParser::EqExpContext *ctx) -> std::any {
  auto result = std::any_cast<Value *>(visitRelExp(ctx->relExp(0)));

  for (size_t i = 1; i < ctx->relExp().size(); i++) {
    auto op = ctx->eqOp(i - 1)->getText();
    auto operand = std::any_cast<Value *>(visitRelExp(ctx->relExp(i)));

    auto constResult = dynamic_cast<ConstantValue *>(result);
    auto constOperand = dynamic_cast<ConstantValue *>(operand);

    if ((constResult != nullptr) && (constOperand != nullptr)) {
      auto operand1 = constResult->isFloat() ? constResult->getFloat()
                                             : constResult->getInt();
      auto operand2 = constOperand->isFloat() ? constOperand->getFloat()
                                              : constOperand->getInt();

      if (op == "==") {
        result = ConstantValue::get(operand1 == operand2 ? 1 : 0);
      } else if (op == "!=") {
        result = ConstantValue::get(operand1 != operand2 ? 1 : 0);
      } else {
        assert(false);
      }
    } else {
      auto resultType = result->getType();
      auto operandType = operand->getType();
      auto floatType = Type::getFloatType();

      if (resultType == floatType || operandType == floatType) {
        if (resultType != floatType) {
          if (constResult != nullptr) {
            result =
                ConstantValue::get(static_cast<float>(constResult->getInt()));
          } else {
            result = builder.createIToFInst(result);
          }
        }
        if (operandType != floatType) {
          if (constOperand != nullptr) {
            operand =
                ConstantValue::get(static_cast<float>(constOperand->getInt()));
          } else {
            operand = builder.createIToFInst(operand);
          }
        }

        if (op == "==") {
          result = builder.createFCmpEQInst(result, operand);
        } else if (op == "!=") {
          result = builder.createFCmpNEInst(result, operand);
        } else {
          assert(false);
        }
      } else {
        if (op == "==") {
          result = builder.createICmpEQInst(result, operand);
        } else if (op == "!=") {
          result = builder.createICmpNEInst(result, operand);
        } else {
          assert(false);
        }
      }
    }
  }

  if (ctx->relExp().size() == 1) {
    auto constResult = dynamic_cast<ConstantValue *>(result);
    if (constResult != nullptr) {
      if (constResult->isFloat()) {
        result = ConstantValue::get(constResult->getFloat() != 0.0F ? 1 : 0);
      } else {
        result = ConstantValue::get(constResult->getInt() != 0 ? 1 : 0);
      }
    }
  }

  return result;
}

/**
 * @brief AST树LAndExp节点的访问函数
 *
 * 为且运算构成的布尔表达式创建一系列跳转链与基本块及其控制流图关系，从而实现短路计算。
 * 特别注意，其中的常量表达式不会被合并运算。
 *
 * @param [in] ctx 指向LAndExp节点的指针
 * @retval 0 表示运行正常
 */
auto SysYIRGenerator::visitLAndExp(SysYParser::LAndExpContext *ctx)
    -> std::any {
  std::stringstream ss;
  BasicBlock *curBlock = builder.getBasicBlock();
  Function *function = curBlock->getParent();
  BasicBlock *trueBlock = builder.getTrueBlock();
  BasicBlock *falseBlock = builder.getFalseBlock();
  auto conds = ctx->eqExp();

  for (size_t i = 0; i < conds.size() - 1; i++) {
    ss << "L" << builder.getLabelIndex();
    BasicBlock *newBlock = function->addBasicBlock(ss.str());
    ss.str("");

    auto *cond = std::any_cast<Value *>(visitEqExp(conds[i]));
    builder.createCondBrInst(cond, newBlock, falseBlock, {}, {});

    BasicBlock::conectBlocks(curBlock, newBlock);
    BasicBlock::conectBlocks(curBlock, falseBlock);

    curBlock = newBlock;
    builder.setPosition(curBlock, curBlock->end());
  }

  auto *cond = std::any_cast<Value *>(visitEqExp(conds.back()));
  builder.createCondBrInst(cond, trueBlock, falseBlock, {}, {});

  BasicBlock::conectBlocks(curBlock, trueBlock);
  BasicBlock::conectBlocks(curBlock, falseBlock);

  return 0;
}

/**
 * @brief AST树LOrExp节点的访问函数
 *
 * 为或运算构成的布尔表达式创建一系列跳转链与基本块及其控制流图关系，从而实现短路计算。
 * 特别注意，其中的常量表达式不会被合并运算。
 *
 * @param [in] ctx 指向LOrExp节点的指针
 * @retval 0 表示运行正常
 */
auto SysYIRGenerator::visitLOrExp(SysYParser::LOrExpContext *ctx) -> std::any {
  std::stringstream ss;
  BasicBlock *curBlock = builder.getBasicBlock();
  Function *function = curBlock->getParent();
  auto conds = ctx->lAndExp();

  for (size_t i = 0; i < conds.size() - 1; i++) {
    ss << "L" << builder.getLabelIndex();
    BasicBlock *newBlock = function->addBasicBlock(ss.str());
    ss.str("");

    builder.pushFalseBlock(newBlock);
    visitLAndExp(conds[i]);
    builder.popFalseBlock();

    builder.setPosition(newBlock, newBlock->end());
  }

  visitLAndExp(conds.back());

  return 0;
}

/**
 * @brief 将tree表示的变量初值转换为ValueCounter表示的变量初值
 *
 * 该函数会为自动填充初值，使得其数量与数组大小一致。
 *
 * 数组赋初值四条规则：
 * 1. 各个子括号的赋值相互独立，仅与其内的符号相关
 * 2. 各个子括号的赋值对应一段连续地址，其维度取决于开始地址(按最高计算)
 * 3. 各括号不足的部分用0/0.0填补
 * 4. 碰到数字，直接赋值即可
 *
 * 该函数将会遵循这四条规则进行填充。并且会为类型不匹配的初值创建IToFInst/FToIInst
 * 指令或者类型转换的字面值。
 *
 * @param [in]  type    表示要赋初值的变量的类型
 * @param [in]  root    表示输入的tree表示的初值
 * @param [in]  dims    表示数组各个维度的大小，对于标量，其为空
 * @param [in]  numDims dims的长度，表示维度数量
 * @param [out] result  存放ValueCounter表示的结果
 * @param [in]  builder 指向IRBuilder对象的指针
 * @return 无返回值
 * @sa ValueTreeNode, ValueCounter
 */

void Helper::tree2Array(Type *type, ValueTreeNode *root,
                        const std::vector<Value *> &dims, unsigned numDims,
                        ValueCounter &result, IRBuilder *builder) {
  auto value = root->getValue();
  auto &children = root->getChildren();
  if (value != nullptr) {
    if (type == value->getType()) {
      result.push_back(value);
    } else {
      if (type == Type::getFloatType()) {
        auto constValue = dynamic_cast<ConstantValue *>(value);
        if (constValue != nullptr) {
          result.push_back(
              ConstantValue::get(static_cast<float>(constValue->getInt())));
        } else {
          result.push_back(builder->createIToFInst(value));
        }
      } else {
        auto constValue = dynamic_cast<ConstantValue *>(value);
        if (constValue != nullptr) {
          result.push_back(
              ConstantValue::get(static_cast<int>(constValue->getFloat())));
        } else {
          result.push_back(builder->createFtoIInst(value));
        }
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
    if (type == Type::getFloatType()) {
      result.push_back(ConstantValue::get(0.0F), num);
    } else {
      result.push_back(ConstantValue::get(0), num);
    }
  }
}
/**
 * @brief 创建外部函数
 *
 * @param [in] paramTypes 形式参数类型列表
 * @param [in] paramNames 形式参数名字列表
 * @param [in] paramDims 形式参数维度列表
 * @param [in] returnType 返回值类型
 * @param [in] funcName 外部函数名字
 * @param [in] pModule 模块指针
 * @param [in] pBuilder IR构建器指针
 * @return 无返回值
 */
void Helper::createExternalFunction(
    const std::vector<Type *> &paramTypes,
    const std::vector<std::string> &paramNames,
    const std::vector<std::vector<Value *>> &paramDims, Type *returnType,
    const std::string &funcName, Module *pModule, IRBuilder *pBuilder) {
  auto funcType = Type::getFunctionType(returnType, paramTypes);
  auto function = pModule->createExternalFunction(funcName, funcType);
  auto entry = function->getEntryBlock();
  pBuilder->setPosition(entry, entry->end());

  for (size_t i = 0; i < paramTypes.size(); ++i) {
    auto alloca = pBuilder->createAllocaInst(
        Type::getPointerType(paramTypes[i]), paramDims[i], paramNames[i]);
    entry->insertArgument(alloca);
    // pModule->addVariable(paramNames[i], alloca);
  }
}
/**
 * @brief 初始化外部函数
 *
 * @param [in] pModule 模块指针
 * @param [in] pBuilder IR构建器指针
 * @return 无返回值
 */
void Helper::initExternalFunction(Module *pModule, IRBuilder *pBuilder) {
  std::vector<Type *> paramTypes;
  std::vector<std::string> paramNames;
  std::vector<std::vector<Value *>> paramDims;
  Type *returnType;
  std::string funcName;

  returnType = Type::getIntType();
  funcName = "getint";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);
  funcName = "getch";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);
  paramTypes.push_back(Type::getIntType());
  paramNames.emplace_back("a");
  paramDims.push_back(std::vector<Value *>{ConstantValue::get(-1)});
  funcName = "getarray";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  returnType = Type::getFloatType();
  paramTypes.clear();
  paramNames.clear();
  paramDims.clear();
  funcName = "getfloat";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  returnType = Type::getIntType();
  paramTypes.push_back(Type::getFloatType());
  paramNames.emplace_back("a");
  paramDims.push_back(std::vector<Value *>{ConstantValue::get(-1)});
  funcName = "getfarray";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  returnType = Type::getVoidType();

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramDims.clear();
  paramDims.emplace_back();
  funcName = "putint";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  funcName = "putch";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramTypes.push_back(Type::getIntType());
  paramDims.clear();
  paramDims.emplace_back();
  paramDims.push_back(std::vector<Value *>{ConstantValue::get(-1)});
  paramNames.clear();
  paramNames.emplace_back("n");
  paramNames.emplace_back("a");
  funcName = "putarray";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getFloatType());
  paramDims.clear();
  paramDims.emplace_back();
  paramNames.clear();
  paramNames.emplace_back("a");
  funcName = "putfloat";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramTypes.push_back(Type::getFloatType());
  paramDims.clear();
  paramDims.emplace_back();
  paramDims.push_back(std::vector<Value *>{ConstantValue::get(-1)});
  paramNames.clear();
  paramNames.emplace_back("n");
  paramNames.emplace_back("a");
  funcName = "putfarray";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramDims.clear();
  paramDims.emplace_back();
  paramNames.clear();
  paramNames.emplace_back("__LINE__");
  funcName = "starttime";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  paramTypes.clear();
  paramTypes.push_back(Type::getIntType());
  paramDims.clear();
  paramDims.emplace_back();
  paramNames.clear();
  paramNames.emplace_back("__LINE__");
  funcName = "stoptime";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);

  returnType = Type::getIntType();
  paramTypes.clear();
  paramTypes.push_back(Type::getPointerType(Type::getIntType()));
  paramTypes.push_back(Type::getIntType());
  paramTypes.push_back(Type::getIntType());
  paramDims.clear();
  paramDims.emplace_back(std::vector<Value *>{ConstantValue::get(-1)});
  paramDims.emplace_back();
  paramDims.emplace_back();
  paramNames.clear();
  paramNames.emplace_back("table");
  paramNames.emplace_back("key1");
  paramNames.emplace_back("key2");
  funcName = "functionCacheLookup";
  Helper::createExternalFunction(paramTypes, paramNames, paramDims, returnType,
                                 funcName, pModule, pBuilder);
}
} // namespace sysy
