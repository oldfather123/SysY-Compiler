//
// Created by q1w2e3r4 on 23-7-10.
//
#include "ValueRef.h"
#include "Builder.h"

#ifndef COMPILER_LLVM_H
#define COMPILER_LLVM_H

// Binary op
ValueRef* BuildAdd(Builder* builder, ValueRef* lhs, ValueRef* rhs);
ValueRef* BuildSub(Builder* builder, ValueRef* lhs, ValueRef* rhs);
ValueRef* BuildMul(Builder* builder, ValueRef* lhs, ValueRef* rhs);
ValueRef* BuildDiv(Builder* builder, ValueRef* lhs, ValueRef* rhs);
ValueRef* BuildMod(Builder* builder, ValueRef* lhs, ValueRef* rhs);

//single op
ValueRef* BuildNeg(Builder* builder, ValueRef* ref);
ValueRef* BuildNot(Builder* builder, ValueRef* ref);
ValueRef* BuildZext(Builder* builder, ValueRef* ref);
ValueRef* BuildSitofp(Builder* builder, ValueRef* ref); // int -> float
ValueRef* BuildFPTrunc(Builder* builder, ValueRef* ref); // float -> int

// 变量声明/存取
Symbol* BuildAlloc(Builder* builder,Type* type,const std::string& name,bool is_const); // 局部变量
Symbol* AddGlobal(Builder* builder,Type* type,const std::string& name,ValueRef *initVal,bool is_const); // 全局变量
ValueRef* BuildLoad(Builder* builder,Pointer *ptr); // load ptr指向的位置(用于数组load)
ValueRef* BuildLoad(Builder* builder,Symbol *symbol); // load symbol(符号load)
void BuildStore(Builder* builder,Pointer* ptr,ValueRef* ref); // save ref -> ptr指向的位置(数组用)
void BuildStore(Builder* builder,Symbol* symbol,ValueRef* ref); // save ref -> symbol(符号用)

ValueRef* BuildAnd(Builder* builder, ValueRef* lhs, ValueRef* rhs);
ValueRef* BuildOr(Builder* builder, ValueRef* lhs, ValueRef* rhs);

// BuildIcmp(虽然下面的都实现了，但请统一调用Cmp接口)
ValueRef* BuildCmp(Builder* builder,ValueRef* lhs, ValueRef* rhs,cmpType type);
//ValueRef* BuildLT(Builder* builder, ValueRef* lhs, ValueRef* rhs);
//ValueRef* BuildLE(Builder* builder, ValueRef* lhs, ValueRef* rhs);
//ValueRef* BuildEquals(Builder* builder, ValueRef* lhs, ValueRef* rhs);
//ValueRef* BuildUnequal(Builder* builder, ValueRef* lhs, ValueRef* rhs);

ValueRef* BuildCall(Builder* builder,Function *func,std::vector<ValueRef *>& args);
void BuildRet(Builder* builder,ValueRef* ret);

//Branch
void BuildBr(Builder* builder,BasicBlock* block);
void BuildCondBr(Builder* builder,ValueRef* cond,BasicBlock* ifTrue,BasicBlock* ifFalse);

//Array
ValueRef* BuildGEP(Builder* builder,ValueRef* arr,std::vector<ValueRef *>& idxs);
ValueRef *IndexOfArray(Builder* builder,ValueRef *ref,ValueRef * index);


#endif //COMPILER_LLVM_H
