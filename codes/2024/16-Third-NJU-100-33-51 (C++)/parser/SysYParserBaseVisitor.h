
// Generated from ./parser/SysYParser.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysYParserVisitor.h"


/**
 * This class provides an empty implementation of SysYParserVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  SysYParserBaseVisitor : public SysYParserVisitor {
public:

  virtual std::any visitProgram(SysYParserParser::ProgramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCompUnit(SysYParserParser::CompUnitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDecl(SysYParserParser::DeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstDecl(SysYParserParser::ConstDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBType(SysYParserParser::BTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstDefSingle(SysYParserParser::ConstDefSingleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstDefArray(SysYParserParser::ConstDefArrayContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstInitValSingle(SysYParserParser::ConstInitValSingleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstInitValArray(SysYParserParser::ConstInitValArrayContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDecl(SysYParserParser::VarDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDefSingle(SysYParserParser::VarDefSingleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDefArray(SysYParserParser::VarDefArrayContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDefSingleInitVal(SysYParserParser::VarDefSingleInitValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDefArrayInitVal(SysYParserParser::VarDefArrayInitValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInitLVal(SysYParserParser::InitLValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInitValSingle(SysYParserParser::InitValSingleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInitValArray(SysYParserParser::InitValArrayContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncDef(SysYParserParser::FuncDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncType(SysYParserParser::FuncTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncFParams(SysYParserParser::FuncFParamsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncFParamSingle(SysYParserParser::FuncFParamSingleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncFParamArray(SysYParserParser::FuncFParamArrayContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlock(SysYParserParser::BlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockItem(SysYParserParser::BlockItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmtAssign(SysYParserParser::StmtAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmtExp(SysYParserParser::StmtExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmtBlock(SysYParserParser::StmtBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmtCond(SysYParserParser::StmtCondContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmtWhile(SysYParserParser::StmtWhileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmtBreak(SysYParserParser::StmtBreakContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmtContinue(SysYParserParser::StmtContinueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmtReturn(SysYParserParser::StmtReturnContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExp(SysYParserParser::ExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCond(SysYParserParser::CondContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLValSingle(SysYParserParser::LValSingleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLValArray(SysYParserParser::LValArrayContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExpParen(SysYParserParser::PrimaryExpParenContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExpLVal(SysYParserParser::PrimaryExpLValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExpNumber(SysYParserParser::PrimaryExpNumberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumber(SysYParserParser::NumberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExpPrimaryExp(SysYParserParser::UnaryExpPrimaryExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExpFuncR(SysYParserParser::UnaryExpFuncRContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExpUnary(SysYParserParser::UnaryExpUnaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryOp(SysYParserParser::UnaryOpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncRParams(SysYParserParser::FuncRParamsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncRParam(SysYParserParser::FuncRParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMulExp(SysYParserParser::MulExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAddExp(SysYParserParser::AddExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelExp(SysYParserParser::RelExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEqExp(SysYParserParser::EqExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLAndExp(SysYParserParser::LAndExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLOrExp(SysYParserParser::LOrExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstExp(SysYParserParser::ConstExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIntDecConst(SysYParserParser::IntDecConstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIntOctConst(SysYParserParser::IntOctConstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIntHexConst(SysYParserParser::IntHexConstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFloatConst(SysYParserParser::FloatConstContext *ctx) override {
    return visitChildren(ctx);
  }


};

