#ifndef PARSER_SYMBOLTABLE_H
#define PARSER_SYMBOLTABLE_H

#include "constant.h"
#include "global_variable.h"
#include "instructions.h"
#include "function.h"
#include "number.h"
#include "value.h"
#include <cassert>
#include <list>
#include <map>
#include <unordered_map>

class SymbolTable {
private:
  std::list<std::unordered_map<std::string, ir::Value *>> _table;

  ir::Value *getItem(const std::string &name) const;

  static std::map<int, ir::Number>
  submap(const std::map<int, ir::Number> &values, int fromKey, int toKey);

  static std::unique_ptr<ir::Constant>
  fuseConst(std::unique_ptr<ir::Type> type,
            const std::map<int, ir::Number> &values, int base);

public:
  size_t size() const { return _table.size(); }

  void out() { _table.pop_front(); }

  void in() { _table.emplace_front(); }

  ir::Value *getData(const std::string &name) const { return getItem(name); }

  bool isGlobal() const { return _table.size() == 1; }

  ir::Function *getFunction(const std::string &name) const;

  std::unique_ptr<ir::Function> makeFunction(std::unique_ptr<ir::Type> type,
                                             const std::string &name);

  std::unique_ptr<ir::GlobalVariable> makeGlobal(bool isConst,
                                                 std::unique_ptr<ir::Type> type,
                                                 const std::string &name,
                                                 ir::Number value);

  std::unique_ptr<ir::GlobalVariable>
  makeGlobal(bool isConst, std::unique_ptr<ir::Type> type,
             const std::string &name, std::map<int, ir::Number> &values);

  std::unique_ptr<ir::AllocaInst> makeLocal(std::unique_ptr<ir::Type> type,
                                            const std::string &name);

  std::unique_ptr<ir::AllocaInst> makeLocal(std::unique_ptr<ir::Type> type,
                                            const std::string &name,
                                            const std::vector<int> &dimensions);

  std::unique_ptr<ir::Argument> makeArgument(std::unique_ptr<ir::Type> type,
                                             const std::string &name);
};

#endif
