#pragma once

#include "instruction.h"
#include "unaryinstr.h"
#include "constant.h"
#include "pointertype.h"
#include "voidtype.h"
#include "value.h"
#include "error.h"
#include "globalvalue.h"
#include <assert.h>

namespace IR
{
    // me = alloca type
    // 要注意的是这里必须返回的是指针类型，记得测试一下。
    struct AllocaInstruction : public Instruction
    {
        bool isConst;
        AllocaInstruction(BasicType *ty, std::string name, bool isConst = false);

        AllocaInstruction(PointerType *type, std::string name, Constant *init, bool isConst);

        static AllocaInstruction *create(BasicType *type, std::string name = "", bool isConst = false)
        {
            PointerType *ptrType = PointerType::get(type);
            return new AllocaInstruction(ptrType, name, isConst);
        }

        static AllocaInstruction *Create(BasicType *type, Constant *init, bool isConst, std::string name = "")
        {
            PointerType *ptrType = PointerType::get(type);
            return new AllocaInstruction(ptrType, name, init, isConst);
        }

        void setInitializer(Constant *init) override
        {
            assert(operands.size() > 0);
            operands[0] = init;
        }

        void emitIR(std::ostream &os) override
        {
            os << getIRName() << " = " << getOpStr() << " " << type->getBaseType()->getTypeName() << std::endl;
        }

        BasicType *getAllocaType() { return type->getBaseType(); }
        bool isPointingConst() { return isConst; }
    };

    // czr：load指令，用于从内存中读取数据
    // me = load type, type* op
    struct LoadInstruction : public UnaryInstruction
    {
        LoadInstruction(BasicType *type, Value *op, std::string name)
            : UnaryInstruction(type, MemoryOp::Load, op)
        {
            this->name = name;
        }

        static LoadInstruction *create(BasicType *type, Value *op, std::string name = "")
        {
            if (!op->getType()->isPoint())
                Error::Error(__PRETTY_FUNCTION__, "Operand must be a pointer type");
            return new LoadInstruction(type, op, name);
        }

        void emitIR(std::ostream &os) override
        {
            std::string lhs = getIRName();
            std::string op = getOperand(0)->getTypeName() + " " + getOperand(0)->getIRName();
            os << lhs << " = " << getOpStr() + " " + getTypeName() << ", " << op << std::endl;
        }

        Value *getSrc() { return getOperand(0); }
    };

    // czr: Store指令，用于将数据存储到内存中，应该都能看懂。
    // Store type lhs, type* dest
    struct StoreInstruction : public Instruction
    {
        StoreInstruction(Value *lhs, Value *dest);

        void emitIR(std::ostream &os) override
        {
            std::string lhs = getOperand(0)->getTypeName() + " " + getOperand(0)->getIRName();
            std::string dest = getOperand(1)->getTypeName() + " " + getOperand(1)->getIRName();
            os << getOpStr() << " " << lhs << ", " << dest << std::endl;
        }

        static StoreInstruction *create(Value *lhs, Value *dest)
        {
            if (!dest->getType()->isPoint())
                Error::Error(__PRETTY_FUNCTION__, "Operand must be a pointer type");
            return new StoreInstruction(lhs, dest);
        }

        Value *getSrc() { return getOperand(0); }

        Value *getDest() { return getOperand(1); }
    };

    // 这个指令比较重要，用于数组的访问，Base必须是一个指针（其实就是计算偏移量的，返回的也是指针）
    // Indices是一个vector, 第一维对应于指针Base，后面的维度对应于Base的维度
    // type* lhs = getelementptr type* base, {index1, index2, ...}
    struct GetElementPtrInstruction : public Instruction
    {
        GetElementPtrInstruction(BasicType *type, Value *base, std::vector<Value *> indices, std::string name)
            : Instruction(type, MemoryOp::GEP)
        {
            setTotalUsers();
            addUse(base);
            for (auto index : indices)
                addUse(index);
            this->name = name;
        }

        bool isPointingConst();

        void emitIR(std::ostream &os) override;
        

        std::vector<Value *> getIndices()
        {
            std::vector<Value *> indices;
            for (int i = 1; i < (int)operands.size(); i++)
                indices.push_back(operands[i]);
            return indices;
        }

        static GetElementPtrInstruction *create(Value *base, std::vector<Value *> indices, std::string name = "");

        static GetElementPtrInstruction *create(BasicType* type, Value *base, std::vector<Value *> indices, std::string name = "");
    };
}