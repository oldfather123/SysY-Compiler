#pragma once

#include "frontend/AST.h"

#include <string>
#include <unordered_map>
#include <vector>

class SemanticAnalysis {
public:
    bool analyze(ast::TranslationUnit &unit);
    const std::vector<std::string> &errors() const;

private:
    struct Symbol {
        ast::Type type;
        ast::Decl *decl = nullptr;
    };

    struct Function {
        ast::Type returnType;
        std::vector<ast::Type> params;
        bool variadic = false;
        bool builtin = false;
        ast::FunctionDecl *decl = nullptr;
    };

    struct ConstValue {
        bool known = false;
        ast::BuiltinType base = ast::BuiltinType::Invalid;
        long long intValue = 0;
        float floatValue = 0.0f;
    };

    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    std::vector<std::unordered_map<std::string, ConstValue>> constants_;
    std::unordered_map<std::string, Function> functions_;
    std::vector<std::string> errors_;
    ast::Type currentFunctionReturn_;
    int loopDepth_ = 0;
    bool sawMain_ = false;

    void reset();
    void installRuntimeFunctions();
    void pushScope();
    void popScope();

    void analyzeDecl(ast::Decl &decl);
    void analyzeVarDecl(ast::VarDecl &decl);
    void analyzeFunctionDecl(ast::FunctionDecl &decl);
    void analyzeStmt(ast::Stmt &stmt);
    void analyzeCompoundStmt(ast::CompoundStmt &stmt);
    void analyzeDeclStmt(ast::DeclStmt &stmt);
    void analyzeInitializer(ast::Type target, std::unique_ptr<ast::Expr> &initializer,
                            bool constOnly, bool checkIntRange,
                            bool arrayElement = false);

    ast::Type inferExpr(ast::Expr &expr, bool allowConditionOps, bool constOnly = false);
    ast::Type inferDeclRef(ast::DeclRefExpr &expr, bool constOnly);
    ast::Type inferParen(ast::ParenExpr &expr, bool allowConditionOps, bool constOnly);
    ast::Type inferArraySubscript(ast::ArraySubscriptExpr &expr, bool allowConditionOps,
                                  bool constOnly);
    ast::Type inferUnary(ast::UnaryOperator &expr, bool allowConditionOps,
                         bool constOnly);
    ast::Type inferBinary(ast::BinaryOperator &expr, bool allowConditionOps,
                          bool constOnly);
    ast::Type inferCall(ast::CallExpr &expr, bool allowConditionOps, bool constOnly);

    ast::Type buildParamType(ast::ParamDecl &param);
    ast::Type buildArrayType(ast::VarDecl &decl, bool constOnly);

    bool declareVariable(const std::string &name, ast::Type type,
                         ast::Decl &decl);
    const Symbol *resolveVariable(const std::string &name) const;
    void rememberConstant(const std::string &name, ConstValue value);
    const ConstValue *resolveConstant(const std::string &name) const;
    bool declareFunction(const std::string &name, Function function,
                         ast::FunctionDecl &decl);
    const Function *resolveFunction(const std::string &name) const;

    bool compatible(ast::Type target, ast::Type source, bool arrayInitializer) const;
    bool compatibleCallArgument(const Function &function, std::size_t paramIndex,
                                ast::Type target, ast::Type source) const;
    bool isNumeric(ast::BuiltinType type) const;
    bool isConditionOperator(ast::BinaryOpcode opcode) const;
    bool isArithmeticOperator(ast::BinaryOpcode opcode) const;
    std::size_t aggregateSize(ast::Type type, std::size_t level) const;
    std::size_t initializerChildLevel(ast::Type type, std::size_t parentLevel,
                                      std::size_t parentStart, std::size_t pos) const;
    std::size_t countInitLeaves(ast::Expr &initializer) const;
    std::size_t checkInitAggregate(ast::Type target,
                                   std::unique_ptr<ast::Expr> &initializer,
                                   bool constOnly, bool checkIntRange,
                                   std::size_t level, std::size_t start,
                                   bool root);
    std::size_t collectNormalizedInitLeaves(
        ast::Type target, std::unique_ptr<ast::Expr> &initializer,
        std::size_t level, std::size_t start, bool root,
        std::vector<std::unique_ptr<ast::Expr>> &leaves,
        std::unordered_map<std::string, ast::SourceLocation> &listLocations) const;
    void normalizeArrayInitializer(ast::Type target,
                                   std::unique_ptr<ast::Expr> &initializer);
    std::unique_ptr<ast::Expr> buildNormalizedInitList(
        ast::Type target, std::size_t level, std::size_t start,
        std::vector<std::unique_ptr<ast::Expr>> &leaves,
        const std::unordered_map<std::string, ast::SourceLocation> &listLocations) const;
    ast::Type scalarElementType(ast::Type type) const;
    ast::Type typeAtLevel(ast::Type type, std::size_t level) const;
    const ast::DeclRefExpr *subscriptRoot(const ast::Expr &expr) const;
    bool isAssignableLValue(const ast::Expr &expr) const;
    bool stmtAlwaysReturns(const ast::Stmt &stmt) const;
    bool stmtMayBreakCurrentLoop(const ast::Stmt &stmt) const;
    bool isFloatRange(ConstValue value) const;
    bool checkValueRange(ast::Type target, ConstValue value) const;
    bool isConstTruthy(ConstValue value) const;

    ConstValue evalConstExpr(ast::Expr &expr);
    ConstValue evalConstLValue(ast::Expr &expr);
    ConstValue evalConstArrayElement(const ast::VarDecl &decl,
                                     const std::vector<long long> &indices);
    std::size_t flattenConstArrayInit(ast::Type target, ast::Expr &initializer,
                                      std::size_t level, std::size_t start,
                                      bool root,
                                      std::vector<ConstValue> &values) const;
    ConstValue evalConstUnary(ast::UnaryOpcode opcode, ConstValue value) const;
    ConstValue evalConstBinary(ast::BinaryOpcode opcode, ConstValue lhs,
                               ConstValue rhs) const;
    ConstValue convertConstValue(ConstValue value, ast::Type target) const;
    bool isIntRange(ConstValue value) const;
    void applyArrayToPointerDecay(std::unique_ptr<ast::Expr> &expr);
    void applyLValueToRValue(std::unique_ptr<ast::Expr> &expr);
    void applyImplicitCast(std::unique_ptr<ast::Expr> &expr, ast::Type target);

    ast::Type setExprType(ast::Expr &expr, ast::Type type) const;
    std::string location(ast::SourceLocation loc) const;
    void addError(ast::SourceLocation loc, const std::string &message);
    void addError(const ast::ASTNode &node, const std::string &message);

    static long long parseIntLiteral(const std::string &text);
    static float parseFloatLiteral(const std::string &text);
};
