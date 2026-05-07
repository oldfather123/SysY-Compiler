//
// Created by q1w2e3r4 on 23-6-30.
//

#include "Type.h"

using namespace std;

Type::Type(Type_Enum type) { this->type = type; }

std::string Type::getTypeName() {
    switch(this->type){
        case INT32TYPE: return "i32";
        case FLOATTYPE: return "float";
        case VOIDTYPE: return "void";
        default: {
            cerr << "shouldn't get here" << endl;
            throw exception();
        }
    }
}

int Type::getDimension() {
    return 0;
}

FuncType::FuncType(Type *retType, vector<Type *>& arguments): Type(FUNCTYPE) {
    this->retType = retType;
    this->arguments = arguments;
}

std::string ArrayType::getTypeName() {
    string name = "[" + to_string(this->ele_cnt) + " x ";
    name += this->elementType->getTypeName();
    name += ']';
    return name;
}

ArrayType::ArrayType(Type* elementType,int ele_cnt): Type(ARRAYTYPE){
    this->elementType = elementType;
    this->ele_cnt = ele_cnt;
    this->dimension = elementType->getDimension() + 1;
    this->size = elementType->getSize() * ele_cnt;
}

std::string PointerType::getTypeName() {
    return this->elementType->getTypeName() + '*';
}

PointerType::PointerType(Type *elementType): Type(POINTERTYPE) {
    this->elementType = elementType;
    this->dimension = elementType->getDimension() + 1;
    this->size = 8;
}
