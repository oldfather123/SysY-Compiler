#include "type.h"
#include "core.h"
#include <cassert>
#include <memory>

namespace ir {

//
// ======== UTILITY: Type::isEqual ========
//

bool Type::isEqual(const Type *lhs, const Type *rhs) {
  if (lhs == rhs)
    return true;
  if (!lhs || !rhs)
    return false;

  if (lhs->getTypeKind() != rhs->getTypeKind())
    return false;

  switch (lhs->getTypeKind()) {
  case TypeKind::BASIC: {
    auto lhsBasic = static_cast<const BasicType *>(lhs);
    auto rhsBasic = static_cast<const BasicType *>(rhs);
    return lhsBasic->getBasicKind() == rhsBasic->getBasicKind();
  }
  case TypeKind::ARRAY: {
    auto lhsArr = static_cast<const ArrayType *>(lhs);
    auto rhsArr = static_cast<const ArrayType *>(rhs);
    return lhsArr->getLength() == rhsArr->getLength() &&
           isEqual(lhsArr->getElementType(), rhsArr->getElementType());
  }
  case TypeKind::POINTER: {
    auto lhsPtr = static_cast<const PointerType *>(lhs);
    auto rhsPtr = static_cast<const PointerType *>(rhs);
    return isEqual(lhsPtr->getPointeeType(), rhsPtr->getPointeeType());
  }
  }

  return false; // unreachable
}

bool Type::isI32() const {
  return isBasic() && (static_cast<const BasicType *>(this)->getBasicKind() ==
                       BasicKind::I32);
}

bool Type::isF32() const {
  return isBasic() && (static_cast<const BasicType *>(this)->getBasicKind() ==
                       BasicKind::F32);
}

//
// ======== BASIC TYPE ========
//

BasicType::BasicType(BasicKind kind) : _kind(kind) {}

size_t BasicType::getSize() const {
  switch (_kind) {
  case BasicKind::I1:
    return 1;
  case BasicKind::I32:
    return 32;
  case BasicKind::F32:
    return 32;
  default:
    return 0;
  }
}

std::string BasicType::str() const {
  switch (_kind) {
  case BasicKind::I1:
    return "i1";
  case BasicKind::I32:
    return "i32";
  case BasicKind::F32:
    return "float";
  case BasicKind::VOID:
    return "void";
  default:
    return "unknown";
  }
}

std::unique_ptr<Type> BasicType::clone() const {
  return std::make_unique<BasicType>(_kind);
}

//
// ======== ARRAY TYPE ========
//

ArrayType::ArrayType(std::unique_ptr<Type> elementType, size_t arraySize)
    : _elementType(std::move(elementType)), _arrayLength(arraySize) {
  assert(_elementType && "Element type cannot be null");
}

size_t ArrayType::getSize() const {
  return _arrayLength * _elementType->getSize();
}

std::string ArrayType::str() const {
  return fmt::format("[{} x {}]", _arrayLength, _elementType->str());
}

std::vector<size_t> ArrayType::getDimensions() const {
  std::vector<size_t> dims;
  const Type *curr = this;

  while (curr->isArray()) {
    auto arr = static_cast<const ArrayType *>(curr);
    dims.push_back(arr->getLength());
    curr = arr->getElementType();
  }

  return dims;
}

BasicType *ArrayType::getInnermostElementType() const {
  const Type *curr = this;
  while (curr->isArray()) {
    curr = static_cast<const ArrayType *>(curr)->getElementType();
  }
  return const_cast<BasicType *>(static_cast<const BasicType *>(curr));
}

std::unique_ptr<Type> ArrayType::clone() const {
  return std::make_unique<ArrayType>(_elementType->clone(), _arrayLength);
}

//
// ======== POINTER TYPE ========
//

PointerType::PointerType(std::unique_ptr<Type> pointeeType)
    : _pointeeType(std::move(pointeeType)) {
  assert(_pointeeType && "Pointee type cannot be null");
}

size_t PointerType::getSize() const {
  return 64; // 64-bit pointer (ATTENTION)
}

std::string PointerType::str() const { return _pointeeType->str() + "*"; }

std::unique_ptr<Type> PointerType::clone() const {
  return std::make_unique<PointerType>(_pointeeType->clone());
}

} // namespace ir
