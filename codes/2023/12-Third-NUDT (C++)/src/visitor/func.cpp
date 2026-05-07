#include "visitor.hpp"

using namespace IR;

static vector<Type *> param_types;
static vector<string> param_names;

std::any Visitor::visitFuncDef(SysYParser::FuncDefContext *ctx) {
  if (!cu()->isGlobalScope()) throw InvalidFuncDef();
  param_types.clear();
  param_names.clear();
  if (ctx->funcFParams()) visit(ctx->funcFParams());
  Function *new_f = cu()->newFunction(Type::function_t(std::any_cast<Type *>(visit(ctx->returnType())), param_types),
                                      ctx->IDENTIFIER()->getText());
  BasicBlock *new_bb = new_f->basicBlock(0);
  Instructions::iterator new_ptr = new_bb->instructions().end();
  builder()->saveState();
  builder()->setState(cu(), new_f, new_bb, new_ptr);
  cu()->inScope();
  for (size_t i = 0; i < param_names.size(); i++) {
    Value *param = builder()->f()->newParam(new Param(param_types[i], builder()->newName()));
    Value *addr = builder()->newAllocaInst(builder()->newName(), param_types[i]);
    builder()->newStoreInst(param, addr);
    cu()->newVariable(false, param_names[i], addr, param);
  }
  visit(ctx->blockStmt());
  cu()->outScope();
  builder()->loadState();
  return nullptr;
}

std::any Visitor::visitFuncFParams(SysYParser::FuncFParamsContext *ctx) {
  for (const auto &param : ctx->funcFParam()) visit(param);
  return nullptr;
}

std::any Visitor::visitFuncFParam(SysYParser::FuncFParamContext *ctx) {
  param_names.emplace_back(ctx->IDENTIFIER()->getText());
  Type *param_type = std::any_cast<Type *>(visit(ctx->bType()));
  if (!ctx->LBRACKET(0)) {
    param_types.emplace_back(param_type);
    return nullptr;
  }
  if (!ctx->exp().size()) {
    param_types.emplace_back(Type::pointer_t(param_type));
    return nullptr;
  }
  for (size_t i = ctx->exp().size(); i; i--) {
    Value *dim = std::any_cast<Value *>(visit(ctx->exp(i - 1)));
    Constant *c = dynamic_cast<Constant *>(dim);
    if (!c || !c->type()->isI32() || *(c->get<int>()) < 0) throw InvalidDim();
    param_type = Type::array_t(param_type, *(c->get<int>()));
  }
  param_types.emplace_back(Type::pointer_t(param_type));
  return nullptr;
}
