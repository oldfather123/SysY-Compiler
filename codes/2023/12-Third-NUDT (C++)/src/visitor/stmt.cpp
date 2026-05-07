#include "visitor.hpp"

using namespace IR;

extern BasicBlock *iftrue;
extern BasicBlock *iffalse;

BasicBlock *continue_bb;
BasicBlock *break_bb;

std::any Visitor::visitStmt(SysYParser::StmtContext *ctx) {
  return visitChildren(ctx);
}

std::any Visitor::visitAssignStmt(SysYParser::AssignStmtContext *ctx) {
  Variable *var = cu()->rvariable(ctx->lValue()->IDENTIFIER()->getText());
  Value *val = std::any_cast<Value *>(visit(ctx->exp()));
  Value *addr = std::any_cast<Value *>(visit(ctx->lValue()));
  if (var->isCompileTimeValue()) throw InvalidAssign();
  assert(val->type()->in({Type::i32_t(), Type::float_t()}));
  assert(addr->type()->deref(1)->in({Type::i32_t(), Type::float_t()}));
  if (val->type() != addr->type()->deref(1)) {
    if (val->type()->isI32())
      val = builder()->newSitofpInst(builder()->newName(), val, Type::float_t());
    else if (val->type()->isFloat())
      val = builder()->newFptosiInst(builder()->newName(), val, Type::i32_t());
  }
  builder()->newStoreInst(val, addr);
  return val;
}

std::any Visitor::visitExpStmt(SysYParser::ExpStmtContext *ctx) {
  return visit(ctx->exp());
}

std::any Visitor::visitIfStmt(SysYParser::IfStmtContext *ctx) {
  BasicBlock *prev_iftrue = iftrue;
  BasicBlock *prev_iffalse = iffalse;
  iftrue = builder()->f()->newBasicBlock();
  iffalse = builder()->f()->newBasicBlock();
  BasicBlock *merge = builder()->f()->newBasicBlock();
  if (Value *cond = std::any_cast<Value *>(visit(ctx->exp()))) {
    if (cond->type()->isI32()) cond = builder()->newIneInst(builder()->newName(), cond, Constant::get(0));
    if (cond->type()->isFloat()) cond = builder()->newFoneInst(builder()->newName(), cond, Constant::get(0.f));
    builder()->newBrInst(cond, iftrue, iffalse);
  }
  builder()->setState(cu(), builder()->f(), iftrue, iftrue->instructions().end());
  visit(ctx->stmt(0));
  if (!builder()->bb()->instructions().size() || !builder()->bb()->instructions().back()->isTerminator())
    builder()->newJmpInst(merge);
  builder()->setState(cu(), builder()->f(), iffalse, iffalse->instructions().end());
  if (ctx->stmt(1)) visit(ctx->stmt(1));
  if (!builder()->bb()->instructions().size() || !builder()->bb()->instructions().back()->isTerminator())
    builder()->newJmpInst(merge);
  builder()->setState(cu(), builder()->f(), merge, merge->instructions().end());
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  return nullptr;
}

std::any Visitor::visitWhileStmt(SysYParser::WhileStmtContext *ctx) {
  BasicBlock *prev_iftrue = iftrue;
  BasicBlock *prev_iffalse = iffalse;
  BasicBlock *prev_continue_bb = continue_bb;
  BasicBlock *prev_break_bb = break_bb;
  BasicBlock *condition = builder()->f()->newBasicBlock();
  iftrue = builder()->f()->newBasicBlock();
  BasicBlock *loop = builder()->f()->newBasicBlock();
  iffalse = builder()->f()->newBasicBlock();
  continue_bb = condition;
  break_bb = iffalse;
  builder()->newJmpInst(condition);
  builder()->setState(cu(), builder()->f(), condition, condition->instructions().end());
  if (Value *cond = std::any_cast<Value *>(visit(ctx->exp()))) {
    if (cond->type()->isI32()) cond = builder()->newIneInst(builder()->newName(), cond, Constant::get(0));
    if (cond->type()->isFloat()) cond = builder()->newFoneInst(builder()->newName(), cond, Constant::get(0.f));
    builder()->newBrInst(cond, iftrue, iffalse);
  }
  builder()->setState(cu(), builder()->f(), iftrue, iftrue->instructions().end());
  visit(ctx->stmt());
  if (!builder()->bb()->instructions().size() || !builder()->bb()->instructions().back()->isTerminator())
    builder()->newJmpInst(loop);
  builder()->setState(cu(), builder()->f(), loop, loop->instructions().end());
  builder()->newJmpInst(condition);
  builder()->setState(cu(), builder()->f(), iffalse, iffalse->instructions().end());
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  continue_bb = prev_continue_bb;
  break_bb = prev_break_bb;
  return nullptr;
}

std::any Visitor::visitBreakStmt(SysYParser::BreakStmtContext *ctx) {
  builder()->newJmpInst(break_bb);
  return nullptr;
}

std::any Visitor::visitContinueStmt(SysYParser::ContinueStmtContext *ctx) {
  builder()->newJmpInst(continue_bb);
  return nullptr;
}

static Value void_value(Type::void_t(), "");

std::any Visitor::visitReturnStmt(SysYParser::ReturnStmtContext *ctx) {
  builder()->newRetInst(ctx->exp() ? std::any_cast<Value *>(visit(ctx->exp())) : &void_value);
  return nullptr;
}

std::any Visitor::visitBlockStmt(SysYParser::BlockStmtContext *ctx) {
  cu()->inScope();
  visitChildren(ctx);
  cu()->outScope();
  return nullptr;
}

std::any Visitor::visitBlockItem(SysYParser::BlockItemContext *ctx) {
  visitChildren(ctx);
  return nullptr;
}

std::any Visitor::visitEmptyStmt(SysYParser::EmptyStmtContext *ctx) {
  return nullptr;
}
