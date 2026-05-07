#include "Type.hh"

#include "Constant.hh"

Int1Type* Type::int1Type = new Int1Type();
Int32Type* Type::int32Type = new Int32Type();
FloatType* Type::floatType = new FloatType();
VoidType* Type::voidType = new VoidType();

//
PointerType* Type::getPointerType(Type* elemType) {
  return new PointerType(elemType);
}

//
ArrayType* Type::getArrayType(int n, Type* elemType) {
  return new ArrayType(n, elemType);
}

//
FuncType* Type::getFuncType(Type* retType, vector<Argument*>& args) {
  FuncType* funcType = new FuncType(retType);
  for (auto arg : args) {
    funcType->pushArgument(arg);
  }
  return funcType;
}

//
FuncType* Type::getFuncType(Type* retType) { return new FuncType(retType); }

string VoidType::toString() const { return "void"; }

string Int1Type::toString() const { return "i1"; }

string Int32Type::toString() const { return "i32"; }

string FloatType::toString() const { return "float"; }

string ArrayType::toString() const {
  return "[" + std::to_string(len) + " x " + elemType->toString() + "]";
}

string PointerType::toString() const { return elemType->toString() + "*"; }

FuncType::FuncType(Type* rType) : Type(TT_FUNCTION, 4), retType(rType) {
  arguments = make_unique<vector<unique_ptr<Argument>>>();
};

void FuncType::pushArgument(Argument* arg) {
  arguments->push_back(unique_ptr<Argument>(arg));
}

bool Value::isFloat() const { return vType && vType->getTypeTag() == TT_FLOAT; }
bool Value::isArray() const { return vType && vType->getTypeTag() == TT_ARRAY; }
bool Value::isInteger() const {
  return vType && vType->getTypeTag() == TT_INT32;
}
bool Value::isPointer() const {
  return vType && vType->getTypeTag() == TT_POINTER;
}

Constant* Int32Type::getZeroInit() { return IntegerConstant::getConstInt(0); }

Constant* FloatType::getZeroInit() { return FloatConstant::getConstFloat(0); }

Constant* ArrayType::getZeroInit() {
  return ArrayConstant::getConstArray(this);
}