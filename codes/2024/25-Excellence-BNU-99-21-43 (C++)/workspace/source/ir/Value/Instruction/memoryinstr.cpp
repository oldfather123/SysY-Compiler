#include "memoryinstr.h"

namespace IR
{
    AllocaInstruction::AllocaInstruction(BasicType *ty, std::string name, bool isConst)
        : Instruction(ty, MemoryOp::Alloca)
    {
        setTotalUsers();
        this->name = name;
        this->isConst = isConst;
        if (isConst)
            operands.push_back(Constant::getZeroValueForType(ty->getBaseType()));
    }

    AllocaInstruction::AllocaInstruction(PointerType *type, std::string name, Constant *init, bool isConst)
        : Instruction(type, MemoryOp::Alloca)
    {
        setTotalUsers();
        this->name = name;
        this->isConst = isConst;
        operands.push_back(init);
    }

    StoreInstruction::StoreInstruction(Value *lhs, Value *dest) : Instruction(BasicType::getVoidType(), MemoryOp::Store)
    {
        setTotalUsers();
        if (!BasicType::checkType(lhs->getType(), dest->getType()->getBaseType()))
        {
            Error::Error(__PRETTY_FUNCTION__, "Binary operands must have the same type");
        }
        addUse(lhs);
        addUse(dest);
    }

    bool GetElementPtrInstruction::isPointingConst()
    {
        if (!type->getBaseType()->isFloat() && !type->getBaseType()->isInt32())
            return false;
        auto base = operands[0];
        if (base->isGlobalVariable() && static_cast<GlobalVariable *>(base)->isPointingConst())
        {
            goto label;
        }
        else if (base->isInstruction() &&
                 static_cast<Instruction *>(base)->getOpcode() == Instruction::Alloca &&
                 static_cast<AllocaInstruction *>(base)->isPointingConst())
            goto label;
        else if (base->isInstruction() &&
                 static_cast<Instruction *>(base)->getOpcode() == Instruction::GEP &&
                 static_cast<GetElementPtrInstruction *>(base)->isPointingConst())
            goto label;
        return false;
    label:
        for (int i = 1; i < (int)operands.size(); i++)
        {
            if (!operands[i]->isConstantInt32())
                return false;
        }
        return true;
    }

    void GetElementPtrInstruction::emitIR(std::ostream &os)
    {
        std::string lhs = getIRName();
        std::string base = getOperand(0)->getTypeName() + " " + getOperand(0)->getIRName();
        std::string indices = "";
        int m = operands.size();
        for (int i = 1; i < m; i++)
        {
            indices += getOperand(i)->getTypeName() + " " + getOperand(i)->getIRName();
            if (i != m - 1)
                indices += ", ";
        }
        os << lhs << " = " << getOpStr() << " " << getTypeName() << " " << base << ", " << indices << std::endl;
    }

    GetElementPtrInstruction *IR::GetElementPtrInstruction::create(Value *base, std::vector<Value *> indices, std::string name)
    {
        if (!base->getType()->isPoint())
            Error::Error(__PRETTY_FUNCTION__, "Operand must be a pointer type");
        if (indices.size() == 0)
            Error::Error(__PRETTY_FUNCTION__, "Indices must not be empty");

        BasicType *geptype = base->getType();
        for (int i = 0; i < (int)indices.size(); i++)
            geptype = geptype->getBaseType();
        return new GetElementPtrInstruction(PointerType::get(geptype), base, indices, name);
    }

    GetElementPtrInstruction *IR::GetElementPtrInstruction::create(BasicType *type, Value *base, std::vector<Value *> indices, std::string name)
    {
        if (!base->getType()->isPoint())
            Error::Error(__PRETTY_FUNCTION__, "Operand must be a pointer type");
        if (indices.size() == 0)
            Error::Error(__PRETTY_FUNCTION__, "Indices must not be empty");

        return new GetElementPtrInstruction(type, base, indices, name);
    }

}