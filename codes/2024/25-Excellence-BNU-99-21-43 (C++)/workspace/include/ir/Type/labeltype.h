#pragma once
#include "basictype.h"

namespace IR
{
    struct LabelType : public BasicType
    {
        LabelType() : BasicType(0, BasicType::TypeID::LABLE) {}

        // czr: 输出类型的名字，比如label (ps: 这tm不能错吧)
        std::string getTypeName() const override
        {
            return "label";
        }

        // czr: 输出空的vector，因为label不是数组（ps: 这tm不能错吧）
        std::vector<int> getArrayDims() const override
        {
            return {};
        }

        // czr: 返回 nullptr，因为label不是数组（ps: 这tm不能错吧）
        BasicType *getBaseType() const override
        {
            return nullptr;
        }

        static LabelType *Instance;

        // czr: 返回全局唯一的LabelType对象
        static LabelType *getInstance()
        {
            if (Instance == nullptr)
            {
                Instance = new LabelType();
            }
            return Instance;
        }
    };
};