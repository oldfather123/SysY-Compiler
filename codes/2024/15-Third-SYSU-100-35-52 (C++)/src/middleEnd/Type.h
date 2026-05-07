#pragma once
#include "utils.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::vector;

enum class TypeID { Void, Bool, Int, Float, Int8, Int64, Arr, Ptr, Invalid };


// type 是 单例模式，除非是arrType 或者 ptrType 否则 都不该主动 delete
// 所有的 普通类型都应该通过提供的接口获取，或者指针的复制，不应该自己 new
struct Type {
    virtual ~Type() = default;
    TypeID id; 

    //Type() = delete;
    //Type(const Type&) = delete;
    //Type(Type&&) = delete;
    //Type& operator=(const Type&) = delete;
    //Type& operator=(Type&&) = delete;

    Type(TypeID type = TypeID::Void) : id{ type } {};
    // Type 不持有 string 
    Type(std::string_view type);

    [[nodiscard]] TypeID getID() const {
        return id;
    }

    virtual string getStr();
    virtual void dump(std::ostream& out = std::cout);
    // FIXME: Type比较有问题，尽量用id
    virtual bool operator==(Type* ptr) {
        return id == ptr->id;
        // return true;
    }
    virtual bool operator!=(Type* ptr) {
        return id != ptr->id;
        // return true;
    }
    [[nodiscard]] bool isVoid() const {
        return id == TypeID::Void;
    }
    [[nodiscard]] bool isBool() const {
        return id == TypeID::Bool;
    }
    [[nodiscard]] bool isInt() const {
        return id == TypeID::Int || id == TypeID::Int8 || id == TypeID::Int64;
    }
    [[nodiscard]] bool isFloat() const {
        return id == TypeID::Float;
    }
    [[nodiscard]] bool isArr() const {
        return id == TypeID::Arr;
    }
    [[nodiscard]] bool isPtr() const {
        return id == TypeID::Ptr;
    }
    [[nodiscard]] bool is64Bit() {
        return id == TypeID::Arr || id == TypeID::Ptr || id == TypeID::Int64;
    }

    static vector<Type*> types;
    static Type* getVoid() {
        return Type::types[0];
    }
    static Type* getBool() {
        return Type::types[1];
    }
    static Type* getInt() {
        return Type::types[2];
    }
    static Type* getFloat() {
        return Type::types[3];
    }
    static Type* getInt8() {
        return Type::types[4];
    }
    static Type* getInt64() {
        return Type::types[5];
    }

    template <typename T>
    T* as() {
        static_assert(std::is_base_of_v<Type, T>);
        return dynamic_cast<T*>(this);
    }
    template <typename T>
    bool is() {
        static_assert(std::is_base_of_v<Type, T>);
        return dynamic_cast<T*>(this);
    }
    template <typename T>
    static Type* get();
};

using TypePtr = Type*;

struct ArrType : Type {
    TypePtr inner;
    int length;
    ArrType(TypePtr inner, int length) : Type{ TypeID::Arr }, inner{ inner }, length{ length } {};
    string getStr() override;
    void dump(std::ostream& out = std::cout) override;
    bool operator==(Type* ptr) override;
    bool operator!=(Type* ptr) override;
    int getSize();
    int getDepth();
};

struct PtrType : Type {
    TypePtr inner;
    PtrType(TypePtr inner) : Type{ TypeID::Ptr }, inner{ inner } {}
    string getStr() override;
    void dump(std::ostream& out = std::cout) override;
    bool operator==(Type* ptr) override;
    bool operator!=(Type* ptr) override;
};