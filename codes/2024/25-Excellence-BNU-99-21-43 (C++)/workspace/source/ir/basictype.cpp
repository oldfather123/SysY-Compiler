#include "basictype.h"
#include "int32type.h"
#include "floattype.h"
#include "voidtype.h"
#include "errortype.h"
#include "arraytype.h"

namespace IR
{
    Int32Type *BasicType::getInt32Type()
    {
        return Int32Type::getInstance();
    }

    FloatType *BasicType::getFloatType()
    {
        return FloatType::getInstance();
    }

    VoidType *BasicType::getVoidType()
    {
        return VoidType::getInstance();
    }

    ErrorType *BasicType::getErrorType()
    {
        return ErrorType::getInstance();
    }

    ArrayType *BasicType::getArrayType(BasicType *baseType, int length)
    {
        return ArrayType::getArrayType(baseType, length);
    }

}