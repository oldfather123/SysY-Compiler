//
// Created by 牛奕博 on 2023/4/17.
//
#ifndef ANUC_TYPE_H
#define ANUC_TYPE_H

#include <iostream>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include "alist.h"
#include "rtti.h"
#include "value.h"

using namespace std;
//类型系统
namespace anuc {
    class Instruction;

    class AllocateInst;

    class InitList;

    class Type {
    public:
        enum TypeKind {
            TK_Block,
            TK_Int1,
            TK_Int32,
            TK_Float,
            TK_Void,
            TK_Array,
            TK_Ptr,
            TK_Func,
            TK_Last,
        };
    private:
        const TypeKind kind;
    public:
        Type(TypeKind kind) : kind(kind) {}

        TypeKind getKind() { return kind; }

        virtual bool isArrayType() { return false; }

        virtual string toString() { return ""; }

        bool static classof(Type *v) { return v->getKind() >= Type::TK_Int1 && v->getKind() <= Type::TK_Last; }

        virtual int getByteSize() { return 0; }

    };

    class BlockType : public Type {
    public:
        BlockType() : Type(Type::TK_Block) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Block; }

        bool isArrayType() { return false; }

        string toString() { return "block"; }
    };

    class Int32Type : public Type {
    public:
        Int32Type() : Type(Type::TK_Int32) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Int32; }

        bool isArrayType() { return false; }

        string toString() { return "i32"; }

        int getByteSize() { return 4; }

    };

    class Int1Type : public Type {
    public:
        Int1Type() : Type(Type::TK_Int1) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Int1; }

        bool isArrayType() { return false; }

        string toString() { return "i1"; }

        int getByteSize() { return 1; }
    };

    class FloatType : public Type {
    public:
        FloatType() : Type(Type::TK_Float) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Float; }

        bool isArrayType() { return false; }

        string toString() { return "float"; }

        int getByteSize() { return 4; }
    };

    class VoidType : public Type {
    public:
        VoidType() : Type(Type::TK_Void) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Void; }

        bool isArrayType() { return false; }

        string toString() { return "void"; }
    };

    //getArrayType 返回数组中存储元素的类型
    //getDepth 获得数组嵌套层数
    class ArrayType : public Type {
        Type *type;
        unsigned size{0};
    public:
        ArrayType(Type *type, unsigned size) : Type(Type::TK_Array), type(type), size(size) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Array; }

        Type *getArrayType() { return type; }

        int getSize() { return size; }

        bool isArrayType() { return true; }

        string toString() {
            return '[' + to_string(size) + " x " + type->toString() + ']';
        }

        int getElementSize() { return type->getByteSize(); }
        //计算占用多少个字节
        int getByteSize() {
            Type *ty = this;
            //记录每一层大小
            vector<int> sizeList;
            while (isa<ArrayType>(ty)) {
                ArrayType *aty = cast<ArrayType>(ty);
                sizeList.push_back(aty->getSize());
                ty = aty->getArrayType();
            }
            int byteSize = ty->getByteSize();
            for(int i = sizeList.size() - 1; i >= 0; --i) {
                byteSize = byteSize * sizeList[i];
            }
            return byteSize;
        }
        int getDepth() {
            int i = 1;
            Type *ty = type;
            while (isa<ArrayType>(ty)) {
                ArrayType *aty = cast<ArrayType>(ty);
                ++i;
                ty = aty->getArrayType();
            }
            return i;
        }

    };

    //getElementType 返回指针指向的类型
    class PointerType : public Type {
        Type *type;
    public:
        PointerType(Type *type) : Type(Type::TK_Ptr), type(type) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Ptr; }

        Type *getElementType() { return type; }

        string toString() { return "ptr"; }

        int getByteSize() { return 8; }

    };

    class FunctionType : public Type {
        Type *returnType;
        std::vector<Type *> argvs;
    public:
        FunctionType(Type *returnType, std::vector<Type *> argvs) : Type(Type::TK_Func), returnType(returnType),
                                                                    argvs(argvs) {}

        bool static classof(Type *v) { return v->getKind() == Type::TK_Func; }

        bool isArrayType() { return false; }

        Type *getRetType() { return returnType; }

        std::vector<Type *> &getArgvs() { return argvs; }

    };




    /*-------------------------------------------------------------------*/
    //constant
    class Constant : public Value {
    public:
        Constant(Type *type, Value::ValueKind kind) : Value(kind, type) {}

        bool static classof(Value *v) {
            return v->getKind() >= Value::VK_Constant
                   && v->getKind() < Value::VK_LastConstant;
        }

    };

    class ConstantInt : public Constant {
        int value;
    public:
        ConstantInt(Type *type, int value) : Constant(type, Value::VK_ConstantInt), value(value) {}

        bool static classof(Value *v) { return v->getKind() == Value::VK_ConstantInt; }

        int getValue() { return value; }

        string toString() { return " " + to_string(value); }

    };

    class ConstantFloat : public Constant {
        float value;
    public:
        ConstantFloat(Type *type, float value) : Constant(type, Value::VK_ConstantFloat), value(value) {}

        bool static classof(Value *v) { return v->getKind() == Value::VK_ConstantFloat; }

        float getValue() { return value; }

        string toString() {
            auto toHex = [=](float number) -> string {
                union FloatToHex {
                    double d;
                    uint64_t i;
                };
                FloatToHex converter;
                converter.d = static_cast<double>(number);
                ostringstream oss;
                oss << hex << setw(16) << setfill('0') << converter.i;
                return oss.str();
            };
            double doubleValue = static_cast<double>(value);
            std::ostringstream oss;
            oss << std::setprecision(std::numeric_limits<double>::digits10)
                << std::scientific << doubleValue;
            std::string result = oss.str();
            //result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
            return " 0x" + toHex(value);
        }

        string toStringInDecimal() {
            return " " + to_string(value);
        }
    };

    struct InitList {
        using indexInfo = pair<vector<int>, Value *>;
        vector<indexInfo> initInfos;
    };

    /*--------------------------------------------*/
    class BaseReg : public Value {
    protected:
        Instruction *def{nullptr};
    public:
        BaseReg(Value::ValueKind kind, Type *type) : Value(kind, type) {}

        bool static classof(Value *v) {
            return v->getKind() >= Value::VK_BaseReg
                   && v->getKind() < Value::VK_LastBaseReg;
        }

        virtual void setInst(Instruction *a) { def = a; }

        virtual Instruction *getDef() { return def; }

        virtual string toString() {
            return "base_reg";
        }
    };
    //寄存器变量 RegisterVar

    class RegisterVar : public BaseReg {
        std::string name;

    public:
        RegisterVar(Type *type, string name) : BaseReg(VK_RegisterVar, type), name(name) {}

        bool static classof(Value *v) { return v->getKind() == VK_RegisterVar; }

//        void setInst(Instruction *a) { def = a; }
//
//        Instruction *getDef() { return def; }

        string getName() { return name; }

        string toString() {
            return " %" + name;
        }
    };


    /*-----------------------------------------*/

    //全局变量（实际上也是指针）
    class GlobalVar : public Value, public alist_node<GlobalVar> {
        string name;
        Type *type;
        Type *valueType;
        Constant *initValue{nullptr};
        //处理列表表达式赋值
        InitList *list{nullptr};
        bool isArray{false};
    public:
        GlobalVar(Type *ty, string name, Constant *initValue) : Value(VK_GlobalVar, ty), type(ty), name(name),
                                                                initValue(initValue) {
            if (isa<ArrayType>(ty)) isArray = true;
            valueType = cast<PointerType>(type)->getElementType();
        }

        GlobalVar(Type *ty, string name) : Value(VK_GlobalVar, ty), type(ty), name(name) {
            if (isa<ArrayType>(ty)) isArray = true;
            valueType = cast<PointerType>(type)->getElementType();
        }

        GlobalVar(Type *ty, string name, InitList *list) : Value(VK_GlobalVar, ty), type(ty), name(name), list(list) {
            if (isa<ArrayType>(ty)) isArray = true;
            valueType = cast<PointerType>(type)->getElementType();
        }

        bool static classof(Value *v) { return v->getKind() == VK_GlobalVar; }

        void setInitValue(Constant *v) {
            initValue = v;
        }

        string getName() { return name; }

        Type *getType() { return type; }

        Type *getValueType() { return valueType; }

        string toString() { return " @" + name; }

        void print() {
            if (initValue) {
                if (isa<ConstantInt>(initValue))
                    cout << "@" + name + " = global " + valueType->toString() << " "
                         << cast<ConstantInt>(initValue)->getValue();
                if (isa<ConstantFloat>(initValue))
                    cout << "@" + name + " = global " + valueType->toString() << " "
                         << cast<ConstantFloat>(initValue)->toString();
                cout << ", align 4" << endl;
            } else if (list) {

            } else {
                cout << "@" + name + " = global " + valueType->toString() << endl;
            }
        }
        InitList *getInitList() {return list;}
        Constant *getInit() {
            return initValue;
        }
    };


}

#endif //ANUC_TYPE_H
