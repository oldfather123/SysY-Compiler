#include "visitor.hpp"

using namespace IR;

std::any Visitor::visitCall(SysYParser::CallContext *ctx) {
  Function *callee = cu()->function(ctx->IDENTIFIER()->getText());
  vector<Value *> params;
  if (ctx->funcRParams()) params = std::any_cast<vector<Value *>>(visit(ctx->funcRParams()));
  if (!callee->paramTypes().size() && params.size()) throw InvalidRealParam();
  if (!callee->paramTypes().size() && !params.size())
    return static_cast<Value *>(builder()->newCallInst(builder()->newName(), callee, params));
  for (size_t i = 0; i < callee->paramTypes().size() - (callee->paramTypes().back()->isDynamic()); i++) {
    Type *f_type = callee->paramTypes()[i];
    Type *r_type = params[i]->type();
    if (f_type == r_type) continue;
    if ((f_type->isPointer() && r_type->isArray()) && (f_type->deref(1) == r_type->deref(1))) continue;
    if ((f_type->in({Type::i32_t(), Type::float_t()})) && (r_type->in({Type::i32_t(), Type::float_t()}))) {
      if (f_type->isI32()) params[i] = builder()->newFptosiInst(builder()->newName(), params[i], Type::i32_t());
      if (f_type->isFloat()) params[i] = builder()->newSitofpInst(builder()->newName(), params[i], Type::float_t());
      continue;
    }
    throw InvalidRealParam();
  }
  return static_cast<Value *>(builder()->newCallInst(builder()->newName(), callee, params));
}

std::any Visitor::visitFuncRParams(SysYParser::FuncRParamsContext *ctx) {
  vector<Value *> params;
  for (const auto &exp : ctx->exp()) params.emplace_back(std::any_cast<Value *>(visit(exp)));
  return params;
}
