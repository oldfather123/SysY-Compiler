//
// Created by yanayn on 7/9/24.
//

#include "VariableTable.h"

Value* VariableTable::get(String tar) {
  Value* target = table[tar];
  if (target == nullptr) {
    if (parent != nullptr) {
      return parent->get(tar);
    } else {
      return nullptr;
    }
  } else {
    return target;
  }
}

void VariableTable::put(String name, Value* tar) { table[name] = tar; }