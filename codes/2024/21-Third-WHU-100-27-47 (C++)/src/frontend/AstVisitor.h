#pragma once

#include "Common.h"
#include "IR.h"
#include "SysyBaseVisitor.h"
#include "SysyParser.h"
#include "Ast.h"

class AstVisitor : public SysyBaseVisitor {
    // Add your class members and methods here

public:
    [[nodiscard]] Ptr<ast::CompUnit> compileUnit(); // return the compiled unit

    virtual antlrcpp::Any visitCompUnit(SysyParser::CompUnitContext *ctx) override;

    virtual antlrcpp::Any visitDecl(SysyParser::DeclContext *ctx) override;

    virtual antlrcpp::Any visitConstDecl(SysyParser::ConstDeclContext *ctx) override;

    virtual antlrcpp::Any visitBType(SysyParser::BTypeContext *ctx) override;

    virtual antlrcpp::Any visitUnaryOpUnaryExp(SysyParser::UnaryOpUnaryExpContext *ctx) override;

    // virtual antlrcpp::Any visitConstDef(SysyParser::ConstDefContext *ctx) override;

    virtual antlrcpp::Any visitSingleConstInitVal(SysyParser::SingleConstInitValContext *ctx) override;

    virtual antlrcpp::Any visitMultiConstInitVal(SysyParser::MultiConstInitValContext *ctx) override;

    // virtual antlrcpp::Any visitItemConstInitVal(SysyParser::ItemConstInitValContext *ctx) override;

    // virtual antlrcpp::Any visitListConstInitVal(SysyParser::ListConstInitValContext *ctx) override;

    virtual antlrcpp::Any visitVarDecl(SysyParser::VarDeclContext *ctx) override;

    virtual antlrcpp::Any visitSingleInitVal(SysyParser::SingleInitValContext *ctx) override;

    virtual antlrcpp::Any visitMultiInitVal(SysyParser::MultiInitValContext *ctx) override;

    // virtual antlrcpp::Any visitVarDefWithOutInitVal(SysyParser::VarDefWithOutInitValContext *ctx) override;

    // virtual antlrcpp::Any visitVarDefWithInitVal(SysyParser::VarDefWithInitValContext *ctx) override;

    // virtual antlrcpp::Any visitItemInitVal(SysyParser::ItemInitValContext *ctx) override;

    // virtual antlrcpp::Any visitListInitVal(SysyParser::ListInitValContext *ctx) override;

    virtual antlrcpp::Any visitFuncDef(SysyParser::FuncDefContext *ctx) override;

    virtual antlrcpp::Any visitFuncType(SysyParser::FuncTypeContext *ctx) override;

    virtual antlrcpp::Any visitFuncFParams(SysyParser::FuncFParamsContext *ctx) override;

    virtual antlrcpp::Any visitFuncFParam(SysyParser::FuncFParamContext *ctx) override;

    virtual antlrcpp::Any visitFuncCallUnaryExp(SysyParser::FuncCallUnaryExpContext *ctx) override;

    virtual antlrcpp::Any visitBlock(SysyParser::BlockContext *ctx) override;

    virtual antlrcpp::Any visitBlockItem(SysyParser::BlockItemContext *ctx) override;

    virtual antlrcpp::Any visitAssignStmt(SysyParser::AssignStmtContext *ctx) override;

    virtual antlrcpp::Any visitExpStmt(SysyParser::ExpStmtContext *ctx) override;

    virtual antlrcpp::Any visitBlockStmt(SysyParser::BlockStmtContext *ctx) override;

    virtual antlrcpp::Any visitIfStmt(SysyParser::IfStmtContext *ctx) override;

    virtual antlrcpp::Any visitIfElseStmt(SysyParser::IfElseStmtContext *ctx) override;

    virtual antlrcpp::Any visitWhileStmt(SysyParser::WhileStmtContext *ctx) override;

    virtual antlrcpp::Any visitBreakStmt(SysyParser::BreakStmtContext *ctx) override;

    virtual antlrcpp::Any visitContinueStmt(SysyParser::ContinueStmtContext *ctx) override;

    virtual antlrcpp::Any visitReturnStmt(SysyParser::ReturnStmtContext *ctx) override;

    virtual antlrcpp::Any visitExp(SysyParser::ExpContext *ctx) override;

    virtual antlrcpp::Any visitCond(SysyParser::CondContext *ctx) override;

    virtual antlrcpp::Any visitLVal(SysyParser::LValContext *ctx) override;

    // virtual antlrcpp::Any visitExpPrimaryExp(SysyParser::ExpPrimaryExpContext *ctx) override;

    virtual antlrcpp::Any visitLValPrimaryExp(SysyParser::LValPrimaryExpContext *ctx) override;

    virtual antlrcpp::Any visitNumberPrimaryExp(SysyParser::NumberPrimaryExpContext *ctx) override;

    virtual antlrcpp::Any visitNumber(SysyParser::NumberContext *ctx) override;

    virtual antlrcpp::Any visitPrimaryUnaryExp(SysyParser::PrimaryUnaryExpContext *ctx) override;

    // virtual antlrcpp::Any visitFuncRParamsUnaryExp(SysyParser::FuncRParamsUnaryExpContext *ctx) override;

    // virtual antlrcpp::Any visitUnaryUnaryExp(SysyParser::UnaryUnaryExpContext *ctx) override;

    virtual antlrcpp::Any visitUnaryOp(SysyParser::UnaryOpContext *ctx) override;

    virtual antlrcpp::Any visitFuncRParams(SysyParser::FuncRParamsContext *ctx) override;

    virtual antlrcpp::Any visitMulMulExp(SysyParser::MulMulExpContext *ctx) override;

    virtual antlrcpp::Any visitUnaryMulExp(SysyParser::UnaryMulExpContext *ctx) override;

    virtual antlrcpp::Any visitAddAddExp(SysyParser::AddAddExpContext *ctx) override;

    virtual antlrcpp::Any visitMulAddExp(SysyParser::MulAddExpContext *ctx) override;

    virtual antlrcpp::Any visitRelRelExp(SysyParser::RelRelExpContext *ctx) override;

    virtual antlrcpp::Any visitAddRelExp(SysyParser::AddRelExpContext *ctx) override;

    virtual antlrcpp::Any visitEqEqExp(SysyParser::EqEqExpContext *ctx) override;

    virtual antlrcpp::Any visitRelEqExp(SysyParser::RelEqExpContext *ctx) override;

    virtual antlrcpp::Any visitEqLAndExp(SysyParser::EqLAndExpContext *ctx) override;

    virtual antlrcpp::Any visitLAndLAndExp(SysyParser::LAndLAndExpContext *ctx) override;

    virtual antlrcpp::Any visitLOrLOrExp(SysyParser::LOrLOrExpContext *ctx) override;

    // virtual antlrcpp::Any visitLOrLAndExp(SysyParser::LOrLAndExpContext *ctx) override;

    virtual antlrcpp::Any visitLAndLOrExp(SysyParser::LAndLOrExpContext *ctx) override;

    virtual antlrcpp::Any visitConstExp(SysyParser::ConstExpContext *ctx) override;

    virtual antlrcpp::Any visitParensPrimaryExp(SysyParser::ParensPrimaryExpContext *ctx) override;

    // 符号表相关函数
    virtual int handleFunctionDef(Ptr<ast::Function> func);

    AstVisitor() { }

    // error函数
    void error(std::string str) {
        printf("%s\n", str.c_str());
    }

private:
    Ptr<ast::CompUnit> m_comp_unit;
};
