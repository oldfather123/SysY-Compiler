#include "global.hpp"
#include "visitor.hpp"

using namespace IR;

std::any Visitor::visitComp(SysYParser::CompContext *ctx) {
  for (const auto &function_interface : builtin_functions)
    cu()->newFunction(function_interface.type, function_interface.name);
  visitChildren(ctx);
  if (!cu()->function("main")) throw NoMainFunc();
  Function *main_function = cu()->function("main");
  if (main_function->type()->isnot(Type::function_t(Type::i32_t(), {}))) throw InvalidMainFuncType();
  return cu();
}
