#include "frontend/SemanticAnalysis.h"

#include <algorithm>
#include <cmath>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>

bool SemanticAnalysis::analyze(ast::TranslationUnit &unit) {
    reset();
    installRuntimeFunctions();
    pushScope();

    for (const std::unique_ptr<ast::Decl> &decl : unit.declarations) {
        analyzeDecl(*decl);
    }

    if (!sawMain_) {
        errors_.push_back("缺少入口函数 int main()");
    }

    popScope();
    return errors_.empty();
}

const std::vector<std::string> &SemanticAnalysis::errors() const {
    return errors_;
}

void SemanticAnalysis::reset() {
    scopes_.clear();
    constants_.clear();
    functions_.clear();
    errors_.clear();
    currentFunctionReturn_ = {};
    loopDepth_ = 0;
    sawMain_ = false;
}

void SemanticAnalysis::installRuntimeFunctions() {
    auto scalar = [](ast::BuiltinType base) {
        ast::Type type;
        type.base = base;
        return type;
    };
    auto array = [](ast::BuiltinType base) {
        ast::Type type;
        type.base = base;
        type.shape = {-1};
        return type;
    };

    functions_["getint"] = {scalar(ast::BuiltinType::Int), {}, false, true, nullptr};
    functions_["getch"] = {scalar(ast::BuiltinType::Int), {}, false, true, nullptr};
    functions_["getfloat"] = {scalar(ast::BuiltinType::Float), {}, false, true, nullptr};
    functions_["getarray"] = {scalar(ast::BuiltinType::Int),
                              {array(ast::BuiltinType::Int)}, false, true, nullptr};
    functions_["getfarray"] = {scalar(ast::BuiltinType::Int),
                               {array(ast::BuiltinType::Float)}, false, true, nullptr};
    functions_["putint"] = {scalar(ast::BuiltinType::Void),
                            {scalar(ast::BuiltinType::Int)}, false, true, nullptr};
    functions_["putch"] = {scalar(ast::BuiltinType::Void),
                           {scalar(ast::BuiltinType::Int)}, false, true, nullptr};
    functions_["putfloat"] = {scalar(ast::BuiltinType::Void),
                              {scalar(ast::BuiltinType::Float)}, false, true, nullptr};
    functions_["putarray"] = {scalar(ast::BuiltinType::Void),
                              {scalar(ast::BuiltinType::Int),
                               array(ast::BuiltinType::Int)},
                              false, true, nullptr};
    functions_["putfarray"] = {scalar(ast::BuiltinType::Void),
                               {scalar(ast::BuiltinType::Int),
                                array(ast::BuiltinType::Float)},
                               false, true, nullptr};
    functions_["putf"] = {scalar(ast::BuiltinType::Void),
                          {scalar(ast::BuiltinType::String)}, true, true, nullptr};
    functions_["starttime"] = {scalar(ast::BuiltinType::Void), {}, false, true,
                               nullptr};
    functions_["stoptime"] = {scalar(ast::BuiltinType::Void), {}, false, true,
                              nullptr};
    functions_["_sysy_starttime"] = {scalar(ast::BuiltinType::Void),
                                     {scalar(ast::BuiltinType::Int)}, false, true,
                                     nullptr};
    functions_["_sysy_stoptime"] = {scalar(ast::BuiltinType::Void),
                                    {scalar(ast::BuiltinType::Int)}, false, true,
                                    nullptr};
}

void SemanticAnalysis::pushScope() {
    scopes_.emplace_back();
    constants_.emplace_back();
}

void SemanticAnalysis::popScope() {
    scopes_.pop_back();
    constants_.pop_back();
}

void SemanticAnalysis::analyzeDecl(ast::Decl &decl) {
    if (auto *var = dynamic_cast<ast::VarDecl *>(&decl)) {
        analyzeVarDecl(*var);
        return;
    }
    if (auto *function = dynamic_cast<ast::FunctionDecl *>(&decl)) {
        analyzeFunctionDecl(*function);
    }
}

void SemanticAnalysis::analyzeVarDecl(ast::VarDecl &decl) {
    decl.type = buildArrayType(decl, true);
    const bool global = scopes_.size() == 1;
    const bool constOnly = global || decl.type.isConst;

    if (decl.initializer != nullptr) {
        analyzeInitializer(decl.type, *decl.initializer, constOnly, constOnly);
    }

    if (declareVariable(decl.name, decl.type, decl)) {
        if (decl.type.isConst && !decl.type.isArray() && decl.initializer != nullptr) {
            ConstValue value = evalConstExpr(*decl.initializer);
            if (value.known) {
                value = convertConstValue(value, decl.type);
                rememberConstant(decl.name, value);
            }
        }
    }
}

void SemanticAnalysis::analyzeFunctionDecl(ast::FunctionDecl &decl) {
    Function function;
    function.returnType = decl.returnType;
    function.decl = &decl;

    for (std::unique_ptr<ast::ParamDecl> &param : decl.params) {
        function.params.push_back(buildParamType(*param));
    }

    const bool declared = declareFunction(decl.name, function, decl);
    if (decl.name == "main") {
        if (sawMain_) {
            addError(decl, "main 函数只能定义一次");
        }
        sawMain_ = true;
        if (decl.returnType.base != ast::BuiltinType::Int || !decl.params.empty()) {
            addError(decl, "main 函数必须是 int main()");
        }
    }

    pushScope();
    const ast::Type previousReturn = currentFunctionReturn_;
    currentFunctionReturn_ = decl.returnType;

    for (std::unique_ptr<ast::ParamDecl> &param : decl.params) {
        declareVariable(param->name, param->type, *param);
    }

    if (declared && decl.body != nullptr) {
        analyzeCompoundStmt(*decl.body);
        if (decl.returnType.base != ast::BuiltinType::Void &&
            !stmtAlwaysReturns(*decl.body)) {
            addError(decl, "非 void 函数并非所有控制路径都返回值");
        }
    }

    currentFunctionReturn_ = previousReturn;
    popScope();
}

void SemanticAnalysis::analyzeStmt(ast::Stmt &stmt) {
    if (auto *compound = dynamic_cast<ast::CompoundStmt *>(&stmt)) {
        analyzeCompoundStmt(*compound);
        return;
    }

    if (auto *declStmt = dynamic_cast<ast::DeclStmt *>(&stmt)) {
        analyzeDeclStmt(*declStmt);
        return;
    }

    if (auto *exprStmt = dynamic_cast<ast::ExprStmt *>(&stmt)) {
        if (exprStmt->expression != nullptr) {
            inferExpr(*exprStmt->expression, false);
        }
        return;
    }

    if (auto *assign = dynamic_cast<ast::AssignStmt *>(&stmt)) {
        ast::Type lhs = inferExpr(*assign->target, false);
        ast::Type rhs = inferExpr(*assign->value, false);
        if (!isAssignableLValue(*assign->target)) {
            addError(*assign, "赋值左侧必须是变量左值");
        }
        if (lhs.isConst) {
            addError(*assign, "不能给 const 左值赋值");
        }
        if (lhs.isArray()) {
            addError(*assign, "赋值左侧必须定位到标量元素，不能是数组");
        }
        if (!compatible(lhs, rhs, false)) {
            addError(*assign, "赋值左右类型不兼容");
        }
        return;
    }

    if (auto *ifStmt = dynamic_cast<ast::IfStmt *>(&stmt)) {
        ast::Type condition = inferExpr(*ifStmt->condition, true);
        if (!condition.isScalar()) {
            addError(*ifStmt, "条件表达式必须是 int/float 标量");
        }
        if (ifStmt->thenBranch != nullptr) {
            analyzeStmt(*ifStmt->thenBranch);
        }
        if (ifStmt->elseBranch != nullptr) {
            analyzeStmt(*ifStmt->elseBranch);
        }
        return;
    }

    if (auto *whileStmt = dynamic_cast<ast::WhileStmt *>(&stmt)) {
        ast::Type condition = inferExpr(*whileStmt->condition, true);
        if (!condition.isScalar()) {
            addError(*whileStmt, "条件表达式必须是 int/float 标量");
        }
        ++loopDepth_;
        if (whileStmt->body != nullptr) {
            analyzeStmt(*whileStmt->body);
        }
        --loopDepth_;
        return;
    }

    if (dynamic_cast<ast::BreakStmt *>(&stmt) != nullptr ||
        dynamic_cast<ast::ContinueStmt *>(&stmt) != nullptr) {
        if (loopDepth_ == 0) {
            addError(stmt, "break/continue 只能出现在 while 循环内");
        }
        return;
    }

    if (auto *returnStmt = dynamic_cast<ast::ReturnStmt *>(&stmt)) {
        if (currentFunctionReturn_.base == ast::BuiltinType::Void) {
            if (returnStmt->value != nullptr) {
                addError(*returnStmt, "void 函数不能返回表达式");
            }
        } else if (returnStmt->value == nullptr) {
            addError(*returnStmt, "非 void 函数需要返回表达式");
        } else {
            ast::Type valueType = inferExpr(*returnStmt->value, false);
            if (!compatible(currentFunctionReturn_, valueType, false)) {
                addError(*returnStmt, "return 表达式类型与函数返回类型不兼容");
            }
        }
    }
}

void SemanticAnalysis::analyzeCompoundStmt(ast::CompoundStmt &stmt) {
    pushScope();
    for (std::unique_ptr<ast::Stmt> &child : stmt.statements) {
        analyzeStmt(*child);
    }
    popScope();
}

void SemanticAnalysis::analyzeDeclStmt(ast::DeclStmt &stmt) {
    for (std::unique_ptr<ast::VarDecl> &decl : stmt.declarations) {
        analyzeVarDecl(*decl);
    }
}

void SemanticAnalysis::analyzeInitializer(ast::Type target, ast::Expr &initializer,
                                          bool constOnly, bool checkIntRange,
                                          bool arrayElement) {
    if (target.isArray()) {
        if (dynamic_cast<ast::InitListExpr *>(&initializer) == nullptr) {
            addError(initializer, "数组初始化必须使用花括号列表");
            inferExpr(initializer, false, constOnly);
            return;
        }

        const std::size_t total = aggregateSize(target, 0);
        const std::size_t leaves = countInitLeaves(initializer);
        if (total != 0 && leaves > total) {
            addError(initializer, "数组初始化元素过多");
        }
        checkInitAggregate(target, initializer, constOnly, checkIntRange, 0, 0,
                           true);
        return;
    }

    if (auto *list = dynamic_cast<ast::InitListExpr *>(&initializer)) {
        setExprType(*list, target);
        addError(initializer, "标量初值不能是列表");
        for (std::unique_ptr<ast::Expr> &value : list->values) {
            inferExpr(*value, false, constOnly);
        }
        return;
    }

    ast::Type source = inferExpr(initializer, false, constOnly);
    if (!compatible(target, source, arrayElement)) {
        addError(initializer, "初始化表达式类型不兼容");
    } else if (checkIntRange) {
        const ConstValue value = evalConstExpr(initializer);
        if (value.known && !checkValueRange(target, value)) {
            addError(initializer, "初始化表达式超出 " +
                                      std::string(ast::toString(target.base)) + " 范围");
        }
    }
}

ast::Type SemanticAnalysis::inferExpr(ast::Expr &expr, bool allowConditionOps,
                                      bool constOnly) {
    if (auto *literal = dynamic_cast<ast::IntegerLiteral *>(&expr)) {
        ast::Type type;
        type.base = ast::BuiltinType::Int;
        return setExprType(*literal, type);
    }
    if (auto *literal = dynamic_cast<ast::FloatLiteral *>(&expr)) {
        ast::Type type;
        type.base = ast::BuiltinType::Float;
        return setExprType(*literal, type);
    }
    if (auto *literal = dynamic_cast<ast::StringLiteral *>(&expr)) {
        ast::Type type;
        type.base = ast::BuiltinType::String;
        return setExprType(*literal, type);
    }
    if (auto *ref = dynamic_cast<ast::DeclRefExpr *>(&expr)) {
        return inferDeclRef(*ref, constOnly);
    }
    if (auto *subscript = dynamic_cast<ast::ArraySubscriptExpr *>(&expr)) {
        return inferArraySubscript(*subscript, allowConditionOps, constOnly);
    }
    if (auto *unary = dynamic_cast<ast::UnaryOperator *>(&expr)) {
        return inferUnary(*unary, allowConditionOps, constOnly);
    }
    if (auto *binary = dynamic_cast<ast::BinaryOperator *>(&expr)) {
        return inferBinary(*binary, allowConditionOps, constOnly);
    }
    if (auto *call = dynamic_cast<ast::CallExpr *>(&expr)) {
        return inferCall(*call, allowConditionOps, constOnly);
    }
    if (auto *list = dynamic_cast<ast::InitListExpr *>(&expr)) {
        for (std::unique_ptr<ast::Expr> &value : list->values) {
            inferExpr(*value, allowConditionOps, constOnly);
        }
        return setExprType(*list, {});
    }

    return setExprType(expr, {});
}

ast::Type SemanticAnalysis::inferDeclRef(ast::DeclRefExpr &expr, bool constOnly) {
    const Symbol *symbol = resolveVariable(expr.name);
    if (symbol == nullptr) {
        addError(expr, "使用未定义变量或常量 '" + expr.name + "'");
        return setExprType(expr, {});
    }

    expr.referencedDecl = symbol->decl;
    if (constOnly && !symbol->type.isConst) {
        addError(expr, "常量表达式只能引用常量，不能引用 '" + expr.name + "'");
    }
    return setExprType(expr, symbol->type);
}

ast::Type
SemanticAnalysis::inferArraySubscript(ast::ArraySubscriptExpr &expr,
                                      bool allowConditionOps,
                                      bool constOnly) {
    ast::Type base = inferExpr(*expr.base, allowConditionOps, constOnly);
    ast::Type index = inferExpr(*expr.index, false, constOnly);
    if (index.base != ast::BuiltinType::Int || index.isArray()) {
        addError(*expr.index, "数组下标必须是 int 表达式");
    }

    if (!base.isArray()) {
        if (const ast::DeclRefExpr *root = subscriptRoot(expr); root != nullptr) {
            const Symbol *symbol = resolveVariable(root->name);
            if (symbol != nullptr) {
                if (!symbol->type.isArray()) {
                    addError(expr, "标量 '" + root->name + "' 不能使用数组下标");
                } else {
                    addError(expr, "数组 '" + root->name + "' 下标数量过多");
                }
            } else {
                addError(expr, "只能对数组表达式使用下标");
            }
        } else {
            addError(expr, "只能对数组表达式使用下标");
        }
        return setExprType(expr, {});
    }

    if (!base.shape.empty()) {
        base.shape.erase(base.shape.begin());
    }
    return setExprType(expr, base);
}

ast::Type SemanticAnalysis::inferUnary(ast::UnaryOperator &expr,
                                       bool allowConditionOps, bool constOnly) {
    if (expr.opcode == ast::UnaryOpcode::LogicalNot && !allowConditionOps) {
        addError(expr, "普通 Exp 中不能使用 ! 运算");
    }

    ast::Type operand = inferExpr(*expr.operand, allowConditionOps, constOnly);
    if (!operand.isScalar()) {
        addError(expr, "一元运算对象必须是 int/float 标量");
        return setExprType(expr, {});
    }

    ast::Type result = operand;
    result.isConst = false;
    if (expr.opcode == ast::UnaryOpcode::LogicalNot) {
        result.base = ast::BuiltinType::Int;
        result.shape.clear();
    }
    return setExprType(expr, result);
}

ast::Type SemanticAnalysis::inferBinary(ast::BinaryOperator &expr,
                                        bool allowConditionOps, bool constOnly) {
    if (isConditionOperator(expr.opcode) && !allowConditionOps) {
        addError(expr, "普通 Exp 中不能使用关系或逻辑运算");
    }

    ast::Type lhs = inferExpr(*expr.lhs, allowConditionOps, constOnly);
    ast::Type rhs = inferExpr(*expr.rhs, allowConditionOps, constOnly);
    ast::Type result;

    if (!lhs.isScalar() || !rhs.isScalar()) {
        addError(expr, "二元运算两侧必须是 int/float 标量");
        return setExprType(expr, result);
    }

    if (isArithmeticOperator(expr.opcode)) {
        if (expr.opcode == ast::BinaryOpcode::Mod &&
            (lhs.base != ast::BuiltinType::Int || rhs.base != ast::BuiltinType::Int)) {
            addError(expr, "取模运算只能用于 int");
        }
        result.base =
            (lhs.base == ast::BuiltinType::Float || rhs.base == ast::BuiltinType::Float)
                ? ast::BuiltinType::Float
                : ast::BuiltinType::Int;
        if (expr.opcode == ast::BinaryOpcode::Mod) {
            result.base = ast::BuiltinType::Int;
        }
        return setExprType(expr, result);
    }

    result.base = ast::BuiltinType::Int;
    return setExprType(expr, result);
}

ast::Type SemanticAnalysis::inferCall(ast::CallExpr &expr, bool allowConditionOps,
                                      bool constOnly) {
    (void)allowConditionOps;
    const Function *function = resolveFunction(expr.callee);
    if (function == nullptr) {
        addError(expr, "调用未定义函数 '" + expr.callee + "'");
        return setExprType(expr, {});
    }

    expr.calleeDecl = function->decl;
    expr.isBuiltin = function->builtin;
    if (constOnly) {
        addError(expr, "常量表达式中不能调用函数 '" + expr.callee + "'");
    }

    std::vector<ast::Type> args;
    for (std::unique_ptr<ast::Expr> &arg : expr.arguments) {
        args.push_back(inferExpr(*arg, false));
    }

    if (!function->variadic && args.size() != function->params.size()) {
        addError(expr, "函数 '" + expr.callee + "' 实参数量不匹配");
    } else if (function->variadic && args.size() < function->params.size()) {
        addError(expr, "函数 '" + expr.callee + "' 实参数量不足");
    }

    const std::size_t fixed = std::min(args.size(), function->params.size());
    for (std::size_t i = 0; i < fixed; ++i) {
        if (!compatible(function->params[i], args[i], false)) {
            addError(expr, "函数 '" + expr.callee + "' 第 " +
                               std::to_string(i + 1) + " 个参数类型不匹配");
        }
    }
    return setExprType(expr, function->returnType);
}

ast::Type SemanticAnalysis::buildParamType(ast::ParamDecl &param) {
    if (!param.isArray) {
        return param.type;
    }

    ast::Type type = param.type;
    type.shape.clear();
    type.shape.push_back(-1);
    for (std::unique_ptr<ast::Expr> &dimension : param.dimensions) {
        ast::Type dimensionType = inferExpr(*dimension, false, true);
        ConstValue value = evalConstExpr(*dimension);
        if (dimensionType.base != ast::BuiltinType::Int || dimensionType.isArray()) {
            addError(*dimension, "数组形参后续维度必须是整型常量表达式");
            type.shape.push_back(-1);
            continue;
        }
        if (!value.known || value.base != ast::BuiltinType::Int || value.intValue < 0 ||
            value.intValue > std::numeric_limits<int>::max()) {
            addError(*dimension, "数组形参后续维度必须能在编译期求得非负整数");
            type.shape.push_back(-1);
            continue;
        }
        type.shape.push_back(value.intValue);
    }
    param.type = type;
    return type;
}

ast::Type SemanticAnalysis::buildArrayType(ast::VarDecl &decl, bool constOnly) {
    ast::Type type = decl.type;
    type.shape.clear();
    for (std::unique_ptr<ast::Expr> &dimension : decl.dimensions) {
        ast::Type dimensionType = inferExpr(*dimension, false, constOnly);
        ConstValue value = evalConstExpr(*dimension);
        if (dimensionType.base != ast::BuiltinType::Int || dimensionType.isArray()) {
            addError(*dimension, "数组维度必须是 int 常量表达式");
            type.shape.push_back(-1);
            continue;
        }
        if (!value.known || value.base != ast::BuiltinType::Int || value.intValue < 0 ||
            value.intValue > std::numeric_limits<int>::max()) {
            addError(*dimension, "数组维度必须能在编译期求得非负整数");
            type.shape.push_back(-1);
            continue;
        }
        type.shape.push_back(value.intValue);
    }
    return type;
}

bool SemanticAnalysis::declareVariable(const std::string &name, ast::Type type,
                                          const ast::Decl &decl) {
    auto &scope = scopes_.back();
    if (scope.find(name) != scope.end()) {
        addError(decl, "重复定义变量或常量 '" + name + "'");
        return false;
    }
    if (scopes_.size() == 1 && functions_.find(name) != functions_.end()) {
        addError(decl, "全局标识符 '" + name + "' 与函数重名");
        return false;
    }
    scope.emplace(name, Symbol{type, &decl});
    return true;
}

const SemanticAnalysis::Symbol *
SemanticAnalysis::resolveVariable(const std::string &name) const {
    for (std::size_t i = scopes_.size(); i > 0; --i) {
        const auto &scope = scopes_[i - 1];
        auto found = scope.find(name);
        if (found != scope.end()) {
            return &found->second;
        }
    }
    return nullptr;
}

void SemanticAnalysis::rememberConstant(const std::string &name,
                                           ConstValue value) {
    if (!constants_.empty()) {
        constants_.back()[name] = value;
    }
}

const SemanticAnalysis::ConstValue *
SemanticAnalysis::resolveConstant(const std::string &name) const {
    for (std::size_t i = constants_.size(); i > 0; --i) {
        const auto &scope = constants_[i - 1];
        auto found = scope.find(name);
        if (found != scope.end()) {
            return &found->second;
        }
    }
    return nullptr;
}

bool SemanticAnalysis::declareFunction(const std::string &name, Function function,
                                          const ast::FunctionDecl &decl) {
    if (functions_.find(name) != functions_.end()) {
        addError(decl, "重复定义函数 '" + name + "'");
        return false;
    }
    if (!scopes_.empty() && scopes_.front().find(name) != scopes_.front().end()) {
        addError(decl, "函数 '" + name + "' 与全局变量或常量重名");
        return false;
    }
    functions_.emplace(name, function);
    return true;
}

const SemanticAnalysis::Function *
SemanticAnalysis::resolveFunction(const std::string &name) const {
    auto found = functions_.find(name);
    if (found == functions_.end()) {
        return nullptr;
    }
    return &found->second;
}

bool SemanticAnalysis::compatible(ast::Type target, ast::Type source,
                                     bool arrayInitializer) const {
    if (target.base == ast::BuiltinType::Invalid ||
        source.base == ast::BuiltinType::Invalid) {
        return false;
    }
    if (target.isArray() || source.isArray()) {
        if (!target.isArray() || !source.isArray() || target.base != source.base ||
            target.shape.size() != source.shape.size()) {
            return false;
        }
        for (std::size_t i = 1; i < target.shape.size(); ++i) {
            const long long targetDim = target.shape[i];
            const long long sourceDim = source.shape[i];
            if (targetDim > 0 && sourceDim > 0 && targetDim != sourceDim) {
                return false;
            }
        }
        return true;
    }
    if (target.base == ast::BuiltinType::String ||
        source.base == ast::BuiltinType::String) {
        return target.base == source.base;
    }
    if (target.base == ast::BuiltinType::Void ||
        source.base == ast::BuiltinType::Void) {
        return target.base == source.base;
    }
    if (arrayInitializer && target.base == ast::BuiltinType::Int &&
        source.base == ast::BuiltinType::Float) {
        return false;
    }
    return isNumeric(target.base) && isNumeric(source.base);
}

bool SemanticAnalysis::isNumeric(ast::BuiltinType type) const {
    return type == ast::BuiltinType::Int || type == ast::BuiltinType::Float;
}

bool SemanticAnalysis::isConditionOperator(ast::BinaryOpcode opcode) const {
    return opcode == ast::BinaryOpcode::Less ||
           opcode == ast::BinaryOpcode::Greater ||
           opcode == ast::BinaryOpcode::LessEqual ||
           opcode == ast::BinaryOpcode::GreaterEqual ||
           opcode == ast::BinaryOpcode::Equal ||
           opcode == ast::BinaryOpcode::NotEqual ||
           opcode == ast::BinaryOpcode::LogicalAnd ||
           opcode == ast::BinaryOpcode::LogicalOr;
}

bool SemanticAnalysis::isArithmeticOperator(ast::BinaryOpcode opcode) const {
    return opcode == ast::BinaryOpcode::Mul || opcode == ast::BinaryOpcode::Div ||
           opcode == ast::BinaryOpcode::Mod || opcode == ast::BinaryOpcode::Add ||
           opcode == ast::BinaryOpcode::Sub;
}

std::size_t SemanticAnalysis::aggregateSize(ast::Type type,
                                               std::size_t level) const {
    std::size_t result = 1;
    for (std::size_t i = level; i < type.shape.size(); ++i) {
        if (type.shape[i] <= 0) {
            return 0;
        }
        result *= static_cast<std::size_t>(type.shape[i]);
    }
    return result;
}

std::size_t SemanticAnalysis::initializerChildLevel(ast::Type type,
                                                       std::size_t parentLevel,
                                                       std::size_t parentStart,
                                                       std::size_t pos) const {
    if (parentLevel + 1 >= type.shape.size()) {
        return type.shape.size();
    }

    const std::size_t relative = pos - parentStart;
    for (std::size_t level = parentLevel + 1; level < type.shape.size(); ++level) {
        const std::size_t size = aggregateSize(type, level);
        if (size != 0 && relative % size == 0) {
            return level;
        }
    }
    return type.shape.size();
}

std::size_t SemanticAnalysis::countInitLeaves(ast::Expr &initializer) const {
    if (auto *list = dynamic_cast<ast::InitListExpr *>(&initializer)) {
        std::size_t count = 0;
        for (const std::unique_ptr<ast::Expr> &value : list->values) {
            count += countInitLeaves(*value);
        }
        return count;
    }
    return 1;
}

std::size_t SemanticAnalysis::checkInitAggregate(ast::Type target,
                                                 ast::Expr &initializer,
                                                 bool constOnly,
                                                 bool checkIntRange,
                                                 std::size_t level,
                                                 std::size_t start,
                                                 bool root) {
    if (auto *list = dynamic_cast<ast::InitListExpr *>(&initializer)) {
        setExprType(*list, typeAtLevel(target, level));
        if (level >= target.shape.size()) {
            addError(*list, "标量初值不能是列表");
            for (std::unique_ptr<ast::Expr> &value : list->values) {
                inferExpr(*value, false, constOnly);
            }
            return start;
        }

        const std::size_t total = aggregateSize(target, 0);
        const std::size_t currentSize = aggregateSize(target, level);
        const std::size_t aggregateEnd = std::min(total, start + currentSize);
        std::size_t pos = start;
        for (std::unique_ptr<ast::Expr> &value : list->values) {
            if (pos >= aggregateEnd) {
                addError(*value, "初始化列表元素过多");
                break;
            }

            if (dynamic_cast<ast::InitListExpr *>(value.get()) == nullptr) {
                pos = checkInitAggregate(target, *value, constOnly, checkIntRange,
                                         level + 1, pos, false);
                continue;
            }

            const std::size_t childLevel =
                initializerChildLevel(target, level, start, pos);
            if (childLevel >= target.shape.size()) {
                addError(*value, "标量初值不能是列表");
                inferExpr(*value, false, constOnly);
                continue;
            }
            pos = checkInitAggregate(target, *value, constOnly, checkIntRange,
                                     childLevel, pos, false);
        }
        return start + currentSize;
    }

    if (root) {
        addError(initializer, "数组初始化必须使用花括号列表");
        inferExpr(initializer, false, constOnly);
        return start;
    }

    const ast::Type source = inferExpr(initializer, false, constOnly);
    const ast::Type element = scalarElementType(target);
    if (!compatible(element, source, true)) {
        addError(initializer, "初始化列表元素类型不兼容");
    }
    if (checkIntRange) {
        const ConstValue value = evalConstExpr(initializer);
        if (value.known && !checkValueRange(element, value)) {
            addError(initializer, "初始化列表元素超出 " +
                                      std::string(ast::toString(element.base)) + " 范围");
        }
    }
    return start + 1;
}

ast::Type SemanticAnalysis::scalarElementType(ast::Type type) const {
    type.shape.clear();
    return type;
}

ast::Type SemanticAnalysis::typeAtLevel(ast::Type type, std::size_t level) const {
    if (level >= type.shape.size()) {
        type.shape.clear();
        return type;
    }

    type.shape.erase(type.shape.begin(),
                     type.shape.begin() + static_cast<std::ptrdiff_t>(level));
    return type;
}

const ast::DeclRefExpr *SemanticAnalysis::subscriptRoot(const ast::Expr &expr) const {
    const ast::Expr *current = &expr;
    while (auto *subscript = dynamic_cast<const ast::ArraySubscriptExpr *>(current)) {
        current = subscript->base.get();
    }
    return dynamic_cast<const ast::DeclRefExpr *>(current);
}

bool SemanticAnalysis::isAssignableLValue(const ast::Expr &expr) const {
    return dynamic_cast<const ast::DeclRefExpr *>(&expr) != nullptr ||
           dynamic_cast<const ast::ArraySubscriptExpr *>(&expr) != nullptr;
}

bool SemanticAnalysis::stmtAlwaysReturns(const ast::Stmt &stmt) const {
    if (dynamic_cast<const ast::ReturnStmt *>(&stmt) != nullptr) {
        return true;
    }

    if (auto *compound = dynamic_cast<const ast::CompoundStmt *>(&stmt)) {
        for (const std::unique_ptr<ast::Stmt> &child : compound->statements) {
            if (stmtAlwaysReturns(*child)) {
                return true;
            }
        }
        return false;
    }

    if (auto *ifStmt = dynamic_cast<const ast::IfStmt *>(&stmt)) {
        return ifStmt->thenBranch != nullptr && ifStmt->elseBranch != nullptr &&
               stmtAlwaysReturns(*ifStmt->thenBranch) &&
               stmtAlwaysReturns(*ifStmt->elseBranch);
    }

    if (auto *whileStmt = dynamic_cast<const ast::WhileStmt *>(&stmt)) {
        if (whileStmt->condition == nullptr || whileStmt->body == nullptr) {
            return false;
        }

        ast::Expr &condition = *const_cast<ast::Expr *>(whileStmt->condition.get());
        const ConstValue value = const_cast<SemanticAnalysis *>(this)->evalConstExpr(
            condition);
        return value.known && isConstTruthy(value) &&
               (stmtAlwaysReturns(*whileStmt->body) ||
                !stmtMayBreakCurrentLoop(*whileStmt->body));
    }

    return false;
}

bool SemanticAnalysis::stmtMayBreakCurrentLoop(const ast::Stmt &stmt) const {
    if (dynamic_cast<const ast::BreakStmt *>(&stmt) != nullptr) {
        return true;
    }

    if (dynamic_cast<const ast::ReturnStmt *>(&stmt) != nullptr) {
        return false;
    }

    if (auto *compound = dynamic_cast<const ast::CompoundStmt *>(&stmt)) {
        for (const std::unique_ptr<ast::Stmt> &child : compound->statements) {
            if (stmtMayBreakCurrentLoop(*child)) {
                return true;
            }
        }
        return false;
    }

    if (auto *ifStmt = dynamic_cast<const ast::IfStmt *>(&stmt)) {
        return (ifStmt->thenBranch != nullptr &&
                stmtMayBreakCurrentLoop(*ifStmt->thenBranch)) ||
               (ifStmt->elseBranch != nullptr &&
                stmtMayBreakCurrentLoop(*ifStmt->elseBranch));
    }

    if (dynamic_cast<const ast::WhileStmt *>(&stmt) != nullptr) {
        return false;
    }

    return false;
}

SemanticAnalysis::ConstValue SemanticAnalysis::evalConstExpr(ast::Expr &expr) {
    if (auto *literal = dynamic_cast<ast::IntegerLiteral *>(&expr)) {
        return {true, ast::BuiltinType::Int, parseIntLiteral(literal->text), 0.0};
    }
    if (auto *literal = dynamic_cast<ast::FloatLiteral *>(&expr)) {
        return {true, ast::BuiltinType::Float, 0, parseFloatLiteral(literal->text)};
    }
    if (auto *ref = dynamic_cast<ast::DeclRefExpr *>(&expr)) {
        if (ref->referencedDecl == nullptr) {
            inferDeclRef(*ref, true);
        }
        const ConstValue *value = resolveConstant(ref->name);
        return value == nullptr ? ConstValue{} : *value;
    }
    if (auto *subscript = dynamic_cast<ast::ArraySubscriptExpr *>(&expr)) {
        return evalConstLValue(*subscript);
    }
    if (auto *unary = dynamic_cast<ast::UnaryOperator *>(&expr)) {
        return evalConstUnary(unary->opcode, evalConstExpr(*unary->operand));
    }
    if (auto *binary = dynamic_cast<ast::BinaryOperator *>(&expr)) {
        return evalConstBinary(binary->opcode, evalConstExpr(*binary->lhs),
                               evalConstExpr(*binary->rhs));
    }
    return {};
}

SemanticAnalysis::ConstValue SemanticAnalysis::evalConstLValue(ast::Expr &expr) {
    if (auto *ref = dynamic_cast<ast::DeclRefExpr *>(&expr)) {
        if (ref->referencedDecl == nullptr) {
            inferDeclRef(*ref, true);
        }
        const ConstValue *value = resolveConstant(ref->name);
        return value == nullptr ? ConstValue{} : *value;
    }

    auto *subscript = dynamic_cast<ast::ArraySubscriptExpr *>(&expr);
    if (subscript == nullptr) {
        return {};
    }

    std::vector<long long> indices;
    ast::Expr *current = &expr;
    while (auto *nested = dynamic_cast<ast::ArraySubscriptExpr *>(current)) {
        const ConstValue index = evalConstExpr(*nested->index);
        if (!index.known || index.base != ast::BuiltinType::Int) {
            return {};
        }
        indices.push_back(index.intValue);
        current = nested->base.get();
    }

    auto *root = dynamic_cast<ast::DeclRefExpr *>(current);
    if (root == nullptr) {
        return {};
    }
    if (root->referencedDecl == nullptr) {
        inferDeclRef(*root, true);
    }

    auto *decl = dynamic_cast<const ast::VarDecl *>(root->referencedDecl);
    if (decl == nullptr || !decl->type.isConst || !decl->type.isArray()) {
        return {};
    }

    std::reverse(indices.begin(), indices.end());
    if (indices.size() != decl->type.shape.size()) {
        return {};
    }
    return evalConstArrayElement(*decl, indices);
}

SemanticAnalysis::ConstValue
SemanticAnalysis::evalConstArrayElement(const ast::VarDecl &decl,
                                           const std::vector<long long> &indices) {
    if (decl.initializer == nullptr || indices.size() != decl.type.shape.size()) {
        return {};
    }

    const std::size_t total = aggregateSize(decl.type, 0);
    if (total == 0) {
        return {};
    }

    std::size_t offset = 0;
    std::size_t stride = total;
    for (std::size_t i = 0; i < indices.size(); ++i) {
        const long long dim = decl.type.shape[i];
        const long long index = indices[i];
        if (dim < 0 || index < 0 || index >= dim) {
            return {};
        }
        stride /= static_cast<std::size_t>(dim == 0 ? 1 : dim);
        offset += static_cast<std::size_t>(index) * stride;
    }

    ConstValue zero;
    zero.known = true;
    zero.base = decl.type.base;

    std::vector<ConstValue> values(total, zero);
    flattenConstArrayInit(decl.type, *decl.initializer, 0, 0, true, values);
    if (offset >= values.size()) {
        return {};
    }
    return convertConstValue(values[offset], scalarElementType(decl.type));
}

std::size_t SemanticAnalysis::flattenConstArrayInit(
    ast::Type target, ast::Expr &initializer, std::size_t level, std::size_t start,
    bool root, std::vector<ConstValue> &values) const {
    if (auto *list = dynamic_cast<ast::InitListExpr *>(&initializer)) {
        if (level >= target.shape.size()) {
            return start;
        }

        const std::size_t total = aggregateSize(target, 0);
        const std::size_t currentSize = aggregateSize(target, level);
        const std::size_t aggregateEnd = std::min(total, start + currentSize);
        std::size_t pos = start;
        for (std::unique_ptr<ast::Expr> &value : list->values) {
            if (pos >= aggregateEnd) {
                break;
            }

            if (dynamic_cast<ast::InitListExpr *>(value.get()) == nullptr) {
                pos = flattenConstArrayInit(target, *value, level + 1, pos, false,
                                            values);
                continue;
            }

            const std::size_t childLevel =
                initializerChildLevel(target, level, start, pos);
            if (childLevel >= target.shape.size()) {
                continue;
            }
            pos = flattenConstArrayInit(target, *value, childLevel, pos, false, values);
        }
        return start + currentSize;
    }

    if (root || start >= values.size()) {
        return start;
    }

    ConstValue value = const_cast<SemanticAnalysis *>(this)->evalConstExpr(initializer);
    if (value.known) {
        values[start] = value;
    }
    return start + 1;
}

SemanticAnalysis::ConstValue
SemanticAnalysis::evalConstUnary(ast::UnaryOpcode opcode, ConstValue value) const {
    if (!value.known || !isNumeric(value.base)) {
        return {};
    }
    if (opcode == ast::UnaryOpcode::Minus) {
        if (value.base == ast::BuiltinType::Float) {
            value.floatValue = -value.floatValue;
        } else {
            value.intValue = -value.intValue;
        }
    } else if (opcode == ast::UnaryOpcode::LogicalNot) {
        const bool truthy = value.base == ast::BuiltinType::Float
                                ? value.floatValue != 0.0
                                : value.intValue != 0;
        value.base = ast::BuiltinType::Int;
        value.intValue = truthy ? 0 : 1;
        value.floatValue = 0.0;
    }
    return value;
}

SemanticAnalysis::ConstValue SemanticAnalysis::evalConstBinary(
    ast::BinaryOpcode opcode, ConstValue lhs, ConstValue rhs) const {
    if (!lhs.known || !rhs.known || !isNumeric(lhs.base) || !isNumeric(rhs.base)) {
        return {};
    }

    auto asDouble = [](ConstValue value) {
        return value.base == ast::BuiltinType::Float
                   ? value.floatValue
                   : static_cast<double>(value.intValue);
    };

    ConstValue result;
    result.known = true;
    if (opcode == ast::BinaryOpcode::Mod) {
        if (lhs.base != ast::BuiltinType::Int || rhs.base != ast::BuiltinType::Int ||
            rhs.intValue == 0) {
            return {};
        }
        result.base = ast::BuiltinType::Int;
        result.intValue = lhs.intValue % rhs.intValue;
        return result;
    }

    if (isArithmeticOperator(opcode)) {
        result.base =
            (lhs.base == ast::BuiltinType::Float || rhs.base == ast::BuiltinType::Float)
                ? ast::BuiltinType::Float
                : ast::BuiltinType::Int;
        if (result.base == ast::BuiltinType::Float) {
            const double l = asDouble(lhs);
            const double r = asDouble(rhs);
            if (opcode == ast::BinaryOpcode::Div && r == 0.0) {
                return {};
            }
            if (opcode == ast::BinaryOpcode::Mul) {
                result.floatValue = l * r;
            } else if (opcode == ast::BinaryOpcode::Div) {
                result.floatValue = l / r;
            } else if (opcode == ast::BinaryOpcode::Add) {
                result.floatValue = l + r;
            } else {
                result.floatValue = l - r;
            }
            return result;
        }

        if (opcode == ast::BinaryOpcode::Div && rhs.intValue == 0) {
            return {};
        }
        if (opcode == ast::BinaryOpcode::Mul) {
            result.intValue = lhs.intValue * rhs.intValue;
        } else if (opcode == ast::BinaryOpcode::Div) {
            result.intValue = lhs.intValue / rhs.intValue;
        } else if (opcode == ast::BinaryOpcode::Add) {
            result.intValue = lhs.intValue + rhs.intValue;
        } else {
            result.intValue = lhs.intValue - rhs.intValue;
        }
        return result;
    }

    const double l = asDouble(lhs);
    const double r = asDouble(rhs);
    result.base = ast::BuiltinType::Int;
    if (opcode == ast::BinaryOpcode::Less) {
        result.intValue = l < r;
    } else if (opcode == ast::BinaryOpcode::Greater) {
        result.intValue = l > r;
    } else if (opcode == ast::BinaryOpcode::LessEqual) {
        result.intValue = l <= r;
    } else if (opcode == ast::BinaryOpcode::GreaterEqual) {
        result.intValue = l >= r;
    } else if (opcode == ast::BinaryOpcode::Equal) {
        result.intValue = l == r;
    } else if (opcode == ast::BinaryOpcode::NotEqual) {
        result.intValue = l != r;
    } else if (opcode == ast::BinaryOpcode::LogicalAnd) {
        result.intValue = (l != 0.0) && (r != 0.0);
    } else if (opcode == ast::BinaryOpcode::LogicalOr) {
        result.intValue = (l != 0.0) || (r != 0.0);
    } else {
        result.known = false;
    }
    return result;
}

SemanticAnalysis::ConstValue
SemanticAnalysis::convertConstValue(ConstValue value, ast::Type target) const {
    if (!value.known) {
        return value;
    }
    if (target.base == ast::BuiltinType::Int && value.base == ast::BuiltinType::Float) {
        value.intValue = static_cast<long long>(value.floatValue);
        value.floatValue = 0.0;
        value.base = ast::BuiltinType::Int;
    } else if (target.base == ast::BuiltinType::Float &&
               value.base == ast::BuiltinType::Int) {
        value.floatValue = static_cast<double>(value.intValue);
        value.intValue = 0;
        value.base = ast::BuiltinType::Float;
    }
    return value;
}

bool SemanticAnalysis::isIntRange(ConstValue value) const {
    if (!value.known) {
        return false;
    }
    if (value.base == ast::BuiltinType::Int) {
        return value.intValue >= std::numeric_limits<int>::min() &&
               value.intValue <= std::numeric_limits<int>::max();
    }
    if (value.base == ast::BuiltinType::Float) {
        return value.floatValue >=
                   static_cast<double>(std::numeric_limits<int>::min()) &&
               value.floatValue <=
                   static_cast<double>(std::numeric_limits<int>::max());
    }
    return false;
}

bool SemanticAnalysis::isFloatRange(ConstValue value) const {
    if (!value.known || !isNumeric(value.base)) {
        return false;
    }

    const double numeric = value.base == ast::BuiltinType::Float
                               ? value.floatValue
                               : static_cast<double>(value.intValue);
    return std::isfinite(numeric) &&
           numeric >= -static_cast<double>(std::numeric_limits<float>::max()) &&
           numeric <= static_cast<double>(std::numeric_limits<float>::max());
}

bool SemanticAnalysis::checkValueRange(ast::Type target, ConstValue value) const {
    if (target.base == ast::BuiltinType::Int) {
        return isIntRange(value);
    }
    if (target.base == ast::BuiltinType::Float) {
        return isFloatRange(value);
    }
    return true;
}

bool SemanticAnalysis::isConstTruthy(ConstValue value) const {
    if (!value.known || !isNumeric(value.base)) {
        return false;
    }
    return value.base == ast::BuiltinType::Float ? value.floatValue != 0.0
                                                 : value.intValue != 0;
}

ast::Type SemanticAnalysis::setExprType(ast::Expr &expr, ast::Type type) const {
    expr.inferredType = type;
    expr.hasType = true;
    return type;
}

std::string SemanticAnalysis::location(ast::SourceLocation loc) const {
    if (loc.line <= 0) {
        return "";
    }
    std::ostringstream out;
    out << "第 " << loc.line << ':' << loc.column << " 行列";
    return out.str();
}

void SemanticAnalysis::addError(ast::SourceLocation loc,
                                const std::string &message) {
    const std::string where = location(loc);
    errors_.push_back(where.empty() ? message : where + " " + message);
}

void SemanticAnalysis::addError(const ast::ASTNode &node,
                                const std::string &message) {
    addError(node.location(), message);
}

long long SemanticAnalysis::parseIntLiteral(const std::string &text) {
    try {
        return std::stoll(text, nullptr, 0);
    } catch (const std::out_of_range &) {
        return std::numeric_limits<long long>::max();
    } catch (const std::exception &) {
        return 0;
    }
}

double SemanticAnalysis::parseFloatLiteral(const std::string &text) {
    errno = 0;
    char *end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (errno != 0 || end == text.c_str()) {
        return 0.0;
    }
    return value;
}
