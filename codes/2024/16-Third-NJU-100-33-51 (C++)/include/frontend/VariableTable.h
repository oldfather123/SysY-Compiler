//
// Created by yanayn on 7/9/24.
//

#ifndef ANTPIE_VARIABLETABLE_H
#define ANTPIE_VARIABLETABLE_H
#define String std::string
#define any antlrcpp::Any
#include <map>
#include <string>

#include "Value.hh"

typedef struct variableTable {
  std::map<String, Value*> table;
  vector<string> FParams;
  struct variableTable* parent;
  variableTable(struct variableTable* prt) {
    parent = prt;
    table = std::map<String, Value*>();
  }

 public:
  Value* get(String tar);
  void put(String name, Value* tar);

} VariableTable;

#endif  // ANTPIE_VARIABLETABLE_H
