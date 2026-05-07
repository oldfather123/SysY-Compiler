#pragma once
#include "basictype.h"
#include <algorithm>
#include <map>

namespace IR
{

    struct ArrayType : public BasicType
    {
        static std::map<std::pair<BasicType *, int>, ArrayType *> arrayTypeMap;
        BasicType *baseType;
        BasicType *whoIAm;
        int length;

        ArrayType(BasicType *baseType, int length) : BasicType(baseType->sz * length, BasicType::TypeID::ARRAY), baseType(baseType), length(length)
        {
            whoIAm = baseType->getBaseType();
        }

        // czr: 输出类型的名字，比如[10 x i32]，[10 x [10 x i32]]等
        std::string getTypeName() const override
        {
            return "[" + std::to_string(length) + " x " + baseType->getTypeName() + "]";
        }

        // czr: 输出类型的维度，比如[10, 10]，[10, 10, 10]等
        std::vector<int> getArrayDims() const override
        {
            std::vector<int> dims = baseType->getArrayDims();
            dims.push_back(length);
            std::reverse(dims.begin(), dims.end());
            return dims;
        }

        // czr: 返回BaseType，即另一个ArrayType或者IntType或FloatType，因为是嵌套的，
        //      比如二维数组的BaseType是一维数组
        BasicType *getBaseType() const override
        {
            return baseType;
        }

        // czr: 返回基本类型，即IntType或FloatType
        BasicType *getWhoIAm() const
        {
            return whoIAm;
        }

        // czr: 这个好像没啥用
        const std::string _getArrayDims_() const override
        {
            return "[" + std::to_string(length) + "]" + baseType->_getArrayDims_();
        }

        // czr: 返回下一个类型，即BaseType (ps: 这个函数在这里好像没啥用)
        BasicType *getNextType() const
        {
            return baseType;
        }

        // czr: 返回数组的长度，比如[10 x i32]的长度是10，[10 x [5 x i32]]的长度是10
        int getLength() const override
        {
            return length;
        }

        // 获取数组的基本类型
        static ArrayType *getArrayType(BasicType *baseType, int length);
    };

}
