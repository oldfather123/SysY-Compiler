#pragma once

#include "frontend/AST.h"
#include "frontend/generated/SysYParser.h"

#include <memory>
#include <vector>

class ASTBuilder {
public:
    std::unique_ptr<ast::TranslationUnit> build(SysYParser::CompUnitContext *ctx);

private:
    ast::SourceLocation locOf(antlr4::ParserRuleContext *ctx) const;
    ast::Type bType(SysYParser::BTypeContext *ctx, bool isConst) const;
    ast::Type funcType(SysYParser::FuncTypeContext *ctx) const;

    std::vector<std::unique_ptr<ast::Decl>> buildDecl(SysYParser::DeclContext *ctx);
    std::vector<std::unique_ptr<ast::VarDecl>>
    buildConstDecl(SysYParser::ConstDeclContext *ctx);
    std::vector<std::unique_ptr<ast::VarDecl>> buildVarDecl(SysYParser::VarDeclContext *ctx);
    std::unique_ptr<ast::FunctionDecl> buildFuncDef(SysYParser::FuncDefContext *ctx);
    std::unique_ptr<ast::ParamDecl> buildFuncFParam(SysYParser::FuncFParamContext *ctx);

    std::unique_ptr<ast::CompoundStmt> buildBlock(SysYParser::BlockContext *ctx);
    std::unique_ptr<ast::Stmt> buildBlockItem(SysYParser::BlockItemContext *ctx);
    std::unique_ptr<ast::Stmt> buildStmt(SysYParser::StmtContext *ctx);

    std::unique_ptr<ast::Expr> buildInitVal(SysYParser::InitValContext *ctx);
    std::unique_ptr<ast::Expr> buildConstInitVal(SysYParser::ConstInitValContext *ctx);
    std::unique_ptr<ast::Expr> buildExp(SysYParser::ExpContext *ctx);
    std::unique_ptr<ast::Expr> buildCond(SysYParser::CondContext *ctx);
    std::unique_ptr<ast::Expr> buildLVal(SysYParser::LValContext *ctx);
    std::unique_ptr<ast::Expr> buildPrimaryExp(SysYParser::PrimaryExpContext *ctx);
    std::unique_ptr<ast::Expr> buildNumber(SysYParser::NumberContext *ctx);
    std::unique_ptr<ast::Expr> buildUnaryExp(SysYParser::UnaryExpContext *ctx);
    std::unique_ptr<ast::Expr> buildFuncRParam(SysYParser::FuncRParamContext *ctx);
    std::unique_ptr<ast::Expr> buildMulExp(SysYParser::MulExpContext *ctx);
    std::unique_ptr<ast::Expr> buildAddExp(SysYParser::AddExpContext *ctx);
    std::unique_ptr<ast::Expr> buildRelExp(SysYParser::RelExpContext *ctx);
    std::unique_ptr<ast::Expr> buildEqExp(SysYParser::EqExpContext *ctx);
    std::unique_ptr<ast::Expr> buildLAndExp(SysYParser::LAndExpContext *ctx);
    std::unique_ptr<ast::Expr> buildLOrExp(SysYParser::LOrExpContext *ctx);
    std::unique_ptr<ast::Expr> buildConstExp(SysYParser::ConstExpContext *ctx);
};
