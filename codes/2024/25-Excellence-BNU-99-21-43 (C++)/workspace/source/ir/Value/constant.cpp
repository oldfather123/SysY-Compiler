#include "constant.h"

namespace IR
{
    Constant *Constant::getZeroValueForType(BasicType *type)
    {
        if (type->isInt32())
            return new ConstantInt32(0);
        else if (type->isFloat())
            return new ConstantFloat(0.0);
        else if (type->isArray())
            return new ConstantArray(type);
        else
        {
            Error::Error(__PRETTY_FUNCTION__, "Unknown type");
            return nullptr;
        }
    }
}