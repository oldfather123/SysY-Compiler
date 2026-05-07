#include "visitor.hpp"

using namespace IR;

std::any Visitor::visitBType(SysYParser::BTypeContext *ctx) {
  if (ctx->INT())
    return Type::i32_t();
  else if (ctx->FLOAT())
    return Type::float_t();
  throw InvalidType();
}

std::any Visitor::visitReturnType(SysYParser::ReturnTypeContext *ctx) {
  if (ctx->VOID())
    return Type::void_t();
  else if (ctx->INT())
    return Type::i32_t();
  else if (ctx->FLOAT())
    return Type::float_t();
  throw InvalidType();
}
