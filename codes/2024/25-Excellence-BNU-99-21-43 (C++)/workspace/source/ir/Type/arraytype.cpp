#include "arraytype.h"

namespace IR
{
    std::map<std::pair<BasicType *, int>, ArrayType *> ArrayType::arrayTypeMap;
    ArrayType *ArrayType::getArrayType(BasicType *baseType, int size)
    {
        if (arrayTypeMap.find(std::make_pair(baseType, size)) != arrayTypeMap.end())
            return arrayTypeMap[std::make_pair(baseType, size)];
        arrayTypeMap[std::make_pair(baseType, size)] = new ArrayType(baseType, size);
        return arrayTypeMap[std::make_pair(baseType, size)];
    }
}