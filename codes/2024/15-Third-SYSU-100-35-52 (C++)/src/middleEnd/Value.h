#pragma once

#include "Label.h"
#include "Type.h"
#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using std::unordered_map;
using std::vector;
struct BasicBlock;

//template<typename T>
//class NullifyingUniquePtr {
//private:
//    std::unique_ptr<T> ptr;
//    T** pp_raw;
//
//public:
//    NullifyingUniquePtr(std::unique_ptr<T>&& p) : ptr(std::move(p)), pp_raw(nullptr) {}
//    
//    ~NullifyingUniquePtr() {
//        if (pp_raw) {
//            *pp_raw = nullptr;
//        }
//    }
//
//    // 模拟 unique_ptr 的 get() 方法，但记录裸指针的地址
//    T* get() {
//        pp_raw = &ptr.get();
//        return ptr.get();
//    }
//
//    // 转发其他常用方法到内部的 unique_ptr
//    T* operator->() const { return ptr.operator->(); }
//    T& operator*() const { return *ptr; }
//    explicit operator bool() const { return static_cast<bool>(ptr); }
//
//    // 禁用拷贝
//    NullifyingUniquePtr(const NullifyingUniquePtr&) = delete;
//    NullifyingUniquePtr& operator=(const NullifyingUniquePtr&) = delete;
//
//    // 允许移动
//    NullifyingUniquePtr(NullifyingUniquePtr&& other) noexcept 
//        : ptr(std::move(other.ptr)), pp_raw(other.pp_raw) {
//        other.pp_raw = nullptr;
//    }
//    
//    NullifyingUniquePtr& operator=(NullifyingUniquePtr&& other) noexcept {
//        if (this != &other) {
//            ptr = std::move(other.ptr);
//            pp_raw = other.pp_raw;
//            other.pp_raw = nullptr;
//        }
//        return *this;
//    }
//};
//
//// 辅助函数，类似于 std::make_unique
//template<typename T, typename... Args>
//NullifyingUniquePtr<T> make_nullifying_unique(Args&&... args) {
//    return NullifyingUniquePtr<T>(std::make_unique<T>(std::forward<Args>(args)...));
//}


enum class ValueID { Const, Instruction, Variable, Void, Function };

struct Value {
    Type* type;
    Label label;
    bool isConst = false;
    virtual ~Value() = default;
    explicit Value(const Value&) = default;
    explicit Value(Value&&) = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) = default;
    Value(Type* type, const string& name = "", bool isConst = false) : type{ type }, isConst{ isConst }, label{ name } {}

    Value() : type{ Type::getVoid() } {}

    [[nodiscard]] virtual ValueID valueId() const = 0;
    virtual void dump(std::ostream& out = std::cout) = 0;

    void setLabel(const string& name) {
        label.setName(name);
    }

    [[nodiscard]] string getName() const {
        return label.name();
    }

    void setType(Type* type) {
        this->type = type;
    }

    [[nodiscard]] Type* getType() const {
        return type;
    }
    [[nodiscard]] bool isInstruction() const {
        return valueId() == ValueID::Instruction;
    }
    [[nodiscard]] bool isVariable() const {
        return valueId() == ValueID::Variable;
    }
    [[nodiscard]] bool isFunction() const {
        return valueId() == ValueID::Function;
    }
    [[nodiscard]] bool isVoid() const {
        return valueId() == ValueID::Void;
    }
    [[nodiscard]] bool isConsantant() const {
        return valueId() == ValueID::Const;
    }

    virtual BasicBlock* getBasicBlock() {
        return nullptr;
    }

    template <typename T>
    T* as() {
        static_assert(std::is_base_of_v<Value, T>);
        const auto ptr = dynamic_cast<T*>(this);
        return ptr;
    }
    template <typename T>
    bool is() {
        static_assert(std::is_base_of_v<Value, T>);
        return dynamic_cast<T*>(this);
    }

    virtual string getStr() {
        return "%" + label.name();
    }

    string getTypeAndStr() {
        return type->getStr() + " " + getStr();
    }

    [[nodiscard]] string getTypeStr() const {
        return type->getStr();
    }

    string getTypePointStr() {
        // FIXME:
        //if(type->isPtr()) {
        //    return type->getStr() + " " + getStr();
        //}
        return type->getStr() + "* " + getStr();
    };
};

using ValuePtr = Value*;

using VariantNativeType = std::variant<bool, int8_t, int, long long, float>;

enum class ConstID { Bool, Int, Float, Unknown };

// const 使用单例模式, 不要主动delete const
struct Const : Value {

    static unordered_map<int, unique_ptr<Value>> constIntMap;
    static unordered_map<float, unique_ptr<Value>> constFloatMap;
    static unordered_map<bool, unique_ptr<Value>> constBoolMap;
    static unordered_map<int8_t, unique_ptr<Value>> constInt8Map;
    static unordered_map<long long, unique_ptr<Value>> constInt64Map;

    union {
        long long intVal;  // int
        float floatVal;    // float
        bool boolVal;
    };

    ConstID id;

    Const(Const&&) = default;
    Const(const Const&) = default;
    Const& operator=(const Const&) = default;
    Const& operator=(Const&&) = default;

    Const(bool boolVal) : Value{ Type::getBool(), "", true }, boolVal{ boolVal }, id(ConstID::Bool) {};
    Const(int intVal) : Value{ Type::getInt(), "", true }, intVal{ intVal }, id(ConstID::Int) {};
    Const(long long intVal) : Value{ Type::getInt(), "", true }, intVal{ intVal }, id(ConstID::Int) {};
    Const(float floatVal) : Value{ Type::getFloat(), "", true }, floatVal{ floatVal }, id(ConstID::Float) {};

    Const(TypePtr type, bool boolVal) : Value{ type, "", true }, boolVal{ boolVal } {
        if(type->isBool()) {
            id = ConstID::Bool;
        }
        if(type->isInt()) {
            id = ConstID::Int;
        }
        if(type->isFloat()) {
            id = ConstID::Float;
        }
        // assert(false);
    };
    Const(TypePtr type, int intVal) : Value{ type, "", true }, intVal{ intVal } {
        if(type->isBool()) {
            id = ConstID::Bool;
        }
        if(type->isInt()) {
            id = ConstID::Int;
        }
        if(type->isFloat()) {
            id = ConstID::Float;
        }
        // assert(false);
    };
    Const(TypePtr type, long long intVal) : Value{ type, "", true }, intVal{ intVal } {
        if(type->isBool()) {
            id = ConstID::Bool;
        }
        if(type->isInt()) {
            id = ConstID::Int;
        }
        if(type->isFloat()) {
            id = ConstID::Float;
        }
        // assert(false);
    };
    Const(TypePtr type, float floatVal) : Value{ type, "", true }, floatVal{ floatVal } {
        if(type->isBool()) {
            id = ConstID::Bool;
        }
        if(type->isInt()) {
            id = ConstID::Int;
        }
        if(type->isFloat()) {
            id = ConstID::Float;
        }
        // assert(false);
    };
    Const(TypePtr type, string value) : Value{ type, "", true } {
        try {
            if(type->isFloat()) {
                floatVal = std::stod(value);  // NOLINT
            } else {
                int scale = 10;
                if(value.size() > 2 && value.substr(0, 2) == "0x")
                    scale = 16;
                else if(value.size() > 1 && value[0] == '0')
                    scale = 8;
                intVal = stoll(value, 0, scale);  // NOLINT
            }
        } catch(const std::exception& e) {
            std::cerr << "const init error(" << intVal << ", " << floatVal << ")" << '\n';  // NOLINT
        }
    };

    string getStr() override;  //{
                               // if(id == ConstID::Bool) {
                               //    return boolVal ? "true" : "false";
                               //}
                               // if(id == ConstID::Int) {
                               //    return std::to_string(intVal);
                               //}
                               // if(id == ConstID::Float) {
                               //    return std::to_string(floatVal);
                               //}
                               // return "unknown";
    //}

    [[nodiscard]] ValueID valueId() const override {
        return ValueID::Const;
    }
    [[nodiscard]] ConstID constId() const {
        return id;
    }

    void dump(std::ostream& out = std::cout) override {
        out << getStr();
    };

    bool isStrongEqual(Const* rhs);
    bool isWeakEqual(Const* rhs);

    // void setValue(double value) {
    //     switch (id)
    //     {
    //     case ConstID::Bool:
    //         boolVal = value;
    //         break;
    //     case ConstID::Int:
    //         intVal = value;
    //         break;
    //     case ConstID::Float:
    //         floatVal = value;
    //         break;
    //     default:
    //         assert(false&&"Const setValue error");
    //     }
    // }

    double getValue() {
        switch (id)
        {
        case ConstID::Bool:
            return boolVal;
        case ConstID::Int:
            return intVal;
        case ConstID::Float:
            return floatVal;
        default:
            assert(false&&"Const getValue error"); 
        }
    }

 

public:
    template <typename T>
    static ValuePtr getConst(TypePtr type, T val);

    static ValuePtr getConst(TypePtr type, string val, string name = "");
};

// template <typename T>
// ValuePtr Const::getConst(TypePtr type, T val) {
//     if(type == Type::getBool())
//         return ValuePtr(new Const(type, (bool)val));
//     if(type == Type::getFloat())
//         return ValuePtr(new Const(type, (float)val));
//     if(type == Type::getInt8())
//         return ValuePtr(new Const(type, (int8_t)val));
//     if(type == Type::getInt64())
//         return ValuePtr(new Const(type, (long long)val));
//     if(type == Type::getInt())
//         return ValuePtr(new Const(type, (int)val));
//
//     assert(false && "getConst error\n");
// }

// 对于void 也请使用提供的接口获取全局唯一的void 而不是自己构建
struct Void : Value {
    static Value* value;
    static Value* get();
    Void() : Value{ Type::getVoid() } {};
    [[nodiscard]] ValueID valueId() const override {
        return ValueID::Void;
    }
    // dump
    void dump(std::ostream& out = std::cout) override {
        out << "void";
    };

    string getStr() override {
        return "void";
    };
};

// NOLINTEND