#pragma once

#include "frontend/AST.h"

#include <iosfwd>
#include <string>

class ASTDumper {
public:
    explicit ASTDumper(std::ostream &out);

    void dump(const ast::TranslationUnit &unit);

private:
    void dumpDecl(const ast::Decl &decl, const std::string &prefix, bool isLast);
    void dumpStmt(const ast::Stmt &stmt, const std::string &prefix, bool isLast);
    void dumpExpr(const ast::Expr &expr, const std::string &prefix, bool isLast);

    void dumpVarDeclChildren(const ast::VarDecl &decl, const std::string &prefix,
                             bool isLastParent);
    void dumpParamDeclChildren(const ast::ParamDecl &decl, const std::string &prefix,
                               bool isLastParent);
    void dumpParamDecl(const ast::ParamDecl &decl, const std::string &prefix,
                       bool isLast);
    void writeNode(const std::string &prefix, bool isLast, const std::string &text);

    std::ostream &out_;
};
