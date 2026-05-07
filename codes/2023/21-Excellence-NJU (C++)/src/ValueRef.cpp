//
// Created by q1w2e3r4 on 23-6-28.
//

#include "ValueRef.h"

#include "Utils.h"
#include "Builder.h"
#include <sstream>
#include <iomanip>

using namespace std;

PointerType * IntPointer = new PointerType(new Type(INT32TYPE));
PointerType * FloatPointer = new PointerType(new Type(FLOATTYPE));

std::string Int_Const::get_Ref() {
    return to_string(value);
}

std::string Float_Const::get_Ref() {
    long long hex_val = *((long long *)&value);

    std::ostringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(16) << std::hex << hex_val;
    string result_hex = ss.str();

    return result_hex;
}

std::string Bool_Const::get_Ref() {
    return to_string(value);
}

std::string Int_Var::get_Ref() {
    return name;
}

std::string Float_Var::get_Ref() {
    return name;
}

Symbol::Symbol(Type* type, const string &name, bool is_const) {
    this->type = SYMBOL;
    this->symbolType = type;
    this->name = name;
    this->is_const = is_const;
    this->is_global = name[0] == '@';
}

ValueRef *Symbol::getElementPtr() {
    Type* symbolType = this->symbolType;
    Type * eleType;
    if(symbolType->type == POINTERTYPE){
        eleType = ((PointerType*)symbolType)->elementType;
    }else if(symbolType->type == ARRAYTYPE){
        eleType = ((ArrayType*)symbolType)->elementType;
    }else{
        cerr << "this symbol is not array/pointer type" << endl;
        throw exception();
    }
    switch(eleType->type){
        case INT32TYPE: return new Pointer(IntPointer);
        case FLOATTYPE: return new Pointer(FloatPointer);
        case ARRAYTYPE: return new Array((ArrayType*)eleType);
        default: cerr << "this type can't be element" << endl; throw exception();
    }
}

std::string Symbol::get_Ref() {
    return this->name;
}

int Symbol::getElementSize() {
    if(this->symbolType->type == POINTERTYPE){
        return ((PointerType*)symbolType)->elementType->getSize();
    }
    else if(this->symbolType->type == ARRAYTYPE){
        return ((ArrayType*)symbolType)->elementType->getSize();
    }
    else{
        cerr << "this type doesn't have element!";
        throw exception();
    }
}

Pointer::Pointer(PointerType *pointerType) {
    this->name = Utils::getNewTamp();
    this->pointerType = pointerType;
    this->type = Ptr;
}

Pointer::Pointer(ValueRef *ref) {// 此构造函数用于将Array* 转化为Pointer *以用于Build/Store，而不创建新的临时变量名
    this->name = ref->get_Ref();
    assert(ref->type == Arr);
    this->pointerType = new PointerType(((Array *)ref)->arrayType);
    ((Array*)ref) -> linked_ptr.push_back(this);
    this->type = Ptr;
}


ValueRef *Pointer::getElementPtr() { // 除了int和float,其他的都解一层引用
    Type* eleType = this->pointerType->elementType;
    switch(eleType->type){
        case INT32TYPE: return new Pointer(IntPointer);
        case FLOATTYPE: return new Pointer(FloatPointer);
        case ARRAYTYPE: return new Array((ArrayType*)eleType);
        default: cerr << "this type can't be element" << endl; throw exception();
    }
}

std::string Pointer::get_Ref() {
    return this->name;
}

ValueRef *Pointer::getElement() { // 全部解一层引用（包括int和float）
    Type* eleType = this->pointerType->elementType;
    switch(eleType->type){
        case INT32TYPE: return new Int_Var();
        case FLOATTYPE: return new Float_Var();
        case POINTERTYPE: return new Pointer((PointerType*) eleType);
        case ARRAYTYPE: return new Array((ArrayType*) eleType);
        default: cerr << "this type can't be element" << endl; throw exception();
    }
}

Array::Array(ArrayType *arrayType) {
    this->name = Utils::getNewTamp();
    this->type = Arr;
    this->arrayType = arrayType;
}

ValueRef *Array::getElementPtr() {
    Type* eleType = this->arrayType->elementType;
    switch(eleType->type){
        case INT32TYPE: return new Pointer(IntPointer);
        case FLOATTYPE: return new Pointer(FloatPointer);
        case ARRAYTYPE: return new Array((ArrayType*)eleType);
        default: cerr << "this type can't be element" << endl; throw exception();
    }
}

std::string Array::get_Ref() {
    return this->name;
}

Pointer* asPointer(ArrayType * arrType){
    // 在传入数组作为参数时，实际上是传入一个指针，因此需将Array类型转化为Pointer类型(注：这步转化不会在IR上有体现，只是方便打印信息)
    // 只有数组类型能够转化为指针，因此可以使用这个函数的ref要么为symbol（且包含类型必须为数组），要么为Array

    Type* eleType = arrType->elementType;
    PointerType* ptrType = new PointerType(eleType);
    return new Pointer(ptrType);
}

std::string Bool_Var::get_Ref() {
    return name;
}


Array_Const::Array_Const(std::vector<std::any>& v) {
    this->type = ArrConst;
    this->values = v;
}

Undef::Undef() {
    this->type = Undefined;
    this->name = "undef";
}

std::string Undef::get_Ref() {
    return "undef";
}

string Undef::get_TypeName() {
    return "undef";
}

Instruction *ValueRef::get_def() {
    if(def.empty())
        return nullptr;
    else
        return *(def.begin()); // SSA form, only has one def
}
