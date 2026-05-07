#include "constantfolder.h"
#include "value.h"
#include "globalvalue.h"
#include "memoryinstr.h"
#include "constant.h"

#define HANDLE_BINARY_FOLDER(OPT, TYPE, OP)                                                                    \
    Value *ConstantFolder::Fold##OPT##TYPE(Value *LHS, Value *RHS)                                             \
    {                                                                                                          \
        if (!LHS->isConstant##TYPE() || !RHS->isConstant##TYPE())                                              \
            return nullptr;                                                                                    \
        return Constant##TYPE::get(                                                                            \
            static_cast<Constant##TYPE *>(LHS)->getValue() OP static_cast<Constant##TYPE *>(RHS)->getValue()); \
    }

#define HANDLE_CMP_FOLDER(OPT, TYPE, OP)                                                                       \
    Value *ConstantFolder::Fold##OPT##TYPE(Value *LHS, Value *RHS)                                             \
    {                                                                                                          \
        if (!LHS->isConstant##TYPE() || !RHS->isConstant##TYPE())                                              \
            return nullptr;                                                                                    \
        return ConstantInt32::get(                                                                             \
            static_cast<Constant##TYPE *>(LHS)->getValue() OP static_cast<Constant##TYPE *>(RHS)->getValue()); \
    }

#define HANDLE_UNARY_FOLDER(OPT, TYPE, OP)                                                 \
    Value *ConstantFolder::Fold##OPT##TYPE(Value *Operand)                                 \
    {                                                                                      \
        if (!Operand->isConstant##TYPE())                                                  \
            return nullptr;                                                                \
        return Constant##TYPE::get(OP static_cast<Constant##TYPE *>(Operand)->getValue()); \
    }

#define HANDLE_CAST_FOLDER(OPT, TYPE, TOTYPE, OP)                                             \
    Value *ConstantFolder::Fold##OPT(Value *Operand)                                          \
    {                                                                                         \
        if (!Operand->isConstant##TYPE())                                                     \
            return nullptr;                                                                   \
        return Constant##TOTYPE::get(OP(static_cast<Constant##TYPE *>(Operand)->getValue())); \
    }

namespace IR
{

    // 二元运算符
    HANDLE_BINARY_FOLDER(Add, Int32, +)
    HANDLE_BINARY_FOLDER(Sub, Int32, -)
    HANDLE_BINARY_FOLDER(Mul, Int32, *)
    HANDLE_BINARY_FOLDER(Div, Int32, /)
    HANDLE_BINARY_FOLDER(Rem, Int32, %)
    HANDLE_BINARY_FOLDER(FAdd, Float, +)
    HANDLE_BINARY_FOLDER(FSub, Float, -)
    HANDLE_BINARY_FOLDER(FMul, Float, *)
    HANDLE_BINARY_FOLDER(FDiv, Float, /)

    // 二元逻辑运算符
    HANDLE_CMP_FOLDER(Lt, Int32, <)
    HANDLE_CMP_FOLDER(Gt, Int32, >)
    HANDLE_CMP_FOLDER(Le, Int32, <=)
    HANDLE_CMP_FOLDER(Ge, Int32, >=)
    HANDLE_CMP_FOLDER(Eq, Int32, ==)
    HANDLE_CMP_FOLDER(Ne, Int32, !=)
    HANDLE_CMP_FOLDER(FEq, Float, ==)
    HANDLE_CMP_FOLDER(FNe, Float, !=)
    HANDLE_CMP_FOLDER(FLt, Float, <)
    HANDLE_CMP_FOLDER(FGt, Float, >)
    HANDLE_CMP_FOLDER(FLe, Float, <=)
    HANDLE_CMP_FOLDER(FGe, Float, >=)

    // 一元运算符
    HANDLE_UNARY_FOLDER(Neg, Int32, -)
    HANDLE_UNARY_FOLDER(FNeg, Float, -)
    HANDLE_UNARY_FOLDER(Not, Int32, !)

    // 类型转换
    HANDLE_CAST_FOLDER(FPtoSI, Float, Int32, static_cast<int>)
    HANDLE_CAST_FOLDER(SItoFP, Int32, Float, static_cast<float>)

    // 位运算符
    HANDLE_BINARY_FOLDER(And, Int32, &)
    HANDLE_BINARY_FOLDER(Or, Int32, |)
    HANDLE_BINARY_FOLDER(Xor, Int32, ^)

    bool ConstantFolder::isPointingConst(Value *v)
    {
        if (v->isConstantPointer())
            return true;
        if (v->isGlobalVariable())
            return static_cast<GlobalVariable *>(v)->isPointingConst();
        if (v->isInstruction() && static_cast<Instruction *>(v)->getOpcode() == Instruction::Alloca)
            return static_cast<AllocaInstruction *>(v)->isPointingConst();
        return false;
    }

    Value *ConstantFolder::FoldGEP(Value *base, std::vector<Value *> indices)
    {
        return nullptr;
    }

    // czr: 《论屎山是如何形成的》
    Value *ConstantFolder::FoldLoad(BasicType *ty, Value *ptr)
    {
        if (ptr->isGlobalVariable())
        {
            GlobalVariable *temp = static_cast<GlobalVariable *>(ptr);
            if (!temp->isPointingConst())
                return nullptr;
            if (ty != temp->getType()->getBaseType())
                Error::Error(__PRETTY_FUNCTION__, "Load type mismatch");
            return temp->getOperand(0);
        }
        else if (ptr->isInstruction() && static_cast<Instruction *>(ptr)->getOpcode() == Instruction::Alloca)
        {
            AllocaInstruction *temp = static_cast<AllocaInstruction *>(ptr);
            if (!temp->isPointingConst())
                return nullptr;
            if (ty != temp->getType()->getBaseType())
                Error::Error(__PRETTY_FUNCTION__, "Load type mismatch");
            return temp->getOperand(0);
        }
        else if (ptr->isInstruction() && static_cast<Instruction *>(ptr)->getOpcode() == Instruction::GEP)
        {
            GetElementPtrInstruction *temp = static_cast<GetElementPtrInstruction *>(ptr);
            if (!temp->isPointingConst())
                return nullptr;
            if (ty != temp->getType()->getBaseType())
                Error::Error(__PRETTY_FUNCTION__, "Load type mismatch");
            auto base = temp->getOperand(0);
            for (int i = 1; i < (int)temp->getNumbOperands(); i++)
            {
                int x = static_cast<ConstantInt32 *>(temp->getOperand(i))->getValue();
                base = static_cast<User *>(base)->getOperand(x);
            }
            return base;
        }
        return nullptr;
    }
}