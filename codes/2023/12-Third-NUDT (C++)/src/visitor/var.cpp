#include "util.hpp"
#include "visitor.hpp"

using namespace IR;

static bool is_compile_time_value;
static Type *basic_type;
static Type *variable_type;
static vector<Value *> inits;
static Type *init_type;

void Visitor::dfsStoreAllocaInit(Value *addr, size_t idx) {
  if (addr->type()->deref(1)->in({Type::i32_t(), Type::float_t()})) {
    if (inits[idx]->type() != addr->type()->deref(1)) {
      if (inits[idx]->type()->isI32())
        inits[idx] = builder()->newSitofpInst(builder()->newName(), inits[idx], Type::float_t());
      else if (inits[idx]->type()->isFloat())
        inits[idx] = builder()->newFptosiInst(builder()->newName(), inits[idx], Type::i32_t());
    }
    builder()->newStoreInst(inits[idx], addr);
    return;
  }
  for (size_t i = 0; i < addr->type()->deref(1)->dim(); i++)
    dfsStoreAllocaInit(
        builder()->newGetelementptrInst(builder()->newName(), addr, {Constant::get(0), Constant::get((int32_t)i)}),
        idx + i * addr->type()->deref(2)->size());
}

std::any Visitor::visitDecl(SysYParser::DeclContext *ctx) {
  is_compile_time_value = ctx->CONST();
  basic_type = std::any_cast<Type *>(visit(ctx->bType()));
  for (const auto &var_def : ctx->varDef()) {
    string name = var_def->lValue()->IDENTIFIER()->getText();
    if (cu()->variable(name) || (cu()->isGlobalScope() && cu()->function(name))) throw InvalidVarDef();
    visit(var_def);
    inits.clear();
    init_type = Type::pointer_t(variable_type);
    if (var_def->ASSIGN()) visit(var_def->init());
    if (cu()->isGlobalScope())
      for (size_t i = 0; i < variable_type->size(); i++)
        inits.emplace_back(basic_type->isI32() ? Constant::get(0) : Constant::get(0.f));
    if (is_compile_time_value && !inits.size()) throw InvalidInit();
    Value *addr = nullptr;
    if (cu()->isGlobalScope())
      addr = builder()->newGlobalInst("@" + name, variable_type, inits);
    else {
      addr = builder()->newAllocaInst(builder()->newName(), variable_type);
      if (inits.size()) dfsStoreAllocaInit(addr, 0);
    }
    cu()->newVariable(is_compile_time_value, name, addr, inits.size() ? inits[0] : nullptr);
  }
  return nullptr;
}

std::any Visitor::visitVarDef(SysYParser::VarDefContext *ctx) {
  variable_type = basic_type;
  for (size_t i = ctx->lValue()->exp().size(); i; i--) {
    Constant *dim = static_cast<Constant *>(std::any_cast<Value *>(visit(ctx->lValue()->exp(i - 1))));
    if (!dim->type()->isI32() || *(dim->get<int32_t>()) < 0) throw InvalidDim();
    variable_type = Type::array_t(variable_type, *(dim->get<int32_t>()));
  }
  return nullptr;
}

std::any Visitor::visitScalarInit(SysYParser::ScalarInitContext *ctx) {
  Value *val = static_cast<Constant *>(std::any_cast<Value *>(visit(ctx->exp())));
  Constant *c = dynamic_cast<Constant *>(val);
  if ((is_compile_time_value || cu()->isGlobalScope()) && !c) throw InvalidInit();
  inits.emplace_back(val);
  return nullptr;
}

std::any Visitor::visitArrayInit(SysYParser::ArrayInitContext *ctx) {
  Type *prev_init_type = init_type;
  do init_type = init_type->deref(1);
  while (init_type->isArray() && inits.size() % init_type->size());
  if (!init_type->isArray()) throw InvalidInit();
  size_t init_size_max = inits.size() + init_type->size();
  for (const auto &init : ctx->init()) visit(init);
  if (inits.size() > init_size_max) throw InvalidInit();
  while (inits.size() < init_size_max) inits.emplace_back(basic_type->isI32() ? Constant::get(0) : Constant::get(.0f));
  init_type = prev_init_type;
  return nullptr;
}
