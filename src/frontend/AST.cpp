#include "frontend/AST.h"

#include <utility>

namespace ast {

bool Type::isArray() const {
    return !shape.empty();
}

bool Type::isScalar() const {
    return shape.empty() && (base == BuiltinType::Int || base == BuiltinType::Float);
}

ASTNode::ASTNode(SourceLocation location) : location_(location) {}

SourceLocation ASTNode::location() const {
    return location_;
}

TranslationUnit::TranslationUnit(SourceLocation location) : ASTNode(location) {}

VarDecl::VarDecl(SourceLocation location, Type type, std::string name)
    : Decl(location), type(type), name(std::move(name)) {}

ParamDecl::ParamDecl(SourceLocation location, Type type, std::string name)
    : Decl(location), type(type), name(std::move(name)) {}

FunctionDecl::FunctionDecl(SourceLocation location, Type returnType, std::string name)
    : Decl(location), returnType(returnType), name(std::move(name)) {}

ExprStmt::ExprStmt(SourceLocation location, std::unique_ptr<Expr> expression)
    : Stmt(location), expression(std::move(expression)) {}

AssignStmt::AssignStmt(SourceLocation location, std::unique_ptr<Expr> target,
                       std::unique_ptr<Expr> value)
    : Stmt(location), target(std::move(target)), value(std::move(value)) {}

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

const char *toString(BuiltinType type) {
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

const char *toString(UnaryOpcode opcode) {
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

const char *toString(BinaryOpcode opcode) {
    switch (opcode) {
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

std::string typeToString(Type type) {
    std::string result;
    if (type.isConst) {
        result += "const ";
    }
    result += toString(type.base);
    for (long long dimension : type.shape) {
        result += '[';
        if (dimension > 0) {
            result += std::to_string(dimension);
        }
        result += ']';
    }
    return result;
}

} // namespace ast
