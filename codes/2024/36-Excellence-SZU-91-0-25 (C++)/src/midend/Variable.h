#pragma once

#include "../utility/System.h"
#include "../utility/Type.h"
#include "../midend/Value.h"

#define VAR_SIZE 30000

class Variable
{
public:
    bool is_const;
    StoreType store_type;
    DataType data_type;

    string name;
    int number;

    vector<int> dimensions;
    int length;
    
    vector<ValueConstant> initial;
    
    bool is_temp;
    bool is_free;

    Variable(){}
    Variable(bool is_const,StoreType store_type,DataType data_type,
             string name,int number,vector<int> dimensions,bool is_temp):
            is_const(is_const),store_type(store_type),data_type(data_type),
            name(name),number(number),dimensions(dimensions),is_temp(is_temp)
            {
                if(dimensions.empty())length=0;
                else
                {
                    length=1;
                    for(int& dimension:dimensions)length*=dimension;
                }

                is_free=false;
            }
    
    void Initialize();

    bool IsNormalVar();
    bool IsArrayPointer();
    bool IsNormalPointer();

    string GetStr();
};

