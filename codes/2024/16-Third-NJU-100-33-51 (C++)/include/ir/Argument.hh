/**
 * Argument ir
 * Create by Zhang Junbin at 2024/6/2
 */

#ifndef _ARGUMENT_H_
#define _ARGUMENT_H_

#include "Value.hh"

class Argument : public Value {
 public:
  Argument(string name, Type* t) : Value(t, name, VT_ARG) {}
  string toString() const;
};

#endif