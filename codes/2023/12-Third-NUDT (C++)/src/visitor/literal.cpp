#include "visitor.hpp"

using namespace IR;

std::any Visitor::visitNumber(SysYParser::NumberContext *ctx) {
  if (ctx->INTLITERAL()) return static_cast<Value *>(Constant::get(parseInt(ctx->INTLITERAL()->getText())));
  if (ctx->FLOATLITERAL())
    return static_cast<Value *>(Constant::get((float)atof(ctx->FLOATLITERAL()->getText().c_str())));
  throw InvalidType();
}

//? TODO Value*
std::any Visitor::visitString(SysYParser::StringContext *ctx) {
  return ctx->STRING()->getText();
}
