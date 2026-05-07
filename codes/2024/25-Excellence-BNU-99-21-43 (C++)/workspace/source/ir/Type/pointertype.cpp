#include "pointertype.h"

namespace IR
{
    std::map<BasicType *, PointerType *> PointerType::pointerTypeMap;
    PointerType *PointerType::get(BasicType *baseType)
    {
        if (pointerTypeMap.find(baseType) != pointerTypeMap.end())
            return pointerTypeMap[baseType];
        PointerType *newType = new PointerType(baseType);
        pointerTypeMap[baseType] = newType;
        return newType;
    }
}