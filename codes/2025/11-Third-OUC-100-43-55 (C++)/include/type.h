#ifndef IR_TYPE_H
#define IR_TYPE_H

#undef VOID

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace ir {

enum class TypeKind { BASIC, ARRAY, POINTER };

class Type {
public:
  virtual ~Type() = default;

  virtual TypeKind getTypeKind() const = 0;
  // Is recommended to use type check + static cast + unique function for
  // ArrayType and PointerType
  virtual Type *getBaseType() const = 0;
  virtual size_t getSize() const = 0;
  // Get the LLVM like string, example: i32
  virtual std::string str() const = 0;
  virtual std::unique_ptr<Type> clone() const = 0;

  bool isBasic() const { return getTypeKind() == TypeKind::BASIC; }
  bool isI32() const;
  bool isF32() const;
  bool isArray() const { return getTypeKind() == TypeKind::ARRAY; }
  bool isPointer() const { return getTypeKind() == TypeKind::POINTER; }

  static bool isEqual(const Type *lhs, const Type *rhs);
  bool operator==(const Type &rhs) const { return isEqual(this, &rhs); }
};

//
// ======== BASIC TYPE ========
//

enum class BasicKind { I1, I32, F32, VOID };

// Any comparison or identification needs to be done with enum class BasicKind,
// check function isEqual in class Type
class BasicType : public Type {
public:
  explicit BasicType(BasicKind kind);

  TypeKind getTypeKind() const override { return TypeKind::BASIC; }
  size_t getSize() const override;
  // Should not be called
  Type *getBaseType() const override { return nullptr; }
  BasicKind getBasicKind() const { return _kind; }

  std::string str() const override;
  std::unique_ptr<Type> clone() const override;

private:
  BasicKind _kind;
};

//
// ======== ARRAY TYPE ========
//

class ArrayType : public Type {
public:
  explicit ArrayType(std::unique_ptr<Type> elementType, size_t arraySize);

  TypeKind getTypeKind() const override { return TypeKind::ARRAY; }
  size_t getSize() const override;
  // Recommend getElementType()
  Type *getBaseType() const override { return _elementType.get(); }
  // Return a viewport instead of the actual ownership
  Type *getElementType() const { return _elementType.get(); }
  size_t getLength() const { return _arrayLength; }

  // Get all dimensions like: int[2][3][4] => [2, 3, 4]
  std::vector<size_t> getDimensions() const;

  // The innermost scaler type int/float of this (one or multi-dims) array
  BasicType *getInnermostElementType() const;

  std::unique_ptr<Type> clone() const override;
  std::string str() const override;

private:
  std::unique_ptr<Type> _elementType;
  size_t _arrayLength;
};

//
// ======== POINTER TYPE ========
//

class PointerType : public Type {
public:
  explicit PointerType(std::unique_ptr<Type> pointeeType);

  TypeKind getTypeKind() const override { return TypeKind::POINTER; }
  size_t getSize() const override;
  // Recommend getPointeeType()
  Type *getBaseType() const override { return _pointeeType.get(); }
  // Return a viewport instead of the actual ownership
  Type *getPointeeType() const { return _pointeeType.get(); }

  std::string str() const override;
  std::unique_ptr<Type> clone() const override;

private:
  std::unique_ptr<Type> _pointeeType;
};

} // namespace ir

#endif // IR_TYPE_H
