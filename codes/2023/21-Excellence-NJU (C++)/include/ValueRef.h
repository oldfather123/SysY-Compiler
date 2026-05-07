//
// Created by q1w2e3r4 on 23-6-28.
//


#ifndef COMPILER_VALUEREF_H
#define COMPILER_VALUEREF_H

#include "antlr4-runtime.h"
#include <vector>
#include <any>
#include "Utils.h"
#include "Type.h"
#include "SysYParser.h"

class Instruction;
enum RefType{IntConst,FloatConst,ArrConst,BoolConst,IntVar,FloatVar,BoolVar,SYMBOL,Ptr,Arr,Block,Undefined};

class ValueRef {
public:
    RefType type;
    std::string name;

    vector<Instruction*> def;
    vector<Instruction*> use;

    virtual std::string get_Ref() = 0;
    virtual std::string get_TypeName() = 0;
    virtual Type* getType(){ throw exception(); }
    virtual int getElementSize(){ throw exception(); }
    virtual ValueRef* getElementPtr(){ throw exception(); } // only array/pointer/symbol can use it
    Instruction* get_def();
};

class Int_Const : public ValueRef{
public:
    int value;

    Int_Const(int val){ this->type = IntConst; this->value = val;}

    std::string get_Ref() override;
    std::string get_TypeName() override { return "i32"; }
};


class Float_Const : public ValueRef{
public:
    double value;

    Float_Const(double val){ this->type = FloatConst; this->value = val;}

    std::string get_Ref() override;
    std::string get_TypeName() override { return "float"; }
};


class Array_Const : public ValueRef{
public:
    // 这个类创建出来纯粹是方便为const数组赋值
    std::vector<std::any> values;

    Array_Const(std::vector<std::any>& v);
    std::string get_Ref() override { return ""; }
    std::string get_TypeName() override { return ""; }
};

class Bool_Const : public ValueRef{
public:
    bool value;

    Bool_Const(bool val){ this->type = BoolConst; this->value = val;}

    std::string get_Ref() override;
    std::string get_TypeName() override { return ""; }
};

class Int_Var : public ValueRef{
public:
    Int_Var(){ this->type = IntVar; this->name = Utils::getNewTamp();}
    Int_Var(const char *name){ this->type = IntVar; this->name = Utils::getNewLabel(name); }

    std::string get_Ref() override;
    std::string get_TypeName() override { return "i32"; }
};


class Float_Var : public ValueRef{
public:
    Float_Var(){ this->type = FloatVar; this->name = Utils::getNewTamp();}
    Float_Var(const char *name){ this->type = FloatVar; this->name = Utils::getNewLabel(name);}

    std::string get_Ref() override;
    std::string get_TypeName() override { return "float"; }
};

class Bool_Var : public ValueRef{
public:
    Bool_Var(){ this->type = BoolVar; this->name = Utils::getNewTamp();}

    std::string get_Ref() override;
    std::string get_TypeName() override { return "i1"; }
};

class Pointer : public ValueRef{
public:
    PointerType* pointerType;

    Pointer(PointerType *pointerType);
    Pointer(ValueRef *symbol); // 实际上是Pointer* (只是由于依赖关系没法写)，此构造函数用于将Symbol* 转化为Pointer *以用于Build/Store，而不创建新的临时变量名

    int getElementSize() override { return pointerType->elementType->getSize(); }
    std::string get_Ref() override;

    ValueRef* getElementPtr() override;

    ValueRef* getElement(); // Load()与getElement的区别在于，前者用于GEP,后者用于BuildLoad/store(理论上你不会用到这个)

    std::string get_TypeName() override { return pointerType->getTypeName(); }

    Type* getType() override { return pointerType; }
};

class Symbol : public ValueRef{
public:
    Type * symbolType;
    bool is_const;
    bool is_global;
    ValueRef* constVal; // used only if it is const( so that constVal = firstVal)

    //Symbol应该不会调用get_Ref和get_Type
    Symbol(Type* type, const string &name, bool is_const);

    int getElementSize() override;

    std::string get_Ref() override;

    ValueRef* getElementPtr() override; // 为数组Symbol准备的

    std::string get_TypeName() override { return symbolType->getTypeName(); }

    Type* getType() override { return symbolType; }
};

class Array : public ValueRef{
public:
    ArrayType* arrayType;
    std::vector<Pointer*> linked_ptr; //与本array同名，且指向相同位置的ptr(实际上他们就是一个东西，只不过为了传参不得不变为ptr)

    Array(ArrayType *arrayType);

    int getElementSize() override { return arrayType->elementType->getSize(); }

    std::string get_Ref() override;

    ValueRef* getElementPtr() override;

    std::string get_TypeName() override { return arrayType->getTypeName(); }

    Type* getType() override { return arrayType; }
};

Pointer* asPointer(ArrayType * arrType);
// 在传入数组作为参数时，实际上是传入一个指针，因此需将Array类型转化为Pointer类型(注：这步转化不会在IR上有体现，只是方便打印信息)
// 只有数组类型能够转化为指针，因此可以使用这个函数的ref要么为symbol（且包含类型必须为数组），要么为Array

class Undef : public ValueRef{
public:
    string get_Ref() override;
    string get_TypeName() override;
    Undef();
};

#endif //COMPILER_VALUEREF_H
