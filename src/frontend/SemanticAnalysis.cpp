#include "frontend/SemanticAnalysis.h"

#include <algorithm>
#include <cmath>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

std::string initLocationKey(std::size_t level, std::size_t start) {
    return std::to_string(level) + ":" + std::to_string(start);
}

ast::Type arrayDecayPointerType(ast::Type type) {
    type.isPointer = true;
    if (!type.shape.empty()) {
        type.shape.erase(type.shape.begin());
    }
    return type;
}

bool isFloating(ast::BuiltinType type) {
    return type == ast::BuiltinType::Float;
}

const ast::Expr *stripArrayDecay(const ast::Expr *expr) {
    const ast::Expr *current = expr;
    while (const auto *paren = dynamic_cast<const ast::ParenExpr *>(current)) {
        current = paren->subExpr.get();
    }
    while (const auto *cast = dynamic_cast<const ast::ImplicitCastExpr *>(current)) {
        if (cast->kind != ast::CastKind::ArrayToPointerDecay &&
            cast->kind != ast::CastKind::LValueToRValue) {
            break;
        }
        current = cast->subExpr.get();
        while (const auto *paren = dynamic_cast<const ast::ParenExpr *>(current)) {
            current = paren->subExpr.get();
        }
    }
    return current;
}

ast::Expr *stripArrayDecay(ast::Expr *expr) {
    ast::Expr *current = expr;
    while (auto *paren = dynamic_cast<ast::ParenExpr *>(current)) {
        current = paren->subExpr.get();
    }
    while (auto *cast = dynamic_cast<ast::ImplicitCastExpr *>(current)) {
        if (cast->kind != ast::CastKind::ArrayToPointerDecay &&
            cast->kind != ast::CastKind::LValueToRValue) {
            break;
        }
        current = cast->subExpr.get();
        while (auto *paren = dynamic_cast<ast::ParenExpr *>(current)) {
            current = paren->subExpr.get();
        }
    }
    return current;
}

} // namespace

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
        type.isPointer = true;
        return type;
    };

    auto runtime = [](ast::Type returnType, std::vector<ast::Type> params = {},
                      bool variadic = false) {
        Function function;
        function.returnType = returnType;
        function.params = std::move(params);
        function.variadic = variadic;
        function.builtin = true;
        return function;
    };

    functions_["getint"] = runtime(scalar(ast::BuiltinType::Int));
    functions_["getch"] = runtime(scalar(ast::BuiltinType::Int));
    functions_["getfloat"] = runtime(scalar(ast::BuiltinType::Float));
    functions_["getarray"] = runtime(scalar(ast::BuiltinType::Int),
                                     {array(ast::BuiltinType::Int)});
    functions_["getfarray"] = runtime(scalar(ast::BuiltinType::Int),
                                      {array(ast::BuiltinType::Float)});
    functions_["putint"] = runtime(scalar(ast::BuiltinType::Void),
                                   {scalar(ast::BuiltinType::Int)});
    functions_["putch"] = runtime(scalar(ast::BuiltinType::Void),
                                  {scalar(ast::BuiltinType::Int)});
    functions_["putfloat"] = runtime(scalar(ast::BuiltinType::Void),
                                     {scalar(ast::BuiltinType::Float)});
    functions_["putarray"] = runtime(scalar(ast::BuiltinType::Void),
                                     {scalar(ast::BuiltinType::Int),
                                      array(ast::BuiltinType::Int)});
    functions_["putfarray"] = runtime(scalar(ast::BuiltinType::Void),
                                      {scalar(ast::BuiltinType::Int),
                                       array(ast::BuiltinType::Float)});
    functions_["putf"] = runtime(scalar(ast::BuiltinType::Void),
                                 {scalar(ast::BuiltinType::String)}, true);
    functions_["starttime"] = runtime(scalar(ast::BuiltinType::Void));
    functions_["stoptime"] = runtime(scalar(ast::BuiltinType::Void));
    functions_["_sysy_starttime"] = runtime(scalar(ast::BuiltinType::Void),
                                            {scalar(ast::BuiltinType::Int)});
    functions_["_sysy_stoptime"] = runtime(scalar(ast::BuiltinType::Void),
                                           {scalar(ast::BuiltinType::Int)});
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
        analyzeInitializer(decl.type, decl.initializer, constOnly, constOnly);
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

    if (dynamic_cast<ast::NullStmt *>(&stmt) != nullptr) {
        return;
    }

    if (auto *ifStmt = dynamic_cast<ast::IfStmt *>(&stmt)) {
        ast::Type condition = inferExpr(*ifStmt->condition, true);
        if (!condition.isScalar()) {
            addError(*ifStmt, "条件表达式必须是 int/float 标量");
        } else {
            applyLValueToRValue(ifStmt->condition);
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
        } else {
            applyLValueToRValue(whileStmt->condition);
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
            applyLValueToRValue(returnStmt->value);
            valueType = returnStmt->value->inferredType;
            if (!compatible(currentFunctionReturn_, valueType, false)) {
                addError(*returnStmt, "return 表达式类型与函数返回类型不兼容");
            } else {
                applyImplicitCast(returnStmt->value, currentFunctionReturn_);
            }
        }
        return;
    }

    if (auto *expr = dynamic_cast<ast::Expr *>(&stmt)) {
        inferExpr(*expr, false);
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

void SemanticAnalysis::analyzeInitializer(ast::Type target,
                                          std::unique_ptr<ast::Expr> &initializer,
                                          bool constOnly, bool checkIntRange,
                                          bool arrayElement) {
    ast::Expr &node = *initializer;
    if (target.isArray()) {
        if (dynamic_cast<ast::InitListExpr *>(&node) == nullptr) {
            addError(node, "数组初始化必须使用花括号列表");
            inferExpr(node, false, constOnly);
            return;
        }

        const std::size_t total = aggregateSize(target, 0);
        const std::size_t leaves = countInitLeaves(node);
        if (total != 0 && leaves > total) {
            addError(node, "数组初始化元素过多");
        }
        checkInitAggregate(target, initializer, constOnly, checkIntRange, 0, 0,
                           true);
        normalizeArrayInitializer(target, initializer);
        return;
    }

    if (auto *list = dynamic_cast<ast::InitListExpr *>(&node)) {
        setExprType(*list, target);
        addError(node, "标量初值不能是列表");
        for (std::unique_ptr<ast::Expr> &value : list->values) {
            inferExpr(*value, false, constOnly);
        }
        return;
    }

    ast::Type source = inferExpr(node, false, constOnly);
    applyLValueToRValue(initializer);
    source = initializer->inferredType;
    if (!compatible(target, source, arrayElement)) {
        addError(node, "初始化表达式类型不兼容");
    } else {
        if (checkIntRange) {
            const ConstValue value = evalConstExpr(node);
            if (value.known && !checkValueRange(target, value)) {
                addError(node, "初始化表达式超出 " +
                                   std::string(ast::toString(target.base)) + " 范围");
            }
        }
        applyImplicitCast(initializer, target);
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
    if (auto *cast = dynamic_cast<ast::ImplicitCastExpr *>(&expr)) {
        inferExpr(*cast->subExpr, allowConditionOps, constOnly);
        return setExprType(*cast, cast->targetType);
    }
    if (auto *paren = dynamic_cast<ast::ParenExpr *>(&expr)) {
        return inferParen(*paren, allowConditionOps, constOnly);
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
    if (auto *implicitInit = dynamic_cast<ast::ImplicitValueInitExpr *>(&expr)) {
        return setExprType(*implicitInit, implicitInit->targetType);
    }
    if (auto *list = dynamic_cast<ast::InitListExpr *>(&expr)) {
        for (std::unique_ptr<ast::Expr> &value : list->values) {
            inferExpr(*value, allowConditionOps, constOnly);
        }
        if (list->arrayFiller != nullptr) {
            inferExpr(*list->arrayFiller, allowConditionOps, constOnly);
        }
        if (list->hasType) {
            return list->inferredType;
        }
        return setExprType(*list, {});
    }

    return setExprType(expr, {});
}

ast::Type SemanticAnalysis::inferParen(ast::ParenExpr &expr, bool allowConditionOps,
                                       bool constOnly) {
    ast::Type type = inferExpr(*expr.subExpr, allowConditionOps, constOnly);
    return setExprType(expr, type);
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
    if (base.isArray()) {
        applyArrayToPointerDecay(expr.base);
        base = expr.base->inferredType;
    } else if (base.isPointer) {
        applyLValueToRValue(expr.base);
        base = expr.base->inferredType;
    }
    ast::Type index = inferExpr(*expr.index, false, constOnly);
    applyLValueToRValue(expr.index);
    index = expr.index->inferredType;
    if (index.base != ast::BuiltinType::Int || !index.isScalar()) {
        addError(*expr.index, "数组下标必须是 int 表达式");
    }

    if (!base.isArray() && !base.isPointer) {
        if (const ast::DeclRefExpr *root = subscriptRoot(expr); root != nullptr) {
            const Symbol *symbol = resolveVariable(root->name);
            if (symbol != nullptr) {
                if (!symbol->type.isArray() && !symbol->type.isPointer) {
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

    if (base.isPointer) {
        base.isPointer = false;
    } else if (!base.shape.empty()) {
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
    applyLValueToRValue(expr.operand);
    operand = expr.operand->inferredType;
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
    if (expr.opcode == ast::BinaryOpcode::Assign) {
        ast::Type lhs = inferExpr(*expr.lhs, false, constOnly);
        ast::Type rhs = inferExpr(*expr.rhs, false, constOnly);
        applyLValueToRValue(expr.rhs);
        rhs = expr.rhs->inferredType;

        if (!isAssignableLValue(*expr.lhs)) {
            addError(expr, "赋值左侧必须是变量左值");
        }
        if (lhs.isConst) {
            addError(expr, "不能给 const 左值赋值");
        }
        if (lhs.isArray()) {
            addError(expr, "赋值左侧必须定位到标量元素，不能是数组");
        }
        if (!compatible(lhs, rhs, false)) {
            addError(expr, "赋值左右类型不兼容");
        } else {
            applyImplicitCast(expr.rhs, lhs);
        }

        ast::Type result = lhs;
        result.isConst = false;
        return setExprType(expr, result);
    }

    if (isConditionOperator(expr.opcode) && !allowConditionOps) {
        addError(expr, "普通 Exp 中不能使用关系或逻辑运算");
    }

    ast::Type lhs = inferExpr(*expr.lhs, allowConditionOps, constOnly);
    ast::Type rhs = inferExpr(*expr.rhs, allowConditionOps, constOnly);
    applyLValueToRValue(expr.lhs);
    applyLValueToRValue(expr.rhs);
    lhs = expr.lhs->inferredType;
    rhs = expr.rhs->inferredType;
    ast::Type result;

    if (!lhs.isScalar() || !rhs.isScalar()) {
        addError(expr, "二元运算两侧必须是 int/float 标量");
        return setExprType(expr, result);
    }

    if (expr.opcode == ast::BinaryOpcode::LogicalAnd ||
        expr.opcode == ast::BinaryOpcode::LogicalOr) {
        result.base = ast::BuiltinType::Int;
        return setExprType(expr, result);
    }

    if (isArithmeticOperator(expr.opcode)) {
        if (expr.opcode == ast::BinaryOpcode::Mod &&
            (lhs.base != ast::BuiltinType::Int || rhs.base != ast::BuiltinType::Int)) {
            addError(expr, "取模运算只能用于 int");
        }
        if (expr.opcode != ast::BinaryOpcode::Mod &&
            (isFloating(lhs.base) || isFloating(rhs.base))) {
            ast::Type floatType;
            floatType.base = ast::BuiltinType::Float;
            applyImplicitCast(expr.lhs, floatType);
            applyImplicitCast(expr.rhs, floatType);
            lhs = expr.lhs->inferredType;
            rhs = expr.rhs->inferredType;
        }
        result.base =
            (isFloating(lhs.base) || isFloating(rhs.base))
                ? ast::BuiltinType::Float
                : ast::BuiltinType::Int;
        if (expr.opcode == ast::BinaryOpcode::Mod) {
            result.base = ast::BuiltinType::Int;
        }
        return setExprType(expr, result);
    }

    if (expr.opcode != ast::BinaryOpcode::LogicalAnd &&
        expr.opcode != ast::BinaryOpcode::LogicalOr &&
        (isFloating(lhs.base) || isFloating(rhs.base))) {
        ast::Type floatType;
        floatType.base = ast::BuiltinType::Float;
        applyImplicitCast(expr.lhs, floatType);
        applyImplicitCast(expr.rhs, floatType);
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
        args.push_back(inferExpr(*arg, false, constOnly));
        if (args.back().isArray()) {
            applyArrayToPointerDecay(arg);
            args.back() = arg->inferredType;
            continue;
        }
        applyLValueToRValue(arg);
        args.back() = arg->inferredType;
    }

    if (!function->variadic && args.size() != function->params.size()) {
        addError(expr, "函数 '" + expr.callee + "' 实参数量不匹配");
    } else if (function->variadic && args.size() < function->params.size()) {
        addError(expr, "函数 '" + expr.callee + "' 实参数量不足");
    }

    const std::size_t fixed = std::min(args.size(), function->params.size());
    for (std::size_t i = 0; i < fixed; ++i) {
        if (!compatibleCallArgument(*function, i, function->params[i], args[i])) {
            addError(expr, "函数 '" + expr.callee + "' 第 " +
                               std::to_string(i + 1) + " 个参数类型不匹配");
        } else {
            applyImplicitCast(expr.arguments[i], function->params[i]);
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
    type.isPointer = true;
    param.type = type;
    param.dimensions.clear();
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
    decl.dimensions.clear();
    return type;
}

bool SemanticAnalysis::declareVariable(const std::string &name, ast::Type type,
                                          ast::Decl &decl) {
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
                                          ast::FunctionDecl &decl) {
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
    if (target.isPointer || source.isPointer) {
        if (!target.isPointer || !source.isPointer || target.base != source.base ||
            target.shape.size() != source.shape.size()) {
            return false;
        }
        for (std::size_t i = 0; i < target.shape.size(); ++i) {
            const long long targetDim = target.shape[i];
            const long long sourceDim = source.shape[i];
            if (targetDim > 0 && sourceDim > 0 && targetDim != sourceDim) {
                return false;
            }
        }
        return true;
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
        isFloating(source.base)) {
        return false;
    }
    return isNumeric(target.base) && isNumeric(source.base);
}

bool SemanticAnalysis::compatibleCallArgument(const Function &function,
                                              std::size_t paramIndex,
                                              ast::Type target,
                                              ast::Type source) const {
    if (compatible(target, source, false)) {
        return true;
    }

    const bool flatArrayRuntimeParam =
        function.builtin &&
        ((function.decl == nullptr && function.params.size() == 1 &&
          paramIndex == 0) ||
         (function.decl == nullptr && function.params.size() == 2 &&
          paramIndex == 1));
    if (!flatArrayRuntimeParam || !target.isPointer || !source.isPointer) {
        return false;
    }

    return target.base == source.base && target.shape.empty();
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
                                                 std::unique_ptr<ast::Expr> &initializer,
                                                 bool constOnly,
                                                 bool checkIntRange,
                                                 std::size_t level,
                                                 std::size_t start,
                                                 bool root) {
    ast::Expr &node = *initializer;
    if (auto *list = dynamic_cast<ast::InitListExpr *>(&node)) {
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
                pos = checkInitAggregate(target, value, constOnly, checkIntRange,
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
            pos = checkInitAggregate(target, value, constOnly, checkIntRange,
                                     childLevel, pos, false);
        }
        return start + currentSize;
    }

    if (root) {
        addError(node, "数组初始化必须使用花括号列表");
        inferExpr(node, false, constOnly);
        return start;
    }

    ast::Type source = inferExpr(node, false, constOnly);
    applyLValueToRValue(initializer);
    source = initializer->inferredType;
    const ast::Type element = scalarElementType(target);
    if (!compatible(element, source, true)) {
        addError(node, "初始化列表元素类型不兼容");
    } else {
        if (checkIntRange) {
            const ConstValue value = evalConstExpr(node);
            if (value.known && !checkValueRange(element, value)) {
                addError(node, "初始化列表元素超出 " +
                                   std::string(ast::toString(element.base)) + " 范围");
            }
        }
        applyImplicitCast(initializer, element);
    }
    return start + 1;
}

std::size_t SemanticAnalysis::collectNormalizedInitLeaves(
    ast::Type target, std::unique_ptr<ast::Expr> &initializer, std::size_t level,
    std::size_t start, bool root, std::vector<std::unique_ptr<ast::Expr>> &leaves,
    std::unordered_map<std::string, ast::SourceLocation> &listLocations) const {
    if (auto *list = dynamic_cast<ast::InitListExpr *>(initializer.get())) {
        if (level >= target.shape.size()) {
            return start;
        }

        listLocations.emplace(initLocationKey(level, start), list->location());

        const std::size_t total = aggregateSize(target, 0);
        const std::size_t currentSize = aggregateSize(target, level);
        const std::size_t aggregateEnd = std::min(total, start + currentSize);
        std::size_t pos = start;
        for (std::unique_ptr<ast::Expr> &value : list->values) {
            if (pos >= aggregateEnd) {
                break;
            }

            if (dynamic_cast<ast::InitListExpr *>(value.get()) == nullptr) {
                pos = collectNormalizedInitLeaves(target, value, level + 1, pos, false,
                                                 leaves, listLocations);
                continue;
            }

            const std::size_t childLevel =
                initializerChildLevel(target, level, start, pos);
            if (childLevel >= target.shape.size()) {
                continue;
            }
            pos = collectNormalizedInitLeaves(target, value, childLevel, pos, false,
                                              leaves, listLocations);
        }
        return start + currentSize;
    }

    if (root || start >= leaves.size()) {
        return start;
    }

    leaves[start] = std::move(initializer);
    return start + 1;
}

void SemanticAnalysis::normalizeArrayInitializer(ast::Type target,
                                                 std::unique_ptr<ast::Expr> &initializer) {
    auto *list = dynamic_cast<ast::InitListExpr *>(initializer.get());
    if (list == nullptr || !target.isArray()) {
        return;
    }

    const std::size_t total = aggregateSize(target, 0);
    std::vector<std::unique_ptr<ast::Expr>> leaves(total);
    std::unordered_map<std::string, ast::SourceLocation> listLocations;
    collectNormalizedInitLeaves(target, initializer, 0, 0, true, leaves, listLocations);
    initializer = buildNormalizedInitList(target, 0, 0, leaves, listLocations);
}

std::unique_ptr<ast::Expr> SemanticAnalysis::buildNormalizedInitList(
    ast::Type target, std::size_t level, std::size_t start,
    std::vector<std::unique_ptr<ast::Expr>> &leaves,
    const std::unordered_map<std::string, ast::SourceLocation> &listLocations) const {
    const ast::Type currentType = typeAtLevel(target, level);
    ast::SourceLocation loc;
    if (const auto found = listLocations.find(initLocationKey(level, start));
        found != listLocations.end()) {
        loc = found->second;
    } else {
        const std::size_t currentSize = aggregateSize(target, level);
        const std::size_t end = std::min(leaves.size(), start + currentSize);
        for (std::size_t i = start; i < end; ++i) {
            if (leaves[i] != nullptr) {
                loc = leaves[i]->location();
                break;
            }
        }
    }

    auto list = std::make_unique<ast::InitListExpr>(loc);
    setExprType(*list, currentType);

    const bool leafAggregate = level + 1 >= target.shape.size();
    if (leafAggregate) {
        const std::size_t slots =
            currentType.shape.empty() || currentType.shape.front() <= 0
                ? 0
                : static_cast<std::size_t>(currentType.shape.front());
        std::size_t explicitCount = 0;
        for (std::size_t i = 0; i < slots && start + i < leaves.size(); ++i) {
            if (leaves[start + i] != nullptr) {
                ++explicitCount;
            }
        }
        if (explicitCount < slots) {
            auto filler =
                std::make_unique<ast::ImplicitValueInitExpr>(ast::SourceLocation{},
                                                             scalarElementType(target));
            setExprType(*filler, scalarElementType(target));
            list->arrayFiller = std::move(filler);
        }
        for (std::size_t i = 0; i < slots && start + i < leaves.size(); ++i) {
            if (leaves[start + i] != nullptr) {
                list->valueOffsets.push_back(i);
                list->values.push_back(std::move(leaves[start + i]));
            }
        }
        return list;
    }

    const std::size_t childCount =
        currentType.shape.empty() || currentType.shape.front() <= 0
            ? 0
            : static_cast<std::size_t>(currentType.shape.front());
    const std::size_t childSize = aggregateSize(target, level + 1);
    std::size_t explicitChildren = 0;
    for (std::size_t child = 0; child < childCount; ++child) {
        const std::size_t childStart = start + child * childSize;
        const std::size_t childEnd = std::min(leaves.size(), childStart + childSize);
        bool hasExplicit =
            listLocations.find(initLocationKey(level + 1, childStart)) !=
            listLocations.end();
        for (std::size_t pos = childStart; pos < childEnd; ++pos) {
            if (leaves[pos] != nullptr) {
                hasExplicit = true;
                break;
            }
        }
        if (!hasExplicit) {
            continue;
        }
        ++explicitChildren;
        list->valueOffsets.push_back(child * childSize);
        list->values.push_back(
            buildNormalizedInitList(target, level + 1, childStart, leaves, listLocations));
    }

    if (explicitChildren < childCount) {
        auto filler = std::make_unique<ast::ImplicitValueInitExpr>(
            ast::SourceLocation{}, typeAtLevel(target, level + 1));
        setExprType(*filler, typeAtLevel(target, level + 1));
        list->arrayFiller = std::move(filler);
    }
    return list;
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
    const ast::Expr *current = stripArrayDecay(&expr);
    while (auto *subscript = dynamic_cast<const ast::ArraySubscriptExpr *>(current)) {
        current = stripArrayDecay(subscript->base.get());
    }
    return dynamic_cast<const ast::DeclRefExpr *>(current);
}

bool SemanticAnalysis::isAssignableLValue(const ast::Expr &expr) const {
    if (const auto *paren = dynamic_cast<const ast::ParenExpr *>(&expr)) {
        return isAssignableLValue(*paren->subExpr);
    }
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
    if (auto *cast = dynamic_cast<ast::ImplicitCastExpr *>(&expr)) {
        ConstValue value = evalConstExpr(*cast->subExpr);
        if (cast->kind == ast::CastKind::LValueToRValue ||
            cast->kind == ast::CastKind::ArrayToPointerDecay) {
            return value;
        }
        if (cast->kind == ast::CastKind::IntToBool ||
            cast->kind == ast::CastKind::FloatToBool) {
            if (!value.known || !isNumeric(value.base)) {
                return {};
            }
            ConstValue result;
            result.known = true;
            result.base = ast::BuiltinType::Int;
            result.intValue = isConstTruthy(value) ? 1 : 0;
            return result;
        }
        return convertConstValue(value, cast->targetType);
    }
    if (auto *paren = dynamic_cast<ast::ParenExpr *>(&expr)) {
        return evalConstExpr(*paren->subExpr);
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
    if (auto *implicitInit = dynamic_cast<ast::ImplicitValueInitExpr *>(&expr)) {
        ConstValue zero;
        zero.known = true;
        zero.base = implicitInit->targetType.base;
        return zero;
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
    ast::Expr *current = stripArrayDecay(&expr);
    while (auto *nested = dynamic_cast<ast::ArraySubscriptExpr *>(current)) {
        const ConstValue index = evalConstExpr(*nested->index);
        if (!index.known || index.base != ast::BuiltinType::Int) {
            return {};
        }
        indices.push_back(index.intValue);
        current = stripArrayDecay(nested->base.get());
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

    if (dynamic_cast<ast::ImplicitValueInitExpr *>(&initializer) != nullptr) {
        return start + 1;
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
        if (isFloating(value.base)) {
            value.floatValue = -value.floatValue;
        } else {
            value.intValue = -value.intValue;
        }
    } else if (opcode == ast::UnaryOpcode::LogicalNot) {
        const bool truthy = isFloating(value.base)
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
    if (opcode == ast::BinaryOpcode::Assign) {
        return {};
    }
    if (!lhs.known || !rhs.known || !isNumeric(lhs.base) || !isNumeric(rhs.base)) {
        return {};
    }

    auto asFloat = [](ConstValue value) {
        return isFloating(value.base)
                   ? value.floatValue
                   : static_cast<float>(value.intValue);
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
            (isFloating(lhs.base) || isFloating(rhs.base))
                ? ast::BuiltinType::Float
                : ast::BuiltinType::Int;
        if (isFloating(result.base)) {
            const float l = asFloat(lhs);
            const float r = asFloat(rhs);
            if (opcode == ast::BinaryOpcode::Div && r == 0.0f) {
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

    const float l = asFloat(lhs);
    const float r = asFloat(rhs);
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
        result.intValue = (l != 0.0f) && (r != 0.0f);
    } else if (opcode == ast::BinaryOpcode::LogicalOr) {
        result.intValue = (l != 0.0f) || (r != 0.0f);
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
    if (target.base == ast::BuiltinType::Int && isFloating(value.base)) {
        value.intValue = static_cast<long long>(value.floatValue);
        value.floatValue = 0.0;
        value.base = ast::BuiltinType::Int;
    } else if (isFloating(target.base) && value.base == ast::BuiltinType::Int) {
        value.floatValue = static_cast<float>(value.intValue);
        value.intValue = 0;
        value.base = target.base;
    } else if (isFloating(target.base) && isFloating(value.base)) {
        value.base = target.base;
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
    if (isFloating(value.base)) {
        return value.floatValue >=
                   static_cast<float>(std::numeric_limits<int>::min()) &&
               value.floatValue <=
                   static_cast<float>(std::numeric_limits<int>::max());
    }
    return false;
}

void SemanticAnalysis::applyArrayToPointerDecay(std::unique_ptr<ast::Expr> &expr) {
    if (expr == nullptr) {
        return;
    }

    if (auto *cast = dynamic_cast<ast::ImplicitCastExpr *>(expr.get())) {
        if (cast->kind == ast::CastKind::ArrayToPointerDecay) {
            return;
        }
    }

    ast::Type source = expr->hasType ? expr->inferredType : inferExpr(*expr, false);
    if (!source.isArray()) {
        return;
    }

    ast::Type target = arrayDecayPointerType(source);
    auto cast = std::make_unique<ast::ImplicitCastExpr>(
        expr->location(), ast::CastKind::ArrayToPointerDecay, target, std::move(expr));
    setExprType(*cast, target);
    expr = std::move(cast);
}

void SemanticAnalysis::applyLValueToRValue(std::unique_ptr<ast::Expr> &expr) {
    if (expr == nullptr) {
        return;
    }

    if (dynamic_cast<ast::ImplicitCastExpr *>(expr.get()) != nullptr) {
        return;
    }

    ast::Type source = expr->hasType ? expr->inferredType : inferExpr(*expr, false);
    if (!source.isScalar() && !source.isPointer) {
        return;
    }

    if (!isAssignableLValue(*expr)) {
        return;
    }

    source.isConst = false;
    auto cast = std::make_unique<ast::ImplicitCastExpr>(
        expr->location(), ast::CastKind::LValueToRValue, source, std::move(expr));
    setExprType(*cast, source);
    expr = std::move(cast);
}

void SemanticAnalysis::applyImplicitCast(std::unique_ptr<ast::Expr> &expr,
                                         ast::Type target) {
    if (expr == nullptr || target.isArray()) {
        return;
    }

    ast::Type source = expr->hasType ? expr->inferredType : inferExpr(*expr, false);
    if (source.isArray() || source.base == ast::BuiltinType::Invalid ||
        target.base == ast::BuiltinType::Invalid ||
        source.base == target.base) {
        return;
    }
    if (!isNumeric(source.base) || !isNumeric(target.base)) {
        return;
    }
    if (auto *cast = dynamic_cast<ast::ImplicitCastExpr *>(expr.get())) {
        if (cast->kind == ast::CastKind::IntToFloat ||
            cast->kind == ast::CastKind::FloatToInt) {
            return;
        }
    }

    ast::CastKind kind;
    if (source.base == ast::BuiltinType::Int && isFloating(target.base)) {
        kind = ast::CastKind::IntToFloat;
    } else if (isFloating(source.base) && target.base == ast::BuiltinType::Int) {
        kind = ast::CastKind::FloatToInt;
    } else {
        return;
    }

    target.shape.clear();
    target.isConst = false;
    auto cast = std::make_unique<ast::ImplicitCastExpr>(expr->location(), kind, target,
                                                        std::move(expr));
    setExprType(*cast, target);
    expr = std::move(cast);
}

bool SemanticAnalysis::isFloatRange(ConstValue value) const {
    if (!value.known || !isNumeric(value.base)) {
        return false;
    }

    const float numeric = isFloating(value.base)
                               ? value.floatValue
                               : static_cast<float>(value.intValue);
    return std::isfinite(numeric) &&
           numeric >= -std::numeric_limits<float>::max() &&
           numeric <= std::numeric_limits<float>::max();
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
    return isFloating(value.base) ? value.floatValue != 0.0 : value.intValue != 0;
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

float SemanticAnalysis::parseFloatLiteral(const std::string &text) {
    errno = 0;
    char *end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (errno != 0 || end == text.c_str()) {
        return 0.0f;
    }
    return value;
}
