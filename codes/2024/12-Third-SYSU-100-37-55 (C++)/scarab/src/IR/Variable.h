#pragma once

#include <vector>
#include <cassert>
#include <cmath>
#include <cinttypes>
#include <iostream>
#include <unordered_map>
#include "Type.h"

using std::unordered_map;
using std::vector;
using namespace std;

struct Value;
struct Instruction;

struct Use {
    Use *pred = nullptr, *next = nullptr;
    Value *val;
    Instruction *user;
    Value *userValue;
    void rmUse();

    Use(){}
    Use(Value* value, Instruction* usr, Value* usrVal=nullptr) : val{value}, user{usr}, userValue{usrVal} {}
};

enum BinaryInstructionOps
{
    Add,
    Mul,
    Sub,
    Div,
    Rem,
    Xor,
    Shl,
    AShr,
    FAdd,
    FSub,
    FMul,
    FDiv
};

struct Value
{
    TypePtr type;
    string name;
    bool isConst=false;
    bool isReg=false;
    bool isVoid=false;

    Value(TypePtr type, string name, bool isConst, bool isReg, bool isVoid) : type{type}, name{name}, isConst{isConst}, isReg{isReg}, isVoid{isVoid} {}
    Value(string name, bool isConst, bool isReg, bool isVoid) : name{name}, isConst{isConst}, isReg{isReg}, isVoid{isVoid} {};
    virtual string toString(){return "";}
    virtual string returnTypeToString() { return type->toString() + " " + toString(); };
    virtual string pointerTypeToString() { return type->toString() + "* " + toString(); };

    Use* useHead=nullptr; 
    int useCount = 0;
    int getUseCount() { return useCount; }
    
    Instruction *I = nullptr;

    void addUseNode(Use * use){
        use->pred = nullptr;
        if(useHead != nullptr){
            useHead->pred = use;
        }
        use->next = useHead;
        useHead = use;
        useCount ++;
    }

    bool hasSingleUse(){
        return useCount == 1; 
    }
    bool hasNoUse(){
        return useCount == 0;
    }
};

typedef shared_ptr<Value> ValuePtr;

Use *newUse(Value *value, Instruction *user);
Use *newUse(Value *value, Instruction *user, Value *userValue);
Use *findUse(Value *value, Value *userValue);
Use *findUse(Value *value, Instruction *user);


struct Reg : Value
{
    static ValuePtr createRegister(TypePtr type, int id) { return ValuePtr(new Reg(type, id)); }

    Reg(TypePtr type, int id) : Value{type, "reg" + to_string(id), false, true, false} {}
    Reg(TypePtr type, string name) : Value{type, name, false, true, false} {}
    virtual string toString() override { return "%" + name; }
};
typedef shared_ptr<Reg> RegPtr;


struct Const : Value
{
    static unordered_map<int8_t, ValuePtr> Int8Map;
    static unordered_map<int,ValuePtr> IntMap;
    static unordered_map<long long,ValuePtr> longlongMap;
    static unordered_map<float, ValuePtr> FloatMap;
    static unordered_map<bool, ValuePtr> BoolMap;

    long long intVal;
    float floatVal;
    bool boolVal;
    template <typename C>
    Const(C val,string constName = "");

    Const(TypePtr type, bool boolVal, string name = "") : Value{type, name, true, false, false}, boolVal{boolVal} {};
    Const(TypePtr type, int intVal, string name = "") : Value{type, name, true, false, false}, intVal{intVal} {};
    Const(TypePtr type, long long intVal, string name = "") : Value{type, name, true, false, false}, intVal{intVal} {};
    Const(TypePtr type, float floatVal, string name = "") : Value{type, name, true, false, false}, floatVal{floatVal} {};

    Const(TypePtr constType, string val, string constName = "") : Value{constType, constName, true, false, false}
    {
        try {
            if (type->isFloat()) {
                floatVal = std::stod(val);
            }
            else {
                int scale = 10;
                if (val.size() > 2 && val.substr(0, 2) == "0x")
                    scale = 16;
                else if (val.size() > 1 && val[0] == '0')
                    scale = 8;
                intVal = stoll(val, 0, scale);
            }
        }
        catch (const std::exception &e) {
            std::cerr << "const init error(" << intVal << ", " << floatVal << ")" << '\n';
        }
    };

    
    virtual string toString() override;
    public:

    static ValuePtr createConstant(TypePtr type,string val,string name = "");

    template <typename T>
    static ValuePtr createConstant(TypePtr type,T val,string name = "");
    static ValuePtr createZero(TypePtr type);
    static ValuePtr createOne(TypePtr type);
};


template <typename C>
Const::Const(C val,string constName): Value{constName, true, false, false}{
    if (std::is_same<C, bool>::value) {
        type = Type::getBool();

        boolVal = bool(val);
    } else if (std::is_same<C, int>::value) {
        type = Type::getInt();

        intVal = int(val);
    } else if (std::is_same<C, long long>::value) {
        type = Type::getInt();

        intVal = int(val);
    } else if (std::is_same<C, float>::value) {
        type = Type::getFloat();

        floatVal = float(val);
    }
}

template <typename T>
ValuePtr Const::createConstant(TypePtr type,T val,string name){
    switch (type->ID){
    case IntID:{
        if(IntMap.find(int(val))!=IntMap.end()){
            return IntMap[int(val)];
        }
        ValuePtr ret = ValuePtr(new Const(type, int(val), name));
        return IntMap[int(val)] = ret;
    }
    case Int8ID:{
        if(Int8Map.find(int8_t(val))!=Int8Map.end()){
            return Int8Map[int8_t(val)];
        }
        ValuePtr ret = ValuePtr(new Const(type, int8_t(val), name));
        return Int8Map[int8_t(val)] = ret;
    }
    case Int64ID:{
        if(longlongMap.find((long long)(val))!=longlongMap.end()){
            return longlongMap[(long long)(val)];
        }
        ValuePtr ret = ValuePtr(new Const(type, (long long)(val), name));
        return longlongMap[(long long)(val)] = ret;
    }
    case FLoatID:{
        if(FloatMap.find(float(val))!=FloatMap.end()){
            return FloatMap[float(val)];
        }
        ValuePtr ret = ValuePtr(new Const(type, float(val), name));
        return FloatMap[float(val)] = ret;
    }
    case BoolID:{
        if(BoolMap.find(bool(val))!=BoolMap.end()){
            return BoolMap[bool(val)];
        }
        ValuePtr ret = ValuePtr(new Const(type, bool(val), name));
        return BoolMap[bool(val)] = ret;
    }
    default:
        return nullptr;
    }
}


struct Void : Value
{
    static ValuePtr value;
    static ValuePtr get() { return value; }
    Void() : Value{Type::getVoid(), "", false, false, true} {};
    virtual string toString() override { return ""; };
};

struct Variable : Value
{
    bool isGlobal;
    Variable(TypePtr type, string name, bool isGlobal, bool isConst) : Value{type, name, isConst, false, false}, isGlobal{isGlobal} {};
    virtual string toString() override;

    virtual void print() = 0;
    virtual bool zero() = 0;
    virtual void printHelper() = 0;
    static shared_ptr<Variable> copy(shared_ptr<Variable> var);
};
typedef shared_ptr<Variable> VariablePtr;

struct Int : Variable
{
    long long intVal;
    Int(string name, bool isGlobal, bool isConst) : Variable{Type::getInt(), name, isGlobal, isConst}, intVal{0} {};
    Int(string name, bool isGlobal, bool isConst, long long intVal) : Variable{Type::getInt(), name, isGlobal, isConst}, intVal{intVal} {};
    Int(string name, bool isGlobal, bool isConst, ValuePtr value);
    Int(TypePtr type, string name, bool isGlobal, bool isConst) : Variable{type, name, isGlobal, isConst}, intVal{0} {};

    virtual void print() override;
    virtual bool zero() override;
    virtual void printHelper() override;
};

struct Float : Variable
{
    float floatVal;
    Float(string name, bool isGlobal, bool isConst) : Variable{Type::getFloat(), name, isGlobal, isConst}, floatVal{0} {};
    Float(string name, bool isGlobal, bool isConst, ValuePtr value);
    Float(string name, bool isGlobal, bool isConst, float value) : Variable{Type::getFloat(), name, isGlobal, isConst}, floatVal{value} {};
    virtual void print() override;
    virtual bool zero() override;
    virtual void printHelper() override;
};

struct Arr : Variable
{
    vector<VariablePtr> inner;
    Arr(string name, bool isGlobal, bool isConst, TypePtr type) : Variable{type, name, isGlobal, isConst} {};
    TypePtr getElementType() { return dynamic_cast<ArrType *>(type.get())->inner; }
    int getElementLength() { return dynamic_cast<ArrType *>(type.get())->size; }
    virtual string returnTypeToString() override { return type->toString() + "* " + toString(); };
    
    bool push(VariablePtr Variable);
    void fill();

    virtual void print() override;
    virtual bool zero() override;
    virtual void printHelper() override;
};

struct Ptr : Variable
{
    Ptr(string name, bool isGlobal, bool isConst, TypePtr type) : Variable{type, name, isGlobal, isConst} {};
    virtual void print() override;
    virtual bool zero() override;
    virtual void printHelper() override;
};

bool rmInsUse(Instruction* I,ValuePtr v);

ValuePtr createNewConst(ValuePtr val1,ValuePtr val2, unsigned OpCode);