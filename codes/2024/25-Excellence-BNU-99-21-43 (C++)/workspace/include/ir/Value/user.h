#pragma once
#include "value.h"
#include "algorithm"

namespace IR
{
    // czr：好像也差不多，不是final类
    struct User
        : public Value
    {
        std::vector<Value *> operands;
        std::vector<Use *> uses; // 用了Value的use

        User(BasicType *type, const unsigned int subID) : Value(type, subID) {}
        User(BasicType *type, std::string name, const unsigned int subID) : Value(type, name, subID) {}

        virtual void addUse(Value *val);

        size_t getNumbOperands() const { return operands.size(); }
        virtual Value *getOperand(unsigned int i) const = 0;
        std::vector<Value *> &getOperands() { return operands; }

        virtual void removeUseFromVector(Use *use);

        static int totalUsers;

        void setTotalUsers() { number = totalUsers++; }
    };
}