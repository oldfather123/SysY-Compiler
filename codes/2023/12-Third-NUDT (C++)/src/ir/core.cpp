#include "core.hpp"

using namespace IR;

using std::to_string;

using I32s = typename std::unordered_map<int32_t, unique_ptr<Constant>>;
using Floats = typename std::unordered_map<float, unique_ptr<Constant>>;

/// @brief Constant
Constant::Constant(int32_t val) : Value(Type::i32_t(), to_string(val)), _constant(val) {
}
Constant::Constant(float val) : Value(Type::float_t(), to_string(val)), _constant(val) {
}
Constant::Constant(int32_t val, Type *type) : Value(type, to_string(val)), _constant(val) {
}
Constant *Constant::getTrue() {
  static Constant true_c = Constant(1, Type::i1_t());
  return &true_c;
}
Constant *Constant::getFalse() {
  static Constant false_c = Constant(1, Type::i1_t());
  return &false_c;
}
Constant *Constant::get(int32_t val) {
  static I32s i32s;
  auto it = i32s.find(val);
  if (it != i32s.end()) return (it->second).get();
  Constant *constant = new Constant(val);
  assert(constant);
  auto res = i32s.emplace(val, constant);
  return res.first->second.get();
}
Constant *Constant::get(float val) {
  static Floats floats;
  auto it = floats.find(val);
  if (it != floats.end()) return (it->second).get();
  Constant *constant = new Constant(val);
  assert(constant);
  auto res = floats.emplace(val, constant);
  return res.first->second.get();
}

#define CONSTANT_CONVERT_OPERATOR_FUNCTION(type_t) \
  Constant::operator type_t() {                    \
    if (_type->isI32())                            \
      return static_cast<type_t>(*get<int32_t>()); \
    else if (_type->isFloat())                     \
      return static_cast<type_t>(*get<float>());   \
    throw InvalidType();                           \
  }

CONSTANT_CONVERT_OPERATOR_FUNCTION(bool)
CONSTANT_CONVERT_OPERATOR_FUNCTION(int32_t)
CONSTANT_CONVERT_OPERATOR_FUNCTION(float)

namespace IR {

#define CONSTANT_UNARY_OPERATOR_FUNCTION(op) \
  Constant *operator op(Constant &rhs) {     \
    if (auto c = rhs.get<int32_t>())         \
      return Constant::get(op(*c));          \
    else if (auto c = rhs.get<float>())      \
      return Constant::get(op(*c));          \
    throw InvalidType();                     \
  }

CONSTANT_UNARY_OPERATOR_FUNCTION(-)
CONSTANT_UNARY_OPERATOR_FUNCTION(!)

#define CONSTANT_BINARY_OPERATOR_FUNCTION(op)                               \
  Constant *operator op(Constant &lhs, Constant &rhs) {                     \
    auto li = lhs.get<int32_t>();                                           \
    auto lf = lhs.get<float>();                                             \
    auto ri = rhs.get<int32_t>();                                           \
    auto rf = rhs.get<float>();                                             \
    if (li && ri) return Constant::get((*li)op(*ri));                       \
    return Constant::get((li ? float(*li) : *lf)op(ri ? float(*ri) : *rf)); \
  }

CONSTANT_BINARY_OPERATOR_FUNCTION(+)
CONSTANT_BINARY_OPERATOR_FUNCTION(-)
CONSTANT_BINARY_OPERATOR_FUNCTION(*)
CONSTANT_BINARY_OPERATOR_FUNCTION(<)
CONSTANT_BINARY_OPERATOR_FUNCTION(<=)
CONSTANT_BINARY_OPERATOR_FUNCTION(>)
CONSTANT_BINARY_OPERATOR_FUNCTION(>=)
CONSTANT_BINARY_OPERATOR_FUNCTION(==)
CONSTANT_BINARY_OPERATOR_FUNCTION(!=)
CONSTANT_BINARY_OPERATOR_FUNCTION(&&)
CONSTANT_BINARY_OPERATOR_FUNCTION(||)

Constant *operator/(Constant &lhs, Constant &rhs) {
  auto li = lhs.get<int32_t>();
  auto lf = lhs.get<float>();
  auto ri = rhs.get<int32_t>();
  auto rf = rhs.get<float>();
  if (li && ri) return Constant::get(*li / *ri);
  return Constant::get((li ? float(*li) : *lf) / (ri ? float(*ri) : *rf));
  throw InvalidDivide();
}

Constant *operator%(Constant &lhs, Constant &rhs) {
  auto li = lhs.get<int32_t>();
  auto ri = rhs.get<int32_t>();
  if (li && ri && (*ri) != 0) return Constant::get(int32_t((*li) / (*ri)));
  throw InvalidRem();
}

}  // namespace IR

/// @brief Variable
Variable::Variable(bool is_compile_time_value, Type *type, const string &name, Value *addr, Value *init)
    : User(type, name), _is_compile_time_value(is_compile_time_value) {
  newOperand(addr);
  if (init) newOperand(init);
}
bool Variable::isCompileTimeValue() {
  return _is_compile_time_value;
}
Value *Variable::addr() {
  return operand(0);
}
Value *Variable::init() {
  return operand(1);
}

/// @brief Inst
Instruction::Instruction(IType itype, Type *type, const string &name) : User(type, name), _itype(itype) {
}
string_view Instruction::itypeName() {
  return ITypeName[_itype];
}
IType Instruction::itype() {
  return _itype;
}
bool Instruction::isTerminator() {
  switch (_itype) {
    case RET:
    case JMP:
    case BR:
      return true;
    default:
      return false;
  }
}
bool Instruction::isUnary() {
  switch (_itype) {
    case FNEG:
    case TRUNC:
    case ZEXT:
    case SEXT:
    case FPTRUNC:
    case FPEXT:
    case FPTOSI:
    case SITOFP:
    case PTRTOINT:
    case INTTOPTR:
      return true;
    default:
      return false;
  }
}
bool Instruction::isBinary() {
  switch (_itype) {
    case ADD:
    case FADD:
    case SUB:
    case FSUB:
    case MUL:
    case FMUL:
    case SDIV:
    case FDIV:
    case SREM:
    case IEQ:
    case INE:
    case ISGT:
    case ISGE:
    case ISLT:
    case ISLE:
    case FOEQ:
    case FONE:
    case FOGT:
    case FOGE:
    case FOLT:
    case FOLE:
      return true;
    default:
      return false;
  }
}
bool Instruction::isBitwise() {
  switch (_itype) {
    case SHL:
    case LSHR:
    case ASHR:
    case AND:
    case OR:
    case XOR:
      return true;
    default:
      return false;
  }
}
bool Instruction::isMemory() {
  switch (_itype) {
    case ALLOCA:
    case LOAD:
    case STORE:
    case GETELEMENTPTR:
      return true;
    default:
      return false;
  }
}
bool Instruction::isConversion() {
  switch (_itype) {
    case TRUNC:
    case ZEXT:
    case SEXT:
    case FPTRUNC:
    case FPEXT:
    case FPTOSI:
    case SITOFP:
    case PTRTOINT:
    case INTTOPTR:
      return true;
    default:
      return false;
  }
}
bool Instruction::isCompare() {
  switch (_itype) {
    case IEQ:
    case INE:
    case ISGT:
    case ISGE:
    case ISLT:
    case ISLE:
    case FOEQ:
    case FONE:
    case FOGT:
    case FOGE:
    case FOLT:
    case FOLE:
      return true;
    default:
      return false;
  }
}
bool Instruction::isOther() {
  switch (_itype) {
    case PHI:
    case CALL:
      return true;
    default:
      return false;
  }
}

/// @brief Block
BasicBlock::BasicBlock(Function *function, const string &name)
    : Value(Type::label_t(), name), _function(function), _sdom(nullptr), _idom(nullptr) {
}
Function *BasicBlock::function() {
  return _function;
}
Instructions &BasicBlock::instructions() {
  return _instructions;
}
vector<BasicBlock *> &BasicBlock::preds() {
  return _preds;
}
vector<BasicBlock *> &BasicBlock::succs() {
  return _succs;
}
size_t BasicBlock::dfn() {
  return _dfn;
}
BasicBlock *BasicBlock::sdom() {
  return _sdom;
}
BasicBlock *BasicBlock::idom() {
  return _idom;
}

/// @brief Param
Param::Param(Type *type, const string &name) : Value(type, name) {
}

/// @brief Function
Function::Function() : Value(Type::function_t(Type::void_t(), {}), ""), _names(0) {
  _basic_blocks.emplace_back(new BasicBlock(this, newName("%")));
}
Function::Function(Type *function_type, const string &name) : Value(function_type, name), _names(0) {
  _basic_blocks.emplace_back(new BasicBlock(this, newName("%")));
}
Type *Function::retType() {
  return _type->retType();
}
vector<Type *> &Function::paramTypes() {
  return _type->paramTypes();
}
Params &Function::params() {
  return _params;
}
Param *Function::newParam(Param *param) {
  _params.emplace_back(param);
  return _params.back().get();
}
BasicBlocks &Function::basicBlocks() {
  return _basic_blocks;
}
BasicBlock *Function::basicBlock(size_t index) {
  auto it = _basic_blocks.begin();
  while (index--) it++;
  return it->get();
}
BasicBlock *Function::newBasicBlock() {
  _basic_blocks.emplace_back(new BasicBlock(this, newName("%")));
  return _basic_blocks.back().get();
}
void Function::delBasicBlock(BasicBlock *block) {
  _basic_blocks.remove_if([&](unique_ptr<BasicBlock> &_block) -> bool {
    return block == _block.get();
  });
}
string Function::newName(const string &prefix) {
  return prefix + to_string(_names++);
}

/// @brief Comp
CompileUnit::CompileUnit() : _curr_function(&_init_function), _curr_variable_table(&_global_table){};
Function *CompileUnit::initFunction() {
  return &_init_function;
}
FunctionTable *CompileUnit::functionTable() {
  return &_function_table;
}
VariableTable *CompileUnit::globalVariableTable() {
  return &_global_table;
}
Scope *CompileUnit::scope() {
  return &_scope;
}
Function *CompileUnit::currFunction() {
  return _curr_function;
}
VariableTable *CompileUnit::currVariableTable() {
  return _curr_variable_table;
}
bool CompileUnit::isGlobalScope() {
  return _scope.size() == 0;
}
void CompileUnit::inScope() {
  _scope.emplace_back(new VariableTable());
  _curr_variable_table = _scope.back().get();
}
void CompileUnit::outScope() {
  _scope.pop_back();
  if (_scope.size())
    _curr_variable_table = _scope.back().get();
  else {
    _curr_function = &_init_function;
    _curr_variable_table = &_global_table;
  }
}
Function *CompileUnit::function(const string &name) {
  auto t = _function_table.find(name);
  if (t != _function_table.end()) return t->second.get();
  return nullptr;
}
Variable *CompileUnit::rvariable(const string &name) {
  for (auto table = _scope.rbegin(); table != _scope.rend(); table++) {
    auto t = (*table)->find(name);
    if (t != (*table)->end()) return t->second.get();
  }
  auto t = _global_table.find(name);
  if (t != _global_table.end()) return t->second.get();
  return nullptr;
}
Variable *CompileUnit::variable(const string &name) {
  auto t = _curr_variable_table->find(name);
  if (t != _curr_variable_table->end()) return t->second.get();
  return nullptr;
}
Function *CompileUnit::newFunction(Type *type, const string &name) {
  if (function(name)) return nullptr;
  auto res = _function_table.emplace(name, new Function(type, name));
  return (_curr_function = (res.first->second).get());
}
Variable *CompileUnit::newVariable(bool is_compile_time_value, const string &name, Value *addr, Value *init) {
  if (_curr_variable_table->find(name) != _curr_variable_table->end()) return nullptr;
  auto res = _curr_variable_table->emplace(
      name, new Variable(is_compile_time_value, addr->type()->deref(1), name, addr, init));
  return (res.first->second).get();
}
