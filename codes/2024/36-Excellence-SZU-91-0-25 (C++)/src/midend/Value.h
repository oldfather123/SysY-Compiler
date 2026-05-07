#pragma once

#include "../utility/System.h"
#include "../utility/Type.h"
#include "../utility/Diagnostor.h"

enum ValueType
{
    VARIABLE,
    CONSTANT
};

class Value
{
public:
    ValueType value_type;
    
    Value(ValueType value_type):value_type(value_type){}
    virtual ~Value()= default;

    bool IsValueConstant();
    bool IsValueVariable();

    virtual string GetStrNoType()=0;
    virtual string GetStrWithType()=0;
};

bool IsEqualValue(shared_ptr<Value>& value1,shared_ptr<Value>& value2);


class ValueVariable:public Value
{
public:
    StoreType store_type;
    DataType data_type;
    int number;
    shared_ptr<Value> index;

    ValueVariable(StoreType store_type,DataType data_type,int number):
        Value(VARIABLE),store_type(store_type),data_type(data_type),number(number){}
    ValueVariable(StoreType store_type,DataType data_type,int number,shared_ptr<Value> index):
        Value(VARIABLE),store_type(store_type),data_type(data_type),number(number),index(index){}

    bool operator==(ValueVariable& other) {
        return this->store_type==other.store_type&&
               this->data_type==other.data_type&&
               this->number==other.number&&
               IsEqualValue(this->index,other.index);
    }

    bool IsNormalVar();
    bool IsArrayElem();
    bool IsPointer();
    
    string GetStrNoType() override;
    string GetStrWithType() override;
};

class ValueConstant:public Value
{
public:
    DataType data_type;

    union{
        int i32;
        float f32;
    }constant_value;

    ValueConstant():Value(CONSTANT){}
    ValueConstant(int value_i32):Value(CONSTANT),data_type(I32){constant_value.i32=value_i32;}
    ValueConstant(float value_f32):Value(CONSTANT),data_type(F32){constant_value.f32=value_f32;}
    ValueConstant(DataType data_type):Value(CONSTANT),data_type(data_type){
        switch(data_type)
        {
            case I32:constant_value.i32=0;break;
            case F32:constant_value.f32=0.0;break;
            default:CompilerError("Invalid data type in ValueConstant()");
        }
    }

    bool operator==(ValueConstant& other) {
        if(this->data_type==I32&&other.data_type==I32)
            return this->constant_value.i32==other.constant_value.i32;
        // else if(this->data_type==F32&&other.data_type==F32)
        //     return this->constant_value.f32==other.constant_value.f32;
        return false;
    }

    void Convert(DataType convert_to);

    string GetStrNoType() override;
    string GetStrWithType() override;

    string GetAsmStr();
};

DataType ValueDataType(shared_ptr<Value>& value);

shared_ptr<ValueConstant> GetValueConstantPtr(shared_ptr<Value>& value);
shared_ptr<ValueVariable> GetValueVariablePtr(shared_ptr<Value>& value);
bool Is1(shared_ptr<Value>& value);
bool Is0(shared_ptr<Value>& value);
