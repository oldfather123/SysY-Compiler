#ifndef __IRCORE_HPP__
#define __IRCORE_HPP__

#include "use.hpp"

namespace IR {

using std::variant;

class Constant;
class Instruction;
class BasicBlock;
class Param;
class Function;
class Variable;
class CompileUnit;

using Instructions = typename std::list<unique_ptr<Instruction>>;
using BasicBlocks = typename std::list<unique_ptr<BasicBlock>>;
using Params = typename std::list<unique_ptr<Param>>;
using FunctionTable = typename std::map<string, unique_ptr<Function>>;
using VariableTable = typename std::map<string, unique_ptr<Variable>>;
using Scope = typename std::list<unique_ptr<VariableTable>>;

class Constant : public Value {  // CompileTimeValue
protected:
  variant<int32_t, float> _constant;

public:
  Constant(int32_t val);
  Constant(float val);
  Constant(int32_t val, Type *type);
  static Constant *getTrue();
  static Constant *getFalse();
  static Constant *get(int32_t val);
  static Constant *get(float val);
  template <typename T>
  T *get() {
    return std::get_if<T>(&_constant);
  }
  operator bool();
  operator int32_t();
  operator float();
  friend Constant *operator-(Constant &rhs);
  friend Constant *operator!(Constant &rhs);
  friend Constant *operator+(Constant &lhs, Constant &rhs);
  friend Constant *operator-(Constant &lhs, Constant &rhs);
  friend Constant *operator*(Constant &lhs, Constant &rhs);
  friend Constant *operator/(Constant &lhs, Constant &rhs);
  friend Constant *operator%(Constant &lhs, Constant &rhs);
  friend Constant *operator<(Constant &lhs, Constant &rhs);
  friend Constant *operator<=(Constant &lhs, Constant &rhs);
  friend Constant *operator>(Constant &lhs, Constant &rhs);
  friend Constant *operator>=(Constant &lhs, Constant &rhs);
  friend Constant *operator==(Constant &lhs, Constant &rhs);
  friend Constant *operator!=(Constant &lhs, Constant &rhs);
  friend Constant *operator&&(Constant &lhs, Constant &rhs);
  friend Constant *operator||(Constant &lhs, Constant &rhs);
};

class Variable : public User {
protected:
  bool _is_compile_time_value;

public:
  Variable(bool is_constant, Type *type, const string &name, Value *addr, Value *init);
  bool isCompileTimeValue();
  Value *addr();
  Value *init();
};

class Instruction : public User {
protected:
  IType _itype;

public:
  Instruction(IType itype, Type *type, const string &name);
  string_view itypeName();
  IType itype();
  bool isTerminator();
  bool isUnary();
  bool isBinary();
  bool isBitwise();
  bool isMemory();
  bool isConversion();
  bool isCompare();
  bool isOther();
};

class BasicBlock : public Value {
protected:
  Function *_function;
  Instructions _instructions;
  vector<BasicBlock *> _preds;
  vector<BasicBlock *> _succs;
  size_t _dfn;
  BasicBlock *_sdom;
  BasicBlock *_idom;

public:
  BasicBlock(Function *function, const string &name);
  Function *function();
  Instructions &instructions();
  vector<BasicBlock *> &preds();
  vector<BasicBlock *> &succs();
  size_t dfn();
  BasicBlock *sdom();
  BasicBlock *idom();
};

class Param : public Value {
public:
  Param(Type *type, const string &name);
};

class Function : public Value {
protected:
  size_t _names;
  Params _params;
  BasicBlocks _basic_blocks;

public:
  Function();  // .init function
  Function(Type *type, const string &name);
  Type *retType();
  vector<Type *> &paramTypes();
  Params &params();
  Param *newParam(Param *param);
  BasicBlocks &basicBlocks();
  BasicBlock *basicBlock(size_t index);
  BasicBlock *newBasicBlock();
  void delBasicBlock(BasicBlock *block);
  void resetNames() {
    _names = 0;
  }
  string newName(const string &prefix = "");
};

class CompileUnit {
protected:
  Function _init_function;
  FunctionTable _function_table;
  VariableTable _global_table;
  Scope _scope;
  Function *_curr_function;
  VariableTable *_curr_variable_table;

public:
  CompileUnit();
  Function *initFunction();
  FunctionTable *functionTable();
  VariableTable *globalVariableTable();
  Scope *scope();
  Function *currFunction();
  VariableTable *currVariableTable();
  bool isGlobalScope();
  void inScope();
  void outScope();
  Function *function(const string &name);
  Variable *rvariable(const string &name);
  Variable *variable(const string &name);
  Function *newFunction(Type *type, const string &name);
  Variable *newVariable(bool is_constant, const string &name, Value *addr, Value *init);
};

}  // namespace IR

#endif
