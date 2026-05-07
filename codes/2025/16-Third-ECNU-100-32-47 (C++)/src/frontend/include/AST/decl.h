#ifndef AAAC_AST_H
#define AAAC_AST_H

#include "AST/const_value.h"
#include "AST/type.h"
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <vector>
#pragma once

namespace aaac {
namespace frontend {
class Expr;
class CompondStmt;
class ASTContext;
class Decl {
public:
    Decl(int pos,std::string ident): pos_(pos), Ident(ident) {}
    virtual ~Decl() = default;
    int getPos() const { return pos_; };
    std::string getIdent() const { return Ident; };
    virtual void PrintTo(std::ostream &os, int d) const = 0;
    virtual std::string getAsString() const;
    virtual void SemAnalyze(ASTContext *Context) = 0;
    virtual void Translate(ASTContext *Context) = 0;
private:
    int pos_;
    std::string Ident;
};

class TUDecl : public Decl {
public:
    TUDecl(int pos): Decl(pos,"") {
        PopulateLibFunction();
    }
    ~TUDecl() {
        for(Decl *decl : decls) delete decl;
    }
    void PrintTo(std::ostream &os, int d) const override;
    TUDecl *append(Decl *decl) { decls.push_back(decl); return this; }
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    void PopulateLibFunction();

private:
    std::vector<Decl *> decls;
    bool HasPopulatedLibFunc = false;
};    


class DeclList;

class VarDecl : public Decl, public ConstValue {
    friend DeclList;
    friend class Var;
public:
    VarDecl(int pos, std::string BType, std::string ident, Expr *init):
        Decl(pos,ident), BType(BType), init(init) {}
    ~VarDecl();
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;
    std::shared_ptr<Type> getType() { return type; }

    void setEscape(bool flag = true) { escape = flag; }
    bool isEscape() const { return escape; }

    void setFormalParm(bool flag = true) { FormalParm = flag; }
    bool isFormalParm() const { return FormalParm; }

    void setGlobal(bool flag = true) { Global = flag; }
    bool isGlobal() const { return Global; }
    
protected:
    std::string BType;
    Expr* init;
    bool escape = false;
    bool FormalParm = false;
    bool Global = false;
    std::shared_ptr<Type> type = nullptr;
};

class ArrayDecl : public VarDecl {
public:
    ArrayDecl(int pos, std::string BType, std::string ident, Expr *init): 
    VarDecl(pos,BType,ident,init) {}
    ~ArrayDecl();
    void appendSubscript(Expr *subscript) { subscripts.push_back(subscript); }
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

private:
    std::vector<Expr *> subscripts;
};

// helper class for constructing ArrayDecl
class SubList {
public:
    SubList* append(Expr *expr) { patch_list.push_back(expr); return this; }
    SubList* prepend(Expr *expr) { patch_list.push_front(expr); return this; }
    void patchTo(ArrayDecl *arr) {
        for(Expr *expr : patch_list) {
            arr->appendSubscript(expr);
        }
    }
private:
    std::list<Expr *> patch_list;
};

class FuncDecl;
// helper class for constructing tudecl or compoundstmt
class DeclList {
public:
    DeclList* append(VarDecl *decl) { patch_list.push_back(decl); return this; }
    DeclList* append(FuncDecl *func) { func_decl = func; return this;}
    void patch(std::string BType, bool isConst = false) {
        for(auto ptr : patch_list) {
            // ptr->isConst_ = isConst;
            ptr->setConst(isConst);
            ptr->BType = BType;
        }
    }
    DeclList* patchTo(TUDecl *tu);
    DeclList* patchTo(CompondStmt *compond);
private:
    std::list<VarDecl *> patch_list;
    FuncDecl *func_decl;
};

using FParmVarDecl = VarDecl;
using FParmList = std::list<FParmVarDecl *>;
class FuncDecl : public Decl {
public:
    FuncDecl(int pos, std::string retType, std::string ident, FParmList *fparms, CompondStmt *body):
        Decl(pos,ident), retType(retType), fparms(fparms), body(body) {}
    ~FuncDecl();
    void PrintTo(std::ostream &os, int d) const override;
    void SemAnalyze(ASTContext *Context) override;
    void Translate(ASTContext *Context) override;

    std::shared_ptr<FunctionType> getFunctionType() { return type; }
    FParmList *getFparms() { return fparms; }

    void setFwdDecl(bool flag = true) { ForwardDeclaration = flag; }
    bool isFwdDecl() const { return ForwardDeclaration; }

private:
    std::string retType;
    FParmList *fparms;
    CompondStmt *body;
    std::shared_ptr<FunctionType> type;
    bool ForwardDeclaration = false;
};

} // namespace frontend
} // namespace aaac


#endif