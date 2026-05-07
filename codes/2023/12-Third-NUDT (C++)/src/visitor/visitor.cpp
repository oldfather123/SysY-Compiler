#include "visitor.hpp"

using namespace IR;

Visitor::Visitor() {
  _builder.initialize(&_cu);
}

CompileUnit *Visitor::cu() {
  return &_cu;
}

Builder *Visitor::builder() {
  return &_builder;
}
