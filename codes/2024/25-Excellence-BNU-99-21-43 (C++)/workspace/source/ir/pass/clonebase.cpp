#include "clonebase.h"

namespace IR
{
    void CloneBase::addBasicBlock(BasicBlock *basicBlock)
    {
        // TODO20
        valueMap[basicBlock] = BasicBlock::get(basicBlock->getOperandName());
    }

    void CloneBase::addSymbolFor(Value *value)
    {
        valueMap[value] = Value::createTemp(value->getType());
    }

    Value *CloneBase::getReplacedValue(Value *value) const
    {
        auto it = valueMap.find(value);
        if (it != valueMap.end())
            return it->second;
        else
        {
            // std::cerr << "Value: " << value->getIRName() << std::endl;
            // assert(value->isConstantInt32() || value->isConstantFloat() || value->isGlobalVariable());
            return value;
        }
    }

    void CloneBase::setValueMapKv(Value *key, Value *value)
    {
        auto it = valueMap.find(key);
        if (it != valueMap.end())
        {
            auto val = (*it).second;
            if (!val->isTemporary())
                Error::Error(__PRETTY_FUNCTION__, __LINE__, "Can't replace a non-temporary value");
            val->replaceAllUsageTo(value);
        }
        valueMap[key] = value;
    }

    Instruction *CloneBase::cloneInstruction(Instruction *cloneInstruction)
    {
        switch (cloneInstruction->getOpcode())
        {
        case Instruction::Add:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createAdd(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Sub:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createSub(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Mul:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createMul(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Div:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createDiv(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Rem:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createRem(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }
        case Instruction::And:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createAnd(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }
        case Instruction::Or:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createOr(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }
        case Instruction::Xor:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createXor(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }
        case Instruction::FAdd:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createFAdd(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }
        case Instruction::FSub:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createFSub(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }
        case Instruction::FMul:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createFMul(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }
        case Instruction::FDiv:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createFDiv(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }
        case Instruction::FRem:
        {
            BinaryInstruction *instr = static_cast<BinaryInstruction *>(cloneInstruction);
            return BinaryInstruction::createFRem(instr->getType(), getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Neg:
        {
            UnaryInstruction *instr = static_cast<UnaryInstruction *>(cloneInstruction);
            return UnaryInstruction::createNeg(instr->getType(), getReplacedValue(instr->getOperand(0)));
        }

        case Instruction::FNeg:
        {
            UnaryInstruction *instr = static_cast<UnaryInstruction *>(cloneInstruction);
            return UnaryInstruction::createFNeg(instr->getType(), getReplacedValue(instr->getOperand(0)));
        }

        case Instruction::Not:
        {
            UnaryInstruction *instr = static_cast<UnaryInstruction *>(cloneInstruction);
            return UnaryInstruction::createNot(instr->getType(), getReplacedValue(instr->getOperand(0)));
        }

        // For Memory Operations
        case Instruction::Alloca:
        {
            AllocaInstruction *instr = static_cast<AllocaInstruction *>(cloneInstruction);
            return AllocaInstruction::create(instr->getType()->getBaseType());
        }

        case Instruction::Load:
        {
            LoadInstruction *instr = static_cast<LoadInstruction *>(cloneInstruction);
            return LoadInstruction::create(instr->getType(), getReplacedValue(instr->getOperand(0)));
        }

        case Instruction::Store:
        {
            StoreInstruction *instr = static_cast<StoreInstruction *>(cloneInstruction);
            return StoreInstruction::create(getReplacedValue(instr->getSrc()), getReplacedValue(instr->getDest()));
        }

        case Instruction::GEP:
        {
            GetElementPtrInstruction *instr = static_cast<GetElementPtrInstruction *>(cloneInstruction);
            std::vector<Value *> indices;
            for (auto v : instr->getIndices())
            {
                indices.push_back(getReplacedValue(v));
            }
            return GetElementPtrInstruction::create(instr->getType(), getReplacedValue(instr->getOperand(0)), indices);
        }

        // For Cast Operations
        case Instruction::FPtoSI:
        {
            CastInstruction *instr = static_cast<CastInstruction *>(cloneInstruction);
            return CastInstruction::createFPtoSI(instr->getType(), getReplacedValue(instr->getOperand(0)));
        }

        case Instruction::SItoFP:
        {
            CastInstruction *instr = static_cast<CastInstruction *>(cloneInstruction);
            return CastInstruction::createSItoFP(instr->getType(), getReplacedValue(instr->getOperand(0)));
        }

            // For Comparison Operations

        case Instruction::Ne:
        {
            ICmpInstruction *instr = static_cast<ICmpInstruction *>(cloneInstruction);
            return ICmpInstruction::createICmpNe(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Lt:
        {
            ICmpInstruction *instr = static_cast<ICmpInstruction *>(cloneInstruction);
            return ICmpInstruction::createICmpLt(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Le:
        {
            ICmpInstruction *instr = static_cast<ICmpInstruction *>(cloneInstruction);
            return ICmpInstruction::createICmpLe(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Gt:
        {
            ICmpInstruction *instr = static_cast<ICmpInstruction *>(cloneInstruction);
            return ICmpInstruction::createICmpGt(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Ge:
        {
            ICmpInstruction *instr = static_cast<ICmpInstruction *>(cloneInstruction);
            return ICmpInstruction::createICmpGe(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::Eq:
        {
            ICmpInstruction *instr = static_cast<ICmpInstruction *>(cloneInstruction);
            return ICmpInstruction::createICmpEq(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::FNe:
        {
            FCmpInstruction *instr = static_cast<FCmpInstruction *>(cloneInstruction);
            return FCmpInstruction::createFCmpFNe(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::FGe:
        {
            FCmpInstruction *instr = static_cast<FCmpInstruction *>(cloneInstruction);
            return FCmpInstruction::createFCmpFGe(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::FGt:
        {
            FCmpInstruction *instr = static_cast<FCmpInstruction *>(cloneInstruction);
            return FCmpInstruction::createFCmpFGt(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::FLe:
        {
            FCmpInstruction *instr = static_cast<FCmpInstruction *>(cloneInstruction);
            return FCmpInstruction::createFCmpFLe(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::FLt:
        {
            FCmpInstruction *instr = static_cast<FCmpInstruction *>(cloneInstruction);
            return FCmpInstruction::createFCmpFLt(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        case Instruction::FEq:
        {
            FCmpInstruction *instr = static_cast<FCmpInstruction *>(cloneInstruction);
            return FCmpInstruction::createFCmpFEq(getReplacedValue(instr->getOperand(0)), getReplacedValue(instr->getOperand(1)));
        }

        // For Other Operations
        case Instruction::Call:
        {
            CallInstruction *instr = static_cast<CallInstruction *>(cloneInstruction);
            std::vector<Value *> args = instr->getArgs();
            std::vector<Value *> newArgs;
            for (auto v : args)
            {
                newArgs.push_back(getReplacedValue(v));
            }
            return CallInstruction::create(instr->getCallee(), newArgs);
        }

        case Instruction::Phi:
        {
            PhiInstruction *instr = static_cast<PhiInstruction *>(cloneInstruction);
            std::vector<PVB> values;
            for (int i = 0; i < instr->getValueBBSize(); i++)
            {
                auto [value, bb] = instr->getValueBB(i);
                values.push_back(std::make_pair(getReplacedValue(value), static_cast<BasicBlock *>(getReplacedValue(bb))));
            }
            return PhiInstruction::create(instr->getType(), values);
        }

        case Instruction::Select:
        {
            Error::Error("InlineFunctionPass", "Select Instruction should not be cloned");
            return nullptr;
        }

        // For Terminator Operations
        case Instruction::Return:
        {
            Error::Error("InlineFunctionPass", "Return Instruction should not be cloned");
            return nullptr;
            // 补充
        }

        case Instruction::BR:
        {
            BranchInstruction *instr = static_cast<BranchInstruction *>(cloneInstruction);
            if (instr->isConditional())
            {
                return BranchInstruction::createCondBr(
                    getReplacedValue(instr->getCondition()),
                    static_cast<BasicBlock *>(getReplacedValue(instr->getTrueBlock())),
                    static_cast<BasicBlock *>(getReplacedValue(instr->getFalseBlock())));
            }
            else
            {
                return BranchInstruction::createBr(
                    static_cast<BasicBlock *>(getReplacedValue(instr->getUnconditionalBlock())));
            }
        }

        case Instruction::InderectBr:
        {
            Error::Error("InlineFunctionPass", "IndirectBr Instruction should not be cloned");
            return nullptr;
        }

        default:
        {
            Error::Error("InlineFunctionPass", "Unknown Instruction");
            return nullptr;
        }
        }
    }
}