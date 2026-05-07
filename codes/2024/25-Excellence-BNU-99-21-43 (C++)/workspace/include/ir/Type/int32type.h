#pragma once
#include "basictype.h"

namespace IR
{

    struct Int32Type : public BasicType
    {
        Int32Type() : BasicType(4, BasicType::TypeID::INT32) {}

        // czr: 输出类型的名字，int32 （ps: 这tm不能错吧）
        std::string getTypeName() const override
        {
            return "int32";
        }

        // czr: 输出空的vector，因为int32不是数组（ps: 这tm不能错吧）
        std::vector<int> getArrayDims() const override
        {
            return {};
        }

        // czr: 返回自己，因为int32不是数组（ps: 这能错？）
        BasicType *getBaseType() const override
        {
            return getInstance();
        }

        // czr: 没用的函数，返回空字符串
        const std::string _getArrayDims_() const override
        {
            return "";
        }

        // 全局唯一的Int32Type对象
        static Int32Type *instance;

        // czr: 返回全局唯一的Int32Type对象
        static Int32Type *getInstance()
        {
            if (instance == nullptr)
            {
                instance = new Int32Type();
            }
            return instance;
        }
    };

}