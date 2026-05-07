//
// Created by q1w2e3r4 on 23-6-27.
//

#include "Scope.h"
#include <iostream>
using namespace std;

//void localZeroInitializer(Builder *builder,ValueRef *ref,Type* type);
void localArrayInit(Builder *builder,ValueRef *ref,Type* type,std::any values);

Scope::Scope(Scope *super, int sid){
    this->super = super;
    vars = unordered_map<string,Symbol*>();
    this->sid = sid;
}

Symbol* Scope::getVar(const string& varName){
    Scope *scope = this;
    unordered_map<string,Symbol*> table = scope->vars;
    auto it = table.find(varName);
    while(it == table.end()){
        scope = scope->super;
        if(scope == nullptr){
            cout << "Var not exists" << endl;
            throw exception();
        }
        table = scope->vars;
        it = table.find(varName);
    }

    return (*it).second;
}

Symbol* Scope::addVar(Builder *builder, Type* type,string& name,bool is_const,ValueRef *initVal){ // if not inited, initVal == nullptr
    if(super == nullptr){ // global var
        Symbol * symbol = AddGlobal(builder,type,"@" + name,initVal,is_const);
        vars[name] = symbol;
        return symbol;
    }
    else{  // local
        Symbol * symbol = BuildAlloc(builder,type,"%" + name,is_const);
        vars[name] = symbol;

        if(initVal != nullptr){
            if(symbol->symbolType->type != ARRAYTYPE) {
                BuildStore(builder, symbol, initVal);
            }
            else{
                // 数组需要特殊初始化
                IRInstruction* memset = new MemsetInstruction(symbol,((Symbol*)symbol)->symbolType->getSize(),0);
                builder->AppendCode(memset);
                localArrayInit(builder,symbol,type,((Array_Const *)initVal)->values);
            }
            if(is_const) {
                symbol->constVal = initVal;
            }
        }

        return symbol;
    }
}

//void localZeroInitializer(Builder *builder,ValueRef *ref,Type* type){
//    if(type->type != ARRAYTYPE){
//        assert(ref->type == Ptr); // it means we can use BuildStore;
//        ValueRef* initVal;
//        if(type->type == INT32TYPE) initVal = new Int_Const(0);
//        else if(type->type == FLOATTYPE) initVal = new Float_Const(0);
//        else throw exception();
//
//        BuildStore(builder,(Pointer*)ref,initVal);
//    }
//    else{
//        int ele_cnt = ((ArrayType*)type)-> ele_cnt;
//        for(int i=0;i<ele_cnt;i++){
//            localZeroInitializer(builder,IndexOfArray(builder, ref, new Int_Const(i)), ((ArrayType*)type)->elementType);
//        }
//    }
//}

void localArrayInit(Builder *builder,ValueRef *ref,Type* type,std::any values){
    if(values.type() == typeid(vector<std::any>)){
        assert(type->type == ARRAYTYPE);

        auto v = any_cast<vector<any> >(values);
        for(int i=0;i<v.size();i++){
            localArrayInit(builder,IndexOfArray(builder, ref, new Int_Const(i)), ((ArrayType*)type)->elementType, v[i]);
        }
//        for(int i=v.size();i<ele_cnt;i++){
//            localZeroInitializer(builder,IndexOfArray(builder, ref, new Int_Const(i)), ((ArrayType*)type)->elementType);
//        }
    }
    else{
        auto initVal = any_cast<ValueRef*> (values);
        assert(ref->type == Ptr); // it means we can use BuildStore;
        BuildStore(builder,(Pointer*)ref,initVal);
    }
}