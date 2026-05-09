#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace ast {

struct SourceLocation {
    int line = 0;
    int column = 0;
};

class CompoundStmt;

enum class BuiltinType {
    Void,
    Int,
    Float,
    String,
    Invalid,
};

struct Type {
    BuiltinType base = BuiltinType::Invalid;
    bool isConst = false;
    bool isPointer = false;
    std::vector<long long> shape;

    [[nodiscard]] bool isArray() const noexcept;
    [[nodiscard]] bool isScalar() const noexcept;
};

enum class UnaryOpcode {
    Plus,
    Minus,
    LogicalNot,
};

enum class BinaryOpcode {
    Assign,
    Mul,
    Div,
    Mod,
    Add,
    Sub,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    Equal,
    NotEqual,
    LogicalAnd,
    LogicalOr,
};

enum class CastKind {
    LValueToRValue,
    ArrayToPointerDecay,
    IntToFloat,
    FloatToInt,
    IntToBool,
    FloatToBool,
};

class ASTNode {
public:
    explicit ASTNode(SourceLocation location = {});
    virtual ~ASTNode() = default;

    [[nodiscard]] const SourceLocation &location() const noexcept;

private:
    SourceLocation location_;
};

class Decl : public ASTNode {
public:
    using ASTNode::ASTNode;
};

class Stmt : public ASTNode {
public:
    using ASTNode::ASTNode;
};

class Expr : public Stmt {
public:
    using Stmt::Stmt;

    Type inferredType;
    bool hasType = false;
};

class TranslationUnit final : public ASTNode {
public:
    explicit TranslationUnit(SourceLocation location = {});

    std::vector<std::unique_ptr<Decl>> declarations;
};

class VarDecl final : public Decl {
public:
    VarDecl(SourceLocation location, Type type, std::string name);

    Type type;
    std::string name;
    std::vector<std::unique_ptr<Expr>> dimensions;
    std::unique_ptr<Expr> initializer;
};

class ParamDecl final : public Decl {
public:
    ParamDecl(SourceLocation location, Type type, std::string name);

    Type type;
    std::string name;
    bool isArray = false;
    std::vector<std::unique_ptr<Expr>> dimensions;
};

class FunctionDecl final : public Decl {
public:
    FunctionDecl(SourceLocation location, Type returnType, std::string name);

    Type returnType;
    std::string name;
    std::vector<std::unique_ptr<ParamDecl>> params;
    std::unique_ptr<CompoundStmt> body;
};

class CompoundStmt final : public Stmt {
public:
    using Stmt::Stmt;

    std::vector<std::unique_ptr<Stmt>> statements;
};

class DeclStmt final : public Stmt {
public:
    using Stmt::Stmt;

    std::vector<std::unique_ptr<VarDecl>> declarations;
};

class NullStmt final : public Stmt {
public:
    using Stmt::Stmt;
};

class IfStmt final : public Stmt {
public:
    explicit IfStmt(SourceLocation location);

    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
};

class WhileStmt final : public Stmt {
public:
    explicit WhileStmt(SourceLocation location);

    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
};

class BreakStmt final : public Stmt {
public:
    using Stmt::Stmt;
};

class ContinueStmt final : public Stmt {
public:
    using Stmt::Stmt;
};

class ReturnStmt final : public Stmt {
public:
    explicit ReturnStmt(SourceLocation location, std::unique_ptr<Expr> value = nullptr);

    std::unique_ptr<Expr> value;
};

class IntegerLiteral final : public Expr {
public:
    IntegerLiteral(SourceLocation location, std::string text);

    std::string text;
};

class FloatLiteral final : public Expr {
public:
    FloatLiteral(SourceLocation location, std::string text);

    std::string text;
};

class StringLiteral final : public Expr {
public:
    StringLiteral(SourceLocation location, std::string text);

    std::string text;
};

class ImplicitCastExpr final : public Expr {
public:
    ImplicitCastExpr(SourceLocation location, CastKind kind, Type targetType,
                     std::unique_ptr<Expr> subExpr);

    CastKind kind;
    Type targetType;
    std::unique_ptr<Expr> subExpr;
};

class ParenExpr final : public Expr {
public:
    ParenExpr(SourceLocation location, std::unique_ptr<Expr> subExpr);

    std::unique_ptr<Expr> subExpr;
};

class DeclRefExpr final : public Expr {
public:
    DeclRefExpr(SourceLocation location, std::string name);

    std::string name;
    const Decl *referencedDecl = nullptr;
};

class ArraySubscriptExpr final : public Expr {
public:
    ArraySubscriptExpr(SourceLocation location, std::unique_ptr<Expr> base,
                       std::unique_ptr<Expr> index);

    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
};

class UnaryOperator final : public Expr {
public:
    UnaryOperator(SourceLocation location, UnaryOpcode opcode,
                  std::unique_ptr<Expr> operand);

    UnaryOpcode opcode;
    std::unique_ptr<Expr> operand;
};

class BinaryOperator final : public Expr {
public:
    BinaryOperator(SourceLocation location, BinaryOpcode opcode,
                   std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs);

    BinaryOpcode opcode;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

class CallExpr final : public Expr {
public:
    CallExpr(SourceLocation location, std::string callee);

    std::string callee;
    const FunctionDecl *calleeDecl = nullptr;
    bool isBuiltin = false;
    std::vector<std::unique_ptr<Expr>> arguments;
};

class InitListExpr final : public Expr {
public:
    using Expr::Expr;

    std::unique_ptr<Expr> arrayFiller;
    std::vector<std::size_t> valueOffsets;
    std::vector<std::unique_ptr<Expr>> values;
};

class ImplicitValueInitExpr final : public Expr {
public:
    ImplicitValueInitExpr(SourceLocation location, Type targetType);

    Type targetType;
};

[[nodiscard]] const char *toString(BuiltinType type) noexcept;
[[nodiscard]] const char *toString(UnaryOpcode opcode) noexcept;
[[nodiscard]] const char *toString(BinaryOpcode opcode) noexcept;
[[nodiscard]] const char *toString(CastKind kind) noexcept;
[[nodiscard]] std::string typeToString(const Type &type);

} // namespace ast
