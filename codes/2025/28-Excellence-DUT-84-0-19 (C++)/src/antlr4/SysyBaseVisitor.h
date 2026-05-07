
// Generated from Sysy.g4 by ANTLR 4.13.2

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

  virtual std::any visitCompUnitItem(SysyParser::CompUnitItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDecl(SysyParser::DeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstDecl(SysyParser::ConstDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInt(SysyParser::IntContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFloat(SysyParser::FloatContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstDef(SysyParser::ConstDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDecl(SysyParser::VarDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDef(SysyParser::VarDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInit(SysyParser::InitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInitList(SysyParser::InitListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncDef(SysyParser::FuncDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncType_(SysyParser::FuncType_Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVoid(SysyParser::VoidContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncFParams(SysyParser::FuncFParamsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitScalarParam(SysyParser::ScalarParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArrayParam(SysyParser::ArrayParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlock(SysyParser::BlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockItem(SysyParser::BlockItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssign(SysyParser::AssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprStmt(SysyParser::ExprStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockStmt(SysyParser::BlockStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfElse(SysyParser::IfElseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhile(SysyParser::WhileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBreak(SysyParser::BreakContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitContinue(SysyParser::ContinueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturn(SysyParser::ReturnContext *ctx) override {
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

  virtual std::any visitPrimaryExp_(SysyParser::PrimaryExp_Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLValExpr(SysyParser::LValExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDecIntConst(SysyParser::DecIntConstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOctIntConst(SysyParser::OctIntConstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHexIntConst(SysyParser::HexIntConstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDecFloatConst(SysyParser::DecFloatConstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHexFloatConst(SysyParser::HexFloatConstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumber(SysyParser::NumberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExp_(SysyParser::UnaryExp_Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCall(SysyParser::CallContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryAdd(SysyParser::UnaryAddContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnarySub(SysyParser::UnarySubContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNot(SysyParser::NotContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStringConst(SysyParser::StringConstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncRParam(SysyParser::FuncRParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncRParams(SysyParser::FuncRParamsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDiv(SysyParser::DivContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMod(SysyParser::ModContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMul(SysyParser::MulContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMulExp_(SysyParser::MulExp_Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAddExp_(SysyParser::AddExp_Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAdd(SysyParser::AddContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSub(SysyParser::SubContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGeq(SysyParser::GeqContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLt(SysyParser::LtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelExp_(SysyParser::RelExp_Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLeq(SysyParser::LeqContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGt(SysyParser::GtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNeq(SysyParser::NeqContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEq(SysyParser::EqContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEqExp_(SysyParser::EqExp_Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLAndExp_(SysyParser::LAndExp_Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnd(SysyParser::AndContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOr(SysyParser::OrContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLOrExp_(SysyParser::LOrExp_Context *ctx) override {
    return visitChildren(ctx);
  }


};

