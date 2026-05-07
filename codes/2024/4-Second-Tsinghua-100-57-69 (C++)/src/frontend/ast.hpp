/*
  本文件定义了ast中各个节点的类
*/


#pragma once

#include <variant>

#include "common/Display.hpp"
#include "common/common.hpp"

#define GetValue(x) (x.type == Int ? x.iv : x.fv)

namespace frontend {
namespace ast {

using UnaryOp = ::UnaryOp; // 指定UnaryOp为全局的UnaryOp，即common.hpp中的UnaryOp
using BinaryOp = ::BinaryOp; // BinaryOp为全局的BinaryOp，即common.hpp中的BinaryOp

/// @brief SysYType是SysY语法的主类型，继承Display来进行显示
class SysYType : public Display {
public:
  virtual ~SysYType() = default;
};

/// @brief 标量类型（一定为int），继承SysYType
class ScalarType : public SysYType {
public:
  using Type = int;

  /// @brief 注：Type为int,1表示float类型，0表示int类型
  explicit ScalarType(Type type) : m_type{type} {}
  virtual ~ScalarType() = default;

  void print(std::ostream &out, unsigned indent) const override;

  Type type() const { return m_type; }

private:
  Type m_type; // 表示它的值
};

class Expression; // 提前声明供使用

/// @brief 数组类型，继承SysYType
class ArrayType : public SysYType {
public:
  /// @brief Dimension是指向Expression类型的智能指针unique_ptr
  using Dimension = std::unique_ptr<Expression>;

  /// @param omit_first_dimension: 是否隐藏第一维大小
  ArrayType(ScalarType type, std::vector<Dimension> dimensions,
            bool omit_first_dimension)
      : m_type{type}, m_dimensions{std::move(dimensions)},
        m_omit_first_dimension{omit_first_dimension} {}
  virtual ~ArrayType() = default;

  void print(std::ostream &out, unsigned indent) const override;
  /// 函数
  ScalarType::Type base_type() const { return m_type.type(); }
  /// 函数
  const std::vector<Dimension> &dimensions() const { return m_dimensions; }
  /// 函数
  bool first_dimension_omitted() const { return m_omit_first_dimension; }

private:
  ScalarType m_type;
  std::vector<Dimension> m_dimensions;
  bool m_omit_first_dimension;
};

/// @brief 标识符类型，继承Display
class Identifier : public Display {
public:
  /// @param mangle: 是否将源代码中变量名改为编译器中使用（C++ABI）的变量名
  Identifier(std::string name, bool const mangle = true)
      : m_name{mangle ? '$' + name : std::move(name)} {}
  virtual ~Identifier() = default;

  void print(std::ostream &out, unsigned indent) const override;

  const std::string &name() const { return m_name; }

private:
  std::string m_name;
};

/// @brief 参数类型，继承Display
class Parameter : public Display {
public:
  Parameter(std::unique_ptr<SysYType> type, Identifier ident)
      : m_type{std::move(type)}, m_ident{std::move(ident)} {}
  virtual ~Parameter() = default;

  void print(std::ostream &out, unsigned indent) const override;

  const std::unique_ptr<SysYType> &type() const { return m_type; }
  const Identifier &ident() const { return m_ident; }

public:
  /// 标记var为mutable意味着类中的const方法可以更改var
  mutable std::shared_ptr<Var> var;

private:
  std::unique_ptr<SysYType> m_type;
  Identifier m_ident;
};

/// @brief ast节点类型，继承Display
class AstNode : public Display {
public:
  virtual ~AstNode() = default;
};

class NumberLiteral;

/// @brief 表达式类型，继承AstNode
class Expression : public AstNode {
public:
  Expression() {}
  virtual ~Expression() = default;

public:
  // 求得的表达式类型
  mutable std::optional<Type> type;
};

/// @brief 左值类型，继承Expression
class LValue : public Expression {
public:
  LValue(Identifier ident, std::vector<std::unique_ptr<Expression>> indices)
      : m_ident{std::move(ident)}, m_indices{std::move(indices)} {}
  virtual ~LValue() = default;

  void print(std::ostream &out, unsigned indent) const override;

  const Identifier &ident() const { return m_ident; }
  const std::vector<std::unique_ptr<Expression>> &indices() const {
    return m_indices;
  }

public:
  mutable std::shared_ptr<Var> var;

private:
  Identifier m_ident;
  std::vector<std::unique_ptr<Expression>> m_indices;
};

/// @brief 一元表达式类型，继承Expression
class UnaryExpr : public Expression {
public:
  UnaryExpr(UnaryOp op, std::unique_ptr<Expression> operand)
      : m_op{op}, m_operand{std::move(operand)} {}
  virtual ~UnaryExpr() = default;

  void print(std::ostream &out, unsigned indent) const override;

  UnaryOp op() const { return m_op; }
  const std::unique_ptr<Expression> &operand() const { return m_operand; }

private:
  UnaryOp m_op;
  std::unique_ptr<Expression> m_operand;
};

/// @brief 二元表达式类型，继承Expression
class BinaryExpr : public Expression {
public:
  BinaryExpr(BinaryOp op, std::unique_ptr<Expression> lhs,
             std::unique_ptr<Expression> rhs)
      : m_op{op}, m_lhs{std::move(lhs)}, m_rhs{std::move(rhs)} {}
  virtual ~BinaryExpr() = default;

  void print(std::ostream &out, unsigned indent) const override;

  BinaryOp op() const { return m_op; }
  const std::unique_ptr<Expression> &lhs() const { return m_lhs; }
  const std::unique_ptr<Expression> &rhs() const { return m_rhs; }

  ConstValue to_const(ConstValue lhs, ConstValue rhs, BinaryOp type_) {
    if (type_ == BinaryOp::And) return ConstValue((int)GetValue(lhs) & (int)GetValue(rhs));
    if (type_ == BinaryOp::Or ) return ConstValue((int)GetValue(lhs) | (int)GetValue(rhs));

    if (lhs.type == Float || rhs.type == Float) {
      if (type_ == BinaryOp::Add) return ConstValue(GetValue(lhs) + GetValue(rhs));
      if (type_ == BinaryOp::Sub) return ConstValue(GetValue(lhs) - GetValue(rhs));
      if (type_ == BinaryOp::Mul) return ConstValue(GetValue(lhs) * GetValue(rhs));
      if (type_ == BinaryOp::Div) return ConstValue(GetValue(lhs) / GetValue(rhs));
    }
    else {
      if (type_ == BinaryOp::Add) return ConstValue((int)GetValue(lhs) + (int)GetValue(rhs));
      if (type_ == BinaryOp::Sub) return ConstValue((int)GetValue(lhs) - (int)GetValue(rhs));
      if (type_ == BinaryOp::Mul) return ConstValue((int)GetValue(lhs) * (int)GetValue(rhs));
      if (type_ == BinaryOp::Div) return ConstValue((int)GetValue(lhs) / (int)GetValue(rhs));
    }
    if (type_ == BinaryOp::Mod) return ConstValue((int)GetValue(lhs) % (int)GetValue(rhs));
    
    if (type_ == BinaryOp::Eq) return lhs == rhs;
    if (type_ == BinaryOp::Neq) return lhs != rhs;

    if (type_ == BinaryOp::Lt) return ConstValue(GetValue(lhs) < GetValue(rhs));
    if (type_ == BinaryOp::Leq) return ConstValue(GetValue(lhs) <= GetValue(rhs));
    if (type_ == BinaryOp::Gt) return ConstValue(GetValue(lhs) > GetValue(rhs));
    if (type_ == BinaryOp::Geq) return ConstValue(GetValue(lhs) >= GetValue(rhs));
    if (type_ == BinaryOp::Shr) return ConstValue((int)GetValue(lhs) >> (int)GetValue(rhs));
    if (type_ == BinaryOp::Shl) return ConstValue((int)GetValue(lhs) << (int)GetValue(rhs));
    assert(false);
}

private:
  BinaryOp m_op;
  std::unique_ptr<Expression> m_lhs, m_rhs;
};

/// @brief 字面量类型，继承Expression
class Literal : public Expression {
public:
  virtual ~Literal() = default;
};

/// @brief 数字字面量类型，继承Literal
class NumberLiteral : public Literal {
public:
  virtual ~NumberLiteral() = default;
};

/// @brief int字面量类型，继承NumberLiteral
class IntLiteral : public NumberLiteral {
public:
  using Value = std::int32_t;
  static_assert(sizeof(Value) == 4);

  IntLiteral(Value value) : m_value{value} {}
  virtual ~IntLiteral() = default;

  Value value() const { return m_value; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  Value m_value;
};

/// @brief float字面量类型，继承NumberLiteral
class FloatLiteral : public NumberLiteral {
public:
  using Value = float;
  static_assert(sizeof(Value) == 4);

  FloatLiteral(Value value) : m_value{value} {}
  virtual ~FloatLiteral() = default;

  Value value() const { return m_value; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  Value m_value;
};

/// @brief 字符串字面量类型，继承AstNode
class StringLiteral : AstNode {
public:
  using Value = std::string;

  StringLiteral(Value value) : m_value{std::move(value)} {}
  virtual ~StringLiteral() = default;

  const std::string &value() const { return m_value; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  Value m_value;
};

/// @brief 函数调用类型，继承Expression
class Call : public Expression {
public:
  using Argument = std::variant<std::unique_ptr<Expression>, StringLiteral>;

  Call(Identifier func, std::vector<Argument> args, unsigned line)
      : m_func{std::move(func)}, m_args{std::move(args)}, m_line(line) {}
  virtual ~Call() = default;

  const Identifier &func() const { return m_func; }
  const std::vector<Argument> &args() const { return m_args; }
  unsigned line() const { return this->m_line; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  Identifier m_func;
  std::vector<Argument> m_args;
  unsigned m_line;
};

/// @brief 可执行操作类型，继承AstNode
class Statement : public AstNode {
public:
  virtual ~Statement() = default;
};

/// @brief ExprStmt类型，继承Statement
class ExprStmt : public Statement {
public:
  explicit ExprStmt(std::unique_ptr<Expression> expr)
      : m_expr{std::move(expr)} {}
  virtual ~ExprStmt() = default;

  const std::unique_ptr<Expression> &expr() const { return m_expr; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  std::unique_ptr<Expression> m_expr;
};

/// @brief 赋值表达式类型，继承Statement
class Assignment : public Statement {
public:
  Assignment(std::unique_ptr<LValue> lhs, std::unique_ptr<Expression> rhs)
      : m_lhs{std::move(lhs)}, m_rhs{std::move(rhs)} {}
  virtual ~Assignment() = default;

  const std::unique_ptr<LValue> &lhs() const { return m_lhs; }
  const std::unique_ptr<Expression> &rhs() const { return m_rhs; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  std::unique_ptr<LValue> m_lhs;
  std::unique_ptr<Expression> m_rhs;
};

/// @brief 初始值类型，继承AstNode
class Initializer : public AstNode {
public:
  using Value = std::variant<std::unique_ptr<Expression>,
                             std::vector<std::unique_ptr<Initializer>>>;

  explicit Initializer(std::unique_ptr<Expression> value)
      : m_value{std::move(value)} {}
  explicit Initializer(std::vector<std::unique_ptr<Initializer>> values)
      : m_value{std::move(values)} {}
  virtual ~Initializer() = default;

  void print(std::ostream &out, unsigned indent) const override;

  const Value &value() const { return m_value; }

private:
  Value m_value;
};

/// @brief 声明表达式类型，继承AstNode
class Declaration : public AstNode {
public:
  /// @param const_qualified: whether is const
  Declaration(std::unique_ptr<SysYType> type, Identifier ident,
              std::unique_ptr<Initializer> init, bool const_qualified)
      : m_type{std::move(type)}, m_ident{std::move(ident)},
        m_init{std::move(init)}, m_const_qualified{const_qualified} {}
  virtual ~Declaration() = default;

  void print(std::ostream &out, unsigned indent) const override;

  const std::unique_ptr<SysYType> &type() const { return m_type; }
  const Identifier &ident() const { return m_ident; }
  const std::unique_ptr<Initializer> &init() const { return m_init; }
  bool const_qualified() const { return m_const_qualified; }

public:
  mutable std::shared_ptr<Var> var;

private:
  std::unique_ptr<SysYType> m_type;
  Identifier m_ident;
  std::unique_ptr<Initializer> m_init;
  bool m_const_qualified;
};

/// @brief 语句块类型，继承Statement
class Block : public Statement {
public:
  using Child =
      std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Statement>>;

  explicit Block(std::vector<Child> children)
      : m_children{std::move(children)} {}
  virtual ~Block() = default;

  const std::vector<Child> &children() const { return m_children; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  std::vector<Child> m_children;
};

/// @brief IF语句类型，继承Statement
class IfElse : public Statement {
public:
  IfElse(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> then,
         std::unique_ptr<Statement> else_)
      : m_cond{std::move(cond)}, m_then{std::move(then)}, m_else{std::move(
                                                              else_)} {}
  virtual ~IfElse() = default;

  const std::unique_ptr<Expression> &cond() const { return m_cond; }
  const std::unique_ptr<Statement> &then() const { return m_then; }
  const std::unique_ptr<Statement> &otherwise() const { return m_else; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  std::unique_ptr<Expression> m_cond;
  std::unique_ptr<Statement> m_then, m_else;
};

/// @brief WHILE语句类型，继承Statement
class While : public Statement {
public:
  While(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> body)
      : m_cond{std::move(cond)}, m_body{std::move(body)} {}
  virtual ~While() = default;

  const std::unique_ptr<Expression> &cond() const { return m_cond; }
  const std::unique_ptr<Statement> &body() const { return m_body; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  std::unique_ptr<Expression> m_cond;
  std::unique_ptr<Statement> m_body;
};

/// @brief BREAK语句类型，继承Statement
class Break : public Statement {
public:
  virtual ~Break() = default;

  void print(std::ostream &out, unsigned indent) const override;
};

/// @brief CONTINUE语句类型，继承Statement
class Continue : public Statement {
public:
  virtual ~Continue() = default;

  void print(std::ostream &out, unsigned indent) const override;
};

/// @brief RETURN语句类型，继承Statement
class Return : public Statement {
public:
  explicit Return(std::unique_ptr<Expression> res) : m_res{std::move(res)} {}
  virtual ~Return() = default;

  const std::unique_ptr<Expression> &res() const { return m_res; }

  void print(std::ostream &out, unsigned indent) const override;

private:
  std::unique_ptr<Expression> m_res;
};

/// @brief FUNCTION语句类型，继承Statement
class Function : public AstNode {
public:
  Function(std::unique_ptr<ScalarType> type, Identifier ident,
           std::vector<std::unique_ptr<Parameter>> params,
           std::unique_ptr<Block> body)
      : m_type{std::move(type)}, m_ident{std::move(ident)},
        m_params{std::move(params)}, m_body{std::move(body)} {}
  virtual ~Function() = default;

  void print(std::ostream &out, unsigned indent) const override;

  const std::unique_ptr<ScalarType> &type() const { return m_type; }
  const Identifier &ident() const { return m_ident; }
  const std::vector<std::unique_ptr<Parameter>> &params() const {
    return m_params;
  }
  const std::unique_ptr<Block> &body() const { return m_body; }

private:
  std::unique_ptr<ScalarType> m_type;
  Identifier m_ident;
  std::vector<std::unique_ptr<Parameter>> m_params;
  std::unique_ptr<Block> m_body;
};

/// @brief 编译单元类型，一定是函数或者全局变量定义，继承AstNode
class CompileUnit : public AstNode {
public:
  using Child =
      std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Function>>;

  explicit CompileUnit(std::vector<Child> children)
      : m_children{std::move(children)} {}
  virtual ~CompileUnit() = default;

  void print(std::ostream &out, unsigned indent) const override;

  const std::vector<Child> &children() const { return m_children; }

private:
  std::vector<Child> m_children;
};

} // namespace ast
} // namespace frontend
