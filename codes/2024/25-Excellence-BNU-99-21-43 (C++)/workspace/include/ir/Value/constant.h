#pragma once

#include "user.h"
#include "int32type.h"
#include "floattype.h"
#include "arraytype.h"
#include "pointertype.h"
#include "error.h"
#include <assert.h>
#include <map>

namespace IR
{
    // czr: 这一块主要需要测的是ConstantArray。尤其是获取元素，看看能不能对上。
    struct Constant : public User
    {
        Constant(BasicType *type, const unsigned int subID) : User(type, subID) {}

        Constant(BasicType *type, std::string name, const unsigned int subID) : User(type, name, subID) {}

        Constant *getOperand(unsigned int i) const override
        {
            if (i >= operands.size())
                Error::Error(__PRETTY_FUNCTION__, "Index out of bounds");
            return static_cast<Constant *>(operands[i]);
        };

        static Constant *getZeroValueForType(BasicType *type);
    };

    struct ConstantInt32 final : public Constant
    {
        int value;

        ConstantInt32(int v) : Constant(BasicType::getInt32Type(), ValueTy::ConstantInt32Val), value(v) {}
        ConstantInt32(int v, std::string name) : Constant(BasicType::getInt32Type(), name, ValueTy::ConstantInt32Val), value(v) {}
        int getValue() const { return value; }

        static ConstantInt32 *get(int value)
        {
            return new ConstantInt32(value);
        }

        std::string getIRName() const override
        {
            return std::to_string(value);
        }
    };

    struct ConstantFloat final : public Constant
    {
        float value;

        ConstantFloat(float v) : Constant(BasicType::getFloatType(), ValueTy::ConstantFloatVal), value(v) {}
        ConstantFloat(float v, std::string name) : Constant(BasicType::getFloatType(), name, ValueTy::ConstantFloatVal), value(v) {}
        float getValue() const { return value; }

        static ConstantFloat *get(float value)
        {
            return new ConstantFloat(value);
        }

        std::string getIRName() const override
        {
            return std::to_string(value);
        }
    };

    struct ConstantArray : public Constant
    {
        std::map<unsigned int, Constant *> elements;

        ConstantArray(BasicType *type) : Constant(type, ValueTy::ConstantArrayVal) {}
        ConstantArray(BasicType *type, std::string name) : Constant(type, name, ValueTy::ConstantArrayVal) {}

        Constant *getOperand(unsigned int i) const override
        {
            auto element = elements.find(i);
            if (element == elements.end())
            {
                if ((int)i >= type->getLength())
                    Error::Error(__PRETTY_FUNCTION__, "Index out of bounds");
                return Constant::getZeroValueForType(static_cast<ArrayType *>(type)->getNextType());
            }
            return element->second;
        }

        void insertOperand(unsigned int i, Constant *value)
        {
            elements[i] = value;
        }

        static ConstantArray *get(BasicType *type)
        {
            if (!type->isArray())
                Error::Error(__PRETTY_FUNCTION__, "Type is not an array type");
            return new ConstantArray(type);
        }

        Constant *addIndex(BasicType *ty, unsigned int i, Constant *value)
        {
            if (elements.find(i) == elements.end())
            {
                insertOperand(i, value);
            }
            else
            {
                Error::Error(__PRETTY_FUNCTION__, "Index has been set");
            }
            return elements[i];
        }

        ConstantArray *addIndex(BasicType *ty, unsigned int i)
        {
            if (elements.find(i) == elements.end())
            {
                elements[i] = ConstantArray::get(ty);
            }
            assert(elements[i]->isConstantArray());
            return static_cast<ConstantArray *>(elements[i]);
        }
    };

    // Constant Pointer points to a constantInt32 or a constantFloat
    struct ConstantPointer : public Constant
    {
        Constant *Value;

        ConstantPointer(Constant *value)
            : Constant(PointerType::get(value->getType()), ValueTy::ConstantPointerVal)
        {
            operands.push_back(value);
        }
        ConstantPointer(std::string name, Constant *value)
            : Constant(PointerType::get(value->getType()), name, ValueTy::ConstantPointerVal)
        {
            operands.push_back(value);
        }

        Constant *getOperand(unsigned int i) const override
        {
            if (i != 0)
                Error::Error(__PRETTY_FUNCTION__, "Index out of bounds");
            return static_cast<Constant *>(operands[0]);
        }

        static ConstantPointer *get(Constant *value)
        {
            return new ConstantPointer(value);
        }
    };
}