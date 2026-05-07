
// Generated from Sysy.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "SysyVisitor.h"


/**
 * This class provides an empty implementation of SysyVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  SysyBaseVisitor : public SysyVisitor {
public:

  virtual std::any visitCompUnit(SysyParser::CompUnitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDecl(SysyParser::DeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstDecl(SysyParser::ConstDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBType(SysyParser::BTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstDef(SysyParser::ConstDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSingleConstInitVal(SysyParser::SingleConstInitValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMultiConstInitVal(SysyParser::MultiConstInitValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDecl(SysyParser::VarDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNoinitVarDef(SysyParser::NoinitVarDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInitVarDef(SysyParser::InitVarDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSingleInitVal(SysyParser::SingleInitValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMultiInitVal(SysyParser::MultiInitValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncDef(SysyParser::FuncDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncType(SysyParser::FuncTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncFParams(SysyParser::FuncFParamsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncFParam(SysyParser::FuncFParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlock(SysyParser::BlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockItem(SysyParser::BlockItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignStmt(SysyParser::AssignStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpStmt(SysyParser::ExpStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockStmt(SysyParser::BlockStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfStmt(SysyParser::IfStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfElseStmt(SysyParser::IfElseStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhileStmt(SysyParser::WhileStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBreakStmt(SysyParser::BreakStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitContinueStmt(SysyParser::ContinueStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturnStmt(SysyParser::ReturnStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExp(SysyParser::ExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCond(SysyParser::CondContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLVal(SysyParser::LValContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParensPrimaryExp(SysyParser::ParensPrimaryExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLValPrimaryExp(SysyParser::LValPrimaryExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumberPrimaryExp(SysyParser::NumberPrimaryExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumber(SysyParser::NumberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryUnaryExp(SysyParser::PrimaryUnaryExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncCallUnaryExp(SysyParser::FuncCallUnaryExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryOpUnaryExp(SysyParser::UnaryOpUnaryExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryOp(SysyParser::UnaryOpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncRParams(SysyParser::FuncRParamsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMulMulExp(SysyParser::MulMulExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryMulExp(SysyParser::UnaryMulExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAddAddExp(SysyParser::AddAddExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMulAddExp(SysyParser::MulAddExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelRelExp(SysyParser::RelRelExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAddRelExp(SysyParser::AddRelExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEqEqExp(SysyParser::EqEqExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelEqExp(SysyParser::RelEqExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEqLAndExp(SysyParser::EqLAndExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLAndLAndExp(SysyParser::LAndLAndExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLAndLOrExp(SysyParser::LAndLOrExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLOrLOrExp(SysyParser::LOrLOrExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstExp(SysyParser::ConstExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEpsilon(SysyParser::EpsilonContext *ctx) override {
    return visitChildren(ctx);
  }


};

