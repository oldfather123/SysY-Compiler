#pragma once
#include "basictype.h"

namespace IR
{
    struct PointerType : public BasicType
    {
        static std::map<BasicType *, PointerType *> pointerTypeMap;
        BasicType *baseType;
        PointerType(BasicType *baseType) : BasicType(8, BasicType::TypeID::POINT), baseType(baseType) {}

        // czr: 输出类型的名字，比如int32 *，注意测试一下数组的情况
        std::string getTypeName() const override
        {
            return baseType->getTypeName() + " *";
        }

        // czr: 输出空的vector，因为int32不是数组（ps: 这tm不能错吧）
        std::vector<int> getArrayDims() const override
        {
            return {};
        }

        // czr: 返回自己，因为int32不是数组（ps: 这能错？）
        BasicType *getBaseType() const override
        {
            return baseType;
        }

        // czr: 没用的函数，返回空字符串
        const std::string _getArrayDims_() const override
        {
            return "";
        }

        static PointerType *get(BasicType *baseType);
    };
}