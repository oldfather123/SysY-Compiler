#ifndef GEECEECEE_IR_TYPE_HPP
#define GEECEECEE_IR_TYPE_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace ir {

// LLVm type, all the types are derived from this class.
// To instance a type, use the static method `get()`, which uses the cache to avoid duplicate instances,
// all the types are singleton, which means that there is only one instance of each type.
// `Type` uses shared_ptr to manage its lifetime.
class Type {
public:
    enum class TypeID { VOID_ID, LABEL_ID, INTEGER_ID, FLOAT_ID, FUNCTION_ID, POINTER_ID, ARRAY_ID, PLACEHOLDER_ID };

public:
    TypeID get_type_id() const { return id; }
    virtual std::string to_string() const = 0;
    [[nodiscard]] virtual int bits_num() const = 0;

    bool operator==(const Type &other) const { return this == &other; }
    bool operator!=(const Type &other) const { return this != &other; }
    friend std::ostream &operator<<(std::ostream &os, const Type &type);
    virtual ~Type() = default;
    [[nodiscard]] bool is_void_ty() const { return id == TypeID::VOID_ID; }
    [[nodiscard]] bool is_integer_ty() const { return id == TypeID::INTEGER_ID; }
    [[nodiscard]] bool is_float_ty() const { return id == TypeID::FLOAT_ID; }
    [[nodiscard]] bool is_function_ty() const { return id == TypeID::FUNCTION_ID; }
    [[nodiscard]] bool is_pointer_ty() const { return id == TypeID::POINTER_ID; }
    [[nodiscard]] bool is_array_ty() const { return id == TypeID::ARRAY_ID; }

protected:
    explicit Type(TypeID id) : id(id) {}

private:
    TypeID id;
};

class VoidType : public Type {
public:
    static std::shared_ptr<VoidType> get();
    std::string to_string() const override;
    int bits_num() const override { __builtin_unreachable(); }

private:
    VoidType() : Type(TypeID::VOID_ID) {}
};

class LabelType : public Type {
public:
    static std::shared_ptr<LabelType> get();
    std::string to_string() const override;
    int bits_num() const override { __builtin_unreachable(); }

private:
    LabelType() : Type(TypeID::LABEL_ID) {}
};

class IntegerType : public Type {
public:
    static std::shared_ptr<IntegerType> get(int bits);
    std::string to_string() const override;
    int bits_num() const override { return bits; }

private:
    int bits;
    IntegerType(int bits) : Type(TypeID::INTEGER_ID), bits(bits) {}
};

class FloatType : public Type {
public:
    static std::shared_ptr<FloatType> get();
    std::string to_string() const override;
    int bits_num() const override { return 32; }

private:
    FloatType() : Type(TypeID::FLOAT_ID) {}
};

class FunctionType : public Type {
public:
    std::shared_ptr<Type> get_return_type() const { return return_type; }
    std::vector<std::shared_ptr<Type>> get_param_types() const { return param_types; }
    static std::shared_ptr<FunctionType> get(const std::shared_ptr<Type> &return_type,
                                             const std::vector<std::shared_ptr<Type>> &param_types);
    std::string to_string() const override;
    int bits_num() const override { __builtin_unreachable(); }

private:
    std::shared_ptr<Type> return_type;
    std::vector<std::shared_ptr<Type>> param_types;

private:
    FunctionType(std::shared_ptr<Type> return_type, std::vector<std::shared_ptr<Type>> param_types)
        : Type(TypeID::FUNCTION_ID), return_type(std::move(return_type)), param_types(std::move(param_types)) {}
};

class PointerType : public Type {
public:
    std::shared_ptr<Type> get_reference_type() const { return reference_type; }
    static std::shared_ptr<PointerType> get(const std::shared_ptr<Type> &reference_type);
    std::string to_string() const override;
    int bits_num() const override { return 32; }

private:
    std::shared_ptr<Type> reference_type;
    PointerType(std::shared_ptr<Type> reference_type)
        : Type(TypeID::POINTER_ID), reference_type(std::move(reference_type)) {}
};

class ArrayType : public Type {
public:
    static std::shared_ptr<ArrayType> get(const std::shared_ptr<Type> &element_type, int size);
    std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<Type> get_element_type() const { return element_type; }
    [[nodiscard]] int get_size() const { return size; }
    int bits_num() const override { return size * element_type->bits_num(); }
    [[nodiscard]] std::vector<int> dims() const {
        if (auto arr_type = std::dynamic_pointer_cast<ArrayType>(element_type)) {
            auto sub_dims = arr_type->dims();
            sub_dims.insert(sub_dims.begin(), size);
            return sub_dims;
        }
        return {size};
    }
    [[nodiscard]] std::shared_ptr<Type> scalar_type() const {
        if (auto arr_type = std::dynamic_pointer_cast<ArrayType>(element_type)) return arr_type->scalar_type();
        return element_type;
    }
    [[nodiscard]] int get_element_nums() const {
        if (auto arr_type = std::dynamic_pointer_cast<ArrayType>(element_type))
            return arr_type->get_element_nums() * size;
        return size;
    }

private:
    std::shared_ptr<Type> element_type;
    int size;
    ArrayType(std::shared_ptr<Type> element_type, int size)
        : Type(TypeID::ARRAY_ID), element_type(std::move(element_type)), size(size) {}
};

// temp type, must be set later
class PlaceholderType : public Type {
public:
    static std::shared_ptr<PlaceholderType> get();
    std::string to_string() const override;
    int bits_num() const override { __builtin_unreachable(); }

private:
    PlaceholderType() : Type(TypeID::PLACEHOLDER_ID) {}
};

}  // namespace ir

#endif
