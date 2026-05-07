#include "Constant.hh"

#include <bitset>
#include <iomanip>
#include <sstream>
map<int, IntegerConstant*> IntegerConstant::constBuffer;
map<float, FloatConstant*> FloatConstant::constBuffer;

IntegerConstant* IntegerConstant::getConstInt(int num) {
  auto elem = constBuffer.find(num);
  if (elem != constBuffer.end()) {
    return elem->second;
  }
  IntegerConstant* intConst = new IntegerConstant(num);
  constBuffer.emplace(num, intConst);
  return intConst;
}

FloatConstant* FloatConstant::getConstFloat(float num) {
  auto elem = constBuffer.find(num);
  if (elem != constBuffer.end()) {
    return elem->second;
  }
  FloatConstant* floatConst = new FloatConstant(num);
  constBuffer.emplace(num, floatConst);
  return floatConst;
}

Constant* Constant::getZeroConstant(Type* t) {
  switch (t->getTypeTag()) {
    case TT_INT1:
      return BoolConstant::getConstBool(0);
    case TT_INT32:
      return IntegerConstant::getConstInt(0);
    case TT_FLOAT:
      return FloatConstant::getConstFloat(0);
    case TT_ARRAY:
      return ArrayConstant::getConstArray((ArrayType*)t);
    default:
      return nullptr;
  }
}

string ArrayConstant::toString() const {
  if (isZeroInit()) {
    return "zeroinitializer";
  }
  string arrStr = "[";
  int len = ((ArrayType*)getType())->getLen();
  Type* type = ((ArrayType*)getType())->getElemType();
  for (int i = 0; i < len; i++) {
    auto loc = elems->find(i);
    Constant* elem;
    if (loc != elems->end()) {
      elem = (Constant*)loc->second;
    } else {
      elem = Constant::getZeroConstant(type);
    }
    if (i != 0) {
      arrStr += ", ";
    }
    arrStr += type->toString();
    arrStr += " ";
    arrStr += elem->toString();
  }
  arrStr += "]";
  return arrStr;
}

void ArrayConstant::put(int loc, Value* v) {
  setZeroInit(false);
  elems->emplace(loc, v);
}

string FloatConstant::toString() const {
  double tValue = value;
  uint64_t bits = *reinterpret_cast<uint64_t*>(&tValue);
  std::stringstream ss;
  ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(16)
     << bits;
  return ss.str();
}

BoolConstant* BoolConstant::one = 0;
BoolConstant* BoolConstant::zero = 0;

BoolConstant* BoolConstant::getConstBool(bool val) {
  if (!one) {
    one = new BoolConstant(1);
    zero = new BoolConstant(0);
  }
  if (val) {
    return one;
  } else {
    return zero;
  }
}