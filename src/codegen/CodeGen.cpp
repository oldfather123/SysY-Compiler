#include "codegen/CodeGen.h"

#include <stdexcept>
#include <string>
#include <functional>
#include <utility>

namespace codegen {
namespace {

std::string locText(const ast::ASTNode &node) {
    const ast::SourceLocation loc = node.location();
    return " at " + std::to_string(loc.line) + ":" + std::to_string(loc.column);
}

std::string exprKind(const ast::Expr &expr) {
    if (dynamic_cast<const ast::IntegerLiteral *>(&expr) != nullptr) {
        return "IntegerLiteral";
    }
    if (dynamic_cast<const ast::FloatLiteral *>(&expr) != nullptr) {
        return "FloatLiteral";
    }
    if (dynamic_cast<const ast::StringLiteral *>(&expr) != nullptr) {
        return "StringLiteral";
    }
    if (dynamic_cast<const ast::DeclRefExpr *>(&expr) != nullptr) {
        return "DeclRefExpr";
    }
    if (dynamic_cast<const ast::ArraySubscriptExpr *>(&expr) != nullptr) {
        return "ArraySubscriptExpr";
    }
    if (dynamic_cast<const ast::UnaryOperator *>(&expr) != nullptr) {
        return "UnaryOperator";
    }
    if (dynamic_cast<const ast::BinaryOperator *>(&expr) != nullptr) {
        return "BinaryOperator";
    }
    if (dynamic_cast<const ast::CallExpr *>(&expr) != nullptr) {
        return "CallExpr";
    }
    if (dynamic_cast<const ast::InitListExpr *>(&expr) != nullptr) {
        return "InitListExpr";
    }
    if (dynamic_cast<const ast::ImplicitCastExpr *>(&expr) != nullptr) {
        return "ImplicitCastExpr";
    }
    if (dynamic_cast<const ast::ParenExpr *>(&expr) != nullptr) {
        return "ParenExpr";
    }
    return "Expr(Unknown)";
}

std::string stmtKind(const ast::Stmt &stmt) {
    if (dynamic_cast<const ast::CompoundStmt *>(&stmt) != nullptr) {
        return "CompoundStmt";
    }
    if (dynamic_cast<const ast::DeclStmt *>(&stmt) != nullptr) {
        return "DeclStmt";
    }
    if (dynamic_cast<const ast::NullStmt *>(&stmt) != nullptr) {
        return "NullStmt";
    }
    if (dynamic_cast<const ast::IfStmt *>(&stmt) != nullptr) {
        return "IfStmt";
    }
    if (dynamic_cast<const ast::WhileStmt *>(&stmt) != nullptr) {
        return "WhileStmt";
    }
    if (dynamic_cast<const ast::BreakStmt *>(&stmt) != nullptr) {
        return "BreakStmt";
    }
    if (dynamic_cast<const ast::ContinueStmt *>(&stmt) != nullptr) {
        return "ContinueStmt";
    }
    if (dynamic_cast<const ast::ReturnStmt *>(&stmt) != nullptr) {
        return "ReturnStmt";
    }
    if (dynamic_cast<const ast::Expr *>(&stmt) != nullptr) {
        return "ExprAsStmt";
    }
    return "Stmt(Unknown)";
}

std::size_t productFromShape(const std::vector<long long> &shape, std::size_t begin) {
    std::size_t total = 1;
    for (std::size_t i = begin; i < shape.size(); ++i) {
        if (shape[i] <= 0) {
            continue;
        }
        total *= static_cast<std::size_t>(shape[i]);
    }
    return total;
}

} // namespace

Builder::Builder(Region *region) : region_(region) {
    if (region_ == nullptr) {
        throw std::invalid_argument("Builder: region is null");
    }
}

BasicBlock *Builder::createBlock(std::string name) {
    insertBlock_ = region_->createBlock(std::move(name));
    return insertBlock_;
}

void Builder::setInsertPoint(BasicBlock *block) {
    if (block == nullptr) {
        throw std::invalid_argument("setInsertPoint: block is null");
    }
    if (block->parent() != region_) {
        throw std::invalid_argument("setInsertPoint: block does not belong to builder region");
    }
    insertBlock_ = block;
}

Op *Builder::createOp(Opcode opcode, ValueType returnType, std::vector<Value> operands,
                      std::optional<std::vector<Attr>> attrs) {
    if (insertBlock_ == nullptr) {
        throw std::logic_error("createOp: insert block is not set");
    }
    auto op = std::make_unique<Op>(opcode, returnType, std::move(operands), std::move(attrs));
    return insertBlock_->appendOp(std::move(op));
}

CodeGen::CodeGen(const ast::TranslationUnit &unit)
    : region_(std::make_unique<Region>("module")), builder_(region_.get()) {
    builder_.createBlock("entry");

    for (const auto &decl : unit.declarations) {
        if (const auto *var = dynamic_cast<const ast::VarDecl *>(decl.get())) {
            emitVarDecl(*var, true);
            continue;
        }
        if (dynamic_cast<const ast::FunctionDecl *>(decl.get()) != nullptr) {
            continue;
        }
        throw std::logic_error("unsupported declaration node" + locText(*decl));
    }

    for (const auto &decl : unit.declarations) {
        if (const auto *func = dynamic_cast<const ast::FunctionDecl *>(decl.get())) {
            emitFunctionDecl(*func);
        }
    }
}

void CodeGen::emitDecl(const ast::Decl &decl) {
    if (const auto *var = dynamic_cast<const ast::VarDecl *>(&decl)) {
        emitVarDecl(*var, !inFunction_);
        return;
    }
    if (const auto *func = dynamic_cast<const ast::FunctionDecl *>(&decl)) {
        emitFunctionDecl(*func);
        return;
    }
    throw std::logic_error("unsupported declaration node" + locText(decl));
}

void CodeGen::emitVarDecl(const ast::VarDecl &decl, bool isGlobal) {
    const auto attrs = std::vector<Attr>{{"name", decl.name},
                                         {"type", typeTag(decl.type)},
                                         {"size", std::to_string(allocSizeBytes(decl.type))}};

    Op *storage = nullptr;
    if (isGlobal) {
        storage = builder_.createOp(Opcode::GlobalOp, ValueType::Void, {}, attrs);
        globals_[decl.name] = Symbol{Value(storage), decl.type};
    } else {
        storage = builder_.createOp(Opcode::AllocaOp, ValueType::Int, {}, attrs);
        if (localScopes_.empty()) {
            throw std::logic_error("local scope is empty while emitting local var '" + decl.name +
                                   "'" + locText(decl));
        }
        localScopes_.back()[decl.name] = Symbol{Value(storage), decl.type};
    }

    if (decl.initializer != nullptr) {
        if (decl.type.isArray()) {
            const ast::BuiltinType elemBase = decl.type.base;
            Value baseAddr(storage);

            if (const auto *list = dynamic_cast<const ast::InitListExpr *>(decl.initializer.get())) {
                auto emitStoreAt = [&](std::size_t linearOffset, Value rhs) {
                    Op *byteOffset = builder_.createOp(
                        Opcode::IntOp, ValueType::Int, {},
                        std::vector<Attr>{{"value", std::to_string(linearOffset * 4)}});
                    Op *elemAddr = builder_.createOp(Opcode::AddLOp, ValueType::Int,
                                                     {baseAddr, Value(byteOffset)});
                    builder_.createOp(Opcode::StoreOp, ValueType::Void, {rhs, Value(elemAddr)});
                };
                auto emitZeroAt = [&](std::size_t linearOffset) {
                    Op *zero = builder_.createOp(
                        elemBase == ast::BuiltinType::Float ? Opcode::FloatOp : Opcode::IntOp,
                        elemBase == ast::BuiltinType::Float ? ValueType::Float : ValueType::Int,
                        {}, std::vector<Attr>{{"value", elemBase == ast::BuiltinType::Float ? "0.0" : "0"}});
                    emitStoreAt(linearOffset, Value(zero));
                };

                std::function<void(std::size_t, std::size_t)> emitZeroRange;
                emitZeroRange = [&](std::size_t start, std::size_t count) {
                    for (std::size_t i = 0; i < count; ++i) {
                        emitZeroAt(start + i);
                    }
                };

                std::function<void(const ast::InitListExpr &, std::size_t, std::size_t)> emitInitList;
                emitInitList = [&](const ast::InitListExpr &initList, std::size_t level, std::size_t startOffset) {
                    const bool leafAggregate = level + 1 >= decl.type.shape.size();
                    const std::size_t childSize = leafAggregate ? 1 : productFromShape(decl.type.shape, level + 1);
                    const std::size_t aggregateSize = productFromShape(decl.type.shape, level);
                    std::vector<bool> covered(aggregateSize, false);

                    for (std::size_t i = 0; i < initList.values.size(); ++i) {
                        const std::size_t relOffset =
                            i < initList.valueOffsets.size() ? initList.valueOffsets[i] : i * childSize;
                        const ast::Expr *valueExpr = initList.values[i].get();
                        const auto *nested = dynamic_cast<const ast::InitListExpr *>(valueExpr);
                        const std::size_t coveredSize = nested != nullptr ? childSize : 1;
                        for (std::size_t j = 0; j < coveredSize && relOffset + j < aggregateSize; ++j) {
                            covered[relOffset + j] = true;
                        }
                        if (nested != nullptr) {
                            emitInitList(*nested, level + 1, startOffset + relOffset);
                        } else {
                            emitStoreAt(startOffset + relOffset, emitExpr(*valueExpr));
                        }
                    }

                    if (!isGlobal) {
                        for (std::size_t i = 0; i < aggregateSize; ++i) {
                            if (!covered[i]) {
                                emitZeroAt(startOffset + i);
                            }
                        }
                    }
                };
                emitInitList(*list, 0, 0);
            }
            return;
        }
        Value rhs = emitExpr(*decl.initializer);
        builder_.createOp(Opcode::StoreOp, ValueType::Void, {rhs, Value(storage)});
    }
}

void CodeGen::emitFunctionDecl(const ast::FunctionDecl &decl) {
    builder_.createBlock("fn.decl." + decl.name + "." + std::to_string(blockId_++));
    const auto fnAttrs = std::vector<Attr>{{"name", decl.name},
                                           {"ret", typeTag(decl.returnType)},
                                           {"argc", std::to_string(decl.params.size())}};
    builder_.createOp(Opcode::FuncOp, ValueType::Void, {}, fnAttrs);
    BasicBlock *fnEntry = newBlock("fn.entry");
    builder_.setInsertPoint(fnEntry);

    inFunction_ = true;
    currentReturnType_ = decl.returnType;
    pushScope();

    int intArgIdx = 0;
    int floatArgIdx = 0;
    int stackArgIdx = 0;

    for (std::size_t i = 0; i < decl.params.size(); ++i) {
        const ast::ParamDecl &param = *decl.params[i];
        std::vector<Attr> argAttrs{{"idx", std::to_string(i)},
                                   {"name", param.name},
                                   {"type", typeTag(param.type)}};
        if (isFloatType(param.type) && !param.type.isPointer) {
            if (floatArgIdx < 8) {
                argAttrs.emplace_back("reg", "fa" + std::to_string(floatArgIdx));
            } else {
                argAttrs.emplace_back("stack", std::to_string(stackArgIdx++));
            }
            floatArgIdx++;
        } else {
            if (intArgIdx < 8) {
                argAttrs.emplace_back("reg", "a" + std::to_string(intArgIdx));
            } else {
                argAttrs.emplace_back("stack", std::to_string(stackArgIdx++));
            }
            intArgIdx++;
        }

        Op *arg = builder_.createOp(Opcode::GetArgOp, toValueType(param.type), {},
                                    std::move(argAttrs));
        Op *addr = builder_.createOp(Opcode::AllocaOp, ValueType::Int, {},
                                     std::vector<Attr>{{"name", param.name},
                                                       {"type", typeTag(param.type)},
                                                       {"size", std::to_string(allocSizeBytes(param.type))}});
        builder_.createOp(Opcode::StoreOp, ValueType::Void, {Value(arg), Value(addr)});
        localScopes_.back()[param.name] = Symbol{Value(addr), param.type};
    }

    if (decl.body != nullptr) {
        emitStmt(*decl.body);
    }

    if (currentReturnType_.base == ast::BuiltinType::Void) {
        builder_.createOp(Opcode::ReturnOp, ValueType::Void);
    }

    popScope();
    inFunction_ = false;
}

void CodeGen::emitStmt(const ast::Stmt &stmt) {
    if (const auto *compound = dynamic_cast<const ast::CompoundStmt *>(&stmt)) {
        pushScope();
        for (const auto &child : compound->statements) {
            emitStmt(*child);
        }
        popScope();
        return;
    }

    if (const auto *declStmt = dynamic_cast<const ast::DeclStmt *>(&stmt)) {
        for (const auto &decl : declStmt->declarations) {
            emitVarDecl(*decl, false);
        }
        return;
    }

    if (dynamic_cast<const ast::NullStmt *>(&stmt) != nullptr) {
        return;
    }

    if (const auto *ifStmt = dynamic_cast<const ast::IfStmt *>(&stmt)) {
        Value cond = emitBoolExpr(*ifStmt->condition);
        BasicBlock *thenBlock = newBlock("if.then");
        BasicBlock *elseBlock =
            ifStmt->elseBranch != nullptr ? newBlock("if.else") : nullptr;
        BasicBlock *mergeBlock = newBlock("if.end");
        emitBranch(cond, thenBlock, elseBlock != nullptr ? elseBlock : mergeBlock);

        builder_.setInsertPoint(thenBlock);
        emitStmt(*ifStmt->thenBranch);
        ensureGotoIfNotTerminated(mergeBlock);

        if (ifStmt->elseBranch != nullptr) {
            builder_.setInsertPoint(elseBlock);
            emitStmt(*ifStmt->elseBranch);
            ensureGotoIfNotTerminated(mergeBlock);
        }

        builder_.setInsertPoint(mergeBlock);
        return;
    }

    if (const auto *whileStmt = dynamic_cast<const ast::WhileStmt *>(&stmt)) {
        BasicBlock *condBlock = newBlock("while.cond");
        BasicBlock *bodyBlock = newBlock("while.body");
        BasicBlock *endBlock = newBlock("while.end");

        emitGoto(condBlock);

        builder_.setInsertPoint(condBlock);
        Value cond = emitBoolExpr(*whileStmt->condition);
        emitBranch(cond, bodyBlock, endBlock);

        loopTargets_.push_back({endBlock, condBlock});
        builder_.setInsertPoint(bodyBlock);
        emitStmt(*whileStmt->body);
        ensureGotoIfNotTerminated(condBlock);
        loopTargets_.pop_back();

        builder_.setInsertPoint(endBlock);
        return;
    }

    if (dynamic_cast<const ast::BreakStmt *>(&stmt) != nullptr) {
        if (loopTargets_.empty()) {
            throw std::logic_error("break used outside loop" + locText(stmt));
        }
        builder_.createOp(Opcode::BreakOp, ValueType::Void);
        emitGoto(loopTargets_.back().first);
        startUnreachableContinuation();
        return;
    }

    if (dynamic_cast<const ast::ContinueStmt *>(&stmt) != nullptr) {
        if (loopTargets_.empty()) {
            throw std::logic_error("continue used outside loop" + locText(stmt));
        }
        builder_.createOp(Opcode::ContinueOp, ValueType::Void);
        emitGoto(loopTargets_.back().second);
        startUnreachableContinuation();
        return;
    }

    if (const auto *ret = dynamic_cast<const ast::ReturnStmt *>(&stmt)) {
        if (ret->value == nullptr) {
            builder_.createOp(Opcode::ReturnOp, ValueType::Void);
        } else {
            Value value = emitExpr(*ret->value);
            builder_.createOp(Opcode::ReturnOp, ValueType::Void, {value});
        }
        startUnreachableContinuation();
        return;
    }

    if (const auto *expr = dynamic_cast<const ast::Expr *>(&stmt)) {
        (void)emitExpr(*expr);
        return;
    }

    throw std::logic_error("unsupported statement node: " + stmtKind(stmt) + locText(stmt));
}

Value CodeGen::emitExpr(const ast::Expr &expr) {
    if (const auto *lit = dynamic_cast<const ast::IntegerLiteral *>(&expr)) {
        Op *op = builder_.createOp(Opcode::IntOp, ValueType::Int, {},
                                   std::vector<Attr>{{"value", lit->text}});
        return Value(op);
    }

    if (const auto *lit = dynamic_cast<const ast::FloatLiteral *>(&expr)) {
        Op *op = builder_.createOp(Opcode::FloatOp, ValueType::Float, {},
                                   std::vector<Attr>{{"value", lit->text}});
        return Value(op);
    }

    if (const auto *ref = dynamic_cast<const ast::DeclRefExpr *>(&expr)) {
        return emitDeclRef(*ref);
    }

    if (const auto *cast = dynamic_cast<const ast::ImplicitCastExpr *>(&expr)) {
        return emitImplicitCast(*cast);
    }

    if (const auto *paren = dynamic_cast<const ast::ParenExpr *>(&expr)) {
        return emitExpr(*paren->subExpr);
    }

    if (const auto *unary = dynamic_cast<const ast::UnaryOperator *>(&expr)) {
        return emitUnary(*unary);
    }

    if (const auto *binary = dynamic_cast<const ast::BinaryOperator *>(&expr)) {
        return emitBinary(*binary);
    }

    if (const auto *call = dynamic_cast<const ast::CallExpr *>(&expr)) {
        return emitCall(*call);
    }

    if (const auto *subscript = dynamic_cast<const ast::ArraySubscriptExpr *>(&expr)) {
        Value addr = emitLValueAddress(*subscript);
        if (subscript->inferredType.isArray()) {
            return addr;
        }
        Op *load = builder_.createOp(
            Opcode::LoadOp, toValueType(subscript->inferredType), {addr},
            std::vector<Attr>{{"type", typeTag(subscript->inferredType)}});
        return Value(load);
    }

    if (const auto *implicitInit = dynamic_cast<const ast::ImplicitValueInitExpr *>(&expr)) {
        if (implicitInit->targetType.base == ast::BuiltinType::Float) {
            Op *op = builder_.createOp(Opcode::FloatOp, ValueType::Float, {},
                                       std::vector<Attr>{{"value", "0.0"}});
            return Value(op);
        }
        Op *op = builder_.createOp(Opcode::IntOp, ValueType::Int, {},
                                   std::vector<Attr>{{"value", "0"}});
        return Value(op);
    }

    throw std::logic_error("unsupported expression: " + exprKind(expr) + locText(expr));
}

Value CodeGen::emitDeclRef(const ast::DeclRefExpr &expr) {
    const Symbol *sym = resolveAny(expr.name);
    if (sym == nullptr) {
        throw std::logic_error("unknown symbol: " + expr.name + locText(expr));
    }
    if (sym->type.isArray()) {
        return sym->address;
    }
    Op *load = builder_.createOp(Opcode::LoadOp, toValueType(sym->type), {sym->address},
                                 std::vector<Attr>{{"type", typeTag(sym->type)}});
    return Value(load);
}

Value CodeGen::emitLValueAddress(const ast::Expr &expr) {
    if (const auto *paren = dynamic_cast<const ast::ParenExpr *>(&expr)) {
        return emitLValueAddress(*paren->subExpr);
    }
    if (const auto *cast = dynamic_cast<const ast::ImplicitCastExpr *>(&expr)) {
        return emitLValueAddress(*cast->subExpr);
    }

    if (const auto *ref = dynamic_cast<const ast::DeclRefExpr *>(&expr)) {
        const Symbol *sym = resolveAny(ref->name);
        if (sym == nullptr) {
            throw std::logic_error("unknown symbol: " + ref->name + locText(*ref));
        }
        if (sym->type.isPointer) {
            Op *ptr = builder_.createOp(Opcode::LoadOp, ValueType::Int, {sym->address},
                                        std::vector<Attr>{{"type", typeTag(sym->type)}});
            return Value(ptr);
        }
        return sym->address;
    }

    if (const auto *subscript = dynamic_cast<const ast::ArraySubscriptExpr *>(&expr)) {
        Value baseAddr = emitLValueAddress(*subscript->base);
        Value index = emitExpr(*subscript->index);

        std::size_t strideElements = 1;
        const ast::Type baseType = subscript->base->inferredType;
        if (!baseType.shape.empty()) {
            strideElements =
                productFromShape(baseType.shape, baseType.isPointer ? 0 : 1);
        }
        const std::size_t strideBytes = strideElements * 4;
        Op *stride = builder_.createOp(Opcode::IntOp, ValueType::Int, {},
                                       std::vector<Attr>{{"value", std::to_string(strideBytes)}});
        Op *byteOffset = builder_.createOp(Opcode::MulIOp, ValueType::Int, {index, Value(stride)});
        Op *elemAddr = builder_.createOp(Opcode::AddLOp, ValueType::Int,
                                         {baseAddr, Value(byteOffset)});
        return Value(elemAddr);
    }

    throw std::logic_error("unsupported lvalue expression: " + exprKind(expr) + locText(expr));
}

Value CodeGen::emitUnary(const ast::UnaryOperator &expr) {
    if (expr.opcode == ast::UnaryOpcode::Plus) {
        return emitExpr(*expr.operand);
    }

    if (expr.opcode == ast::UnaryOpcode::Minus) {
        Value operand = emitExpr(*expr.operand);
        Op *op = builder_.createOp(isFloatType(expr.inferredType) ? Opcode::MinusFOp : Opcode::MinusOp,
                                   toValueType(expr.inferredType), {operand});
        return Value(op);
    }

    Value operand = emitBoolExpr(*expr.operand);
    Op *op = builder_.createOp(Opcode::NotOp, ValueType::Int, {operand});
    return Value(op);
}

Value CodeGen::emitBinary(const ast::BinaryOperator &expr) {
    if (expr.opcode == ast::BinaryOpcode::Assign) {
        Value addr = emitLValueAddress(*expr.lhs);
        Value rhs = emitExpr(*expr.rhs);
        builder_.createOp(Opcode::StoreOp, ValueType::Void, {rhs, addr});
        return rhs;
    }

    if (expr.opcode == ast::BinaryOpcode::LogicalAnd ||
        expr.opcode == ast::BinaryOpcode::LogicalOr) {
        Op *resultAddr = builder_.createOp(
            Opcode::AllocaOp, ValueType::Int, {},
            std::vector<Attr>{{"type", "int"}, {"size", "4"}});
        BasicBlock *rhsBlock = newBlock("logic.rhs");
        BasicBlock *shortBlock = newBlock("logic.short");
        BasicBlock *endBlock = newBlock("logic.end");

        Value lhs = emitBoolExpr(*expr.lhs);
        if (expr.opcode == ast::BinaryOpcode::LogicalAnd) {
            emitBranch(lhs, rhsBlock, shortBlock);
        } else {
            emitBranch(lhs, shortBlock, rhsBlock);
        }

        builder_.setInsertPoint(shortBlock);
        Op *shortValue = builder_.createOp(
            Opcode::IntOp, ValueType::Int, {},
            std::vector<Attr>{{"value", expr.opcode == ast::BinaryOpcode::LogicalAnd ? "0" : "1"}});
        builder_.createOp(Opcode::StoreOp, ValueType::Void, {Value(shortValue), Value(resultAddr)});
        emitGoto(endBlock);

        builder_.setInsertPoint(rhsBlock);
        Value rhs = emitBoolExpr(*expr.rhs);
        builder_.createOp(Opcode::StoreOp, ValueType::Void, {rhs, Value(resultAddr)});
        emitGoto(endBlock);

        builder_.setInsertPoint(endBlock);
        Op *load = builder_.createOp(
            Opcode::LoadOp, ValueType::Int, {Value(resultAddr)},
            std::vector<Attr>{{"type", "int"}});
        return Value(load);
    }

    Value lhs = emitExpr(*expr.lhs);
    Value rhs = emitExpr(*expr.rhs);
    const bool isFloat = isFloatType(expr.lhs->inferredType) || isFloatType(expr.rhs->inferredType);

    Opcode opcode = Opcode::AddIOp;
    switch (expr.opcode) {
    case ast::BinaryOpcode::Assign:
        break;
    case ast::BinaryOpcode::Add:
        opcode = isFloat ? Opcode::AddFOp : Opcode::AddIOp;
        break;
    case ast::BinaryOpcode::Sub:
        opcode = isFloat ? Opcode::SubFOp : Opcode::SubIOp;
        break;
    case ast::BinaryOpcode::Mul:
        opcode = isFloat ? Opcode::MulFOp : Opcode::MulIOp;
        break;
    case ast::BinaryOpcode::Div:
        opcode = isFloat ? Opcode::DivFOp : Opcode::DivIOp;
        break;
    case ast::BinaryOpcode::Mod:
        opcode = isFloat ? Opcode::ModFOp : Opcode::ModIOp;
        break;
    case ast::BinaryOpcode::Less:
        opcode = isFloat ? Opcode::LtFOp : Opcode::LtOp;
        break;
    case ast::BinaryOpcode::Greater:
        opcode = isFloat ? Opcode::LtFOp : Opcode::LtOp;
        std::swap(lhs, rhs);
        break;
    case ast::BinaryOpcode::LessEqual:
        opcode = isFloat ? Opcode::LeFOp : Opcode::LeOp;
        break;
    case ast::BinaryOpcode::GreaterEqual:
        opcode = isFloat ? Opcode::LeFOp : Opcode::LeOp;
        std::swap(lhs, rhs);
        break;
    case ast::BinaryOpcode::Equal:
        opcode = isFloat ? Opcode::EqFOp : Opcode::EqOp;
        break;
    case ast::BinaryOpcode::NotEqual:
        opcode = isFloat ? Opcode::NeFOp : Opcode::NeOp;
        break;
    case ast::BinaryOpcode::LogicalAnd:
    case ast::BinaryOpcode::LogicalOr:
        break;
    }

    const ValueType retType =
        (expr.opcode == ast::BinaryOpcode::Less || expr.opcode == ast::BinaryOpcode::Greater ||
         expr.opcode == ast::BinaryOpcode::LessEqual ||
         expr.opcode == ast::BinaryOpcode::GreaterEqual ||
         expr.opcode == ast::BinaryOpcode::Equal ||
         expr.opcode == ast::BinaryOpcode::NotEqual)
            ? ValueType::Int
            : (isFloat ? ValueType::Float : ValueType::Int);

    Op *op = builder_.createOp(opcode, retType, {lhs, rhs});
    return Value(op);
}

Value CodeGen::emitCall(const ast::CallExpr &expr) {
    std::vector<Value> args;
    args.reserve(expr.arguments.size());
    for (const auto &arg : expr.arguments) {
        args.push_back(emitExpr(*arg));
    }

    const ValueType retType = toValueType(expr.inferredType);
    Op *op = builder_.createOp(Opcode::CallOp, retType, std::move(args),
                               std::vector<Attr>{{"callee", expr.callee},
                                                 {"ret", typeTag(expr.inferredType)}});
    return Value(op);
}

Value CodeGen::emitBoolExpr(const ast::Expr &expr) {
    return toBoolValue(emitExpr(expr), expr.inferredType);
}

Value CodeGen::emitImplicitCast(const ast::ImplicitCastExpr &expr) {
    Value input = emitExpr(*expr.subExpr);
    switch (expr.kind) {
    case ast::CastKind::IntToFloat: {
        Op *op = builder_.createOp(Opcode::I2FOp, ValueType::Float, {input});
        return Value(op);
    }
    case ast::CastKind::FloatToInt: {
        Op *op = builder_.createOp(Opcode::F2IOp, ValueType::Int, {input});
        return Value(op);
    }
    case ast::CastKind::IntToBool:
    case ast::CastKind::FloatToBool: {
        Op *op = builder_.createOp(Opcode::SetNotZeroOp, ValueType::Int, {input});
        return Value(op);
    }
    case ast::CastKind::LValueToRValue:
    case ast::CastKind::ArrayToPointerDecay:
        return input;
    }

    throw std::logic_error("unsupported implicit cast kind" + locText(expr));
}

Value CodeGen::toBoolValue(Value value, ast::Type type) {
    if (type.isPointer || type.base == ast::BuiltinType::Int) {
        Op *op = builder_.createOp(Opcode::SetNotZeroOp, ValueType::Int, {value});
        return Value(op);
    }
    if (type.base == ast::BuiltinType::Float) {
        Op *zero = builder_.createOp(Opcode::FloatOp, ValueType::Float, {},
                                     std::vector<Attr>{{"value", "0.0"}});
        Op *op = builder_.createOp(Opcode::NeFOp, ValueType::Int, {value, Value(zero)});
        return Value(op);
    }
    return value;
}

CodeGen::Symbol *CodeGen::resolveLocal(const std::string &name) {
    for (auto it = localScopes_.rbegin(); it != localScopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

const CodeGen::Symbol *CodeGen::resolveLocal(const std::string &name) const {
    for (auto it = localScopes_.rbegin(); it != localScopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

const CodeGen::Symbol *CodeGen::resolveAny(const std::string &name) const {
    if (const Symbol *local = resolveLocal(name)) {
        return local;
    }
    auto globalIt = globals_.find(name);
    if (globalIt != globals_.end()) {
        return &globalIt->second;
    }
    return nullptr;
}

void CodeGen::pushScope() {
    localScopes_.emplace_back();
}

void CodeGen::popScope() {
    if (localScopes_.empty()) {
        throw std::logic_error("scope stack underflow");
    }
    localScopes_.pop_back();
}

bool CodeGen::isFloatType(ast::Type type) {
    return type.base == ast::BuiltinType::Float;
}

ValueType CodeGen::toValueType(ast::Type type) {
    if (type.isPointer || type.isArray()) {
        return ValueType::Int;
    }
    if (type.base == ast::BuiltinType::Float) {
        return ValueType::Float;
    }
    if (type.base == ast::BuiltinType::Void) {
        return ValueType::Void;
    }
    return ValueType::Int;
}

std::string CodeGen::typeTag(ast::Type type) {
    return ast::typeToString(type);
}

std::size_t CodeGen::allocSizeBytes(ast::Type type) {
    if (type.isPointer) {
        return 8;
    }
    std::size_t elements = 1;
    if (type.isArray()) {
        for (long long dim : type.shape) {
            if (dim > 0) {
                elements *= static_cast<std::size_t>(dim);
            }
        }
    }
    return elements * 4;
}

bool CodeGen::isTerminator(Opcode opcode) {
    return opcode == Opcode::ReturnOp || opcode == Opcode::GotoOp ||
           opcode == Opcode::BranchOp;
}

BasicBlock *CodeGen::newBlock(const std::string &prefix) {
    return builder_.region()->createBlock(prefix + "." + std::to_string(blockId_++));
}

void CodeGen::emitGoto(BasicBlock *target) {
    builder_.createOp(Opcode::GotoOp, ValueType::Void, {},
                      std::vector<Attr>{{"target", target->name()}});
}

void CodeGen::emitBranch(Value cond, BasicBlock *trueTarget, BasicBlock *falseTarget) {
    builder_.createOp(Opcode::BranchOp, ValueType::Void, {cond},
                      std::vector<Attr>{{"true", trueTarget->name()},
                                        {"false", falseTarget->name()}});
}

void CodeGen::ensureGotoIfNotTerminated(BasicBlock *target) {
    BasicBlock *cur = builder_.insertBlock();
    if (cur == nullptr || cur->empty()) {
        emitGoto(target);
        return;
    }
    Op *last = cur->opAt(cur->size() - 1);
    if (last == nullptr || !isTerminator(last->opcode())) {
        emitGoto(target);
    }
}

void CodeGen::startUnreachableContinuation() {
    BasicBlock *dead = newBlock("dead");
    builder_.setInsertPoint(dead);
}

} // namespace codegen
