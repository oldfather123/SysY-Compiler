#ifndef AAAC_EXPR_H
#define AAAC_EXPR_H

#include "AST/const_value.h"
#include "AST/decl.h"
#include "AST/type.h"
#include "common.h"
#include "frontend.h"
#include "stmt.h"
#include "sym.h"
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>
#pragma once

namespace aaac {
namespace frontend {

class Expr : public Stmt, public ConstValue {
public:
    Expr(int pos):Stmt(pos), ConstValue(), extype(Expressioned) {}
    ~Expr() = default;
    virtual void SemAnalyze(ASTContext *Context) = 0;
    virtual void Translate(ASTContext *Context) = 0;

    virtual std::shared_ptr<Type> getType() const { return type; }
    void setType(std::shared_ptr<Type> type) { this->type = type; }

    enum ExType: int {
        Expressioned,
        Conditioned,
        Voided,
    };
    void setExType(ExType type) { extype = type; }
    ExType getExType() const { return extype; }
protected:
    std::shared_ptr<Type> type = nullptr;
    ExType extype;
};

enum UnaryOp {
    Positive,
    Negative,
    Negate,
};

std::string getString(UnaryOp op);

class UnaryOperator : public Expr {
public:
    UnaryOperator(int pos, UnaryOp op, Expr *expr): Expr(pos), op(op), expr(expr) { }
    ~UnaryOperator() { delete expr; }
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
private:
    void TranslateLogic(ASTContext *Context);
    void TranslateArith(ASTContext *Context);

private:
    UnaryOp op;
    Expr *expr;
};

enum BinaryOp {
    PLUS_OP,
    MINUS_OP,
    TIMES_OP,
    DIVIDE_OP,
    MODULE_OP,
    EQ_OP,
    NEQ_OP,
    LT_OP,
    LE_OP,
    GT_OP,
    GE_OP,
    AND_OP,
    OR_OP,
};

std::string getString(BinaryOp op);
bool isArithOp(BinaryOp op);
bool isCondOp(BinaryOp op);
template<typename T>
T calculateArith(T lhs, T rhs, BinaryOp op) {
    switch (op) {
        case PLUS_OP:
            return lhs + rhs;
        case MINUS_OP:
            return lhs - rhs;
        case TIMES_OP:
            return lhs * rhs;
        case DIVIDE_OP:
            return lhs / rhs;
        case MODULE_OP:
            assert(typeid(T) == typeid(int));
            return (int)lhs % (int)rhs;
        default:
            Assert(false,"unreachable");
    }
}

class BinaryOperator : public Expr {
public:
    BinaryOperator(int pos, BinaryOp op, Expr *lhs, Expr *rhs):
        Expr(pos), op(op), lhs(lhs), rhs(rhs) {}
    ~BinaryOperator() { delete lhs; delete rhs; }
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    bool isCond() { return PLUS_OP <= op && op <= MODULE_OP; }
    bool isExpr() { return EQ_OP <= op && op <= OR_OP; }
    void CondSemAnalyze(ASTContext *Context);
    void ArithSemAnalyze(ASTContext *Context);
    void TranslateLogic(ASTContext *Context);
    void TranslateCompr(ASTContext *Context);
    void TranslateArith(ASTContext *Context);
private:
    BinaryOp op;
    Expr *lhs, *rhs;
};

template<typename T>
class Literal : public Expr {
public:
    Literal<T>(int pos, T value):Expr(pos), value(value) {}
    void setValue(T v) { value = v; }
    T getValue() { return value; }
protected:
    T value;
};

class IntLiteral : public Literal<int> {
public:
    IntLiteral(int pos, int value):Literal<int>(pos,value) {
        type = TypeFactory::IntTy; 
        setConst();
        setConstExpr(this);
    }
    ~IntLiteral() { setConstExpr(nullptr); }
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
};

class FloatLiteral : public Literal<float> {
public:
    FloatLiteral(int pos, float value): Literal<float>(pos, value) {
        type = TypeFactory::FloatTy;
        setConst();
        setConstExpr(this);
    }
    ~FloatLiteral() { setConstExpr(nullptr); }
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
};

enum class CastType : uint8_t{
    IntToFloat,
    FloatToInt,
};
class ImplicitCast : public Expr {
public:
    ImplicitCast(Expr *expr, CastType castTy);
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
    
private:
    Expr *expr;
    CastType castTy;
};

class InitList : public Expr {
public:
    InitList(int pos): Expr(pos) {}
    ~InitList() {
        for(Expr *expr : init_list) delete expr;
    }
    void PrintTo(std::ostream &os, int d) const override;
    InitList* append(Expr *expr) { init_list.push_back(expr); return this; }
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
    void TranslateGlobal(ASTContext *Context);
    void TranslateLocal(ASTContext *Context);

    // size of init expr filled in bytes
    int getLen() { return init_list.size(); }
    void clear() { init_list.clear(); }
    InitList* copy_constlist(int subscript = -1);
    Expr* get_element(int subscript);

    void setReference(ArrayDecl *decl) { reference = decl; }

private:
    std::vector<Expr *> init_list;
    ArrayDecl *reference;
};

using RParm = Expr;
using RparmList = std::list<RParm *>;
class CallExpr : public Expr {
public:
    CallExpr(int pos, std::string func, RparmList *rparms):
        Expr(pos), func_name(func), rparms(rparms) {}
    ~CallExpr() {
        for(auto ptr : *rparms) delete ptr;
        delete rparms; 
    }
    // the type which other expr cares about is the return type, so we override
    // this method
    std::shared_ptr<Type> getType() const override;
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    std::string func_name;
    RparmList *rparms;
    FuncDecl *funcdecl;
};

// stands for simple lvalue
class Var : public Expr {
public:
    Var(int pos, std::string ident): Expr(pos),ident(ident),reference(nullptr) {}
    std::string getIdent() { return ident; };
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
    virtual void TranslateAssign(ASTContext *Context, Operand var);
    bool isGlobal() const { return reference->isGlobal(); }

private:
    std::string ident;
    VarDecl *reference;
};

class SubscriptVar : public Var {
public:
    SubscriptVar(int pos, Var *var, Expr *subscript):
        Var(pos,var->getIdent()), var(var), subscript(subscript) {}
    ~SubscriptVar() { delete var; delete subscript; }
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
    void TranslateAssign(ASTContext *Context, Operand var) override;

private:
    Var *var;
    Expr *subscript;
};

class EmptyExpr : public Expr {
public:
    EmptyExpr(int pos): Expr(pos) {}
    void PrintTo(std::ostream &os, int d) const override {};
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
};

} // namespace frontend
} // namespace aaac

#endif