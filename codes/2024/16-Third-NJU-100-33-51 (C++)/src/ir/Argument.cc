#include "Type.hh"
#include "Argument.hh"

string Argument::toString() const {
  return "%" + getName();
}
