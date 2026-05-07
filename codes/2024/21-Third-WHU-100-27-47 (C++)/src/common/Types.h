/* 数据类型 */

#pragma once

#include "Defs.h"

/// @brief Represents primary data type
enum class PrimaryDataType {
    Unknown,
    Void,
    Int,
    Float,
    Bool,
    String,
};

/// @brief Represents data type, readonly & copyable
struct DataType {
    friend struct std::hash<DataType>;

public:
    enum { UNSPECIFIED_ARRAY_SIZE = -1 };

private:
    PrimaryDataType _baseDataType{};
    bool _isConst = false;
    bool _isPointer = false; // DataType can be array pointer ([]*) or primary type pointer (*), but can't be pointer of pointer (**) or pointer array (*[])
    bool _is64Bit = false;
    Vector<int> _arraySizes{};

public: /* constructors */
    DataType(const PrimaryDataType &baseType, const Vector<int> &arraySizes, const bool &isConst, const bool &isPointer, const bool &is64Bit)
        : _baseDataType(baseType), _arraySizes(arraySizes), _isConst(isConst), _isPointer(isPointer), _is64Bit(is64Bit) { }
    DataType() : DataType(PrimaryDataType::Unknown) { }
    DataType(const PrimaryDataType &baseType, const Vector<int> &arraySizes = Vector<int>())
        : DataType(baseType, arraySizes, false, false, false) { }

    /// @brief Get array type of a data type
    /// @param arraySize Size of the array (default `UNSPECFIED_ARRAY_SIZE` means no size)
    /// @return Array type of `this` (should be a non-pointer non-void type)
    const DataType arrayType(const int &arraySize = UNSPECIFIED_ARRAY_SIZE) const {
        dbgassert(!this->_isPointer, "Cannot get array type of a pointer type");
        dbgassert(*this != PrimaryDataType::Void, "Cannot get array type of Void");
        Vector<int> arraySizes;
        arraySizes.assign(this->_arraySizes.begin(), this->_arraySizes.end());
        arraySizes.push_back(arraySize);
        return DataType(this->_baseDataType, arraySizes);
    }
    /// @brief Get element type of a type
    /// @param
    /// @return Element type of `this` (should be a non-pointer array type)
    const DataType elemType() const {
        dbgassert(!this->_isPointer, "Cannot get element type of a pointer type");
        dbgassert(this->_arraySizes.size() != 0, "Cannot get element type of a primary data type");
        Vector<int> arraySizes;
        arraySizes.assign(this->_arraySizes.begin() + 1, this->_arraySizes.end());
        return DataType(this->_baseDataType, arraySizes);
    }
    /// @brief Get pointer type of a non-pointer type
    /// @param
    /// @return Pointer type of `this` (should be a non-pointer type)
    const DataType ptrType() const {
        dbgassert(!this->_isPointer, "Cannot get pointer type of a pointer type");
        return DataType(this->_baseDataType, this->_arraySizes, this->_isConst, true, this->_is64Bit);
    }
    /// @brief Get dereferenced type of a pointer type
    /// @param
    /// @return Deferenced non-pointer type of `this` (should be a pointer type)
    const DataType derefType() const {
        dbgassert(this->_isPointer, "Cannot get deref type of a non-pointer type");
        return DataType(this->_baseDataType, this->_arraySizes, this->_isConst, false, this->_is64Bit);
    }
    /// @brief Get const type of a type
    /// @param
    /// @return Const type of `this`
    DataType constType() {
        return DataType(this->_baseDataType, this->_arraySizes, true, this->_isPointer, this->_is64Bit);
    }
    const DataType longType() const {
        return DataType(this->_baseDataType, this->_arraySizes, this->_isConst, this->_isPointer, true);
    }
    /// @brief Copy constructor
    /// @param other
    DataType(const DataType &other) = default;

public: /* properties */
    const PrimaryDataType &baseDataType() const { return _baseDataType; }
    const Vector<int> &arraySizes() const { return _arraySizes; }
    const bool &isConst() const { return _isConst; }
    const bool &isPointer() const { return _isPointer; }
    const bool &is64Bit() const { return _is64Bit; }
    int arrayDimensionCount() const { return _arraySizes.size(); }
    bool isPrimary() const { return !isPointer() && arrayDimensionCount() == 0; }
    bool isArray() const { return !isPointer() && arrayDimensionCount() != 0; }
    bool isNumeric() const { return isPrimary() && (_baseDataType == PrimaryDataType::Int || _baseDataType == PrimaryDataType::Float || _baseDataType == PrimaryDataType::Bool); }
    int size() const {
        if (isPointer()) {
            return 1;
        }
        int size = 1;
        for (auto arraySize : _arraySizes) {
            size *= arraySize;
        }
        return size;
    }
    int bytes() const {
        if (isPointer()) {
            return 8; // 64-bit pointer
        } else if (is64Bit()) {
            return 8;
        } else {
            int baseBytes = 0;
            switch (_baseDataType) {
                case PrimaryDataType::Int:
                case PrimaryDataType::Float: {
                    baseBytes = 4;
                    break;
                }
                case PrimaryDataType::Bool: {
                    baseBytes = 1;
                    break;
                }
                default: {
                    dbgassert(false, "Unsupported base data type");
                    break;
                }
            }
            return baseBytes * size();
        }
    }

public: /* methods */
    const bool operator==(const PrimaryDataType &primaryDataType) const {
        return isPrimary() && _baseDataType == primaryDataType;
    }
    const bool operator!=(const PrimaryDataType &primaryDataType) const {
        return !(*this == primaryDataType);
    }
    const bool operator==(const DataType &other) const {
        return this->_baseDataType == other._baseDataType && this->_arraySizes == other._arraySizes && this->_isPointer == other._isPointer && this->_is64Bit == other._is64Bit;
    }
    const bool operator!=(const DataType &other) const {
        return !(*this == other);
    }

    String toString() const;
};

namespace std {
    template <>
    struct hash<DataType> {
        std::size_t operator()(const DataType &dataType) const {
            std::size_t result = 0;
            hashCombine(result, dataType._baseDataType);
            hashCombine(result, dataType._isConst);
            hashCombine(result, dataType._isPointer);
            hashCombine(result, dataType._is64Bit);
            for (auto arraySize : dataType._arraySizes) {
                hashCombine(result, arraySize);
            }
            return result;
        }
    };
}
