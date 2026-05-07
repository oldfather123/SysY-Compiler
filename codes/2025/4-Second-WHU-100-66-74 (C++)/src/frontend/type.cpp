#include "type.h"
#include <sstream>

namespace compiler
{

    // 单例实例
    static std::shared_ptr<VoidType> voidTypeInstance = nullptr;
    static std::shared_ptr<IntType> intTypeInstance = nullptr;
    static std::shared_ptr<FloatType> floatTypeInstance = nullptr;

    // 获取Void类型实例
    std::shared_ptr<VoidType> VoidType::getInstance()
    {
        if (!voidTypeInstance)
        {
            voidTypeInstance = std::make_shared<VoidType>();
        }
        return voidTypeInstance;
    }

    // 获取Int类型实例
    std::shared_ptr<IntType> IntType::getInstance()
    {
        if (!intTypeInstance)
        {
            intTypeInstance = std::make_shared<IntType>();
        }
        return intTypeInstance;
    }

    // 获取Float类型实例
    std::shared_ptr<FloatType> FloatType::getInstance()
    {
        if (!floatTypeInstance)
        {
            floatTypeInstance = std::make_shared<FloatType>();
        }
        return floatTypeInstance;
    }

    // ArrayType的toString方法实现
    std::string ArrayType::toString() const
    {
        std::stringstream ss;
        ss << elementType_->toString();

        for (int dim : dimensions_)
        {
            ss << "[";
            if (dim != -1)
            {
                ss << dim;
            }
            ss << "]";
        }

        return ss.str();
    }

    // FunctionType的toString方法实现
    std::string FunctionType::toString() const
    {
        std::stringstream ss;
        ss << returnType_->toString() << " (";

        for (size_t i = 0; i < paramTypes_.size(); i++)
        {
            if (i > 0)
            {
                ss << ", ";
            }
            ss << paramTypes_[i]->toString();
        }

        ss << ")";
        return ss.str();
    }

    // 类型工具实现
    namespace TypeUtils
    {

        std::shared_ptr<ArrayType> createArrayType(std::shared_ptr<Type> elementType, const std::vector<int> &dimensions)
        {
            return std::make_shared<ArrayType>(elementType, dimensions);
        }

        std::shared_ptr<FunctionType> createFunctionType(std::shared_ptr<Type> returnType, const std::vector<std::shared_ptr<Type>> &paramTypes)
        {
            return std::make_shared<FunctionType>(returnType, paramTypes);
        }

    } // namespace TypeUtils

} // namespace compiler