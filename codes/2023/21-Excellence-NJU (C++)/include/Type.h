//
// Created by q1w2e3r4 on 23-6-30.
//

#ifndef COMPILER_TYPE_H
#define COMPILER_TYPE_H

#include "antlr4-runtime.h"
#include <map>
#include <vector>

using namespace std;

enum Type_Enum{VOIDTYPE,INT32TYPE,FLOATTYPE,ARRAYTYPE,FUNCTYPE,POINTERTYPE};

class Type {
public:
    Type_Enum type;
    static map<Type_Enum,string> Type_Names;
    int size = 4;

    Type(Type_Enum type);

    virtual std::string getTypeName();
    virtual int getDimension();
    int getSize(){ return size; }
};

class FuncType : public Type{
public:
    vector<Type*> arguments;
    Type* retType;

    FuncType(Type *retType, vector<Type *>& arguments);
};

class ArrayType: public Type{
public:
    Type* elementType; // 这个数组内存储的是什么类型的数据
    int ele_cnt;
    int dimension;

    ArrayType(Type* elementType,int ele_cnt);

    int getDimension() override { return dimension; }

    std::string getTypeName() override;
};

class PointerType: public Type{
public:
    Type * elementType;
    int dimension;

    PointerType(Type* elementType);

    int getDimension() override { return dimension; }

    std::string getTypeName() override;
};

#endif //COMPILER_TYPE_H
