
// Generated from /home/jpy794/actions-runner/_work/sysy-compiler/sysy-compiler/src/ast/grammer/sysy.g4 by ANTLR 4.13.0

#pragma once


#include "antlr4-runtime.h"
#include "sysyVisitor.h"


/**
 * This class provides an empty implementation of sysyVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  sysyBaseVisitor : public sysyVisitor {
public:

  virtual std::any visitCompUnit(sysyParser::CompUnitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGlobalDef(sysyParser::GlobalDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVardecl(sysyParser::VardeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVardef(sysyParser::VardefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarInit(sysyParser::VarInitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncdef(sysyParser::FuncdefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncparam(sysyParser::FuncparamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlock(sysyParser::BlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpStmt(sysyParser::ExpStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStmt(sysyParser::StmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLval(sysyParser::LvalContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFuncCall(sysyParser::FuncCallContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParenExp(sysyParser::ParenExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExp(sysyParser::ExpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumber(sysyParser::NumberContext *ctx) override {
    return visitChildren(ctx);
  }


};

