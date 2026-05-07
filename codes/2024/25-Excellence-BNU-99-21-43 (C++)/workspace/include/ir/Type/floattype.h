#pragma once

#include "basictype.h"

namespace IR
{

    struct FloatType : public BasicType
    {
        FloatType() : BasicType(4, BasicType::TypeID::FLOAT) {}

        // czr: 输出类型的名字，比如float (ps: 这tm不能错吧)
        std::string getTypeName() const override
        {
            return "float";
        }

        // czr: 输出空的vector，因为float不是数组（ps: 这tm不能错吧）
        std::vector<int> getArrayDims() const override
        {
            return {};
        }

        // czr: 返回自己，因为float不是数组（ps: 这tm不能错吧）
        BasicType *getBaseType() const override
        {
            return getInstance();
        }

        // czr: 返回空字符串，因为float不是数组（ps: 这tm不能错吧）
        const std::string _getArrayDims_() const override
        {
            return "";
        }

        // 全局唯一的FloatType对象
        static FloatType *instance;

        // czr: 返回全局唯一的FloatType对象
        static FloatType *getInstance()
        {
            if (instance == nullptr)
            {
                instance = new FloatType();
            }
            return instance;
        }
    };
}