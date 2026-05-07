#ifndef __VISITOR_HPP__
#define __VISITOR_HPP__

#include "SysYBaseVisitor.h"
#include "global.hpp"
#include "ir.hpp"
#include "util.hpp"

using namespace IR;
using namespace Util;

namespace IR {

class Visitor;

class Visitor : public SysYBaseVisitor {
private:
  CompileUnit _cu;
  Builder _builder;

public:
  Visitor();

  CompileUnit *cu();

  Builder *builder();

  void dfsStoreAllocaInit(Value *addr, size_t idx);

  virtual std::any visitComp(SysYParser::CompContext *ctx) override;

  virtual std::any visitDecl(SysYParser::DeclContext *ctx) override;

  virtual std::any visitVarDef(SysYParser::VarDefContext *ctx) override;

  virtual std::any visitScalarInit(SysYParser::ScalarInitContext *ctx) override;

  virtual std::any visitArrayInit(SysYParser::ArrayInitContext *ctx) override;

  virtual std::any visitFuncDef(SysYParser::FuncDefContext *ctx) override;

  virtual std::any visitFuncFParams(SysYParser::FuncFParamsContext *ctx) override;

  virtual std::any visitFuncFParam(SysYParser::FuncFParamContext *ctx) override;

  virtual std::any visitStmt(SysYParser::StmtContext *ctx) override;

  virtual std::any visitAssignStmt(SysYParser::AssignStmtContext *ctx) override;

  virtual std::any visitExpStmt(SysYParser::ExpStmtContext *ctx) override;

  virtual std::any visitIfStmt(SysYParser::IfStmtContext *ctx) override;

  virtual std::any visitWhileStmt(SysYParser::WhileStmtContext *ctx) override;

  virtual std::any visitBreakStmt(SysYParser::BreakStmtContext *ctx) override;

  virtual std::any visitContinueStmt(SysYParser::ContinueStmtContext *ctx) override;

  virtual std::any visitReturnStmt(SysYParser::ReturnStmtContext *ctx) override;

  virtual std::any visitBlockStmt(SysYParser::BlockStmtContext *ctx) override;

  virtual std::any visitBlockItem(SysYParser::BlockItemContext *ctx) override;

  virtual std::any visitEmptyStmt(SysYParser::EmptyStmtContext *ctx) override;

  virtual std::any visitParenExp(SysYParser::ParenExpContext *ctx) override;

  virtual std::any visitLValueExp(SysYParser::LValueExpContext *ctx) override;

  virtual std::any visitNumberExp(SysYParser::NumberExpContext *ctx) override;

  virtual std::any visitStringExp(SysYParser::StringExpContext *ctx) override;

  virtual std::any visitCallExp(SysYParser::CallExpContext *ctx) override;

  virtual std::any visitUnaryExp(SysYParser::UnaryExpContext *ctx) override;

  virtual std::any visitMulExp(SysYParser::MulExpContext *ctx) override;

  virtual std::any visitAddExp(SysYParser::AddExpContext *ctx) override;

  virtual std::any visitRelExp(SysYParser::RelExpContext *ctx) override;

  virtual std::any visitEqExp(SysYParser::EqExpContext *ctx) override;

  virtual std::any visitAndExp(SysYParser::AndExpContext *ctx) override;

  virtual std::any visitOrExp(SysYParser::OrExpContext *ctx) override;

  virtual std::any visitCall(SysYParser::CallContext *ctx) override;

  virtual std::any visitFuncRParams(SysYParser::FuncRParamsContext *ctx) override;

  virtual std::any visitLValue(SysYParser::LValueContext *ctx) override;

  virtual std::any visitNumber(SysYParser::NumberContext *ctx) override;

  virtual std::any visitString(SysYParser::StringContext *ctx) override;

  virtual std::any visitReturnType(SysYParser::ReturnTypeContext *ctx) override;

  virtual std::any visitBType(SysYParser::BTypeContext *ctx) override;
};

}  // namespace IR

#endif
