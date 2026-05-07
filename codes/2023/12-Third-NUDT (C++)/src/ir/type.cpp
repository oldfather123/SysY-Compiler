#include "type.hpp"

using namespace IR;

using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

using PointerTypes = typename std::unordered_map<Type *, unique_ptr<PointerType>>;
using ArrayTypes = typename std::unordered_set<unique_ptr<ArrayType>>;
using FunctionTypes = typename std::unordered_set<unique_ptr<FunctionType>>;

/// @brief Type
Type::Type(BType btype) : _btype(btype) {
}
Type *Type::void_t() {
  static Type void_type(VOID);
  return &void_type;
}
Type *Type::char_t() {
  static Type char_type(CHAR);
  return &char_type;
}
Type *Type::i1_t() {
  static Type i1_type(I1);
  return &i1_type;
}
Type *Type::i32_t() {
  static Type i32_type(I32);
  return &i32_type;
}
Type *Type::float_t() {
  static Type float_type(FLOAT);
  return &float_type;
}
Type *Type::label_t() {
  static Type label_type(LABEL);
  return &label_type;
}
Type *Type::dynamic_t() {
  static Type dynamic_type(DYNAMIC);
  return &dynamic_type;
}
Type *Type::undefine_t() {
  static Type undefine_type(UNDEFINE);
  return &undefine_type;
}
Type *Type::pointer_t(Type *base) {
  return PointerType::get(base);
}
Type *Type::array_t(Type *base, size_t dim) {
  return ArrayType::get(base, dim);
}
Type *Type::function_t(Type *ret, const vector<Type *> &param_types) {
  return FunctionType::get(ret, param_types);
}
bool Type::in(const vector<Type *> &types) {
  for (const auto &type : types)
    if (this == type) return true;
  return false;
}
bool Type::notin(const vector<Type *> &types) {
  for (const auto &type : types)
    if (this == type) return false;
  return true;
}
bool Type::is(Type *type) {
  return this == type;
}
bool Type::isnot(Type *type) {
  return this != type;
}
bool Type::isVoid() {
  return _btype == VOID;
}
bool Type::isChar() {
  return _btype == CHAR;
}
bool Type::isI1() {
  return _btype == I1;
}
bool Type::isI32() {
  return _btype == I32;
}
bool Type::isFloat() {
  return _btype == FLOAT;
}
bool Type::isLabel() {
  return _btype == LABEL;
}
bool Type::isDynamic() {
  return _btype == DYNAMIC;
}
bool Type::isUndefine() {
  return _btype == UNDEFINE;
}
bool Type::isPointer() {
  return _btype == POINTER;
}
bool Type::isArray() {
  return _btype == ARRAY;
}
bool Type::isFunction() {
  return _btype == FUNCTION;
}
Type *Type::btype() {
  switch (_btype) {
    case VOID:
    case CHAR:
    case I1:
    case I32:
    case FLOAT:
    case LABEL:
    case UNDEFINE:
    case FUNCTION:
      return this;
    default:
      throw InvalidType();
  }
}
string Type::name() {
  switch (_btype) {
    case VOID:
      return "void";
    case CHAR:
      return "char";
    case I1:
      return "i1";
    case I32:
      return "i32";
    case FLOAT:
      return "float";
    case LABEL:
      return "label";
    case DYNAMIC:
      return "...";
    case POINTER:
      return "ptr";
    default:
      throw InvalidType();
  }
}
size_t Type::sizeOf() {
  switch (_btype) {
    case VOID:
      return 0;
    case CHAR:
    case I1:
      return 1;
    case I32:
    case FLOAT:
      return 4;
    case LABEL:
    case POINTER:
    case FUNCTION:
      return 8;
    default:
      throw InvalidType();
  }
}
size_t Type::size() {
  switch (_btype) {
    case VOID:
    case CHAR:
    case I1:
    case I32:
    case FLOAT:
    case LABEL:
    case POINTER:
    case FUNCTION:
      return 1;
    default:
      throw InvalidType();
  }
}
size_t Type::refSize() {
  switch (_btype) {
    case VOID:
    case CHAR:
    case I1:
    case I32:
    case FLOAT:
    case LABEL:
    case FUNCTION:
      return 0;
    default:
      throw InvalidType();
  }
}
Type *Type::deref(size_t deref) {
  if (deref == 0) return this;
  throw InvalidType();
}
size_t Type::dim() {
  throw InvalidType();
}
Type *Type::toPointer() {
  throw InvalidType();
}
Type *Type::retType() {
  throw InvalidType();
}
vector<Type *> &Type::paramTypes() {
  throw InvalidType();
}

/// @brief PointerType
PointerType::PointerType(Type *base) : Type(POINTER), _base(base) {
}
PointerType *PointerType::get(Type *base) {
  static PointerTypes pointer_types;
  auto it = pointer_types.find(base);
  if (it != pointer_types.end()) return (it->second).get();
  PointerType *pointer_type = new PointerType(base);
  assert(pointer_type);
  auto res = pointer_types.emplace(base, pointer_type);
  return res.first->second.get();
}
Type *PointerType::btype() {
  return deref(1)->btype();
}
size_t PointerType::refSize() {
  return 1 + _base->refSize();
}
Type *PointerType::deref(size_t deref_size) {
  if (deref_size == 0) return this;
  return _base->deref(deref_size - 1);
}

/// @brief ArrayType
ArrayType::ArrayType(Type *base, size_t dim) : Type(ARRAY), _base(base), _dim(dim), _size(_base->size() * dim) {
}
ArrayType *ArrayType::get(Type *base, size_t dim) {
  static ArrayTypes array_types;
  auto it = find_if(array_types.begin(), array_types.end(), [&](const unique_ptr<ArrayType> &array_type) -> bool {
    if (base != array_type->_base) return false;
    if (dim != array_type->dim()) return false;
    return true;
  });
  if (it != array_types.end()) return it->get();
  ArrayType *array_type = new ArrayType(base, dim);
  assert(array_type);
  auto res = array_types.emplace(array_type);
  return res.first->get();
}
Type *ArrayType::btype() {
  return deref(1)->btype();
}
string ArrayType::name() {
  return '[' + to_string(dim()) + " x " + deref(1)->name() + ']';
}
size_t ArrayType::sizeOf() {
  return _dim * _base->sizeOf();
}
size_t ArrayType::size() {
  return _size;
}
size_t ArrayType::refSize() {
  return 1 + _base->refSize();
}
Type *ArrayType::deref(size_t deref_size) {
  if (deref_size == 0) return this;
  return _base->deref(deref_size - 1);
}
size_t ArrayType::dim() {
  return _dim;
}
Type *ArrayType::toPointer() {
  return PointerType::get(_base);
}

/// @brief FuncType
FunctionType::FunctionType(Type *ret_type, const vector<Type *> &param_types)
    : Type(FUNCTION), _ret_type(ret_type), _param_types(param_types) {
}
FunctionType *FunctionType::get(Type *ret, const vector<Type *> &param_types) {
  static FunctionTypes func_types;
  auto it = find_if(func_types.begin(), func_types.end(), [&](const unique_ptr<FunctionType> &func_type) -> bool {
    if (ret != func_type->retType()) return false;
    if (param_types.size() != func_type->paramTypes().size()) return false;
    return equal(param_types.begin(), param_types.end(), func_type->paramTypes().begin());
  });
  if (it != func_types.end()) return it->get();
  FunctionType *func_type = new FunctionType(ret, param_types);
  assert(func_type);
  return func_types.emplace(func_type).first->get();
}
Type *FunctionType::retType() {
  return _ret_type;
}
vector<Type *> &FunctionType::paramTypes() {
  return _param_types;
}
string FunctionType::name() {
  return _ret_type->name() + "(" +
         accumulate(_param_types.begin(), _param_types.end(), string(""),
                    [&](const string &acc, Type *type) -> string {
                      return acc + (acc.length() ? ", " : "") + type->name();
                    }) +
         ")";
}
