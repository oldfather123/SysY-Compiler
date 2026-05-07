#include "Type.h"

#include <memory>
#include <string_view>

std::vector<Type*> Type::types = {
    new Type("void"),
    new Type("bool"),
    new Type("int"),
    new Type("float"),
    new Type("int8"),
    new Type("int64")
};


template <>
Type* Type::get<void>() {
    return Type::types[0];
}

template <>
Type* Type::get<bool>() {
    return Type::types[1];
}

template <>
Type* Type::get<int>() {
    return Type::types[2];
}

template <>
Type* Type::get<float>() {
    return Type::types[3];
}

template <>
Type* Type::get<int8_t>() {
    return Type::types[4];
}

template <>
Type* Type::get<long long>() {
    return Type::types[5];
}

Type::Type(std::string_view type) {
    if(type == "int") {
        id = TypeID::Int;
    } else if(type == "void") {
        id = TypeID::Void;
    } else if(type == "bool") {
        id = TypeID::Bool;
    } else if(type == "float") {
        id = TypeID::Float;
    } else if(type == "int8") {
        id = TypeID::Int8;
    } else if(type == "int64") {
        id = TypeID::Int64;
    } else {
        cout << "Invalid type: " << type << endl;
    }
}

string Type::getStr() {
    if(id == TypeID::Int) {
        return "i32";
    }
    if(id == TypeID::Void) {
        return "void";
    }
    if(id == TypeID::Bool) {
        return "i1";
    }
    if(id == TypeID::Float) {
        return "float";
    }
    if(id == TypeID::Int8) {
        return "i8";
    }
    if(id == TypeID::Int64) {
        return "i64";
    }
    return "Invalid type";
}

void Type::dump(std::ostream& out) {
    out << getStr();
}

string ArrType::getStr() {
    return "[" + to_string(length) + " x " + inner->getStr() + "]";
}

int ArrType::getSize() {
    if(inner->isArr())
        return length * (dynamic_cast<ArrType*>(inner)->getSize());
    return length;
}

int ArrType::getDepth() {
    if(inner->isArr())
        return 1 + dynamic_cast<ArrType*>(inner)->getDepth();
    return 1;
}

bool ArrType::operator==(Type* ptr) {
    if(ptr->isArr()) {
        auto arrPtr = dynamic_cast<ArrType*>(ptr);
        // 理论上 inner 不能为 nll
        ASSERT(inner != nullptr, "inner is nll");
        return arrPtr->length == length && inner->operator==(arrPtr->inner);
    }
    return false;
}

bool ArrType::operator!=(Type* ptr) {
    return !operator==(ptr);
}

void ArrType::dump(std::ostream& out) {
    out << getStr();
}

string PtrType::getStr() {
    return inner->getStr() + "*";
}

bool PtrType::operator==(Type* ptr) {
    if(ptr->isPtr()) {
        auto another = dynamic_cast<PtrType*>(ptr);
        // 理论上 inner 不能为 nll
        ASSERT(inner != nullptr, "inner is nll");
        return inner->operator==(another->inner);
    }
    return false;
}

bool PtrType::operator!=(Type* ptr) {
    return !operator==(ptr);
}

void PtrType::dump(std::ostream& out) {
    out << getStr();
}
