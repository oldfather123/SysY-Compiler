#include "frontend/AST.h"

#include <utility>

namespace ast {

namespace {

void appendArrayShape(std::string &result, const std::vector<long long> &shape) {
    for (long long dimension : shape) {
        result += '[';
        if (dimension > 0) {
            result += std::to_string(dimension);
        }
        result += ']';
    }
}

} // namespace

bool Type::isArray() const noexcept {
    return !isPointer && !shape.empty();
}

bool Type::isScalar() const noexcept {
    return !isPointer && shape.empty() &&
           (base == BuiltinType::Int || base == BuiltinType::Float);
}

ASTNode::ASTNode(SourceLocation location) : location_(location) {}

const SourceLocation &ASTNode::location() const noexcept {
    return location_;
}

TranslationUnit::TranslationUnit(SourceLocation location) : ASTNode(location) {}

VarDecl::VarDecl(SourceLocation location, Type type, std::string name)
    : Decl(location), type(std::move(type)), name(std::move(name)) {}

ParamDecl::ParamDecl(SourceLocation location, Type type, std::string name)
    : Decl(location), type(std::move(type)), name(std::move(name)) {}

FunctionDecl::FunctionDecl(SourceLocation location, Type returnType, std::string name)
    : Decl(location), returnType(std::move(returnType)), name(std::move(name)) {}

IfStmt::IfStmt(SourceLocation location) : Stmt(location) {}

WhileStmt::WhileStmt(SourceLocation location) : Stmt(location) {}

ReturnStmt::ReturnStmt(SourceLocation location, std::unique_ptr<Expr> value)
    : Stmt(location), value(std::move(value)) {}

IntegerLiteral::IntegerLiteral(SourceLocation location, std::string text)
    : Expr(location), text(std::move(text)) {}

FloatLiteral::FloatLiteral(SourceLocation location, std::string text)
    : Expr(location), text(std::move(text)) {}

StringLiteral::StringLiteral(SourceLocation location, std::string text)
    : Expr(location), text(std::move(text)) {}

ImplicitCastExpr::ImplicitCastExpr(SourceLocation location, CastKind kind,
                                   Type targetType,
                                   std::unique_ptr<Expr> subExpr)
    : Expr(location), kind(kind), targetType(std::move(targetType)),
      subExpr(std::move(subExpr)) {}

ParenExpr::ParenExpr(SourceLocation location, std::unique_ptr<Expr> subExpr)
    : Expr(location), subExpr(std::move(subExpr)) {}

DeclRefExpr::DeclRefExpr(SourceLocation location, std::string name)
    : Expr(location), name(std::move(name)) {}

ArraySubscriptExpr::ArraySubscriptExpr(SourceLocation location,
                                       std::unique_ptr<Expr> base,
                                       std::unique_ptr<Expr> index)
    : Expr(location), base(std::move(base)), index(std::move(index)) {}

UnaryOperator::UnaryOperator(SourceLocation location, UnaryOpcode opcode,
                             std::unique_ptr<Expr> operand)
    : Expr(location), opcode(opcode), operand(std::move(operand)) {}

BinaryOperator::BinaryOperator(SourceLocation location, BinaryOpcode opcode,
                               std::unique_ptr<Expr> lhs,
                               std::unique_ptr<Expr> rhs)
    : Expr(location), opcode(opcode), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

CallExpr::CallExpr(SourceLocation location, std::string callee)
    : Expr(location), callee(std::move(callee)) {}

ImplicitValueInitExpr::ImplicitValueInitExpr(SourceLocation location, Type targetType)
    : Expr(location), targetType(std::move(targetType)) {}

const char *toString(BuiltinType type) noexcept {
    switch (type) {
    case BuiltinType::Void:
        return "void";
    case BuiltinType::Int:
        return "int";
    case BuiltinType::Float:
        return "float";
    case BuiltinType::String:
        return "string";
    case BuiltinType::Invalid:
        return "invalid";
    }
    return "invalid";
}

const char *toString(UnaryOpcode opcode) noexcept {
    switch (opcode) {
    case UnaryOpcode::Plus:
        return "+";
    case UnaryOpcode::Minus:
        return "-";
    case UnaryOpcode::LogicalNot:
        return "!";
    }
    return "?";
}

const char *toString(BinaryOpcode opcode) noexcept {
    switch (opcode) {
    case BinaryOpcode::Assign:
        return "=";
    case BinaryOpcode::Mul:
        return "*";
    case BinaryOpcode::Div:
        return "/";
    case BinaryOpcode::Mod:
        return "%";
    case BinaryOpcode::Add:
        return "+";
    case BinaryOpcode::Sub:
        return "-";
    case BinaryOpcode::Less:
        return "<";
    case BinaryOpcode::Greater:
        return ">";
    case BinaryOpcode::LessEqual:
        return "<=";
    case BinaryOpcode::GreaterEqual:
        return ">=";
    case BinaryOpcode::Equal:
        return "==";
    case BinaryOpcode::NotEqual:
        return "!=";
    case BinaryOpcode::LogicalAnd:
        return "&&";
    case BinaryOpcode::LogicalOr:
        return "||";
    }
    return "?";
}

const char *toString(CastKind kind) noexcept {
    switch (kind) {
    case CastKind::LValueToRValue:
        return "LValueToRValue";
    case CastKind::ArrayToPointerDecay:
        return "ArrayToPointerDecay";
    case CastKind::IntToFloat:
        return "IntegralToFloating";
    case CastKind::FloatToInt:
        return "FloatingToIntegral";
    case CastKind::IntToBool:
        return "IntegralToBoolean";
    case CastKind::FloatToBool:
        return "FloatingToBoolean";
    }
    return "?";
}

std::string typeToString(const Type &type) {
    std::string result;
    if (type.isConst) {
        result += "const ";
    }
    result += toString(type.base);
    if (type.isPointer) {
        if (type.shape.empty()) {
            result += " *";
        } else {
            result += " (*)";
        }
    }
    appendArrayShape(result, type.shape);
    return result;
}

} // namespace ast
