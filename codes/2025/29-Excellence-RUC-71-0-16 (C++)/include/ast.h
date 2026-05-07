#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// ===--------------------------------------------------------------------=== //
// 辅助定义
// ===--------------------------------------------------------------------=== //

struct SourceLocation {
  int line = 0;
};

enum class BaseType { INT, FLOAT, VOID, STRING };
enum class UnaryOp { PLUS, MINUS, NOT };
enum class BinaryOp {
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  GT,
  GTE,
  LT,
  LTE,
  EQ,
  NEQ,
  AND,
  OR
};

// ===--------------------------------------------------------------------=== //
// AST节点前向声明 - 按文法命名
// ===--------------------------------------------------------------------=== //

class ASTVisitor;
class SyntaxNode;
class Decl;
class Stmt;
class Exp;
class InitValBase;
class InitVal;
class CompUnit;
class ConstDecl;
class ConstDef;
class VarDecl;
class VarDef;
class FuncDef;
class FuncFParam;
class Block;
class IfStmt;
class WhileStmt;
class ExpStmt;
class AssignStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;
class UnaryExp;
class BinaryExp;
class LVal;
class FunctionCall;
class Number;
class StringLiteral;
class ConstInitVal;

// ===--------------------------------------------------------------------=== //
// 访问者模式接口
// ===--------------------------------------------------------------------=== //

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;
  virtual void visit(CompUnit &node) = 0;
  virtual void visit(ConstDecl &node) = 0;
  virtual void visit(ConstDef &node) = 0;
  virtual void visit(VarDecl &node) = 0;
  virtual void visit(VarDef &node) = 0;
  virtual void visit(FuncDef &node) = 0;
  virtual void visit(FuncFParam &node) = 0;
  virtual void visit(Block &node) = 0;
  virtual void visit(IfStmt &node) = 0;
  virtual void visit(WhileStmt &node) = 0;
  virtual void visit(ExpStmt &node) = 0;
  virtual void visit(AssignStmt &node) = 0;
  virtual void visit(ReturnStmt &node) = 0;
  virtual void visit(BreakStmt &node) = 0;
  virtual void visit(ContinueStmt &node) = 0;
  virtual void visit(UnaryExp &node) = 0;
  virtual void visit(BinaryExp &node) = 0;
  virtual void visit(LVal &node) = 0;
  virtual void visit(FunctionCall &node) = 0;
  virtual void visit(Number &node) = 0;
  virtual void visit(StringLiteral &node) = 0;
  virtual void visit(InitVal &node) = 0;
  virtual void visit(ConstInitVal &node) = 0;
};

// ===--------------------------------------------------------------------=== //
// AST节点基类
// ===--------------------------------------------------------------------=== //

class SyntaxNode {
public:
  SourceLocation location;
  explicit SyntaxNode(SourceLocation loc = {}) : location(loc) {}
  virtual ~SyntaxNode() = default;
  virtual void accept(ASTVisitor &visitor) = 0;
};

class Decl : public SyntaxNode {
  using SyntaxNode::SyntaxNode;
};
class Stmt : public SyntaxNode {
  using SyntaxNode::SyntaxNode;
};
class Exp : public SyntaxNode {
  using SyntaxNode::SyntaxNode;
};
class InitValBase : public SyntaxNode {
public:
  using SyntaxNode::SyntaxNode;
};

// ===--------------------------------------------------------------------=== //
// 具体AST节点定义
// ===--------------------------------------------------------------------=== //

// CompUnit → CompUnit (Decl | FuncDef) | (Decl | FuncDef)
class CompUnit : public SyntaxNode {
public:
  std::vector<std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>>>
      items;
  explicit CompUnit(SourceLocation loc = {}) : SyntaxNode(loc) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
class ConstDecl : public Decl {
public:
  BaseType type;
  std::vector<std::unique_ptr<ConstDef>> definitions;
  ConstDecl(BaseType type, std::vector<std::unique_ptr<ConstDef>> defs,
            SourceLocation loc = {})
      : Decl(loc), type(type), definitions(std::move(defs)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// ConstDef → Ident {'[' ConstExp ']'} '=' ConstInitVal
class ConstDef : public SyntaxNode {
public:
  std::string name;
  std::vector<std::unique_ptr<Exp>> array_dimensions;
  std::unique_ptr<ConstInitVal> initializer;
  ConstDef(std::string name, std::vector<std::unique_ptr<Exp>> dims,
           std::unique_ptr<ConstInitVal> init, SourceLocation loc = {})
      : SyntaxNode(loc), name(std::move(name)),
        array_dimensions(std::move(dims)), initializer(std::move(init)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// VarDecl → BType VarDef { ',' VarDef } ';'
class VarDecl : public Decl {
public:
  BaseType type;
  std::vector<std::unique_ptr<VarDef>> definitions;
  VarDecl(BaseType type, std::vector<std::unique_ptr<VarDef>> defs,
          SourceLocation loc = {})
      : Decl(loc), type(type), definitions(std::move(defs)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// VarDef → Ident {'[' ConstExp ']'} | Ident {'[' ConstExp ']'} '=' InitVal
class VarDef : public SyntaxNode {
public:
  std::string name;
  std::vector<std::unique_ptr<Exp>> array_dimensions;
  std::optional<std::unique_ptr<InitVal>> initializer;
  VarDef(std::string name, std::vector<std::unique_ptr<Exp>> dims,
         std::optional<std::unique_ptr<InitVal>> init, SourceLocation loc = {})
      : SyntaxNode(loc), name(std::move(name)),
        array_dimensions(std::move(dims)), initializer(std::move(init)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
class FuncDef : public SyntaxNode {
public:
  BaseType return_type;
  std::string name;
  std::vector<std::unique_ptr<FuncFParam>> parameters;
  std::unique_ptr<Block> body;
  FuncDef(BaseType ret_type, std::string name,
          std::vector<std::unique_ptr<FuncFParam>> params,
          std::unique_ptr<Block> body, SourceLocation loc = {})
      : SyntaxNode(loc), return_type(ret_type), name(std::move(name)),
        parameters(std::move(params)), body(std::move(body)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// FuncFParam → BType Ident ['[' ']' { '[' Exp ']' }]
class FuncFParam : public SyntaxNode {
public:
  BaseType type;
  std::string name;
  bool is_array_pointer;
  std::vector<std::unique_ptr<Exp>> array_dimensions;
  FuncFParam(BaseType type, std::string name, bool is_array_ptr,
             std::vector<std::unique_ptr<Exp>> dims, SourceLocation loc = {})
      : SyntaxNode(loc), type(type), name(std::move(name)),
        is_array_pointer(is_array_ptr), array_dimensions(std::move(dims)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// Block → '{' { BlockItem } '}'
// BlockItem → Decl | Stmt
class Block : public Stmt {
public:
  std::vector<std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>>> items;
  explicit Block(SourceLocation loc = {}) : Stmt(loc) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
class IfStmt : public Stmt {
public:
  std::unique_ptr<Exp> condition;
  std::unique_ptr<Stmt> then_statement;
  std::optional<std::unique_ptr<Stmt>> else_statement;
  IfStmt(std::unique_ptr<Exp> cond, std::unique_ptr<Stmt> then_stmt,
         std::optional<std::unique_ptr<Stmt>> else_stmt,
         SourceLocation loc = {})
      : Stmt(loc), condition(std::move(cond)),
        then_statement(std::move(then_stmt)),
        else_statement(std::move(else_stmt)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// 'while' '(' Cond ')' Stmt
class WhileStmt : public Stmt {
public:
  std::unique_ptr<Exp> condition;
  std::unique_ptr<Stmt> body;
  WhileStmt(std::unique_ptr<Exp> cond, std::unique_ptr<Stmt> body,
            SourceLocation loc = {})
      : Stmt(loc), condition(std::move(cond)), body(std::move(body)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// [Exp] ';'
class ExpStmt : public Stmt {
public:
  std::optional<std::unique_ptr<Exp>> expression;
  explicit ExpStmt(std::optional<std::unique_ptr<Exp>> expr,
                   SourceLocation loc = {})
      : Stmt(loc), expression(std::move(expr)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// LVal '=' Exp ';'
class AssignStmt : public Stmt {
public:
  std::unique_ptr<LVal> lvalue;
  std::unique_ptr<Exp> rvalue;

  AssignStmt(std::unique_ptr<LVal> lval, std::unique_ptr<Exp> rval,
             SourceLocation loc = {})
      : Stmt(loc), lvalue(std::move(lval)), rvalue(std::move(rval)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// 'return' [Exp] ';'
class ReturnStmt : public Stmt {
public:
  std::optional<std::unique_ptr<Exp>> expression;
  explicit ReturnStmt(std::optional<std::unique_ptr<Exp>> expr,
                      SourceLocation loc = {})
      : Stmt(loc), expression(std::move(expr)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// 'break' ';'
class BreakStmt : public Stmt {
public:
  explicit BreakStmt(SourceLocation loc = {}) : Stmt(loc) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// 'continue' ';'
class ContinueStmt : public Stmt {
public:
  explicit ContinueStmt(SourceLocation loc = {}) : Stmt(loc) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
class UnaryExp : public Exp {
public:
  UnaryOp op;
  std::unique_ptr<Exp> operand;
  UnaryExp(UnaryOp op, std::unique_ptr<Exp> operand, SourceLocation loc = {})
      : Exp(loc), op(op), operand(std::move(operand)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// 二元表达式 - 涵盖 AddExp, MulExp, RelExp, EqExp, LAndExp, LOrExp
class BinaryExp : public Exp {
public:
  BinaryOp op;
  std::unique_ptr<Exp> lhs;
  std::unique_ptr<Exp> rhs;
  BinaryExp(BinaryOp op, std::unique_ptr<Exp> lhs, std::unique_ptr<Exp> rhs,
            SourceLocation loc = {})
      : Exp(loc), op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// LVal → Ident {'[' Exp ']'}
class LVal : public Exp {
public:
  std::string name;
  std::vector<std::unique_ptr<Exp>> indices;
  LVal(std::string name, std::vector<std::unique_ptr<Exp>> indices = {},
       SourceLocation loc = {})
      : Exp(loc), name(std::move(name)), indices(std::move(indices)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// Ident '(' [FuncRParams] ')'
class FunctionCall : public Exp {
public:
  std::string function_name;
  std::vector<std::unique_ptr<Exp>> arguments;
  FunctionCall(std::string name, std::vector<std::unique_ptr<Exp>> args,
               SourceLocation loc = {})
      : Exp(loc), function_name(std::move(name)), arguments(std::move(args)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// Number → IntConst | floatConst
class Number : public Exp {
public:
  std::variant<int, float> value;
  explicit Number(int val, SourceLocation loc = {}) : Exp(loc), value(val) {}
  explicit Number(float val, SourceLocation loc = {}) : Exp(loc), value(val) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// StringLiteral → StringConst
class StringLiteral : public Exp {
public:
  std::string value;
  explicit StringLiteral(std::string val, SourceLocation loc = {})
      : Exp(loc), value(std::move(val)) {}
  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// ConstInitVal → ConstExp | '{' [ConstInitVal { ',' ConstInitVal }] '}'
class ConstInitVal : public InitValBase {
public:
  // 单个表达式或初始化列表
  std::variant<std::unique_ptr<Exp>, std::vector<std::unique_ptr<ConstInitVal>>>
      value;

  // 单个表达式构造
  explicit ConstInitVal(std::unique_ptr<Exp> expr, SourceLocation loc = {})
      : InitValBase(loc), value(std::move(expr)) {}

  // 初始化列表构造
  explicit ConstInitVal(std::vector<std::unique_ptr<ConstInitVal>> inits,
                        SourceLocation loc = {})
      : InitValBase(loc), value(std::move(inits)) {}

  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// InitVal → Exp | '{' [InitVal { ',' InitVal }] '}'
class InitVal : public InitValBase {
public:
  // 单个表达式或初始化列表
  std::variant<std::unique_ptr<Exp>, std::vector<std::unique_ptr<InitVal>>>
      value;

  // 单个表达式构造
  explicit InitVal(std::unique_ptr<Exp> expr, SourceLocation loc = {})
      : InitValBase(loc), value(std::move(expr)) {}

  // 初始化列表构造
  explicit InitVal(std::vector<std::unique_ptr<InitVal>> inits,
                   SourceLocation loc = {})
      : InitValBase(loc), value(std::move(inits)) {}

  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

// ===--------------------------------------------------------------------=== //
// 通用工具函数 - 减少代码重复
// ===--------------------------------------------------------------------=== //

// 枚举到字符串转换函数 - 统一在此处定义，避免重复
namespace AST {
inline std::string baseTypeToString(BaseType type) {
  switch (type) {
  case BaseType::INT:
    return "int";
  case BaseType::FLOAT:
    return "float";
  case BaseType::VOID:
    return "void";
  case BaseType::STRING:
    return "string";
  default:
    return "unknown";
  }
}

inline std::string unaryOpToString(UnaryOp op) {
  switch (op) {
  case UnaryOp::PLUS:
    return "+";
  case UnaryOp::MINUS:
    return "-";
  case UnaryOp::NOT:
    return "!";
  default:
    return "?";
  }
}

inline std::string binaryOpToString(BinaryOp op) {
  switch (op) {
  case BinaryOp::ADD:
    return "+";
  case BinaryOp::SUB:
    return "-";
  case BinaryOp::MUL:
    return "*";
  case BinaryOp::DIV:
    return "/";
  case BinaryOp::MOD:
    return "%";
  case BinaryOp::GT:
    return ">";
  case BinaryOp::GTE:
    return ">=";
  case BinaryOp::LT:
    return "<";
  case BinaryOp::LTE:
    return "<=";
  case BinaryOp::EQ:
    return "==";
  case BinaryOp::NEQ:
    return "!=";
  case BinaryOp::AND:
    return "&&";
  case BinaryOp::OR:
    return "||";
  default:
    return "?";
  }
}
} // namespace AST
