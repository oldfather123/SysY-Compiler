
// Generated from SysY.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"
#include "../include/SysYVisitor.h"


/**
 * This class provides an empty implementation of SysYVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  SysYBaseVisitor : public SysYVisitor {
public:

  virtual std::any visitCompUnit(SysYParser::CompUnitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDecl(SysYParser::DeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBtype(SysYParser::BtypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDef(SysYParser::VarDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInitValue(SysYParser::InitValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncDecl(SysYParser::FuncDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncDef(SysYParser::FuncDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncType(SysYParser::FuncTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncFParams(SysYParser::FuncFParamsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncFParam(SysYParser::FuncFParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockStmt(SysYParser::BlockStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockItem(SysYParser::BlockItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmt(SysYParser::StmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignStmt(SysYParser::AssignStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpStmt(SysYParser::ExpStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfStmt(SysYParser::IfStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhileStmt(SysYParser::WhileStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBreakStmt(SysYParser::BreakStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitContinueStmt(SysYParser::ContinueStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturnStmt(SysYParser::ReturnStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEmptyStmt(SysYParser::EmptyStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelationExp(SysYParser::RelationExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarExp(SysYParser::VarExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMultiplicativeExp(SysYParser::MultiplicativeExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumberExp(SysYParser::NumberExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAndExp(SysYParser::AndExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExp(SysYParser::UnaryExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParenExp(SysYParser::ParenExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStringExp(SysYParser::StringExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOrExp(SysYParser::OrExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCallExp(SysYParser::CallExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAdditiveExp(SysYParser::AdditiveExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEqualExp(SysYParser::EqualExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCall(SysYParser::CallContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVar(SysYParser::VarContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLValue(SysYParser::LValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumber(SysYParser::NumberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitString(SysYParser::StringContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncRParams(SysYParser::FuncRParamsContext *ctx) override {
    return visitChildren(ctx);
  }


};

