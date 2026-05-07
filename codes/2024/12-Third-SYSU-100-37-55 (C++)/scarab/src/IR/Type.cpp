#include "Type.h"

vector<shared_ptr<Type> > Type::types = {
    TypePtr(new Type(VoidID)),
    TypePtr(new Type(BoolID)),
    TypePtr(new Type(IntID)),
    TypePtr(new Type(FLoatID)),
    TypePtr(new Type(Int8ID)),
    TypePtr(new Type(Int64ID))
};


Type::Type(string typeString){
    static const std::map<std::string, TypeID> string2Type = {
        {"int", IntID},
        {"void", VoidID},
        {"bool", BoolID},
        {"float", FLoatID},
        {"int8", Int8ID},
        {"int64", Int64ID}
    };

    auto it = string2Type.find(typeString);
    if (it != string2Type.end()) {
        ID = it->second;
    } else {
        cout << "wrong type: " << typeString << endl;
    }
}

string Type::toString() {
    static const std::map<int, std::string> ID2String = {
        {Int8ID, "i8"},
        {Int64ID, "i64"},
        {IntID, "i32"},
        {FLoatID, "float"},
        {BoolID, "i1"},
        {VoidID, "void"}
    };

    auto it = ID2String.find(ID);
    if (it != ID2String.end()) {
        return it->second;
    } else {
        return "wrong type";
    }
}

string ArrType::toString() {
    string retStr =  "[" + to_string(size) + " x " + inner->toString() + "]";
    return retStr;
}

int ArrType::getSize(){
    int retSize = inner->isArr() ? size * (dynamic_cast<ArrType *>(inner.get())->getSize()) : size;
    return retSize;
}

int ArrType::getDepth(){
    int retDepth = inner->isArr() ? 1 + dynamic_cast<ArrType*>(inner.get())->getDepth() : 1;
    return retDepth;
}

bool ArrType::operator==(shared_ptr<Type> ptr){
    if (ptr->isArr()){
        auto arrPtr = dynamic_cast<ArrType *>(ptr.get());
        return arrPtr->size == size && inner->operator == (arrPtr->inner);
    }
    return false;
}

string PtrType::toString() {
    string retStr = inner->toString() + "*";
    return retStr;
}

bool PtrType::operator==(shared_ptr<Type> ptr){
    if (ptr->isPtr()){
        auto another = dynamic_cast<PtrType *>(ptr.get());
        return inner->operator == (another->inner);
    }
    return false;
}