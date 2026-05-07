#include "visitor.hpp"

using namespace IR;

std::any Visitor::visitLValue(SysYParser::LValueContext *ctx) {
  string name = ctx->IDENTIFIER()->getText();
  if (!cu()->rvariable(name)) throw UnknownVar();
  Variable *variable = cu()->rvariable(name);
  if (variable->isCompileTimeValue() && variable->addr()->type()->refSize() == 1) return variable->init();
  Value *addr = variable->addr();
  for (const auto &exp : ctx->exp()) {
    Value *index = std::any_cast<Value *>(visit(exp));
    addr = builder()->newGetelementptrInst(builder()->newName(), addr, {Constant::get(0), index});
  }
  return addr;
}
