#pragma once

#include "ast.h"
#include "symtab.h"
#include "types.h"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

// 语义错误类型
enum class SemanticErrorType {
  UNDEFINED_SYMBOL,      // 未定义的符号
  REDEFINED_SYMBOL,      // 重复定义的符号
  TYPE_MISMATCH,         // 类型不匹配
  INVALID_ARRAY_INDEX,   // 无效的数组索引
  INVALID_FUNCTION_CALL, // 无效的函数调用
  BREAK_CONTINUE_ERROR,  // break/continue使用错误
  MISSING_RETURN,        // 缺少返回语句
  INVALID_ASSIGNMENT,    // 无效的赋值
  CONST_MODIFICATION,    // 修改常量
  DIMENSION_MISMATCH     // 数组维度不匹配
};

// 语义错误信息
struct SemanticError {
  SemanticErrorType type;
  std::string message;
  SourceLocation location;

  SemanticError(SemanticErrorType t, std::string msg, SourceLocation loc)
      : type(t), message(std::move(msg)), location(loc) {}
};

// 延迟检查项 - 记录需要稍后检查的引用
struct PendingReference {
  enum Type { VARIABLE_REF, FUNCTION_CALL, ARRAY_ACCESS } type;
  std::string name;                                  // 引用的符号名
  SourceLocation location;                           // 引用位置
  Exp *expression;                                   // 引用的表达式节点
  std::vector<std::shared_ptr<Type>> argument_types; // 函数调用参数类型

  PendingReference(Type t, std::string n, SourceLocation loc, Exp *expr)
      : type(t), name(std::move(n)), location(loc), expression(expr) {}
};

// 语义分析器类 - 实现一次扫描 + 延迟检查策略
class SemanticAnalyzer : public ASTVisitor {
private:
  SymbolTable symbol_table;
  std::vector<SemanticError> errors;
  std::vector<PendingReference> pending_references;

  // 分析状态
  bool in_function = false;                           // 是否在函数内部
  bool in_loop = false;                               // 是否在循环内部
  std::shared_ptr<Type> current_function_return_type; // 当前函数返回类型
  std::string current_function_name;                  // 当前函数名
  BaseType current_decl_type = BaseType::INT; // 当前声明的基本类型

  // 辅助方法
  void reportError(SemanticErrorType type, const std::string &message,
                   SourceLocation location);
  void addPendingReference(PendingReference::Type type, const std::string &name,
                           SourceLocation location, Exp *expression);

  // 类型推导和检查
  std::shared_ptr<Type> inferExpressionType(Exp *expression);
  bool isTypeCompatible(const Type &expected, const Type &actual);
  bool canImplicitConvert(const Type &from, const Type &to);

  // 常量表达式求值
  std::optional<std::variant<int, float>>
  evaluateConstExpression(Exp *expression);
  std::optional<std::variant<int, float>>
  evaluateConstInitVal(ConstInitVal *init_val);

  // 数组维度计算
  std::vector<int>
  calculateArrayDimensions(const std::vector<std::unique_ptr<Exp>> &dims);

  // 函数相关检查
  void checkFunctionCall(FunctionCall &call);
  bool checkFunctionParameters(const FunctionType &func_type,
                               const std::vector<std::unique_ptr<Exp>> &args);

public:
  SemanticAnalyzer() = default;

  // 执行语义分析的主要接口
  bool analyze(CompUnit &root);

  // 获取分析结果
  const std::vector<SemanticError> &getErrors() const { return errors; }
  bool hasErrors() const { return !errors.empty(); }

  // 打印错误信息
  void printErrors(std::ostream &out = std::cerr) const;

  // 获取符号表（用于调试）
  const SymbolTable &getSymbolTable() const { return symbol_table; }

  // 访问者模式接口实现
  void visit(CompUnit &node) override;
  void visit(ConstDecl &node) override;
  void visit(ConstDef &node) override;
  void visit(VarDecl &node) override;
  void visit(VarDef &node) override;
  void visit(FuncDef &node) override;
  void visit(FuncFParam &node) override;
  void visit(Block &node) override;
  void visit(IfStmt &node) override;
  void visit(WhileStmt &node) override;
  void visit(ExpStmt &node) override;
  void visit(AssignStmt &node) override;
  void visit(ReturnStmt &node) override;
  void visit(BreakStmt &node) override;
  void visit(ContinueStmt &node) override;
  void visit(UnaryExp &node) override;
  void visit(BinaryExp &node) override;
  void visit(LVal &node) override;
  void visit(FunctionCall &node) override;
  void visit(Number &node) override;
  void visit(StringLiteral &node) override;
  void visit(InitVal &node) override;
  void visit(ConstInitVal &node) override;

private:
  // 延迟检查阶段
  void performDelayedChecks();
  void checkPendingReferences();
  void checkMainFunction();
};