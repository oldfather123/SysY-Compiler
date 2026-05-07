#pragma once

#include <vector>
#include <cassert>
#include <cmath>
#include <cinttypes>
#include "Type.h"

using std::vector;

//每个Value的use类组织成一个双向链表，删除时间复杂度为O(1)
struct Value;
struct Instruction;

struct Use {
    Use *prev = nullptr, *next = nullptr;
    Value *val;
    Instruction *user;
    bool isDead = false;
    void rmUse();
    void useDead() {
        isDead = true; 
    }
};



Use *newUse(Value *value, Instruction *user);
struct Value
{
    TypePtr type;
    string name;
    bool isConst=false;
    bool isReg=false;
    bool isVoid=false;
    Value(TypePtr type, string name, bool isConst, bool isReg, bool isVoid) : type{type}, name{name}, isConst{isConst}, isReg{isReg}, isVoid{isVoid} {}
    virtual string getStr() = 0;
    virtual string myTypeStr() { return type->getStr(); };
    virtual string getTypeStr() { return type->getStr() + " " + getStr(); };
    virtual string getPointStr() { return type->getStr() + "* " + getStr(); };

    Use* useHead=nullptr; //双向链表头
    
    //记录该reg所代表的指令
    Instruction* I = nullptr;

    void addUse(Use * u){
        u->prev = nullptr;
        if(useHead){
            useHead->prev = u;
        }
        u->next = useHead;
        useHead = u;
    }
};
typedef shared_ptr<Value> ValuePtr;


struct Reg : Value
{
    static ValuePtr getReg(TypePtr type, int id) { return ValuePtr(new Reg(type, id)); }
    //暂时加个reg，不然跑不了
    Reg(TypePtr type, int id) : Value{type, "reg" + to_string(id), false, true, false} {}
    Reg(TypePtr type, string name) : Value{type, name, false, true, false} {}
    virtual string getStr() override { return "%" + name; }
};
typedef shared_ptr<Reg> RegPtr;

struct Const : Value
{
    long long intVal;     // int
    float floatVal; // float
    bool boolVal;
    Const(bool boolVal, string name = "") : Value{Type::getBool(), name, true, false, false}, boolVal{boolVal} {};
    Const(int intVal, string name = "") : Value{Type::getInt(), name, true, false, false}, intVal{intVal} {};
    Const(long long intVal, string name = "") : Value{Type::getInt(), name, true, false, false}, intVal{intVal} {};
    Const(float floatVal, string name = "") : Value{Type::getFloat(), name, true, false, false}, floatVal{floatVal} {};
    Const(TypePtr type, int intVal, string name = "") : Value{type, name, true, false, false}, intVal{intVal} {};
    Const(TypePtr type, long long intVal, string name = "") : Value{type, name, true, false, false}, intVal{intVal} {};
    Const(TypePtr type, float floatVal, string name = "") : Value{type, name, true, false, false}, floatVal{floatVal} {};
    Const(TypePtr type, string value, string name = "") : Value{type, name, true, false, false}
    {
        try
        {
            if (type->isFloat())
            {
                floatVal = std::stod(value);
            }
            else
            {
                int scale = 10;
                if (value.size() > 2 && value.substr(0, 2) == "0x")
                    scale = 16;
                else if (value.size() > 1 && value[0] == '0')
                    scale = 8;
                intVal = stoll(value, 0, scale);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "const init error(" << intVal << ", " << floatVal << ")" << '\n';
        }
    };
    virtual string getStr() override;
};

struct Void : Value
{
    static ValuePtr value;
    static ValuePtr get() { return value; }
    Void() : Value{Type::getVoid(), "", false, false, true} {};
    virtual string getStr() override { return ""; };
};

struct Variable : Value
{
    bool isGlobal;
    Variable(TypePtr type, string name, bool isGlobal, bool isConst) : Value{type, name, isConst, false, false}, isGlobal{isGlobal} {};
    virtual string getStr() override;

    virtual void print() = 0;
    virtual void printHelper() = 0;
    virtual bool zero() = 0;
    static shared_ptr<Variable> copy(shared_ptr<Variable> var);
};
typedef shared_ptr<Variable> VariablePtr;

struct Int : Variable
{
    long long intVal;
    Int(string name, bool isGlobal, bool isConst) : Variable{Type::getInt(), name, isGlobal, isConst}, intVal{0} {};
    Int(string name, bool isGlobal, bool isConst, long long intVal) : Variable{Type::getInt(), name, isGlobal, isConst}, intVal{intVal} {};
    Int(string name, bool isGlobal, bool isConst, ValuePtr value);
    virtual void print() override;
    virtual void printHelper() override;
    virtual bool zero() override;
};

struct Float : Variable
{
    float floatVal;
    Float(string name, bool isGlobal, bool isConst) : Variable{Type::getFloat(), name, isGlobal, isConst}, floatVal{0} {};
    Float(string name, bool isGlobal, bool isConst, ValuePtr value);
    Float(string name, bool isGlobal, bool isConst, float value) : Variable{Type::getFloat(), name, isGlobal, isConst}, floatVal{value} {};
    virtual void print() override;
    virtual void printHelper() override;
    virtual bool zero() override;
};

struct Arr : Variable
{
    vector<VariablePtr> inner; // 初始化的数据
    Arr(string name, bool isGlobal, bool isConst, TypePtr type) : Variable{type, name, isGlobal, isConst} {};
    TypePtr getElementType() { return dynamic_cast<ArrType *>(type.get())->inner; }
    int getElementLength() { return dynamic_cast<ArrType *>(type.get())->length; }

    bool push(VariablePtr Variable);
    void fill();

    virtual void print() override;
    virtual void printHelper() override;
    virtual bool zero() override;
};

struct Ptr : Variable
{
    Ptr(string name, bool isGlobal, bool isConst, TypePtr type) : Variable{type, name, isGlobal, isConst} {};
    virtual void print() override;
    virtual void printHelper() override;
    virtual bool zero() override;
};

bool rmInstructionUse(shared_ptr<Instruction> I,ValuePtr v);
bool rmInstructionUse(Instruction* I,ValuePtr v);