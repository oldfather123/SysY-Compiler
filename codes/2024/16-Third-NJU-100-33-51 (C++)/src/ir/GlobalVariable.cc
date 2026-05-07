#include "GlobalVariable.hh"

GlobalVariable::GlobalVariable(Type* type, string name)
    : GlobalValue(Type::getPointerType(type), name, VT_GLOBALVAR) {
  initValue = Constant::getZeroConstant(type);
}

GlobalVariable::GlobalVariable(Type* type, Constant* init, string name)
    : GlobalValue(Type::getPointerType(type), name, VT_GLOBALVAR) {
  initValue = init;
}

void GlobalVariable::printIR(ostream& stream) const {
  stream << toString() << " = dso_local global "
         << initValue->getType()->toString() << " " << initValue->toString();
}
