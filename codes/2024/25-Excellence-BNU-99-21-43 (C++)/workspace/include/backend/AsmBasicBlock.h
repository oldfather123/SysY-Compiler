#pragma once

#include "binaryinstr.h"
#include "memoryinstr.h"
#include "otherinstr.h"
#include "terminatorinstr.h"
#include "unaryinstr.h"

#include "basicblock.h"
#include "globalvalue.h"

#include "AsmInstHeaders.h"

#include <map>
namespace Backend
{
    class AsmFunction;
    class MulPlannerOperand;

    class AsmBasicBlock
    {
    private:
        AsmFunction *asmFunction;
        IR::BasicBlock *irBlock;

        AsmOperandLabel *blockOperandLabel;
        AsmInstLabel *blockInstLabel;

        std::map<IR::BasicBlock *, AsmInstBasicList> phiOperation;
        std::map<IR::BasicBlock *, std::map<IR::Value *, std::vector<AsmOperandRegister *>>> phiValueMap;

    public:
        AsmBasicBlock(AsmFunction *asmFunction, IR::BasicBlock *irBlock)
            : asmFunction(asmFunction), irBlock(irBlock)
        {
            blockOperandLabel = new AsmOperandLabel(irBlock->getIRName(), false);
            blockInstLabel = new AsmInstLabel(blockOperandLabel);
        }

        void genBB();
        void genInst(IR::Instruction *inst);

        void genInstBinary(IR::BinaryInstruction *inst); // add, sub, mul, div, rem (int / float), xor, or, and
        void genInstBinary_IntAdd(IR::BinaryInstruction *inst);
        void genInstBinary_IntSub(IR::BinaryInstruction *inst);
        void genInstBinary_IntMul(IR::BinaryInstruction *inst);
        void genInstBinary_IntDiv(IR::BinaryInstruction *inst);
        void genInstBinary_IntRem(IR::BinaryInstruction *inst);
        void genInstBinary_IntXor(IR::BinaryInstruction *inst);
        void genInstBinary_IntOr(IR::BinaryInstruction *inst);
        void genInstBinary_IntAnd(IR::BinaryInstruction *inst);
        void genInstBinary_FPAdd(IR::BinaryInstruction *inst);
        void genInstBinary_FPSub(IR::BinaryInstruction *inst);
        void genInstBinary_FPMul(IR::BinaryInstruction *inst);
        void genInstBinary_FPDiv(IR::BinaryInstruction *inst);
        AsmOperandBasic *genConstantMultiplyPlan(AsmOperandRegisterInt *opRegister, MulPlannerOperand *plan, int bitLength);
        void genInstSignedConstIntDivide(IR::BinaryInstruction *inst, IR::ConstantInt32 *constDivisor);

        void genInstAlloca(IR::AllocaInstruction *inst);
        void genInstLoad(IR::LoadInstruction *inst);
        void genInstStore(IR::StoreInstruction *inst);
        void genInstGetElementPtr(IR::GetElementPtrInstruction *inst);

        void genInstUnary(IR::UnaryInstruction *inst); // neg, fneg, not
        void genInstUnary_Neg(IR::UnaryInstruction *inst);
        void genInstUnary_FNeg(IR::UnaryInstruction *inst);
        void genInstUnary_Not(IR::UnaryInstruction *inst);

        void genInstCast(IR::CastInstruction *inst);
        void genInstCast_FPtoSI(IR::CastInstruction *inst);
        void genInstCast_SItoFP(IR::CastInstruction *inst);

        void genInstBranch(IR::BranchInstruction *inst);
        void genInstReturn(IR::ReturnInstruction *inst);
        void genInstBranchWithZero(IR::Value *cmpValue, IR::Instruction::CmpOp condition, AsmOperandLabel *trueLabel);  // j[condition]z cmpValue, trueLabel

        void genInstCmp(IR::CmpInstruction *inst);
        void genInstCmp_ICmp(IR::ICmpInstruction *inst);
        void genInstCmp_FCmp(IR::FCmpInstruction *inst);

        void genInstPhi(IR::PhiInstruction *inst);
        void genInstCall(IR::CallInstruction *inst);

        void preGenInstsPhi();
        void sufGenInstsPhi();

        AsmOperandLabel *getBlockOperandLabel();
        AsmInstLabel *getBlockInstLabel();
        IR::BasicBlock *getIRBB();
    };
}