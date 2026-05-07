#ifndef COMPILER2025_X_TYPE_H
#define COMPILER2025_X_TYPE_H

#include <string>
#include <vector>
#include <memory>

namespace compiler
{

    // 类型的基类
    class Type
    {
    public:
        enum class TypeKind
        {
            VOID,
            INT,
            FLOAT,
            ARRAY,
            FUNCTION
        };

        Type(TypeKind kind) : kind_(kind) {}
        virtual ~Type() = default;

        TypeKind getKind() const { return kind_; }
        virtual std::string toString() const = 0;

    private:
        TypeKind kind_;
    };

    // Void类型
    class VoidType : public Type
    {
    public:
        VoidType() : Type(TypeKind::VOID) {}
        std::string toString() const override { return "void"; }

        static std::shared_ptr<VoidType> getInstance();
    };

    // Int类型
    class IntType : public Type
    {
    public:
        IntType() : Type(TypeKind::INT) {}
        std::string toString() const override { return "int"; }

        // 获取全局唯一的IntType实例（单例模式，节省内存，方便类型比较）
        static std::shared_ptr<IntType> getInstance();
    };

    // Float类型
    class FloatType : public Type
    {
    public:
        FloatType() : Type(TypeKind::FLOAT) {}
        std::string toString() const override { return "float"; }

        // 获取全局唯一的FloatType实例（单例模式，节省内存，方便类型比较）
        static std::shared_ptr<FloatType> getInstance();
    };

    // 数组类型
    class ArrayType : public Type
    {
    public:
        ArrayType(std::shared_ptr<Type> elementType, const std::vector<int> &dimensions)
            : Type(TypeKind::ARRAY), elementType_(elementType), dimensions_(dimensions) {}

        std::shared_ptr<Type> getElementType() const { return elementType_; }
        const std::vector<int> &getDimensions() const { return dimensions_; }
        int getRank() const { return dimensions_.size(); }

        std::string toString() const override;

    private:
        std::shared_ptr<Type> elementType_;
        std::vector<int> dimensions_; // -1表示该维度的大小未知
    };

    // 函数类型
    class FunctionType : public Type
    {
    public:
        FunctionType(std::shared_ptr<Type> returnType, const std::vector<std::shared_ptr<Type>> &paramTypes)
            : Type(TypeKind::FUNCTION), returnType_(returnType), paramTypes_(paramTypes) {}

        std::shared_ptr<Type> getReturnType() const { return returnType_; }
        const std::vector<std::shared_ptr<Type>> &getParamTypes() const { return paramTypes_; }

        std::string toString() const override;

    private:
        std::shared_ptr<Type> returnType_;
        std::vector<std::shared_ptr<Type>> paramTypes_;
    };

    // 类型工具函数
    namespace TypeUtils
    {
        std::shared_ptr<ArrayType> createArrayType(std::shared_ptr<Type> elementType, const std::vector<int> &dimensions);
        std::shared_ptr<FunctionType> createFunctionType(std::shared_ptr<Type> returnType, const std::vector<std::shared_ptr<Type>> &paramTypes);
    }

} // namespace compiler

#endif // COMPILER2025_X_TYPE_H