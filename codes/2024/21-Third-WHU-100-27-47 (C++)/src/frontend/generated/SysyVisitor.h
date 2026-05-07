
// Generated from Sysy.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysyParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by SysyParser.
 */
class  SysyVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SysyParser.
   */
    virtual std::any visitCompUnit(SysyParser::CompUnitContext *context) = 0;

    virtual std::any visitDecl(SysyParser::DeclContext *context) = 0;

    virtual std::any visitConstDecl(SysyParser::ConstDeclContext *context) = 0;

    virtual std::any visitBType(SysyParser::BTypeContext *context) = 0;

    virtual std::any visitConstDef(SysyParser::ConstDefContext *context) = 0;

    virtual std::any visitSingleConstInitVal(SysyParser::SingleConstInitValContext *context) = 0;

    virtual std::any visitMultiConstInitVal(SysyParser::MultiConstInitValContext *context) = 0;

    virtual std::any visitVarDecl(SysyParser::VarDeclContext *context) = 0;

    virtual std::any visitNoinitVarDef(SysyParser::NoinitVarDefContext *context) = 0;

    virtual std::any visitInitVarDef(SysyParser::InitVarDefContext *context) = 0;

    virtual std::any visitSingleInitVal(SysyParser::SingleInitValContext *context) = 0;

    virtual std::any visitMultiInitVal(SysyParser::MultiInitValContext *context) = 0;

    virtual std::any visitFuncDef(SysyParser::FuncDefContext *context) = 0;

    virtual std::any visitFuncType(SysyParser::FuncTypeContext *context) = 0;

    virtual std::any visitFuncFParams(SysyParser::FuncFParamsContext *context) = 0;

    virtual std::any visitFuncFParam(SysyParser::FuncFParamContext *context) = 0;

    virtual std::any visitBlock(SysyParser::BlockContext *context) = 0;

    virtual std::any visitBlockItem(SysyParser::BlockItemContext *context) = 0;

    virtual std::any visitAssignStmt(SysyParser::AssignStmtContext *context) = 0;

    virtual std::any visitExpStmt(SysyParser::ExpStmtContext *context) = 0;

    virtual std::any visitBlockStmt(SysyParser::BlockStmtContext *context) = 0;

    virtual std::any visitIfStmt(SysyParser::IfStmtContext *context) = 0;

    virtual std::any visitIfElseStmt(SysyParser::IfElseStmtContext *context) = 0;

    virtual std::any visitWhileStmt(SysyParser::WhileStmtContext *context) = 0;

    virtual std::any visitBreakStmt(SysyParser::BreakStmtContext *context) = 0;

    virtual std::any visitContinueStmt(SysyParser::ContinueStmtContext *context) = 0;

    virtual std::any visitReturnStmt(SysyParser::ReturnStmtContext *context) = 0;

    virtual std::any visitExp(SysyParser::ExpContext *context) = 0;

    virtual std::any visitCond(SysyParser::CondContext *context) = 0;

    virtual std::any visitLVal(SysyParser::LValContext *context) = 0;

    virtual std::any visitParensPrimaryExp(SysyParser::ParensPrimaryExpContext *context) = 0;

    virtual std::any visitLValPrimaryExp(SysyParser::LValPrimaryExpContext *context) = 0;

    virtual std::any visitNumberPrimaryExp(SysyParser::NumberPrimaryExpContext *context) = 0;

    virtual std::any visitNumber(SysyParser::NumberContext *context) = 0;

    virtual std::any visitPrimaryUnaryExp(SysyParser::PrimaryUnaryExpContext *context) = 0;

    virtual std::any visitFuncCallUnaryExp(SysyParser::FuncCallUnaryExpContext *context) = 0;

    virtual std::any visitUnaryOpUnaryExp(SysyParser::UnaryOpUnaryExpContext *context) = 0;

    virtual std::any visitUnaryOp(SysyParser::UnaryOpContext *context) = 0;

    virtual std::any visitFuncRParams(SysyParser::FuncRParamsContext *context) = 0;

    virtual std::any visitMulMulExp(SysyParser::MulMulExpContext *context) = 0;

    virtual std::any visitUnaryMulExp(SysyParser::UnaryMulExpContext *context) = 0;

    virtual std::any visitAddAddExp(SysyParser::AddAddExpContext *context) = 0;

    virtual std::any visitMulAddExp(SysyParser::MulAddExpContext *context) = 0;

    virtual std::any visitRelRelExp(SysyParser::RelRelExpContext *context) = 0;

    virtual std::any visitAddRelExp(SysyParser::AddRelExpContext *context) = 0;

    virtual std::any visitEqEqExp(SysyParser::EqEqExpContext *context) = 0;

    virtual std::any visitRelEqExp(SysyParser::RelEqExpContext *context) = 0;

    virtual std::any visitEqLAndExp(SysyParser::EqLAndExpContext *context) = 0;

    virtual std::any visitLAndLAndExp(SysyParser::LAndLAndExpContext *context) = 0;

    virtual std::any visitLAndLOrExp(SysyParser::LAndLOrExpContext *context) = 0;

    virtual std::any visitLOrLOrExp(SysyParser::LOrLOrExpContext *context) = 0;

    virtual std::any visitConstExp(SysyParser::ConstExpContext *context) = 0;

    virtual std::any visitEpsilon(SysyParser::EpsilonContext *context) = 0;


};

