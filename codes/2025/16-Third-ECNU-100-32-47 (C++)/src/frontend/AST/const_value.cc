#include "AST/const_value.h"
#include "AST/expr.h"
namespace aaac {
namespace frontend {
ConstValue::~ConstValue() {
    delete expr;
}
}
}