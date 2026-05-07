#ifndef AAAC_STMT_H
#define AAAC_STMT_H
#include <iostream>
#include <list>
#include <vector>
#pragma once

namespace aaac {
namespace frontend {

class Expr;
class Var;
class SubscriptVar;
class Decl;
class ASTContext;

class Stmt {
public:
    Stmt(int pos) : pos(pos) {}
    virtual ~Stmt() = default;
    int getPos() const { return pos; };
    virtual void PrintTo(std::ostream &os,int d) const = 0;
    virtual std::string getAsString() const;
    virtual void SemAnalyze(ASTContext *Context) = 0;
    virtual void Translate(ASTContext *Context) = 0;

private:
    int pos;
};

class AssignStmt : public Stmt {
public:
    AssignStmt(int pos, Var *var, Expr *expr):
        Stmt(pos), var(var), expr(expr) {}
    ~AssignStmt();
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    Var *var;
    Expr *expr;
};

class DeclStmt : public Stmt {
public:
    DeclStmt(int pos, Decl *decl): Stmt(pos), decl(decl) {}
    ~DeclStmt();
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    Decl *decl;
};

class CompondStmt : public Stmt {
public:
    CompondStmt(int pos): Stmt(pos) {}
    ~CompondStmt();
    void appendStmt(Stmt *stmt) { stmts.push_back(stmt); }
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    std::list<Stmt *> stmts;
};

class IfStmt : public Stmt {
public:
    IfStmt(int pos, Expr *cond, Stmt *then, Stmt *elsee):
        Stmt(pos), cond(cond), then(then), elsee(elsee) {}
    ~IfStmt();
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    void TranslateWithElse(ASTContext *Context);
    void TranslateNoElse(ASTContext *Context);

    Expr *cond;
    Stmt *then;
    Stmt *elsee;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(int pos, Expr *cond, Stmt *body):
        Stmt(pos), cond(cond), body(body) {}
    ~WhileStmt();
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    Expr *cond;
    Stmt *body;
};

class ReturnStmt : public Stmt {
public:
    ReturnStmt(int pos, Expr* ret): Stmt(pos), ret(ret) {}
    ~ReturnStmt();
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    Expr *ret;
};

class BreakStmt : public Stmt {
public:
    BreakStmt(int pos): Stmt(pos) {}
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
};

class ContinueStmt : public Stmt {
public:
    ContinueStmt(int pos): Stmt(pos) {}
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
};

class EmptyStmt : public Stmt {
public:
    EmptyStmt(int pos):Stmt(pos) {}
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
};

} // namespace frontend
} // namespace aaac

#endif