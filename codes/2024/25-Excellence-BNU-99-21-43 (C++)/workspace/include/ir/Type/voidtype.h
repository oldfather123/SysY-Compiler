#pragma once

#include "basictype.h"

namespace IR
{
    // czr: 与其他类型测试差不多。
    struct VoidType : public BasicType
    {
        VoidType() : BasicType(0, BasicType::TypeID::VOID) {}

        std::string getTypeName() const override
        {
            return "void";
        }

        std::vector<int> getArrayDims() const override
        {
            return {};
        }

        BasicType *getBaseType() const override
        {
            return getInstance();
        }

        const std::string _getArrayDims_() const override
        {
            return "";
        }

        // 全局唯一的VoidType对象
        static VoidType *instance;
        static VoidType *getInstance()
        {
            if (instance == nullptr)
            {
                instance = new VoidType();
            }
            return instance;
        }
    };
}