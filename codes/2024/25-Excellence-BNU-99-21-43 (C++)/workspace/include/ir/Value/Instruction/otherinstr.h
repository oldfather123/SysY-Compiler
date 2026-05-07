#pragma once
#include "instruction.h"
#include "error.h"
#include "globalvalue.h"

#define HANDLE_CMP_INST(TYPE, OPCODE)                                      \
    static TYPE##Instruction *create##TYPE##OPCODE(Value *lhs, Value *rhs) \
    {                                                                      \
        return TYPE##Instruction::create(OPCODE, lhs, rhs);                \
    }

using PVB = std::pair<IR::Value *, IR::BasicBlock *>;
namespace IR
{
    struct PhiInstruction : public Instruction
    {
        std::set<PVB> incomingValue;

        PhiInstruction(BasicType *type, std::vector<PVB> value, std::string name = "");

        std::vector<PVB> getDifferentPVB();

        void emitIR(std::ostream &os) override;

        static PhiInstruction *create(BasicType *type, std::vector<PVB> value, std::string name = "")
        {
            return new PhiInstruction(type, value, name);
        }

        int getValueBBSize() const
        {
            return operands.size() / 2;
        }

        PVB getValueBB(int i) const;

        std::vector<PVB> getIncomingValue() const;

        void insertIncomingValue(PVB value);

        bool allSameValue();

        void removeUseFromVector(Use *use) override;

        void removeEntry(BasicBlock *bb);
    };

    // 比较函数，没啥好测的，测一下emitIR，用ICmpInstruction和FCmpInstruction测试，使用静态函数创建对象
    struct CmpInstruction : public Instruction
    {
        const unsigned int cmpCode;
        CmpInstruction(CmpOp op, Value *lhs, Value *rhs);

        unsigned int getCmpCode() const
        {
            return cmpCode;
        }

        void emitIR(std::ostream &os) override
        {
            os << getIRName() << " = " << getOpStr() << " " << type->getTypeName() << " " << getOperand(0)->getIRName() << ", " << getOperand(1)->getIRName() << std::endl;
        }
    };

    struct ICmpInstruction : public CmpInstruction
    {
        ICmpInstruction(CmpOp cmp, Value *lhs, Value *rhs) : CmpInstruction(cmp, lhs, rhs) {}
        static ICmpInstruction *create(CmpOp cmp, Value *lhs, Value *rhs)
        {
            return new ICmpInstruction(cmp, lhs, rhs);
        }

        HANDLE_CMP_INST(ICmp, Gt)
        HANDLE_CMP_INST(ICmp, Ge)
        HANDLE_CMP_INST(ICmp, Lt)
        HANDLE_CMP_INST(ICmp, Le)
        HANDLE_CMP_INST(ICmp, Eq)
        HANDLE_CMP_INST(ICmp, Ne)

        // 创建一个新的ICmpInstruction，其比较结果与原ICmpInstruction相反
        static ICmpInstruction *createNotICmp(ICmpInstruction *icmp, bool resultNot = true)
        {
            auto cmp = icmp->getOpcode();
            auto newCmp = 0;
            switch (cmp)
            {
            case ICmpInstruction::Gt:
                newCmp = ICmpInstruction::Le;
                break;
            case ICmpInstruction::Ge:
                newCmp = ICmpInstruction::Lt;
                break;
            case ICmpInstruction::Lt:
                newCmp = ICmpInstruction::Ge;
                break;
            case ICmpInstruction::Le:
                newCmp = ICmpInstruction::Gt;
                break;
            case ICmpInstruction::Eq:
                newCmp = ICmpInstruction::Ne;
                break;
            case ICmpInstruction::Ne:
                newCmp = ICmpInstruction::Eq;
                break;
            default:
                Error::Error("ICmpInstruction::createNotICmp", "Unknown opcode");
                break;
            }
            if (!resultNot)
                return ICmpInstruction::create((CmpOp)newCmp, icmp->getOperand(1), icmp->getOperand(0));
            else
                return ICmpInstruction::create((CmpOp)newCmp, icmp->getOperand(0), icmp->getOperand(1));
        }
    };

    struct FCmpInstruction : public CmpInstruction
    {
        FCmpInstruction(CmpOp cmp, Value *lhs, Value *rhs) : CmpInstruction(cmp, lhs, rhs) {}
        static FCmpInstruction *create(CmpOp cmp, Value *lhs, Value *rhs)
        {
            return new FCmpInstruction(cmp, lhs, rhs);
        }
        HANDLE_CMP_INST(FCmp, FGt)
        HANDLE_CMP_INST(FCmp, FGe)
        HANDLE_CMP_INST(FCmp, FLt)
        HANDLE_CMP_INST(FCmp, FLe)
        HANDLE_CMP_INST(FCmp, FEq)
        HANDLE_CMP_INST(FCmp, FNe)

        // 创建一个新的ICmpInstruction，其比较结果与原ICmpInstruction相反
        static FCmpInstruction *createNotFCmp(FCmpInstruction *icmp, bool resultNot = true)
        {
            auto cmp = icmp->getOpcode();
            auto newCmp = 0;
            switch (cmp)
            {
            case FCmpInstruction::FGt:
                newCmp = FCmpInstruction::FLe;
                break;
            case FCmpInstruction::FGe:
                newCmp = FCmpInstruction::FLt;
                break;
            case FCmpInstruction::FLt:
                newCmp = FCmpInstruction::FGe;
                break;
            case FCmpInstruction::FLe:
                newCmp = FCmpInstruction::FGt;
                break;
            case FCmpInstruction::FEq:
                newCmp = FCmpInstruction::FNe;
                break;
            case FCmpInstruction::FNe:
                newCmp = FCmpInstruction::FEq;
                break;
            default:
                Error::Error("ICmpInstruction::createNotFCmp", "Unknown opcode");
                break;
            }
            if (!resultNot)
                return FCmpInstruction::create((CmpOp)newCmp, icmp->getOperand(1), icmp->getOperand(0));
            else
                return FCmpInstruction::create((CmpOp)newCmp, icmp->getOperand(0), icmp->getOperand(1));
        };
    };

    struct CallInstruction : public Instruction
    {
        CallInstruction(Function *func, std::vector<Value *> args);
        void emitIR(std::ostream &os) override;

        static CallInstruction *create(Function *func, std::vector<Value *> args)
        {
            return new CallInstruction(func, args);
        }

        Function *getCallee()
        {
            assert(operands[0]->isFunction());
            return static_cast<Function *>(operands[0]);
        }

        std::vector<Value *> getArgs()
        {
            std::vector<Value *> args;
            for (int i = 1; i < (int)operands.size(); i++)
                args.push_back(operands[i]);
            return args;
        }

        bool isReturnVoid() { return type->isVoid(); }
    };
}
