#include "../include/semantic.h"

bool SemanticAnalyzer::analyze(CompUnit &root) {
  // 清空之前的分析结果
  errors.clear();
  pending_references.clear();

  // 第一阶段：一次扫描，建立符号表和记录待检查项
  try {
    root.accept(*this);
  } catch (const std::exception &e) {
    reportError(SemanticErrorType::UNDEFINED_SYMBOL,
                "Internal error during analysis: " + std::string(e.what()),
                {0});
    return false;
  }

  // 第二阶段：延迟检查
  performDelayedChecks();

  return !hasErrors();
}

void SemanticAnalyzer::printErrors(std::ostream &out) const {
  for (const auto &error : errors) {
    out << "语义错误 ";
    if (error.location.line > 0) {
      out << "(第" << error.location.line << "行): ";
    } else {
      out << ": ";
    }
    out << error.message << std::endl;
  }
}

// ===--------------------------------------------------------------------=== //
// 错误报告和延迟检查管理
// ===--------------------------------------------------------------------=== //

void SemanticAnalyzer::reportError(SemanticErrorType type,
                                   const std::string &message,
                                   SourceLocation location) {
  errors.emplace_back(type, message, location);
}

void SemanticAnalyzer::addPendingReference(PendingReference::Type type,
                                           const std::string &name,
                                           SourceLocation location,
                                           Exp *expression) {
  pending_references.emplace_back(type, name, location, expression);
}

// ===--------------------------------------------------------------------=== //
// 类型推导和检查
// ===--------------------------------------------------------------------=== //

std::shared_ptr<Type> SemanticAnalyzer::inferExpressionType(Exp *expression) {
  if (!expression)
    return nullptr;

  // 使用dynamic_cast来确定表达式类型
  if (auto *number = dynamic_cast<Number *>(expression)) {
    if (std::holds_alternative<int>(number->value)) {
      return makeBasicType(BaseType::INT);
    } else {
      return makeBasicType(BaseType::FLOAT);
    }
  }

  if (dynamic_cast<StringLiteral *>(expression)) {
    return makeBasicType(BaseType::STRING);
  }

  if (auto *lval = dynamic_cast<LVal *>(expression)) {
    // 查找符号类型
    auto *symbol = symbol_table.lookup(lval->name);
    if (symbol) {
      auto base_type = symbol->type;

      // 如果有数组索引，需要推导元素类型
      if (!lval->indices.empty() && base_type) {
        auto *array_type = dynamic_cast<ArrayType *>(base_type.get());
        if (array_type) {
          // 根据索引维度确定结果类型
          size_t index_count = lval->indices.size();
          size_t array_dims = array_type->dimensions.size();

          if (index_count > array_dims) {
            // 索引过多，类型错误，但这里先返回基础类型
            return array_type->element_type;
          } else if (index_count == array_dims) {
            // 完全索引，返回元素类型
            return array_type->element_type;
          } else {
            // 部分索引，返回子数组类型
            std::vector<int> remaining_dims(array_type->dimensions.begin() +
                                                index_count,
                                            array_type->dimensions.end());
            return makeArrayType(array_type->element_type, remaining_dims);
          }
        } else {
          // 对非数组进行索引访问，类型错误
          return nullptr;
        }
      }

      return base_type;
    } else {
      // 记录为延迟检查项
      addPendingReference(PendingReference::VARIABLE_REF, lval->name,
                          lval->location, expression);
      return nullptr;
    }
  }

  if (auto *binary = dynamic_cast<BinaryExp *>(expression)) {
    auto left_type = inferExpressionType(binary->lhs.get());
    auto right_type = inferExpressionType(binary->rhs.get());

    if (!left_type || !right_type)
      return nullptr;

    // 关系运算符和逻辑运算符返回int
    if (binary->op == BinaryOp::GT || binary->op == BinaryOp::GTE ||
        binary->op == BinaryOp::LT || binary->op == BinaryOp::LTE ||
        binary->op == BinaryOp::EQ || binary->op == BinaryOp::NEQ ||
        binary->op == BinaryOp::AND || binary->op == BinaryOp::OR) {
      return makeBasicType(BaseType::INT);
    }

    // 算术运算符：如果有一个是float，结果是float
    if (auto *left_basic = dynamic_cast<BasicType *>(left_type.get())) {
      if (auto *right_basic = dynamic_cast<BasicType *>(right_type.get())) {
        if (left_basic->type == BaseType::FLOAT ||
            right_basic->type == BaseType::FLOAT) {
          return makeBasicType(BaseType::FLOAT);
        } else {
          return makeBasicType(BaseType::INT);
        }
      }
    }
  }

  if (auto *unary = dynamic_cast<UnaryExp *>(expression)) {
    auto operand_type = inferExpressionType(unary->operand.get());
    if (unary->op == UnaryOp::NOT) {
      return makeBasicType(BaseType::INT);
    }
    return operand_type;
  }

  if (auto *call = dynamic_cast<FunctionCall *>(expression)) {
    auto *symbol = symbol_table.lookup(call->function_name);
    if (symbol && symbol->kind == SymbolKind::FUNCTION) {
      if (auto *func_type = dynamic_cast<FunctionType *>(symbol->type.get())) {
        return func_type->return_type;
      }
    } else {
      // 记录为延迟检查项
      addPendingReference(PendingReference::FUNCTION_CALL, call->function_name,
                          call->location, expression);
    }
  }

  return nullptr;
}

bool SemanticAnalyzer::isTypeCompatible(const Type &expected,
                                        const Type &actual) {
  return expected.equals(actual) || canImplicitConvert(actual, expected);
}

bool SemanticAnalyzer::canImplicitConvert(const Type &from, const Type &to) {
  auto *from_basic = dynamic_cast<const BasicType *>(&from);
  auto *to_basic = dynamic_cast<const BasicType *>(&to);

  if (!from_basic || !to_basic)
    return false;

  // int和float可以互相隐式转换
  if (from_basic->type == BaseType::INT && to_basic->type == BaseType::FLOAT) {
    return true;
  }
  if (from_basic->type == BaseType::FLOAT && to_basic->type == BaseType::INT) {
    return true;
  }

  return false;
}

// ===--------------------------------------------------------------------=== //
// 常量表达式求值
// ===--------------------------------------------------------------------=== //

std::optional<std::variant<int, float>>
SemanticAnalyzer::evaluateConstExpression(Exp *expression) {
  if (!expression)
    return std::nullopt;

  if (auto *number = dynamic_cast<Number *>(expression)) {
    return number->value;
  }

  if (auto *lval = dynamic_cast<LVal *>(expression)) {
    auto *symbol = symbol_table.lookup(lval->name);
    if (symbol && symbol->kind == SymbolKind::CONSTANT &&
        symbol->constant_value) {
      return symbol->constant_value.value();
    }
  }

  if (auto *binary = dynamic_cast<BinaryExp *>(expression)) {
    auto left_val = evaluateConstExpression(binary->lhs.get());
    auto right_val = evaluateConstExpression(binary->rhs.get());

    if (!left_val || !right_val)
      return std::nullopt;

    // 处理int运算
    if (std::holds_alternative<int>(left_val.value()) &&
        std::holds_alternative<int>(right_val.value())) {
      int left = std::get<int>(left_val.value());
      int right = std::get<int>(right_val.value());

      switch (binary->op) {
      case BinaryOp::ADD:
        return left + right;
      case BinaryOp::SUB:
        return left - right;
      case BinaryOp::MUL:
        return left * right;
      case BinaryOp::DIV:
        if (right != 0)
          return left / right;
        else
          return std::nullopt;
      case BinaryOp::MOD:
        if (right != 0)
          return left % right;
        else
          return std::nullopt;
      default:
        return std::nullopt;
      }
    }

    // 处理float运算
    if (std::holds_alternative<float>(left_val.value()) &&
        std::holds_alternative<float>(right_val.value())) {
      float left = std::get<float>(left_val.value());
      float right = std::get<float>(right_val.value());

      switch (binary->op) {
      case BinaryOp::ADD:
        return left + right;
      case BinaryOp::SUB:
        return left - right;
      case BinaryOp::MUL:
        return left * right;
      case BinaryOp::DIV:
        if (right != 0.0f)
          return left / right;
        else
          return std::nullopt;
      default:
        return std::nullopt;
      }
    }

    // 处理混合类型运算 (int和float)
    if ((std::holds_alternative<int>(left_val.value()) &&
         std::holds_alternative<float>(right_val.value())) ||
        (std::holds_alternative<float>(left_val.value()) &&
         std::holds_alternative<int>(right_val.value()))) {

      float left = std::holds_alternative<int>(left_val.value())
                       ? static_cast<float>(std::get<int>(left_val.value()))
                       : std::get<float>(left_val.value());
      float right = std::holds_alternative<int>(right_val.value())
                        ? static_cast<float>(std::get<int>(right_val.value()))
                        : std::get<float>(right_val.value());

      switch (binary->op) {
      case BinaryOp::ADD:
        return left + right;
      case BinaryOp::SUB:
        return left - right;
      case BinaryOp::MUL:
        return left * right;
      case BinaryOp::DIV:
        if (right != 0.0f)
          return left / right;
        else
          return std::nullopt;
      default:
        return std::nullopt;
      }
    }
  }

  if (auto *unary = dynamic_cast<UnaryExp *>(expression)) {
    auto operand_val = evaluateConstExpression(unary->operand.get());
    if (!operand_val)
      return std::nullopt;

    if (std::holds_alternative<int>(operand_val.value())) {
      int value = std::get<int>(operand_val.value());
      switch (unary->op) {
      case UnaryOp::PLUS:
        return value;
      case UnaryOp::MINUS:
        return -value;
      case UnaryOp::NOT:
        return value == 0 ? 1 : 0;
      default:
        return std::nullopt;
      }
    }

    if (std::holds_alternative<float>(operand_val.value())) {
      float value = std::get<float>(operand_val.value());
      switch (unary->op) {
      case UnaryOp::PLUS:
        return value;
      case UnaryOp::MINUS:
        return -value;
      case UnaryOp::NOT:
        return value == 0.0f ? 1 : 0;
      default:
        return std::nullopt;
      }
    }
  }

  return std::nullopt;
}

std::optional<std::variant<int, float>>
SemanticAnalyzer::evaluateConstInitVal(ConstInitVal *init_val) {
  if (!init_val)
    return std::nullopt;

  // 检查是否为单个表达式（非数组初始化）
  if (std::holds_alternative<std::unique_ptr<Exp>>(init_val->value)) {
    auto &expr = std::get<std::unique_ptr<Exp>>(init_val->value);
    return evaluateConstExpression(expr.get());
  }

  // 数组初始化暂不支持求值
  return std::nullopt;
}

// ===--------------------------------------------------------------------=== //
// 数组维度计算
// ===--------------------------------------------------------------------=== //

std::vector<int> SemanticAnalyzer::calculateArrayDimensions(
    const std::vector<std::unique_ptr<Exp>> &dims) {
  std::vector<int> dimensions;

  for (const auto &dim : dims) {
    auto value = evaluateConstExpression(dim.get());
    if (value && std::holds_alternative<int>(value.value())) {
      int size = std::get<int>(value.value());
      if (size > 0) {
        dimensions.push_back(size);
      } else {
        dimensions.push_back(-1); // 无效维度
      }
    } else {
      dimensions.push_back(-1); // 无法计算的维度
    }
  }

  return dimensions;
}

// ===--------------------------------------------------------------------=== //
// AST节点访问实现
// ===--------------------------------------------------------------------=== //

void SemanticAnalyzer::visit(CompUnit &node) {
  // 遍历所有顶层声明和函数定义
  for (auto &item : node.items) {
    std::visit([this](auto &ptr) { ptr->accept(*this); }, item);
  }
}

void SemanticAnalyzer::visit(ConstDecl &node) {
  // 保存当前声明类型
  BaseType prev_type = current_decl_type;
  current_decl_type = node.type;

  for (auto &def : node.definitions) {
    def->accept(*this);
  }

  // 恢复之前的类型
  current_decl_type = prev_type;
}

void SemanticAnalyzer::visit(ConstDef &node) {
  // 检查是否重复定义
  if (symbol_table.lookupCurrent(node.name)) {
    reportError(SemanticErrorType::REDEFINED_SYMBOL,
                "常量 '" + node.name + "' 重复定义", node.location);
    return;
  }

  // 计算数组维度
  std::vector<int> dimensions = calculateArrayDimensions(node.array_dimensions);

  // 创建类型
  std::shared_ptr<Type> type;
  if (dimensions.empty()) {
    // 普通常量，类型从父节点的ConstDecl获取
    type = makeBasicType(current_decl_type);
  } else {
    type = makeArrayType(makeBasicType(current_decl_type), dimensions);
  }

  // 创建符号信息
  auto symbol =
      std::make_shared<SymbolInfo>(SymbolKind::CONSTANT, type, node.name, true);
  symbol->setArrayDimensions(dimensions);

  // 计算初值
  if (node.initializer) {
    node.initializer->accept(*this);
    // 尝试计算常量值
    if (dimensions.empty()) {
      // 只对非数组常量计算值
      auto const_value = evaluateConstInitVal(node.initializer.get());
      if (const_value.has_value()) {
        if (current_decl_type == BaseType::INT &&
            std::holds_alternative<int>(const_value.value())) {
          symbol->setConstantValue(std::get<int>(const_value.value()));
        } else if (current_decl_type == BaseType::FLOAT &&
                   std::holds_alternative<float>(const_value.value())) {
          symbol->setConstantValue(std::get<float>(const_value.value()));
        } else if (current_decl_type == BaseType::FLOAT &&
                   std::holds_alternative<int>(const_value.value())) {
          // int可以隐式转换为float
          symbol->setConstantValue(
              static_cast<float>(std::get<int>(const_value.value())));
        }
      }
    }
  }

  // 插入符号表
  if (!symbol_table.insert(node.name, symbol)) {
    reportError(SemanticErrorType::REDEFINED_SYMBOL,
                "常量 '" + node.name + "' 插入符号表失败", node.location);
  }
}

void SemanticAnalyzer::visit(VarDecl &node) {
  // 保存当前声明类型
  BaseType prev_type = current_decl_type;
  current_decl_type = node.type;

  for (auto &def : node.definitions) {
    def->accept(*this);
  }

  // 恢复之前的类型
  current_decl_type = prev_type;
}

void SemanticAnalyzer::visit(VarDef &node) {
  // 检查是否重复定义
  if (symbol_table.lookupCurrent(node.name)) {
    reportError(SemanticErrorType::REDEFINED_SYMBOL,
                "变量 '" + node.name + "' 重复定义", node.location);
    return;
  }

  // 计算数组维度
  std::vector<int> dimensions = calculateArrayDimensions(node.array_dimensions);

  // 创建类型
  std::shared_ptr<Type> type;
  if (dimensions.empty()) {
    type = makeBasicType(current_decl_type);
  } else {
    type = makeArrayType(makeBasicType(current_decl_type), dimensions);
  }

  // 创建符号信息
  bool is_initialized = node.initializer.has_value();
  auto symbol = std::make_shared<SymbolInfo>(SymbolKind::VARIABLE, type,
                                             node.name, is_initialized);
  symbol->setArrayDimensions(dimensions);

  // 处理初值
  if (node.initializer) {
    (*node.initializer)->accept(*this);
  }

  // 插入符号表
  if (!symbol_table.insert(node.name, symbol)) {
    reportError(SemanticErrorType::REDEFINED_SYMBOL,
                "变量 '" + node.name + "' 插入符号表失败", node.location);
  }
}

void SemanticAnalyzer::visit(FuncDef &node) {
  // 检查是否重复定义
  if (symbol_table.lookupCurrent(node.name)) {
    reportError(SemanticErrorType::REDEFINED_SYMBOL,
                "函数 '" + node.name + "' 重复定义", node.location);
    return;
  }

  // 构建参数类型列表
  std::vector<std::shared_ptr<Type>> param_types;
  for (const auto &param : node.parameters) {
    // 根据参数类型构建实际类型
    std::shared_ptr<Type> param_type;
    if (param->is_array_pointer) {
      // 数组指针参数
      std::vector<int> dimensions =
          calculateArrayDimensions(param->array_dimensions);
      param_type = makeArrayType(makeBasicType(param->type), dimensions);
    } else {
      param_type = makeBasicType(param->type);
    }
    param_types.push_back(param_type);
  }

  // 创建函数类型
  auto return_type = makeBasicType(node.return_type);
  auto func_type = makeFunctionType(return_type, param_types);

  // 创建函数符号
  auto symbol = std::make_shared<SymbolInfo>(SymbolKind::FUNCTION, func_type,
                                             node.name, true);

  // 插入符号表
  if (!symbol_table.insert(node.name, symbol)) {
    reportError(SemanticErrorType::REDEFINED_SYMBOL,
                "函数 '" + node.name + "' 插入符号表失败", node.location);
    return;
  }

  // 进入函数作用域
  symbol_table.enterScope();
  bool prev_in_function = in_function;
  auto prev_return_type = current_function_return_type;
  auto prev_function_name = current_function_name;

  in_function = true;
  current_function_return_type = return_type;
  current_function_name = node.name;

  // 处理参数
  for (auto &param : node.parameters) {
    param->accept(*this);
  }

  // 处理函数体
  if (node.body) {
    node.body->accept(*this);
  }

  // 恢复状态
  in_function = prev_in_function;
  current_function_return_type = prev_return_type;
  current_function_name = prev_function_name;
  symbol_table.exitScope();
}

void SemanticAnalyzer::visit(FuncFParam &node) {
  // 检查参数是否重复定义
  if (symbol_table.lookupCurrent(node.name)) {
    reportError(SemanticErrorType::REDEFINED_SYMBOL,
                "参数 '" + node.name + "' 重复定义", node.location);
    return;
  }

  // 创建参数类型
  std::shared_ptr<Type> type;
  if (node.is_array_pointer) {
    // 数组指针参数
    std::vector<int> dimensions =
        calculateArrayDimensions(node.array_dimensions);
    type = makeArrayType(makeBasicType(node.type), dimensions);
  } else {
    type = makeBasicType(node.type);
  }

  // 创建参数符号
  auto symbol = std::make_shared<SymbolInfo>(SymbolKind::PARAMETER, type,
                                             node.name, true);
  symbol->is_array_pointer = node.is_array_pointer;

  // 插入符号表
  if (!symbol_table.insert(node.name, symbol)) {
    reportError(SemanticErrorType::REDEFINED_SYMBOL,
                "参数 '" + node.name + "' 插入符号表失败", node.location);
  }
}

void SemanticAnalyzer::visit(Block &node) {
  // 进入新的作用域
  symbol_table.enterScope();

  // 处理块内容
  for (auto &item : node.items) {
    std::visit([this](auto &ptr) { ptr->accept(*this); }, item);
  }

  // 退出作用域
  symbol_table.exitScope();
}

void SemanticAnalyzer::visit(IfStmt &node) {
  // 检查条件表达式
  if (node.condition) {
    node.condition->accept(*this);
  }

  // 检查then分支
  if (node.then_statement) {
    node.then_statement->accept(*this);
  }

  // 检查else分支
  if (node.else_statement) {
    node.else_statement->get()->accept(*this);
  }
}

void SemanticAnalyzer::visit(WhileStmt &node) {
  // 检查条件表达式
  if (node.condition) {
    node.condition->accept(*this);
  }

  // 进入循环作用域
  bool prev_in_loop = in_loop;
  in_loop = true;

  // 检查循环体
  if (node.body) {
    node.body->accept(*this);
  }

  // 恢复状态
  in_loop = prev_in_loop;
}

void SemanticAnalyzer::visit(ExpStmt &node) {
  if (node.expression) {
    (*node.expression)->accept(*this);
  }
}

void SemanticAnalyzer::visit(AssignStmt &node) {
  std::shared_ptr<Type> lvalue_type = nullptr;
  std::shared_ptr<Type> rvalue_type = nullptr;

  // 检查左值
  if (node.lvalue) {
    node.lvalue->accept(*this);

    // 获取左值类型（考虑数组访问）
    lvalue_type = inferExpressionType(node.lvalue.get());

    // 检查是否为常量
    auto *symbol = symbol_table.lookup(node.lvalue->name);
    if (symbol && symbol->kind == SymbolKind::CONSTANT) {
      reportError(SemanticErrorType::CONST_MODIFICATION,
                  "不能修改常量 '" + node.lvalue->name + "'", node.location);
      return;
    }
  }

  // 检查右值
  if (node.rvalue) {
    node.rvalue->accept(*this);
    rvalue_type = inferExpressionType(node.rvalue.get());
  }

  // 类型兼容性检查
  if (lvalue_type && rvalue_type) {
    if (!isTypeCompatible(*lvalue_type, *rvalue_type)) {
      reportError(SemanticErrorType::TYPE_MISMATCH,
                  "赋值类型不匹配：不能将 '" + rvalue_type->toString() +
                      "' 赋值给 '" + lvalue_type->toString() + "'",
                  node.location);
    }
  }
}

void SemanticAnalyzer::visit(ReturnStmt &node) {
  if (!in_function) {
    reportError(SemanticErrorType::INVALID_ASSIGNMENT,
                "return语句只能在函数内使用", node.location);
    return;
  }

  if (node.expression) {
    (*node.expression)->accept(*this);

    // 检查返回类型匹配
    if (current_function_return_type) {
      auto return_expr_type = inferExpressionType((*node.expression).get());
      if (return_expr_type) {
        if (!isTypeCompatible(*current_function_return_type,
                              *return_expr_type)) {
          reportError(SemanticErrorType::TYPE_MISMATCH,
                      "返回类型不匹配：期望 '" +
                          current_function_return_type->toString() +
                          "'，实际 '" + return_expr_type->toString() + "'",
                      node.location);
        }
      }
    }
  } else {
    // 检查void函数
    if (current_function_return_type) {
      auto *basic_type =
          dynamic_cast<BasicType *>(current_function_return_type.get());
      if (!basic_type || basic_type->type != BaseType::VOID) {
        reportError(SemanticErrorType::TYPE_MISMATCH, "非void函数必须返回值",
                    node.location);
      }
    }
  }
}

void SemanticAnalyzer::visit(BreakStmt &node) {
  if (!in_loop) {
    reportError(SemanticErrorType::BREAK_CONTINUE_ERROR,
                "break语句只能在循环内使用", node.location);
  }
}

void SemanticAnalyzer::visit(ContinueStmt &node) {
  if (!in_loop) {
    reportError(SemanticErrorType::BREAK_CONTINUE_ERROR,
                "continue语句只能在循环内使用", node.location);
  }
}

void SemanticAnalyzer::visit(UnaryExp &node) {
  if (node.operand) {
    node.operand->accept(*this);
  }
}

void SemanticAnalyzer::visit(BinaryExp &node) {
  if (node.lhs) {
    node.lhs->accept(*this);
  }
  if (node.rhs) {
    node.rhs->accept(*this);
  }
}

void SemanticAnalyzer::visit(LVal &node) {
  // 查找符号
  auto *symbol = symbol_table.lookup(node.name);
  if (!symbol) {
    // 添加到延迟检查列表
    addPendingReference(PendingReference::VARIABLE_REF, node.name,
                        node.location, &node);
    return;
  }

  // 检查数组访问
  for (auto &index : node.indices) {
    index->accept(*this);
  }
}

void SemanticAnalyzer::visit(FunctionCall &node) {
  // 查找函数符号
  auto *symbol = symbol_table.lookup(node.function_name);
  if (!symbol) {
    // 添加到延迟检查列表
    addPendingReference(PendingReference::FUNCTION_CALL, node.function_name,
                        node.location, &node);
  }

  // 检查参数
  for (auto &arg : node.arguments) {
    arg->accept(*this);
  }
}

void SemanticAnalyzer::visit(Number & /*node*/) {
  // 数字字面量不需要检查
}

void SemanticAnalyzer::visit(StringLiteral & /*node*/) {
  // 字符串字面量不需要检查
}

void SemanticAnalyzer::visit(InitVal &node) {
  std::visit(
      [this](auto &value) {
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>,
                                     std::unique_ptr<Exp>>) {
          if (value)
            value->accept(*this);
        } else {
          for (auto &item : value) {
            if (item)
              item->accept(*this);
          }
        }
      },
      node.value);
}

void SemanticAnalyzer::visit(ConstInitVal &node) {
  std::visit(
      [this](auto &value) {
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>,
                                     std::unique_ptr<Exp>>) {
          if (value)
            value->accept(*this);
        } else {
          for (auto &item : value) {
            if (item)
              item->accept(*this);
          }
        }
      },
      node.value);
}

// ===--------------------------------------------------------------------=== //
// 延迟检查实现
// ===--------------------------------------------------------------------=== //

void SemanticAnalyzer::performDelayedChecks() {
  checkPendingReferences();
  checkMainFunction();
}

void SemanticAnalyzer::checkPendingReferences() {
  for (const auto &ref : pending_references) {
    auto *symbol = symbol_table.lookup(ref.name);
    if (!symbol) {
      reportError(SemanticErrorType::UNDEFINED_SYMBOL,
                  "未定义的符号 '" + ref.name + "'", ref.location);
    }
  }
}

void SemanticAnalyzer::checkMainFunction() {
  auto *main_symbol = symbol_table.lookup("main");
  if (!main_symbol) {
    reportError(SemanticErrorType::UNDEFINED_SYMBOL, "缺少main函数", {0});
    return;
  }

  if (main_symbol->kind != SymbolKind::FUNCTION) {
    reportError(SemanticErrorType::TYPE_MISMATCH, "main必须是函数", {0});
    return;
  }

  // 检查main函数签名
  auto *func_type = dynamic_cast<FunctionType *>(main_symbol->type.get());
  if (!func_type) {
    reportError(SemanticErrorType::TYPE_MISMATCH, "main函数类型错误", {0});
    return;
  }

  // main函数应该返回int
  auto *return_type = dynamic_cast<BasicType *>(func_type->return_type.get());
  if (!return_type || return_type->type != BaseType::INT) {
    reportError(SemanticErrorType::TYPE_MISMATCH, "main函数应该返回int类型",
                {0});
  }
}