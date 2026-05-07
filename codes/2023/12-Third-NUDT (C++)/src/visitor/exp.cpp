#include "visitor.hpp"

using namespace IR;

BasicBlock *iftrue = nullptr;
BasicBlock *iffalse = nullptr;

std::any Visitor::visitParenExp(SysYParser::ParenExpContext *ctx) {
  return visit(ctx->exp());
}

std::any Visitor::visitLValueExp(SysYParser::LValueExpContext *ctx) {
  Value *val = std::any_cast<Value *>(visit(ctx->lValue()));
  return dynamic_cast<Constant *>(val) ? val : builder()->newLoadInst(builder()->newName(), val);
}

std::any Visitor::visitNumberExp(SysYParser::NumberExpContext *ctx) {
  return visit(ctx->number());
}

std::any Visitor::visitStringExp(SysYParser::StringExpContext *ctx) {
  return visit(ctx->string());
}

std::any Visitor::visitCallExp(SysYParser::CallExpContext *ctx) {
  return visit(ctx->call());
}

std::any Visitor::visitUnaryExp(SysYParser::UnaryExpContext *ctx) {
  BasicBlock *prev_iftrue = iftrue;
  BasicBlock *prev_iffalse = iffalse;
  if (ctx->NOT() && iftrue && iffalse) {
    iftrue = prev_iffalse;
    iffalse = prev_iftrue;
    if (Value *cond = std::any_cast<Value *>(visit(ctx->exp()))) {
      if (cond->type()->isI32()) cond = builder()->newIneInst(builder()->newName(), cond, Constant::get(0));
      if (cond->type()->isFloat()) cond = builder()->newFoneInst(builder()->newName(), cond, Constant::get(0.f));
      builder()->newBrInst(cond, iftrue, iffalse);
    }
    iftrue = prev_iftrue;
    iffalse = prev_iffalse;
    return static_cast<Value *>(nullptr);
  }
  iftrue = nullptr;
  iffalse = nullptr;
  Value *rhs = std::any_cast<Value *>(visit(ctx->exp()));
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  Constant *c = dynamic_cast<Constant *>(rhs);
  if (c) {
    if (ctx->ADD()) return static_cast<Value *>(c);
    if (ctx->SUB()) return static_cast<Value *>(-*c);
    if (ctx->NOT()) return static_cast<Value *>(!*c);
    throw InvalidOperator();
  }
  if (rhs->type()->isI1()) {
    if (ctx->NOT()) {
      rhs = builder()->newIeqInst(builder()->newName(), rhs, Constant::getFalse());
      return static_cast<Value *>(builder()->newZextInst(builder()->newName(), rhs, Type::i32_t()));
    }
    rhs = builder()->newZextInst(builder()->newName(), rhs, Type::i32_t());
  }
  if (rhs->type()->isI32()) {
    if (ctx->ADD()) return rhs;
    if (ctx->SUB()) return static_cast<Value *>(builder()->newSubInst(builder()->newName(), Constant::get(0), rhs));
    if (ctx->NOT()) return static_cast<Value *>(builder()->newIeqInst(builder()->newName(), rhs, Constant::get(0)));
  }
  if (ctx->ADD()) return rhs;
  if (ctx->SUB()) return static_cast<Value *>(builder()->newFnegInst(builder()->newName(), rhs));
  if (ctx->NOT()) return static_cast<Value *>(builder()->newFoeqInst(builder()->newName(), rhs, Constant::get(0.f)));
  throw InvalidOperator();
}

std::any Visitor::visitMulExp(SysYParser::MulExpContext *ctx) {
  BasicBlock *prev_iftrue = iftrue;
  BasicBlock *prev_iffalse = iffalse;
  iftrue = nullptr;
  iffalse = nullptr;
  Value *lhs = std::any_cast<Value *>(visit(ctx->exp(0)));
  Value *rhs = std::any_cast<Value *>(visit(ctx->exp(1)));
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  Constant *l = dynamic_cast<Constant *>(lhs);
  Constant *r = dynamic_cast<Constant *>(rhs);
  if (l && r) {
    if (ctx->MUL()) return static_cast<Value *>((*l) * (*r));
    if (ctx->DIV()) return static_cast<Value *>((*l) / (*r));
    if (ctx->MOD()) return static_cast<Value *>((*l) % (*r));
    throw InvalidOperator();
  }
  if (lhs->type()->isI1()) lhs = builder()->newZextInst(builder()->newName(), lhs, Type::i32_t());
  if (rhs->type()->isI1()) rhs = builder()->newZextInst(builder()->newName(), rhs, Type::i32_t());
  if (lhs->type()->isI32() && rhs->type()->isI32()) {
    if (ctx->MUL()) return static_cast<Value *>(builder()->newMulInst(builder()->newName(), lhs, rhs));
    if (ctx->DIV()) return static_cast<Value *>(builder()->newSdivInst(builder()->newName(), lhs, rhs));
    if (ctx->MOD()) return static_cast<Value *>(builder()->newSremInst(builder()->newName(), lhs, rhs));
    throw InvalidOperator();
  }
  if (lhs->type()->isI32()) lhs = builder()->newSitofpInst(builder()->newName(), lhs, Type::float_t());
  if (rhs->type()->isI32()) rhs = builder()->newSitofpInst(builder()->newName(), rhs, Type::float_t());
  if (ctx->MUL()) return static_cast<Value *>(builder()->newFmulInst(builder()->newName(), lhs, rhs));
  if (ctx->DIV()) return static_cast<Value *>(builder()->newFdivInst(builder()->newName(), lhs, rhs));
  throw InvalidOperator();
}

std::any Visitor::visitAddExp(SysYParser::AddExpContext *ctx) {
  BasicBlock *prev_iftrue = iftrue;
  BasicBlock *prev_iffalse = iffalse;
  iftrue = nullptr;
  iffalse = nullptr;
  Value *lhs = std::any_cast<Value *>(visit(ctx->exp(0)));
  Value *rhs = std::any_cast<Value *>(visit(ctx->exp(1)));
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  Constant *l = dynamic_cast<Constant *>(lhs);
  Constant *r = dynamic_cast<Constant *>(rhs);
  if (l && r) {
    if (ctx->ADD()) return static_cast<Value *>((*l) + (*r));
    if (ctx->SUB()) return static_cast<Value *>((*l) - (*r));
    throw InvalidOperator();
  }
  if (lhs->type()->isI1()) lhs = builder()->newZextInst(builder()->newName(), lhs, Type::i32_t());
  if (rhs->type()->isI1()) rhs = builder()->newZextInst(builder()->newName(), rhs, Type::i32_t());
  if (lhs->type()->isI32() && rhs->type()->isI32()) {
    if (ctx->ADD()) return static_cast<Value *>(builder()->newAddInst(builder()->newName(), lhs, rhs));
    if (ctx->SUB()) return static_cast<Value *>(builder()->newSubInst(builder()->newName(), lhs, rhs));
    throw InvalidOperator();
  }
  if (lhs->type()->isI32()) lhs = builder()->newSitofpInst(builder()->newName(), lhs, Type::float_t());
  if (rhs->type()->isI32()) rhs = builder()->newSitofpInst(builder()->newName(), rhs, Type::float_t());
  if (ctx->ADD()) return static_cast<Value *>(builder()->newFaddInst(builder()->newName(), lhs, rhs));
  if (ctx->SUB()) return static_cast<Value *>(builder()->newFsubInst(builder()->newName(), lhs, rhs));
  throw InvalidOperator();
}

std::any Visitor::visitRelExp(SysYParser::RelExpContext *ctx) {
  BasicBlock *prev_iftrue = iftrue;
  BasicBlock *prev_iffalse = iffalse;
  iftrue = nullptr;
  iffalse = nullptr;
  Value *lhs = std::any_cast<Value *>(visit(ctx->exp(0)));
  Value *rhs = std::any_cast<Value *>(visit(ctx->exp(1)));
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  Constant *l = dynamic_cast<Constant *>(lhs);
  Constant *r = dynamic_cast<Constant *>(rhs);
  if (l && r) {
    if (ctx->LT()) return static_cast<Value *>((*l) < (*r));
    if (ctx->LE()) return static_cast<Value *>((*l) <= (*r));
    if (ctx->GT()) return static_cast<Value *>((*l) > (*r));
    if (ctx->GE()) return static_cast<Value *>((*l) >= (*r));
    throw InvalidOperator();
  }
  if (lhs->type()->isI1()) lhs = builder()->newZextInst(builder()->newName(), lhs, Type::i32_t());
  if (rhs->type()->isI1()) rhs = builder()->newZextInst(builder()->newName(), rhs, Type::i32_t());
  if (lhs->type()->isI32() && rhs->type()->isI32()) {
    if (ctx->LT()) return static_cast<Value *>(builder()->newIsltInst(builder()->newName(), lhs, rhs));
    if (ctx->LE()) return static_cast<Value *>(builder()->newIsleInst(builder()->newName(), lhs, rhs));
    if (ctx->GT()) return static_cast<Value *>(builder()->newIsgtInst(builder()->newName(), lhs, rhs));
    if (ctx->GE()) return static_cast<Value *>(builder()->newIsgeInst(builder()->newName(), lhs, rhs));
    throw InvalidOperator();
  }
  if (lhs->type()->isI32()) lhs = builder()->newSitofpInst(builder()->newName(), lhs, Type::float_t());
  if (rhs->type()->isI32()) rhs = builder()->newSitofpInst(builder()->newName(), rhs, Type::float_t());
  if (ctx->LT()) return static_cast<Value *>(builder()->newFoltInst(builder()->newName(), lhs, rhs));
  if (ctx->LE()) return static_cast<Value *>(builder()->newFoleInst(builder()->newName(), lhs, rhs));
  if (ctx->GT()) return static_cast<Value *>(builder()->newFogtInst(builder()->newName(), lhs, rhs));
  if (ctx->GE()) return static_cast<Value *>(builder()->newFogeInst(builder()->newName(), lhs, rhs));
  throw InvalidOperator();
}

std::any Visitor::visitEqExp(SysYParser::EqExpContext *ctx) {
  BasicBlock *prev_iftrue = iftrue;
  BasicBlock *prev_iffalse = iffalse;
  iftrue = nullptr;
  iffalse = nullptr;
  Value *lhs = std::any_cast<Value *>(visit(ctx->exp(0)));
  Value *rhs = std::any_cast<Value *>(visit(ctx->exp(1)));
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  Constant *l = dynamic_cast<Constant *>(lhs);
  Constant *r = dynamic_cast<Constant *>(rhs);
  if (l && r) {
    if (ctx->EQ()) return static_cast<Value *>((*l) == (*r));
    if (ctx->NE()) return static_cast<Value *>((*l) != (*r));
    throw InvalidOperator();
  }
  if (lhs->type()->isI1()) lhs = builder()->newZextInst(builder()->newName(), lhs, Type::i32_t());
  if (rhs->type()->isI1()) rhs = builder()->newZextInst(builder()->newName(), rhs, Type::i32_t());
  if (lhs->type()->isI32() && rhs->type()->isI32()) {
    if (ctx->EQ()) return static_cast<Value *>(builder()->newIeqInst(builder()->newName(), lhs, rhs));
    if (ctx->NE()) return static_cast<Value *>(builder()->newIneInst(builder()->newName(), lhs, rhs));
    throw InvalidOperator();
  }
  if (lhs->type()->isI32()) lhs = builder()->newSitofpInst(builder()->newName(), lhs, Type::float_t());
  if (rhs->type()->isI32()) rhs = builder()->newSitofpInst(builder()->newName(), rhs, Type::float_t());
  if (ctx->EQ()) return static_cast<Value *>(builder()->newFoeqInst(builder()->newName(), lhs, rhs));
  if (ctx->NE()) return static_cast<Value *>(builder()->newFoneInst(builder()->newName(), lhs, rhs));
  throw InvalidOperator();
}

std::any Visitor::visitAndExp(SysYParser::AndExpContext *ctx) {
  BasicBlock *prev_iftrue = iftrue;
  BasicBlock *prev_iffalse = iffalse;
  if (iftrue && iffalse) {
    BasicBlock *next = builder()->f()->newBasicBlock();
    iftrue = next;
    iffalse = prev_iffalse;
    if (Value *cond = std::any_cast<Value *>(visit(ctx->exp(0)))) {
      if (cond->type()->isI32()) cond = builder()->newIneInst(builder()->newName(), cond, Constant::get(0));
      if (cond->type()->isFloat()) cond = builder()->newFoneInst(builder()->newName(), cond, Constant::get(0.f));
      builder()->newBrInst(cond, iftrue, iffalse);
    }
    builder()->setState(cu(), builder()->f(), next, next->instructions().end());
    iftrue = prev_iftrue;
    iffalse = prev_iffalse;
    if (Value *cond = std::any_cast<Value *>(visit(ctx->exp(1)))) {
      if (cond->type()->isI32()) cond = builder()->newIneInst(builder()->newName(), cond, Constant::get(0));
      if (cond->type()->isFloat()) cond = builder()->newFoneInst(builder()->newName(), cond, Constant::get(0.f));
      builder()->newBrInst(cond, iftrue, iffalse);
    }
    iftrue = prev_iftrue;
    iffalse = prev_iffalse;
    return static_cast<Value *>(nullptr);
  }
  BasicBlock *curr = builder()->bb();
  BasicBlock *next = builder()->f()->newBasicBlock();
  BasicBlock *merge = builder()->f()->newBasicBlock();
  iftrue = nullptr;
  iffalse = nullptr;
  Value *lhs = std::any_cast<Value *>(visit(ctx->exp(0)));
  if (lhs->type()->isI32()) lhs = builder()->newIneInst(builder()->newName(), lhs, Constant::get(0));
  if (lhs->type()->isFloat()) lhs = builder()->newFoneInst(builder()->newName(), lhs, Constant::get(0.f));
  builder()->newBrInst(lhs, next, merge);
  builder()->setState(cu(), builder()->f(), next, next->instructions().end());
  Value *rhs = std::any_cast<Value *>(visit(ctx->exp(1)));
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  if (rhs->type()->isI32()) rhs = builder()->newIneInst(builder()->newName(), rhs, Constant::get(0));
  if (rhs->type()->isFloat()) rhs = builder()->newFoneInst(builder()->newName(), rhs, Constant::get(0.f));
  builder()->newJmpInst(merge);
  builder()->setState(cu(), builder()->f(), merge, next->instructions().end());
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  return static_cast<Value *>(
      builder()->newPhiInst(Type::i1_t(), builder()->newName(), {Constant::getFalse(), rhs}, {curr, next}));
}

std::any Visitor::visitOrExp(SysYParser::OrExpContext *ctx) {
  BasicBlock *prev_iftrue = iftrue;
  BasicBlock *prev_iffalse = iffalse;
  if (iftrue && iffalse) {
    BasicBlock *next = builder()->f()->newBasicBlock();
    iftrue = prev_iftrue;
    iffalse = next;
    if (Value *cond = std::any_cast<Value *>(visit(ctx->exp(0)))) {
      if (cond->type()->isI32()) cond = builder()->newIneInst(builder()->newName(), cond, Constant::get(0));
      if (cond->type()->isFloat()) cond = builder()->newFoneInst(builder()->newName(), cond, Constant::get(0.f));
      builder()->newBrInst(cond, iftrue, iffalse);
    }
    builder()->setState(cu(), builder()->f(), next, next->instructions().end());
    iftrue = prev_iftrue;
    iffalse = prev_iffalse;
    if (Value *cond = std::any_cast<Value *>(visit(ctx->exp(1)))) {
      if (cond->type()->isI32()) cond = builder()->newIneInst(builder()->newName(), cond, Constant::get(0));
      if (cond->type()->isFloat()) cond = builder()->newFoneInst(builder()->newName(), cond, Constant::get(0.f));
      builder()->newBrInst(cond, iftrue, iffalse);
    }
    iftrue = prev_iftrue;
    iffalse = prev_iffalse;
    return static_cast<Value *>(nullptr);
  }
  BasicBlock *curr = builder()->bb();
  BasicBlock *next = builder()->f()->newBasicBlock();
  BasicBlock *merge = builder()->f()->newBasicBlock();
  iftrue = nullptr;
  iffalse = nullptr;
  Value *lhs = std::any_cast<Value *>(visit(ctx->exp(0)));
  if (lhs->type()->isI32()) lhs = builder()->newIneInst(builder()->newName(), lhs, Constant::get(0));
  if (lhs->type()->isFloat()) lhs = builder()->newFoneInst(builder()->newName(), lhs, Constant::get(0.f));
  builder()->newBrInst(lhs, merge, next);
  builder()->setState(cu(), builder()->f(), next, next->instructions().end());
  Value *rhs = std::any_cast<Value *>(visit(ctx->exp(1)));
  iftrue = prev_iftrue;
  iffalse = prev_iffalse;
  if (rhs->type()->isI32()) rhs = builder()->newIneInst(builder()->newName(), rhs, Constant::get(0));
  if (rhs->type()->isFloat()) rhs = builder()->newFoneInst(builder()->newName(), rhs, Constant::get(0.f));
  builder()->newJmpInst(merge);
  builder()->setState(cu(), builder()->f(), merge, next->instructions().end());
  return static_cast<Value *>(
      builder()->newPhiInst(Type::i1_t(), builder()->newName(), {Constant::getTrue(), rhs}, {curr, next}));
}
