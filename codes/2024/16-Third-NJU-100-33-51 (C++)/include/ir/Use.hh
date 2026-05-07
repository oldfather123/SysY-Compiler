/**
 * Use: record the user of value
 * Create by Zhang Junbin at 2024/6/1
 */

#ifndef _USE_H_
#define _USE_H_

class Value;
class Instruction;

struct Use {
  Value* value;
  Instruction* instr;
  Use* next;
  Use* pre;

  Use(Instruction* instr_, Value* value_)
      : value(value_), instr(instr_), next(0), pre(0) {}
  void replaceValue(Value* value);

  void removeFromValue();
};

#endif