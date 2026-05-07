#pragma once
#include "basictype.h"
#include "error.h"

namespace IR
{
    // czr: 这个类是一个特殊的类，只有一个对象，用来表示错误类型
    //      用于在类型检查的时候，当出现错误时返回这个对象
    struct ErrorType : public BasicType
    {
        ErrorType() : BasicType(0, BasicType::TypeID::ERROR) {}

        std::string getTypeName() const override
        {
            Error::Error(__PRETTY_FUNCTION__, "Type not defined");
            return "";
        }

        std::vector<int> getArrayDims() const override
        {
            Error::Error(__PRETTY_FUNCTION__, "Type not defined");
            return {};
        }

        BasicType *getBaseType() const override
        {
            Error::Error(__PRETTY_FUNCTION__, "Type not defined");
            return nullptr;
        }

        const std::string _getArrayDims_() const override
        {
            Error::Error(__PRETTY_FUNCTION__, "Type not defined");
            return "";
        }

        // 全局唯一的ErrorType对象
        static ErrorType *instance;
        static ErrorType *getInstance()
        {
            if (instance == nullptr)
            {
                instance = new ErrorType();
            }
            return instance;
        }
    };
}