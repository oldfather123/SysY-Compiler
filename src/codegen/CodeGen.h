#pragma once

#include "codegen/Defs.h"
#include "frontend/AST.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

namespace codegen {

class Builder {
public:
    explicit Builder(Region *region);

    Region *region() const { return region_; }
    BasicBlock *insertBlock() const { return insertBlock_; }

    BasicBlock *createBlock(std::string name = {});
    void setInsertPoint(BasicBlock *block);

    Op *createOp(Opcode opcode, ValueType returnType = ValueType::Void,
                 std::vector<Value> operands = {},
                 std::optional<std::vector<Attr>> attrs = std::nullopt);

private:
    Region *region_ = nullptr;
    BasicBlock *insertBlock_ = nullptr;
};

class CodeGen {
public:
    explicit CodeGen(const ast::TranslationUnit &unit);

    Region *region() { return region_.get(); }
    const Region *region() const { return region_.get(); }

private:
    struct Symbol {
        Value address;
        ast::Type type;
    };

    using SymbolTable = std::unordered_map<std::string, Symbol>;

    void emitDecl(const ast::Decl &decl);
    void emitStmt(const ast::Stmt &stmt);
    Value emitExpr(const ast::Expr &expr);

    void emitVarDecl(const ast::VarDecl &decl, bool isGlobal);
    void emitFunctionDecl(const ast::FunctionDecl &decl);

    Value emitBinary(const ast::BinaryOperator &expr);
    Value emitUnary(const ast::UnaryOperator &expr);
    Value emitDeclRef(const ast::DeclRefExpr &expr);
    Value emitLValueAddress(const ast::Expr &expr);
    Value emitCall(const ast::CallExpr &expr);
    Value emitImplicitCast(const ast::ImplicitCastExpr &expr);
    Value emitBoolExpr(const ast::Expr &expr);
    Value toBoolValue(Value value, ast::Type type);

    Symbol *resolveLocal(const std::string &name);
    const Symbol *resolveLocal(const std::string &name) const;
    const Symbol *resolveAny(const std::string &name) const;

    void pushScope();
    void popScope();

    static bool isFloatType(ast::Type type);
    static ValueType toValueType(ast::Type type);
    static std::string typeTag(ast::Type type);
    static std::size_t allocSizeBytes(ast::Type type);
    static bool isTerminator(Opcode opcode);

    BasicBlock *newBlock(const std::string &prefix);
    void emitGoto(BasicBlock *target);
    void emitBranch(Value cond, BasicBlock *trueTarget, BasicBlock *falseTarget);
    void ensureGotoIfNotTerminated(BasicBlock *target);
    void startUnreachableContinuation();

    std::unique_ptr<Region> region_;
    Builder builder_;
    SymbolTable globals_;
    std::vector<SymbolTable> localScopes_;
    std::vector<std::pair<BasicBlock *, BasicBlock *>> loopTargets_;
    ast::Type currentReturnType_;
    bool inFunction_ = false;
    std::size_t blockId_ = 0;
};

} // namespace codegen
