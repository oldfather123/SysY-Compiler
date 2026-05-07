#pragma once
#include <iostream>
#include <vector>
#include <map>

namespace IR
{
    struct BasicType;
    struct Int32Type;
    struct FloatType;
    struct VoidType;
    struct ErrorType;
    struct ArrayType;
    struct FunctionType;
    struct PointerType;

    // czr: 这个不测，因为是纯虚函数，构造不了对象
    struct BasicType
    {
        enum TypeID
        {
            INT32,
            FLOAT,
            VOID,
            POINT,
            ERROR,
            ARRAY,
            FUNCTION,
            LABLE,
        };

        int sz;
        const unsigned int typeID = 0;
        // sz: size of the type in bytes
        BasicType(int sz, const TypeID type) : sz(sz), typeID(type){}

        // 虚函数
        virtual int getLength() const { return 1; }
        virtual std::string getTypeName() const = 0;
        virtual std::vector<int> getArrayDims() const = 0;
        virtual BasicType *getBaseType() const = 0;
        virtual const std::string _getArrayDims_() const
        {
            return "";
        }

        bool isArray() const { return typeID == TypeID::ARRAY; }
        bool isInt32() const { return typeID == TypeID::INT32; }
        bool isFloat() const { return typeID == TypeID::FLOAT; }
        bool isVoid() const { return typeID == TypeID::VOID; }
        bool isError() const { return typeID == TypeID::ERROR; }
        bool isFunction() const { return typeID == TypeID::FUNCTION; }
        bool isLable() const { return typeID == TypeID::LABLE; }
        bool isPoint() const { return typeID == TypeID::POINT; }


        int getSize() const { return sz; }

        // 静态成员函数，返回一个全局唯一的BasicType对象
        static Int32Type *getInt32Type();
        static FloatType *getFloatType();
        static VoidType *getVoidType();
        static ErrorType *getErrorType();
        static ArrayType *getArrayType(BasicType *baseType, int length);

        static bool checkType(BasicType *a, BasicType *b) {
            if (a->isInt32() && b->isInt32())
                return true;
            if (a->isFloat() && b->isFloat())
                return true;
            if (a->isVoid() && b->isVoid())
                return true;
            if (a->isPoint() && b->isPoint())
                return checkType(a->getBaseType(), b->getBaseType());
            if (a->isArray() && b->isArray())
                return checkType(a->getBaseType(), b->getBaseType());
            return false;
        }
    };

    
}