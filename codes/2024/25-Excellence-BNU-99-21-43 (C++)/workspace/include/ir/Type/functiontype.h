#pragma once

#include "basictype.h"

namespace IR
{
    struct FunctionType : public BasicType
    {
        // 函数类型包含返回类型和参数类型
        BasicType *returnType;
        std::vector<BasicType *> paramTypes;

        FunctionType(BasicType *returnType, const std::vector<BasicType *> &paramTypes)
            : BasicType(0, BasicType::TypeID::FUNCTION), returnType(returnType), paramTypes(paramTypes) {}

        FunctionType(BasicType *returnType)
            : BasicType(0, BasicType::TypeID::FUNCTION), returnType(returnType) {}

        // czr: 用于添加参数类型
        void setParamType(BasicType *paramType)
        {
            paramTypes.push_back(paramType);
        }
        // czr: 用于获取返回类型
        BasicType *getParamType(unsigned int i)
        {
            return paramTypes[i];
        }
        // czr: 用于获取Type的名字
        std::string getTypeName() const override
        {
            std::string returnTypeName = returnType->getTypeName() + '(';
            int m = paramTypes.size();
            for (int i = 0; i < m; i++)
            {
                returnTypeName += paramTypes[i]->getTypeName();
                if (i != m - 1)
                    returnTypeName += ',';
            }
            return returnTypeName + ')';
        }
        // czr: 用于获取数组的维度, 返回空的vector
        std::vector<int> getArrayDims() const override
        {
            return {};
        }
        // czr: 用于获取基本类型，即返回类型
        BasicType *getBaseType() const override
        {
            return returnType;
        }
        // czr: 用于获取数组的维度，返回空字符串
        const std::string _getArrayDims_() const override
        {
            return "";
        }

        // czr: 用于构造一个FunctionType对象（ps：不可能错吧）
        static FunctionType *get(BasicType *returnType, const std::vector<BasicType *> &paramTypes)
        {
            return new FunctionType(returnType, paramTypes);
        }

        // czr: 用于构造一个FunctionType对象（ps：不可能错吧）
        static FunctionType *get(BasicType *returnType)
        {
            return new FunctionType(returnType);
        }
    };
}