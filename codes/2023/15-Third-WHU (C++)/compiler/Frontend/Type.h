#pragma once
#ifndef Type_H
#define Type_H

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

//ͨ��TypeID����ʾ����
class Type {
public:
    enum TypeID {
        Int,
        Float,
        Void,
        Array,
        Point
    };
    void setTypeID(Type::TypeID ID) {
        this->ID = ID;
    }
    Type::TypeID getTypeID() const {
        return ID;
    }
private:
    Type::TypeID ID;

};

//����������
class ValType {
private:
    Type* BaseType;
    bool IsConst = false;
public:
    void setType(Type* BaseType) {
        this->BaseType = BaseType;
    }
    void setConst() {
        this->IsConst = true;
    }
    Type* getBaseType() {
        return this->BaseType;
    }
    bool isConstQualified() const {
        return IsConst;
    }
    std::string getTypeStr();
};

//��������
class IntType : public Type {
public:
    IntType() {
        setTypeID(Int);
    }
};

//����������
class FloatType : public Type {
public:
    FloatType() {
        setTypeID(Float);
    }
};

//��������void
class VoidType : public Type {
public:
    VoidType() {
        setTypeID(Void);
    }
};

//��ArrayType��PointType�Ƶ���AST.h����********



//����������
class FuncType {
    ValType* ReturnType;//����ֵ����
    std::vector<ValType*> ParamTypes;//��������
public:
    void setReturnType(ValType* BaseType) {
        this->ReturnType = BaseType;
    }
    ValType* getReturnType() {
        return this->ReturnType;
    }
    void setParamTypes(std::vector<ValType*> vec) {
        this->ParamTypes = std::move(vec);
    }
    std::vector<ValType*> getParamTypes() {
        return this->ParamTypes;
    }
    std::string getTypeStr();
};



#endif // !Type_H
