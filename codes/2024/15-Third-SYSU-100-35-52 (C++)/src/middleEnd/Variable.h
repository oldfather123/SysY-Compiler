#pragma once

#include <cassert>
#include <utility>

#include "Instruction.h"
#include "Label.h"

// TODO: 希望 variable 是 一个 abstract class  有机会可以 继续抽象
struct Variable : User {
    bool isGlobal;
    Variable(TypePtr type, const string& name, bool isGlobal = false, bool isConst = false)
        : User{ type, name, isConst }, isGlobal{ isGlobal } {};

    ~Variable() override = default;
    void dump(std::ostream& out = std::cout) override {};
    [[nodiscard]] ValueID valueId() const override {
        return ValueID::Variable;
    }
    string getStr() override;



    virtual void printHelper(std::ostream& out = std::cout) { // NOLINT
        assert(false);
    };
    virtual bool zero() {
        assert(false);
    };
    static Variable* copy(Variable* var);
};

using VariablePtr = Variable*;

struct Int : Variable {
    long long intVal;
    Int(const string& name, bool isGlobal = false, bool isConst = false)
        : Variable{ Type::getInt(), name, isGlobal, isConst }, intVal{ 0 } {};
    Int(const string& name, bool isGlobal, bool isConst, long long intVal)
        : Variable{ Type::getInt(), name, isGlobal, isConst }, intVal{ intVal } {};
    Int(string name, bool isGlobal, bool isConst, Value* value);
    void dump(std::ostream& out = std::cout) override;
    void printHelper(std::ostream& out = std::cout) override;
    bool zero() override;
};

struct Float : Variable {
    float floatVal;
    Float(const string& name, bool isGlobal = false, bool isConst = false)
        : Variable{ Type::getFloat(), name, isGlobal, isConst }, floatVal{ 0 } {};
    Float(string name, bool isGlobal, bool isConst, ValuePtr value);
    Float(const string& name, bool isGlobal, bool isConst, float value)
        : Variable{ Type::getFloat(), name, isGlobal, isConst }, floatVal{ value } {};
    void dump(std::ostream& out = std::cout) override;
    void printHelper(std::ostream& out = std::cout) override;
    bool zero() override;
};

struct Arr : Variable {
    vector<VariablePtr> inner;  // 初始化的数据
    Arr(const string& name, bool isGlobal, bool isConst, TypePtr type) : Variable{ type, name, isGlobal, isConst } {};
    TypePtr getElementType() {
        return dynamic_cast<ArrType*>(type)->inner;
    }
    int getElementLength() {
        return dynamic_cast<ArrType*>(type)->length;
    }

    bool push(VariablePtr variable);
    void fill();

    void dump(std::ostream& out = std::cout) override;
    void printHelper(std::ostream& out = std::cout) override;
    bool zero() override;
};

struct Ptr : Variable {
    Ptr(const string& name, bool isGlobal, bool isConst, TypePtr type) : Variable{ type, name, isGlobal, isConst } {};
    void dump(std::ostream& out = std::cout) override;
    void printHelper(std::ostream& out = std::cout) override;
    bool zero() override;
};
